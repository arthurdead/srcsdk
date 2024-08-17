#include "cbase.h"
#include "heist_player.h"
#include "heist_gamerules.h"
#include "ilagcompensationmanager.h"
#include "in_buttons.h"
#include "team.h"
#include "gamevars_shared.h"
#include "predicted_viewmodel.h"
#include "gamestats.h"
#include "tier0/vprof.h"
#include "bone_setup.h"
#include "filesystem.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CYCLELATCH_UPDATE_INTERVAL 0.2f

#define HEISTPLAYER_PHYSDAMAGE_SCALE 4.0f

#define HEIST_COMMAND_MAX_RATE 0.3f

ConVar sv_SecobMod__increment_killed("sv_SecobMod__increment_killed", "0", 0, "The level of Increment Killed as a convar.");

extern ConVar sv_maxunlag;
extern int gEvilImpulse101;
extern ConVar flashlight;

LINK_ENTITY_TO_CLASS(player, CHeistPlayer);

extern void SendProxy_Origin(const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);

void *SendProxy_SendNonLocalDataTable(const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID)
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient(objectID - 1);
	return (void *)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER(SendProxy_SendNonLocalDataTable);

BEGIN_SEND_TABLE_NOBASE(CHeistPlayer, DT_HeistLocalPlayerExclusive)
	SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
	SendPropFloat(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f),
	//SendPropAngle(SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE(CHeistPlayer, DT_HeistNonLocalPlayerExclusive)
	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
	SendPropFloat(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f),
	SendPropAngle(SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_cycleLatch), 4, SPROP_UNSIGNED),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CHeistPlayer, DT_Heist_Player)
	SendPropExclude("DT_BaseAnimating", "m_flPoseParameter"),
	SendPropExclude("DT_BaseAnimating", "m_flPlaybackRate"),
	SendPropExclude("DT_BaseAnimating", "m_nSequence"),
	SendPropExclude("DT_BaseEntity", "m_angRotation"),
	SendPropExclude("DT_BaseAnimatingOverlay", "overlay_vars"),

	SendPropExclude("DT_BaseEntity", "m_vecOrigin"),

	SendPropExclude("DT_ServerAnimationData", "m_flCycle"),
	SendPropExclude("DT_AnimTimeMustBeFirst", "m_flAnimTime"),

	SendPropExclude("DT_BaseFlex", "m_flexWeight"),
	SendPropExclude("DT_BaseFlex", "m_blinktoggle"),
	SendPropExclude("DT_BaseFlex", "m_viewtarget"),

	SendPropDataTable("heistlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HeistLocalPlayerExclusive), SendProxy_SendLocalDataTable),
	SendPropDataTable("heistnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HeistNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable),

	SendPropInt(SENDINFO(m_iSpawnInterpCounter), 4),
END_SEND_TABLE()

BEGIN_DATADESC(CHeistPlayer)
END_DATADESC()

CHeistPlayer::CHeistPlayer()
{
	m_PlayerAnimState = CreateHeistPlayerAnimState(this);
	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_iSpawnInterpCounter = 0;

	m_bEnterObserver = false;
	m_bReady = false;

	m_cycleLatch = 0;
	m_cycleLatchTimer.Invalidate();

	BaseClass::ChangeTeam( 0 );
}

CHeistPlayer::~CHeistPlayer()
{
	m_PlayerAnimState->Release();
}

void CHeistPlayer::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
}

void CHeistPlayer::Precache()
{
	BaseClass::Precache();

	PrecacheModel("models/barney.mdl");
}

static bool TestEntityPosition(CBasePlayer *pPlayer)
{
	trace_t	trace;
	UTIL_TraceEntity(pPlayer, pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), MASK_PLAYERSOLID, &trace);
	return (trace.startsolid == 0);
}

static int FindPassableSpace(CBasePlayer *pPlayer, const Vector &direction, float step, Vector &oldorigin)
{
	for(int i = 0; i < 100; i++ ) {
		Vector origin = pPlayer->GetAbsOrigin();
		VectorMA(origin, step, direction, origin);

		pPlayer->SetAbsOrigin(origin);

		if(TestEntityPosition(pPlayer)) {
			VectorCopy( pPlayer->GetAbsOrigin(), oldorigin );
			return 1;
		}
	}

	return 0;
}

void CHeistPlayer::Spawn(void)
{
	if(m_bTransition) {
		if(m_bTransitionTeleported) {
			g_pGameRules->GetPlayerSpawnSpot(this);
		}

		m_bTransition = false;
		m_bTransitionTeleported = false;
		return;
	}

	BaseClass::Spawn();

	if(!IsObserver()) {
		SetParent(NULL);
		SetMoveType(MOVETYPE_NOCLIP);
		AddEFlags(EFL_NOCLIP_ACTIVE);

		CPlayerState *pl2 = PlayerData();
		Assert( pl2 );

		RemoveEFlags(EFL_NOCLIP_ACTIVE);
		SetMoveType(MOVETYPE_WALK);

		Vector oldorigin = GetAbsOrigin();
		if(!TestEntityPosition(this)) {
			Vector forward, right, up;

			AngleVectors ( pl2->v_angle, &forward, &right, &up);

			// Try to move into the world
			if(!FindPassableSpace(this, forward, 1, oldorigin)) {
				if(!FindPassableSpace(this, right, 1, oldorigin)) {
					if(!FindPassableSpace(this, right, -1, oldorigin)) {
						if(!FindPassableSpace(this, up, 1, oldorigin)) {
							if(!FindPassableSpace(this, up, -1, oldorigin)) {
								if(!FindPassableSpace(this, forward, -1, oldorigin)) {
									SetCollisionBounds(VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX);
									AddFlag(FL_DUCKING);
									m_Local.m_bDucked = true;
									m_Local.m_bDucking = false;
									CommitSuicide();
								}
							}
						}
					}
				}
			}

			SetAbsOrigin( oldorigin );
		}

		int MovedYet = 0;

	LoopSpot:
		CBaseEntity *ent = NULL;
		for(CEntitySphereQuery sphere( GetAbsOrigin(), 0.1 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity()) {
			if(ent->IsPlayer() && ent != this) {
				AddFlag(FL_ONGROUND);

				if(MovedYet == 0) {
					UTIL_ClientPrintAll( HUD_PRINTCENTER, "#BLOCKING_SPAWN");
				}

				++MovedYet;

				if(MovedYet >= 6) {
					ent->TakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 3000, DMG_GENERIC));
				} else {
					goto LoopSpot;
				}
			}

			MovedYet = 0;
		}

		pl.deadflag = false;
		RemoveSolidFlags(FSOLID_NOT_SOLID);

		RemoveEffects(EF_NODRAW);
	}

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND);

	m_impactEnergyScale = HEISTPLAYER_PHYSDAMAGE_SCALE;

	if(HeistGamerules()->IsIntermission()) {
		AddFlag(FL_FROZEN);
	} else {
		RemoveFlag(FL_FROZEN);
	}

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);

	m_bReady = false;

	m_cycleLatchTimer.Start(CYCLELATCH_UPDATE_INTERVAL);

	DoAnimationEvent(PLAYERANIMEVENT_SPAWN);

	SetModel("models/barney.mdl");
}

ConVar sv_heist_dev_players_disguised("sv_heist_dev_players_disguised", "1");

Class_T CHeistPlayer::Classify()
{
	return sv_heist_dev_players_disguised.GetBool() ? CLASS_HEISTER_DISGUISED : CLASS_HEISTER;
}

void CHeistPlayer::PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize)
{
	BaseClass::PickupObject(pObject, bLimitMassAndSize);
}

bool CHeistPlayer::Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	return bRet;
}

void CHeistPlayer::PreThink()
{
	BaseClass::PreThink();
	State_PreThink();

	m_vecTotalBulletForce = vec3_origin;
}

void CHeistPlayer::PostThink()
{
	BaseClass::PostThink();

	if(GetFlags() & FL_DUCKING) {
		SetCollisionBounds(VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX);
	}

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles(angles);

	m_angEyeAngles = EyeAngles();
	m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);

	if(IsAlive() && m_cycleLatchTimer.IsElapsed()) {
		m_cycleLatchTimer.Start(CYCLELATCH_UPDATE_INTERVAL);
		m_cycleLatch.GetForModify() = 16 * GetCycle();
	}
}

void CHeistPlayer::PlayerDeathThink()
{
	if(!IsObserver()) {
		BaseClass::PlayerDeathThink();
	}
}

void CHeistPlayer::FireBullets(const FireBulletsInfo_t &info)
{
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	FireBulletsInfo_t modinfo = info;

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );

	lagcompensation->FinishLagCompensation( this );
}

void CHeistPlayer::NoteWeaponFired()
{
	Assert(m_pCurrentCommand);

	if(m_pCurrentCommand) {
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CHeistPlayer::WantsLagCompensationOnEntity(const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const
{
	if(!(pCmd->buttons & IN_ATTACK) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5)) {
		return false;
	}

	return BaseClass::WantsLagCompensationOnEntity(pEntity, pCmd, pEntityTransmitBits); 
}

bool CHeistPlayer::BumpWeapon(CBaseCombatWeapon *pWeapon)
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	if(!IsAllowedToPickupWeapons()) {
		return false;
	}

	if(pOwner || !Weapon_CanUse(pWeapon) || !g_pGameRules->CanHavePlayerItem(this, pWeapon)) {
		if(gEvilImpulse101) {
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	if(!pWeapon->FVisible(this, MASK_SOLID) && !(GetFlags() & FL_NOTARGET)) {
		return false;
	}

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());
	if(bOwnsWeaponAlready == true) {
		if(Weapon_EquipAmmoOnly(pWeapon)) {
			pWeapon->CheckRespawn();

			UTIL_Remove( pWeapon );
			return true;
		} else {
			return false;
		}
	}

	pWeapon->CheckRespawn();

	pWeapon->AddSolidFlags(FSOLID_NOT_SOLID);
	pWeapon->AddEffects(EF_NODRAW);

	Weapon_Equip( pWeapon );

	return true;
}

void CHeistPlayer::ChangeTeam( int iTeam )
{
	bool bKill = false;

	if(HeistGamerules()->IsTeamplay() != true && iTeam != TEAM_SPECTATOR) {
		iTeam = TEAM_UNASSIGNED;
	}

	if(HeistGamerules()->IsTeamplay() == true) {
		if(iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED) {
			bKill = true;
		}
	}

	BaseClass::ChangeTeam( iTeam );

	if(iTeam == TEAM_SPECTATOR) {
		RemoveAllItems(true);

		State_Transition(STATE_OBSERVER_MODE);
	}

	if(bKill == true) {
		CommitSuicide();
	}
}

bool CHeistPlayer::HandleCommand_JoinTeam(int team)
{
	if(!GetGlobalTeam(team) || team == 0) {
		Warning("HandleCommand_JoinTeam( %d ) - invalid team index.\n", team);
		return false;
	}

	if(team == TEAM_SPECTATOR) {
		if(!mp_allowspectators.GetInt()) {
			ClientPrint(this, HUD_PRINTCENTER, "#Cannot_Be_Spectator");
			return false;
		}

		if(GetTeamNumber() != TEAM_UNASSIGNED && !IsDead()) {
			m_fNextSuicideTime = gpGlobals->curtime;

			CommitSuicide();

			IncrementFragCount(1);
		}

		ChangeTeam(TEAM_SPECTATOR);
		return true;
	} else {
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	ChangeTeam(team);

	return true;
}

bool CHeistPlayer::ClientCommand(const CCommand &args)
{
	if(FStrEq(args[0], "spectate")) {
		if(ShouldRunRateLimitedCommand(args)) {
			HandleCommand_JoinTeam(TEAM_SPECTATOR);
		}

		return true;
	} else if(FStrEq(args[0], "jointeam")) {
		if(args.ArgC() < 2) {
			Warning( "Player sent bad jointeam syntax\n" );
		}

		if(ShouldRunRateLimitedCommand(args)) {
			int iTeam = atoi(args[1]);
			HandleCommand_JoinTeam(iTeam);
		}

		return true;
	} else if(FStrEq(args[0], "joingame")) {
		return true;
	}

	return BaseClass::ClientCommand( args );
}

#ifdef _WIN32
#pragma warning(disable: 4065)
#endif

void CHeistPlayer::CheatImpulseCommands(int iImpulse)
{
	switch(iImpulse)
	{
	default:
		BaseClass::CheatImpulseCommands(iImpulse);
	}
}

bool CHeistPlayer::ShouldRunRateLimitedCommand(const CCommand &args)
{
	int i = m_RateLimitLastCommandTimes.Find(args[0]);
	if(i == m_RateLimitLastCommandTimes.InvalidIndex()) {
		m_RateLimitLastCommandTimes.Insert(args[0], gpGlobals->curtime);
		return true;
	} else if((gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HEIST_COMMAND_MAX_RATE) {
		return false;
	} else {
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHeistPlayer::CreateViewModel(int index /*=0*/)
{
	Assert(index >= 0 && index < MAX_VIEWMODELS);

	if(GetViewModel(index)) {
		return;
	}

	CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName("predicted_viewmodel");
	if(vm) {
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		vm->SetIndex(index);
		DispatchSpawn(vm);
		vm->FollowEntity(this, false);
		m_hViewModel.Set(index, vm);
	}
}

int CHeistPlayer::FlashlightIsOn()
{
	return IsEffectActive(EF_DIMLIGHT);
}

void CHeistPlayer::FlashlightTurnOn()
{
	if(flashlight.GetInt() > 0 && IsAlive()) {
		AddEffects(EF_DIMLIGHT);
		EmitSound("HL2Player.FlashlightOn");
	}
}

void CHeistPlayer::FlashlightTurnOff( void )
{
	RemoveEffects(EF_DIMLIGHT);

	if(IsAlive()) {
		EmitSound("HL2Player.FlashlightOff");
	}
}

void CHeistPlayer::Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
}

void CHeistPlayer::Event_Killed(const CTakeDamageInfo &info)
{
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce(m_vecTotalBulletForce);

	sv_SecobMod__increment_killed.SetValue(sv_SecobMod__increment_killed.GetInt()+1);

	BaseClass::Event_Killed(subinfo);

	CBaseEntity *pAttacker = info.GetAttacker();
	if(pAttacker) {
		int iScoreToAdd = 1;
		if(pAttacker == this) {
			iScoreToAdd = -1;
		}

		GetGlobalTeam(pAttacker->GetTeamNumber())->AddScore(iScoreToAdd);
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects(EF_NODRAW);
}

int CHeistPlayer::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	m_vecTotalBulletForce += inputInfo.GetDamageForce();

	gamestats->Event_PlayerDamage(this, inputInfo);

	return BaseClass::OnTakeDamage(inputInfo);
}

void CHeistPlayer::DeathSound(const CTakeDamageInfo &info)
{
	char szStepSound[128];
	Q_snprintf(szStepSound, sizeof(szStepSound), "Player.Die");

	const char *pModelName = STRING(GetModelName());

	CSoundParameters params;
	if(GetParametersForSound(szStepSound, params, pModelName) == false) {
		return;
	}

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound(filter, entindex(), ep);
}

CBaseEntity* CHeistPlayer::EntSelectSpawnPoint()
{
	return BaseClass::EntSelectSpawnPoint();
}

void CHeistPlayer::Reset()
{	
	ResetDeathCount();
	ResetFragCount();
}

bool CHeistPlayer::IsReady()
{
	return m_bReady;
}

void CHeistPlayer::SetReady(bool bReady)
{
	m_bReady = bReady;
}

void CHeistPlayer::CheckChatText(char *p, int bufsize)
{
	char *buf = new char[bufsize];
	int pos = 0;

	for(char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize-1; pSrc++) {
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	Q_strncpy( p, buf, bufsize );

	delete[] buf;

	const char *pReadyCheck = p;

	HeistGamerules()->CheckChatForReadySignal(this, pReadyCheck);
}

void CHeistPlayer::State_Transition(HeistPlayerState newState)
{
	State_Leave();
	State_Enter(newState);
}

void CHeistPlayer::State_Enter(HeistPlayerState newState)
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo(newState);

	if(m_pCurStateInfo && m_pCurStateInfo->pfnEnterState) {
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

void CHeistPlayer::State_Leave()
{
	if(m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState) {
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CHeistPlayer::State_PreThink()
{
	if(m_pCurStateInfo && m_pCurStateInfo->pfnPreThink) {
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}

CHeistPlayerStateInfo *CHeistPlayer::State_LookupInfo(HeistPlayerState state)
{
	static CHeistPlayerStateInfo playerStateInfos[]{
		{STATE_ACTIVE, "STATE_ACTIVE", &CHeistPlayer::State_Enter_ACTIVE, NULL, &CHeistPlayer::State_PreThink_ACTIVE},
		{STATE_OBSERVER_MODE, "STATE_OBSERVER_MODE", &CHeistPlayer::State_Enter_OBSERVER_MODE, NULL, &CHeistPlayer::State_PreThink_OBSERVER_MODE}
	};

	for(int i = 0; i < ARRAYSIZE(playerStateInfos); ++i) {
		if(playerStateInfos[i].m_iPlayerState == state) {
			return &playerStateInfos[i];
		}
	}

	return NULL;
}

bool CHeistPlayer::StartObserverMode(int mode)
{
	if(m_bEnterObserver == true) {
		VPhysicsDestroyObject();
		return BaseClass::StartObserverMode( mode );
	}

	return false;
}

void CHeistPlayer::StopObserverMode()
{
	m_bEnterObserver = false;
	BaseClass::StopObserverMode();
}

void CHeistPlayer::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;

	if(IsNetClient()) {
		const char *pIdealMode = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_spec_mode");
		if(pIdealMode) {
			observerMode = atoi(pIdealMode);
			if(observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING) {
				observerMode = m_iObserverLastMode;
			}
		}
	}

	m_bEnterObserver = true;
	StartObserverMode( observerMode );
}

void CHeistPlayer::State_PreThink_OBSERVER_MODE()
{
	//Assert(GetMoveType() == MOVETYPE_FLY);
	Assert(m_takedamage == DAMAGE_NO);
	Assert(IsSolidFlagSet(FSOLID_NOT_SOLID));
	//Assert(IsEffectActive(EF_NODRAW));

	Assert(m_lifeState == LIFE_DEAD);
	Assert(pl.deadflag);
}

void CHeistPlayer::State_Enter_ACTIVE()
{
	SetMoveType(MOVETYPE_WALK);

	//RemoveSolidFlags(FSOLID_NOT_SOLID);

	m_Local.m_iHideHUD = 0;
}

void CHeistPlayer::State_PreThink_ACTIVE()
{
}

bool CHeistPlayer::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
	if(!pPlayer) {
		return false;
	}

	return true;
}

Vector CHeistPlayer::GetAutoaimVector(float flScale)
{
	Vector forward;
	AngleVectors(EyeAngles() + m_Local.m_vecPunchAngle, &forward);
	return forward;
}

void CHeistPlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	return;
}

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEPlayerAnimEvent, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent(const char *name)
		: CBaseTempEntity(name)
	{
	}

	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerAnimEvent, DT_TEPlayerAnimEvent)
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropInt(SENDINFO(m_iEvent), Q_log2(PLAYERANIMEVENT_COUNT) + 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nData), 32)
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent("PlayerAnimEvent");

void TE_PlayerAnimEvent(CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData)
{
	CPVSFilter filter((const Vector&)pPlayer->EyePosition());

	filter.UsePredictionRules();

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

void CHeistPlayer::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	m_PlayerAnimState->DoAnimationEvent(event, nData);
	TE_PlayerAnimEvent(this, event, nData);
}

void CHeistPlayer::SetupBones(matrix3x4_t *pBoneToWorld, int boneMask)
{
	VPROF_BUDGET("CHeistPlayer::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM);

	Assert(GetModelPtr());
	CStudioHdr *pStudioHdr = GetModelPtr();
	if(!pStudioHdr) {
		return;
	}

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	Vector adjOrigin = GetAbsOrigin() + Vector(0, 0, m_flEstIkOffset);

	CBoneBitList boneComputed;
	if(m_pIk) {
		++m_iIKCounter;
		m_pIk->Init(pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask);
		GetSkeleton(pStudioHdr, pos, q, boneMask);

		m_pIk->UpdateTargets(pos, q, pBoneToWorld, boneComputed);
		CalculateIKLocks(gpGlobals->curtime);
		m_pIk->SolveDependencies(pos, q, pBoneToWorld, boneComputed);
	} else {
		GetSkeleton(pStudioHdr, pos, q, boneMask);
	}

	CBaseAnimating *pParent = dynamic_cast<CBaseAnimating *>(GetMoveParent());
	if(pParent) {
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if(pParentCache) {
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				m_PlayerAnimState->GetRenderAngles(),
				adjOrigin,
				pos,
				q,
				pBoneToWorld,
				pParent,
				pParentCache
			);

			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr,
		m_PlayerAnimState->GetRenderAngles(),
		adjOrigin,
		pos,
		q,
		-1,
		GetModelScale(),
		pBoneToWorld,
		boneMask
	);
}

void CHeistPlayer::SetArmorValue( int value )
{
	BaseClass::SetArmorValue(value);
	m_iArmor = value;
}

void CHeistPlayer::SetMaxArmorValue( int MaxArmorValue )
{
	m_iMaxArmor = MaxArmorValue;
}

void CHeistPlayer::IncrementArmorValue( int nCount, int nMaxValue )
{ 
	nMaxValue = m_iMaxArmor;
	BaseClass::IncrementArmorValue(nCount, nMaxValue );
}

void CHeistPlayer::SaveTransitionFile()
{
	FileHandle_t hFile = g_pFullFileSystem->Open("transition.cfg", "w");

	if(hFile == FILESYSTEM_INVALID_HANDLE) {
		Warning("Invalid filesystem handle \n");

		CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
		g_pFullFileSystem->WriteFile("cfg/transition.cfg", "MOD", buf);
		return;
	} else {
		for(int i = 1; i <= gpGlobals->maxClients; ++i) {
			CHeistPlayer *pPlayerMP = ToHeistPlayer(UTIL_PlayerByIndex(i));
			if(pPlayerMP == NULL) {
				g_pFullFileSystem->Close( hFile );
				return;
			}

			int HealthValue = pPlayerMP->m_iHealth;
			int ArmourValue = pPlayerMP->m_iArmor;
			int WeaponSlot = 0;

			WeaponSlot = 0;

			char tmpSteamid[32];
			Q_snprintf(tmpSteamid, sizeof(tmpSteamid), "\"%s\"\n""{\n", engine->GetPlayerNetworkIDString(pPlayerMP->edict()));
			g_pFullFileSystem->Write(&tmpSteamid, strlen(tmpSteamid), hFile);

			char data3[32];
			Q_snprintf( data3, sizeof(data3), "\"Health" "\" ");
			char data4[32];
			Q_snprintf(data4, sizeof(data4), "\"%i\"\n", HealthValue);
			g_pFullFileSystem->Write(&data3, strlen(data3), hFile);
			g_pFullFileSystem->Write(&data4, strlen(data4), hFile);

			char data5[32];
			Q_snprintf( data5, sizeof(data5), "\"Armour" "\" ");
			char data6[32];
			Q_snprintf(data6, sizeof(data6), "\"%i\"\n", ArmourValue);
			g_pFullFileSystem->Write(&data5, strlen(data5), hFile);
			g_pFullFileSystem->Write(&data6, strlen(data6), hFile);

			CBaseCombatWeapon *pCheck;

			CBasePlayer *pPlayer = ToBasePlayer(pPlayerMP);
			const char *weaponName = "";
			weaponName = pPlayer->GetActiveWeapon()->GetClassname();

			char ActiveWepPre[32];
			Q_snprintf(ActiveWepPre, sizeof(ActiveWepPre), "\n""\"ActiveWeapon\" ");
			g_pFullFileSystem->Write(&ActiveWepPre, strlen(ActiveWepPre), hFile);

			char ActiveWep[32];
			Q_snprintf(ActiveWep, sizeof(ActiveWep), "\"%s\"\n", weaponName);
			g_pFullFileSystem->Write(&ActiveWep, strlen(ActiveWep), hFile);

			for(int i = 0 ; i < pPlayer->WeaponCount(); ++i) {
				pCheck = pPlayer->GetWeapon(i);
				if(!pCheck) {
					continue;
				}

				int TempPrimaryClip = pPlayer->GetAmmoCount(pCheck->GetPrimaryAmmoType());
				int TempSecondaryClip = pPlayer->GetAmmoCount(pCheck->GetSecondaryAmmoType());

				int ammoIndex_Pri = pCheck->GetPrimaryAmmoType();
				int ammoIndex_Sec = pCheck->GetSecondaryAmmoType();

				int ammoPrimaryClipLeft = pCheck->Clip1();
				int ammoSecondaryClipLeft = pCheck->Clip2();

				char pCheckWep[32];
				Q_snprintf(pCheckWep, sizeof(pCheckWep), "\n\"Weapon_%i\" \"%s\"\n", WeaponSlot, pCheck->GetClassname());
				g_pFullFileSystem->Write( &pCheckWep, strlen(pCheckWep), hFile );

				if(TempPrimaryClip >= 1) {
					char PrimaryClip[32];
					Q_snprintf(PrimaryClip, sizeof(PrimaryClip), "\n\"Weapon_%i_PriClip\" \"%i\"\n", WeaponSlot, TempPrimaryClip);
					g_pFullFileSystem->Write(&PrimaryClip, strlen(PrimaryClip), hFile );

					if(ammoIndex_Pri != -1) {
						char PrimaryWeaponClipAmmoType[32];
						Q_snprintf(PrimaryWeaponClipAmmoType,sizeof(PrimaryWeaponClipAmmoType), "\"Weapon_%i_PriClipAmmo\" ", WeaponSlot);

						char PrimaryClipAmmoType[32];
						if(GetAmmoDef()->GetAmmoOfIndex(ammoIndex_Pri)->pName) {
							Q_snprintf(PrimaryClipAmmoType, sizeof(PrimaryClipAmmoType), "\"%s\"\n", GetAmmoDef()->GetAmmoOfIndex(ammoIndex_Pri)->pName);
						} else {
							Q_snprintf(PrimaryClipAmmoType, sizeof(PrimaryClipAmmoType), "\n");
						}

						char PrimaryClipAmmoLeft[32];
						Q_snprintf(PrimaryClipAmmoLeft, sizeof(PrimaryClipAmmoLeft), "\"Weapon_%i_PriClipAmmoLeft\" \"%i\"\n", WeaponSlot, ammoPrimaryClipLeft);
						g_pFullFileSystem->Write(&PrimaryWeaponClipAmmoType, strlen(PrimaryWeaponClipAmmoType), hFile);
						g_pFullFileSystem->Write(&PrimaryClipAmmoType, strlen(PrimaryClipAmmoType), hFile);
						g_pFullFileSystem->Write(&PrimaryClipAmmoLeft, strlen(PrimaryClipAmmoLeft), hFile);
					}
				}

				if(TempSecondaryClip >= 1) {
					char SecondaryClip[32];
					Q_snprintf(SecondaryClip,sizeof(SecondaryClip), "\n\"Weapon_%i_SecClip\" \"%i\"\n", WeaponSlot,TempSecondaryClip);
					g_pFullFileSystem->Write( &SecondaryClip, strlen(SecondaryClip), hFile );

					if(ammoIndex_Sec != -1) {
						char SecondaryWeaponClipAmmoType[32];
						Q_snprintf(SecondaryWeaponClipAmmoType,sizeof(SecondaryWeaponClipAmmoType), "\"Weapon_%i_SecClipAmmo\" ", WeaponSlot);

						char SecondaryClipAmmoType[32];
						if(GetAmmoDef()->GetAmmoOfIndex(ammoIndex_Pri)->pName) {
							Q_snprintf(SecondaryClipAmmoType, sizeof(SecondaryClipAmmoType), "\"%s\"\n", GetAmmoDef()->GetAmmoOfIndex(ammoIndex_Sec)->pName);
						} else {
							Q_snprintf(SecondaryClipAmmoType, sizeof(SecondaryClipAmmoType), "\n");
						}

						char SecondaryClipAmmoLeft[32];
						Q_snprintf(SecondaryClipAmmoLeft, sizeof(SecondaryClipAmmoLeft), "\"Weapon_%i_SecClipAmmoLeft\" \"%i\"\n", WeaponSlot, ammoSecondaryClipLeft);
						g_pFullFileSystem->Write(&SecondaryWeaponClipAmmoType, strlen(SecondaryWeaponClipAmmoType), hFile );
						g_pFullFileSystem->Write(&SecondaryClipAmmoType, strlen(SecondaryClipAmmoType), hFile );
						g_pFullFileSystem->Write(&SecondaryClipAmmoLeft, strlen(SecondaryClipAmmoLeft), hFile);
					}
				}

				++WeaponSlot;
			}

			char SecClose[32];
			Q_snprintf(SecClose, sizeof(SecClose), "\n\n}\n\n");
			g_pFullFileSystem->Write(&SecClose, strlen(SecClose), hFile);
		}

		g_pFullFileSystem->Close(hFile);
	}
}
