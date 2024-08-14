#include "cbase.h"
#include "c_heist_player.h"
#include "view.h"
#include "takedamageinfo.h"
#include "heist_gamerules.h"
#include "beam_flags.h"
#include "iviewrender_beams.h"
#include "r_efx.h"
#include "dlight.h"
#include "in_buttons.h"
#include "c_basetempentity.h"
#include "prediction.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CHeistPlayer
	#undef CHeistPlayer
#endif

#define CYCLELATCH_TOLERANCE 0.15f

extern void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

LINK_ENTITY_TO_CLASS(player, C_HeistPlayer);

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistLocalPlayerExclusive)
	RecvPropVector(RECVINFO_NAME(m_vecNetworkOrigin, m_vecOrigin)),
	RecvPropFloat(RECVINFO(m_angEyeAngles[0])),
	//RecvPropFloat(RECVINFO(m_angEyeAngles[1])),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistNonLocalPlayerExclusive)
	RecvPropVector(RECVINFO_NAME(m_vecNetworkOrigin, m_vecOrigin)),
	RecvPropFloat(RECVINFO(m_angEyeAngles[0])),
	RecvPropFloat(RECVINFO(m_angEyeAngles[1])),

	RecvPropInt(RECVINFO(m_cycleLatch), 0, &C_HeistPlayer::RecvProxy_CycleLatch),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_HeistPlayer, DT_Heist_Player, CHeistPlayer)
	RecvPropDataTable("heistlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistLocalPlayerExclusive)),
	RecvPropDataTable("heistnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistNonLocalPlayerExclusive)),
	RecvPropInt(RECVINFO(m_iSpawnInterpCounter)),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_HeistPlayer)
	DEFINE_PRED_FIELD(m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE|FTYPEDESC_PRIVATE|FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE|FTYPEDESC_PRIVATE|FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE|FTYPEDESC_PRIVATE|FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_ARRAY_TOL(m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE|FTYPEDESC_PRIVATE, 0.02f),
	DEFINE_PRED_FIELD(m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE|FTYPEDESC_PRIVATE|FTYPEDESC_NOERRORCHECK),
END_PREDICTION_DATA()

C_HeistPlayer::C_HeistPlayer()
	: m_iv_angEyeAngles( "C_HeistPlayer::m_iv_angEyeAngles" )
{
	m_iIDEntIndex = 0;
	m_iSpawnInterpCounterCache = 0;

	AddVar(&m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR);

	m_PlayerAnimState = CreateHeistPlayerAnimState( this );

	m_pFlashlightBeam = NULL;

	m_flServerCycle = -1.0f;
}

C_HeistPlayer::~C_HeistPlayer()
{
	ReleaseFlashlight();
	m_PlayerAnimState->Release();
}

int C_HeistPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

void C_HeistPlayer::UpdateIDTarget()
{
	if(!IsLocalPlayer()) {
		return;
	}

	m_iIDEntIndex = 0;

	if(GetObserverMode() == OBS_MODE_CHASE ||
		GetObserverMode() == OBS_MODE_DEATHCAM) {
		return;
	}

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA(MainViewOrigin(), 1500, MainViewForward(), vecEnd);
	VectorMA(MainViewOrigin(), 10, MainViewForward(), vecStart);
	UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	if(!tr.startsolid && tr.DidHitNonWorldEntity()) {
		C_BaseEntity *pEntity = tr.m_pEnt;

		if(pEntity && (pEntity != this)) {
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

void C_HeistPlayer::TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;

	float flDistance = 0.0f;
	if(info.GetAttacker()) {
		flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
	}

	if(m_takedamage) {
		AddMultiDamage(info, this);

		int blood = BloodColor();

		CBaseEntity *pAttacker = info.GetAttacker();

		if(pAttacker) {
			if(HeistGamerules()->IsTeamplay() && pAttacker->InSameTeam(this) == true) {
				return;
			}
		}

		if(blood != DONT_BLEED) {
			SpawnBlood(vecOrigin, vecDir, blood, flDistance);
			TraceBleed(flDistance, vecDir, ptr, info.GetDamageType());
		}
	}
}

C_HeistPlayer *C_HeistPlayer::GetLocalHeistPlayer()
{
	return (C_HeistPlayer *)C_BasePlayer::GetLocalPlayer();
}

void C_HeistPlayer::Initialize()
{
	m_headYawPoseParam = LookupPoseParameter("head_yaw");
	GetPoseParameterRange(m_headYawPoseParam, m_headYawMin, m_headYawMax);

	m_headPitchPoseParam = LookupPoseParameter("head_pitch");
	GetPoseParameterRange(m_headPitchPoseParam, m_headPitchMin, m_headPitchMax);

	CStudioHdr *hdr = GetModelPtr();
	for(int i = 0; i < hdr->GetNumPoseParameters(); ++i) {
		SetPoseParameter( hdr, i, 0.0 );
	}
}

CStudioHdr *C_HeistPlayer::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	Initialize();

	if(m_PlayerAnimState) {
		m_PlayerAnimState->OnNewModel();
	}

	return hdr;
}

void C_HeistPlayer::UpdateLookAt()
{
	if(m_headYawPoseParam < 0 || m_headPitchPoseParam < 0) {
		return;
	}

	m_viewtarget = m_vLookAtTarget;

	QAngle desiredAngles;
	Vector to = m_vLookAtTarget - EyePosition();
	VectorAngles(to, desiredAngles);

	QAngle bodyAngles(0, 0, 0);
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	float desired = AngleNormalize(desiredAngles[YAW] - bodyAngles[YAW]);
	desired = clamp( desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle(desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime);

	m_flCurrentHeadYaw = AngleNormalize(m_flCurrentHeadYaw - flBodyYawDiff);
	desired = clamp(desired, m_headYawMin, m_headYawMax);

	SetPoseParameter(m_headYawPoseParam, m_flCurrentHeadYaw);

	desired = AngleNormalize(desiredAngles[PITCH]);
	desired = clamp(desired, m_headPitchMin, m_headPitchMax);
	
	m_flCurrentHeadPitch = ApproachAngle(desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime);
	m_flCurrentHeadPitch = AngleNormalize(m_flCurrentHeadPitch);
	SetPoseParameter(m_headPitchPoseParam, m_flCurrentHeadPitch);
}

void C_HeistPlayer::ClientThink()
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors(GetLocalAngles(), &vForward);

	for(int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient) {
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if(!pEnt || !pEnt->IsPlayer()) {
			continue;
		}

		if(pEnt->entindex() == entindex()) {
			continue;
		}

		Vector vTargetOrigin = pEnt->GetAbsOrigin();
		Vector vMyOrigin =  GetAbsOrigin();

		Vector vDir = vTargetOrigin - vMyOrigin;
		
		if(vDir.Length() > 128) {
			continue;
		}

		VectorNormalize(vDir);

		if(DotProduct(vForward, vDir) < 0.0f) {
			continue;
		}

		m_vLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if(bFoundViewTarget == false) {
		m_vLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	UpdateIDTarget();
}

int C_HeistPlayer::DrawModel(int flags)
{
	if(!m_bReadyToDraw) {
		return 0;
	}

    return BaseClass::DrawModel(flags);
}

bool C_HeistPlayer::ShouldReceiveProjectedTextures(int flags)
{
	Assert(flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK);

	if(IsEffectActive(EF_NODRAW)) {
		return false;
	}

	if(flags & SHADOW_FLAGS_FLASHLIGHT) {
		return true;
	}

	return BaseClass::ShouldReceiveProjectedTextures(flags);
}

void C_HeistPlayer::DoImpactEffect(trace_t &tr, int nDamageType)
{
	if(GetActiveWeapon()) {
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

void C_HeistPlayer::PreThink()
{
	BaseClass::PreThink();
}

const QAngle &C_HeistPlayer::EyeAngles()
{
	if(IsLocalPlayer()) {
		return BaseClass::EyeAngles();
	} else {
		return m_angEyeAngles;
	}
}

void C_HeistPlayer::AddEntity()
{
	BaseClass::AddEntity();

	SetLocalAnglesDim(X_INDEX, 0);

	if(this != C_BasePlayer::GetLocalPlayer()) {
		if(IsEffectActive(EF_DIMLIGHT)) {
			int iAttachment = LookupAttachment( "anim_attachment_RH" );
			if(iAttachment < 0) {
				return;
			}

			Vector vecOrigin;
			QAngle eyeAngles = m_angEyeAngles;
			GetAttachment(iAttachment, vecOrigin, eyeAngles);

			Vector vForward;
			AngleVectors(eyeAngles, &vForward);

			trace_t tr;
			UTIL_TraceLine(vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

			if(!m_pFlashlightBeam) {
				BeamInfo_t beamInfo;
				beamInfo.m_nType = TE_BEAMPOINTS;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_pszModelName = "sprites/glow01.vmt";
				beamInfo.m_pszHaloName = "sprites/glow01.vmt";
				beamInfo.m_flHaloScale = 3.0;
				beamInfo.m_flWidth = 8.0f;
				beamInfo.m_flEndWidth = 35.0f;
				beamInfo.m_flFadeLength = 300.0f;
				beamInfo.m_flAmplitude = 0;
				beamInfo.m_flBrightness = 60.0;
				beamInfo.m_flSpeed = 0.0f;
				beamInfo.m_nStartFrame = 0.0;
				beamInfo.m_flFrameRate = 0.0;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beamInfo.m_nSegments = 8;
				beamInfo.m_bRenderable = true;
				beamInfo.m_flLife = 0.5;
				beamInfo.m_nFlags = FBEAM_FOREVER|FBEAM_ONLYNOISEONCE|FBEAM_NOTILE|FBEAM_HALOBEAM;
				m_pFlashlightBeam = beams->CreateBeamPoints(beamInfo);
			}

			if(m_pFlashlightBeam) {
				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beams->UpdateBeamInfo(m_pFlashlightBeam, beamInfo);

				dlight_t *el = effects->CL_AllocDlight(0);
				el->origin = tr.endpos;
				el->radius = 50; 
				el->color.r = 200;
				el->color.g = 200;
				el->color.b = 200;
				el->die = gpGlobals->curtime + 0.1;
			}
		} else if(m_pFlashlightBeam) {
			ReleaseFlashlight();
		}
	}
}

ShadowType_t C_HeistPlayer::ShadowCastType()
{
	if(!IsVisible()) {
		return SHADOWS_NONE;
	}

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

const QAngle &C_HeistPlayer::GetRenderAngles()
{
	if(IsRagdoll()) {
		return vec3_angle;
	} else {
		return m_PlayerAnimState->GetRenderAngles();
	}
}

bool C_HeistPlayer::ShouldDraw()
{
	if(!IsAlive()) {
		return false;
	}

	//if(GetTeamNumber() == TEAM_SPECTATOR)
	//return false;

	if(IsLocalPlayer() && IsRagdoll()) {
		return true;
	}

	if(IsRagdoll()) {
		return false;
	}

	return BaseClass::ShouldDraw();
}

void C_HeistPlayer::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	if(state == SHOULDTRANSMIT_END) {
		if(m_pFlashlightBeam != NULL) {
			ReleaseFlashlight();
		}
	}

	BaseClass::NotifyShouldTransmit( state );
}

void C_HeistPlayer::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if(type == DATA_UPDATE_CREATED) {
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}

	UpdateVisibility();
}

void C_HeistPlayer::PostDataUpdate(DataUpdateType_t updateType)
{
	if(m_iSpawnInterpCounter != m_iSpawnInterpCounterCache) {
		MoveToLastReceivedPosition(true);
		ResetLatched();
		m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
	}

	BaseClass::PostDataUpdate( updateType );
}

void C_HeistPlayer::RecvProxy_CycleLatch(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	C_HeistPlayer* pPlayer = static_cast<C_HeistPlayer *>(pStruct);

	float flServerCycle = (float)pData->m_Value.m_Int / 16.0f;
	float flCurCycle = pPlayer->GetCycle();

	if(fabs(flCurCycle - flServerCycle) > CYCLELATCH_TOLERANCE) {
		pPlayer->SetServerIntendedCycle(flServerCycle);
	}
}

void C_HeistPlayer::ReleaseFlashlight()
{
	if(m_pFlashlightBeam) {
		m_pFlashlightBeam->flags = 0;
		m_pFlashlightBeam->die = gpGlobals->curtime - 1;

		m_pFlashlightBeam = NULL;
	}
}

float C_HeistPlayer::GetFOV()
{
	float flFOVOffset = C_BasePlayer::GetFOV();

	int min_fov = GetMinFOV();

	flFOVOffset = MAX(min_fov, flFOVOffset);

	return flFOVOffset;
}

Vector C_HeistPlayer::GetAutoaimVector(float flDelta)
{
	Vector forward;
	AngleVectors(EyeAngles() + m_Local.m_vecPunchAngle, &forward);
	return forward;
}

void C_HeistPlayer::ItemPreFrame()
{
	if(GetFlags() & FL_FROZEN) {
		return;
	}

	if(m_nButtons & IN_ZOOM) {
		m_nButtons &= ~(IN_ATTACK|IN_ATTACK2);
	}

	BaseClass::ItemPreFrame();

}

void C_HeistPlayer::ItemPostFrame()
{
	if(GetFlags() & FL_FROZEN) {
		return;
	}

	BaseClass::ItemPostFrame();
}

void C_HeistPlayer::CalcView(Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov)
{
	if(m_lifeState != LIFE_ALIVE && !IsObserver()) {
		Vector origin = EyePosition();

		BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);

		eyeOrigin = origin;

		Vector vForward; 
		AngleVectors(eyeAngles, &vForward);

		VectorNormalize(vForward);
		VectorMA(origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin);

		Vector WALL_MIN(-WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET);
		Vector WALL_MAX(WALL_OFFSET, WALL_OFFSET, WALL_OFFSET);

		trace_t trace;
		C_BaseEntity::PushEnableAbsRecomputations(false);
		UTIL_TraceHull(origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace);
		C_BaseEntity::PopEnableAbsRecomputations();

		if(trace.fraction < 1.0) {
			eyeOrigin = trace.endpos;
		}

		return;
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

void C_HeistPlayer::UpdateClientSideAnimation()
{
	m_PlayerAnimState->Update(EyeAngles()[YAW], EyeAngles()[PITCH]);

	BaseClass::UpdateClientSideAnimation();
}

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEPlayerAnimEvent, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate(DataUpdateType_t updateType)
	{
		C_HeistPlayer *pPlayer = dynamic_cast<C_HeistPlayer *>(m_hPlayer.Get());
		if(pPlayer && !pPlayer->IsDormant()) {
			pPlayer->DoAnimationEvent((PlayerAnimEvent_t)m_iEvent.Get(), m_nData);
		}
	}

public:
	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_CLIENTCLASS_EVENT(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent);

BEGIN_RECV_TABLE_NOBASE(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent)
	RecvPropEHandle(RECVINFO(m_hPlayer)),
	RecvPropInt(RECVINFO(m_iEvent)),
	RecvPropInt(RECVINFO(m_nData))
END_RECV_TABLE()

void C_HeistPlayer::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	if(IsLocalPlayer()) {
		if((prediction->InPrediction() && !prediction->IsFirstTimePredicted())) {
			return;
		}
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent(event, nData);
}

void C_HeistPlayer::CalculateIKLocks(float currentTime)
{
	if(!m_pIk) {
		return;
	}

	int targetCount = m_pIk->m_target.Count();
	if(targetCount == 0) {
		return;
	}

	SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
	partition->SuppressLists(PARTITION_ALL_CLIENT_EDICTS, false);
	CBaseEntity::PushEnableAbsRecomputations(false);

	for(int i = 0; i < targetCount; ++i) {
		trace_t trace;
		CIKTarget *pTarget = &m_pIk->m_target[i];

		if(!pTarget->IsActive()) {
			continue;
		}

		switch(pTarget->type)
		{
		case IK_GROUND:
			pTarget->SetPos(Vector(pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z));
			pTarget->SetAngles(GetRenderAngles());

			break;

		case IK_ATTACHMENT: {
			C_BaseEntity *pEntity = NULL;
			float flDist = pTarget->est.radius;

			for(CEntitySphereQuery sphere( pTarget->est.pos, 64 ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity()) {
				C_BaseAnimating *pAnim = pEntity->GetBaseAnimating();
				if(!pAnim) {
					continue;
				}

				int iAttachment = pAnim->LookupAttachment(pTarget->offset.pAttachmentName);
				if(iAttachment <= 0) {
					continue;
				}

				Vector origin;
				QAngle angles;
				pAnim->GetAttachment(iAttachment, origin, angles);

				float d = (pTarget->est.pos - origin).Length();
				if(d >= flDist) {
					continue;
				}

				flDist = d;
				pTarget->SetPos(origin);
				pTarget->SetAngles(angles);
			}

			if(flDist >= pTarget->est.radius) {
				pTarget->IKFailed( );
			}
		} break;
		}
	}

	CBaseEntity::PopEnableAbsRecomputations();
	partition->SuppressLists(curSuppressed, true);
}
