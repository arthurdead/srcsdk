#ifndef HEIST_PLAYER_H
#define HEIST_PLAYER_H

#pragma once

#include "basemultiplayerplayer.h"
#include "utldict.h"
#include "heist_player_shared.h"
#include "heist_playeranimstate.h"

class CHeistPlayer;

class CHeistPlayerStateInfo
{
public:
	HeistPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CHeistPlayer::*pfnEnterState)();
	void (CHeistPlayer::*pfnLeaveState)();

	void (CHeistPlayer::*pfnPreThink)();
};

class CHeistPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS(CHeistPlayer, CBaseMultiplayerPlayer);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	CHeistPlayer();
	~CHeistPlayer() override;

	static CHeistPlayer *CreatePlayer(const char *className, edict_t *ed)
	{
		CHeistPlayer::s_PlayerEdict = ed;
		return (CHeistPlayer *)CreateEntityByName(className);
	}

	void UpdateOnRemove() override;
	void Precache() override;
	void Spawn() override;

	Class_T Classify() override;

	void PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize) override;

	static void SaveTransitionFile();

	void IncrementArmorValue(int nCount, int nMaxValue = -1);
	void SetArmorValue(int value );
	void SetMaxArmorValue(int MaxArmorValue);

	int GetArmorValue()
	{ return m_iArmor; }
	int GetMaxArmorValue()
	{ return m_iMaxArmor; }

	void PostThink() override;
	void PreThink() override;
	void PlayerDeathThink() override;

	bool ClientCommand(const CCommand &args) override;
	void CreateViewModel(int viewmodelindex = 0) override;
	void Event_Killed(const CTakeDamageInfo &info) override;
	int OnTakeDamage(const CTakeDamageInfo &inputInfo) override;

	bool WantsLagCompensationOnEntity(const CBaseEntity *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const override;

	void FireBullets(const FireBulletsInfo_t &info) override;
	bool Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex = 0) override;
	bool BumpWeapon(CBaseCombatWeapon *pWeapon) override;
	void ChangeTeam(int iTeam) override;
	void PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force) override;
	void Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL) override;
	void DeathSound(const CTakeDamageInfo &info) override;
	CBaseEntity *EntSelectSpawnPoint() override;

	int FlashlightIsOn() override;
	void FlashlightTurnOn() override;
	void FlashlightTurnOff() override;

	Vector GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL) override;
	Vector GetAutoaimVector(float flDelta) override;

	void CheatImpulseCommands(int iImpulse) override;

	void SetAnimation(PLAYER_ANIM playerAnim) override;

	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);
	void SetupBones(matrix3x4_t *pBoneToWorld, int boneMask) override;

	void Reset();

	bool IsReady();
	void SetReady(bool bReady);

	void CheckChatText(char *p, int bufsize);

	void State_Transition(HeistPlayerState newState);
	void State_Enter(HeistPlayerState newState);
	void State_Leave();
	void State_PreThink();
	CHeistPlayerStateInfo *State_LookupInfo(HeistPlayerState state);

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	bool StartObserverMode(int mode) override;
	void StopObserverMode() override;

	bool CanHearAndReadChatFrom(CBasePlayer *pPlayer) override;

	void SetSpotted(bool value);

private:
	void PrecacheFootStepSounds();
	bool HandleCommand_JoinTeam(int team);
	void NoteWeaponFired();
	bool ShouldRunRateLimitedCommand(const CCommand &args);

	int m_iArmor;
	int m_iMaxArmor;

	Vector m_vecTotalBulletForce;

	CHeistPlayerAnimState *m_PlayerAnimState;

	int m_iLastWeaponFireUsercmd;

	HeistPlayerState m_iPlayerState;
	CHeistPlayerStateInfo *m_pCurStateInfo;

	CNetworkVar(int, m_iSpawnInterpCounter);

	CNetworkQAngle(m_angEyeAngles);

	CUtlDict<float, int> m_RateLimitLastCommandTimes;

	bool m_bEnterObserver;
	bool m_bReady;

	CNetworkVar(int, m_cycleLatch);
	CountdownTimer m_cycleLatchTimer;

	CNetworkVar(bool, m_bSpotted);
};

inline CHeistPlayer *ToHeistPlayer(CBaseEntity *pEntity)
{
	if(!pEntity || !pEntity->IsPlayer()) {
		return NULL;
	}

	return dynamic_cast<CHeistPlayer *>(pEntity);
}

#endif
