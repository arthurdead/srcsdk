//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: encapsulates and implements all the accessing of the game dll from external
//			sources (only the engine at the time of writing)
//			This files ONLY contains functions and data necessary to build an interface
//			to external modules
//===========================================================================//

#include "cbase.h"
#include "gamestringpool.h"
#include "mapentities_shared.h"
#include "game.h"
#include "entityapi.h"
#include "client.h"
#include "entitylist.h"
#include "gamerules.h"
#include "soundent.h"
#include "player.h"
#include "server_class.h"
#include "ndebugoverlay.h"
#include "ivoiceserver.h"
#include <stdarg.h>
#include "movehelper_server.h"
#include "networkstringtable_gamedll.h"
#include "filesystem.h"
#include "func_areaportalwindow.h"
#include "igamesystem.h"
#include "init_factory.h"
#include "vstdlib/random.h"
#include "env_wind_shared.h"
#include "engine/IEngineSound.h"
#include "ispatialpartition.h"
#include "textstatsmgr.h"
#include "bitbuf.h"
#include "tier0/vprof.h"
#include "effect_dispatch_data.h"
#include "engine/IStaticPropMgr.h"
#include "TemplateEntities.h"
#include "ai_speech.h"
#include "soundenvelope.h"
#include "usermessages.h"
#include "physics.h"
#include "igameevents.h"
#include "EventLog.h"
#include "datacache/idatacache.h"
#include "engine/ivdebugoverlay.h"
#include "shareddefs.h"
#include "props.h"
#include "timedeventmgr.h"
#include "gameinterface.h"
#include "eventqueue.h"
#include "hltvdirector.h"
#if defined( REPLAY_ENABLED )
#include "replay/iserverreplaycontext.h"
#endif
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ai_responsesystem.h"
#include "util.h"
#include "tier0/icommandline.h"
#include "datacache/imdlcache.h"
#include "engine/iserverplugin.h"
#include "vstdlib/jobthread.h"
#include "functors.h"
#include "foundry/iserverfoundry.h"
#include "entity_tools_server.h"
#include "env_debughistory.h"
#include "player_voice_listener.h"
#include "activitylist.h"
#include "eventlist.h"
#include "ai_schedule.h"
#include "ai_basenpc.h"
#include "ai_squad.h"
#include "world.h"
#include "editor_sendcommand.h"
#include "ModelSoundsCache.h"

#ifndef SWDS
#include "ienginevgui.h"
#include "vgui_gamedll_int.h"
#include "vgui_controls/AnimationController.h"
#endif

#include "ragdoll_shared.h"
#include "toolframework/iserverenginetools.h"
#include "sceneentity.h"
#include "appframework/IAppSystemGroup.h"
#include "scenefilecache/ISceneFileCache.h"
#include "tier2/tier2.h"
#include "particles/particles.h"
#include "gamestats.h"
#include "matchmaking/imatchframework.h"
#include "particle_parse.h"
#include "steam/steam_gameserver.h"
#include "tier3/tier3.h"
#include "serverbenchmark_base.h"
#include "querycache.h"
#include "globalstate.h"
#include "hackmgr/hackmgr.h"
#ifndef SWDS
#include "game_loopback/igameserverloopback.h"
#include "game_loopback/igameclientloopback.h"
#include "game_loopback/igameloopback.h"
#endif
#include "recast/recast_mgr.h"
#include "hackmgr/dlloverride.h"

#ifndef SWDS
#include "IGameUIFuncs.h"
#endif

#ifdef PORTAL
#include "prop_portal_shared.h"
#include "portal_player.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef SWDS
extern IToolFrameworkServer *g_pToolFrameworkServer;
#endif

extern IParticleSystemQuery *g_pParticleSystemQuery;

IUploadGameStats *gamestatsuploader = NULL;

CTimedEventMgr g_NetworkPropertyEventMgr;

// Engine interfaces.
IVEngineServer	*engine = NULL;
IVoiceServer	*g_pVoiceServer = NULL;
INetworkStringTableContainer *networkstringtable = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IUniformRandomStream *random_valve = NULL;
IEngineSound *enginesound = NULL;
ISpatialPartition *partition = NULL;
IVModelInfo *modelinfo = NULL;
IEngineTrace *enginetrace = NULL;
IGameEventManager2 *gameeventmanager = NULL;
#ifndef SWDS
IVDebugOverlay * debugoverlay = NULL;
#endif
IServerPluginHelpers *serverpluginhelpers = NULL;
#ifndef SWDS
IServerEngineTools *serverenginetools = NULL;
IServerFoundry *serverfoundry = NULL;
#endif
ISceneFileCache *scenefilecache = NULL;
IMatchFramework *g_pMatchFramework = NULL;
#if defined( REPLAY_ENABLED )
IReplaySystem *g_pReplay = NULL;
IServerReplayContext *g_pReplayServerContext = NULL;
#endif
#ifndef SWDS
IEngineVGui *enginevgui = NULL;
IGameUIFuncs *gameuifuncs = NULL;
#endif // SERVER_USES_VGUI

#ifndef SWDS
CSysModule* game_loopbackDLL = NULL;
IGameLoopback* g_pGameLoopback = NULL;

CSysModule* clientDLL = NULL;
IGameClientLoopback* g_pGameClientLoopback = NULL;
#endif

CSysModule* vphysicsDLL = NULL;

IGameSystem *SoundEmitterSystem();
void SoundSystemPreloadSounds( void );

void SceneManager_ClientActive( CBasePlayer *player );

class IMaterialSystem;
class IStudioRender;

#ifdef _DEBUG
static ConVar s_UseNetworkVars( "UseNetworkVars", "1", FCVAR_CHEAT, "For profiling, toggle network vars." );
#endif

extern ConVar sv_noclipduringpause;
ConVar sv_massreport( "sv_massreport", "0" );
ConVar sv_force_transmit_ents( "sv_force_transmit_ents", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Will transmit all entities to client, regardless of PVS conditions (will still skip based on transmit flags, however)." );

extern ConVar *sv_maxreplay;
extern ConVar *host_thread_mode;
extern ConVar *hide_server;

#ifndef SWDS
bool g_bTextMode = false;
#endif

// String tables
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;
INetworkStringTable *g_pStringTableEffectDispatch = NULL;
INetworkStringTable *g_pStringTableVguiScreen = NULL;
INetworkStringTable *g_pStringTableMaterials = NULL;
INetworkStringTable *g_pStringTableInfoPanel = NULL;
INetworkStringTable *g_pStringTableClientSideChoreoScenes = NULL;
INetworkStringTable *g_pStringTableServerMapCycle = NULL;
INetworkStringTable *g_pStringTableExtraParticleFiles = NULL;

// Holds global variables shared between engine and game.
CGlobalVars *gpGlobals;
edict_t *g_pDebugEdictBase = 0;
static int		g_nCommandClientIndex = 0;

#ifdef _DEBUG
static ConVar sv_showhitboxes( "sv_showhitboxes", "-1", FCVAR_CHEAT, "Send server-side hitboxes for specified entity to client (NOTE:  this uses lots of bandwidth, use on listen server only)." );
#endif

void PrecachePointTemplates();

static ClientPutInServerOverrideFn g_pClientPutInServerOverride = NULL;

CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

const IChangeInfoAccessor *CBaseEdict::GetChangeAccessor() const
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

void ClientPutInServerOverride( ClientPutInServerOverrideFn fn )
{
	g_pClientPutInServerOverride = fn;
}

ConVar ai_post_frame_navigation( "ai_post_frame_navigation", "0" );
class CPostFrameNavigationHook;
extern CPostFrameNavigationHook *PostFrameNavigationSystem( void );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_GetCommandClientIndex( void )
{
	// -1 == unknown,dedicated server console
	// 0  == player 1

	return g_nCommandClientIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *UTIL_GetCommandClient( void )
{
	int idx = UTIL_GetCommandClientIndex();
	if ( idx != -1 )
	{
		return UTIL_PlayerByIndex( idx+1 );
	}

	// HLDS console issued command
	return NULL;
}

extern void InitializeServerCvars( void );

CBaseEntity*	FindPickerEntity( CBasePlayer* pPlayer );
float			GetFloorZ(const Vector &origin);
void			UpdateAllClientData( void );
void			DrawMessageEntities();

// For now just using one big AI network
extern ConVar think_limit;


#if 0
//-----------------------------------------------------------------------------
// Purpose: Draw output overlays for any measure sections
// Input  : 
//-----------------------------------------------------------------------------
void DrawMeasuredSections(void)
{
	int		row = 1;
	float	rowheight = 0.025;

	CMeasureSection *p = CMeasureSection::GetList();
	while ( p )
	{
		char str[256];
		Q_snprintf(str,sizeof(str),"%s",p->GetName());
		NDebugOverlay::ScreenText( 0.01,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );
		
		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.28,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );

		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetMaxTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.34,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );


		row++;

		p = p->GetNext();
	}

	bool sort_reset = false;

	// Time to redo sort?
	if ( measure_resort.GetFloat() > 0.0 &&
		engine->Time() >= CMeasureSection::m_dNextResort )
	{
		// Redo it
		CMeasureSection::SortSections();
		// Set next time
		CMeasureSection::m_dNextResort = engine->Time() + measure_resort.GetFloat();
		// Flag to reset sort accumulator, too
		sort_reset = true;
	}

	// Iterate through the sections now
	p = CMeasureSection::GetList();
	while ( p )
	{
		// Update max 
		p->UpdateMax();

		// Reset regular accum.
		p->Reset();
		// Reset sort accum less often
		if ( sort_reset )
		{
			p->SortReset();
		}
		p = p->GetNext();
	}

}
#endif

#ifndef SWDS
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DrawAllDebugOverlays( void ) 
{
	NDebugOverlay::PurgeTextOverlays();

	// If in debug select mode print the selection entities name or classname
	if (CBaseEntity::m_bInDebugSelect)
	{
		CBasePlayer* pPlayer =  UTIL_PlayerByIndex( CBaseEntity::m_nDebugPlayer );

		if (pPlayer)
		{
			// First try to trace a hull to an entity
			CBaseEntity *pEntity = FindPickerEntity( pPlayer );

			if ( pEntity ) 
			{
				pEntity->DrawDebugTextOverlays();
				pEntity->DrawBBoxOverlay();
				pEntity->SendDebugPivotOverlay();
			}
		}
	}

	// --------------------------------------------------------
	//  Draw debug overlay lines 
	// --------------------------------------------------------
	UTIL_DrawOverlayLines();

	// PERFORMANCE: only do this in developer mode
	if ( developer->GetInt() )
	{
		// iterate through all objects for debug overlays
		const CEntInfo *pInfo = gEntList.FirstEntInfo();

		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			CBaseEntity *ent = (CBaseEntity *)pInfo->m_pBaseEnt;
			// HACKHACK: to flag off these calls
			if ( ent->m_debugOverlays || ent->m_pTimedOverlay )
			{
				MDLCACHE_CRITICAL_SECTION();
				ent->DrawDebugGeometryOverlays();
			}
		}
	}

	if ( sv_massreport.GetInt() )
	{
		// iterate through all objects for debug overlays
		const CEntInfo *pInfo = gEntList.FirstEntInfo();

		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			CBaseEntity *ent = (CBaseEntity *)pInfo->m_pBaseEnt;
			if (!ent->VPhysicsGetObject())
				continue;

			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"%s: Mass: %.2f kg / %.2f lb (%s)", 
				STRING( ent->GetModelName() ), ent->VPhysicsGetObject()->GetMass(), 
				kg2lbs(ent->VPhysicsGetObject()->GetMass()), 
				GetMassEquivalent(ent->VPhysicsGetObject()->GetMass()));
			ent->EntityText(0, tempstr, 0);
		}
	}

	// A hack to draw point_message entities w/o developer required
	DrawMessageEntities();
}
#endif

CServerGameDLL g_ServerGameDLL;
IServerGameDLLEx *serverGameDLLEx = &g_ServerGameDLL;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL, g_ServerGameDLL);

// When bumping the version to this interface, check that our assumption is still valid and expose the older version in the same way
COMPILE_TIME_ASSERT( INTERFACEVERSION_SERVERGAMEDLL_INT == 10 );

static ConVar sv_threaded_init("sv_threaded_init", "0");

static bool InitGameSystems( CreateInterfaceFn appSystemFactory )
{
	// The string system must init first + shutdown last
	IGameSystem::Add( GameStringSystem() );

	// Physics must occur before the sound envelope manager
	IGameSystem::Add( PhysicsGameSystem() );
	
	// Used to service deferred navigation queries for NPCs
	IGameSystem::Add( (IGameSystem *) PostFrameNavigationSystem() );

	// Add game log system
	IGameSystem::Add( GameLogSystem() );

	// Add HLTV director 
	IGameSystem::Add( HLTVDirectorSystem() );

#if defined( REPLAY_ENABLED )
	// Add Replay director
	IGameSystem::Add( ReplayDirectorSystem() );
#endif

	// Add sound emitter
	IGameSystem::Add( SoundEmitterSystem() );

#ifndef SWDS
	// Startup vgui
	if ( enginevgui && !engine->IsDedicatedServer() )
	{
		if(!VGui_Startup( appSystemFactory ))
			return false;
	}
#endif // SERVER_USES_VGUI

	// load Mod specific game events ( MUST be before InitAllSystems() so it can pickup the mod specific events)
	gameeventmanager->LoadEventsFromFile("resource/ModEvents.res");

	if ( !IGameSystem::InitAllSystems() )
		return false;

#if defined( REPLAY_ENABLED )
	if ( gameeventmanager->LoadEventsFromFile( "resource/replayevents.res" ) <= 0 )
	{
		Warning( "\n*\n* replayevents.res MISSING.\n*\n\n" );
		return false;
	}
#endif

	// Due to dependencies, these are not autogamesystems
	if ( !ModelSoundsCacheInit() )
	{
		return false;
	}

	InvalidateQueryCache();

	RecastMgr().Init();

	// init the gamestatsupload connection
	gamestatsuploader->InitConnection();

	// UNDONE: Make most of these things server systems or precache_registers
	// =================================================
	//	Activities
	// =================================================
	ActivityList_Free();
	ActivityList_Init();
	ActivityList_RegisterSharedActivities();

	EventList_Free();
	EventList_Init();
	EventList_RegisterSharedEvents();

	return true;
}

bool CServerGameDLL::DLLInit( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals)
{
	if ( (g_pFullFileSystem = (IFileSystem *)fileSystemFactory(FILESYSTEM_INTERFACE_VERSION,NULL)) == NULL )
		return false;

	char gamebin_path[MAX_PATH];
	g_pFullFileSystem->GetSearchPath("GAMEBIN", false, gamebin_path, ARRAYSIZE(gamebin_path));
	char *comma = V_strstr(gamebin_path,";");
	if(comma) {
		*comma = '\0';
	}
	V_AppendSlash(gamebin_path, ARRAYSIZE(gamebin_path));
	int gamebin_length = V_strlen(gamebin_path);

	HackMgr_SwapVphysics( physicsFactory, appSystemFactory, vphysicsDLL );

#ifdef SWDS
	if(!HackMgr_Server_PreInit(this, appSystemFactory, physicsFactory, fileSystemFactory, pGlobals, true))
		return false;
#else
	if(!HackMgr_Server_PreInit(this, appSystemFactory, physicsFactory, fileSystemFactory, pGlobals, false))
		return false;
#endif

	COM_TimestampedLog( "ConnectTier1/2/3Libraries - Start" );

	ConnectTier1Libraries( &appSystemFactory, 1 );
	ConnectTier2Libraries( &appSystemFactory, 1 );
	ConnectTier3Libraries( &appSystemFactory, 1 );

	COM_TimestampedLog( "ConnectTier1/2/3Libraries - Finish" );

	// Connected in ConnectTier1Libraries
	if ( g_pCVar == NULL )
		return false;

	COM_TimestampedLog( "Factories - Start" );

	// init each (seperated for ease of debugging)
	if ( (engine = (IVEngineServer*)appSystemFactory(INTERFACEVERSION_VENGINESERVER, NULL)) == NULL )
		return false;
	if ( (g_pVoiceServer = (IVoiceServer*)appSystemFactory(INTERFACEVERSION_VOICESERVER, NULL)) == NULL )
		return false;
	if ( (networkstringtable = (INetworkStringTableContainer *)appSystemFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL)) == NULL )
		return false;
	if ( (staticpropmgr = (IStaticPropMgrServer *)appSystemFactory(INTERFACEVERSION_STATICPROPMGR_SERVER,NULL)) == NULL )
		return false;
	if ( (random_valve = (IUniformRandomStream *)appSystemFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (enginesound = (IEngineSound *)appSystemFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (partition = (ISpatialPartition *)appSystemFactory(INTERFACEVERSION_SPATIALPARTITION, NULL)) == NULL )
		return false;
	if ( (modelinfo = (IVModelInfo *)appSystemFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (enginetrace = (IEngineTrace *)appSystemFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL)) == NULL )
		return false;
	if ( (gameeventmanager = (IGameEventManager2 *)appSystemFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL)) == NULL )
		return false;
	if ( !g_pDataCache )
		return false;
	if ( !g_pSoundEmitterSystem )
		return false;
	if ( (gamestatsuploader = (IUploadGameStats *)appSystemFactory( INTERFACEVERSION_UPLOADGAMESTATS, NULL )) == NULL )
		return false;
	if ( !g_pMDLCache )
		return false;
	if ( (serverpluginhelpers = (IServerPluginHelpers *)appSystemFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL)) == NULL )
		return false;
	if ( (scenefilecache = (ISceneFileCache *)appSystemFactory( SCENE_FILE_CACHE_INTERFACE_VERSION, NULL )) == NULL )
		return false;

	g_pMatchFramework = (IMatchFramework *)appSystemFactory( IMATCHFRAMEWORK_VERSION_STRING, NULL );

#ifndef SWDS
	if ( CommandLine()->FindParm( "-textmode" ) || engine->IsDedicatedServer() )
		g_bTextMode = true;
#endif

#ifndef SWDS
	// If not running dedicated, grab the engine vgui interface
	if ( !engine->IsDedicatedServer() && !g_bTextMode )
	{
		// This interface is optional, and is only valid when running with -tools
		serverenginetools = ( IServerEngineTools * )appSystemFactory( VSERVERENGINETOOLS_INTERFACE_VERSION, NULL );
	}
#endif

#ifndef SWDS
	// If not running dedicated, grab the engine vgui interface
	if ( !engine->IsDedicatedServer() && !g_bTextMode )
	{
		if ( ( enginevgui = ( IEngineVGui * )appSystemFactory(VENGINE_VGUI_VERSION, NULL)) == NULL )
			return false;

		gameuifuncs = (IGameUIFuncs * )appSystemFactory( VENGINE_GAMEUIFUNCS_VERSION, NULL );
	}
#endif // SERVER_USES_VGUI

#ifndef SWDS
	if(!engine->IsDedicatedServer()) {
		V_strcat_safe(gamebin_path, "game_loopback" DLL_EXT_STRING);
		Sys_LoadInterface( gamebin_path, GAMELOOPBACK_INTERFACE_VERSION, &game_loopbackDLL, reinterpret_cast< void** >( &g_pGameLoopback ) );
		gamebin_path[gamebin_length] = '\0';

		V_strcat_safe(gamebin_path, "client" DLL_EXT_STRING);
		clientDLL = Sys_LoadModule(gamebin_path);
		gamebin_path[gamebin_length] = '\0';
		if(clientDLL) {
			CreateInterfaceFn clientFactory = Sys_GetFactory(clientDLL);
			if(clientFactory) {
				int status = IFACE_OK;
				g_pGameClientLoopback = (IGameClientLoopback *)clientFactory(GAMECLIENTLOOPBACK_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					g_pGameClientLoopback = NULL;
				}
			}
		}
	}
#endif

	COM_TimestampedLog( "Factories - Finish" );

	COM_TimestampedLog( "g_pSoundEmitterSystem->Connect" );

	// Yes, both the client and game .dlls will try to Connect, the soundemittersystem.dll will handle this gracefully
	if ( !g_pSoundEmitterSystem->Connect( appSystemFactory ) )
		return false;

	// cache the globals
	gpGlobals = pGlobals;

	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();

	COM_TimestampedLog( "MathLib_Init" );

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	// save these in case other system inits need them
	factorylist_t factories;
	factories.engineFactory = appSystemFactory;
	factories.fileSystemFactory = fileSystemFactory;
	factories.physicsFactory = physicsFactory;
	FactoryList_Store( factories );

	COM_TimestampedLog( "gameeventmanager->LoadEventsFromFile" );

	// load used game events  
	gameeventmanager->LoadEventsFromFile("resource/gameevents.res");

	// init the cvar list first in case inits want to reference them
	InitializeServerCvars();

	COM_TimestampedLog( "g_pParticleSystemMgr->Init" );

	// Initialize the particle system
	if ( !g_pParticleSystemMgr->Init( g_pParticleSystemQuery ) )
	{
		return false;
	}

	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;

	bool bInitSuccess = false;
	if ( sv_threaded_init.GetBool() )
	{
		CFunctorJob *pGameJob = new CFunctorJob( CreateFunctor( ParseParticleEffects, false, false ) );
		g_pThreadPool->AddJob( pGameJob );

		bInitSuccess = InitGameSystems( appSystemFactory );

		// FIXME: This method is a bit of a hack.
		// Try to update the screen every .06 seconds while waiting.
		float flLastUpdateTime = -1.0f;

		while( !pGameJob->IsFinished() )
		{
			float flTime = Plat_FloatTime();

			if ( flTime - flLastUpdateTime > .06f )
			{
				flLastUpdateTime = flTime;
			}

			ThreadSleep( 0 );
		}

		pGameJob->Release();
	}
	else
	{
		COM_TimestampedLog( "ParseParticleEffects" );
		ParseParticleEffects( false, false );

		COM_TimestampedLog( "InitGameSystems - Start" );
		bInitSuccess = InitGameSystems( appSystemFactory );
		COM_TimestampedLog( "InitGameSystems - Finish" );
	}

	if(!bInitSuccess)
		return false;

	g_AI_SchedulesManager.CreateStringRegistries();

	// =================================================
	//	Load and Init AI Schedules
	// =================================================
	COM_TimestampedLog( "LoadAllSchedules" );
	g_AI_SchedulesManager.LoadAllSchedules();

#ifndef SWDS
	if(!engine->IsDedicatedServer() && !g_bTextMode) {
		// try to get debug overlay, may be NULL if on HLDS
		debugoverlay = (IVDebugOverlay *)appSystemFactory( VDEBUG_OVERLAY_INTERFACE_VERSION, NULL );
	}
#endif

	return true;
}

void CServerGameDLL::PostInit()
{
	// init sentence group playback stuff from sentences.txt.
	// ok to call this multiple times, calls after first are ignored.
	COM_TimestampedLog( "SENTENCEG_Init()" );
	SENTENCEG_Init();

	IGameSystem::PostInitAllSystems();

#ifndef SWDS
	if ( !engine->IsDedicatedServer() && enginevgui && !g_bTextMode )
	{
		if ( VGui_PostInit() )
		{
			// all good
		}
	}
#endif // SERVER_USES_VGUI
}

void CServerGameDLL::PostToolsInit()
{
#ifndef SWDS
	if ( serverenginetools && !engine->IsDedicatedServer() && !g_bTextMode )
	{
		serverfoundry = ( IServerFoundry * )serverenginetools->QueryInterface( VSERVERFOUNDRY_INTERFACE_VERSION );
	}
#endif
}

void CServerGameDLL::DLLShutdown( void )
{
	g_AI_SchedulesManager.DeleteAllSchedules();
	g_AI_AgentSchedulesManager.DeleteAllSchedules();
	g_AI_SchedulesManager.DestroyStringRegistries();
	g_AI_AgentSchedulesManager.DestroyStringRegistries();

	// Due to dependencies, these are not autogamesystems
	ModelSoundsCacheShutdown();

	char *pFilename = g_TextStatsMgr.GetStatsFilename();
	if ( !pFilename || !pFilename[0] )
	{
		g_TextStatsMgr.SetStatsFilename( "stats.txt" );
	}
	g_TextStatsMgr.WriteFile( g_pFullFileSystem );

	IGameSystem::ShutdownAllSystems();

#ifndef SWDS
	if ( enginevgui && !engine->IsDedicatedServer() )
	{
		VGui_Shutdown();
	}
#endif // SERVER_USES_VGUI

#ifndef SWDS
	if ( clientDLL )
		Sys_UnloadModule( clientDLL );

	if ( game_loopbackDLL )
		Sys_UnloadModule( game_loopbackDLL );
#endif

	if ( vphysicsDLL )
		Sys_UnloadModule( vphysicsDLL );

	// reset (shutdown) the gamestatsupload connection
	gamestatsuploader->InitConnection();

	gameeventmanager = NULL;
	
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	ConVar_Unregister();
	DisconnectTier1Libraries();
}

bool CServerGameDLL::ReplayInit( CreateInterfaceFn fnReplayFactory )
{
#if defined( REPLAY_ENABLED )
	if ( (g_pReplay = ( IReplaySystem *)fnReplayFactory( REPLAY_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (g_pReplayServerContext = g_pReplay->SV_GetContext()) == NULL )
		return false;
	return true;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: See shareddefs.h for redefining this.  Don't even think about it, though, for HL2.  Or you will pay.  ywb 9/22/03
// Output : float
//-----------------------------------------------------------------------------
float CServerGameDLL::GetTickInterval( void ) const
{
	float tickinterval = DEFAULT_TICK_INTERVAL;

	// override if tick rate specified in command line
	if ( CommandLine()->CheckParm( "-tickrate" ) )
	{
		float tickrate = CommandLine()->ParmValue( "-tickrate", 0 );
		if ( tickrate > 10 )
			tickinterval = 1.0f / tickrate;
	}

	return tickinterval;
}

// This is called when a new game is started. (restart, map)
bool CServerGameDLL::GameInit( void )
{
	ResetGlobalState();
	engine->ServerCommand( "exec game.cfg\n" );
	engine->ServerExecute( );
	CBaseEntity::sm_bAccurateTriggerBboxChecks = true;

	IGameEvent *event = gameeventmanager->CreateEvent( "game_init" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	return true;
}

// This is called when a game ends (server disconnect, death, restart, load)
// NOT on level transitions within a game
void CServerGameDLL::GameShutdown( void )
{
	ResetGlobalState();
}

static bool g_OneWayTransition = false;
void Game_SetOneWayTransition( void )
{
	g_OneWayTransition = true;
}

extern void SetupDefaultLightstyle();

#ifndef SWDS
extern ConVar *room_type;
#endif

extern CWorld *g_WorldEntity;
extern CBaseEntity				*g_pLastSpawn;
extern void InitBodyQue(void);
extern void W_Precache(void);

extern ConVar sv_stepsize;

// Called any time a new level is started (after GameInit() also on level transitions within a game)
bool CServerGameDLL::LevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool background )
{
	VPROF("CServerGameDLL::LevelInit");

	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;

	// IGameSystem::LevelInitPreEntityAllSystems() is called when the world is precached
	// That happens either in LoadGameState() or in MapEntity_ParseAllEntities()
	if ( background )
	{
		gpGlobals->eLoadType = MapLoad_Background;
	}
	else
	{
		gpGlobals->eLoadType = MapLoad_NewGame;
	}

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
#ifndef SWDS
	if ( !engine->IsDedicatedServer() )
	{
		// listen server
		const char *cfgfile = lservercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log_Msg( LOG_GAMERULES,"Executing listen server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}
	else
#endif
	{
		// dedicated server
		const char *cfgfile = servercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log_Msg( LOG_GAMERULES,"Executing dedicated server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}

	nextlevel.SetValue( "" );

	//Tony; parse custom manifest if exists!
	ParseParticleEffectsMap( pMapName, false );

	g_fGameOver = false;
	g_pLastSpawn = NULL;
	g_Language.SetValue( LANGUAGE_ENGLISH );	// TODO use VGUI to get current language
	g_AIFriendliesTalkSemaphore.Release();
	g_AIFoesTalkSemaphore.Release();
	g_OneWayTransition = false;

	sv_stepsize.Revert();

#ifndef SWDS
	if( !engine->IsDedicatedServer() )
		room_type->SetValue( 0 );
#endif

	// Set up game rules
	Assert( !GameRules() );
	if (GameRules())
	{
		UTIL_Remove( GameRules() );
	}

	InstallGameRules();
	Assert( GameRules() );
	GameRules()->Init();

	// Only allow precaching between LevelInitPreEntity and PostEntity
	CBaseEntity::SetAllowPrecache( true );

	COM_TimestampedLog( "IGameSystem::LevelInitPreEntityAllSystems" );
	IGameSystem::LevelInitPreEntityAllSystems();

	COM_TimestampedLog( "PrecacheStandardParticleSystems()" );
	// Precache standard particle systems
	PrecacheStandardParticleSystems( );

	// the area based ambient sounds MUST be the first precache_sounds

	COM_TimestampedLog( "W_Precache()" );
	
	// player precaches     
	W_Precache();									// get weapon precaches
	
	COM_TimestampedLog( "ClientPrecache()" );
	ClientPrecache();

	COM_TimestampedLog( "PrecacheTempEnts()" );
	// precache all temp ent stuff
	CBaseTempEntity::PrecacheTempEnts();

	COM_TimestampedLog( "g_pGameRules->Precache" );
	GameRules()->Precache();

	// Call all registered precachers.
	CPrecacheRegister::Precache();

	// =================================================
	//	Initialize NPC Relationships
	// =================================================
	GameRules()->InitDefaultAIRelationships();
	CBaseCombatCharacter::InitInteractionSystem();

	g_EventQueue.Init();

	SetupDefaultLightstyle();

	g_WorldEntity = (CWorld *)CreateEntityByName("worldspawn", 0);
	g_WorldEntity->SetLocalOrigin( vec3_origin );
	g_WorldEntity->SetLocalAngles( vec3_angle );
	DispatchSpawn(g_WorldEntity);

	COM_TimestampedLog( "g_pGameRules->CreateStandardEntities()" );
	// Create the player resource
	GameRules()->CreateStandardEntities();

	CSoundEnt::InitSoundEnt();

	COM_TimestampedLog( "InitBodyQue()" );
	InitBodyQue();

	// Clear out entity references, and parse the entities into it.
	g_MapEntityRefs.Purge();
	CMapLoadEntityFilter filter;
	MapEntity_ParseAllEntities( pMapEntities, &filter );

	// Now call the mod specific parse
	LevelInit_ParseAllEntities( pMapEntities );

	// Sometimes an ent will Remove() itself during its precache, so RemoveImmediate won't happen.
	// This makes sure those ents get cleaned up.
	gEntList.CleanupDeleteList();

	// Now that all of the active entities have been loaded in, precache any entities who need point_template parameters
	//  to be parsed (the above code has loaded all point_template entities)
	PrecachePointTemplates();

	g_pServerBenchmark->StartBenchmark();

	// load MOTD from file into stringtable
	LoadMessageOfTheDay();

	// ask for the latest game rules
	GameRules()->UpdateGameplayStatsFromSteam();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float GetRealTime()
{
	return engine->Time();
}

//-----------------------------------------------------------------------------
// Purpose: called after every level change and load game, iterates through all the
//			active entities and gives them a chance to fix up their state
//-----------------------------------------------------------------------------
#ifdef DEBUG
bool g_bReceivedChainedActivate;
bool g_bCheckForChainedActivate;
#define BeginCheckChainedActivate() if (0) ; else { g_bCheckForChainedActivate = true; g_bReceivedChainedActivate = false; }
#define EndCheckChainedActivate( bCheck )	\
	if (0) ; else \
	{ \
		if ( bCheck ) \
		{ \
			AssertMsg( g_bReceivedChainedActivate, "Entity (%i/%s/%s) failed to call base class Activate()\n", pClass->entindex(), pClass->GetClassname(), STRING( pClass->GetEntityName() ) );	\
		} \
		g_bCheckForChainedActivate = false; \
	}
#else
#define BeginCheckChainedActivate()			((void)0)
#define EndCheckChainedActivate( bCheck )	((void)0)
#endif

void CServerGameDLL::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	if ( gEntList.ResetDeleteList() != 0 )
	{
		Msg( "%s", "ERROR: Entity delete queue not empty on level start!\n" );
	}

	for ( CBaseEntity *pClass = gEntList.FirstEnt(); pClass != NULL; pClass = gEntList.NextEnt(pClass) )
	{
		if ( pClass && !pClass->IsDormant() )
		{
			MDLCACHE_CRITICAL_SECTION();

			BeginCheckChainedActivate();
			pClass->Activate();
			
			// We don't care if it finished activating if it decided to remove itself.
			EndCheckChainedActivate( !( pClass->GetEFlags() & EFL_KILLME ) ); 
		}
	}

	IGameSystem::LevelInitPostEntityAllSystems();

	ResetWindspeed();

	// No more precaching after PostEntityAllSystems!!!
	CBaseEntity::SetAllowPrecache( false );

	RecastMgr().Load();

	// only display the think limit when the game is run with "developer" mode set
	if ( !developer->GetInt() )
	{
		think_limit.SetValue( 0 );
	}

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[MAX_PATH] = { 0 };
	// Map names cannot contain quotes or control characters so this is safe but silly that we have to do it.
	Q_snprintf( szCommand, sizeof( szCommand ), "exec \"%s.cfg\"\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

#ifndef SWDS
	if(!engine->IsDedicatedServer() && !g_bTextMode) {
		// --------------------------------------------------------
		// If in edit mode start WC session and make sure we are
		// running the same map in WC and the engine
		// --------------------------------------------------------
		if (engine->IsInEditMode())
		{
			int status = Editor_BeginSession(STRING(gpGlobals->mapname), gpGlobals->mapversion, false);
			if (status == Editor_NotRunning)
			{
				DevMsg("\nAborting map_edit\nHammer not running...\n\n");
				UTIL_CenterPrintAll( "Hammer not running...\n" );
				engine->ServerCommand("kickall \"Hammer not running...\"\n");
				engine->ServerCommand("disconnect\n");
			}
			else if (status == Editor_BadCommand)
			{
				DevMsg("\nAborting map_edit\nHammer/Engine map versions different...\n\n");
				UTIL_CenterPrintAll( "Hammer/Engine map versions different...\n" );
				engine->ServerCommand("kickall \"Hammer/Engine map versions different...\"\n");
				engine->ServerCommand("disconnect\n");
			}
			else
			{
				// Increment version number when session begins
				gpGlobals->mapversion++;
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after the steam API has been activated post-level startup
//-----------------------------------------------------------------------------
void CServerGameDLL::GameServerSteamAPIActivated( void )
{
	if ( SteamGameServer() )
	{
	#ifndef SWDS
		if( engine->IsDedicatedServer() )
	#endif
			SteamGameServer()->GetGameplayStats();
	}

#ifdef TF_DLL
	GCClientSystem()->GameServerActivate();
	InventoryManager()->GameServerSteamAPIActivated();
	TFMapsWorkshop()->GameServerSteamAPIActivated();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after the steam API has been activated post-level startup
//-----------------------------------------------------------------------------
void CServerGameDLL::GameServerSteamAPIShutdown( void )
{
#ifdef TF_DLL
	GCClientSystem()->Shutdown();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called at the start of every game frame
//-----------------------------------------------------------------------------
ConVar  trace_report( "trace_report", "0" );

extern void GameStartFrame( void );
extern void ServiceEventQueue( void );
extern void Physics_RunThinkFunctions( bool simulating );

void CServerGameDLL::GameFrame( bool simulating )
{
	VPROF( "CServerGameDLL::GameFrame" );

	// Ugly HACK! to prevent the game time from changing when paused
	if( m_bWasPaused != HackMgr_IsGamePaused() )
	{
		m_fPauseTime = gpGlobals->curtime;
		m_nPauseTick = gpGlobals->tickcount;
		m_bWasPaused = HackMgr_IsGamePaused();
	}
	if( HackMgr_IsGamePaused() )
	{
		gpGlobals->curtime = m_fPauseTime;
		gpGlobals->tickcount = m_nPauseTick;
	}

#ifndef SWDS
	if( engine->IsDedicatedServer() || gpGlobals->maxClients > 1 )
#endif
	{
		HackMgr_SetGamePaused( engine->IsPaused() );
	}

	// All the calls to us from the engine prior to gameframe (like LevelInit & ServerActivate)
	// are done before the engine has got the Steam API connected, so we have to wait until now to connect ourselves.
	GameRules()->UpdateGameplayStatsFromSteam();

	if ( CBaseEntity::IsSimulatingOnAlternateTicks() )
	{
		// only run simulation on even numbered ticks
		if ( gpGlobals->tickcount & 1 )
		{
			UpdateAllClientData();
			return;
		}
		// If we're skipping frames, then the frametime is 2x the normal tick
		gpGlobals->frametime *= 2.0f;
	}

	float oldframetime = gpGlobals->frametime;

#ifdef _DEBUG
	// For profiling.. let them enable/disable the networkvar manual mode stuff.
	g_bUseNetworkVars = s_UseNetworkVars.GetBool();
#endif

	// Delete anything that was marked for deletion
	//  outside of server frameloop (e.g., in response to concommand)
	gEntList.CleanupDeleteList();

#ifndef SWDS
	if( !engine->IsDedicatedServer() && !g_bTextMode )
		HandleFoundryEntitySpawnRecords();
#endif

	IGameSystem::FrameUpdatePreEntityThinkAllSystems();
	GameStartFrame();

	RecastMgr().Update( gpGlobals->frametime );

	{
		VPROF( "gamestatsuploader->UpdateConnection" );
		gamestatsuploader->UpdateConnection();
	}

	{
		VPROF( "UpdateQueryCache" );
		UpdateQueryCache();
	}

	{
		VPROF( "g_pServerBenchmark->UpdateBenchmark" );
		g_pServerBenchmark->UpdateBenchmark();
	}

	Physics_RunThinkFunctions( simulating );
	
	IGameSystem::FrameUpdatePostEntityThinkAllSystems();

	// UNDONE: Make these systems IGameSystems and move these calls into FrameUpdatePostEntityThink()
	// service event queue, firing off any actions whos time has come
	ServiceEventQueue();

	// free all ents marked in think functions
	gEntList.CleanupDeleteList();

	// FIXME:  Should this only occur on the final tick?
	UpdateAllClientData();

	if ( GameRules() )
	{
		VPROF( "g_pGameRules->EndGameFrame" );
		GameRules()->EndGameFrame();
	}

	if ( trace_report.GetBool() )
	{
		int total = 0, totals[3];
		for ( int i = 0; i < 3; i++ )
		{
			totals[i] = enginetrace->GetStatByIndex( i, true );
			if ( totals[i] > 0 )
			{
				total += totals[i];
			}
		}

		if ( total )
		{
			Msg("Trace: %d, contents %d, enumerate %d\n", totals[0], totals[1], totals[2] );
		}
	}

	// Any entities that detect network state changes on a timer do it here.
	g_NetworkPropertyEventMgr.FireEvents();

	gpGlobals->frametime = oldframetime;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame even if not ticking
// Input  : simulating - 
//-----------------------------------------------------------------------------
void CServerGameDLL::PreClientUpdate( bool simulating )
{
	if ( !simulating )
		return;

	/*
	if (game_speeds.GetInt())
	{
		DrawMeasuredSections();
	}
	*/

#if defined _DEBUG && !defined SWDS
	if( !engine->IsDedicatedServer() && !g_bTextMode )
		DrawAllDebugOverlays();
#endif

	IGameSystem::PreClientUpdateAllSystems();

#if defined _DEBUG && !defined SWDS
	if( !engine->IsDedicatedServer() && !g_bTextMode )
	{
		if ( sv_showhitboxes.GetInt() == -1 )
			return;

		if ( sv_showhitboxes.GetInt() == 0 )
		{
			// assume it's text
			CBaseEntity *pEntity = NULL;

			while (1)
			{
				pEntity = gEntList.FindEntityByName( pEntity, sv_showhitboxes.GetString() );
				if ( !pEntity )
					break;

				CBaseAnimating *anim = dynamic_cast< CBaseAnimating * >( pEntity );

				if (anim)
				{
					anim->DrawServerHitboxes();
				}
			}
			return;
		}

		CBaseAnimating *anim = dynamic_cast< CBaseAnimating * >( CBaseEntity::Instance( engine->PEntityOfEntIndex( sv_showhitboxes.GetInt() ) ) );
		if ( !anim )
			return;

		anim->DrawServerHitboxes();
	}
#endif
}

void CServerGameDLL::Think( bool finalTick )
{
	
}

void CServerGameDLL::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}


// Called when a level is shutdown (including changing levels)
void CServerGameDLL::LevelShutdown( void )
{
#ifndef SWDS
	if(!engine->IsDedicatedServer()) {
		// If in edit mode tell Hammer I'm ending my session. This re-enables
		// the Hammer UI so they can continue editing the map.
		Editor_EndSession(false);
	}
#endif

	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;

	IGameSystem::LevelShutdownPreClearSteamAPIContextAllSystems();

	MDLCACHE_CRITICAL_SECTION();
	IGameSystem::LevelShutdownPreEntityAllSystems();

	// YWB:
	// This entity pointer is going away now and is corrupting memory on level transitions/restarts
	CSoundEnt::ShutdownSoundEnt();

	ClearDebugHistory();

	gEntList.Clear();

	InvalidateQueryCache();

	RecastMgr().Reset();

	if ( GameRules() )
	{
		GameRules()->LevelShutdown();
		UTIL_Remove( GameRules() );
	}

	IGameSystem::LevelShutdownPostEntityAllSystems();

	g_pServerBenchmark->EndBenchmark();

	// In case we quit out during initial load
	CBaseEntity::SetAllowPrecache( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : ServerClass*
//-----------------------------------------------------------------------------
ServerClass* CServerGameDLL::GetAllServerClasses()
{
	return g_pServerClassHead;
}


const char *CServerGameDLL::GetGameDescription( void )
{
	return ::GetGameDescription();
}

void CServerGameDLL::CreateNetworkStringTables( void )
{
	// Create any shared string tables here (and only here!)
	// E.g.:  xxx = networkstringtable->CreateStringTable( "SceneStrings", 512 );
	g_pStringTableExtraParticleFiles = networkstringtable->CreateStringTable( "ExtraParticleFilesTable", MAX_PARTICLESYSTEMS_STRINGS );
	g_pStringTableParticleEffectNames = networkstringtable->CreateStringTable( "ParticleEffectNames", MAX_PARTICLESYSTEMS_STRINGS );
	g_pStringTableEffectDispatch = networkstringtable->CreateStringTable( "EffectDispatch", MAX_EFFECT_DISPATCH_STRINGS );
	g_pStringTableVguiScreen = networkstringtable->CreateStringTable( "VguiScreen", MAX_VGUI_SCREEN_STRINGS );
	g_pStringTableMaterials = networkstringtable->CreateStringTable( "Materials", MAX_MATERIAL_STRINGS );
	g_pStringTableInfoPanel = networkstringtable->CreateStringTable( "InfoPanel", MAX_INFOPANEL_STRINGS );
	g_pStringTableClientSideChoreoScenes = networkstringtable->CreateStringTable( "Scenes", MAX_CHOREO_SCENES_STRINGS );
	g_pStringTableServerMapCycle = networkstringtable->CreateStringTable( "ServerMapCycle", 128 );

	Assert( g_pStringTableParticleEffectNames &&
			g_pStringTableEffectDispatch &&
			g_pStringTableVguiScreen &&
			g_pStringTableMaterials &&
			g_pStringTableInfoPanel &&
			g_pStringTableClientSideChoreoScenes &&
			g_pStringTableServerMapCycle &&
			g_pStringTableExtraParticleFiles);

	// Need this so we have the error material always handy
	PrecacheMaterial( "debug/debugempty" );
	Assert( GetMaterialIndex( "debug/debugempty" ) == 0 );

	PrecacheParticleSystem( "error" );	// ensure error particle system is handy
	Assert( GetParticleSystemIndex( "error" ) == 0 );

	PrecacheEffect( "error" );	// ensure error effect is handy
	Assert( GetEffectIndex( "error" ) == 0 );

	CreateNetworkStringTables_GameRules();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_type - 
//			*name - 
//			size - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------

bool CServerGameDLL::GetUserMessageInfo( int msg_type, char *name, int maxnamelength, int& size )
{
	if ( !usermessages->IsValidIndex( msg_type ) )
		return false;

	Q_strncpy( name, usermessages->GetUserMessageName( msg_type ), maxnamelength );
	size = usermessages->GetUserMessageSize( msg_type );
	return true;
}

CStandardSendProxies* CServerGameDLL::GetStandardSendProxies()
{
	return &g_StandardSendProxies;
}

#include "client_textmessage.h"

//-----------------------------------------------------------------------------
// Purpose: Returns true if the game DLL wants the server not to be made public.
//			Used by commentary system to hide multiplayer commentary servers from the master.
//-----------------------------------------------------------------------------
bool CServerGameDLL::ShouldHideServer( void )
{
	if ( hide_server && hide_server->GetBool() )
		return true;

	if ( gpGlobals && gpGlobals->eLoadType == MapLoad_Background )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CServerGameDLL::InvalidateMdlCache()
{
	CBaseAnimating *pAnimating;
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		pAnimating = dynamic_cast<CBaseAnimating *>(pEntity);
		if ( pAnimating )
		{
			pAnimating->InvalidateMdlCache();
		}
	}
}

// interface to the new GC based lobby system
IServerGCLobby *CServerGameDLL::GetServerGCLobby()
{
	return NULL;
}

void CServerGameDLL::ServerHibernationUpdate( bool bHibernating )
{
	m_bIsHibernating = bHibernating;

	if ( engine && m_bIsHibernating && GameRules() )
	{
		GameRules()->OnServerHibernating();
	}
}

KeyValues * CServerGameDLL::FindLaunchOptionByValue( KeyValues *pLaunchOptions, char const *szLaunchOption )
{
	if ( !pLaunchOptions || !szLaunchOption || !*szLaunchOption )
		return NULL;

	for ( KeyValues *val = pLaunchOptions->GetFirstSubKey(); val; val = val->GetNextKey() )
	{
		char const *szValue = val->GetString();
		if ( szValue && *szValue && !Q_stricmp( szValue, szLaunchOption ) )
			return val;
	}

	return NULL;
}

bool CServerGameDLL::ShouldPreferSteamAuth()
{
	return true;
}

bool CServerGameDLL::SupportsRandomMaps()
{
	return false;
}

// return true to disconnect client due to timeout (used to do stricter timeouts when the game is sure the client isn't loading a map)
bool CServerGameDLL::ShouldTimeoutClient( int nUserID, float flTimeSinceLastReceived )
{
	if ( !GameRules() )
		return false;

	return GameRules()->ShouldTimeoutClient( nUserID, flTimeSinceLastReceived );
}

//-----------------------------------------------------------------------------
void CServerGameDLL::Status( void (*print) (const char *fmt, ...) )
{
	if ( GameRules() )
	{
		GameRules()->Status( print );
	}
}

//-----------------------------------------------------------------------------
void CServerGameDLL::PrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
                                            /* in/out */ char *pszMapFile, size_t nMapFileSize )
{

}

//-----------------------------------------------------------------------------
IServerGameDLL::ePrepareLevelResourcesResult
CServerGameDLL::AsyncPrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
                                            /* in/out */ char *pszMapFile, size_t nMapFileSize,
                                            float *flProgress /* = NULL */ )
{
	if ( flProgress )
	{
		*flProgress = 1.f;
	}
	return IServerGameDLL::ePrepareLevelResources_Prepared;
}

//-----------------------------------------------------------------------------
IServerGameDLL::eCanProvideLevelResult CServerGameDLL::CanProvideLevel( /* in/out */ char *pMapName, int nMapNameMax )
{
	return IServerGameDLL::eCanProvideLevel_CannotProvide;
}

//-----------------------------------------------------------------------------
bool CServerGameDLL::IsManualMapChangeOkay( const char **pszReason )
{
	if ( GameRules() )
	{
		return GameRules()->IsManualMapChangeOkay( pszReason );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sanity-check to verify that a path is a relative path inside the game dir
// Taken From: engine/cmd.cpp
//-----------------------------------------------------------------------------
static bool IsValidPath( const char *pszFilename )
{
	if ( !pszFilename )
	{
		return false;
	}

	if ( Q_strlen( pszFilename ) <= 0    ||
		 Q_IsAbsolutePath( pszFilename ) || // to protect absolute paths
		 Q_strstr( pszFilename, ".." ) )    // to protect relative paths
	{
		return false;
	}

	return true;
}

static void ValidateMOTDFilename( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVar *var = (ConVar *)pConVar;
	if ( !IsValidPath( var->GetString() ) )
	{
		var->SetValue( var->GetDefault() );
	}
}

static ConVar motdfile( "motdfile", "motd.txt", 0, "The MOTD file to load.", ValidateMOTDFilename );
static ConVar motdfile_text( "motdfile_text", "motd_text.txt", 0, "The text-only MOTD file to use for clients that have disabled HTML MOTDs.", ValidateMOTDFilename );
static ConVar hostfile( "hostfile", "host.txt", FCVAR_RELEASE, "The HOST file to load.", ValidateMOTDFilename );
void CServerGameDLL::LoadMessageOfTheDay()
{
	LoadSpecificMOTDMsg( motdfile, "motd" );
	LoadSpecificMOTDMsg( motdfile_text, "motd_text" );
	LoadSpecificMOTDMsg( hostfile, "hostfile" );
}

void CServerGameDLL::LoadSpecificMOTDMsg( const ConVar &convar, const char *pszStringName )
{
	CUtlBuffer buf;

	// Generate preferred filename, which is in the cfg folder.
	char szPreferredFilename[ MAX_PATH ];
	V_sprintf_safe( szPreferredFilename, "cfg/%s", convar.GetString() );

	// Check the preferred filename first
	char szResolvedFilename[ MAX_PATH ];
	V_strcpy_safe( szResolvedFilename, szPreferredFilename );
	bool bFound = g_pFullFileSystem->ReadFile( szResolvedFilename, "GAME", buf );

	// Not found?  Try in the root, which is the old place it used to go.
	if ( !bFound )
	{

		V_strcpy_safe( szResolvedFilename, convar.GetString() );
		bFound = g_pFullFileSystem->ReadFile( szResolvedFilename, "GAME", buf );
	}

	// Still not found?  See if we can try the default.
	if ( !bFound && !V_stricmp( convar.GetString(), convar.GetDefault() ) )
	{
		V_strcpy_safe( szResolvedFilename, szPreferredFilename );
		char *dotTxt = V_stristr( szResolvedFilename, ".txt" );
		Assert ( dotTxt != NULL );
		if ( dotTxt ) V_strcpy( dotTxt, "_default.txt" );
		bFound = g_pFullFileSystem->ReadFile( szResolvedFilename, "GAME", buf );
	}

	if ( !bFound )
	{
		Log_Warning( LOG_GAMERULES,"'%s' not found; not loaded\n", szPreferredFilename );
		return;
	}

	if ( buf.TellPut() > 2048 )
	{
		Log_Warning(LOG_GAMERULES,"'%s' is too big; not loaded\n", szResolvedFilename );
		return;
	}
	buf.PutChar( '\0' );

	if ( V_stricmp( szPreferredFilename, szResolvedFilename ) == 0)
	{
		Log_Msg( LOG_GAMERULES,"Set %s from file '%s'\n", pszStringName, szResolvedFilename );
	}
	else
	{
		Log_Msg( LOG_GAMERULES, "Set %s from file '%s'.  ('%s' was not found.)\n", pszStringName, szResolvedFilename, szPreferredFilename );
	}

	g_pStringTableInfoPanel->AddString( true, pszStringName, buf.TellPut(), buf.Base() );
}

//-----------------------------------------------------------------------------
// Precaches a vgui screen overlay material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName )
{
	Assert( pMaterialName && pMaterialName[0] );
	g_pStringTableMaterials->AddString( true, pMaterialName );
}


//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName )
{
	if (pMaterialName)
	{
		int nIndex = g_pStringTableMaterials->FindStringIndex( pMaterialName );
		
		if (nIndex != INVALID_STRING_INDEX )
		{
			return nIndex;
		}
		else
		{
			DevMsg("Warning! GetMaterialIndex: couldn't find material %s\n ", pMaterialName );
			return 0;
		}
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nMaterialIndex )
{
	return g_pStringTableMaterials->GetString( nMaterialIndex );
}


//-----------------------------------------------------------------------------
// Precaches a vgui screen overlay material
//-----------------------------------------------------------------------------
int PrecacheParticleSystem( const char *pParticleSystemName )
{
	Assert( pParticleSystemName && pParticleSystemName[0] );
	return g_pStringTableParticleEffectNames->AddString( true, pParticleSystemName );
}

void PrecacheParticleFileAndSystems( const char *pParticleSystemFile )
{
	g_pParticleSystemMgr->ShouldLoadSheets( true );
	g_pParticleSystemMgr->ReadParticleConfigFile( pParticleSystemFile, true, false );
	g_pParticleSystemMgr->DecommitTempMemory();

	Assert( pParticleSystemFile && pParticleSystemFile[0] );
	g_pStringTableExtraParticleFiles->AddString( true, pParticleSystemFile );
}

void PrecacheGameSoundsFile( const char *pSoundFile )
{
	g_pSoundEmitterSystem->AddSoundsFromFile( pSoundFile, true );
	SoundSystemPreloadSounds();
}

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetParticleSystemIndex( const char *pParticleSystemName )
{
	if ( pParticleSystemName )
	{
		int nIndex = g_pStringTableParticleEffectNames->FindStringIndex( pParticleSystemName );
		if (nIndex != INVALID_STRING_INDEX )
			return nIndex;

		Log_Warning(LOG_PARTICLES,"Server: Missing precache for particle system \"%s\"!\n", pParticleSystemName );
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetParticleSystemNameFromIndex( int nMaterialIndex )
{
	if ( nMaterialIndex < g_pStringTableParticleEffectNames->GetMaxStrings() )
		return g_pStringTableParticleEffectNames->GetString( nMaterialIndex );
	return "error";
}

//-----------------------------------------------------------------------------
// Precaches an effect (used by DispatchEffect)
//-----------------------------------------------------------------------------
void PrecacheEffect( const char *pEffectName )
{
	Assert( pEffectName && pEffectName[0] );
	g_pStringTableEffectDispatch->AddString( true, pEffectName );
}


//-----------------------------------------------------------------------------
// Converts a previously precached effect into an index
//-----------------------------------------------------------------------------
int GetEffectIndex( const char *pEffectName )
{
	if ( pEffectName )
	{
		int nIndex = g_pStringTableEffectDispatch->FindStringIndex( pEffectName );
		if (nIndex != INVALID_STRING_INDEX )
			return nIndex;

		Log_Warning(LOG_PARTICLES,"Server: Missing precache for effect \"%s\"!\n", pEffectName );
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts a previously precached effect index into a string
//-----------------------------------------------------------------------------
const char *GetEffectNameFromIndex( int nEffectIndex )
{
	if ( nEffectIndex < g_pStringTableEffectDispatch->GetMaxStrings() )
		return g_pStringTableEffectDispatch->GetString( nEffectIndex );
	return "error";
}

//-----------------------------------------------------------------------------
// Returns true if host_thread_mode is set to non-zero (and engine is running in threaded mode)
//-----------------------------------------------------------------------------
bool IsEngineThreaded()
{
	return host_thread_mode->GetBool();
}

class CServerGameEnts : public IServerGameEnts
{
public:
	virtual void			SetDebugEdictBase(edict_t *base);
	virtual void			MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 );
	virtual void			FreeContainingEntity( edict_t * ); 
	virtual edict_t*		BaseEntityToEdict( CBaseEntity *pEnt );
	virtual CBaseEntity*	EdictToBaseEntity( edict_t *pEdict );
	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts );
	virtual void			PrepareForFullUpdate( edict_t *pEdict );
};
EXPOSE_SINGLE_INTERFACE(CServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

void CServerGameEnts::SetDebugEdictBase(edict_t *base)
{
	g_pDebugEdictBase = base;
}

//-----------------------------------------------------------------------------
// Purpose: Marks entities as touching
// Input  : *e1 - 
//			*e2 - 
//-----------------------------------------------------------------------------
void CServerGameEnts::MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 )
{
	CBaseEntity *entity = GetContainingEntity( e1 );
	CBaseEntity *entityTouched = GetContainingEntity( e2 );
	if ( entity && entityTouched )
	{
		// HACKHACK: UNDONE: Pass in the trace here??!?!?
		trace_t tr;
		UTIL_ClearTrace( tr );
		tr.endpos = (entity->GetAbsOrigin() + entityTouched->GetAbsOrigin()) * 0.5;
		entity->PhysicsMarkEntitiesAsTouching( entityTouched, tr );
	}
}

void CServerGameEnts::FreeContainingEntity( edict_t *e )
{
	::FreeContainingEntity(e);
}

edict_t* CServerGameEnts::BaseEntityToEdict( CBaseEntity *pEnt )
{
	if ( pEnt )
		return pEnt->edict();
	else
		return NULL;
}

CBaseEntity* CServerGameEnts::EdictToBaseEntity( edict_t *pEdict )
{
	if ( pEdict )
		return CBaseEntity::Instance( pEdict );
	else
		return NULL;
}


/* Yuck.. ideally this would be in CServerNetworkProperty's header, but it requires CBaseEntity and
// inlining it gives a nice speedup.
inline void CServerNetworkProperty::CheckTransmit( CCheckTransmitInfo *pInfo )
{
	// If we have a transmit proxy, let it hook our ShouldTransmit return value.
	if ( m_pTransmitProxy )
	{
		nShouldTransmit = m_pTransmitProxy->ShouldTransmit( pInfo, nShouldTransmit );
	}

	if ( m_pOuter->ShouldTransmit( pInfo ) )
	{
		m_pOuter->SetTransmit( pInfo );
	}
} */

void CServerGameEnts::CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts )
{
	// NOTE: for speed's sake, this assumes that all networkables are CBaseEntities and that the edict list
	// is consecutive in memory. If either of these things change, then this routine needs to change, but
	// ideally we won't be calling any virtual from this routine. This speedy routine was added as an
	// optimization which would be nice to keep.
	edict_t *pBaseEdict = engine->PEntityOfEntIndex( 0 );

	// get recipient player's skybox:
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	Assert( pRecipientEntity && pRecipientEntity->IsPlayer() );
	if ( !pRecipientEntity )
		return;
	
	MDLCACHE_CRITICAL_SECTION();
	CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );
	const int skyBoxArea = pRecipientPlayer->m_Local.m_skybox3d.area;

	const bool bIsHLTV = pRecipientPlayer->IsHLTV();
	const bool bIsReplay = pRecipientPlayer->IsReplay();

	// m_pTransmitAlways must be set if HLTV client
	Assert( bIsHLTV == ( pInfo->m_pTransmitAlways != NULL) ||
		    bIsReplay == ( pInfo->m_pTransmitAlways != NULL) );

	for ( int i=0; i < nEdicts; i++ )
	{
		int iEdict = pEdictIndices[i];

		edict_t *pEdict = &pBaseEdict[iEdict];
		Assert( pEdict == engine->PEntityOfEntIndex( iEdict ) );
		int nFlags = pEdict->m_fStateFlags & (FL_EDICT_DONTSEND|FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_FULLCHECK);

		// entity needs no transmit
		if ( nFlags & FL_EDICT_DONTSEND )
			continue;
		
		// entity is already marked for sending
		if ( pInfo->m_pTransmitEdict->Get( iEdict ) )
			continue;
		
		if ( nFlags & FL_EDICT_ALWAYS )
		{
			// FIXME: Hey! Shouldn't this be using SetTransmit so as 
			// to also force network down dependent entities?
			while ( true )
			{
				// mark entity for sending
				pInfo->m_pTransmitEdict->Set( iEdict );
	
				if ( bIsHLTV || bIsReplay )
				{
					pInfo->m_pTransmitAlways->Set( iEdict );
				}

				CBaseEntity *pEnt = static_cast<CBaseEntity*>( pEdict->GetIServerEntity() );
				if ( !pEnt )
					break;

				CBaseEntity *pParent = pEnt->GetParent();
				if ( !pParent )
					break;

				pEdict = pParent->edict();
				iEdict = pParent->entindex();
			}
			continue;
		}

		// FIXME: Would like to remove all dependencies
		CBaseEntity *pEnt = ( CBaseEntity * )pEdict->GetIServerEntity();
		Assert( dynamic_cast< CBaseEntity* >( pEdict->GetUnknown() ) == pEnt );

		if ( nFlags == FL_EDICT_FULLCHECK )
		{
			// do a full ShouldTransmit() check, may return FL_EDICT_CHECKPVS
			nFlags = pEnt->ShouldTransmit( pInfo );

			Assert( !(nFlags & FL_EDICT_FULLCHECK) );

			if ( nFlags & FL_EDICT_ALWAYS )
			{
				pEnt->SetTransmit( pInfo, true );
				continue;
			}	
		}

		// don't send this entity
		if ( !( nFlags & FL_EDICT_PVSCHECK ) )
			continue;

		CServerNetworkProperty *netProp = static_cast<CServerNetworkProperty*>( pEdict->GetNetworkable() );

		if ( bIsHLTV || bIsReplay )
		{
			// for the HLTV/Replay we don't cull against PVS
			if ( netProp->AreaNum() == skyBoxArea )
			{
				pEnt->SetTransmit( pInfo, true );
			}
			else
			{
				pEnt->SetTransmit( pInfo, false );
			}
			continue;
		}

		// Always send entities in the player's 3d skybox.
		// Sidenote: call of AreaNum() ensures that PVS data is up to date for this entity
		bool bSameAreaAsSky = netProp->AreaNum() == skyBoxArea;
		if ( bSameAreaAsSky )
		{
			pEnt->SetTransmit( pInfo, true );
			continue;
		}

		bool bInPVS = netProp->IsInPVS( pInfo );
		if ( bInPVS || sv_force_transmit_ents.GetBool() )
		{
			// only send if entity is in PVS
			pEnt->SetTransmit( pInfo, false );
			continue;
		}

		// If the entity is marked "check PVS" but it's in hierarchy, walk up the hierarchy looking for the
		//  for any parent which is also in the PVS.  If none are found, then we don't need to worry about sending ourself
		CBaseEntity *orig = pEnt;
		CBaseEntity *check = pEnt->GetParent();

		// BUG BUG:  I think it might be better to build up a list of edict indices which "depend" on other answers and then
		// resolve them in a second pass.  Not sure what happens if an entity has two parents who both request PVS check?
        while ( check )
		{
			int checkIndex = check->entindex();

			// Parent already being sent
			if ( pInfo->m_pTransmitEdict->Get( checkIndex ) )
			{
				orig->SetTransmit( pInfo, true );
				break;
			}

			edict_t *checkEdict = check->edict();
			int checkFlags = checkEdict->m_fStateFlags & (FL_EDICT_DONTSEND|FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_FULLCHECK);
			if ( checkFlags & FL_EDICT_DONTSEND )
				break;

			if ( checkFlags & FL_EDICT_ALWAYS )
			{
				orig->SetTransmit( pInfo, true );
				break;
			}

			if ( checkFlags == FL_EDICT_FULLCHECK )
			{
				// do a full ShouldTransmit() check, may return FL_EDICT_CHECKPVS
				nFlags = check->ShouldTransmit( pInfo );
				Assert( !(nFlags & FL_EDICT_FULLCHECK) );
				if ( nFlags & FL_EDICT_ALWAYS )
				{
					check->SetTransmit( pInfo, true );
					orig->SetTransmit( pInfo, true );
				}
				break;
			}

			if ( checkFlags & FL_EDICT_PVSCHECK )
			{
				// Check pvs
				check->NetworkProp()->RecomputePVSInformation();
				bool bMoveParentInPVS = check->NetworkProp()->IsInPVS( pInfo );
				if ( bMoveParentInPVS )
				{
					orig->SetTransmit( pInfo, true );
					break;
				}
			}

			// Continue up chain just in case the parent itself has a parent that's in the PVS...
			check = check->GetParent();
		}
	}

//	Msg("A:%i, N:%i, F: %i, P: %i\n", always, dontSend, fullCheck, PVS );
}

//-----------------------------------------------------------------------------
// Purpose: called before a full update, so the server can flush any custom PVS info, etc
//-----------------------------------------------------------------------------
void CServerGameEnts::PrepareForFullUpdate( edict_t *pEdict )
{
	CBaseEntity *pEntity = CBaseEntity::Instance( pEdict );

	Assert( pEntity && pEntity->IsPlayer() );

	if ( !pEntity )
		return;

	CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pEntity );
	pPlayer->PrepareForFullUpdate();
}

CServerGameClients g_ServerGameClients;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS, g_ServerGameClients );


//-----------------------------------------------------------------------------
// Purpose: called when a player tries to connect to the server
// Input  : *pEdict - the new player
//			char *pszName - the players name
//			char *pszAddress - the IP address of the player
//			reject - output - fill in with the reason why
//			maxrejectlen -- sizeof output buffer
//			the player was not allowed to connect.
// Output : Returns TRUE if player is allowed to join, FALSE if connection is denied.
//-----------------------------------------------------------------------------
bool CServerGameClients::ClientConnect( edict_t *pEdict, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{	
	if ( !GameRules() )
		return true;
	
	return GameRules()->ClientConnected( pEdict, pszName, pszAddress, reject, maxrejectlen );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player is fully active (i.e. ready to receive messages)
// Input  : *pEntity - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientActive( edict_t *pEdict )
{
	MDLCACHE_CRITICAL_SECTION();
	
	::ClientActive( pEdict );

	// notify all entities that the player is now in the game
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		pEntity->PostClientActive();
	}

	// Tell the sound controller to check looping sounds
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	CSoundEnvelopeController::GetController().CheckLoopingSoundsForPlayer( pPlayer );
	SceneManager_ClientActive( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSpawned( edict_t *pPlayer )
{
	if ( GameRules() )
	{
		GameRules()->ClientSpawned( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player is fully connect ( initial baseline entities have been received )
// Input  : *pEntity - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientFullyConnect( edict_t *pEdict )
{
	::ClientFullyConnect( pEdict );
}

//-----------------------------------------------------------------------------
// Purpose: called when a player disconnects from a server
// Input  : *pEdict - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientDisconnect( edict_t *pEdict )
{
	extern bool	g_fGameOver;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if ( player )
	{
		if ( !g_fGameOver )
		{
			player->SetMaxSpeed( 0.0f );

			CSound *pSound;
			pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEdict ) );
			{
				// since this client isn't around to think anymore, reset their sound. 
				if ( pSound )
				{
					pSound->Reset();
				}
			}

		// since the edict doesn't get deleted, fix it so it doesn't interfere.
			player->RemoveFlag( FL_AIMTARGET ); // don't attract autoaim
			player->AddFlag( FL_DONTTOUCH );	// stop it touching anything
			player->AddFlag( FL_NOTARGET );	// stop NPCs noticing it
			player->AddSolidFlags( FSOLID_NOT_SOLID );		// nonsolid

			if ( GameRules() )
			{
				GameRules()->ClientDisconnected( pEdict );
				gamestats->Event_PlayerDisconnected( player );
			}
		}

		// Make sure all Untouch()'s are called for this client leaving
		CBaseEntity::PhysicsRemoveTouchedList( player );
		CBaseEntity::PhysicsRemoveGroundList( player );

		// Make sure anything we "own" is simulated by the server from now on
		player->ClearPlayerSimulationList();

	#if defined( TF_DLL )
		if ( !player->IsFakeClient() )
		{
			CSteamID steamID;
			if ( player->GetSteamID( &steamID ) )
			{
				GTFGCClientSystem()->ClientDisconnected( steamID );
			}
			else
			{
				if ( !player->IsReplay() && !player->IsHLTV() )
				{
					Log("WARNING: ClientDisconnected, but we don't know his SteamID?\n");
				}
			}
		}
	#endif
	}
}

void CServerGameClients::ClientPutInServer( edict_t *pEntity, const char *playername )
{
	if ( g_pClientPutInServerOverride )
		g_pClientPutInServerOverride( pEntity, playername );
	else
		::ClientPutInServer( pEntity, playername );
}

void CServerGameClients::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetContainingEntity( pEntity ) );
	::ClientCommand( pPlayer, args );
}

//-----------------------------------------------------------------------------
// Purpose: called after the player changes userinfo - gives dll a chance to modify 
//			it before it gets sent into the rest of the engine->
// Input  : *pEdict - the player
//			*infobuffer - their infobuffer
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSettingsChanged( edict_t *pEdict )
{
	// Is the client spawned yet?
	if ( !pEdict->GetUnknown() )
		return;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	
	if ( !player )
		return;

	bool bAllowNetworkingClientSettingsChange = GameRules()->IsConnectedUserInfoChangeAllowed( player );
	if ( bAllowNetworkingClientSettingsChange )
	{

#define QUICKGETCVARVALUE(v) (engine->GetClientConVarValue( player->entindex(), v ))

	// get network setting for prediction & lag compensation
	
	// Unfortunately, we have to duplicate the code in cdll_bounded_cvars.cpp here because the client
	// doesn't send the virtualized value up (because it has no way to know when the virtualized value
	// changes). Possible todo: put the responsibility on the bounded cvar to notify the engine when
	// its virtualized value has changed.		
	
	player->m_nUpdateRate = Q_atoi( QUICKGETCVARVALUE("cl_updaterate") );
	static const ConVar *pMinUpdateRate = g_pCVar->FindVar( "sv_minupdaterate" );
	static const ConVar *pMaxUpdateRate = g_pCVar->FindVar( "sv_maxupdaterate" );
	if ( pMinUpdateRate && pMaxUpdateRate )
		player->m_nUpdateRate = clamp( player->m_nUpdateRate, (int) pMinUpdateRate->GetFloat(), (int) pMaxUpdateRate->GetFloat() );

	bool useInterpolation = Q_atoi( QUICKGETCVARVALUE("cl_interpolate") ) != 0;
	if ( useInterpolation )
	{
		float flLerpRatio = Q_atof( QUICKGETCVARVALUE("cl_interp_ratio") );
		if ( flLerpRatio == 0 )
			flLerpRatio = 1.0f;
		float flLerpAmount = Q_atof( QUICKGETCVARVALUE("cl_interp") );

		static const ConVar *pMin = g_pCVar->FindVar( "sv_client_min_interp_ratio" );
		static const ConVar *pMax = g_pCVar->FindVar( "sv_client_max_interp_ratio" );
		if ( pMin && pMax && pMin->GetFloat() != -1 )
		{
			flLerpRatio = clamp( flLerpRatio, pMin->GetFloat(), pMax->GetFloat() );
		}
		else
		{
			if ( flLerpRatio == 0 )
				flLerpRatio = 1.0f;
		}
		// #define FIXME_INTERP_RATIO
		player->m_fLerpTime = MAX( flLerpAmount, flLerpRatio / player->m_nUpdateRate );
	}
	else
	{
		player->m_fLerpTime = 0.0f;
	}
	
	bool usePrediction = Q_atoi( QUICKGETCVARVALUE("cl_predict")) != 0;

	if ( usePrediction )
	{
		player->m_bPredictWeapons  = Q_atoi( QUICKGETCVARVALUE("cl_predictweapons")) != 0;
		player->m_bLagCompensation = Q_atoi( QUICKGETCVARVALUE("cl_lagcompensation")) != 0;
	}
	else
	{
		player->m_bPredictWeapons  = false;
		player->m_bLagCompensation = false;
	}
	

#undef QUICKGETCVARVALUE
	}

	GameRules()->ClientSettingsChanged( player );
}


#ifdef PORTAL
//-----------------------------------------------------------------------------
// Purpose: Runs CFuncAreaPortalBase::UpdateVisibility on each portal
// Input  : pAreaPortal - The Area portal to test for visibility from portals
// Output : int - 1 if any portal needs this area portal open, 0 otherwise.
//-----------------------------------------------------------------------------
int TestAreaPortalVisibilityThroughPortals ( CFuncAreaPortalBase* pAreaPortal, edict_t *pViewEntity, unsigned char *pvs, int pvssize  )
{
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
		return 0;

	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();

	for ( int i = 0; i != iPortalCount; ++i )
	{
		CProp_Portal* pLocalPortal = pPortals[ i ];
		if ( pLocalPortal && pLocalPortal->m_bActivated )
		{
			CProp_Portal* pRemotePortal = pLocalPortal->m_hLinkedPortal.Get();

			// Make sure this portal's linked portal is in the PVS before we add what it can see
			if ( pRemotePortal && pRemotePortal->m_bActivated && pRemotePortal->NetworkProp() && 
				pRemotePortal->NetworkProp()->IsInPVS( pViewEntity, pvs, pvssize ) )
			{
				bool bIsOpenOnClient = true;
				float fovDistanceAdjustFactor = 1.0f;
				Vector portalOrg = pLocalPortal->GetAbsOrigin();
				int iPortalNeedsThisPortalOpen = pAreaPortal->UpdateVisibility( portalOrg, fovDistanceAdjustFactor, bIsOpenOnClient );

				// Stop checking on success, this portal needs to be open
				if ( iPortalNeedsThisPortalOpen )
				{
					return iPortalNeedsThisPortalOpen;
				}
			}
		}
	}
	
	return 0;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
//  view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
//  entity's origin is used.  Either is offset by the m_vecViewOffset to get the eye position.
// From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
//  override the actual PAS or PVS values, or use a different origin.
// NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
// Input  : *pViewEntity - 
//			*pClient - 
//			**pvs - 
//			**pas - 
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs, int pvssize )
{
	Vector org;

	// Reset the PVS!!!
	engine->ResetPVS( pvs, pvssize );

#ifndef SWDS
	if(!engine->IsDedicatedServer()) {
		g_pToolFrameworkServer->PreSetupVisibility();
	}
#endif

	// Find the client's PVS
	CBaseEntity *pVE = NULL;
	if ( pViewEntity )
	{
		pVE = GetContainingEntity( pViewEntity );
		// If we have a viewentity, it overrides the player's origin
		if ( pVE )
		{
			org = pVE->EyePosition();
			engine->AddOriginToPVS( org );
		}
	}

	float fovDistanceAdjustFactor = 1;

	CBasePlayer *pPlayer = ( CBasePlayer * )GetContainingEntity( pClient );
	if ( pPlayer )
	{
		org = pPlayer->EyePosition();
		pPlayer->SetupVisibility( pVE, pvs, pvssize );
		UTIL_SetClientVisibilityPVS( pClient, pvs, pvssize );
		fovDistanceAdjustFactor = pPlayer->GetFOVDistanceAdjustFactorForNetworking();
	}

	unsigned char portalBits[MAX_AREA_PORTAL_STATE_BYTES];
	memset( portalBits, 0, sizeof( portalBits ) );
	
	int portalNums[512];
	int isOpen[512];
	int iOutPortal = 0;

	for( unsigned short i = g_AreaPortals.Head(); i != g_AreaPortals.InvalidIndex(); i = g_AreaPortals.Next(i) )
	{
		CFuncAreaPortalBase *pCur = g_AreaPortals[i];

		bool bIsOpenOnClient = true;
		
		// Update our array of which portals are open and flush it if necessary.		
		portalNums[iOutPortal] = pCur->m_portalNumber;
		isOpen[iOutPortal] = pCur->UpdateVisibility( org, fovDistanceAdjustFactor, bIsOpenOnClient );

#ifdef PORTAL
		// If the client doesn't need this open, test if portals might need this area portal open
		if ( isOpen[iOutPortal] == 0 )
		{
			isOpen[iOutPortal] = TestAreaPortalVisibilityThroughPortals( pCur, pViewEntity, pvs, pvssize );
		}
#endif

		++iOutPortal;
		if ( iOutPortal >= ARRAYSIZE( portalNums ) )
		{
			engine->SetAreaPortalStates( portalNums, isOpen, iOutPortal );
			iOutPortal = 0;
		}

		// Version 0 portals (ie: shipping Half-Life 2 era) are always treated as open
		// for purposes of the m_chAreaPortalBits array on the client.
		if ( pCur->m_iPortalVersion == 0 )
			bIsOpenOnClient = true;

		if ( bIsOpenOnClient )
		{
			if ( pCur->m_portalNumber < 0 )
				continue;
			else if ( pCur->m_portalNumber >= sizeof( portalBits ) * 8 )
				Error( "ClientSetupVisibility: portal number (%d) too large", pCur->m_portalNumber );
			else
				portalBits[pCur->m_portalNumber >> 3] |= (1 << (pCur->m_portalNumber & 7));
		}	
	}

	// Flush the remaining areaportal states.
	engine->SetAreaPortalStates( portalNums, isOpen, iOutPortal );

	if ( pPlayer )
	{
		// Update the area bits that get sent to the client.
		pPlayer->m_Local.UpdateAreaBits( pPlayer, portalBits );

#ifdef PORTAL 
		// *After* the player's view has updated its area bits, add on any other areas seen by portals
		CPortal_Player* pPortalPlayer = dynamic_cast<CPortal_Player*>( pPlayer );
		if ( pPortalPlayer )
		{
			pPortalPlayer->UpdatePortalViewAreaBits( pvs, pvssize );
		}
#endif //PORTAL
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			*buf - 
//			numcmds - 
//			totalcmds - 
//			dropped_packets - 
//			ignore - 
//			paused - 
// Output : float
//-----------------------------------------------------------------------------
#define CMD_MAXBACKUP 64

float CServerGameClients::ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
	int dropped_packets, bool ignore, bool paused )
{
#ifndef SWDS
	if(!engine->IsDedicatedServer()) {
		if( gpGlobals->maxClients == 1 ) {
			HackMgr_SetGamePaused( paused );
		}
	}
#endif

	int				i;
	CUserCmd		*from, *to;

	// We track last three command in case we drop some 
	//  packets but get them back.
	CUserCmd cmds[ CMD_MAXBACKUP ];  

	CUserCmd		cmdNull;  // For delta compression
	
	Assert( numcmds >= 0 );
	Assert( ( totalcmds - numcmds ) >= 0 );

	CBasePlayer *pPlayer = NULL;
	CBaseEntity *pEnt = CBaseEntity::Instance(player);
	if ( pEnt && pEnt->IsPlayer() )
	{
		pPlayer = static_cast< CBasePlayer * >( pEnt );
	}
	// Too many commands?
	if ( totalcmds < 0 || totalcmds >= ( CMD_MAXBACKUP - 1 ) )
	{
		const char *name = "unknown";
		if ( pPlayer )
		{
			name = pPlayer->GetPlayerName();
		}

		Msg("CBasePlayer::ProcessUsercmds: too many cmds %i sent for player %s\n", totalcmds, name );
		// FIXME:  Need a way to drop the client from here
		//SV_DropClient ( host_client, false, "CMD_MAXBACKUP hit" );
		buf->SetOverflowFlag();
		return 0.0f;
	}

	// Initialize for reading delta compressed usercmds
	cmdNull.Reset();
	from = &cmdNull;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		to = &cmds[ i ];
		ReadUsercmd( buf, to, from, pPlayer );
		from = to;
	}

	// Client not fully connected or server has gone inactive  or is paused, just ignore
	if ( ignore || !pPlayer )
	{
		return 0.0f;
	}

	MDLCACHE_CRITICAL_SECTION();
	pPlayer->ProcessUsercmds( cmds, numcmds, totalcmds, dropped_packets, paused );

	return TICK_INTERVAL;
}


void CServerGameClients::PostClientMessagesSent( void )
{
	VPROF("CServerGameClients::PostClient");
	gEntList.PostClientMessagesSent();
}

// Sets the client index for the client who typed the command into his/her console
void CServerGameClients::SetCommandClient( int index )
{
	g_nCommandClientIndex = index;
}

int	CServerGameClients::GetReplayDelay( edict_t *pEdict, int &entity )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );

	if ( !pPlayer )
		return 0;

	entity = pPlayer->GetReplayEntity();

	return pPlayer->GetDelayTicks();
}


//-----------------------------------------------------------------------------
// The client's userinfo data lump has changed
//-----------------------------------------------------------------------------
void CServerGameClients::ClientEarPosition( edict_t *pEdict, Vector *pEarOrigin )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if (pPlayer)
	{
		*pEarOrigin = pPlayer->EarPosition();
	}
	else
	{
		// Shouldn't happen
		Assert(0);
		*pEarOrigin = vec3_origin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
// Output : CPlayerState
//-----------------------------------------------------------------------------
CPlayerState *CServerGameClients::GetPlayerState( edict_t *player )
{
	// Is the client spawned yet?
	if ( !player || !player->GetUnknown() )
		return NULL;

	CBasePlayer *pBasePlayer = ( CBasePlayer * )CBaseEntity::Instance( player );
	if ( !pBasePlayer )
		return NULL;

	return &pBasePlayer->pl;
}

//-----------------------------------------------------------------------------
// Purpose: Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
//  can be added here
// Input  : *buf - 
//			buflen - 
//-----------------------------------------------------------------------------
void CServerGameClients::GetBugReportInfo( char *buf, int buflen )
{
	recentNPCSpeech_t speech[ SPEECH_LIST_MAX_SOUNDS ];
	int  num;
	int  i;

	buf[ 0 ] = 0;

	if ( gpGlobals->maxClients == 1 )
	{
		CBaseEntity *ent = FindPickerEntity( UTIL_GetCommandClientIfAdmin() ); 

		if ( ent )
		{
			Q_snprintf( buf, buflen, "Picker %i/%s - ent %s model %s\n",
				ent->entindex(),
				ent->GetClassname(),
				STRING( ent->GetEntityName() ),
				STRING( ent->GetModelName() ) );
		}

		// get any sounds that were spoken by NPCs recently
		num = GetRecentNPCSpeech( speech );
		if ( num > 0 )
		{
			Q_snprintf( buf, buflen, "%sRecent NPC speech:\n", buf );
			for( i = 0; i < num; i++ )
			{
				Q_snprintf( buf, buflen, "%s   time: %6.3f   sound name: %s   scene: %s\n", buf, speech[ i ].time, speech[ i ].name, speech[ i ].sceneName );
			}
			Q_snprintf( buf, buflen, "%sCurrent time: %6.3f\n", buf, gpGlobals->curtime );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A user has had their network id setup and validated 
//-----------------------------------------------------------------------------
void CServerGameClients::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
}

//-----------------------------------------------------------------------------
// Purpose: A player sent a voice packet
//-----------------------------------------------------------------------------
void CServerGameClients::ClientVoice( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if (pPlayer)
	{
		pPlayer->OnVoiceTransmit();
		
		// Notify the voice listener that we've spoken
		PlayerVoiceListener().AddPlayerSpeakTime( pPlayer );
	}
}

int CServerGameClients::GetMaxHumanPlayers()
{
	if ( GameRules() )
	{
		return GameRules()->GetMaxHumanPlayers();
	}
	return -1;
}

// The client has submitted a keyvalues command
void CServerGameClients::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	if ( !pKeyValues )
		return;

	char const *szCommand = pKeyValues->GetName();

	if ( GameRules() )
	{
		GameRules()->ClientCommandKeyValues( pEntity, pKeyValues );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bf_write *g_pMsgBuffer = NULL;

void EntityMessageBegin( CBaseEntity * entity, bool reliable /*= false*/ ) 
{
	Assert( !g_pMsgBuffer );

	Assert ( entity );

	g_pMsgBuffer = engine->EntityMessageBegin( entity->entindex(), entity->GetServerClass(), reliable );
}

void UserMessageBegin( IRecipientFilter& filter, const char *messagename )
{
	Assert( !g_pMsgBuffer );

	Assert( messagename );

	int msg_type = usermessages->LookupUserMessage( messagename );
	
	if ( msg_type == -1 )
	{
		Log_Error( LOG_USERMSG,"UserMessageBegin:  Unregistered message '%s'\n", messagename );
		g_pMsgBuffer = NULL;
	}
	else
	{
		g_pMsgBuffer = engine->UserMessageBegin( &filter, msg_type );
	}
}

void MessageEnd( void )
{
	Assert( g_pMsgBuffer );

	if( !g_pMsgBuffer ) {
		Log_Error( LOG_USERMSG, "MessageEnd called with no active message\n" );
		return;
	}

	engine->MessageEnd();

	g_pMsgBuffer = NULL;
}

void MessageWriteByte( int iValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WRITE_BYTE called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteByte( iValue );
}

void MessageWriteBytes( const void *pBuf, int nBytes )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG,"WRITE_BYTES called with no active message\n");
		return;
	}

	g_pMsgBuffer->WriteBytes( pBuf, nBytes );
}

void MessageWriteChar( int iValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WRITE_CHAR called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteChar( iValue );
}

void MessageWriteShort( int iValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WRITE_SHORT called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteWord( int iValue )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WRITE_WORD called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteWord( iValue );
}

void MessageWriteLong( long iValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteLong called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteLong( iValue );
}

void MessageWriteFloat( float flValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteFloat called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteFloat( flValue );
}

void MessageWriteAngle( float flValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteAngle called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBitAngle( flValue, 8 );
}

void MessageWriteCoord( float flValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteCoord called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBitCoord( flValue );
}

void MessageWriteVec3Coord( const Vector& rgflValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteVec3Coord called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBitVec3Coord( rgflValue );
}

void MessageWriteVec3Normal( const Vector& rgflValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteVec3Normal called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBitVec3Normal( rgflValue );
}

void MessageWriteBitVecIntegral( const Vector& vecValue )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "MessageWriteBitVecIntegral called with no active message\n" );
		return;
	}

	for ( int i = 0; i < 3; ++i )
	{
		g_pMsgBuffer->WriteBitCoordMP( vecValue[ i ], true, false );
	}
}

void MessageWriteAngles( const QAngle& rgflValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteVec3Normal called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBitAngles( rgflValue );
}

void MessageWriteString( const char *sz )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteString called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteString( sz );
}

void MessageWriteEntity( int iValue)
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteEntity called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteEHandle( CBaseEntity *pEntity )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteEHandle called with no active message\n" );
		return;
	}

	long iEncodedEHandle;
	
	if( pEntity )
	{
		EHANDLE hEnt = pEntity;

		int iSerialNum = hEnt.GetSerialNumber() & ( (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1 );
		iEncodedEHandle = hEnt.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		iEncodedEHandle = INVALID_NETWORKED_EHANDLE_VALUE;
	}
	
	g_pMsgBuffer->WriteLong( iEncodedEHandle );
}

// bitwise
void MessageWriteBool( bool bValue )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteBool called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteOneBit( bValue ? 1 : 0 );
}

void MessageWriteUBitLong( unsigned int data, int numbits )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteUBitLong called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteUBitLong( data, numbits );
}

void MessageWriteSBitLong( int data, int numbits )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteSBitLong called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteSBitLong( data, numbits );
}

void MessageWriteBits( const void *pIn, int nBits )
{
	if (!g_pMsgBuffer) {
		Log_Error( LOG_USERMSG, "WriteBits called with no active message\n" );
		return;
	}

	g_pMsgBuffer->WriteBits( pIn, nBits );
}

#ifndef SWDS
static bool IsDedicatedServer()
{
	if( engine )
		return engine->IsDedicatedServer();

	return false;
}
#endif

class CServerDLLSharedAppSystems : public IServerDLLSharedAppSystems
{
public:
	CServerDLLSharedAppSystems()
	{
	#ifndef SWDS
		if(!IsDedicatedServer()) {
			AddAppSystem( "soundemittersystem" DLL_EXT_STRING, SOUNDEMITTERSYSTEM_INTERFACE_VERSION );
			AddAppSystem( "scenefilecache" DLL_EXT_STRING, SCENE_FILE_CACHE_INTERFACE_VERSION );
		} else
	#endif
		{
			AddAppSystem( "soundemittersystem_srv" DLL_EXT_STRING, SOUNDEMITTERSYSTEM_INTERFACE_VERSION );
			AddAppSystem( "scenefilecache_srv" DLL_EXT_STRING, SCENE_FILE_CACHE_INTERFACE_VERSION );
		}

	#ifndef SWDS
		if(!IsDedicatedServer()) {
			AddAppSystem( "game_loopback" DLL_EXT_STRING, GAMELOOPBACK_INTERFACE_VERSION );
			AddAppSystem( "client" DLL_EXT_STRING, GAMECLIENTLOOPBACK_INTERFACE_VERSION );
		}
	#endif
	}

	virtual int	Count()
	{
		return m_Systems.Count();
	}
	virtual char const *GetDllName( int idx )
	{
		return m_Systems[ idx ].m_pModuleName;
	}
	virtual char const *GetInterfaceName( int idx )
	{
		return m_Systems[ idx ].m_pInterfaceName;
	}
private:
	void AddAppSystem( char const *moduleName, char const *interfaceName )
	{
		AppSystemInfo_t sys;
		sys.m_pModuleName = moduleName;
		sys.m_pInterfaceName = interfaceName;
		m_Systems.AddToTail( sys );
	}

	CUtlVector< AppSystemInfo_t >	m_Systems;
};

EXPOSE_SINGLE_INTERFACE( CServerDLLSharedAppSystems, IServerDLLSharedAppSystems, SERVER_DLL_SHARED_APPSYSTEMS );

static CServerGameTags s_GameServerTags;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerGameTags, IServerGameTags, INTERFACEVERSION_SERVERGAMETAGS, s_GameServerTags );

IServerGameTagsEx *CServerGameDLL::GetIServerGameTags()
{
	return &s_GameServerTags;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CServerGameTags::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	if ( pCvarTagList && GameRules() )
	{
		GameRules()->GetTaggedConVarList( pCvarTagList );
	}
}

void CServerGameTags::GetMatchmakingTags( char *buf, size_t bufSize )
{
	char * const bufBase = buf;

	// Trim the last comma if anything was written
	if ( buf > bufBase )
		buf[ -1 ] = 0;
}

void CServerGameTags::GetMatchmakingGameData( char *buf, size_t bufSize )
{
	char * const bufBase = buf;

	// Trim the last comma if anything was written
	if ( buf > bufBase )
		buf[ -1 ] = 0;
}

void CServerGameTags::GetServerBrowserGameData( char *buf, size_t bufSize )
{
	char * const bufBase = buf;

	// Trim the last comma if anything was written
	if ( buf > bufBase )
		buf[ -1 ] = 0;
}

bool CServerGameTags::GetServerBrowserMapOverride( char *buf, size_t bufSize )
{
	buf[0] = '\0';
	return false;
}

bool GetSteamIDForPlayerIndex( int iPlayerIndex, CSteamID &steamid )
{
	const CSteamID *pResult = engine->GetClientSteamIDByPlayerIndex( iPlayerIndex );
	if ( pResult ) {
		steamid = *pResult;
		return true;
	}

	// Return a bogus steam ID
	return false;
}

#ifndef SWDS
class CGameServerLoopback : public CBaseAppSystem< IGameServerLoopback >
{
public:
	virtual IRecastMgr *GetRecastMgr()
	{ return &RecastMgr(); }

	virtual map_datamap_t *GetMapDatamaps()
	{ return g_pMapDatamapsHead; }

#ifdef _DEBUG
	virtual const char *GetEntityClassname( int entnum, int iSerialNum )
	{
		CBaseEntity *pEnt = g_pEntityList->LookupEntityByNetworkIndex( entnum );
		if(pEnt)
			return pEnt->GetClassname();

		return NULL;
	}
#endif
};

static CGameServerLoopback s_ServerGameLoopback;
IGameServerLoopback *g_pGameServerLoopback = &s_ServerGameLoopback;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameServerLoopback, IGameServerLoopback, GAMESERVERLOOPBACK_INTERFACE_VERSION, s_ServerGameLoopback );
#endif
