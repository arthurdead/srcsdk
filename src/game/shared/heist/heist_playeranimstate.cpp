#include "cbase.h"
#include "heist_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MOVING_MINIMUM_SPEED 0.5f

#define HEIST_RUN_SPEED 320.0f
#define HEIST_WALK_SPEED 75.0f
#define HEIST_CROUCHWALK_SPEED 110.0f

CHeistPlayerAnimState* CreateHeistPlayerAnimState(CHeistPlayer *pPlayer)
{
	MDLCACHE_CRITICAL_SECTION();

	PlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = HEIST_RUN_SPEED;
	movementData.m_flWalkSpeed = HEIST_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	CHeistPlayerAnimState *pRet = new CHeistPlayerAnimState(pPlayer, movementData);
	pRet->InitHeistAnimState(pPlayer);
	return pRet;
}

CHeistPlayerAnimState::CHeistPlayerAnimState()
{
	m_pHeistPlayer = NULL;
}

CHeistPlayerAnimState::CHeistPlayerAnimState(CBasePlayer *pPlayer, PlayerMovementData_t &movementData)
	: CPlayerAnimState( pPlayer, movementData )
{
	m_pHeistPlayer = NULL;
}

CHeistPlayerAnimState::~CHeistPlayerAnimState()
{
}

void CHeistPlayerAnimState::InitHeistAnimState(CHeistPlayer *pPlayer)
{
	m_pHeistPlayer = pPlayer;
}

void CHeistPlayerAnimState::ClearAnimationState()
{
	BaseClass::ClearAnimationState();
}

Activity CHeistPlayerAnimState::TranslateActivity(Activity actDesired)
{
	Activity translateActivity = actDesired;

	CBaseCombatWeapon *pActiveWep = m_pHeistPlayer->GetActiveWeapon();
	if(pActiveWep) {
		translateActivity = pActiveWep->ActivityOverride(translateActivity, nullptr);
	}

	return translateActivity;
}

void CHeistPlayerAnimState::ComputePoseParam_MoveYaw(CStudioHdr *pStudioHdr)
{
	BaseClass::ComputePoseParam_MoveYaw(pStudioHdr);
}

void CHeistPlayerAnimState::EstimateYaw()
{
	BaseClass::EstimateYaw();
}

void CHeistPlayerAnimState::Update(float eyeYaw, float eyePitch)
{
	VPROF("CHeistPlayerAnimState::Update");

	if(!m_pHeistPlayer) {
		return;
	}

	CStudioHdr *pStudioHdr = m_pHeistPlayer->GetModelPtr();
	if(!pStudioHdr ) {
		return;
	}

	if(!ShouldUpdateAnimState()) {
		ClearAnimationState();
		return;
	}

	m_flEyeYaw = AngleNormalize(eyeYaw);
	m_flEyePitch = AngleNormalize(eyePitch);

	ComputeSequences(pStudioHdr);

	if(SetupPoseParameters(pStudioHdr)) {
		ComputePoseParam_MoveYaw(pStudioHdr);
		ComputePoseParam_AimPitch(pStudioHdr);
		ComputePoseParam_AimYaw(pStudioHdr);
	}

#ifdef CLIENT_DLL 
	if(C_BasePlayer::ShouldDrawLocalPlayer()) {
		m_pHeistPlayer->SetPlaybackRate(1.0f);
	}
#endif
}

void CHeistPlayerAnimState::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	Activity iGestureActivity = ACT_INVALID;

	switch(event)
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE);
		} else {
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE);
		}

		iGestureActivity = ACT_VM_PRIMARYATTACK;
		break;

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		if(!IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD)) {
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData);
		}

		break;

	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
		} else {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
		}

		iGestureActivity = ACT_VM_PRIMARYATTACK;
		break;

	case PLAYERANIMEVENT_ATTACK_PRE:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
		} else {
			iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
		}

		RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false);
		break;

	case PLAYERANIMEVENT_ATTACK_POST:
		RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE);
		break;

	case PLAYERANIMEVENT_RELOAD:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
		} else {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
		}

		break;

	case PLAYERANIMEVENT_RELOAD_LOOP:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
		} else {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
		}

		break;

	case PLAYERANIMEVENT_RELOAD_END:
		if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
		} else {
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
		}

		break;

	default:
		BaseClass::DoAnimationEvent( event, nData );
		break;
	}

#ifdef CLIENT_DLL
	if(iGestureActivity != ACT_INVALID) {
		CBaseCombatWeapon *pWeapon = m_pHeistPlayer->GetActiveWeapon();
		if(pWeapon) {
			pWeapon->SendWeaponAnim(iGestureActivity);
			pWeapon->DoAnimationEvents(pWeapon->GetModelPtr());
		}
	}
#endif
}

bool CHeistPlayerAnimState::HandleSwimming(Activity &idealActivity)
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	return bInWater;
}

bool CHeistPlayerAnimState::HandleMoving(Activity &idealActivity)
{
	return BaseClass::HandleMoving( idealActivity );
}

bool CHeistPlayerAnimState::HandleDucking(Activity &idealActivity)
{
	if(m_pHeistPlayer->GetFlags() & FL_DUCKING) {
		if(GetOuterXYSpeed() < MOVING_MINIMUM_SPEED ) {
			idealActivity = ACT_MP_CROUCH_IDLE;
		} else {
			idealActivity = ACT_MP_CROUCHWALK;
		}

		return true;
	}

	return false;
}

bool CHeistPlayerAnimState::HandleJumping(Activity &idealActivity)
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	if(m_bJumping) {
		if(m_bFirstJumpFrame) {
			m_bFirstJumpFrame = false;
			RestartMainSequence();
		}

		if(m_pHeistPlayer->GetWaterLevel() >= WL_Waist) {
			m_bJumping = false;
			RestartMainSequence();
		} else if(gpGlobals->curtime - m_flJumpStartTime > 0.2f) {
			if(m_pHeistPlayer->GetFlags() & FL_ONGROUND) {
				m_bJumping = false;
				RestartMainSequence();

				RestartGesture(GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND);
			}
		}

		if(m_bJumping) {
			if(gpGlobals->curtime - m_flJumpStartTime > 0.5) {
				idealActivity = ACT_MP_JUMP_FLOAT;
			} else {
				idealActivity = ACT_MP_JUMP_START;
			}
		}
	}

	if(m_bJumping) {
		return true;
	}

	return false;
}

bool CHeistPlayerAnimState::SetupPoseParameters(CStudioHdr *pStudioHdr)
{
	if (m_bPoseParameterInit) {
		return true;
	}

	if(!pStudioHdr) {
		return false;
	}

	m_PoseParameterData.m_iMoveX = m_pHeistPlayer->LookupPoseParameter(pStudioHdr, "move_x");
	m_PoseParameterData.m_iMoveY = m_pHeistPlayer->LookupPoseParameter(pStudioHdr, "move_y");
	if((m_PoseParameterData.m_iMoveX < 0) || (m_PoseParameterData.m_iMoveY < 0)) {
		return false;
	}

	m_PoseParameterData.m_iAimPitch = m_pHeistPlayer->LookupPoseParameter(pStudioHdr, "aim_pitch");
	if(m_PoseParameterData.m_iAimPitch < 0) {
		return false;
	}

	m_PoseParameterData.m_iAimYaw = m_pHeistPlayer->LookupPoseParameter(pStudioHdr, "aim_yaw");
	if(m_PoseParameterData.m_iAimYaw < 0) {
		return false;
	}

	m_bPoseParameterInit = true;

	return true;
}

void CHeistPlayerAnimState::ComputePoseParam_AimPitch(CStudioHdr *pStudioHdr)
{
	float flAimPitch = m_flEyePitch;

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch );
	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

void CHeistPlayerAnimState::ComputePoseParam_AimYaw(CStudioHdr *pStudioHdr)
{
	Vector vecVelocity;
	GetOuterAbsVelocity(vecVelocity);

	bool bMoving = (vecVelocity.Length() > 1.0f) ? true : false;

	if(bMoving || m_bForceAimYaw) {
		m_flGoalFeetYaw = m_flEyeYaw;
	} else {
		if(m_PoseParameterData.m_flLastAimTurnTime <= 0.0f) {
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		} else {
			float flYawDelta = AngleNormalize(m_flGoalFeetYaw - m_flEyeYaw);

			if(fabs( flYawDelta ) > 45.0f) {
				float flSide = (flYawDelta > 0.0f) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += (45.0f * flSide);
			}
		}
	}

	m_flGoalFeetYaw = AngleNormalize(m_flGoalFeetYaw);

	if(m_flGoalFeetYaw != m_flCurrentFeetYaw) {
		if(m_bForceAimYaw) {
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		} else {
			ConvergeYawAngles(m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw);
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	m_angRender[YAW] = m_flCurrentFeetYaw;

	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize(flAimYaw);

	m_pHeistPlayer->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw);
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = m_pHeistPlayer->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	m_pHeistPlayer->SetAbsAngles( angle );
#endif
}

float CHeistPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	CStudioHdr *pStudioHdr = m_pHeistPlayer->GetModelPtr();

	if(pStudioHdr == NULL) {
		return 1.0f;
	}

	float prevX = m_pHeistPlayer->GetPoseParameter( m_PoseParameterData.m_iMoveX );
	float prevY = m_pHeistPlayer->GetPoseParameter( m_PoseParameterData.m_iMoveY );

	float d = sqrt( prevX * prevX + prevY * prevY );

	float newY, newX;
	if(d == 0.0) { 
		newX = 1.0;
		newY = 0.0;
	} else {
		newX = prevX / d;
		newY = prevY / d;
	}

	m_pHeistPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, newX );
	m_pHeistPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, newY );

	float speed = m_pHeistPlayer->GetSequenceGroundSpeed( m_pHeistPlayer->GetSequence() );

	m_pHeistPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, prevX );
	m_pHeistPlayer->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, prevY );

	return speed;
}

