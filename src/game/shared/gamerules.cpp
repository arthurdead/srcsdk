//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "ammodef.h"
#include "tier0/vprof.h"
#include "KeyValues.h"
#include "iachievementmgr.h"
#include "tier0/icommandline.h"
#include "mp_shareddefs.h"
#include "usermessages.h"
#include "filesystem.h"

#ifdef CLIENT_DLL

#include "c_team.h"

#else

#include "player.h"
#include "game.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "voice_gamemgr.h"
#include "globalstate.h"
#include "player_resource.h"
#include "gamestats.h"
#include "hltvdirector.h"
#include "viewport_panel_names.h"
#include "team.h"
#include "mapentities_shared.h"
#include "gameinterface.h"
#include "eventqueue.h"
#include "iscorer.h"
#include "client.h"
#include "items.h"
#include "subs.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_GAMERULES, "GameRules Server" );
#else
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_GAMERULES, "GameRules Client" );
#endif

BEGIN_NETWORK_TABLE_NOBASE(CSharedGameRules, DT_GameRules)
	PropFloat(PROPINFO(m_flGravityMultiplier), 0, SPROP_NOSCALE),
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
IMPLEMENT_CLIENTCLASS_NULL( C_GameRules, DT_GameRules, CGameRules )
#else
IMPLEMENT_SERVERCLASS( CGameRules, DT_GameRules );
#endif

#ifdef GAME_DLL
void *NetProxy_GameRules( const SendPropInfo *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CGameRules *pRules = GameRules();
	Assert(pRules);
	return pRules;
}
#else
void NetProxy_GameRules(const RecvProp *pProp, void **pOut, void *pData, int objectID)
{
	C_GameRules *pRules = GameRules();
	Assert(pRules);
	*pOut = pRules;
}
#endif

// Don't send any of the CBaseEntity stuff..
BEGIN_NETWORK_TABLE_NOBASE( CSharedGameRulesProxy, DT_GameRulesProxy )
	PropDataTable("gamerules_data", 0, SPROP_NONE, &REFERENCE_DATATABLE(DT_GameRules), NetProxy_GameRules)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( GameRulesProxy, DT_GameRulesProxy )

CSharedGameRules*	s_pGameRules = NULL;
CSharedGameRules* GameRules()
{
	return s_pGameRules;
}

REGISTER_GAMERULES_CLASS_ALIASED( GameRules );

#ifdef CLIENT_DLL
ConVar cl_autowepswitch("cl_autowepswitch", "1", FCVAR_ARCHIVE|FCVAR_USERINFO, "Automatically switch to picked up weapons (if more powerful)");
#endif

ConVar g_Language( "g_Language", "0", FCVAR_REPLICATED );
ConVar sk_autoaim_mode( "sk_autoaim_mode", "1", FCVAR_ARCHIVE | FCVAR_REPLICATED );

ConVar mp_chattime(
		"mp_chattime", 
		"10", 
		FCVAR_REPLICATED,
		"amount of time players can chat after the game is over",
		true, 1,
		true, 120 );

#ifdef GAME_DLL
void MPTimeLimitCallback( IConVarRef var, const char *pOldString, float flOldValue )
{
	if ( var.GetInt() < 0 )
	{
		var.SetValue( 0 );
	}

	if ( GameRules() )
	{
		GameRules()->HandleTimeLimitChange();
	}
}
#endif 

ConVar mp_timelimit( "mp_timelimit", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "game time per map in minutes"
#ifdef GAME_DLL
					, MPTimeLimitCallback 
#endif
					);

ConVar fraglimit( "mp_fraglimit","0", FCVAR_NOTIFY|FCVAR_REPLICATED, "The number of kills at which the map ends");

ConVar mp_show_voice_icons( "mp_show_voice_icons", "1", FCVAR_REPLICATED, "Show overhead player voice icons when players are speaking.\n" );

#ifdef GAME_DLL

ConVar tv_delaymapchange( "tv_delaymapchange", "0", FCVAR_NONE, "Delays map change until broadcast is complete" );
ConVar tv_delaymapchange_protect( "tv_delaymapchange_protect", "1", FCVAR_NONE, "Protect against doing a manual map change if HLTV is broadcasting and has not caught up with a major game event such as round_end" );

ConVar mp_restartgame( "mp_restartgame", "0", FCVAR_GAMEDLL, "If non-zero, game will restart in the specified number of seconds" );
ConVar mp_restartgame_immediate( "mp_restartgame_immediate", "0", FCVAR_GAMEDLL, "If non-zero, game will restart immediately" );

ConVar mp_mapcycle_empty_timeout_seconds( "mp_mapcycle_empty_timeout_seconds", "0", FCVAR_REPLICATED, "If nonzero, server will cycle to the next map if it has been empty on the current map for N seconds");

void cc_SkipNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( GameRules() )
	{
		GameRules()->SkipNextMapInCycle();
	}
}

void cc_GotoNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( GameRules() )
	{
		GameRules()->ChangeLevel();
	}
}

ConCommand skip_next_map( "skip_next_map", cc_SkipNextMapInCycle, "Skips the next map in the map rotation for the server." );
ConCommand changelevel_next( "changelevel_next", cc_GotoNextMapInCycle, "Immediately changes to the next map in the map rotation for the server." );

ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "0", FCVAR_GAMEDLL, "WaitingForPlayers time length in seconds" );

ConVar mp_waitingforplayers_restart( "mp_waitingforplayers_restart", "0", FCVAR_GAMEDLL, "Set to 1 to start or restart the WaitingForPlayers period." );
ConVar mp_waitingforplayers_cancel( "mp_waitingforplayers_cancel", "0", FCVAR_GAMEDLL, "Set to 1 to end the WaitingForPlayers period." );
ConVar mp_clan_readyrestart( "mp_clan_readyrestart", "0", FCVAR_GAMEDLL, "If non-zero, game will restart once someone from each team gives the ready signal" );
ConVar mp_clan_ready_signal( "mp_clan_ready_signal", "ready", FCVAR_GAMEDLL, "Text that team leader from each team must speak for the match to begin" );

ConVar nextlevel( "nextlevel", 
				  "", 
				  FCVAR_GAMEDLL | FCVAR_NOTIFY,
				  "If set to a valid map name, will trigger a changelevel to the specified map at the end of the round" );

// Hook into the convar from the engine
extern ConVarBase *skill;

ConVar sv_weapon_respawn_time("sv_weapon_respawn_time", "20", FCVAR_GAMEDLL|FCVAR_NOTIFY);
ConVar sv_item_respawn_time("sv_item_respawn_time", "30", FCVAR_GAMEDLL|FCVAR_NOTIFY);

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

#define FRIENDLY_FIRE_GLOBALNAME "friendly_fire_override"

extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;
extern bool	g_fGameOver;

int CSharedGameRules::m_nMapCycleTimeStamp = 0;
int CSharedGameRules::m_nMapCycleindex = 0;
CUtlStringList CSharedGameRules::m_MapList;

ConVar log_verbose_enable( "log_verbose_enable", "0", FCVAR_GAMEDLL, "Set to 1 to enable verbose server log on the server." );
ConVar log_verbose_interval( "log_verbose_interval", "3.0", FCVAR_GAMEDLL, "Determines the interval (in seconds) for the verbose server log." );

CBaseEntity	*g_pLastSpawn = NULL;

#endif // CLIENT_DLL

ConVar	teamplay( "mp_teamplay","0", FCVAR_NOTIFY|FCVAR_REPLICATED );
extern ConVarBase *deathmatch;
extern ConVarBase *coop;

CViewVectors g_DefaultViewVectors(
	Vector(-16, -16, 0 ),		//VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  71 ),		//VEC_HULL_MAX (m_vHullMax)
	Vector( 0, 0, 62 ),			//VEC_VIEW (m_vView)

	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  55 ),		//VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 37 ),			//VEC_DUCK_VIEW		(m_vDuckView)

	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX	(m_vObsHullMax)

	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)
);

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

// ------------------------------------------------------------------------------------ //
// CGameRulesProxy implementation.
// ------------------------------------------------------------------------------------ //

CSharedGameRulesProxy *g_pGameRulesProxy = NULL;

#ifdef CLIENT_DLL
	#define CGameRulesProxy C_GameRulesProxy
#endif

CSharedGameRulesProxy::CGameRulesProxy()
{
	// allow map placed proxy entities to overwrite the static one
	if ( !g_pGameRulesProxy ) {
		g_pGameRulesProxy = this;
		AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}
}

CSharedGameRulesProxy::~CGameRulesProxy()
{
	if ( g_pGameRulesProxy == this )
		g_pGameRulesProxy = NULL;
}

#ifdef CLIENT_DLL
	#undef CGameRulesProxy
#endif

void CSharedGameRulesProxy::Spawn( void )
{
	if(g_pGameRulesProxy && g_pGameRulesProxy != this) {
		UTIL_Remove(this);
		return;
	}

	BaseClass::Spawn();
}

#ifndef CLIENT_DLL
void CSharedGameRulesProxy::NotifyNetworkStateChanged()
{
	if ( g_pGameRulesProxy )
		g_pGameRulesProxy->NetworkStateChanged();
}

void CSharedGameRulesProxy::NotifyNetworkStateChanged( unsigned short offset )
{
	if ( g_pGameRulesProxy )
		g_pGameRulesProxy->NetworkStateChanged( offset );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DamageTypes_t CSharedGameRules::Damage_GetTimeBased( void )
{
	DamageTypes_t iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DamageTypes_t	CSharedGameRules::Damage_GetShouldGibCorpse( void )
{
	DamageTypes_t iDamage = ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DamageTypes_t CSharedGameRules::Damage_GetShowOnHud( void )
{
	DamageTypes_t iDamage = ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DamageTypes_t	CSharedGameRules::Damage_GetNoPhysicsForce( void )
{
	DamageTypes_t iTimeBasedDamage = Damage_GetTimeBased();
	DamageTypes_t iDamage = ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DamageTypes_t	CSharedGameRules::Damage_GetShouldNotBleed( void )
{
	DamageTypes_t iDamage = ( DMG_POISON | DMG_ACID );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedGameRules::Damage_IsTimeBased( DamageTypes_t iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN ) ) != DMG_GENERIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedGameRules::Damage_ShouldGibCorpse( DamageTypes_t iDmgType )
{
	// Damage types that gib the corpse.
	return ( ( iDmgType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB ) ) != DMG_GENERIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedGameRules::Damage_ShowOnHUD( DamageTypes_t iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK ) ) != DMG_GENERIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedGameRules::Damage_NoPhysicsForce( DamageTypes_t iDmgType )
{
	// Damage types that don't have to supply a physics force & position.
	DamageTypes_t iTimeBasedDamage = Damage_GetTimeBased();
	return ( ( iDmgType & ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE ) ) != DMG_GENERIC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedGameRules::Damage_ShouldNotBleed( DamageTypes_t iDmgType )
{
	// Damage types that don't make the player bleed.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID ) ) != DMG_GENERIC );
}

void CSharedGameRules::SafeRemoveIfDesired()
{
	if(s_pGameRules && s_pGameRules != this) {
		UTIL_Remove(this);
		return;
	}

	BaseClass::SafeRemoveIfDesired();
}

#ifdef GAME_DLL
DEFINE_ENTITY_FACTORY( CGameRulesProxy );

IEntityFactory *CSharedGameRules::ProxyFactory()
{
	return &g_CGameRulesProxyFactory;
}

class CGameRulesEntityFactory : public IEntityFactory
{
public:
	CGameRulesEntityFactory()
	{
		EntityFactoryDictionary()->InstallFactory( this, "gamerules_proxy" );
	}

	CBaseEntity *Create( const char *pClassName )
	{
		return m_pTargetFactory->Create( pClassName );
	}

	void Destroy( CBaseEntity *pNetworkable )
	{
		m_pTargetFactory->Destroy( pNetworkable );
	}

	size_t GetEntitySize()
	{
		return m_pTargetFactory->GetEntitySize();
	}

	const char *DllClassname() const 
	{
		return m_pTargetFactory->DllClassname();
	}

	const char *MapClassname() const
	{
		return "gamerules_proxy";
	}

	map_datamap_t *GetMapDataDesc() const
	{
		return m_pTargetFactory->GetMapDataDesc();
	}

	IEntityFactory *m_pTargetFactory;
};
static CGameRulesEntityFactory gamerulesProxyFactory;
#endif

bool CSharedGameRules::Init()
{
#ifdef GAME_DLL
	gamerulesProxyFactory.m_pTargetFactory = ProxyFactory();

	// Initialize the custom response rule dictionaries.
	InitCustomResponseRulesDicts();

	RefreshSkillData( true );

	LoadMapCycleFile();
#endif

	return BaseClass::Init();
}

#ifdef CLIENT_DLL
	#define CGameRules C_GameRules
#endif

CSharedGameRules::CGameRules() : CAutoGameSystemPerFrame( V_STRINGIFY(CGameRules) ), CGameEventListener()
{
	Assert( !s_pGameRules );
	if(!s_pGameRules)
		s_pGameRules = this;

	m_flGravityMultiplier = 1.0f;

#ifdef GAME_DLL
	m_flIntermissionEndTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bChangelevelDone = false;

	GetVoiceGameMgr()->Init( g_pVoiceGameMgrHelper, gpGlobals->maxClients );
	ClearMultiDamage();

	m_flNextVerboseLogOutput = 0.0f;

	m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;
#endif

	ListenForGameEvent( "recalculate_holidays" );

	LoadVoiceCommandScript();
}

CSharedGameRules::~CGameRules()
{
	Assert( s_pGameRules == this );
	if(s_pGameRules == this)
		s_pGameRules = NULL;
}

#ifdef CLIENT_DLL
	#undef CGameRules
#endif

#ifdef CLIENT_DLL

const char *CSharedGameRules::GetVoiceCommandSubtitle( int iMenu, int iItem )
{
	Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return "";

	Assert( iItem >= 0 && iItem < m_VoiceCommandMenus.Element( iMenu ).Count() );
	if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
		return "";

	VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

	Assert( pItem );

	return pItem->m_szSubtitle;
}

// Returns false if no such menu is declared or if it's an empty menu
bool CSharedGameRules::GetVoiceMenuLabels( int iMenu, KeyValues *pKV )
{
	Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return false;

	int iNumItems = m_VoiceCommandMenus.Element( iMenu ).Count();

	for ( int i=0; i<iNumItems; i++ )
	{
		VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( i );

		KeyValues *pLabelKV = new KeyValues( pItem->m_szMenuLabel );

		pKV->AddSubKey( pLabelKV );
	}

	return iNumItems > 0;
}

bool CSharedGameRules::IsBonusChallengeTimeBased( void )
{
	return true;
}

bool CSharedGameRules::IsLocalPlayer( int nEntIndex )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	return ( pLocalPlayer && pLocalPlayer == ClientEntityList().GetEnt( nEntIndex ) );
}

#else

//-----------------------------------------------------------------------------
// Precache game-specific resources
//-----------------------------------------------------------------------------
void CSharedGameRules::Precache( void ) 
{
	// Used by particle property
	PrecacheEffect( "ParticleEffect" );
	PrecacheEffect( "ParticleEffectStop" );

	// Used by default impact system
	PrecacheEffect( "GlassImpact" );
	PrecacheEffect( "Impact" );
	PrecacheEffect( "RagdollImpact" );
	PrecacheEffect( "gunshotsplash"	);
	PrecacheEffect( "TracerSound" );
	PrecacheEffect( "Tracer" );

	// Used by physics impacts
	PrecacheEffect( "watersplash" );
	PrecacheEffect( "waterripple" );

	// Used by UTIL_BloodImpact, which is used in many places
	PrecacheEffect( "bloodimpact" );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CSharedGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, AmmoIndex_t iAmmoIndex )
{
	if ( iAmmoIndex != AMMO_INVALID_INDEX )
	{
		// Get the max carrying capacity for this ammo
		int iMaxCarry = GetAmmoDef()->MaxCarry( iAmmoIndex, pPlayer );

		// Does the player have room for more of this type of ammo?
		if ( pPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CSharedGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName )
{
	return CanHaveAmmo( pPlayer, GetAmmoDef()->Index(szName) );
}

//=========================================================
//=========================================================

//-----------------------------------------------------------------------------
// Purpose: Finds a player start entity of the given classname. If any entity of
//			of the given classname has the SF_PLAYER_START_MASTER flag set, that
//			is the entity that will be returned. Otherwise, the first entity of
//			the given classname is returned.
// Input  : pszClassName - should be "info_player_start", "info_player_coop", or
//			"info_player_deathmatch"
//-----------------------------------------------------------------------------
CBaseEntity *FindPlayerStart(const char *pszClassName)
{
	CBaseEntity *pStartEnt = gEntList.FindEntityByClassname(NULL, pszClassName);
	CBaseEntity *pStartFirst = pStartEnt;
	while (pStartEnt != NULL)
	{
		CPlayerStart *pStart = assert_cast<CPlayerStart *>(pStartEnt);

		if (pStart->HasSpawnFlags(SF_PLAYER_START_MASTER))
		{
			return pStart;
		}

		pStartEnt = gEntList.FindEntityByClassname(pStartEnt, pszClassName);
	}

	return pStartFirst;
}

CBaseEntity *CSharedGameRules::GetPlayerSpawnSpot( CBaseCombatCharacter *pPlayer )
{
	CBaseEntity *pSpot;

// choose a info_player_deathmatch point
	if (IsCoOp())
	{
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_coop");
		if ( pSpot )
			goto ReturnSpot;
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_start");
		if ( pSpot ) 
			goto ReturnSpot;
	}
	else if ( IsDeathmatch() )
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for ( int i = random_valve->RandomInt(1,5); i > 0; i-- )
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( !pSpot )  // skip over the null point
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( !pPlayer || IsSpawnPointValid( pSpot, pPlayer ) )
				{
					if ( pSpot->GetLocalOrigin() == vec3_origin )
					{
						pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( pSpot )
		{
			CBaseEntity *ent = NULL;
			for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsCombatCharacter() && ent != pPlayer )
					ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if ( !gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = FindPlayerStart( "info_player_start" );
		if ( pSpot )
			goto ReturnSpot;
	}
	else
	{
		pSpot = gEntList.FindEntityByName( NULL, gpGlobals->startspot );
		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( !pSpot  )
	{
		Log_Warning( LOG_GAMERULES,"PutClientInServer: no info_player_start on level\n");
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}

// checks if the spot is clear of players
bool CSharedGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBaseCombatCharacter *pPlayer  )
{
	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return false;
	}

	CBaseEntity *ent = NULL;
	for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsCombatCharacter() && ent != pPlayer )
			return false;
	}

	return true;
}

//=========================================================
//=========================================================
bool CSharedGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	if ( weaponstay.GetInt() > 0 )
	{
		// check if the player already has this weapon
		for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
		{
			if ( pPlayer->GetWeapon(i) == pWeapon )
			{
				return false;
			}
		}
	}

	if ( pWeapon->UsesPrimaryAmmo() )
	{
		if ( !CanHaveAmmo( pPlayer, pWeapon->m_iPrimaryAmmoType ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if ( pPlayer->Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) )
			{
				return FALSE;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if ( pPlayer->Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) )
		{
			return FALSE;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return TRUE;
}

void CSharedGameRules::SetSkillLevel( int iLevel )
{
	int oldLevel = g_iSkillLevel; 

	if ( iLevel < 1 )
	{
		iLevel = 1;
	}
	else if ( iLevel > 3 )
	{
		iLevel = 3; 
	}

	g_iSkillLevel = iLevel;

	if( g_iSkillLevel != oldLevel )
	{
		OnSkillLevelChanged( g_iSkillLevel );
	}
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CSharedGameRules::RefreshSkillData ( bool forceUpdate )
{
	if ( !forceUpdate )
	{
		if ( GlobalEntity_IsInTable( "skill.cfg" ) )
			return;
	}

	GlobalEntity_Add( "skill.cfg", STRING(gpGlobals->mapname), GLOBAL_ON );

	SetSkillLevel( skill->GetInt() );

	char	szExec[256];
	Q_snprintf( szExec,sizeof(szExec), "exec skill%d.cfg\n", GetSkillLevel() );

	engine->ServerCommand( szExec );
	engine->ServerExecute();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsExplosionTraceBlocked( trace_t *ptr )
{
	if( ptr->DidHitWorld() )
		return true;

	if( ptr->m_pEnt == NULL )
		return false;

	if( ptr->m_pEnt->GetMoveType() == MOVETYPE_PUSH )
	{
		// All doors are push, but not all things that push are doors. This 
		// narrows the search before we start to do classname compares.
		if( FClassnameIs(ptr->m_pEnt, "prop_door_rotating") ||
        FClassnameIs(ptr->m_pEnt, "func_door") ||
        FClassnameIs(ptr->m_pEnt, "func_door_rotating") )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Default implementation of radius damage
//-----------------------------------------------------------------------------
#define ROBUST_RADIUS_PROBE_DIST 16.0f // If a solid surface blocks the explosion, this is how far to creep along the surface looking for another way to the target
void CSharedGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, Class_T iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const ContentsFlags_t MASK_RADIUS_DAMAGE = MASK_SHOT & (~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc, MASK_WATER ) & MASK_WATER) ? true : false;

	if( bInWater )
	{
		// Only muffle the explosion if deeper than 2 feet in water.
		if( !(UTIL_PointContents(vecSrc + Vector(0, 0, 24), MASK_WATER) & MASK_WATER) )
		{
			bInWater = false;
		}
	}
	
	vecSrc.z += 1;// in case grenade is lying on the ground

	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// blast's don't tavel into or out of water
		if (bInWater && pEntity->GetWaterLevel() == 0)
			continue;

		if (!bInWater && pEntity->GetWaterLevel() == 3)
			continue;

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			if ( IsExplosionTraceBlocked(&tr) )
			{
				if( ShouldUseRobustRadiusDamage( pEntity ) )
				{
					if( vecSpot.DistToSqr( vecSrc ) > flHalfRadiusSqr )
					{
						// Only use robust model on a target within one-half of the explosion's radius.
						continue;
					}

					Vector vecToTarget = vecSpot - tr.endpos;
					VectorNormalize( vecToTarget );

					// We're going to deflect the blast along the surface that 
					// interrupted a trace from explosion to this target.
					Vector vecUp, vecDeflect;
					CrossProduct( vecToTarget, tr.plane.normal, vecUp );
					CrossProduct( tr.plane.normal, vecUp, vecDeflect );
					VectorNormalize( vecDeflect );

					// Trace along the surface that intercepted the blast...
					UTIL_TraceLine( tr.endpos, tr.endpos + vecDeflect * ROBUST_RADIUS_PROBE_DIST, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 255, 0, false, 10 );

					// ...to see if there's a nearby edge that the explosion would 'spill over' if the blast were fully simulated.
					UTIL_TraceLine( tr.endpos, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, false, 10 );

					if( tr.fraction != 1.0 && tr.DidHitWorld() )
					{
						// Still can't reach the target.
						continue;
					}
					// else fall through
				}
				else
				{
					continue;
				}
			}

			// UNDONE: Probably shouldn't let children block parents either?  Or maybe those guys should set their owner if they want this behavior?
			// HL2 - Dissolve damage is not reduced by interposing non-world objects
			if( tr.m_pEnt && tr.m_pEnt != pEntity && tr.m_pEnt != pEntityIgnore && tr.m_pEnt->GetOwnerEntity() != pEntity )
			{
				// Some entity was hit by the trace, meaning the explosion does not have clear
				// line of sight to the entity that it's trying to hurt. If the world is also
				// blocking, we do no damage.
				CBaseEntity *pBlockingEntity = tr.m_pEnt;
				//Msg( "%s may be blocked by %s...", pEntity->GetClassname(), pBlockingEntity->GetClassname() );

				UTIL_TraceLine( vecSrc, vecSpot, CONTENTS_SOLID, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

				if( tr.fraction != 1.0 )
				{
					continue;
				}
				
				// Now, if the interposing object is physics, block some explosion force based on its mass.
				if( pBlockingEntity->VPhysicsGetObject() )
				{
					const float MASS_ABSORB_ALL_DAMAGE = 350.0f;
					float flMass = pBlockingEntity->VPhysicsGetObject()->GetMass();
					float scale = flMass / MASS_ABSORB_ALL_DAMAGE;

					// Absorbed all the damage.
					if( scale >= 1.0f )
					{
						continue;
					}

					ASSERT( scale > 0.0f );
					flBlockedDamagePercent = scale;
					//Msg("  Object (%s) weighing %fkg blocked %f percent of explosion damage\n", pBlockingEntity->GetClassname(), flMass, scale * 100.0f);
				}
				else
				{
					// Some object that's not the world and not physics. Generically block 25% damage
					flBlockedDamagePercent = 0.25f;
				}
			}
		}

		// decrease damage for an ent that's farther from the bomb.
		float flDistanceToEnt = ( vecSrc - tr.endpos ).Length();
		flAdjustedDamage = flDistanceToEnt * falloff;
		flAdjustedDamage = info.GetDamage() - flAdjustedDamage;

		if ( flAdjustedDamage <= 0 )
		{
			continue;
		}

		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetRadius( flRadius );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

		// Now make a consideration for skill level!
		if( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
		{
			// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
			adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize( dir );

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
		{
			if ( !( adjustedInfo.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) )
			{
				CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
			}
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
			adjustedInfo.SetDamageForce( dir * flForce );
			adjustedInfo.SetDamagePosition( vecSrc );
		}

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );

		if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && ToBaseCombatCharacter( tr.m_pEnt ) )
		{
			// This is a total hack!!!
			bool bIsPrimary = true;
			CBasePlayer *player = ToBasePlayer( info.GetAttacker() );
			CBaseCombatWeapon *pWeapon = player->GetActiveWeapon();

			gamestats->Event_WeaponHit( player, bIsPrimary, (pWeapon != NULL) ? pWeapon->GetClassname() : "NULL", info );
		}
	}
}

void CSharedGameRules::FrameUpdatePostEntityThink()
{
	VPROF( "CSharedGameRules::FrameUpdatePostEntityThink" );
	Think();

	float flNow = Plat_FloatTime();

	// Update time when client was last connected
	if ( m_flTimeLastMapChangeOrPlayerWasConnected <= 0.0f )
	{
		m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
	}
	else
	{
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			player_info_t pi;
			if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
				continue;
#if defined( REPLAY_ENABLED )
			if ( pi.ishltv || pi.isreplay || pi.fakeplayer )
#else
			if ( pi.ishltv || pi.fakeplayer )
#endif
				continue;

			m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
			break;
		}
	}

	// Check if we should cycle the map because we've been empty
	// for long enough
	if ( mp_mapcycle_empty_timeout_seconds.GetInt() > 0 )
	{
		int iIdleSeconds = (int)( flNow - m_flTimeLastMapChangeOrPlayerWasConnected );
		if ( iIdleSeconds >= mp_mapcycle_empty_timeout_seconds.GetInt() )
		{

			Log( "Server has been empty for %d seconds on this map, cycling map as per mp_mapcycle_empty_timeout_seconds\n", iIdleSeconds );
			ChangeLevel();
		}
	}
}

float CSharedGameRules::GetMapRemainingTime() const
{
	if(mp_timelimit.GetInt() <= 0) {
		return 0;
	}

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f) - gpGlobals->curtime;
	return timeleft;
}

static const char *s_PreserveEnts[] = {
	"gamerules_proxy",
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
	"worldspawn",
};

class CMapEntityFilter : public IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity(const char *pClassname)
	{
		if(!FindInList(s_PreserveEnts, pClassname, ARRAYSIZE(s_PreserveEnts))) {
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

void CSharedGameRules::CleanUpMap()
{
	CBaseEntity *pCur = gEntList.FirstEnt();
	while(pCur) {
		CBaseCombatWeapon *pWeapon = pCur->MyCombatWeaponPointer();
		if(pWeapon) {
			if(!pWeapon->GetPlayerOwner()) {
				UTIL_Remove(pCur);
			}
		} else if(!FindInList(s_PreserveEnts, pCur->GetClassname(), ARRAYSIZE(s_PreserveEnts))) {
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	gEntList.CleanupDeleteList();

	g_EventQueue.Clear();

	CMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	MapEntity_ParseAllEntities(engine->GetMapEntitiesString(), &filter, true);
}

void CSharedGameRules::CheckRestartGame()
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
}

void CSharedGameRules::Think()
{
	GetVoiceGameMgr()->Update( gpGlobals->frametime );

	SetSkillLevel( skill->GetInt() );

	gpGlobals->coop = IsCoOp();
	gpGlobals->deathmatch = IsDeathmatch();
	gpGlobals->teamplay = IsTeamplay();

	if ( log_verbose_enable.GetBool() )
	{
		if ( m_flNextVerboseLogOutput < gpGlobals->curtime )
		{
			ProcessVerboseLogOutput();
			m_flNextVerboseLogOutput = gpGlobals->curtime + log_verbose_interval.GetFloat();
		}
	}

	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		if(m_flIntermissionEndTime < gpGlobals->curtime) {
			if(!m_bChangelevelDone) {
				ChangeLevel();
				m_bChangelevelDone = true;
			}
		}
		return;
	}
	
	if(GetMapRemainingTime() < 0)
	{
		GoToIntermission();
		return;
	}

	if(gpGlobals->curtime > m_tmNextPeriodicThink) {
		CheckRestartGame();
		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	if(m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime) {
		RestartGame();
	}

	float flFragLimit = fraglimit.GetFloat();
	if ( flFragLimit )
	{
		// check if any player is over the frag limit
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
	}

	ManageObjectRelocation();
}

//=========================================================
//AddLevelDesignerPlacedWeapon
//=========================================================
void CSharedGameRules::AddLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) == -1 )
	{
		m_hRespawnableItemsAndWeapons.AddToTail( pEntity );
	}
}

//=========================================================
//RemoveLevelDesignerPlacedWeapon
//=========================================================
void CSharedGameRules::RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) != -1 )
	{
		m_hRespawnableItemsAndWeapons.FindAndRemove( pEntity );
	}
}

bool CSharedGameRules::CheckGameOver()
{
	if(g_fGameOver) {
		if(m_flIntermissionEndTime < gpGlobals->curtime) {
			ChangeLevel();
		}

		return true;
	}

	return false;
}

void CSharedGameRules::RestartGame()
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
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if(!pPlayer) {
			continue;
		}

		if(pPlayer->GetActiveWeapon()) {
			pPlayer->GetActiveWeapon()->Holster();
		}

		pPlayer->RemoveAllItems(true);
		respawn(pPlayer, false);
		pPlayer->ResetScores();
	}

	int iNumTeams = GetNumberOfTeams();
	for(int i = 0; i < iNumTeams; ++i) {
		CTeam *pTeam = GetGlobalTeamByIndex( i );
		if ( pTeam )
		{
			pTeam->SetScore( 0 );
		}
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

bool IsManagedObjectAnItem(CBaseEntity *pObject)
{
	return pObject->IsCombatItem();
}

bool IsManagedObjectAWeapon(CBaseEntity *pObject)
{
	return pObject->IsBaseCombatWeapon();
}

bool GetObjectsOriginalParameters(CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles)
{
	if(IsManagedObjectAnItem(pObject)) {
		CItem *pItem = (CItem *)pObject;
		if(pItem->m_flNextResetCheckTime > gpGlobals->curtime) {
			return false;
		}

		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_item_respawn_time.GetFloat();
		return true;
	} else if(IsManagedObjectAWeapon(pObject)) {
		CBaseCombatWeapon *pWeapon = pObject->MyCombatWeaponPointer();
		if(pWeapon->m_flNextResetCheckTime > gpGlobals->curtime ) {
			return false;
		}

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}

void CSharedGameRules::ManageObjectRelocation( void )
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

//-----------------------------------------------------------------------------
// Purpose: Called at the end of GameFrame (i.e. after all game logic has run this frame)
//-----------------------------------------------------------------------------
void CSharedGameRules::EndGameFrame( void )
{
	// If you hit this assert, it means something called AddMultiDamage() and didn't ApplyMultiDamage().
	// The g_MultiDamage.m_hAttacker & g_MultiDamage.m_hInflictor should give help you figure out the culprit.
	Assert( g_MultiDamage.IsClear() );
	if ( !g_MultiDamage.IsClear() )
	{
		Warning("Unapplied multidamage left in the system:\nTarget: %s\nInflictor: %s\nAttacker: %s\nDamage: %.2f\n", 
			g_MultiDamage.GetTarget()->GetDebugName(),
			g_MultiDamage.GetInflictor()->GetDebugName(),
			g_MultiDamage.GetAttacker()->GetDebugName(),
			g_MultiDamage.GetDamage() );
		ApplyMultiDamage();
	}
}

void CSharedGameRules::OnSkillLevelChanged( int iNewLevel )
{
	variant_t varNewLevel;
	varNewLevel.SetInt(iNewLevel);

	// Iterate through all logic_skill entities and fire them
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "logic_skill");
	while (pEntity)
	{
		pEntity->AcceptInput("SkillLevelChanged", NULL, NULL, varNewLevel, 0);
		pEntity = gEntList.FindEntityByClassname(pEntity, "logic_skill");
	}

	// Fire game event for difficulty level changed
	IGameEvent *event = gameeventmanager->CreateEvent("skill_changed");
	if (event)
	{
		event->SetInt("skill_level", iNewLevel);
		gameeventmanager->FireEvent(event);
	}
}

//-----------------------------------------------------------------------------
// trace line rules
//-----------------------------------------------------------------------------
float CSharedGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
					 ContentsFlags_t mask, trace_t *ptr )
{
	UTIL_TraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
	return 1.0f;
}

void CSharedGameRules::CreateStandardEntities()
{
	if(g_pGameRulesProxy) {
		UTIL_Remove(g_pGameRulesProxy);
	}
	g_pGameRulesProxy = (CGameRulesProxy *)CBaseEntity::Create( "gamerules_proxy", vec3_origin, vec3_angle );

	if(g_pPlayerResource) {
		UTIL_Remove(g_pPlayerResource);
	}
	g_pPlayerResource = (CPlayerResource *)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );

	int iNumTeams = NumTeams();
	if(iNumTeams < NUM_SHARED_TEAMS) {
		iNumTeams = NUM_SHARED_TEAMS;
	}
	for(int i = 0; i < iNumTeams; ++i) {
		const char *pTeamName = GetIndexedTeamName((Team_t)i);

		CTeam *pTeam = static_cast<CTeam *>(CBaseEntity::Create("team_manager", vec3_origin, vec3_angle));
		pTeam->Init(pTeamName, (Team_t)i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inform client(s) they can mark the indicated achievement as completed (SERVER VERSION)
// Input  : filter - which client(s) to send this to
//			iAchievementID - The enumeration value of the achievement to mark (see TODO:Kerry, what file will have the mod's achievement enum?) 
//-----------------------------------------------------------------------------
void CSharedGameRules::MarkAchievement( IRecipientFilter& filter, char const *pchAchievementName )
{
	gamestats->Event_IncrementCountedStatistic( vec3_origin, pchAchievementName, 1.0f );

	IAchievementMgr *pAchievementMgr = engine->GetAchievementMgr();
	if ( !pAchievementMgr )
		return;
	pAchievementMgr->OnMapEvent( pchAchievementName );
}

const char *CSharedGameRules::GetChatFormat(bool bTeamOnly, CBasePlayer *pPlayer)
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
				pszFormat = "Chat_Team_Loc";
			} else {
				pszFormat = "Chat_Team";
			}
		}
	} else {
		if(pPlayer->GetTeamNumber() != TEAM_SPECTATOR) {
			pszFormat = "Chat_All";
		} else {
			pszFormat = "Chat_AllSpec";
		}
	}

	return pszFormat;
}

bool CSharedGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	if(pPlayer->GetActiveWeapon() && pPlayer->IsNetClient())
	{
		const char *cl_autowepswitch = engine->GetClientConVarValue(engine->IndexOfEdict(pPlayer->edict()), "cl_autowepswitch");
		if(cl_autowepswitch && atoi(cl_autowepswitch) <= 0)
		{
			return false;
		}
	}

	//Must have ammo
	if ( ( pWeapon->HasAnyAmmo() == false ) && ( pPlayer->GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) <= 0 ) )
		return false;

	if ( !pPlayer->Weapon_CanSwitchTo( pWeapon ) )
	{
		// Can't switch weapons for some reason.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon() )
	{
		// Player doesn't have an active item, might as well switch.
		return true;
	}

	if ( !pWeapon->AllowsAutoSwitchTo() )
	{
		// The given weapon should not be auto switched to from another weapon.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
	{
		// The active weapon does not allow autoswitching away from it.
		return false;
	}

	//Don't switch if our current gun doesn't want to be holstered
	if ( pPlayer->GetActiveWeapon()->CanHolster() == false )
		return false;

	//Only switch if the weapon is better than what we're using
	if ( ( pWeapon != pPlayer->GetActiveWeapon() ) && ( pWeapon->GetWeight() <= pPlayer->GetActiveWeapon()->GetWeight() ) )
		return false;

	return false;
}

void CSharedGameRules::ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			if( pPlayer->GetTeam() )
				pPlayer->GetTeam()->RemovePlayer(pPlayer);

			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			pPlayer->RemoveAllItems( true );// destroy all of the players weapons and items

			// Kill off view model entities
			pPlayer->DestroyViewModels();

			pPlayer->SetConnected( PlayerDisconnected );
		}
	}
}

float CSharedGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.GetFloat();

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
}

bool CSharedGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	GetVoiceGameMgr()->ClientConnected( pEntity );
	return true;
}

const char *CSharedGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( pPlayer && pPlayer->IsAlive() == false )
	{
		if ( bTeamOnly )
			return "*DEAD*(TEAM)";
		else
			return "*DEAD*";
	}
	
	return "";
}

void CSharedGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		char text[256];
		Q_snprintf( text,sizeof(text), "%s changed name to %s\n", pszOldName, pszName );

		UTIL_ClientPrintAll( HUD_PRINTTALK, text );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	if ( pszFov )
	{
		int iFov = atoi(pszFov);
		iFov = clamp( iFov, 75, 90 );
		pPlayer->SetDefaultFOV( iFov );
	}
}

void CSharedGameRules::SkipNextMapInCycle()
{
	char szSkippedMap[MAX_MAP_NAME];
	char szNextMap[MAX_MAP_NAME];

	GetNextLevelName( szSkippedMap, sizeof( szSkippedMap ) );
	IncrementMapCycleIndex();
	GetNextLevelName( szNextMap, sizeof( szNextMap ) );

	Msg( "Skipping: %s\tNext map: %s\n", szSkippedMap, szNextMap );

	if ( nextlevel.GetString() && *nextlevel.GetString() )
	{
		Msg( "Warning! \"nextlevel\" is set to \"%s\" and will override the next map to be played.\n", nextlevel.GetString() );
	}
}

void CSharedGameRules::IncrementMapCycleIndex()
{
	// Reset index if we've passed the end of the map list
	if ( ++m_nMapCycleindex >= m_MapList.Count() )
	{
		m_nMapCycleindex = 0;
	}
}

bool CSharedGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CBaseExpresserPlayer *pPlayer = ToBaseExpresserPlayer( pEdict );

	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "voicemenu" ) )
	{
		if ( args.ArgC() < 3 )
			return true;

		int iMenu = atoi( args[1] );
		int iItem = atoi( args[2] );

		if(pPlayer)
			VoiceCommand( pPlayer, iMenu, iItem );

		return true;
	}
	else if ( FStrEq( pcmd, "achievement_earned" ) )
	{
		return true;
	}

	if( pPlayer )
	{
		if( GetVoiceGameMgr()->ClientCommand( pPlayer, args ) )
			return true;
	}

	if(pPlayer->ClientCommand(args))
		return true;

	return false;
}

void CSharedGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	CBasePlayer *pPlayer = ToBasePlayer( CBaseEntity::Instance( pEntity ) );

	if ( !pPlayer )
		return;

	char const *pszCommand = pKeyValues->GetName();
	if ( pszCommand && pszCommand[0] )
	{
		if ( FStrEq( pszCommand, "AchievementEarned" ) )
		{
			if ( pPlayer->ShouldAnnounceAchievement() )
			{
				int nAchievementID = pKeyValues->GetInt( "achievementID" );

				IGameEvent * event = gameeventmanager->CreateEvent( "achievement_earned" );
				if ( event )
				{
					event->SetInt( "player", pPlayer->entindex() );
					event->SetInt( "achievement", nAchievementID );
					gameeventmanager->FireEvent( event );
				}

				pPlayer->OnAchievementEarned( nAchievementID );
			}
		}
	}
}

VoiceCommandMenuItem_t *CSharedGameRules::VoiceCommand( CBaseExpresserPlayer *pPlayer, int iMenu, int iItem )
{
	// have the player speak the concept that is in a particular menu slot
	if ( !pPlayer )
		return NULL;

	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return NULL;

	if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
		return NULL;

	VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

	Assert( pItem );

	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if ( pPlayer->CanSpeakVoiceCommand() )
	{
		CAI_Expresser *pExpresser = pPlayer->GetExpresser();
		Assert( pExpresser );
		pExpresser->AllowMultipleScenes();

		if ( pPlayer->SpeakConceptIfAllowed( pItem->m_iConcept, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
		{
			// show a subtitle if we need to
			if ( pItem->m_bShowSubtitle )
			{
				CRecipientFilter filter;

				if ( pItem->m_bDistanceBasedSubtitle )
				{
					filter.AddRecipientsByPAS( pPlayer->WorldSpaceCenter() );

					// further reduce the range to a certain radius
					int i;
					for ( i = filter.GetRecipientCount()-1; i >= 0; i-- )
					{
						int index = filter.GetRecipientIndex(i);

						CBasePlayer *pListener = UTIL_PlayerByIndex( index );

						if ( pListener && pListener != pPlayer )
						{
							float flDist = ( pListener->WorldSpaceCenter() - pPlayer->WorldSpaceCenter() ).Length2D();

							if ( flDist > VOICE_COMMAND_MAX_SUBTITLE_DIST )
								filter.RemoveRecipientByPlayerIndex( index );
						}
					}
				}
				else
				{
					filter.AddAllPlayers();
				}

				// if we aren't a disguised spy
				if ( !pPlayer->ShouldShowVoiceSubtitleToEnemy() )
				{
					// remove players on other teams
					filter.RemoveRecipientsNotOnTeam( pPlayer->GetTeam() );
				}

				// Register this event in the mod-specific usermessages .cpp file if you hit this assert
				Assert( usermessages->LookupUserMessage( "VoiceSubtitle" ) != -1 );

				// Send a subtitle to anyone in the PAS
				UserMessageBegin( filter, "VoiceSubtitle" );
					WRITE_BYTE( pPlayer->entindex() );
					WRITE_BYTE( iMenu );
					WRITE_BYTE( iItem );
				MessageEnd();
			}

			pPlayer->NoteSpokeVoiceCommand( szResponse );
		}
		else
		{
			pItem = NULL;
		}

		pExpresser->DisallowMultipleScenes();
		return pItem;
	}

	return NULL;
}

bool CSharedGameRules::IsLoadingBugBaitReport()
{
#ifndef SWDS
	return ( !engine->IsDedicatedServer() && CommandLine()->CheckParm( "-bugbait" ) && sv_cheats->GetBool() );
#else
	return false;
#endif
}

void CSharedGameRules::HaveAllPlayersSpeakConceptIfAllowed( int iConcept, int iTeam /* = TEAM_UNASSIGNED */, const char *modifiers /* = NULL */ )
{
	CBaseExpresserPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBaseExpresserPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( iTeam != TEAM_UNASSIGNED )
		{
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		pPlayer->SpeakConceptIfAllowed( iConcept, modifiers );
	}
}

void CSharedGameRules::RandomPlayersSpeakConceptIfAllowed( int iConcept, int iNumRandomPlayer /*= 1*/, int iTeam /*= TEAM_UNASSIGNED*/, const char *modifiers /*= NULL*/ )
{
	CUtlVector< CBaseExpresserPlayer* > speakCandidates;

	CBaseExpresserPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBaseExpresserPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( iTeam != TEAM_UNASSIGNED )
		{
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		speakCandidates.AddToTail( pPlayer );
	}

	int iSpeaker = iNumRandomPlayer;
	while ( iSpeaker > 0 && speakCandidates.Count() > 0 )
	{
		int iRandomSpeaker = RandomInt( 0, speakCandidates.Count() - 1 );
		speakCandidates[ iRandomSpeaker ]->SpeakConceptIfAllowed( iConcept, modifiers );
		speakCandidates.FastRemove( iRandomSpeaker );
		iSpeaker--;
	}
}

void CSharedGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	// sv_gravity
	KeyValues *pGravity = new KeyValues( "sv_gravity" );
	pGravity->SetString( "convar", "sv_gravity" );
	pGravity->SetString( "tag", "gravity" );

	pCvarTagList->AddSubKey( pGravity );

	// sv_alltalk
	KeyValues *pAllTalk = new KeyValues( "sv_alltalk" );
	pAllTalk->SetString( "convar", "sv_alltalk" );
	pAllTalk->SetString( "tag", "alltalk" );

	pCvarTagList->AddSubKey( pAllTalk );
}

void CSharedGameRules::PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = IN_NONE;
		pPlayer->m_nButtons = IN_NONE;
		pPlayer->m_afButtonReleased = IN_NONE;
	}
}

void CSharedGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	bool		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	addDefault = true;

	while ( (pWeaponEntity = gEntList.FindEntityByClassname( pWeaponEntity, "game_player_equip" )) != NULL)
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = false;
	}
}

CBasePlayer *CSharedGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor )
{
	if ( pKiller)
	{
		if ( pKiller->IsPlayer() )
			return (CBasePlayer*)pKiller;

		// Killing entity might be specifying a scorer player
		IScorer *pScorerInterface = dynamic_cast<IScorer*>( pKiller );
		if ( pScorerInterface )
		{
			CBasePlayer *pPlayer = pScorerInterface->GetScorer();
			if ( pPlayer )
				return pPlayer;
		}

		// Inflicting entity might be specifying a scoring player
		pScorerInterface = dynamic_cast<IScorer*>( pInflictor );
		if ( pScorerInterface )
		{
			CBasePlayer *pPlayer = pScorerInterface->GetScorer();
			if ( pPlayer )
				return pPlayer;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns player who should receive credit for kill
//-----------------------------------------------------------------------------
CBasePlayer *CSharedGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	// if this method not overridden by subclass, just call our default implementation
	return GetDeathScorer( pKiller, pInflictor );
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CSharedGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	if(IsIntermission()) {
		return;
	}

	DeathNotice( pVictim, info );

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
	
	pVictim->IncrementDeathCount( 1 );

	// dvsents2: uncomment when removing all FireTargets
	// variant_t value;
	// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	// Did the player kill himself?
	if ( pVictim == pScorer )  
	{			
		if ( UseSuicidePenalty() )
		{
			// Players lose a frag for killing themselves
			pVictim->IncrementFragCount( -1 );
		}			
	}
	else if ( pScorer )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pScorer->IncrementFragCount( IPointsForKill( pScorer, pVictim ) );
		
		// Allow the scorer to immediately paint a decal
		pScorer->AllowImmediateDecalPainting();

		// dvsents2: uncomment when removing all FireTargets
		//variant_t value;
		//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
		FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
	}
	else
	{  
		if ( UseSuicidePenalty() )
		{
			// Players lose a frag for letting the world kill them			
			pVictim->IncrementFragCount( -1 );
		}					
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CSharedGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );

	// Custom damage type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetDeathNoticeName();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( V_strnicmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( V_strnicmp( killer_weapon_name, "NPC_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( V_strnicmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( V_strstr( killer_weapon_name, "physics" ) != NULL )
		{
			killer_weapon_name = "physics";
		}
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
	if ( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetInt("customkill", info.GetDamageCustom() );
		event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
		event->SetString("weapon", killer_weapon_name );
		gameeventmanager->FireEvent( event );
	}

}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CSharedGameRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->curtime + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->curtime + sv_weapon_respawn_time.GetFloat();
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CSharedGameRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CSharedGameRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
	return pWeapon->GetOriginalSpawnOrigin();
}

Vector CSharedGameRules::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CSharedGameRules::VecItemRespawnAngles(CItem *pItem)
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CSharedGameRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon->HasSpawnFlags( SF_WEAPON_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

bool CSharedGameRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
	return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
}

int CSharedGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if(!pPlayer || !pTarget) {
		return GR_NOTTEAMMATE;
	}

	if((*GetTeamID(pPlayer) == '\0') || (*GetTeamID(pTarget) == '\0')) {
		return GR_NOTTEAMMATE;
	}

	if(V_stricmp(GetTeamID(pPlayer), GetTeamID(pTarget)) == 0) {
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

bool CSharedGameRules::PlayFootstepSounds( CBasePlayer *pl )
{
	if ( footsteps.GetInt() == 0 )
		return false;

	if ( pl->IsOnLadder() || pl->GetAbsVelocity().Length2D() > 220 )
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CSharedGameRules::FAllowFlashlight( void ) 
{ 
	return flashlight.GetInt() != 0; 
}

//=========================================================
//=========================================================
bool CSharedGameRules::FAllowNPCs( void )
{
	return ( allowNPCs.GetInt() != 0 );
}

//=========================================================
//======== CMultiplayRules private functions ===========

void CSharedGameRules::GoToIntermission( void )
{
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	float flWaitTime = mp_chattime.GetInt();

	if ( tv_delaymapchange.GetBool() )
	{
		if ( HLTVDirector()->IsActive() )	
			flWaitTime = MAX( flWaitTime, HLTVDirector()->GetDelay() );
#if defined( REPLAY_ENABLED )
		else if ( ReplayDirector()->IsActive() )
			flWaitTime = MAX ( flWaitTime, ReplayDirector()->GetDelay() );
#endif
	}

	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );
	}

	// Tell the clients to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	UTIL_CalculateHolidays();
}

// Strip ' ' and '\n' characters from string.
static void StripWhitespaceChars( char *szBuffer )
{
	char *szOut = szBuffer;

	for ( char *szIn = szOut; *szIn; szIn++ )
	{
		if ( *szIn != ' ' && *szIn != '\r' )
			*szOut++ = *szIn;
	}
	*szOut = '\0';
}

void CSharedGameRules::GetNextLevelName( char *pszNextMap, int bufsize, bool bRandom /* = false */ )
{
	char mapcfile[MAX_PATH];
	DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

	// Check the time of the mapcycle file and re-populate the list of level names if the file has been modified
	const int nMapCycleTimeStamp = g_pFullFileSystem->GetPathTime( mapcfile, "GAME" );

	if ( 0 == nMapCycleTimeStamp )
	{
		// Map cycle file does not exist, make a list containing only the current map
		char *szCurrentMapName = new char[MAX_MAP_NAME];
		Q_strncpy( szCurrentMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
		m_MapList.AddToTail( szCurrentMapName );
	}
	else
	{
		// If map cycle file has changed or this is the first time through ...
		if ( m_nMapCycleTimeStamp != nMapCycleTimeStamp )
		{
			// Reload
			LoadMapCycleFile();
		}
	}

	// If somehow we have no maps in the list then add the current one
	if ( 0 == m_MapList.Count() )
	{
		char *szDefaultMapName = new char[MAX_MAP_NAME];
		Q_strncpy( szDefaultMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
		m_MapList.AddToTail( szDefaultMapName );
	}

	if ( bRandom )
	{
		m_nMapCycleindex = RandomInt( 0, m_MapList.Count() - 1 );
	}

	// Here's the return value
	Q_strncpy( pszNextMap, m_MapList[m_nMapCycleindex], bufsize);
}

void CSharedGameRules::DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew )
{
	static char szLastResult[ MAX_PATH ];

	const char *pszVar = mapcyclefile.GetString();
	if ( *pszVar == '\0' )
	{
		if ( bForceSpew || V_stricmp( szLastResult, "__novar") )
		{
			Msg( "mapcyclefile convar not set.\n" );
			V_strcpy_safe( szLastResult, "__novar" );
		}
		*pszResult = '\0';
		return;
	}

	char szRecommendedName[ MAX_PATH ];
	V_sprintf_safe( szRecommendedName, "cfg/%s", pszVar );

	// First, look for a mapcycle file in the cfg directory, which is preferred
	V_strncpy( pszResult, szRecommendedName, nSizeResult );
	if ( g_pFullFileSystem->FileExists( pszResult, "GAME" ) )
	{
		if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
		{
			Msg( "Using map cycle file '%s'.\n", pszResult );
			V_strcpy_safe( szLastResult, pszResult );
		}
		return;
	}

	// Nope?  Try the root.
	V_strncpy( pszResult, pszVar, nSizeResult );
	if ( g_pFullFileSystem->FileExists( pszResult, "GAME" ) )
	{
		if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
		{
			Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
			V_strcpy_safe( szLastResult, pszResult );
		}
		return;
	}

	// Nope?  Use the default.
	if ( !V_stricmp( pszVar, "mapcycle.txt" ) )
	{
		V_strncpy( pszResult, "cfg/mapcycle_default.txt", nSizeResult );
		if ( g_pFullFileSystem->FileExists( pszResult, "GAME" ) )
		{
			if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
			{
				Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
				V_strcpy_safe( szLastResult, pszResult );
			}
			return;
		}
	}

	// Failed
	*pszResult = '\0';
	if ( bForceSpew || V_stricmp( szLastResult, "__notfound") )
	{
		Log_Msg( LOG_GAMERULES,"Map cycle file '%s' was not found.\n", szRecommendedName );
		V_strcpy_safe( szLastResult, "__notfound" );
	}
}

void CSharedGameRules::LoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
{
	CSharedGameRules::RawLoadMapCycleFileIntoVector( pszMapCycleFile, mapList );
}

void CSharedGameRules::RawLoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
{
	CUtlBuffer buf;
	if ( !g_pFullFileSystem->ReadFile( pszMapCycleFile, "GAME", buf ) )
		return;
	buf.PutChar( 0 );
	V_SplitString( (char*)buf.Base(), "\n", mapList );

	for ( int i = 0; i < mapList.Count(); i++ )
	{
		bool bIgnore = false;

		// Strip out ' ' and '\r' chars.
		StripWhitespaceChars( mapList[i] );

		if ( !Q_strncmp( mapList[i], "//", 2 ) || mapList[i][0] == '\0' )
		{
			bIgnore = true;
		}

		if ( bIgnore )
		{
			delete [] mapList[i];
			mapList.Remove( i );
			--i;
		}
	}
}

void CSharedGameRules::FreeMapCycleFileVector( CUtlVector<char *> &mapList )
{
	// Clear out existing map list. Not using Purge() or PurgeAndDeleteAll() because they won't delete [] each element.
	for ( int i = 0; i < mapList.Count(); i++ )
	{
		delete [] mapList[i];
	}

	mapList.RemoveAll();
}

bool CSharedGameRules::IsManualMapChangeOkay( const char **pszReason )
{
	if ( HLTVDirector()->IsActive() && ( HLTVDirector()->GetDelay() >= HLTV_MIN_DIRECTOR_DELAY ) )
	{
		if ( tv_delaymapchange.GetBool() && tv_delaymapchange_protect.GetBool() )
		{
			float flLastEvent = GetLastMajorEventTime();
			if ( flLastEvent > -1 )
			{
				if ( flLastEvent > ( gpGlobals->curtime - ( HLTVDirector()->GetDelay() + 3 ) ) ) // +3 second delay to prevent instant change after a major event
				{
					*pszReason = "\n***WARNING*** Map change blocked. HLTV is broadcasting and has not caught up to the last major game event yet.\nYou can disable this check by setting the value of the server convar \"tv_delaymapchange_protect\" to 0.\n";
					return false;
				}
			}
		}
	}

	return true;
}

bool CSharedGameRules::IsMapInMapCycle( const char *pszName )
{
	for ( int i = 0; i < m_MapList.Count(); i++ )
	{
		if ( V_stricmp( pszName, m_MapList[i] ) == 0 )
		{
			return true;
		}
	}	

	return false;
}

void CSharedGameRules::ChangeLevel( void )
{
	char szNextMap[MAX_MAP_NAME];

	if ( nextlevel.GetString() && *nextlevel.GetString() )
	{
		Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
	}
	else
	{
		GetNextLevelName( szNextMap, sizeof(szNextMap) );
		IncrementMapCycleIndex();
	}

	ChangeLevelToMap( szNextMap );
}

bool CSharedGameRules::IsOfficialMap() const
{
	return false;
}

void CSharedGameRules::LoadMapCycleFile( void )
{
	int nOldCycleIndex = m_nMapCycleindex;
	m_nMapCycleindex = 0;

	char mapcfile[MAX_PATH];
	DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

	FreeMapCycleFileVector( m_MapList );

	const int nMapCycleTimeStamp = g_pFullFileSystem->GetPathTime( mapcfile, "GAME" );
	m_nMapCycleTimeStamp = nMapCycleTimeStamp;

	// Repopulate map list from mapcycle file
	LoadMapCycleFileIntoVector( mapcfile, m_MapList );

	// Load server's mapcycle into network string table for client-side voting
	if ( g_pStringTableServerMapCycle )
	{
		CUtlString sFileList;
		for ( int i = 0; i < m_MapList.Count(); i++ )
		{
			sFileList += m_MapList[i];
			sFileList += '\n';
		}

		g_pStringTableServerMapCycle->AddString( true, "ServerMapCycle", sFileList.Length() + 1, sFileList.String() );
	}

	// If the current map is in the same location in the new map cycle, keep that index. This gives better behavior
	// when reloading a map cycle that has the current map in it multiple times.
	int nOldPreviousMap = ( nOldCycleIndex == 0 ) ? ( m_MapList.Count() - 1 ) : ( nOldCycleIndex - 1 );
	if ( nOldCycleIndex >= 0 && nOldCycleIndex < m_MapList.Count() &&
	     nOldPreviousMap >= 0 && nOldPreviousMap < m_MapList.Count() &&
	     V_strcmp( STRING( gpGlobals->mapname ), m_MapList[ nOldPreviousMap ] ) == 0 )
	{
		// The old index is still valid, and falls after our current map in the new cycle, use it
		m_nMapCycleindex = nOldCycleIndex;
	}
	else
	{
		// Otherwise, if the current map selection is in the list, set m_nMapCycleindex to the map that follows it.
		for ( int i = 0; i < m_MapList.Count(); i++ )
		{
			if ( V_strcmp( STRING( gpGlobals->mapname ), m_MapList[i] ) == 0 )
			{
				m_nMapCycleindex = i;
				IncrementMapCycleIndex();
				break;
			}
		}
	}
}

void CSharedGameRules::ChangeLevelToMap( const char *pszMap )
{
	g_fGameOver = true;
	m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;
	Msg( "CHANGE LEVEL: %s\n", pszMap );
	engine->ChangeLevel( pszMap, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper allowing gamerules to change the way client-finding in PVS works
//-----------------------------------------------------------------------------
edict_t *CSharedGameRules::DoFindClientInPVS( edict_t *pEdict, unsigned char *pvs, unsigned pvssize )
{
	return UTIL_FindClientInPVSGuts( pEdict, pvs, pvssize );
}

void CSharedGameRules::InitDefaultAIRelationships()
{
	CBaseCombatCharacter::AllocateDefaultRelationships();
	CBaseCombatCharacter::AllocateDefaultFactionRelationships();

	int iNumClasses = NumEntityClasses();
	for(int i = 0; i < iNumClasses; ++i) {
		for(int j = 0; j < iNumClasses; ++j) {
			CBaseCombatCharacter::SetDefaultRelationship((Class_T)i, (Class_T)j, D_NU, 0);
		}
	}

	int iNumFactions = NumFactions();
	for(int i = 0; i < iNumFactions; ++i) {
		for(int j = 0; j < iNumFactions; ++j) {
			CBaseCombatCharacter::SetDefaultFactionRelationship((Faction_T)i, (Faction_T)j, D_NU, 0);
		}
	}
}

Team_t CSharedGameRules::GetTeamIndex( const char *pTeamName )
{
	if(!pTeamName || *pTeamName == '\0') {
		return TEAM_INVALID;
	}

	int numTeams = GetNumberOfTeams();
	for(int i = 0; i < numTeams; ++i) {
		CTeam *pTeam = GetGlobalTeamByIndex( i );
		if(pTeam) {
			if(FStrEq(pTeamName, pTeam->GetName())) {
				return (Team_t)i;
			}
		}
	}

	if(FStrEq(pTeamName, "Unassigned") || FStrEq(pTeamName, "Neutral")) {
		return TEAM_UNASSIGNED;
	} else if(FStrEq(pTeamName, "Spectator")) {
		return TEAM_SPECTATOR;
	}

	return TEAM_INVALID;
}

const char *CSharedGameRules::GetIndexedTeamName( Team_t teamIndex )
{
	CTeam *pTeam = GetGlobalTeamByTeam( teamIndex );
	if(pTeam) {
		return pTeam->GetName();
	}

	switch(teamIndex) {
	case TEAM_UNASSIGNED:
		return "Unassigned";
	case TEAM_SPECTATOR:
		return "Spectator";
	default:
		return NULL;
	}
}

void CSharedGameRules::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib )
{
	Team_t index = GetTeamIndex(pTeamName);
	if(index == TEAM_INVALID) {
		index = TEAM_SPECTATOR;
	}

	if(bKill) {
		pPlayer->CommitSuicide(bGib, true);
	}

	pPlayer->ChangeTeam(index);
}

const char* CSharedGameRules::AIClassText(Class_T classType)
{
	switch (classType)
	{
		case CLASS_NONE:			return "CLASS_NONE";
		case CLASS_PLAYER:			return "CLASS_PLAYER";
		default:					return "MISSING CLASS in AIClassText()";
	}
}

const char* CSharedGameRules::AIFactionText(Faction_T classType)
{
	switch (classType)
	{
		case FACTION_NONE:			return "FACTION_NONE";
		default:					return "MISSING FACTION in AIFactionText()";
	}
}

void CSharedGameRules::InitHUD( CBasePlayer *pl )
{
} 

//=========================================================
//=========================================================
bool CSharedGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	return true;
}

//=========================================================
//=========================================================
bool CSharedGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	return true;
}

//=========================================================
//=========================================================
bool CSharedGameRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return true;
}

//=========================================================
//=========================================================
float CSharedGameRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->curtime;//now!
}

bool CSharedGameRules::AllowAutoTargetCrosshair( void )
{
	return ( aimcrosshair.GetInt() != 0 );
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CSharedGameRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
//=========================================================
bool CSharedGameRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return true;
}

//=========================================================
//=========================================================
void CSharedGameRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CSharedGameRules::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->HasSpawnFlags( SF_ITEM_NORESPAWN ) )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CSharedGameRules::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->curtime + sv_item_respawn_time.GetFloat();
}

//=========================================================
//=========================================================
void CSharedGameRules::PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
bool CSharedGameRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return true;
}


//=========================================================
//=========================================================
float CSharedGameRules::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CSharedGameRules::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CSharedGameRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CSharedGameRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

//-----------------------------------------------------------------------------
// Gets our global friendly fire override.
//-----------------------------------------------------------------------------
ThreeState_t CSharedGameRules::GlobalFriendlyFire()
{
	return GlobalEntity_IsInTable(FRIENDLY_FIRE_GLOBALNAME) ? TO_THREESTATE(GlobalEntity_GetState(FRIENDLY_FIRE_GLOBALNAME)) : TRS_NONE;
}

//-----------------------------------------------------------------------------
// Sets our global friendly fire override.
//-----------------------------------------------------------------------------
void CSharedGameRules::SetGlobalFriendlyFire(ThreeState_t val)
{
	GlobalEntity_Add(MAKE_STRING(FRIENDLY_FIRE_GLOBALNAME), gpGlobals->mapname, (GLOBALESTATE)val);
}

#endif // !CLIENT_DLL

void CSharedGameRules::FireGameEvent( IGameEvent *event )
{
#ifdef CLIENT_DLL
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "recalculate_holidays" ) )
	{
		UTIL_CalculateHolidays();
	}
#endif
}

void CSharedGameRules::LoadVoiceCommandScript( void )
{
	KeyValues *pKV = new KeyValues( "VoiceCommands" );

	if ( pKV->LoadFromFile( g_pFullFileSystem, "scripts/voicecommands.txt", "GAME" ) )
	{
		for ( KeyValues *menu = pKV->GetFirstSubKey(); menu != NULL; menu = menu->GetNextKey() )
		{
			int iMenuIndex = m_VoiceCommandMenus.AddToTail();

			int iNumItems = 0;

			// for each subkey of this menu, add a menu item
			for ( KeyValues *menuitem = menu->GetFirstSubKey(); menuitem != NULL; menuitem = menuitem->GetNextKey() )
			{
				iNumItems++;

				if ( iNumItems > 9 )
				{
					Warning( "Trying to load more than 9 menu items in voicecommands.txt, extras ignored" );
					continue;
				}

				VoiceCommandMenuItem_t item;

#ifndef CLIENT_DLL
				int iConcept = GetMPConceptIndexFromString( menuitem->GetString( "concept", "" ) );
				if ( iConcept == MP_CONCEPT_NONE )
				{
					Warning( "Voicecommand script attempting to use unknown concept. Need to define new concepts in code. ( %s )\n", menuitem->GetString( "concept", "" ) );
				}
				item.m_iConcept = iConcept;

				item.m_bShowSubtitle = ( menuitem->GetInt( "show_subtitle", 0 ) > 0 );
				item.m_bDistanceBasedSubtitle = ( menuitem->GetInt( "distance_check_subtitle", 0 ) > 0 );

				Q_strncpy( item.m_szGestureActivity, menuitem->GetString( "activity", "" ), sizeof( item.m_szGestureActivity ) ); 
#else
				Q_strncpy( item.m_szSubtitle, menuitem->GetString( "subtitle", "" ), MAX_VOICE_COMMAND_SUBTITLE );
				Q_strncpy( item.m_szMenuLabel, menuitem->GetString( "menu_label", "" ), MAX_VOICE_COMMAND_SUBTITLE );

#endif
				m_VoiceCommandMenus.Element( iMenuIndex ).AddToTail( item );
			}
		}
	}

	pKV->deleteThis();
}

// ----------------------------------------------------------------------------- //
// Shared CGameRules implementation.
// ----------------------------------------------------------------------------- //

void CSharedGameRules::LevelShutdown( void )
{
	g_Teams.Purge();
}

bool CSharedGameRules::SwitchToNextBestWeapon( CSharedBaseCombatCharacter *pPlayer, CSharedBaseCombatWeapon *pCurrentWeapon )
{
	CSharedBaseCombatWeapon *pWeapon = GetNextBestWeapon( pPlayer, pCurrentWeapon );

	if ( pWeapon != NULL )
		return pPlayer->Weapon_Switch( pWeapon ) != WEAPON_SWITCH_FAILED;
	
	return false;
}

CSharedBaseCombatWeapon *CSharedGameRules::GetNextBestWeapon( CSharedBaseCombatCharacter *pPlayer, CSharedBaseCombatWeapon *pCurrentWeapon )
{
	CSharedBaseCombatWeapon *pCheck;
	CSharedBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

	int iCurrentWeight = -1;
	int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	// If I have a weapon, make sure I'm allowed to holster it
	if ( pCurrentWeapon )
	{
		if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
		{
			// Either this weapon doesn't allow autoswitching away from it or I
			// can't put this weapon away right now, so I can't switch.
			return NULL;
		}

		iCurrentWeight = pCurrentWeapon->GetWeight();
	}

	for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
	{
		pCheck = pPlayer->GetWeapon( i );
		if ( !pCheck )
			continue;

		// If we have an active weapon and this weapon doesn't allow autoswitching away
		// from another weapon, skip it.
		if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
			continue;

		if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
		{
			// this weapon is from the same category. 
			if ( pCheck->HasAnyAmmo() )
			{
				if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
				{
					return pCheck;
				}
			}
		}
		else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
		{
			//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
			// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
			// that the player was using. This will end up leaving the player with his heaviest-weighted 
			// weapon. 
			if ( pCheck->HasAnyAmmo() )
			{
				// if this weapon is useable, flag it as the best
				iBestWeight = pCheck->GetWeight();
				pBest = pCheck;
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	return pBest;
}

//=========================================================
//=========================================================
bool CSharedGameRules::IsDeathmatch( void )
{
	return deathmatch->GetInt() != 0;
}

//=========================================================
//=========================================================
bool CSharedGameRules::IsCoOp( void )
{
	return coop->GetInt() != 0;
}

//=========================================================
//=========================================================
bool CSharedGameRules::IsTeamplay( void )
{
	return teamplay.GetInt() != 0;
}

bool CSharedGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		::V_swap(collisionGroup0,collisionGroup1);
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

	if((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) && collisionGroup1 == COLLISION_GROUP_WEAPON) {
		return false;
	}

	if(collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER) {
		return false;
	}

	if(collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED) {
		return false;
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		// let debris and multiplayer objects collide
		return true;
	}
	
	// Only let projectile blocking debris collide with projectiles
	if ( collisionGroup0 == COLLISION_GROUP_PROJECTILE && collisionGroup1 == COLLISION_GROUP_DEBRIS_BLOCK_PROJECTILE )
		return true;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS_BLOCK_PROJECTILE || collisionGroup1 == COLLISION_GROUP_DEBRIS_BLOCK_PROJECTILE )
		return false;

	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------

	// Don't bother if either is in a vehicle...
	if (( collisionGroup0 == COLLISION_GROUP_IN_VEHICLE ) || ( collisionGroup1 == COLLISION_GROUP_IN_VEHICLE ))
		return false;

	if ( ( collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER ) && ( collisionGroup0 != COLLISION_GROUP_NPC ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && ( collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR ) )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER )
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}

	// Dissolving guys only collide with COLLISION_GROUP_NONE
	if ( (collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING) )
	{
		if ( collisionGroup0 != COLLISION_GROUP_NONE )
			return false;
	}

	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	// This change was breaking HL2DM
	// Adrian: TEST! Interactive Debris doesn't collide with the player.
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup1 == COLLISION_GROUP_PLAYER ) )
		 return false;

	if ( collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS )
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if ( collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE )
		return false;

	// Projectiles hit everything but debris, weapons, + other projectiles
	if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE )
		{
			return false;
		}
	}

	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE || 
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	// collision with vehicle clip entity??
	if ( collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP )
	{
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE )
			return true;
		// vehicle clip against non-vehicle, no collision
		return false;
	}

	return true;
}


const CViewVectors* CSharedGameRules::GetViewVectors() const
{
	return &g_DefaultViewVectors;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how much damage the given ammo type should do to the victim
//			when fired by the attacker.
// Input  : pAttacker - Dude what shot the gun.
//			pVictim - Dude what done got shot.
//			nAmmoType - What been shot out.
// Output : How much hurt to put on dude what done got shot (pVictim).
//-----------------------------------------------------------------------------
float CSharedGameRules::GetAmmoDamage( CSharedBaseEntity *pAttacker, CSharedBaseEntity *pVictim, AmmoIndex_t nAmmoType )
{
	float flDamage = 0;
	CAmmoDef *pAmmoDef = GetAmmoDef();

	if ( pAttacker && pAttacker->IsPlayer() )
	{
		flDamage = pAmmoDef->PlrDamage( nAmmoType );
	}
	else
	{
		flDamage = pAmmoDef->NPCDamage( nAmmoType );
	}

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected ( user ID )
//-----------------------------------------------------------------------------
bool CSameTeamGroup::Less( const CSameTeamGroup &p1, const CSameTeamGroup &p2 )
{
	// sort by score
	return ( p1.Score() > p2.Score() );
}

CSameTeamGroup::CSameTeamGroup() : 
	m_nScore( INT_MIN )
{
}

CSameTeamGroup::CSameTeamGroup( const CSameTeamGroup &src )
{
	m_nScore = src.m_nScore;
	m_Players = src.m_Players;
}

int CSameTeamGroup::Score() const 
{ 
	return m_nScore; 
}

CSharedBasePlayer *CSameTeamGroup::GetPlayer( int idx )
{
	return m_Players[ idx ];
}

int CSameTeamGroup::Count() const
{
	return m_Players.Count();
}