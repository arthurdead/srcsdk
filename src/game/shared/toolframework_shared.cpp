#include "toolframework_shared.h"
#include "toolframework/itooldictionary.h"
#include "toolframework/itoolsystem.h"
#include "utlvector.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/EditablePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CToolDictionary : public CBaseAppSystem<IToolDictionary>
{
public:
	virtual int GetToolCount() const;
	virtual IToolSystem *GetTool( int index );
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual void CreateTools();

private:
	CUtlVector<IToolSystem *> m_Tools;
};

int CToolDictionary::GetToolCount() const
{
	return m_Tools.Count();
}

IToolSystem *CToolDictionary::GetTool( int index )
{
	if ( index < 0 || index >= m_Tools.Count() )
	{
		return NULL;
	}
	return m_Tools[ index ];
}

void *CToolDictionary::QueryInterface( const char *pInterfaceName )
{
	return NULL;
}

static CToolDictionary g_ToolDictionary;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( IToolDictionary, CToolDictionary, VTOOLDICTIONARY_INTERFACE_VERSION, g_ToolDictionary );

#ifndef GAME_DLL
IServerTools	*servertools = NULL;
#else
extern IServerTools	*servertools;
#endif

#ifndef CLIENT_DLL
IClientTools	*clienttools = NULL;
#else
extern IClientTools	*clienttools;
#endif

class CGameToolSystem : public IToolSystem
{
public:
	CGameToolSystem();

	// Name describing the tool
	virtual char const *GetToolName();

	// Called at the end of engine startup (after client .dll and server .dll have been loaded)
	virtual bool Init();

	// Called during RemoveTool or when engine is shutting down
	virtual void	Shutdown();

	// Called after server.dll is loaded
	virtual bool	ServerInit( CreateInterfaceFn serverFactory ); 
	// Called after client.dll is loaded
	virtual bool	ClientInit( CreateInterfaceFn clientFactory ); 

	virtual void	ServerShutdown();
	virtual void	ClientShutdown();

	// Allow tool to override quitting, called before Shutdown(), return no to abort quitting
	virtual bool	CanQuit(); 

	// Called when another system wiches to post a message to the tool and/or a specific entity
	// FIXME:  Are KeyValues too inefficient here?
    virtual void	PostMessage( HTOOLHANDLE hEntity, KeyValues *message );

	// Called oncer per frame even when no level is loaded... (call ProcessMessages())
	virtual void	Think( bool finalTick );

// Server calls:

	// Level init, shutdown
	virtual void	ServerLevelInitPreEntity();
	// entities are created / spawned / precached here
	virtual void	ServerLevelInitPostEntity();

	virtual void	ServerLevelShutdownPreEntity();
	// Entities are deleted / released here...
	virtual void	ServerLevelShutdownPostEntity();
	// end of level shutdown

	// Called each frame before entities think
	virtual void	ServerFrameUpdatePreEntityThink();
	// called after entities think
	virtual void	ServerFrameUpdatePostEntityThink();
	virtual void	ServerPreClientUpdate();
	virtual void	ServerPreSetupVisibility();

	// Used to allow the tool to spawn different entities when it's active
	virtual const char* GetEntityData( const char *pActualEntityData );

// Client calls:
	// Level init, shutdown
	virtual void	ClientLevelInitPreEntity();
	// entities are created / spawned / precached here
	virtual void	ClientLevelInitPostEntity();

	virtual void	ClientLevelShutdownPreEntity();
	// Entities are deleted / released here...
	virtual void	ClientLevelShutdownPostEntity();
	// end of level shutdown
	// Called before rendering
	virtual void	ClientPreRender();
	virtual void	ClientPostRender();

	// Let tool override viewport for engine
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );

	// let tool override view/camera
	virtual bool	SetupEngineView( Vector &origin, QAngle &angles, float &fov );

	// let tool override microphone
	virtual bool	SetupAudioState( AudioState_t &audioState );

	// Should the client be allowed to render the view normally?
	virtual bool	ShouldGameRenderView();
	virtual bool	IsThirdPersonCamera();

	// is the current tool recording?
	virtual bool	IsToolRecording();

	virtual IMaterialProxy *LookupProxy( const char *proxyName );

	// Possible hooks for rendering
	// virtual void	Think( float curtime, float frametime );
	// virtual void Prerender();
	// virtual void Render3D();
	// virtual void Render2D();
// Tool activation/deactivation

	// This tool is being activated
	virtual void	OnToolActivate();
	// Another tool is being activated
	virtual void	OnToolDeactivate();

	virtual bool	TrapKey( ButtonCode_t key, bool down );

	virtual bool	GetSoundSpatialization( int iUserData, int guid, SpatializationInfo_t& info );

	// Unlike the client .dll pre/post render stuff, these get called no matter whether a map is loaded and they only get called once per frame!!!
	virtual void		RenderFrameBegin();
	virtual void		RenderFrameEnd();

	// wraps the entire frame - surrounding all other begin/end and pre/post calls
	virtual void		HostRunFrameBegin();
	virtual void		HostRunFrameEnd();

	// See enginevgui.h for paintmode_t enum definitions
	virtual void		VGui_PreRender( PaintMode_t paintMode );
	virtual void		VGui_PostRender( PaintMode_t paintMode );

	virtual void		VGui_PreSimulate();
	virtual void		VGui_PostSimulate();

private:
	EditablePanel *m_pPanel;

	bool m_bActive;
};

static CGameToolSystem g_GameToolSystem;

void CToolDictionary::CreateTools()
{
	m_Tools.AddToTail(&g_GameToolSystem);
}

CGameToolSystem::CGameToolSystem()
{
	m_bActive = false;
	m_pPanel = NULL;
}

const char *CGameToolSystem::GetToolName()
{
#ifdef GAME_DLL
	return "server";
#else
	return "client";
#endif
}

bool CGameToolSystem::Init()
{
	g_pVGuiLocalize->AddFile( "resource/dmecontrols_%language%.txt" );
	g_pVGuiLocalize->AddFile( "resource/toolshared_%language%.txt" );
	g_pVGuiLocalize->AddFile( "resource/boxrocket_%language%.txt" );

	//TODO!!!!!!!!!!!
	m_pPanel = CREATE_PANEL(EditablePanel, NULL, GetToolName());

	int w, h;
	surface()->GetScreenSize( w, h );

	m_pPanel->SetBounds( 0, 0, w, h );
	m_pPanel->SetPaintBackgroundEnabled( true );
	m_pPanel->SetPaintBorderEnabled( false );
	m_pPanel->SetPaintEnabled( false );
	m_pPanel->SetCursor( vgui::dc_none );
	m_pPanel->SetVisible( false );

	return true;
}

void CGameToolSystem::Shutdown()
{
	if(m_pPanel) {
		m_pPanel->DeletePanel();
		m_pPanel = NULL;
	}
}

bool CGameToolSystem::ServerInit( CreateInterfaceFn serverFactory )
{
#ifndef GAME_DLL
	servertools = ( IServerTools * )serverFactory( VSERVERTOOLS_INTERFACE_VERSION, NULL );
#endif

	return true;
}

bool CGameToolSystem::ClientInit( CreateInterfaceFn clientFactory )
{
#ifndef CLIENT_DLL
	clienttools = ( IClientTools * )clientFactory( VCLIENTTOOLS_INTERFACE_VERSION, NULL );
#endif

	return true;
}

void CGameToolSystem::ServerShutdown()
{
#ifndef GAME_DLL
	servertools = NULL;
#endif
}

void CGameToolSystem::ClientShutdown()
{
#ifndef CLIENT_DLL
	clienttools = NULL;
#endif
}

bool CGameToolSystem::CanQuit()
{
	return true;
}

void CGameToolSystem::PostMessage( HTOOLHANDLE hEntity, KeyValues *message )
{
}

void CGameToolSystem::Think( bool finalTick )
{
}

void CGameToolSystem::ServerLevelInitPreEntity()
{
}

void CGameToolSystem::ServerLevelInitPostEntity()
{
}

void CGameToolSystem::ServerLevelShutdownPreEntity()
{
}

void CGameToolSystem::ServerLevelShutdownPostEntity()
{
}

void CGameToolSystem::ServerFrameUpdatePreEntityThink()
{
}

void CGameToolSystem::ServerFrameUpdatePostEntityThink()
{
}

void CGameToolSystem::ServerPreClientUpdate()
{
}

void CGameToolSystem::ServerPreSetupVisibility()
{
}

const char *CGameToolSystem::GetEntityData( const char *pActualEntityData )
{
	return pActualEntityData;
}

void CGameToolSystem::ClientLevelInitPreEntity()
{
}

void CGameToolSystem::ClientLevelInitPostEntity()
{
}

void CGameToolSystem::ClientLevelShutdownPreEntity()
{
}

void CGameToolSystem::ClientLevelShutdownPostEntity()
{
}

void CGameToolSystem::ClientPreRender()
{
}

void CGameToolSystem::ClientPostRender()
{
}

void CGameToolSystem::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
}

bool CGameToolSystem::SetupEngineView( Vector &origin, QAngle &angles, float &fov )
{
	return false;
}

bool CGameToolSystem::SetupAudioState( AudioState_t &audioState )
{
	return false;
}

bool CGameToolSystem::ShouldGameRenderView()
{
	return true;
}

bool CGameToolSystem::IsThirdPersonCamera()
{
	return false;
}

bool CGameToolSystem::IsToolRecording()
{
	return false;
}

IMaterialProxy *CGameToolSystem::LookupProxy( const char *proxyName )
{
	return NULL;
}

void CGameToolSystem::OnToolActivate()
{
	m_bActive = true;

	if(m_pPanel) {
		m_pPanel->SetMouseInputEnabled( true );
		m_pPanel->SetVisible( false );
	}
}

void CGameToolSystem::OnToolDeactivate()
{
	m_bActive = false;

	if(m_pPanel) {
		m_pPanel->SetMouseInputEnabled( false );
		m_pPanel->SetVisible( true );
	}
}

bool CGameToolSystem::TrapKey( ButtonCode_t key, bool down )
{
	if(!m_bActive)
		return false;



	return false;
}

bool CGameToolSystem::GetSoundSpatialization( int iUserData, int guid, SpatializationInfo_t& info )
{
	return true;
}

void CGameToolSystem::RenderFrameBegin()
{
}

void CGameToolSystem::RenderFrameEnd()
{
}

void CGameToolSystem::HostRunFrameBegin()
{
}

void CGameToolSystem::HostRunFrameEnd()
{
}

void CGameToolSystem::VGui_PreRender( PaintMode_t paintMode )
{
}

void CGameToolSystem::VGui_PostRender( PaintMode_t paintMode )
{
}

void CGameToolSystem::VGui_PreSimulate()
{
}

void CGameToolSystem::VGui_PostSimulate()
{
}
