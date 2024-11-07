//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef CDLL_CLIENT_INT_H
#define CDLL_CLIENT_INT_H
#pragma once

#include "iclientnetworkable.h"
#include "utllinkedlist.h"
#include "cdll_int.h"
#include "eiface.h"
#include "videocfg/videocfg.h"

class IVModelRender;
class IVEngineClient;
class IVModelRender;
class IVEfx;
class IVRenderView;
class IVDebugOverlay;
class IMaterialSystem;
class IMaterialSystemStub;
class IDataCache;
class IMDLCache;
class IVModelInfoClient;
class IEngineVGui;
class ISpatialPartition;
class IBaseClientDLL;
class ISpatialPartition;
class IFileSystem;
class IStaticPropMgrClient;
class IShadowMgr;
class IEngineSound;
class IMatSystemSurface;
class IMaterialSystemHardwareConfig;
class ISharedGameRules;
class IEngineTrace;
class IGameUIFuncs;
class IGameEventManager2;
class IPhysicsGameTrace;
class CGlobalVarsBase;
class IClientTools;
class C_BaseAnimating;
class IColorCorrectionSystem;
class IInputSystem;
class ISceneFileCache;
class IVideoServices;
class IClientReplayContext;
class IReplayManager;
class IEngineReplay;
class IEngineClientReplay;
class IReplayScreenshotManager;
class CSteamID;
class IShaderExtension;
class IGameLoopback;
class IGameServerLoopback;
class IGameClientLoopback;
class ISoundEmitterSystemBase;
class CHudCloseCaption;
class IServerEngineTools;

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

class AchievementsAndStatsInterface;

//=============================================================================
// HPE_END
//=============================================================================

extern IVModelRender *modelrender;
extern IVEngineClient	*engine;
extern IVModelRender *modelrender;
extern IVEfx *effects;
extern IVRenderView *render;
extern IVDebugOverlay *debugoverlay;
extern IMaterialSystemStub *materials_stub;
extern IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
extern IVModelInfoClient *modelinfo;
extern IEngineVGui *enginevgui;
extern ISpatialPartition* partition;
extern IBaseClientDLL *clientdll;
extern IStaticPropMgrClient *staticpropmgr;
extern IShadowMgr *shadowmgr;
extern IEngineSound *enginesound;
extern IEngineTrace *enginetrace;
extern IGameUIFuncs *gameuifuncs;
extern IGameEventManager2 *gameeventmanager;
extern IPhysicsGameTrace *physgametrace;
extern CGlobalVarsBase *gpGlobals;
extern IClientTools *clienttools;
extern ISceneFileCache *scenefilecache;
extern IUploadGameStats *gamestatsuploader;
extern IReplaySystem *g_pReplay;
extern IClientReplayContext *g_pClientReplayContext;
extern IReplayManager *g_pReplayManager;
extern IReplayScreenshotManager *g_pReplayScreenshotManager;
extern IEngineReplay *g_pEngineReplay;
extern IEngineClientReplay *g_pEngineClientReplay;
extern IShaderExtension* g_pShaderExtension;
extern IGameLoopback* g_pGameLoopback;
extern IGameServerLoopback* g_pGameServerLoopback;
extern IGameClientLoopback* g_pGameClientLoopback;
extern IServerEngineTools *serverenginetools;

//-----------------------------------------------------------------------------
// Purpose: engine to client .dll interface
//-----------------------------------------------------------------------------
class CClientDll : public IBaseClientDLL
{
public:
	CClientDll();

	virtual int						Connect( CreateInterfaceFn appSystemFactory, CGlobalVarsBase *pGlobals );
	virtual int						Init( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals );

	virtual void					PostInit();
	virtual void					Shutdown( void );

	virtual bool					ReplayInit( CreateInterfaceFn fnReplayFactory );
	virtual bool					ReplayPostInit();

	virtual void					LevelInitPreEntity( const char *pMapName );
	virtual void					LevelInitPostEntity();
	virtual void					LevelShutdown( void );

	virtual ClientClass				*GetAllClasses( void );

	virtual int						HudVidInit( void );
	virtual void					HudProcessInput( bool bActive );
	virtual void					HudUpdate( bool bActive );
	virtual void					HudReset( void );
	virtual void					HudText( const char * message );

	// Mouse Input Interfaces
	virtual void					IN_ActivateMouse( void );
	virtual void					IN_DeactivateMouse( void );
	virtual void					IN_Accumulate( void );
	virtual void					IN_ClearStates( void );
	virtual bool					IN_IsKeyDown( const char *name, bool& isdown );
	virtual void					IN_OnMouseWheeled( int nDelta );
	// Raw signal
	virtual int						IN_KeyEvent( int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual void					IN_SetSampleTime( float frametime );
	// Create movement command
	virtual void					CreateMove ( int sequence_number, float input_sample_frametime, bool active );
	virtual void					ExtraMouseSample( float frametime, bool active );
	virtual bool					WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand );	
	virtual void					EncodeUserCmdToBuffer( bf_write& buf, int slot );
	virtual void					DecodeUserCmdFromBuffer( bf_read& buf, int slot );


	virtual void					View_Render( vrect_t *rect );
	virtual void					RenderView( const CViewSetup &view, int nClearFlags, int whatToDraw );
	virtual void					View_Fade( ScreenFade_t *pSF );
	
	virtual void					SetCrosshairAngle( const QAngle& angle );

	virtual void					InitSprite( CEngineSprite *pSprite, const char *loadname );
	virtual void					ShutdownSprite( CEngineSprite *pSprite );

	virtual int						GetSpriteSize( void ) const;

	virtual void					VoiceStatus( int entindex, qboolean bTalking );

	virtual void					InstallStringTableCallback( const char *tableName );

	virtual void					FrameStageNotify( ClientFrameStage_t curStage );

	virtual bool					DispatchUserMessage( int msg_type, bf_read &msg_data );

	// Given a list of "S(wavname) S(wavname2)" tokens, look up the localized text and emit
	//  the appropriate close caption if running with closecaption = 1
	virtual void			EmitSentenceCloseCaption( char const *tokenstream );
	virtual void			EmitCloseCaption( char const *captionname, float duration );

	virtual CStandardRecvProxies* GetStandardRecvProxies();

	virtual bool			CanRecordDemo( char *errorMsg, int length ) const;

	virtual void			OnDemoRecordStart( char const* pDemoBaseName );
	virtual void			OnDemoRecordStop();
	virtual void			OnDemoPlaybackStart( char const* pDemoBaseName );
	virtual void			OnDemoPlaybackStop();

	virtual void			RecordDemoPolishUserInput( int nCmdIndex );

	// Cache replay ragdolls
	virtual bool			CacheReplayRagdolls( const char* pFilename, int nStartTick );

	virtual bool			ShouldDrawDropdownConsole();

	// Get client screen dimensions
	virtual int				GetScreenWidth();
	virtual int				GetScreenHeight();

	// Gets the location of the player viewpoint
	virtual bool			GetPlayerView( CViewSetup &playerView );

	// Matchmaking
	virtual void			SetupGameProperties( CUtlVector< XUSER_CONTEXT > &contexts, CUtlVector< XUSER_PROPERTY > &properties );
	virtual uint			GetPresenceID( const char *pIDName );
	virtual const char		*GetPropertyIdString( const uint id );
	virtual void			GetPropertyDisplayString( uint id, uint value, char *pOutput, int nBytes );
	virtual void			StartStatsReporting( HANDLE handle, bool bArbitrated );

	virtual void			InvalidateMdlCache();

	virtual void			ReloadFilesInList( IFileList *pFilesToReload );

	// Let the client handle UI toggle - if this function returns false, the UI will toggle, otherwise it will not.
	virtual bool			HandleUiToggle();

	// Allow the console to be shown?
	virtual bool			ShouldAllowConsole();

	// Get renamed recv tables
	virtual CRenamedRecvTableInfo	*GetRenamedRecvTableInfos();

	virtual bool			ShouldHideLoadingPlaque( void );

	// Get the mouthinfo for the sound being played inside UI panels
	virtual CMouthInfo		*GetClientUIMouthInfo();

	// Notify the client that a file has been received from the game server
	virtual void			FileReceived( const char * fileName, unsigned int transferID );

	virtual const char* TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName );
	
	virtual void			ClientAdjustStartSoundParams( struct StartSoundParams_t& params );
	
	// Returns true if the disconnect command has been handled by the client
	virtual bool DisconnectAttempt( void );

	virtual void			CenterStringOff();

	virtual void			OnScreenSizeChanged( int nOldWidth, int nOldHeight );
	virtual IMaterialProxy *InstantiateMaterialProxy( const char *proxyName );

	virtual vgui::VPANEL	GetFullscreenClientDLLVPanel( void );
	virtual void			MarkEntitiesAsTouching( IClientEntity *e1, IClientEntity *e2 );
	virtual void			OnKeyBindingChanged( ButtonCode_t buttonCode, char const *pchKeyName, char const *pchNewBinding );
	virtual bool			HandleGameUIEvent( const InputEvent_t &event );

public:
	void PrecacheMaterial( const char *pMaterialName );

	virtual bool IsConnectedUserInfoChangeAllowed( IConVar *pCvar );

	virtual void			SetBlurFade( float scale );
	
	virtual void			ResetHudCloseCaption();

	virtual bool			SupportsRandomMaps();

	// This can just be a wrapper on MapEntity_ParseAllEntities, but CS does some tricks in here
	// with the entity list.
	void LevelInit_ParseAllEntities( const char *pMapEntities );

private:

	void UncacheAllMaterials( );
	void ResetStringTablePointers();

	CUtlRBTree< IMaterial * > m_CachedMaterials;

	CHudCloseCaption		*m_pHudCloseCaption;

	bool m_bWasPaused;
	float m_fPauseTime;
	int m_nPauseTick;
};

// Returns the CPU/GPU level
CPULevel_t GetCPULevel();
// Returns the actual value of the CPU level convar, even on the 360
CPULevel_t GetActualCPULevel();
GPULevel_t GetGPULevel();
void ConfigureCurrentSystemLevel();

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

extern AchievementsAndStatsInterface* g_pAchievementsAndStatsInterface;

//=============================================================================
// HPE_END
//=============================================================================

// Set to true between LevelInit and LevelShutdown.
extern bool	g_bLevelInitialized;
extern bool g_bTextMode;

// Returns true if a new OnDataChanged event is registered for this frame.
bool AddDataChangeEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent );

void ClearDataChangedEvent( int iStoredEvent );

//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts precached material indices into strings
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nIndex );

//-----------------------------------------------------------------------------
// Precache-related methods for particle systems
//-----------------------------------------------------------------------------
int PrecacheParticleSystem( const char *pParticleSystemName );
int GetParticleSystemIndex( const char *pParticleSystemName );
const char *GetParticleSystemNameFromIndex( int nIndex );
void PrecacheEffect( const char *pEffectName );


//-----------------------------------------------------------------------------
// Called during bone setup to test perf
//-----------------------------------------------------------------------------
void TrackBoneSetupEnt( C_BaseAnimating *pEnt );

bool IsEngineThreaded();

/// Returns Steam ID, given player index.   Returns an invalid SteamID upon
/// failure
extern bool GetSteamIDForPlayerIndex( int iPlayerIndex, CSteamID &steamid );

#endif // CDLL_CLIENT_INT_H
