//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include <crtmemdebug.h>
#include "vgui_int.h"
#include "clientmode.h"
#include "iinput.h"
#include "iviewrender.h"
#include "ivieweffects.h"
#include "ivmodemanager.h"
#include "prediction.h"
#include "clientsideeffects.h"
#include "particlemgr.h"
#include "steam/steam_api.h"
#include "initializer.h"
#include "smoke_fog_overlay.h"
#include "view.h"
#include "ienginevgui.h"
#include "iefx.h"
#include "enginesprite.h"
#include "networkstringtable_clientdll.h"
#include "voice_status.h"
#include "filesystem.h"
#include "c_te_legacytempents.h"
#include "c_rope.h"
#include "engine/ishadowmgr.h"
#include "engine/IStaticPropMgr.h"
#include "hud_basechat.h"
#include "hud_crosshair.h"
#include "view_shared.h"
#include "env_wind_shared.h"
#include "detailobjectsystem.h"
#include "clienteffectprecachesystem.h"
#include "soundenvelope.h"
#include "c_basetempentity.h"
#include "materialsystem/imaterialsystemstub.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/IShaderExtension.h"
#include "c_soundscape.h"
#include "engine/ivdebugoverlay.h"
#include "vguicenterprint.h"
#include "iviewrender_beams.h"
#include "tier0/vprof.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "usermessages.h"
#include "gamestringpool.h"
#include "c_user_message_register.h"
#include "IGameUIFuncs.h"
#include "igameevents.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "kbutton.h"
#include "tier0/icommandline.h"
#include "gamerules_register.h"
#include "vgui_controls/AnimationController.h"
#include "bitmap/tgawriter.h"
#include "c_world.h"
#include "perfvisualbenchmark.h"	
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "hud_closecaption.h"
#include "colorcorrectionmgr.h"
#include "physpropclientside.h"
#include "panelmetaclassmgr.h"
#include "c_vguiscreen.h"
#include "imessagechars.h"
#include "game/client/IGameClientExports.h"
#include "client_factorylist.h"
#include "ragdoll_shared.h"
#include "rendertexture.h"
#include "view_scene.h"
#include "iclientmode.h"
#include "con_nprint.h"
#include "inputsystem/iinputsystem.h"
#include "appframework/IAppSystemGroup.h"
#include "scenefilecache/ISceneFileCache.h"
#include "tier2/tier2dm.h"
#include "tier3/tier3.h"
#include "ihudlcd.h"
#include "toolframework_client.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/replay_ragdoll.h"
#include "qlimits.h"
#include "replay/replay.h"
#include "replay/ireplaysystem.h"
#include "replay/iclientreplay.h"
#include "replay/ienginereplay.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/iclientreplaycontext.h"
#include "replay/vgui/replayconfirmquitdlg.h"
#include "replay/vgui/replaybrowsermainpanel.h"
#include "replay/vgui/replayinputpanel.h"
#include "replay/vgui/replayperformanceeditor.h"
#endif
#include "vgui/ILocalize.h"
#include "vgui/IVGui.h"
#include "ipresence.h"
#include "cdll_bounded_cvars.h"
#include "matsys_controls/matsyscontrols.h"
#include "gamestats.h"
#include "particle_parse.h"
#include "renamed_recvtable_compat.h"
#include "mouthinfo.h"
#include "vgui/IInputInternal.h"
#include "game/client/iviewport.h"
#include "vstdlib/jobthread.h"
#include "imaterialproxydict.h"
#include "keybindinglistener.h"
#include "vgui_int.h"
#include "hackmgr/hackmgr.h"
#include "game_loopback/igameserverloopback.h"
#include "game_loopback/igameclientloopback.h"
#include "game_loopback/igameloopback.h"
#include "hackmgr/dlloverride.h"
#include "recast/recast_mgr.h"
#include "activitylist.h"
#include "gamepadui/igamepadui.h"
#include "GameUI/IGameUI.h"
#include "GameUI/IGameConsole.h"
#include "gameui_sys.h"
#include "game_controls/igameuisystemmgr.h"
#include "matchmaking/imatchframework.h"
#include "mapentities_shared.h"
#include "toolframework/iserverenginetools.h"
#include "ModelSoundsCache.h"
#include "querycache.h"
#include "point_template.h"
#include "baseclientrendertargets.h"
#include "viewpostprocess.h"
#include "iserver.h"

// NVNT includes
#include "hud_macros.h"

#if defined( TF_CLIENT_DLL )
#include "abuse_report.h"
#endif

#ifdef WORKSHOP_IMPORT_ENABLED
#include "fbxsystem/fbxsystem.h"
#endif

extern vgui::IInputInternal *g_InputInternal;

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

#include "achievements_and_stats_interface.h"

//=============================================================================
// HPE_END
//=============================================================================


#ifdef PORTAL
#include "PortalRender.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IClientMode *GetClientModeNormal();

extern ConVarBase *dsp_room;

// IF YOU ADD AN INTERFACE, EXTERN IT IN THE HEADER FILE.
IVEngineClient	*engine = NULL;
IVModelRender *modelrender = NULL;
IVEfx *effects = NULL;
IVRenderView *render = NULL;
IVDebugOverlay *debugoverlay = NULL;
IMaterialSystemStub *materials_stub = NULL;
IVModelInfoClient *modelinfo = NULL;
IEngineVGui *enginevgui = NULL;
INetworkStringTableContainer *networkstringtable = NULL;
ISpatialPartition* partition = NULL;
IShadowMgr *shadowmgr = NULL;
IStaticPropMgrClient *staticpropmgr = NULL;
IEngineSound *enginesound = NULL;
IUniformRandomStream *random_valve = NULL;
static CGaussianRandomStream s_GaussianRandomStream;
CGaussianRandomStream *randomgaussian = &s_GaussianRandomStream;
ISharedGameRules *sharedgamerules = NULL;
IEngineTrace *enginetrace = NULL;
IGameUIFuncs *gameuifuncs = NULL;
IGameEventManager2 *gameeventmanager = NULL;
ISceneFileCache *scenefilecache = NULL;
IMatchFramework *g_pMatchFramework = NULL;
IUploadGameStats *gamestatsuploader = NULL;
IClientReplayContext *g_pClientReplayContext = NULL;
#if defined( REPLAY_ENABLED )
IReplayManager *g_pReplayManager = NULL;
IReplayMovieManager *g_pReplayMovieManager = NULL;
IReplayScreenshotManager *g_pReplayScreenshotManager = NULL;
IReplayPerformanceManager *g_pReplayPerformanceManager = NULL;
IReplayPerformanceController *g_pReplayPerformanceController = NULL;
IEngineReplay *g_pEngineReplay = NULL;
IEngineClientReplay *g_pEngineClientReplay = NULL;
IReplaySystem *g_pReplay = NULL;
#endif
IServerEngineTools *serverenginetools = NULL;

CSysModule* gamepaduiDLL = NULL;
IGamepadUI* g_pGamepadUI = NULL;

CSysModule* gameuiDLL = NULL;

IGameUISystemMgr *g_pGameUISystemMgr = NULL;

IGameUI* g_pGameUIRedirectTarget = NULL;
IGameUIEx* g_pGameUIEx = NULL;
IGameUISystemMgr* g_pGameUISystemMgrRedirectTarget = NULL;
IGameConsole* g_pGameConsoleRedirectTarget = NULL;

class CGameUIRedirect : public IGameUI
{
public:
	virtual void Initialize( CreateInterfaceFn appFactory )
	{ g_pGameUIRedirectTarget->Initialize( appFactory ); }
	virtual void PostInit()
	{ g_pGameUIRedirectTarget->PostInit(); }

	// connect to other interfaces at the same level (gameui.dll/server.dll/client.dll)
	virtual void Connect( CreateInterfaceFn gameFactory )
	{ g_pGameUIRedirectTarget->Initialize( gameFactory ); }

	virtual void Start()
	{ g_pGameUIRedirectTarget->Start(); }
	virtual void Shutdown()
	{ g_pGameUIRedirectTarget->Shutdown(); }
	virtual void RunFrame()
	{ g_pGameUIRedirectTarget->RunFrame(); }

	// notifications
	virtual void OnGameUIActivated()
	{ g_pGameUIRedirectTarget->OnGameUIActivated(); }
	virtual void OnGameUIHidden()
	{ g_pGameUIRedirectTarget->OnGameUIHidden(); }

	virtual void OnLevelLoadingStarted(bool bShowProgressDialog)
	{ g_pGameUIRedirectTarget->OnLevelLoadingStarted(bShowProgressDialog); }
	virtual void OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason)
	{ g_pGameUIRedirectTarget->OnLevelLoadingFinished(bError, failureReason, extendedReason); }

	// level loading progress, returns true if the screen needs updating
	virtual bool UpdateProgressBar(float progress, const char *statusText)
	{ return g_pGameUIRedirectTarget->UpdateProgressBar(progress, statusText); }
	// Shows progress desc, returns previous setting... (used with custom progress bars )
	virtual bool SetShowProgressText( bool show )
	{ return g_pGameUIRedirectTarget->SetShowProgressText(show); }

	// !!!!!!!!!members added after "GameUI011" initial release!!!!!!!!!!!!!!!!!!!
	virtual void ShowNewGameDialog( int chapter )
	{ g_pGameUIRedirectTarget->ShowNewGameDialog(chapter); }

	// inserts specified panel as background for level load dialog
	virtual void SetLoadingBackgroundDialog( vgui::VPANEL panel )
	{ g_pGameUIRedirectTarget->SetLoadingBackgroundDialog(panel); }

	// Bonus maps interfaces
	virtual void BonusMapUnlock( const char *pchFileName = NULL, const char *pchMapName = NULL )
	{ g_pGameUIRedirectTarget->BonusMapUnlock(pchFileName, pchMapName); }
	virtual void BonusMapComplete( const char *pchFileName = NULL, const char *pchMapName = NULL )
	{ g_pGameUIRedirectTarget->BonusMapComplete(pchFileName, pchMapName); }
	virtual void BonusMapChallengeUpdate( const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest )
	{ g_pGameUIRedirectTarget->BonusMapChallengeUpdate(pchFileName, pchMapName, pchChallengeName, iBest); }
	virtual void BonusMapChallengeNames( char *pchFileName, char *pchMapName, char *pchChallengeName )
	{ g_pGameUIRedirectTarget->BonusMapChallengeNames(pchFileName, pchMapName, pchChallengeName); }
	virtual void BonusMapChallengeObjectives( int &iBronze, int &iSilver, int &iGold )
	{ g_pGameUIRedirectTarget->BonusMapChallengeObjectives(iBronze, iSilver, iGold); }
	virtual void BonusMapDatabaseSave( void )
	{ g_pGameUIRedirectTarget->BonusMapDatabaseSave(); }
	virtual int BonusMapNumAdvancedCompleted( void )
	{ return g_pGameUIRedirectTarget->BonusMapNumAdvancedCompleted(); }
	virtual void BonusMapNumMedals( int piNumMedals[ 3 ] )
	{ g_pGameUIRedirectTarget->BonusMapNumMedals(piNumMedals); }

	virtual void OnConnectToServer(const char *game, int IP, int connectionPort, int queryPort)
	{ g_pGameUIRedirectTarget->OnConnectToServer(game, IP, connectionPort, queryPort); }

	virtual void SetProgressOnStart()
	{ g_pGameUIRedirectTarget->SetProgressOnStart(); }
	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure )
	{ g_pGameUIRedirectTarget->OnDisconnectFromServer(eSteamLoginFailure); }

	virtual void OnConfirmQuit( void )
	{ g_pGameUIRedirectTarget->OnConfirmQuit(); }

	virtual bool IsMainMenuVisible( void )
	{ return g_pGameUIRedirectTarget->IsMainMenuVisible(); }

	// Client DLL is providing us with a panel that it wants to replace the main menu with
	virtual void SetMainMenuOverride( vgui::VPANEL panel )
	{ g_pGameUIRedirectTarget->SetMainMenuOverride(panel); }
	// Client DLL is telling us that a main menu command was issued, probably from its custom main menu panel
	virtual void SendMainMenuCommand( const char *pszCommand )
	{ g_pGameUIRedirectTarget->SendMainMenuCommand(pszCommand); }
};

static CGameUIRedirect g_GameUIRedirect;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUIRedirect, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUIRedirect);

class CGameUISystemMgrRedirect : public IGameUISystemMgr
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory )
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->Connect( factory ) : true; }
	virtual void Disconnect()
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->Disconnect();
	}

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName )
	{
		if (!Q_strcmp(	pInterfaceName, GAMEUISYSTEMMGR_INTERFACE_VERSION ))
			return (IGameUISystemMgr*)this;

		return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->QueryInterface( pInterfaceName ) : NULL;
	}

	// Init, shutdown
	virtual InitReturnVal_t Init()
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->Init() : INIT_OK; }
	virtual void Shutdown()
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->Shutdown();
	}

	virtual void SetGameUIVisible( bool bVisible )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->SetGameUIVisible( bVisible );
	}
	virtual bool GetGameUIVisible()
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->GetGameUIVisible() : false; }

	// Load the game UI menu screen
	// key values are owned and released by caller
	virtual IGameUISystem * LoadGameUIScreen( KeyValues *kvScreenLoadSettings )
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->LoadGameUIScreen( kvScreenLoadSettings ) : NULL; }
	virtual void ReleaseAllGameUIScreens()
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->ReleaseAllGameUIScreens();
	}

	virtual void SetSoundPlayback( IGameUISoundPlayback *pPlayback )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->SetSoundPlayback( pPlayback );
	}
	virtual void UseGameInputSystemEventQueue( bool bEnable )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->UseGameInputSystemEventQueue( bEnable );
	}
	virtual void SetInputContext( InputContextHandle_t hInputContext )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->SetInputContext( hInputContext );
	}
	virtual void RegisterInputEvent( const InputEvent_t &iEvent )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->RegisterInputEvent( iEvent );
	}

	virtual void RunFrame()
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->RunFrame();
	}
	virtual void Render( const Rect_t &viewport, DmeTime_t flCurrentTime )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->Render( viewport, flCurrentTime );
	}
	virtual void Render( IRenderContext *pRenderContext, PlatWindow_t hWnd, const Rect_t &viewport, DmeTime_t flCurrentTime )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->Render( pRenderContext, hWnd, viewport, flCurrentTime );
	}

	virtual void RegisterScreenControllerFactory( char const *szControllerName, IGameUIScreenControllerFactory *pFactory )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->RegisterScreenControllerFactory( szControllerName, pFactory );
	}
	virtual IGameUIScreenControllerFactory * GetScreenControllerFactory( char const *szControllerName )
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->GetScreenControllerFactory( szControllerName ) : NULL; }

	virtual void SendEventToAllScreens( KeyValues *kvGlobalEvent )
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->SendEventToAllScreens( kvGlobalEvent );
	}

	virtual IGameUISystemSurface * GetSurface()
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->GetSurface() : NULL; }
	virtual IGameUISchemeMgr * GetSchemeMgr()
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->GetSchemeMgr() : NULL; }
	virtual IGameUIMiscUtils * GetMiscUtils()
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->GetMiscUtils() : NULL; }

	// Init any render targets needed by the UI.
	virtual void InitRenderTargets()
	{
		if(g_pGameUISystemMgrRedirectTarget)
			g_pGameUISystemMgrRedirectTarget->InitRenderTargets();
	}
	virtual IMaterialProxy *CreateProxy( const char *proxyName )
	{ return g_pGameUISystemMgrRedirectTarget ? g_pGameUISystemMgrRedirectTarget->CreateProxy( proxyName ) : NULL; }
};

static CGameUISystemMgrRedirect g_GameUISystemMgrRedirect;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUISystemMgrRedirect, IGameUISystemMgr, GAMEUISYSTEMMGR_INTERFACE_VERSION, g_GameUISystemMgrRedirect);

class CGameConsoleRedirect : public IGameConsole
{
#ifdef __MINGW32__
private:
	void __DTOR__()
	{
		this->~CGameConsoleRedirect();
	}
#endif

public:
	// activates the console, makes it visible and brings it to the foreground
	virtual void Activate()
	{ g_pGameConsoleRedirectTarget->Activate(); }

	virtual void Initialize()
	{ g_pGameConsoleRedirectTarget->Initialize(); }

	// hides the console
	virtual void Hide()
	{ g_pGameConsoleRedirectTarget->Hide(); }

	// clears the console
	virtual void Clear()
	{ g_pGameConsoleRedirectTarget->Clear(); }

	// return true if the console has focus
	virtual bool IsConsoleVisible()
	{ return g_pGameConsoleRedirectTarget->IsConsoleVisible(); }

	virtual void SetParent( vgui::VPANEL parent )
	{ g_pGameConsoleRedirectTarget->SetParent(parent); }
};

static CGameConsoleRedirect g_GameConsoleRedirect;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameConsoleRedirect, IGameConsole, GAMECONSOLE_INTERFACE_VERSION, g_GameConsoleRedirect);

CSysModule* shaderDLL = NULL;
IShaderExtension* g_pShaderExtension = NULL;

CSysModule* game_loopbackDLL = NULL;
IGameLoopback* g_pGameLoopback = NULL;

CSysModule* serverDLL = NULL;
IGameServerLoopback* s_GameServerLoopback = NULL;

IServer *g_pListenServer = NULL;

bool IsListenServerHost()
{
	return g_pListenServer && g_pListenServer->IsActive();
}

IGameServerLoopback* GetGameServerLoopback()
{
	if( !IsListenServerHost() )
		return NULL;

	return s_GameServerLoopback;
}

CSysModule* videoServicesDLL = NULL;

CSysModule* vphysicsDLL = NULL;

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

AchievementsAndStatsInterface* g_pAchievementsAndStatsInterface = NULL;

//=============================================================================
// HPE_END
//=============================================================================

IGameSystem *SoundEmitterSystem();
IGameSystem *ToolFrameworkClientSystem();

static bool g_bRequestCacheUsedMaterials = false;
void RequestCacheUsedMaterials()
{
	g_bRequestCacheUsedMaterials = true;
}

void ProcessCacheUsedMaterials()
{
	if ( !g_bRequestCacheUsedMaterials )
		return;

	g_bRequestCacheUsedMaterials = false;
	if ( g_pMaterialSystem )
	{
        g_pMaterialSystem->CacheUsedMaterials();
	}
}

// String tables
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;
INetworkStringTable *g_pStringTableExtraParticleFiles = NULL;
INetworkStringTable *g_StringTableEffectDispatch = NULL;
INetworkStringTable *g_StringTableVguiScreen = NULL;
INetworkStringTable *g_pStringTableMaterials = NULL;
INetworkStringTable *g_pStringTableInfoPanel = NULL;
INetworkStringTable *g_pStringTableClientSideChoreoScenes = NULL;
INetworkStringTable *g_pStringTableServerMapCycle = NULL;

static CGlobalVarsBase dummyvars( true );
// So stuff that might reference gpGlobals during DLL initialization won't have a NULL pointer.
// Once the engine calls Init on this DLL, this pointer gets assigned to the shared data in the engine
CGlobalVarsBase *gpGlobals = &dummyvars;
class CHudChat;
class CViewRender;
extern CViewRender g_DefaultViewRender;

extern void StopAllRumbleEffects( void );

static C_BaseEntityClassList *s_pClassLists = NULL;
C_BaseEntityClassList::C_BaseEntityClassList()
{
	m_pNextClassList = s_pClassLists;
	s_pClassLists = this;
}
C_BaseEntityClassList::~C_BaseEntityClassList()
{
}

// Any entities that want an OnDataChanged during simulation register for it here.
class CDataChangedEvent
{
public:
	CDataChangedEvent() {}
	CDataChangedEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent )
	{
		m_pEntity = ent;
		m_UpdateType = updateType;
		m_pStoredEvent = pStoredEvent;
	}

	IClientNetworkable	*m_pEntity;
	DataUpdateType_t	m_UpdateType;
	int					*m_pStoredEvent;
};

CUtlLinkedList<CDataChangedEvent, unsigned short> g_DataChangedEvents;
ClientFrameStage_t g_CurFrameStage = FRAME_UNDEFINED;


class IMoveHelper;

void DispatchHudText( const char *pszName );

static ConVar s_CV_ShowParticleCounts("showparticlecounts", "0", 0, "Display number of particles drawn per frame");
static ConVar s_cl_team("cl_team", "default", FCVAR_USERINFO|FCVAR_ARCHIVE, "Default team when joining a game");
static ConVar s_cl_class("cl_class", "default", FCVAR_USERINFO|FCVAR_ARCHIVE, "Default class when joining a game");

// Physics system
bool g_bLevelInitialized = false;
bool g_bTextMode = false;

static ConVarBase *g_pcv_ThreadMode = NULL;

//-----------------------------------------------------------------------------
// Purpose: interface for gameui to modify voice bans
//-----------------------------------------------------------------------------
class CGameClientExports : public IGameClientExports
{
#ifdef __MINGW32__
private:
	void __DTOR__()
	{
		this->~CGameClientExports();
	}
#endif

public:
	// ingame voice manipulation
	bool IsPlayerGameVoiceMuted(int playerIndex)
	{
		return GetClientVoiceMgr()->IsPlayerBlocked(playerIndex);
	}

	void MutePlayerGameVoice(int playerIndex)
	{
		GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, true);
	}

	void UnmutePlayerGameVoice(int playerIndex)
	{
		GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, false);
	}

	void OnGameUIActivated( void )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "gameui_activated" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	void OnGameUIHidden( void )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "gameui_hidden" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

    //=============================================================================
    // HPE_BEGIN
    // [dwenger] Necessary for stats display
    //=============================================================================

    void CreateAchievementsPanel( vgui::Panel* pParent )
    {
        if (g_pAchievementsAndStatsInterface)
        {
            g_pAchievementsAndStatsInterface->CreatePanel( pParent );
        }
    }

    void DisplayAchievementPanel()
    {
        if (g_pAchievementsAndStatsInterface)
        {
            g_pAchievementsAndStatsInterface->DisplayPanel();
        }
    }

    void ShutdownAchievementPanel()
    {
        if (g_pAchievementsAndStatsInterface)
        {
            g_pAchievementsAndStatsInterface->ReleasePanel();
        }
    }

	int GetAchievementsPanelMinWidth( void ) const
	{
        if ( g_pAchievementsAndStatsInterface )
        {
            return g_pAchievementsAndStatsInterface->GetAchievementsPanelMinWidth();
        }

		return 0;
	}

    //=============================================================================
    // HPE_END
    //=============================================================================

	const char *GetHolidayString()
	{
		return UTIL_GetActiveHolidaysString();
	}

	// if true, the gameui applies the blur effect
	bool ClientWantsBlurEffect( void )
	{
		if ( GetViewPortInterface()->GetActivePanel() && GetViewPortInterface()->GetActivePanel()->WantsBackgroundBlurred() )
			return true;

		return false;
	}
};

EXPOSE_SINGLE_INTERFACE( CGameClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION );

class CClientDLLSharedAppSystems : public IClientDLLSharedAppSystems
{
public:
	CClientDLLSharedAppSystems()
	{
		AddAppSystem( "soundemittersystem" DLL_EXT_STRING, SOUNDEMITTERSYSTEM_INTERFACE_VERSION );
		AddAppSystem( "scenefilecache" DLL_EXT_STRING, SCENE_FILE_CACHE_INTERFACE_VERSION );
		AddAppSystem( "client", GAMEUISYSTEMMGR_INTERFACE_VERSION );
		if(!CommandLine()->HasParm("-textmode") && !CommandLine()->HasParm("-dedicated")) {
			AddAppSystem( "game_shader_dx9" DLL_EXT_STRING, SHADEREXTENSION_INTERFACE_VERSION );
		}
		AddAppSystem( "game_loopback" DLL_EXT_STRING, GAMELOOPBACK_INTERFACE_VERSION );
		if(!CommandLine()->HasParm("-nogamedll") && !CommandLine()->HasParm("-dedicated")) {
			AddAppSystem( "server" DLL_EXT_STRING, GAMESERVERLOOPBACK_INTERFACE_VERSION );
		}
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

EXPOSE_SINGLE_INTERFACE( CClientDLLSharedAppSystems, IClientDLLSharedAppSystems, CLIENT_DLL_SHARED_APPSYSTEMS );


//-----------------------------------------------------------------------------
// Helper interface for voice.
//-----------------------------------------------------------------------------
class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 128;
	}

	virtual void UpdateCursorState()
	{
	}

	virtual bool			CanShowSpeakerLabels()
	{
		return true;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;

//-----------------------------------------------------------------------------
// Code to display which entities are having their bones setup each frame.
//-----------------------------------------------------------------------------

ConVar cl_ShowBoneSetupEnts( "cl_ShowBoneSetupEnts", "0", 0, "Show which entities are having their bones setup each frame." );

class CBoneSetupEnt
{
public:
	char m_ModelName[128];
	int m_Index;
	int m_Count;
};

bool BoneSetupCompare( const CBoneSetupEnt &a, const CBoneSetupEnt &b )
{
	return a.m_Index < b.m_Index;
}

CUtlRBTree<CBoneSetupEnt> g_BoneSetupEnts( BoneSetupCompare );


void TrackBoneSetupEnt( C_BaseAnimating *pEnt )
{
#ifdef _DEBUG
	if ( !cl_ShowBoneSetupEnts.GetInt() )
		return;

	CBoneSetupEnt ent;
	ent.m_Index = pEnt->entindex();
	unsigned short i = g_BoneSetupEnts.Find( ent );
	if ( i == g_BoneSetupEnts.InvalidIndex() )
	{
		Q_strncpy( ent.m_ModelName, modelinfo->GetModelName( pEnt->GetModel() ), sizeof( ent.m_ModelName ) );
		ent.m_Count = 1;
		g_BoneSetupEnts.Insert( ent );
	}
	else
	{
		g_BoneSetupEnts[i].m_Count++;
	}
#endif
}

void DisplayBoneSetupEnts()
{
#ifdef _DEBUG
	if ( !cl_ShowBoneSetupEnts.GetInt() )
		return;

	unsigned short i;
	int nElements = 0;
	for ( i=g_BoneSetupEnts.FirstInorder(); i != g_BoneSetupEnts.LastInorder(); i=g_BoneSetupEnts.NextInorder( i ) )
		++nElements;
		
	engine->Con_NPrintf( 0, "%d bone setup ents (name/count/entindex) ------------", nElements );

	con_nprint_s printInfo;
	printInfo.time_to_live = -1;
	printInfo.fixed_width_font = true;
	printInfo.color[0] = printInfo.color[1] = printInfo.color[2] = 1;
	
	printInfo.index = 2;
	for ( i=g_BoneSetupEnts.FirstInorder(); i != g_BoneSetupEnts.LastInorder(); i=g_BoneSetupEnts.NextInorder( i ) )
	{
		CBoneSetupEnt *pEnt = &g_BoneSetupEnts[i];
		
		if ( pEnt->m_Count >= 3 )
		{
			printInfo.color[0] = 1;
			printInfo.color[1] = 0;
			printInfo.color[2] = 0;
		}
		else if ( pEnt->m_Count == 2 )
		{
			printInfo.color[0] = (float)200 / 255;
			printInfo.color[1] = (float)220 / 255;
			printInfo.color[2] = 0;
		}
		else
		{
			printInfo.color[0] = 1;
			printInfo.color[1] = 1;
			printInfo.color[2] = 1;
		}
		engine->Con_NXPrintf( &printInfo, "%25s / %3d / %3d", pEnt->m_ModelName, pEnt->m_Count, pEnt->m_Index );
		printInfo.index++;
	}

	g_BoneSetupEnts.RemoveAll();
#endif
}


CClientDll g_Client;
IBaseClientDLL *clientdll = &g_Client;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientDll, IBaseClientDLL, CLIENT_DLL_INTERFACE_VERSION, g_Client );


//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName )
{
	g_Client.PrecacheMaterial( pMaterialName );
}

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName )
{
	if (pMaterialName)
	{
		int nIndex = g_pStringTableMaterials->FindStringIndex( pMaterialName );
		Assert( nIndex >= 0 );
		if (nIndex >= 0)
			return nIndex;
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts precached material indices into strings
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nIndex )
{
	if (nIndex != (g_pStringTableMaterials->GetMaxStrings() - 1))
	{
		return g_pStringTableMaterials->GetString( nIndex );
	}
	else
	{
		return NULL;
	}
}


//-----------------------------------------------------------------------------
// Precaches a particle system
//-----------------------------------------------------------------------------
int PrecacheParticleSystem( const char *pParticleSystemName )
{
	int nIndex = g_pStringTableParticleEffectNames->AddString( false, pParticleSystemName );
	g_pParticleSystemMgr->PrecacheParticleSystem( pParticleSystemName );
	return nIndex;
}


//-----------------------------------------------------------------------------
// Converts a previously precached particle system into an index
//-----------------------------------------------------------------------------
int GetParticleSystemIndex( const char *pParticleSystemName )
{
	if ( pParticleSystemName )
	{
		int nIndex = g_pStringTableParticleEffectNames->FindStringIndex( pParticleSystemName );
		if ( nIndex != INVALID_STRING_INDEX )
			return nIndex;
		DevWarning("Client: Missing precache for particle system \"%s\"!\n", pParticleSystemName );
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts precached particle system indices into strings
//-----------------------------------------------------------------------------
const char *GetParticleSystemNameFromIndex( int nIndex )
{
	if ( nIndex < g_pStringTableParticleEffectNames->GetMaxStrings() )
		return g_pStringTableParticleEffectNames->GetString( nIndex );
	return "error";
}

void PrecacheEffect( const char *pEffectName )
{
	IClientEffect *pEffect = ClientEffectPrecacheSystem()->Find( pEffectName );
	if(!pEffect)
		return;

	pEffect->Precache();
}

//-----------------------------------------------------------------------------
// Returns true if host_thread_mode is set to non-zero (and engine is running in threaded mode)
//-----------------------------------------------------------------------------
bool IsEngineThreaded()
{
	if ( g_pcv_ThreadMode )
	{
		return g_pcv_ThreadMode->GetBool();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CClientDll::CClientDll() 
{
	// Kinda bogus, but the logic in the engine is too convoluted to put it there
	g_bLevelInitialized = false;

	m_pHudCloseCaption = NULL;

	SetDefLessFunc( m_CachedMaterials );
}

extern void InitializeClientCvars( void );

extern IGameSystem *ViewportClientSystem();

static ConVar cl_threaded_init("cl_threaded_init", "0");

bool InitParticleManager()
{
	if (!ParticleMgr()->Init(MAX_TOTAL_PARTICLES, g_pMaterialSystem))
		return false;

	return true;
}

bool InitGameSystems( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory )
{
	if(!g_bTextMode) {
		if (!VGui_Startup( appSystemFactory ))
			return false;
	}

	// Add the client systems.	
	
	// Client Leaf System has to be initialized first, since DetailObjectSystem uses it
	IGameSystem::Add( GameStringSystem() );
	IGameSystem::Add( SoundEmitterSystem() );
	if(!g_bTextMode) {
		IGameSystem::Add( ToolFrameworkClientSystem() );
		IGameSystem::Add( ClientLeafSystem() );
		IGameSystem::Add( DetailObjectSystem() );
		IGameSystem::Add( ViewportClientSystem() );
		IGameSystem::Add( ClientEffectPrecacheSystem() );
		IGameSystem::Add( g_pClientShadowMgr );
		IGameSystem::Add( g_pColorCorrectionMgr );	// NOTE: This must happen prior to ClientThinkList (color correction is updated there)
		IGameSystem::Add( g_pGameUIGameSystem );
	}
	IGameSystem::Add( ClientThinkList() );
	IGameSystem::Add( ClientSoundscapeSystem() );
	if(!g_bTextMode) {
		IGameSystem::Add( PerfVisualBenchmark() );
	}

#if defined( TF_CLIENT_DLL )
	IGameSystem::Add( CustomTextureToolCacheGameSystem() );
	IGameSystem::Add( TFSharedContentManager() );
#endif

#if defined( TF_CLIENT_DLL )
	if ( g_AbuseReportMgr != NULL )
	{
		IGameSystem::Add( g_AbuseReportMgr );
	}
#endif

#if defined( CLIENT_DLL ) && defined( COPY_CHECK_STRESSTEST )
	IGameSystem::Add( GetPredictionCopyTester() );
#endif

	modemanager->Init( );

	if(!g_bTextMode) {
		// Load the ClientScheme just once
		vgui::scheme()->LoadSchemeFromFileEx( VGui_GetFullscreenRootVPANEL(), "resource/ClientScheme.res", "ClientScheme");

		if(GetClientMode() != GetFullscreenClientMode()) {
			GetClientMode()->InitViewport();
		}
		GetFullscreenClientMode()->InitViewport();

		GetHud().Init();
	}

	if(GetClientMode() != GetFullscreenClientMode()) {
		GetClientMode()->Init();
	}
	GetFullscreenClientMode()->Init();

	if ( !IGameSystem::InitAllSystems() )
		return false;

	// Due to dependencies, these are not autogamesystems
	if ( !ModelSoundsCacheInit() )
	{
		return false;
	}

	if(GetClientMode() != GetFullscreenClientMode()) {
		GetClientMode()->Enable();
	}
	GetFullscreenClientMode()->EnableWithRootPanel( VGui_GetFullscreenRootVPANEL() );

	if(!g_bTextMode) {
		GetViewRenderInstance()->Init();
		GetViewEffects()->Init();
	}

	C_BaseTempEntity::PrecacheTempEnts();

	input->Init_All();

	if(!g_bTextMode) {
		VGui_CreateGlobalPanels();

		InitSmokeFogOverlay();
	}

	// Register user messages..
	CUserMessageRegister::RegisterAll();

	ClientVoiceMgr_Init();

	// Embed voice status icons inside chat element
	{
		vgui::VPANEL parent = enginevgui->GetPanel( PANEL_CLIENTDLL );
		GetClientVoiceMgr()->Init( &g_VoiceStatusHelper, parent );
	}

	if ( !PhysicsDLLInit( physicsFactory ) )
		return false;
	
	InvalidateQueryCache();

	RecastMgr().Init();

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

	COM_TimestampedLog( "C_BaseAnimating::InitBoneSetupThreadPool" );
	C_BaseAnimating::InitBoneSetupThreadPool();

	return true;
}

static void *ClientCreateInterfaceHook(const char *pName, int *pCode)
{
	static char buffer[MAX_PATH];

	const char *pProxyInterface = V_strstr(pName, INTERNAL_IMATERIAL_PROXY_INTERFACE_VERSION);
	if(pProxyInterface != NULL) {
		int namelen = (pProxyInterface-pName)+1;
		V_strncpy(buffer, pName, namelen);
		buffer[namelen] = '\0';
		IMaterialProxy *proxy = g_Client.InstantiateMaterialProxy(buffer);
		Assert(proxy);
		if(proxy) {
			if(pCode)
				*pCode = IFACE_OK;
		} else {
			if(pCode)
				*pCode = IFACE_FAILED;
		}
		return proxy;
	}

	if(pCode) {
		*pCode = IFACE_FAILED;
	}

	return NULL;
}

INIT_PRIORITY(101) struct ClientCreateInterfaceHookInit {
	ClientCreateInterfaceHookInit() {
		Sys_SetCreateInterfaceHook(ClientCreateInterfaceHook);
	}
} g_ClientCreateInterfaceHookInit;

int CClientDll::Connect( CreateInterfaceFn appSystemFactory, CGlobalVarsBase *pGlobals )
{
	if ( CommandLine()->HasParm( "-textmode" ) || CommandLine()->HasParm( "-dedicated" ) )
		g_bTextMode = true;

	InitCRTMemDebug();
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	// Hook up global variables
	gpGlobals = pGlobals;

	Sys_SetCreateInterfaceHook(ClientCreateInterfaceHook);

	ConnectTier1Libraries( &appSystemFactory, 1 );
	ConnectTier2Libraries( &appSystemFactory, 1 );
	ConnectTier3Libraries( &appSystemFactory, 1 );

	// Connected in ConnectTier1Libraries
	if ( g_pCVar == NULL )
	{
		return false;
	}

	// Initialize the console variables.
	InitializeClientCvars();

	if(!g_bTextMode) {
		g_pMaterialSystem->BeginRenderTargetAllocation();
		g_BaseClientRenderTargets.InitClientRenderTargetsReal( g_pMaterialSystem, g_pMaterialSystemHardwareConfig );
		g_pMaterialSystem->EndRenderTargetAllocation();
	}

	GetCurrentTonemappingSystem()->UpdateBucketRanges();

	return true;
}

// Purpose: Called when the DLL is first loaded.
// Input  : engineFactory - 
// Output : int
//-----------------------------------------------------------------------------
int CClientDll::Init( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals )
{
	if ( (g_pFullFileSystem = (IFileSystem *)appSystemFactory(FILESYSTEM_INTERFACE_VERSION, NULL)) == NULL )
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

	if(!HackMgr_Client_PreInit(this, appSystemFactory, physicsFactory, pGlobals))
		return false;

	factorylist_t factories;
	factories.appSystemFactory = appSystemFactory;
	factories.physicsFactory = physicsFactory;
	FactoryList_Store( factories );

	if(!Connect( appSystemFactory, pGlobals ))
		return false;

	COM_TimestampedLog( "ClientDLL factories - Start" );
	// We aren't happy unless we get all of our interfaces.
	// please don't collapse this into one monolithic boolean expression (impossible to debug)
	if ( (engine = (IVEngineClient *)appSystemFactory( VENGINE_CLIENT_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (enginetrace = (IEngineTrace *)appSystemFactory( INTERFACEVERSION_ENGINETRACE_CLIENT, NULL )) == NULL )
		return false;
	if ( !g_pDataCache )
		return false;
	if ( !g_pMDLCache )
		return false;
	if ( (modelinfo = (IVModelInfoClient *)appSystemFactory(VMODELINFO_CLIENT_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (networkstringtable = (INetworkStringTableContainer *)appSystemFactory(INTERFACENAME_NETWORKSTRINGTABLECLIENT,NULL)) == NULL )
		return false;
	if ( (partition = (ISpatialPartition *)appSystemFactory(INTERFACEVERSION_SPATIALPARTITION, NULL)) == NULL )
		return false;
	if ( (staticpropmgr = (IStaticPropMgrClient *)appSystemFactory(INTERFACEVERSION_STATICPROPMGR_CLIENT, NULL)) == NULL )
		return false;
	if ( (enginesound = (IEngineSound *)appSystemFactory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (random_valve = (IUniformRandomStream *)appSystemFactory(VENGINE_CLIENT_RANDOM_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (gameeventmanager = (IGameEventManager2 *)appSystemFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL)) == NULL )
		return false;
	if ( !g_pSoundEmitterSystem )
		return false;
	if ( !g_pInputSystem )
		return false;
	if ( (scenefilecache = (ISceneFileCache *)appSystemFactory( SCENE_FILE_CACHE_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( ( gamestatsuploader = (IUploadGameStats *)appSystemFactory( INTERFACEVERSION_UPLOADGAMESTATS, NULL )) == NULL )
		return false;

	if( !g_bTextMode ) {
		if ( (gameuifuncs = (IGameUIFuncs * )appSystemFactory( VENGINE_GAMEUIFUNCS_VERSION, NULL )) == NULL )
			return false;
		if ( (shadowmgr = (IShadowMgr *)appSystemFactory(ENGINE_SHADOWMGR_INTERFACE_VERSION, NULL)) == NULL )
			return false;
		if ( (enginevgui = (IEngineVGui *)appSystemFactory(VENGINE_VGUI_VERSION, NULL )) == NULL )
			return false;
		if ( (modelrender = (IVModelRender *)appSystemFactory( VENGINE_HUDMODEL_INTERFACE_VERSION, NULL )) == NULL )
			return false;
		if ( (effects = (IVEfx *)appSystemFactory( VENGINE_EFFECTS_INTERFACE_VERSION, NULL )) == NULL )
			return false;
		if ( (render = (IVRenderView *)appSystemFactory( VENGINE_RENDERVIEW_INTERFACE_VERSION, NULL )) == NULL )
			return false;
	}

	g_pMatchFramework = (IMatchFramework *)appSystemFactory( IMATCHFRAMEWORK_VERSION_STRING, NULL );

	if(!CommandLine()->HasParm("-nogamedll") && !CommandLine()->HasParm("-dedicated")) {
		IVEngineServer *engserver = (IVEngineServer *)appSystemFactory( INTERFACEVERSION_VENGINESERVER, NULL );
		if(engserver) {
			g_pListenServer = engserver->GetIServer();
		}
	}

	if( !g_bTextMode ) {
		debugoverlay = (IVDebugOverlay *)appSystemFactory( VDEBUG_OVERLAY_INTERFACE_VERSION, NULL );
	}

#if defined( REPLAY_ENABLED )
	if ( (g_pEngineReplay = (IEngineReplay *)appSystemFactory( ENGINE_REPLAY_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (g_pEngineClientReplay = (IEngineClientReplay *)appSystemFactory( ENGINE_REPLAY_CLIENT_INTERFACE_VERSION, NULL )) == NULL )
		return false;
#endif

	if (!g_pMatSystemSurface)
		return false;

	if( !g_bTextMode ) {
		// This interface is optional, and is only valid when running with -tools
		serverenginetools = ( IServerEngineTools * )appSystemFactory( VSERVERENGINETOOLS_INTERFACE_VERSION, NULL );
	}

#ifdef WORKSHOP_IMPORT_ENABLED
	if ( !ConnectDataModel( appSystemFactory ) )
		return false;
	if ( InitDataModel() != INIT_OK )
		return false;
	InitFbx();
#endif

	if(!g_bTextMode) {
		HackMgr_SwapVideoServices( appSystemFactory, videoServicesDLL );

		V_strcat_safe(gamebin_path, "GameUI" DLL_EXT_STRING);
		gameuiDLL = Sys_LoadModule(gamebin_path);
		gamebin_path[gamebin_length] = '\0';
		if(gameuiDLL) {
			CreateInterfaceFn gameuiFactory = Sys_GetFactory(gameuiDLL);
			if(gameuiFactory) {
				int status = IFACE_OK;
				g_pGameUIRedirectTarget = (IGameUI *)gameuiFactory(GAMEUI_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					g_pGameUIRedirectTarget = NULL;
				}

				g_pGameUIEx = (IGameUIEx *)gameuiFactory(GAMEUI_EX_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					g_pGameUIEx = NULL;
				}

				status = IFACE_OK;
				g_pGameConsoleRedirectTarget = (IGameConsole *)gameuiFactory(GAMECONSOLE_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					g_pGameConsoleRedirectTarget = NULL;
				}

				status = IFACE_OK;
				g_pGameUISystemMgrRedirectTarget = (IGameUISystemMgr *)gameuiFactory(GAMEUISYSTEMMGR_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					g_pGameUISystemMgrRedirectTarget = NULL;
				}
			}
		}

		g_pGameUISystemMgr = (IGameUISystemMgr*)appSystemFactory( GAMEUISYSTEMMGR_INTERFACE_VERSION, NULL );

		V_strcat_safe(gamebin_path, "gamepadui" DLL_EXT_STRING);
		Sys_LoadInterface( gamebin_path, GAMEPADUI_INTERFACE_VERSION, &gamepaduiDLL, reinterpret_cast< void** >( &g_pGamepadUI ) );
		gamebin_path[gamebin_length] = '\0';

		if(g_pGamepadUI) {
			g_pGamepadUI->Initialize(appSystemFactory);
		}

		V_strcat_safe(gamebin_path, "game_shader_dx9" DLL_EXT_STRING);
		Sys_LoadInterface( gamebin_path, SHADEREXTENSION_INTERFACE_VERSION, &shaderDLL, reinterpret_cast< void** >( &g_pShaderExtension ) );
		gamebin_path[gamebin_length] = '\0';
	}

	V_strcat_safe(gamebin_path, "game_loopback" DLL_EXT_STRING);
	Sys_LoadInterface( gamebin_path, GAMELOOPBACK_INTERFACE_VERSION, &game_loopbackDLL, reinterpret_cast< void** >( &g_pGameLoopback ) );
	gamebin_path[gamebin_length] = '\0';

	if(!CommandLine()->HasParm("-nogamedll") && !CommandLine()->HasParm("-dedicated")) {
		V_strcat_safe(gamebin_path, "server" DLL_EXT_STRING);
		serverDLL = Sys_LoadModule(gamebin_path);
		gamebin_path[gamebin_length] = '\0';
		if(serverDLL) {
			CreateInterfaceFn serverFactory = Sys_GetFactory(serverDLL);
			if(serverFactory) {
				int status = IFACE_OK;
				s_GameServerLoopback = (IGameServerLoopback *)serverFactory(GAMESERVERLOOPBACK_INTERFACE_VERSION, &status);
				if(status != IFACE_OK) {
					s_GameServerLoopback = NULL;
				}
			}
		}
	}

	COM_TimestampedLog( "g_pSoundEmitterSystem->Connect" );
	// Yes, both the client and game .dlls will try to Connect, the soundemittersystem.dll will handle this gracefully
	if ( !g_pSoundEmitterSystem->Connect( appSystemFactory ) )
	{
		return false;
	}

	if ( CommandLine()->FindParm( "-makedevshots" ) )
		g_MakingDevShots = true;

	// Not fatal if the material system stub isn't around.
	materials_stub = (IMaterialSystemStub*)appSystemFactory( MATERIAL_SYSTEM_STUB_INTERFACE_VERSION, NULL );

	if( !g_pMaterialSystemHardwareConfig )
		return false;

	// Hook up the gaussian random number generator
	s_GaussianRandomStream.AttachToStream( random_valve );

	g_pcv_ThreadMode = g_pCVar->FindVarBase( "host_thread_mode" );

	if (!Initializer::InitializeAllObjects())
		return false;

	engine->SetRestrictServerCommands(true);
	engine->SetRestrictClientCommands(true);

	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;

	bool bInitSuccess = false;
	if ( cl_threaded_init.GetBool() )
	{
		CFunctorJob *pGameJob = new CFunctorJob( CreateFunctor( InitParticleManager ) );
		g_pThreadPool->AddJob( pGameJob );
		bInitSuccess = InitGameSystems( appSystemFactory, physicsFactory );
		pGameJob->WaitForFinishAndRelease();
	}
	else
	{
		COM_TimestampedLog( "ParticleMgr()->Init" );
		if (!InitParticleManager())
			return false;

		COM_TimestampedLog( "InitGameSystems - Start" );
		bInitSuccess = InitGameSystems( appSystemFactory, physicsFactory );
		COM_TimestampedLog( "InitGameSystems - End" );
	}

	if(!bInitSuccess)
		return false;

	if(!g_bTextMode) {
		// This is a fullscreen element, so only lives on slot 0!!!
		m_pHudCloseCaption = GET_FULLSCREEN_HUDELEMENT( CHudCloseCaption );
	}

	COM_TimestampedLog( "ClientDLL Init - Finish" );
	return true;
}

bool CClientDll::ReplayInit( CreateInterfaceFn fnReplayFactory )
{
#if defined( REPLAY_ENABLED )
	if ( !IsPC() )
		return false;
	if ( (g_pReplay = (IReplaySystem *)fnReplayFactory( REPLAY_INTERFACE_VERSION, NULL ) ) == NULL )
		return false;
	if ( (g_pClientReplayContext = g_pReplay->CL_GetContext()) == NULL )
		return false;

	return true;
#else
	return false;
#endif
}

bool CClientDll::ReplayPostInit()
{
#if defined( REPLAY_ENABLED )
	if ( ( g_pReplayManager = g_pClientReplayContext->GetReplayManager() ) == NULL )
		return false;
	if ( ( g_pReplayScreenshotManager = g_pClientReplayContext->GetScreenshotManager() ) == NULL )
		return false;
	if ( ( g_pReplayPerformanceManager = g_pClientReplayContext->GetPerformanceManager() ) == NULL )
		return false;
	if ( ( g_pReplayPerformanceController = g_pClientReplayContext->GetPerformanceController() ) == NULL )
		return false;
	if ( ( g_pReplayMovieManager = g_pClientReplayContext->GetMovieManager() ) == NULL )
		return false;
	return true;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after client & server DLL are loaded and all systems initialized
//-----------------------------------------------------------------------------
void CClientDll::PostInit()
{
	COM_TimestampedLog( "IGameSystem::PostInitAllSystems - Start" );
	IGameSystem::PostInitAllSystems();
	COM_TimestampedLog( "IGameSystem::PostInitAllSystems - Finish" );

	dsp_room->SetValue( 1 );
}

void VGui_ClearVideoPanels();

//-----------------------------------------------------------------------------
// Purpose: Called when the client .dll is being dismissed
//-----------------------------------------------------------------------------
void CClientDll::Shutdown( void )
{
	if (g_pAchievementsAndStatsInterface)
	{
		g_pAchievementsAndStatsInterface->ReleasePanel();
	}

	VGui_ClearVideoPanels();

	if ( g_pVideo )
	{
		g_pVideo->Shutdown();
		g_pVideo->Disconnect();
		g_pVideo = nullptr;
	}

	C_BaseAnimating::ShutdownBoneSetupThreadPool();

	ClientVoiceMgr_Shutdown();

	Initializer::FreeAllObjects();

	if(GetClientMode() != GetFullscreenClientMode()) {
		GetClientMode()->Disable();
	}
	GetFullscreenClientMode()->Disable();

	if(GetClientMode() != GetFullscreenClientMode()) {
		GetClientMode()->Shutdown();
	}
	GetFullscreenClientMode()->Shutdown();

	input->Shutdown_All();
	TermSmokeFogOverlay();
	GetViewRenderInstance()->Shutdown();
	g_pParticleSystemMgr->UncacheAllParticleSystems();
	UncacheAllMaterials();

	IGameSystem::ShutdownAllSystems();
	
	GetHud().Shutdown();
	VGui_Shutdown();

	RopeManager()->Shutdown();

	ParticleMgr()->Term();

	// Due to dependencies, these are not autogamesystems
	ModelSoundsCacheShutdown();

	if(g_pGamepadUI) {
		g_pGamepadUI->Shutdown();
	}

	if ( shaderDLL )
		Sys_UnloadModule( shaderDLL );

	if ( serverDLL )
		Sys_UnloadModule( serverDLL );

	if ( game_loopbackDLL )
		Sys_UnloadModule( game_loopbackDLL );

	if ( videoServicesDLL )
		Sys_UnloadModule( videoServicesDLL );

	if ( gameuiDLL )
		Sys_UnloadModule( gameuiDLL );

	if ( gamepaduiDLL )
		Sys_UnloadModule( gamepaduiDLL );

	if ( vphysicsDLL )
		Sys_UnloadModule( vphysicsDLL );
	
	ClearKeyValuesCache();

#ifdef WORKSHOP_IMPORT_ENABLED
	ShutdownDataModel();
	DisconnectDataModel();
	ShutdownFbx();
#endif
	
	// This call disconnects the VGui libraries which we rely on later in the shutdown path, so don't do it
	DisconnectTier3Libraries( );
	DisconnectTier2Libraries( );
	ConVar_Unregister();
	DisconnectTier1Libraries( );

	gameeventmanager = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//  Called when the game initializes
//  and whenever the vid_mode is changed
//  so the HUD can reinitialize itself.
// Output : int
//-----------------------------------------------------------------------------
int CClientDll::HudVidInit( void )
{
	if(!g_bTextMode) {
		if(g_pGamepadUI) {
			g_pGamepadUI->VidInit();
		}

		GetHud().VidInit();
	}

	GetClientVoiceMgr()->VidInit();

	return 1;
}

//-----------------------------------------------------------------------------
// Method used to allow the client to filter input messages before the 
// move record is transmitted to the server
//-----------------------------------------------------------------------------
void CClientDll::HudProcessInput( bool bActive )
{
	GetClientMode()->ProcessInput( bActive );
}

extern void ServiceEventQueue( void );

//-----------------------------------------------------------------------------
// Purpose: Called when shared data gets changed, allows dll to modify data
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CClientDll::HudUpdate( bool bActive )
{
	float frametime = gpGlobals->frametime;

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

	if(!g_bTextMode) {
		if(g_pGamepadUI) {
			g_pGamepadUI->OnUpdate(frametime);
		}
	}

#if defined( TF_CLIENT_DLL )
	CRTime::UpdateRealTime();
#endif

	GetClientVoiceMgr()->Frame( frametime );

	if(!g_bTextMode) {
		GetHud().UpdateHud( bActive );
	}

	{
		VPROF( "UpdateQueryCache" );
		UpdateQueryCache();
	}

	{
		C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false ); 
		IGameSystem::UpdateAllSystems( frametime );
	}

	RecastMgr().Update( frametime );

	if(!g_bTextMode) {
		// run vgui animations
		vgui::GetAnimationController()->UpdateAnimations( engine->Time() );
	}

	hudlcd->SetGlobalStat( "(time_int)", VarArgs( "%d", (int)gpGlobals->curtime ) );
	hudlcd->SetGlobalStat( "(time_float)", VarArgs( "%.2f", gpGlobals->curtime ) );
}

//-----------------------------------------------------------------------------
// Purpose: Called to restore to "non"HUD state.
//-----------------------------------------------------------------------------
void CClientDll::HudReset( void )
{
	if(!g_bTextMode) {
		GetHud().VidInit();
	}

	PhysicsReset();
}

//-----------------------------------------------------------------------------
// Purpose: Called to add hud text message
//-----------------------------------------------------------------------------
void CClientDll::HudText( const char * message )
{
	DispatchHudText( message );
}

bool IsGameUIPanelVisible()
{
	if ( !g_pGameUIEx || !g_pGameUIEx->IsPanelVisible() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Handler for input events for the new game ui system
//-----------------------------------------------------------------------------
bool CClientDll::HandleGameUIEvent( const InputEvent_t &inputEvent )
{
	// TODO: when embedded UI will be used for HUD, we will need it to maintain
	// a separate screen for HUD and a separate screen stack for pause menu & main menu.
	// for now only render embedded UI in pause menu & main menu
	if ( !IsGameUIPanelVisible() )
		return false;

	return g_pGameUIGameSystem->RegisterInputEvent( inputEvent );
}

extern ConVar hud_freezecamhide;
extern bool IsTakingAFreezecamScreenshot();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CClientDll::ShouldDrawDropdownConsole()
{
	if(g_bTextMode)
		return false;

#if defined( REPLAY_ENABLED )
	if ( hud_freezecamhide.GetBool() && IsTakingAFreezecamScreenshot() )
	{
		return false;
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : ClientClass
//-----------------------------------------------------------------------------
ClientClass *CClientDll::GetAllClasses( void )
{
	return g_pClientClassHead;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientDll::IN_ActivateMouse( void )
{
	input->ActivateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientDll::IN_DeactivateMouse( void )
{
	input->DeactivateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientDll::IN_Accumulate ( void )
{
	input->AccumulateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientDll::IN_ClearStates ( void )
{
	input->ClearStates();
}

//-----------------------------------------------------------------------------
// Purpose: Engine can query for particular keys
// Input  : *name - 
//-----------------------------------------------------------------------------
bool CClientDll::IN_IsKeyDown( const char *name, bool& isdown )
{
	kbutton_t *key = input->FindKey( name );
	if ( !key )
	{
		return false;
	}
	
	isdown = ( key->state & 1 ) ? true : false;

	// Found the key by name
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Engine can issue a key event
// Input  : eventcode - 
//			keynum - 
//			*pszCurrentBinding - 
void CClientDll::IN_OnMouseWheeled( int nDelta )
{
#if defined( REPLAY_ENABLED )
	CReplayPerformanceEditorPanel *pPerfEditor = ReplayUI_GetPerformanceEditor();
	if ( pPerfEditor )
	{
		pPerfEditor->OnInGameMouseWheelEvent( nDelta );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Engine can issue a key event
// Input  : eventcode - 
//			keynum - 
//			*pszCurrentBinding - 
// Output : int
//-----------------------------------------------------------------------------
int CClientDll::IN_KeyEvent( int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	return input->KeyEvent( eventcode, keynum, pszCurrentBinding );
}

void CClientDll::ExtraMouseSample( float frametime, bool active )
{
	Assert( C_BaseEntity::IsAbsRecomputationsEnabled() );
	Assert( C_BaseEntity::IsAbsQueriesValid() );

	C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false ); 

	MDLCACHE_CRITICAL_SECTION();
	input->ExtraMouseSample( frametime, active );
}

void CClientDll::IN_SetSampleTime( float frametime )
{
	input->Joystick_SetSampleTime( frametime );
	input->IN_SetSampleTime( frametime );
}
//-----------------------------------------------------------------------------
// Purpose: Fills in usercmd_s structure based on current view angles and key/controller inputs
// Input  : frametime - timestamp for last frame
//			*cmd - the command to fill in
//			active - whether the user is fully connected to a server
//-----------------------------------------------------------------------------
void CClientDll::CreateMove ( int sequence_number, float input_sample_frametime, bool active )
{
	Assert( C_BaseEntity::IsAbsRecomputationsEnabled() );
	Assert( C_BaseEntity::IsAbsQueriesValid() );

	C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false ); 

	MDLCACHE_CRITICAL_SECTION();
	input->CreateMove( sequence_number, input_sample_frametime, active );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			from - 
//			to - 
//-----------------------------------------------------------------------------
bool CClientDll::WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand )
{
	return input->WriteUsercmdDeltaToBuffer( buf, from, to, isnewcommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CClientDll::EncodeUserCmdToBuffer( bf_write& buf, int slot )
{
	input->EncodeUserCmdToBuffer( buf, slot );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CClientDll::DecodeUserCmdFromBuffer( bf_read& buf, int slot )
{
	input->DecodeUserCmdFromBuffer( buf, slot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientDll::View_Render( vrect_t *rect )
{
	VPROF( "View_Render" );

	if(g_bTextMode)
		return;

	// UNDONE: This gets hit at startup sometimes, investigate - will cause NaNs in calcs inside Render()
	if ( rect->width == 0 || rect->height == 0 )
		return;

	GetViewRenderInstance()->Render( rect );
	UpdatePerfStats();
}


//-----------------------------------------------------------------------------
// Gets the location of the player viewpoint
//-----------------------------------------------------------------------------
bool CClientDll::GetPlayerView( CViewSetup &playerView )
{
	playerView = *GetViewRenderInstance()->GetPlayerViewSetup();
	return true;
}

//-----------------------------------------------------------------------------
// Matchmaking
//-----------------------------------------------------------------------------
void CClientDll::SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties )
{
	presence->SetupGameProperties( contexts, properties );
}

uint CClientDll::GetPresenceID( const char *pIDName )
{
	return presence->GetPresenceID( pIDName );
}

const char *CClientDll::GetPropertyIdString( const uint id )
{
	return presence->GetPropertyIdString( id );
}

void CClientDll::GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes )
{
	presence->GetPropertyDisplayString( id, value, pOutput, nBytes );
}

void CClientDll::StartStatsReporting( HANDLE handle, bool bArbitrated )
{
	presence->StartStatsReporting( handle, bArbitrated );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CClientDll::InvalidateMdlCache()
{
	C_BaseAnimating *pAnimating;
	for ( C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity) )
	{
		pAnimating = pEntity->GetBaseAnimating();
		if ( pAnimating )
		{
			pAnimating->InvalidateMdlCache();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSF - 
//-----------------------------------------------------------------------------
void CClientDll::View_Fade( ScreenFade_t *pSF )
{
	if ( pSF != NULL )
		GetViewEffects()->Fade( *pSF );
}

// CPU level
//-----------------------------------------------------------------------------
void ConfigureCurrentSystemLevel( );
void OnCPULevelChanged( IConVarRef var, const char *pOldValue, float flOldValue )
{
	ConfigureCurrentSystemLevel();
}

static ConVar cpu_level( "cpu_level", "2", 0, "CPU Level - Default: High", OnCPULevelChanged );
CPULevel_t GetCPULevel()
{
	return GetActualCPULevel();
}

CPULevel_t GetActualCPULevel()
{
	// Should we cache system_level off during level init?
	CPULevel_t nSystemLevel = (CPULevel_t)clamp( cpu_level.GetInt(), 0, CPU_LEVEL_PC_COUNT-1 );
	return nSystemLevel;
}

//-----------------------------------------------------------------------------
// GPU level
//-----------------------------------------------------------------------------
void OnGPULevelChanged( IConVarRef var, const char *pOldValue, float flOldValue )
{
	ConfigureCurrentSystemLevel();
}

static ConVar gpu_level( "gpu_level", "3", 0, "GPU Level - Default: High", OnGPULevelChanged );
GPULevel_t GetGPULevel()
{
	// Should we cache system_level off during level init?
	GPULevel_t nSystemLevel = (GPULevel_t)clamp( gpu_level.GetInt(), 0, GPU_LEVEL_PC_COUNT-1 );
	return nSystemLevel;
}


//-----------------------------------------------------------------------------
// System Memory level
//-----------------------------------------------------------------------------
void OnMemLevelChanged( IConVarRef var, const char *pOldValue, float flOldValue )
{
	ConfigureCurrentSystemLevel();
}

static ConVar mem_level( "mem_level", "2", 0, "Memory Level - Default: High", OnMemLevelChanged );
MemLevel_t GetMemLevel()
{
	// Should we cache system_level off during level init?
	MemLevel_t nSystemLevel = (MemLevel_t)clamp( mem_level.GetInt(), 0, MEM_LEVEL_PC_COUNT-1 );
	return nSystemLevel;
}

//-----------------------------------------------------------------------------
// GPU Memory level
//-----------------------------------------------------------------------------
void OnGPUMemLevelChanged( IConVarRef var, const char *pOldValue, float flOldValue )
{
	ConfigureCurrentSystemLevel();
}

static ConVar gpu_mem_level( "gpu_mem_level", "2", 0, "Memory Level - Default: High", OnGPUMemLevelChanged );
GPUMemLevel_t GetGPUMemLevel()
{
	// Should we cache system_level off during level init?
	GPUMemLevel_t nSystemLevel = (GPUMemLevel_t)clamp( gpu_mem_level.GetInt(), 0, GPU_MEM_LEVEL_PC_COUNT-1 );
	return nSystemLevel;
}

extern ConVarBase *mat_dxlevel;

void ConfigureCurrentSystemLevel()
{
	// Check if the user supports at least pixel shader 2.0b
	int nMaxDXLevel = g_pMaterialSystemHardwareConfig ? g_pMaterialSystemHardwareConfig->GetMaxDXSupportLevel() : 90;
	if( mat_dxlevel && mat_dxlevel->GetInt() < 95 )
	{
		Warning( "Your graphics card does not seem to support shader model 3.0 or higher. Reported dx level: %d (max setting: %d).\n", 
				mat_dxlevel->GetInt(), nMaxDXLevel );
	}

	C_BaseEntity::UpdateVisibilityAllEntities();
	if ( GetViewRenderInstance() )
	{
		GetViewRenderInstance()->InitFadeData();
	}
}

extern C_World *g_pClientWorld;

extern void W_Precache(void);

static bool g_bLevelParsed = false;

void OnWorldSpawned()
{
	if(g_bLevelParsed)
		return;

	char const *pMapEntities = engine->GetMapEntitiesString();

	// Clear out entity references, and parse the entities into it.
	g_MapEntityRefs.Purge();
	CMapLoadEntityFilter filter;
	MapEntity_ParseAllEntities( pMapEntities, &filter );

	// Now call the mod specific parse
	g_Client.LevelInit_ParseAllEntities( pMapEntities );

	// Sometimes an ent will Remove() itself during its precache, so RemoveImmediate won't happen.
	// This makes sure those ents get cleaned up.
	cl_entitylist->CleanupDeleteList();

	// Now that all of the active entities have been loaded in, precache any entities who need point_template parameters
	//  to be parsed (the above code has loaded all point_template entities)
	PrecachePointTemplates();

	g_bLevelParsed = true;
}

extern modelindex_t g_sModelIndexWorld;

modelindex_t g_sModelIndexEmpty = INVALID_MODEL_INDEX;
modelindex_t g_sModelIndexError = INVALID_MODEL_INDEX;

//-----------------------------------------------------------------------------
// Purpose: Per level init
//-----------------------------------------------------------------------------
void CClientDll::LevelInitPreEntity( char const* pMapName )
{
	// HACK: Bogus, but the logic is too complicated in the engine
	if (g_bLevelInitialized)
		return;
	g_bLevelInitialized = true;

	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;

	input->LevelInit();

	if(!g_bTextMode) {
		GetViewEffects()->LevelInit();
	}

	ClientVoiceMgr_LevelInit();

	//Tony; loadup per-map manifests.
	ParseParticleEffectsMap( pMapName, true );

	// Tell mode manager that map is changing
	modemanager->LevelInit( pMapName );
	ParticleMgr()->LevelInit();

	hudlcd->SetGlobalStat( "(mapname)", pMapName );

	if(!g_bTextMode) {
		clienteffects->Flush();
		GetViewRenderInstance()->LevelInit();
		ResetToneMapping(1.0);
	}

	tempents->LevelInit();

	// Always force reset to normal mode upon receipt of world in new map
	modemanager->SwitchMode( CLIENTMODE_NORMAL, true );

	g_sModelIndexEmpty = modelinfo->GetModelIndex( "models/empty.mdl" );
	g_sModelIndexWorld = modelinfo->GetModelIndex( engine->GetLevelName() );
	g_sModelIndexError = modelinfo->GetModelIndex( "models/error.mdl" );

	// don't set direct because of FCVAR_USERINFO
	if ( !cl_predict->GetInt() )
	{
		engine->ClientCmd( "cl_predict 1" );
	}

	IGameSystem::LevelInitPreEntityAllSystems();

	// Get weapon precaches
	W_Precache();

	// Call all registered precachers.
	CPrecacheRegister::Precache();

	RecastMgr().Load();

	if(!g_bTextMode) {
		if(g_pGamepadUI) {
			g_pGamepadUI->OnLevelInitializePreEntity();
		}

		GetHud().LevelInit();
	}

#if defined( REPLAY_ENABLED )
	// Initialize replay ragdoll recorder
	if ( !engine->IsPlayingDemo() )
	{
		CReplayRagdollRecorder::Instance().Init();
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Per level init
//-----------------------------------------------------------------------------
void CClientDll::LevelInitPostEntity( )
{
	IGameSystem::LevelInitPostEntityAllSystems();

	ResetWindspeed();

	C_PhysPropClientside::RecreateAll();

	if(!g_bTextMode) {
		if(g_pGamepadUI) {
			g_pGamepadUI->OnLevelInitializePostEntity();
		}

		GetCenterPrint()->Clear();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset our global string table pointers
//-----------------------------------------------------------------------------
void CClientDll::ResetStringTablePointers()
{
	g_pStringTableParticleEffectNames = NULL;
	g_StringTableEffectDispatch = NULL;
	g_StringTableVguiScreen = NULL;
	g_pStringTableMaterials = NULL;
	g_pStringTableInfoPanel = NULL;
	g_pStringTableClientSideChoreoScenes = NULL;
	g_pStringTableServerMapCycle = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Per level de-init
//-----------------------------------------------------------------------------
void CClientDll::LevelShutdown( void )
{
	// HACK: Bogus, but the logic is too complicated in the engine
	if (!g_bLevelInitialized)
	{
		ResetStringTablePointers();
		return;
	}

	g_bLevelInitialized = false;
	g_bLevelParsed = false;

	// Disable abs recomputations when everything is shutting down
	C_BaseEntity::EnableAbsRecomputations( false );

	// Level shutdown sequence.
	// First do the pre-entity shutdown of all systems
	IGameSystem::LevelShutdownPreEntityAllSystems();

	C_PhysPropClientside::DestroyAll();

	modemanager->LevelShutdown();

	// Remove temporary entities before removing entities from the client entity list so that the te_* may
	// clean up before hand.
	tempents->LevelShutdown();

	// Now release/delete the entities
	cl_entitylist->Release();

	C_BaseEntityClassList *pClassList = s_pClassLists;
	while ( pClassList )
	{
		pClassList->LevelShutdown();
		pClassList = pClassList->m_pNextClassList;
	}

	InvalidateQueryCache();

	// Now do the post-entity shutdown of all systems
	IGameSystem::LevelShutdownPostEntityAllSystems();

	g_sModelIndexWorld = INVALID_MODEL_INDEX;

	RecastMgr().Reset();

	GetViewRenderInstance()->LevelShutdown();
	beams->ClearBeams();
	ParticleMgr()->RemoveAllEffects();
	
	StopAllRumbleEffects();

	if(g_pGamepadUI) {
		g_pGamepadUI->OnLevelShutdown();
	}

	GetHud().LevelShutdown();

	GetCenterPrint()->Clear();

	ClientVoiceMgr_LevelShutdown();

	messagechars->Clear();

	// don't want to do this for TF2 because we have particle systems in our
	// character loadout screen that can be viewed when we're not connected to a server
	g_pParticleSystemMgr->UncacheAllParticleSystems();

	UncacheAllMaterials();

	// string tables are cleared on disconnect from a server, so reset our global pointers to NULL
	ResetStringTablePointers();

#if defined( REPLAY_ENABLED )
	// Shutdown the ragdoll recorder
	CReplayRagdollRecorder::Instance().Shutdown();
	CReplayRagdollCache::Instance().Shutdown();
#endif

	// Should never be paused at this point
	HackMgr_SetGamePaused( false );
	m_bWasPaused = false;
}


//-----------------------------------------------------------------------------
// Purpose: Engine received crosshair offset ( autoaim )
// Input  : angle - 
//-----------------------------------------------------------------------------
void CClientDll::SetCrosshairAngle( const QAngle& angle )
{
	CHudCrosshair *crosshair = GET_HUDELEMENT( CHudCrosshair );
	if ( crosshair )
	{
		crosshair->SetCrosshairAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper to initialize sprite from .spr semaphor
// Input  : *pSprite - 
//			*loadname - 
//-----------------------------------------------------------------------------
void CClientDll::InitSprite( CEngineSprite *pSprite, const char *loadname )
{
	if ( pSprite )
	{
		pSprite->Init( loadname );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSprite - 
//-----------------------------------------------------------------------------
void CClientDll::ShutdownSprite( CEngineSprite *pSprite )
{
	if ( pSprite )
	{
		pSprite->Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells engine how much space to allocate for sprite objects
// Output : int
//-----------------------------------------------------------------------------
int CClientDll::GetSpriteSize( void ) const
{
	return sizeof( CEngineSprite );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : entindex - 
//			bTalking - 
//-----------------------------------------------------------------------------
void CClientDll::VoiceStatus( int entindex, qboolean bTalking )
{
	GetClientVoiceMgr()->UpdateSpeakerStatus( entindex, !!bTalking );
}


//-----------------------------------------------------------------------------
// Called when the string table for materials changes
//-----------------------------------------------------------------------------
void OnMaterialStringTableChanged( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
	// Make sure this puppy is precached
	g_Client.PrecacheMaterial( newString );
	RequestCacheUsedMaterials();
}

//-----------------------------------------------------------------------------
// Called when the string table for dispatch effects changes
//-----------------------------------------------------------------------------
void OnEffectStringTableChanged( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
	// Make sure this puppy is precached
	PrecacheEffect( newString );
	RequestCacheUsedMaterials();
}

//-----------------------------------------------------------------------------
// Called when the string table for particle systems changes
//-----------------------------------------------------------------------------
void OnParticleSystemStringTableChanged( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
	// Make sure this puppy is precached
	g_pParticleSystemMgr->PrecacheParticleSystem( newString );
	RequestCacheUsedMaterials();
}

//-----------------------------------------------------------------------------
// Called when the string table for particle files changes
//-----------------------------------------------------------------------------
void OnPrecacheParticleFile( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
	g_pParticleSystemMgr->ShouldLoadSheets( true );
	g_pParticleSystemMgr->ReadParticleConfigFile( newString, true, false );
	g_pParticleSystemMgr->DecommitTempMemory();
}

//-----------------------------------------------------------------------------
// Called when the string table for VGUI changes
//-----------------------------------------------------------------------------
void OnVguiScreenTableChanged( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
	// Make sure this puppy is precached
	vgui::Panel *pPanel = PanelMetaClassMgr()->CreatePanelMetaClass( newString, 100, NULL, NULL );
	if ( pPanel )
		PanelMetaClassMgr()->DestroyPanelMetaClass( pPanel );
}

//-----------------------------------------------------------------------------
// Purpose: Preload the string on the client (if single player it should already be in the cache from the server!!!)
// Input  : *object - 
//			*stringTable - 
//			stringNumber - 
//			*newString - 
//			*newData - 
//-----------------------------------------------------------------------------
void OnSceneStringTableChanged( void *object, INetworkStringTable *stringTable, int stringNumber, const char *newString, void const *newData )
{
}

//-----------------------------------------------------------------------------
// Purpose: Hook up any callbacks here, the table definition has been parsed but 
//  no data has been added yet
//-----------------------------------------------------------------------------
void CClientDll::InstallStringTableCallback( const char *tableName )
{
	// Here, cache off string table IDs
	if (!Q_strcasecmp(tableName, "VguiScreen"))
	{
		// Look up the id 
		g_StringTableVguiScreen = networkstringtable->FindTable( tableName );

		// When the material list changes, we need to know immediately
		g_StringTableVguiScreen->SetStringChangedCallback( NULL, OnVguiScreenTableChanged );
	}
	else if (!Q_strcasecmp(tableName, "Materials"))
	{
		// Look up the id 
		g_pStringTableMaterials = networkstringtable->FindTable( tableName );

		// When the material list changes, we need to know immediately
		g_pStringTableMaterials->SetStringChangedCallback( NULL, OnMaterialStringTableChanged );
	}
	else if ( !Q_strcasecmp( tableName, "EffectDispatch" ) )
	{
		g_StringTableEffectDispatch = networkstringtable->FindTable( tableName );

		// When the material list changes, we need to know immediately
		g_StringTableEffectDispatch->SetStringChangedCallback( NULL, OnEffectStringTableChanged );
	}
	else if ( !Q_strcasecmp( tableName, "InfoPanel" ) )
	{
		g_pStringTableInfoPanel = networkstringtable->FindTable( tableName );
	}
	else if ( !Q_strcasecmp( tableName, "Scenes" ) )
	{
		g_pStringTableClientSideChoreoScenes = networkstringtable->FindTable( tableName );
		networkstringtable->SetAllowClientSideAddString( g_pStringTableClientSideChoreoScenes, true );
		g_pStringTableClientSideChoreoScenes->SetStringChangedCallback( NULL, OnSceneStringTableChanged );
	}
	else if ( !Q_strcasecmp( tableName, "ParticleEffectNames" ) )
	{
		g_pStringTableParticleEffectNames = networkstringtable->FindTable( tableName );
		networkstringtable->SetAllowClientSideAddString( g_pStringTableParticleEffectNames, true );
		// When the particle system list changes, we need to know immediately
		g_pStringTableParticleEffectNames->SetStringChangedCallback( NULL, OnParticleSystemStringTableChanged );
	}
	else if ( !Q_strcasecmp( tableName, "ExtraParticleFilesTable" ) )
	{
		g_pStringTableExtraParticleFiles = networkstringtable->FindTable( tableName );
		networkstringtable->SetAllowClientSideAddString( g_pStringTableExtraParticleFiles, true );
		// When the particle system list changes, we need to know immediately
		g_pStringTableExtraParticleFiles->SetStringChangedCallback( NULL, OnPrecacheParticleFile );
	}
	else if ( !Q_strcasecmp( tableName, "ServerMapCycle" ) )
	{
		g_pStringTableServerMapCycle = networkstringtable->FindTable( tableName );
	}

	InstallStringTableCallback_GameRules();
}


//-----------------------------------------------------------------------------
// Material precache
//-----------------------------------------------------------------------------
void CClientDll::PrecacheMaterial( const char *pMaterialName )
{
	Assert( pMaterialName );

	int nLen = Q_strlen( pMaterialName );
	char *pTempBuf = (char*)stackalloc( nLen + 1 );
	memcpy( pTempBuf, pMaterialName, nLen + 1 );
	char *pFound = Q_strstr( pTempBuf, ".vmt\0" );
	if ( pFound )
	{
		*pFound = 0;
	}
		
	IMaterial *pMaterial = g_pMaterialSystem->FindMaterial( pTempBuf, TEXTURE_GROUP_PRECACHED );
	if ( !IsErrorMaterial( pMaterial ) )
	{
		int idx = m_CachedMaterials.Find( pMaterial );
		if ( idx == m_CachedMaterials.InvalidIndex() )
		{
			pMaterial->IncrementReferenceCount();
			m_CachedMaterials.Insert( pMaterial );
		}
	}
#ifdef _OSX
	else
	{
		printf("\n ##### CClientDll::PrecacheMaterial could not find material %s (%s)", pMaterialName, pTempBuf );
	}
#endif
}

void CClientDll::UncacheAllMaterials( )
{
	for ( int i = m_CachedMaterials.FirstInorder(); i != m_CachedMaterials.InvalidIndex(); i = m_CachedMaterials.NextInorder( i ) )
	{
		m_CachedMaterials[i]->DecrementReferenceCount();
	}
	m_CachedMaterials.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CClientDll::DispatchUserMessage( int msg_type, bf_read &msg_data )
{
	return usermessages->DispatchUserMessage( msg_type, msg_data );
}


void SimulateEntities()
{
	VPROF_BUDGET("Client SimulateEntities", VPROF_BUDGETGROUP_CLIENT_SIM);

	// Service timer events (think functions).
  	ClientThinkList()->PerformThinkFunctions();

	C_BaseEntity::SimulateEntities();

	// UNDONE: Make these systems IGameSystems and move these calls into FrameUpdatePostEntityThink()
	// service event queue, firing off any actions whos time has come
	ServiceEventQueue();
}


bool AddDataChangeEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent )
{
	VPROF( "AddDataChangeEvent" );

	Assert( ent );
	// Make sure we don't already have an event queued for this guy.
	if ( *pStoredEvent >= 0 )
	{
		Assert( g_DataChangedEvents[*pStoredEvent].m_pEntity == ent );

		// DATA_UPDATE_CREATED always overrides DATA_UPDATE_CHANGED.
		if ( updateType == DATA_UPDATE_CREATED )
			g_DataChangedEvents[*pStoredEvent].m_UpdateType = updateType;
	
		return false;
	}
	else
	{
		*pStoredEvent = g_DataChangedEvents.AddToTail( CDataChangedEvent( ent, updateType, pStoredEvent ) );
		return true;
	}
}


void ClearDataChangedEvent( int iStoredEvent )
{
	if ( iStoredEvent != -1 )
		g_DataChangedEvents.Remove( iStoredEvent );
}


void ProcessOnDataChangedEvents()
{
	VPROF_("ProcessOnDataChangedEvents", 1, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);
	FOR_EACH_LL( g_DataChangedEvents, i )
	{
		CDataChangedEvent *pEvent = &g_DataChangedEvents[i];

		// Reset their stored event identifier.		
		*pEvent->m_pStoredEvent = -1;

		// Send the event.
		IClientNetworkable *pNetworkable = pEvent->m_pEntity;
		pNetworkable->OnDataChanged( pEvent->m_UpdateType );
	}

	g_DataChangedEvents.Purge();
}


void UpdateClientRenderableInPVSStatus()
{
	// Vis for this view should already be setup at this point.

	// For each client-only entity, notify it if it's newly coming into the PVS.
	CUtlLinkedList<CClientEntityList::CPVSNotifyInfo,unsigned short> &theList = ClientEntityList().GetPVSNotifiers();
	FOR_EACH_LL( theList, i )
	{
		CClientEntityList::CPVSNotifyInfo *pInfo = &theList[i];

		if ( pInfo->m_InPVSStatus & INPVS_YES )
		{
			// Ok, this entity already thinks it's in the PVS. No need to notify it.
			// We need to set the INPVS_YES_THISFRAME flag if it's in this frame at all, so we 
			// don't tell the entity it's not in the PVS anymore at the end of the frame.
			if ( !( pInfo->m_InPVSStatus & INPVS_THISFRAME ) )
			{
				if ( ClientLeafSystem()->IsRenderableInPVS( pInfo->m_pRenderable ) )
				{
					pInfo->m_InPVSStatus |= INPVS_THISFRAME;
				}
			}
		}
		else
		{
			// This entity doesn't think it's in the PVS yet. If it is now in the PVS, let it know.
			if ( ClientLeafSystem()->IsRenderableInPVS( pInfo->m_pRenderable ) )
			{
				pInfo->m_InPVSStatus |= ( INPVS_YES | INPVS_THISFRAME | INPVS_NEEDSNOTIFY );
			}
		}
	}	
}

void UpdatePVSNotifiers()
{
	MDLCACHE_CRITICAL_SECTION();

	// At this point, all the entities that were rendered in the previous frame have INPVS_THISFRAME set
	// so we can tell the entities that aren't in the PVS anymore so.
	CUtlLinkedList<CClientEntityList::CPVSNotifyInfo,unsigned short> &theList = ClientEntityList().GetPVSNotifiers();
	FOR_EACH_LL( theList, i )
	{
		CClientEntityList::CPVSNotifyInfo *pInfo = &theList[i];

		// If this entity thinks it's in the PVS, but it wasn't in the PVS this frame, tell it so.
		if ( pInfo->m_InPVSStatus & INPVS_YES )
		{
			if ( pInfo->m_InPVSStatus & INPVS_THISFRAME )
			{
				if ( pInfo->m_InPVSStatus & INPVS_NEEDSNOTIFY )
				{
					pInfo->m_pNotify->OnPVSStatusChanged( true );
				}
				// Clear it for the next time around.
				pInfo->m_InPVSStatus &= ~( INPVS_THISFRAME | INPVS_NEEDSNOTIFY );
			}
			else
			{
				pInfo->m_InPVSStatus &= ~INPVS_YES;
				pInfo->m_pNotify->OnPVSStatusChanged( false );
			}
		}
	}
}


void OnRenderStart()
{
	VPROF( "OnRenderStart" );
	MDLCACHE_CRITICAL_SECTION();
	MDLCACHE_COARSE_LOCK();

#ifdef PORTAL
	g_pPortalRender->UpdatePortalPixelVisibility(); //updating this one or two lines before querying again just isn't cutting it. Update as soon as it's cheap to do so.
#endif

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	C_BaseEntity::SetAbsQueriesValid( false );

	Rope_ResetCounters();

	// Interpolate server entities and move aiments.
	{
		PREDICTION_TRACKVALUECHANGESCOPE( "interpolation" );
		C_BaseEntity::InterpolateServerEntities();
	}

	{
		// vprof node for this bloc of math
		VPROF( "OnRenderStart: dirty bone caches");
		// Invalidate any bone information.
		C_BaseAnimating::InvalidateBoneCaches();
		C_BaseFlex::InvalidateFlexCaches();

		C_BaseEntity::SetAbsQueriesValid( true );
		C_BaseEntity::EnableAbsRecomputations( true );

		// Enable access to all model bones except view models.
		// This is necessary for aim-ent computation to occur properly
		C_BaseAnimating::PushAllowBoneAccess( true, false, "OnRenderStart->CViewRender::SetUpView" ); // pops in CViewRender::SetUpView

		// FIXME: This needs to be done before the player moves; it forces
		// aiments the player may be attached to to forcibly update their position
		C_BaseEntity::MarkAimEntsDirty();
	}

	// Make sure the camera simulation happens before OnRenderStart, where it's used.
	// NOTE: the only thing that happens in CAM_Think is thirdperson related code.
	input->CAM_Think();

	C_BaseAnimating::PopBoneAccess( "OnRenderStart->CViewRender::SetUpView" ); // pops the (true, false) bone access set in OnRenderStart

	// Enable access to all model bones until rendering is done
	C_BaseAnimating::PushAllowBoneAccess( true, true, "CViewRender::SetUpView->OnRenderEnd" ); // pop is in OnRenderEnd()

		// This will place all entities in the correct position in world space and in the KD-tree
	C_BaseAnimating::UpdateClientSideAnimations();

	// This will place the player + the view models + all parent
	// entities	at the correct abs position so that their attachment points
	// are at the correct location
	GetViewRenderInstance()->OnRenderStart();

	RopeManager()->OnRenderStart();

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	// Process OnDataChanged events.
	ProcessOnDataChangedEvents();

	// Reset the overlay alpha. Entities can change the state of this in their think functions.
	g_SmokeFogOverlayAlpha = 0;	

	// This must occur prior to SimulatEntities,
	// which is where the client thinks for c_colorcorrection + c_colorcorrectionvolumes
	// update the color correction weights.
	// FIXME: The place where IGameSystem::Update is called should be in here
	// so we don't have to explicitly call ResetColorCorrectionWeights + SimulateEntities, etc.
	g_pColorCorrectionMgr->ResetColorCorrectionWeights();

	C_BaseAnimating::ThreadedBoneSetup();

	// Simulate all the entities.
	SimulateEntities();
	PhysicsSimulate();

	// Tony; in multiplayer do some extra stuff. like re-calc the view if in a vehicle!
	GetViewRenderInstance()->PostSimulate();

	{
		VPROF_("Client TempEnts", 0, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);
		// This creates things like temp entities.
		engine->FireEvents();

		// Update temp entities
		tempents->Update();

		// Update temp ent beams...
		beams->UpdateTempEntBeams();
		
		// Lock the frame from beam additions
		SetBeamCreationAllowed( false );
	}

	// Update particle effects (eventually, the effects should use Simulate() instead of having
	// their own update system).
	{
		// Enable FP exceptions here when FP_EXCEPTIONS_ENABLED is defined,
		// to help track down bad math.
		FPExceptionEnabler enableExceptions;
		VPROF_BUDGET( "ParticleMgr()->Simulate", VPROF_BUDGETGROUP_PARTICLE_SIMULATION );
		ParticleMgr()->Simulate( gpGlobals->frametime );
	}

	// Now that the view model's position is setup and aiments are marked dirty, update
	// their positions so they're in the leaf system correctly.
	C_BaseEntity::CalcAimEntPositions();

	// For entities marked for recording, post bone messages to IToolSystems
	if ( ToolsEnabled() )
	{
		C_BaseEntity::ToolRecordEntities();
	}

#if defined( REPLAY_ENABLED )
	// This will record any ragdolls if Replay mode is enabled on the server
	CReplayRagdollRecorder::Instance().Think();
	CReplayRagdollCache::Instance().Think();
#endif

	// Finally, link all the entities into the leaf system right before rendering.
	C_BaseEntity::AddVisibleEntities();

	ClientLeafSystem()->RecomputeRenderableLeaves();
	g_pClientShadowMgr->ReprojectShadows();
	g_pClientShadowMgr->AdvanceFrame();
	ClientLeafSystem()->DisableLeafReinsertion( true );
}


void OnRenderEnd()
{
	ClientLeafSystem()->DisableLeafReinsertion( false );

	// Disallow access to bones (access is enabled in CViewRender::SetUpView).
	C_BaseAnimating::PopBoneAccess( "CViewRender::SetUpView->OnRenderEnd" );

	UpdatePVSNotifiers();

	DisplayBoneSetupEnts();
}



void CClientDll::FrameStageNotify( ClientFrameStage_t curStage )
{
	g_CurFrameStage = curStage;

	switch( curStage )
	{
	default:
		break;

	case FRAME_RENDER_START:
		{
			VPROF( "CClientDll::FrameStageNotify FRAME_RENDER_START" );

			// Last thing before rendering, run simulation.
			OnRenderStart();
		}
		break;
		
	case FRAME_RENDER_END:
		{
			VPROF( "CClientDll::FrameStageNotify FRAME_RENDER_END" );
			OnRenderEnd();

			PREDICTION_SPEWVALUECHANGES();
		}
		break;
		
	case FRAME_NET_UPDATE_START:
		{
			VPROF( "CClientDll::FrameStageNotify FRAME_NET_UPDATE_START" );
			// disabled all recomputations while we update entities
			C_BaseEntity::EnableAbsRecomputations( false );
			C_BaseEntity::SetAbsQueriesValid( false );
			Interpolation_SetLastPacketTimeStamp( engine->GetLastTimeStamp() );
			partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

			PREDICTION_STARTTRACKVALUE( "netupdate" );
		}
		break;
	case FRAME_NET_UPDATE_END:
		{
			ProcessCacheUsedMaterials();

			// reenable abs recomputation since now all entities have been updated
			C_BaseEntity::EnableAbsRecomputations( true );
			C_BaseEntity::SetAbsQueriesValid( true );
			partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

			PREDICTION_ENDTRACKVALUE();
		}
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		{
			VPROF( "CClientDll::FrameStageNotify FRAME_NET_UPDATE_POSTDATAUPDATE_START" );
			PREDICTION_STARTTRACKVALUE( "postdataupdate" );
		}
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		{
			VPROF( "CClientDll::FrameStageNotify FRAME_NET_UPDATE_POSTDATAUPDATE_END" );
			PREDICTION_ENDTRACKVALUE();
			// Let prediction copy off pristine data
			prediction->PostEntityPacketReceived();
			HLTVCamera()->PostEntityPacketReceived();
#if defined( REPLAY_ENABLED )
			ReplayCamera()->PostEntityPacketReceived();
#endif
		}
		break;
	case FRAME_START:
		{
			// Mark the frame as open for client fx additions
			SetFXCreationAllowed( true );
			SetBeamCreationAllowed( true );
			C_BaseEntity::CheckCLInterpChanged();
		}
		break;
	}
}

extern ConVarBase *closecaption;

// Given a list of "S(wavname) S(wavname2)" tokens, look up the localized text and emit
//  the appropriate close caption if running with closecaption = 1
void CClientDll::EmitSentenceCloseCaption( char const *tokenstream )
{
	if ( !closecaption->GetBool() )
		return;

	if ( m_pHudCloseCaption )
	{
		m_pHudCloseCaption->ProcessSentenceCaptionStream( tokenstream );
	}
}


void CClientDll::EmitCloseCaption( char const *captionname, float duration )
{
	if ( !closecaption->GetBool() )
		return;

	if ( m_pHudCloseCaption )
	{
		m_pHudCloseCaption->ProcessCaption( captionname, duration );
	}
}

CStandardRecvProxies* CClientDll::GetStandardRecvProxies()
{
	return &g_StandardRecvProxies;
}

bool CClientDll::CanRecordDemo( char *errorMsg, int length ) const
{
	if ( GetClientModeNormal() )
	{
		return GetClientModeNormal()->CanRecordDemo( errorMsg, length );
	}

	return true;
}

void CClientDll::OnDemoRecordStart( char const* pDemoBaseName )
{
}

void CClientDll::OnDemoRecordStop()
{
}

void CClientDll::OnDemoPlaybackStart( char const* pDemoBaseName )
{
#if defined( REPLAY_ENABLED )
	// Load any ragdoll override frames from disk
	char szRagdollFile[MAX_OSPATH];
	V_snprintf( szRagdollFile, sizeof(szRagdollFile), "%s.dmx", pDemoBaseName );
	CReplayRagdollCache::Instance().Init( szRagdollFile );
#endif
}

void CClientDll::OnDemoPlaybackStop()
{
#ifdef DEMOPOLISH_ENABLED
	if ( DemoPolish_GetController().m_bInit )
	{
		DemoPolish_GetController().Shutdown();
	}
#endif

#if defined( REPLAY_ENABLED )
	CReplayRagdollCache::Instance().Shutdown();
#endif
}

int CClientDll::GetScreenWidth()
{
	return ScreenWidth();
}

int CClientDll::GetScreenHeight()
{
	return ScreenHeight();
}

void CClientDll::RecordDemoPolishUserInput( int nCmdIndex )
{
#ifdef DEMOPOLISH_ENABLED
	Assert( engine->IsRecordingDemo() );
	Assert( IsDemoPolishEnabled() );	// NOTE: cl_demo_polish_enabled checked in engine.
	
	CUserCmd const* pUserCmd = input->GetUserCmd( nCmdIndex );
	Assert( pUserCmd );
	if ( pUserCmd )
	{
		DemoPolish_GetRecorder().RecordUserInput( pUserCmd );
	}
#endif
}

bool CClientDll::CacheReplayRagdolls( const char* pFilename, int nStartTick )
{
#if defined( REPLAY_ENABLED )
	return Replay_CacheRagdolls( pFilename, nStartTick );
#else
	return false;
#endif
}

// NEW INTERFACES
// See RenderViewInfo_t
void CClientDll::RenderView( const CViewSetup &setup, int nClearFlags, int whatToDraw )
{
	VPROF("RenderView");
	CViewSetupEx setupex = setup;
	GetViewRenderInstance()->RenderView( setupex, setupex, nClearFlags, whatToDraw );
}

bool CClientDll::ShouldHideLoadingPlaque( void )
{
	return false;

}

void ReloadSoundEntriesInList( IFileList *pFilesToReload );

//-----------------------------------------------------------------------------
// For sv_pure mode. The filesystem figures out which files the client needs to reload to be "pure" ala the server's preferences.
//-----------------------------------------------------------------------------
void CClientDll::ReloadFilesInList( IFileList *pFilesToReload )
{
	ReloadParticleEffectsInList( pFilesToReload );
	ReloadSoundEntriesInList( pFilesToReload );
}

void CClientDll::CenterStringOff()
{
	GetCenterPrint()->Clear();
}

void CClientDll::OnScreenSizeChanged( int nOldWidth, int nOldHeight )
{
	// Tell split screen system
	VGui_OnScreenSizeChanged();
}

IMaterialProxy *CClientDll::InstantiateMaterialProxy( const char *proxyName )
{
	IMaterialProxy *pProxy = g_pGameUIGameSystem->CreateProxy( proxyName );
	if ( pProxy )
		return pProxy;

	return GetMaterialProxyDict().CreateProxy( proxyName );
}

vgui::VPANEL CClientDll::GetFullscreenClientDLLVPanel( void )
{
	return VGui_GetFullscreenRootVPANEL();
}

bool CClientDll::HandleUiToggle()
{
#if defined( REPLAY_ENABLED )
	if ( !g_pEngineReplay || !g_pEngineReplay->IsSupportedModAndPlatform() )
		return false;

	CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
	if ( !pEditor )
		return false;

	pEditor->HandleUiToggle();

	return true;

#else
	return false;
#endif
}

bool CClientDll::ShouldAllowConsole()
{
	return true;
}

CRenamedRecvTableInfo *CClientDll::GetRenamedRecvTableInfos()
{
	return g_pRenamedRecvTableInfoHead;
}

CMouthInfo g_ClientUIMouth;
// Get the mouthinfo for the sound being played inside UI panels
CMouthInfo *CClientDll::GetClientUIMouthInfo()
{
	return &g_ClientUIMouth;
}

void CClientDll::FileReceived( const char * fileName, unsigned int transferID )
{
	if ( GameRules() )
	{
		GameRules()->OnFileReceived( fileName, transferID );
	}
}

void CClientDll::ClientAdjustStartSoundParams( StartSoundParams_t& params )
{
	if(GameRules())
		GameRules()->ClientAdjustStartSoundParams( params );
}

const char* CClientDll::TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName )
{
	if ( !GameRules() )
		return pchEffectName;

	return GameRules()->TranslateEffectForVisionFilter( pchEffectType, pchEffectName );
}

bool CClientDll::DisconnectAttempt( void )
{
	if ( GameRules() )
	{
		return GameRules()->HandleDisconnectAttempt();
	}

	return false;
}

bool CClientDll::IsConnectedUserInfoChangeAllowed( IConVarRef pCvar )
{
	return GameRules() ? GameRules()->IsConnectedUserInfoChangeAllowed( NULL ) : true;
}

bool GetSteamIDForPlayerIndex( int iPlayerIndex, CSteamID &steamid )
{
	player_info_t pi;
	if ( SteamUtils() )
	{
		if ( engine->GetPlayerInfo( iPlayerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				steamid = CSteamID( pi.friendsID, 1, SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Marks entities as touching
// Input  : *e1 - 
//			*e2 - 
//-----------------------------------------------------------------------------
void CClientDll::MarkEntitiesAsTouching( IClientEntity *e1, IClientEntity *e2 )
{
	C_BaseEntity *entity = e1->GetBaseEntity();
	C_BaseEntity *entityTouched = e2->GetBaseEntity();
	if ( entity && entityTouched )
	{
		trace_t tr;
		UTIL_ClearTrace( tr );
		tr.endpos = (entity->GetAbsOrigin() + entityTouched->GetAbsOrigin()) * 0.5;
		entity->PhysicsMarkEntitiesAsTouching( entityTouched, tr );
	}
}

class CKeyBindingListenerMgr : public IKeyBindingListenerMgr
{
public:
	struct BindingListeners_t
	{
		BindingListeners_t()
		{
		}

		BindingListeners_t( const BindingListeners_t &other )
		{
			m_List.CopyArray( other.m_List.Base(), other.m_List.Count() );
		}

		CUtlVector< IKeyBindingListener * > m_List;
	};

	// Callback when button is bound
	virtual void AddListenerForCode( IKeyBindingListener *pListener, ButtonCode_t buttonCode )
	{
		CUtlVector< IKeyBindingListener * > &list = m_CodeListeners[ buttonCode ];
		if ( list.Find( pListener ) != list.InvalidIndex() )
			return;
		list.AddToTail( pListener );
	}

	// Callback whenver binding is set to a button
	virtual void AddListenerForBinding( IKeyBindingListener *pListener, char const *pchBindingString )
	{
		int idx = m_BindingListeners.Find( pchBindingString );
		if ( idx == m_BindingListeners.InvalidIndex() )
		{
			idx = m_BindingListeners.Insert( pchBindingString );
		}

		CUtlVector< IKeyBindingListener * > &list = m_BindingListeners[ idx ].m_List;
		if ( list.Find( pListener ) != list.InvalidIndex() )
			return;
		list.AddToTail( pListener );
	}

	virtual void RemoveListener( IKeyBindingListener *pListener )
	{
		for ( int i = 0; i < ARRAYSIZE( m_CodeListeners ); ++i )
		{
			CUtlVector< IKeyBindingListener * > &list = m_CodeListeners[ i ];
			list.FindAndRemove( pListener );
		}

		for ( int i = m_BindingListeners.First(); i != m_BindingListeners.InvalidIndex(); i = m_BindingListeners.Next( i ) )
		{
			CUtlVector< IKeyBindingListener * > &list = m_BindingListeners[ i ].m_List;
			list.FindAndRemove( pListener );
		}
	}

	void OnKeyBindingChanged( ButtonCode_t buttonCode, char const *pchKeyName, char const *pchNewBinding )
	{
		CUtlVector< IKeyBindingListener * > &list = m_CodeListeners[ buttonCode ];
		for ( int i = 0 ; i < list.Count(); ++i )
		{
			list[ i ]->OnKeyBindingChanged( buttonCode, pchKeyName, pchNewBinding );
		}

		int idx = m_BindingListeners.Find( pchNewBinding );
		if ( idx != m_BindingListeners.InvalidIndex() )
		{
			CUtlVector< IKeyBindingListener * > &list = m_BindingListeners[ idx ].m_List;
			for ( int i = 0 ; i < list.Count(); ++i )
			{
				list[ i ]->OnKeyBindingChanged( buttonCode, pchKeyName, pchNewBinding );
			}
		}
	}

private:
	CUtlVector< IKeyBindingListener * > m_CodeListeners[ BUTTON_CODE_COUNT ];
	CUtlDict< BindingListeners_t, int > m_BindingListeners;
};

static CKeyBindingListenerMgr g_KeyBindingListenerMgr;

IKeyBindingListenerMgr *g_pKeyBindingListenerMgr = &g_KeyBindingListenerMgr;
void CClientDll::OnKeyBindingChanged( ButtonCode_t buttonCode, char const *pchKeyName, char const *pchNewBinding )
{
	g_KeyBindingListenerMgr.OnKeyBindingChanged( buttonCode, pchKeyName, pchNewBinding );
}

void CClientDll::SetBlurFade( float scale )
{
	GetClientMode()->SetBlurFade( scale );
}

void CClientDll::ResetHudCloseCaption()
{
	
}

bool CClientDll::SupportsRandomMaps()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: interface from materialsystem to client, currently just for recording into tools
//-----------------------------------------------------------------------------
class CClientMaterialSystem : public IClientMaterialSystem
{
	virtual HTOOLHANDLE GetCurrentRecordingEntity()
	{
		if ( !ToolsEnabled() )
			return HTOOLHANDLE_INVALID;

		if ( !clienttools->IsInRecordingMode() )
			return HTOOLHANDLE_INVALID;

		C_BaseEntity *pEnt = GetViewRenderInstance()->GetCurrentlyDrawingEntity();
		if ( !pEnt || !pEnt->IsToolRecording() )
			return HTOOLHANDLE_INVALID;

		return pEnt->GetToolHandle();
	}
	virtual void PostToolMessage( HTOOLHANDLE hEntity, KeyValues *pMsg )
	{
		ToolFramework_PostToolMessage( hEntity, pMsg );
	}
};

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CClientMaterialSystem s_ClientMaterialSystem;
IClientMaterialSystem *g_pClientMaterialSystem = &s_ClientMaterialSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientMaterialSystem, IClientMaterialSystem, VCLIENTMATERIALSYSTEM_INTERFACE_VERSION, s_ClientMaterialSystem );

class CGameClientLoopback : public CBaseAppSystem< IGameClientLoopback >
{
public:
	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	// If pBoxColors is specified (it's an array of 6), then it'll copy the light contribution at each box side.
	virtual void		ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color, Vector *pBoxColors=NULL )
	{ engine->ComputeLighting( pt, pNormal, bClamp, color, pBoxColors ); }

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void		ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color )
	{ engine->ComputeDynamicLighting( pt, pNormal, color ); }

	// Get the lighting intensivty for a specified point
	// If bClamp is specified, the resulting Vector is restricted to the 0.0 to 1.0 for each element
	virtual Vector				GetLightForPoint(const Vector &pos, bool bClamp)
	{ return engine->GetLightForPoint( pos, bClamp ); }

	// Just get the leaf ambient light - no caching, no samples
	virtual Vector			GetLightForPointFast(const Vector &pos, bool bClamp)
	{ return engine->GetLightForPointFast( pos, bClamp ); }

	// Returns the color of the ambient light
	virtual void		GetAmbientLightColor( Vector& color )
	{ engine->GetAmbientLightColor( color); }

	virtual map_datamap_t *GetMapDatamaps()
	{ return g_pMapDatamapsHead; }
};

static CGameClientLoopback s_ClientGameLoopback;
IGameClientLoopback *g_pGameClientLoopback = &s_ClientGameLoopback;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameClientLoopback, IGameClientLoopback, GAMECLIENTLOOPBACK_INTERFACE_VERSION, s_ClientGameLoopback );
