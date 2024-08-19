#ifndef HEIST_GAMERULES_H
#define HEIST_GAMERULES_H

#pragma once

#include "teamplay_gamerules.h"

#ifdef CLIENT_DLL
	#define CHeistGamerules C_HeistGamerules
	#define CHeistGamerulesProxy C_HeistGamerulesProxy
#endif

#ifndef CLIENT_DLL
	#include "heist_player.h"
#endif

enum
{
	TEAM_HEISTERS = FIRST_GAME_TEAM,
	TEAM_POLICE,
};

#define VEC_CROUCH_TRACE_MIN HeistGamerules()->GetHeistViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX HeistGamerules()->GetHeistViewVectors()->m_vCrouchTraceMax

class CHeistGamerulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS(CHeistGamerulesProxy, CGameRulesProxy);
	DECLARE_NETWORKCLASS();
};

class HeistViewVectors : public CViewVectors
{
public:
	HeistViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax )
	: CViewVectors(
		vView,
		vHullMin,
		vHullMax,
		vDuckHullMin,
		vDuckHullMax,
		vDuckView,
		vObsHullMin,
		vObsHullMax,
		vDeadViewHeight
	) {
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;
};

class CHeistGamerules : public CTeamplayRules
{
public:
	DECLARE_CLASS(CHeistGamerules, CTeamplayRules);

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();
#endif

	CHeistGamerules();
	~CHeistGamerules() override;

	bool ShouldCollide(int collisionGroup0, int collisionGroup1) override;

	const unsigned char *GetEncryptionKey() override
	{ return (unsigned char *)"x9Ke0BY7"; }
	const CViewVectors *GetViewVectors() const override;

	const HeistViewVectors *GetHeistViewVectors() const;

	float GetMapRemainingTime();
	void CleanUpMap();
	void CheckRestartGame();
	void RestartGame();

#ifndef CLIENT_DLL
	const char *GetGameDescription() override;
	void DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info ) override;
	void GoToIntermission() override;
	int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget) override;
	void ClientSettingsChanged( CBasePlayer *pPlayer ) override;
	void CreateStandardEntities() override;
	void Think() override;
	float FlWeaponTryRespawn(CBaseCombatWeapon *pWeapon) override;
	float FlWeaponRespawnTime(CBaseCombatWeapon *pWeapon) override;
	Vector VecWeaponRespawnSpot(CBaseCombatWeapon *pWeapon) override;
	int WeaponShouldRespawn(CBaseCombatWeapon *pWeapon) override;
	bool ClientCommand(CBaseEntity *pEdict, const CCommand &args) override;
	void Precache() override;

	Vector VecItemRespawnSpot(CItem *pItem) override;
	QAngle VecItemRespawnAngles(CItem *pItem) override;
	float FlItemRespawnTime(CItem *pItem ) override;
	bool CanHavePlayerItem(CBasePlayer *pPlayer, CBaseCombatWeapon *pItem) override;
	bool FShouldSwitchWeapon(CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon) override;

	void AddLevelDesignerPlacedObject(CBaseEntity *pEntity);
	void RemoveLevelDesignerPlacedObject(CBaseEntity *pEntity);
	void ManageObjectRelocation();
	void CheckChatForReadySignal(CHeistPlayer *pPlayer, const char *chatmsg);
	const char *GetChatFormat(bool bTeamOnly, CBasePlayer *pPlayer) override;

	void InitDefaultAIRelationships() override;

	void ClientDisconnected(edict_t *pClient) override;
	void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info) override;
#endif

	bool IsTeamplay()
	{ return m_bTeamPlayEnabled; }

	bool CheckGameOver();
	bool IsIntermission();

	void CheckAllPlayersReady();

	bool IsConnectedUserInfoChangeAllowed(CBasePlayer *pPlayer) override;

	void SetSpotted(bool value);

	bool AnyoneSpotted() const
	{ return m_bHeistersSpotted; }

private:
	CNetworkVar(bool, m_bTeamPlayEnabled);
	CNetworkVar(float, m_flGameStartTime);
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
	float m_tmNextPeriodicThink;
	float m_flRestartGameTime;
	bool m_bCompleteReset;
	bool m_bAwaitingReadyRestart;
	bool m_bHeardAllPlayersReady;

	CNetworkVar(bool, m_bHeistersSpotted);

#ifndef CLIENT_DLL
	bool m_bChangelevelDone;
#endif
};

inline CHeistGamerules *HeistGamerules()
{
	return static_cast<CHeistGamerules*>(g_pGameRules);
}

#endif
