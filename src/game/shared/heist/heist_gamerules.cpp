#include "cbase.h"
#include "heist_gamerules.h"
#include "voice_gamemgr.h"
#include "gamevars_shared.h"
#include "viewport_panel_names.h"
#include "weapon_heistbase.h"
#include "ammodef.h"
#include "suspicioner.h"

#ifdef GAME_DLL
#include "team.h"
#include "items.h"
#include "eventqueue.h"
#else
#include "c_team.h"
#endif

#ifdef GAME_DLL
#include "game.h"
#include "mapentities.h"
#include "gameinterface.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ENTITY_INTOLERANCE 100

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

#define BULLET_MASS_GRAINS_TO_LB(grains) (0.002285f*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains) lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

#define BULLET_IMPULSE_EXAGGERATION 3.5f
#define BULLET_IMPULSE(grains, ftpersec) ((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

#ifdef GAME_DLL
ConVar sv_heist_weapon_respawn_time("sv_heist_weapon_respawn_time", "20", FCVAR_GAMEDLL|FCVAR_NOTIFY);
ConVar sv_heist_item_respawn_time("sv_heist_item_respawn_time", "30", FCVAR_GAMEDLL|FCVAR_NOTIFY);
ConVar mp_ready_signal("mp_ready_signal", "ready", FCVAR_GAMEDLL, "Text that each player must speak for the match to begin");
ConVar mp_readyrestart("mp_readyrestart", "0", FCVAR_GAMEDLL, "If non-zero, game will restart once each player gives the ready signal");
#endif

#ifdef CLIENT_DLL
ConVar cl_autowepswitch("cl_autowepswitch", "1", FCVAR_ARCHIVE|FCVAR_USERINFO, "Automatically switch to picked up weapons (if more powerful)");
#endif

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
extern ConVar mp_chattime;

extern bool FindInList(const char **pStrings, const char *pToFind);

const char *sTeamNames[] = {
	"Unassigned",
	"Spectator",
	"Heisters",
	"Police",
};

REGISTER_GAMERULES_CLASS(CHeistGamerules);

BEGIN_NETWORK_TABLE_NOBASE(CHeistGamerules, DT_HeistGamerules)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bTeamPlayEnabled)),
	RecvPropBool(RECVINFO(m_bHeistersSpotted)),
#else
	SendPropBool(SENDINFO(m_bTeamPlayEnabled)),
	SendPropBool(SENDINFO(m_bHeistersSpotted)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(heist_gamerules, CHeistGamerulesProxy);
IMPLEMENT_NETWORKCLASS_ALIASED(HeistGamerulesProxy, DT_HeistGamerulesProxy)

static HeistViewVectors g_HeistViewVectors(
	Vector(0, 0, 64),

	Vector(-16, -16, 0),
	Vector(16, 16, 72),

	Vector(-16, -16, 0),
	Vector(16, 16, 36),
	Vector(0, 0, 28),

	Vector(-10, -10, -10),
	Vector(10, 10, 10),

	Vector(0, 0, 14),

	Vector(-16, -16, 0),
	Vector(16, 16, 60)
);

static const char *s_PreserveEnts[] = {
	"ai_network",
	"ai_hint",
	"heist_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"",
};

#ifdef CLIENT_DLL
void RecvProxy_HeistGamerules(const RecvProp *pProp, void **pOut, void *pData, int objectID)
{
	CHeistGamerules *pRules = HeistGamerules();
	Assert(pRules);
	*pOut = pRules;
}

BEGIN_RECV_TABLE(CHeistGamerulesProxy, DT_HeistGamerulesProxy)
	RecvPropDataTable("heist_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistGamerules), RecvProxy_HeistGamerules)
END_RECV_TABLE()
#else
void *SendProxy_HeistGamerules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CHeistGamerules *pRules = HeistGamerules();
	Assert(pRules);
	return pRules;
}

BEGIN_SEND_TABLE(CHeistGamerulesProxy, DT_HeistGamerulesProxy)
	SendPropDataTable("heist_gamerules_data", 0, &REFERENCE_SEND_TABLE(DT_HeistGamerules), SendProxy_HeistGamerules)
END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL
class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity)
	{
		return (pListener->GetTeamNumber() == pTalker->GetTeamNumber());
	}
};

CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;
#endif

CHeistGamerules::CHeistGamerules()
{
#ifndef CLIENT_DLL
	for(int i = 0; i < ARRAYSIZE(sTeamNames); ++i) {
		CTeam *pTeam = static_cast<CTeam *>(CreateEntityByName("team_manager"));
		pTeam->Init(sTeamNames[i], i);

		g_Teams.AddToTail(pTeam);
	}

	InitDefaultAIRelationships();

	m_bTeamPlayEnabled = teamplay.GetBool();
	m_flIntermissionEndTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bHeardAllPlayersReady = false;
	m_bAwaitingReadyRestart = false;
	m_bChangelevelDone = false;
#endif
}

const CViewVectors *CHeistGamerules::GetViewVectors()const
{
	return &g_HeistViewVectors;
}

const HeistViewVectors *CHeistGamerules::GetHeistViewVectors()const
{
	return &g_HeistViewVectors;
}

CHeistGamerules::~CHeistGamerules()
{
#ifndef CLIENT_DLL
	g_Teams.Purge();
#endif
}

#ifndef CLIENT_DLL
void CHeistGamerules::CreateStandardEntities()
{
	BaseClass::CreateStandardEntities();

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create("heist_gamerules", vec3_origin, vec3_angle);
	Assert(pEnt);
}

float CHeistGamerules::FlWeaponRespawnTime(CBaseCombatWeapon *pWeapon)
{
#ifndef CLIENT_DLL
	if(weaponstay.GetInt() > 0) {
		if(!(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD)) {
			return 0;
		}
	}

	return sv_heist_weapon_respawn_time.GetFloat();
#else
	return 0;
#endif
}
#endif

bool CHeistGamerules::IsIntermission()
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#else
	return false;
#endif
}

#ifndef CLIENT_DLL
void CHeistGamerules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	if(IsIntermission()) {
		return;
	}

	BaseClass::PlayerKilled(pVictim, info);
}

void CHeistGamerules::Think()
{
	CGameRules::Think();

	if(g_fGameOver ) {
		if(m_flIntermissionEndTime < gpGlobals->curtime) {
			if(!m_bChangelevelDone) {
				ChangeLevel();
				m_bChangelevelDone = true;
			}
		}

		return;
	}

	if(GetMapRemainingTime() < 0) {
		GoToIntermission();
		return;
	}

	if(gpGlobals->curtime > m_tmNextPeriodicThink) {
		CheckAllPlayersReady();
		CheckRestartGame();
		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	if(m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime) {
		RestartGame();
	}

	if(m_bAwaitingReadyRestart && m_bHeardAllPlayersReady) {
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "All players ready. Game will restart in 5 seconds");
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "All players ready. Game will restart in 5 seconds");

		m_flRestartGameTime = gpGlobals->curtime + 5;
		m_bAwaitingReadyRestart = false;
	}

	ManageObjectRelocation();
}

void CHeistGamerules::GoToIntermission()
{
	if(g_fGameOver) {
		return;
	}

	g_fGameOver = true;

	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	for(int i = 0; i < MAX_PLAYERS; ++i) {
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer) {
			continue;
		}

		pPlayer->ShowViewPortPanel(PANEL_SCOREBOARD);
		pPlayer->AddFlag(FL_FROZEN);
	}
}
#endif

bool CHeistGamerules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if(g_fGameOver) {
		if(m_flIntermissionEndTime < gpGlobals->curtime) {
			ChangeLevel();
		}

		return true;
	}
#endif

	return false;
}

#ifndef CLIENT_DLL
float CHeistGamerules::FlWeaponTryRespawn(CBaseCombatWeapon *pWeapon)
{
	if(pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD)) {
		if(gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE)) {
			return 0;
		}

		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

Vector CHeistGamerules::VecWeaponRespawnSpot(CBaseCombatWeapon *pWeapon)
{
	CWeaponHeistBase *pHeistWeapon = dynamic_cast<CWeaponHeistBase *>(pWeapon);
	if(pHeistWeapon) {
		return pHeistWeapon->GetOriginalSpawnOrigin();
	}

	return pWeapon->GetAbsOrigin();
}
#endif

#ifndef CLIENT_DLL
CItem *IsManagedObjectAnItem(CBaseEntity *pObject)
{
	return dynamic_cast<CItem *>(pObject);
}

CWeaponHeistBase *IsManagedObjectAWeapon(CBaseEntity *pObject)
{
	return dynamic_cast<CWeaponHeistBase *>(pObject);
}

bool GetObjectsOriginalParameters(CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles)
{
	if(CItem *pItem = IsManagedObjectAnItem(pObject)) {
		if(pItem->m_flNextResetCheckTime > gpGlobals->curtime) {
			return false;
		}

		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_heist_item_respawn_time.GetFloat();
		return true;
	} else if(CWeaponHeistBase *pWeapon = IsManagedObjectAWeapon(pObject)) {
		if(pWeapon->m_flNextResetCheckTime > gpGlobals->curtime ) {
			return false;
		}

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_heist_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}

void CHeistGamerules::ManageObjectRelocation( void )
{
	int iTotal = m_hRespawnableItemsAndWeapons.Count();

	if(iTotal > 0) {
		for(int i = 0; i < iTotal; ++i) {
			CBaseEntity *pObject = m_hRespawnableItemsAndWeapons[i].Get();

			if(pObject) {
				Vector vSpawOrigin;
				QAngle vSpawnAngles;

				if(GetObjectsOriginalParameters(pObject, vSpawOrigin, vSpawnAngles) == true) {
					float flDistanceFromSpawn = (pObject->GetAbsOrigin() - vSpawOrigin ).Length();

					if(flDistanceFromSpawn > WEAPON_MAX_DISTANCE_FROM_SPAWN) {
						bool shouldReset = false;
						IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

						if(pPhysics) {
							shouldReset = pPhysics->IsAsleep();
						} else {
							shouldReset = (pObject->GetFlags() & FL_ONGROUND) ? true : false;
						}

						if(shouldReset) {
							pObject->Teleport(&vSpawOrigin, &vSpawnAngles, NULL);
							pObject->EmitSound("AlyxEmp.Charge");

							IPhysicsObject *pPhys = pObject->VPhysicsGetObject();
							if(pPhys) {
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
}

void CHeistGamerules::AddLevelDesignerPlacedObject(CBaseEntity *pEntity)
{
	if(m_hRespawnableItemsAndWeapons.Find(pEntity) == -1) {
		m_hRespawnableItemsAndWeapons.AddToTail(pEntity);
	}
}

void CHeistGamerules::RemoveLevelDesignerPlacedObject(CBaseEntity *pEntity)
{
	if(m_hRespawnableItemsAndWeapons.Find(pEntity) != -1) {
		m_hRespawnableItemsAndWeapons.FindAndRemove(pEntity);
	}
}

Vector CHeistGamerules::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CHeistGamerules::VecItemRespawnAngles(CItem *pItem)
{
	return pItem->GetOriginalSpawnAngles();
}

float CHeistGamerules::FlItemRespawnTime(CItem *pItem)
{
	return sv_heist_item_respawn_time.GetFloat();
}

bool CHeistGamerules::CanHavePlayerItem(CBasePlayer *pPlayer, CBaseCombatWeapon *pItem)
{
	if(weaponstay.GetInt() > 0) {
		if(pPlayer->Weapon_OwnsThisType(pItem->GetClassname(), pItem->GetSubType())) {
			return false;
		}
	}

	return BaseClass::CanHavePlayerItem(pPlayer, pItem);
}

int CHeistGamerules::WeaponShouldRespawn(CBaseCombatWeapon *pWeapon)
{
	if(pWeapon->HasSpawnFlags(SF_NORESPAWN)) {
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

void CHeistGamerules::ClientDisconnected(edict_t *pClient)
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);
	if(pPlayer) {
		if(pPlayer->GetTeam()) {
			pPlayer->GetTeam()->RemovePlayer(pPlayer);
		}
	}

	BaseClass::ClientDisconnected(pClient);
}

void CHeistGamerules::SetSpotted(bool value)
{
	m_bHeistersSpotted = value;

	if(value) {
		CSuspicioner::ClearAll();
	}
}

void CHeistGamerules::DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	const char *killer_weapon_name = "world";
	int killer_ID = 0;

	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer(pKiller, pInflictor);

	if(info.GetDamageCustom()) {
		killer_weapon_name = GetDamageCustomString(info);
		if(pScorer) {
			killer_ID = pScorer->GetUserID();
		}
	} else {
		if(pScorer) {
			killer_ID = pScorer->GetUserID();

			if(pInflictor) {
				if(pInflictor == pScorer) {
					if(pScorer->GetActiveWeapon()) {
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				} else {
					killer_weapon_name = pInflictor->GetClassname();
				}
			}
		} else {
			killer_weapon_name = pInflictor->GetClassname();
		}

		if(strncmp(killer_weapon_name, "weapon_", 7) == 0) {
			killer_weapon_name += 7;
		} else if(strncmp(killer_weapon_name, "npc_", 4) == 0) {
			killer_weapon_name += 4;
		} else if(strncmp(killer_weapon_name, "func_", 5) == 0) {
			killer_weapon_name += 5;
		} else if(strstr(killer_weapon_name, "physics")) {
			killer_weapon_name = "physics";
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent("player_death");
	if(event) {
		event->SetInt("userid", pVictim->GetUserID());
		event->SetInt("attacker", killer_ID);
		event->SetString("weapon", killer_weapon_name);
		event->SetInt("priority", 7);
		gameeventmanager->FireEvent(event);
	}
}

void CHeistGamerules::ClientSettingsChanged(CBasePlayer *pPlayer)
{
	CHeistPlayer *pHeistPlayer = ToHeistPlayer(pPlayer);
	if(pHeistPlayer == NULL) {
		return;
	}

	BaseClass::ClientSettingsChanged( pPlayer );
}

int CHeistGamerules::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	if(!pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false) {
		return GR_NOTTEAMMATE;
	}

	if((*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp(GetTeamID(pPlayer), GetTeamID(pTarget))) {
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

const char *CHeistGamerules::GetGameDescription( void )
{ 
	return "Heist"; 
}
#endif 

bool CHeistGamerules::IsConnectedUserInfoChangeAllowed(CBasePlayer *pPlayer)
{
	return true;
}

float CHeistGamerules::GetMapRemainingTime()
{
	if(mp_timelimit.GetInt() <= 0) {
		return 0;
	}

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f) - gpGlobals->curtime;
	return timeleft;
}

#ifndef CLIENT_DLL
void CHeistGamerules::Precache()
{
}
#endif

bool CHeistGamerules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	if(collisionGroup0 > collisionGroup1) {
		V_swap(collisionGroup0, collisionGroup1);
	}

	if((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) && collisionGroup1 == COLLISION_GROUP_WEAPON) {
		return false;
	}

	if(collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) {
		collisionGroup0 = COLLISION_GROUP_PLAYER;
	}

	if(collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT) {
		collisionGroup1 = COLLISION_GROUP_PLAYER;
	}

	if(collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER) {
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	if(collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER) {
		return false;
	}

	if(collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED) {
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 

}

#ifndef CLIENT_DLL
bool CHeistGamerules::ClientCommand(CBaseEntity *pEdict, const CCommand &args)
{
	if(BaseClass::ClientCommand(pEdict, args)) {
		return true;
	}

	CHeistPlayer *pPlayer = (CHeistPlayer *)pEdict;
	if(pPlayer->ClientCommand(args)) {
		return true;
	}

	return false;
}
#endif

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if(!bInitted) {
		bInitted = true;

		def.AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 8, 0, 0);
	}

	return &def;
}

#ifndef CLIENT_DLL
bool CHeistGamerules::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon)
{
	if(pPlayer->GetActiveWeapon() && pPlayer->IsNetClient()) {
		const char *cl_autowepswitch = engine->GetClientConVarValue(engine->IndexOfEdict(pPlayer->edict()), "cl_autowepswitch");
		if(cl_autowepswitch && atoi(cl_autowepswitch) <= 0) {
			return false;
		}
	}

	return BaseClass::FShouldSwitchWeapon(pPlayer, pWeapon);
}

void CHeistGamerules::RestartGame()
{
	if(mp_timelimit.GetInt() < 0) {
		mp_timelimit.SetValue(0);
	}

	m_flGameStartTime = gpGlobals->curtime;

	if(!IsFinite(m_flGameStartTime.Get())) {
		Warning("Trying to set a NaN game start time\n");
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();

	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(i);
		if(!pPlayer) {
			continue;
		}

		if(pPlayer->GetActiveWeapon()) {
			pPlayer->GetActiveWeapon()->Holster();
		}

		pPlayer->RemoveAllItems(true);
		respawn(pPlayer, false);
		pPlayer->Reset();
	}

	CTeam *pHeisters = GetGlobalTeam( TEAM_HEISTERS );
	CTeam *pPolice = GetGlobalTeam( TEAM_POLICE );

	if ( pHeisters )
	{
		pHeisters->SetScore( 0 );
	}

	if ( pPolice )
	{
		pPolice->SetScore( 0 );
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;
	m_bCompleteReset = false;

	IGameEvent *event = gameeventmanager->CreateEvent("round_start");
	if(event) {
		event->SetInt("fraglimit", 0);
		event->SetInt("priority", 6);
		event->SetString("objective", "Heist");
		gameeventmanager->FireEvent( event );
	}
}

void CHeistGamerules::CleanUpMap()
{
	CBaseEntity *pCur = gEntList.FirstEnt();
	while(pCur) {
		CWeaponHeistBase *pWeapon = dynamic_cast<CWeaponHeistBase *>(pCur);
		if(pWeapon) {
			if(!pWeapon->GetPlayerOwner()) {
				UTIL_Remove(pCur);
			}
		} else if(!FindInList(s_PreserveEnts, pCur->GetClassname())) {
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	gEntList.CleanupDeleteList();

	g_EventQueue.Clear();

	class CHeistMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity(const char *pClassname)
		{
			if(!FindInList(s_PreserveEnts, pClassname)) {
				return true;
			} else {
				if(m_iIterator != g_MapEntityRefs.InvalidIndex()) {
					m_iIterator = g_MapEntityRefs.Next(m_iIterator);
				}

				return false;
			}
		}

		virtual CBaseEntity *CreateNextEntity(const char *pClassname)
		{
			if(m_iIterator == g_MapEntityRefs.InvalidIndex()) {
				Assert(false);
				return NULL;
			} else {
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if(ref.m_iEdict == -1 || engine->PEntityOfEntIndex(ref.m_iEdict)) {
					return CreateEntityByName(pClassname);
				} else {
					return CreateEntityByName(pClassname, ref.m_iEdict);
				}
			}
		}

	public:
		int m_iIterator;
	};

	CHeistMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	MapEntity_ParseAllEntities(engine->GetMapEntitiesString(), &filter, true);
}

void CHeistGamerules::CheckChatForReadySignal(CHeistPlayer *pPlayer, const char *chatmsg)
{
	if(m_bAwaitingReadyRestart && FStrEq(chatmsg, mp_ready_signal.GetString())) {
		if(!pPlayer->IsReady()) {
			pPlayer->SetReady( true );
		}
	}
}

void CHeistGamerules::CheckRestartGame()
{
	int iRestartDelay = mp_restartgame.GetInt();
	if(iRestartDelay > 0) {
		if(iRestartDelay > 60) {
			iRestartDelay = 60;
		}

		char strRestartDelay[64];
		Q_snprintf(strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay);
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS");
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS");

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}

	if(mp_readyrestart.GetBool()) {
		m_bAwaitingReadyRestart = true;
		m_bHeardAllPlayersReady = false;

		const char *pszReadyString = mp_ready_signal.GetString();

		if(pszReadyString == NULL || Q_strlen(pszReadyString) > 16) {
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent("heist_ready_restart");
		if(event) {
			gameeventmanager->FireEvent(event);
		}

		mp_readyrestart.SetValue(0);

		m_flRestartGameTime = -1;
	}
}

void CHeistGamerules::CheckAllPlayersReady()
{
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(i);
		if(!pPlayer) {
			continue;
		}

		if(!pPlayer->IsReady()) {
			return;
		}
	}

	m_bHeardAllPlayersReady = true;
}

const char *CHeistGamerules::GetChatFormat(bool bTeamOnly, CBasePlayer *pPlayer)
{
	if(!pPlayer) {
		return NULL;
	}

	const char *pszFormat = NULL;

	if(bTeamOnly == TRUE) {
		if(pPlayer->GetTeamNumber() == TEAM_SPECTATOR) {
			pszFormat = "Heist_Chat_Spec";
		} else {
			const char *chatLocation = GetChatLocation(bTeamOnly, pPlayer);
			if(chatLocation && *chatLocation) {
				pszFormat = "Heist_Chat_Team_Loc";
			} else {
				pszFormat = "Heist_Chat_Team";
			}
		}
	} else {
		if(pPlayer->GetTeamNumber() != TEAM_SPECTATOR) {
			pszFormat = "Heist_Chat_All";
		} else {
			pszFormat = "Heist_Chat_AllSpec";
		}
	}

	return pszFormat;
}

void InitBodyQue()
{
}

void CHeistGamerules::InitDefaultAIRelationships()
{
	CBaseCombatCharacter::AllocateDefaultRelationships();

	for(int i = 0; i < NUM_AI_CLASSES; ++i) {
		for(int j = 0; j < NUM_AI_CLASSES; ++j) {
			CBaseCombatCharacter::SetDefaultRelationship((Class_T)i, (Class_T)j, D_NU, 0);
		}
	}

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_HEISTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_POLICE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_HEISTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_POLICE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_POLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_HEISTER, D_HT, 0);
}
#endif
