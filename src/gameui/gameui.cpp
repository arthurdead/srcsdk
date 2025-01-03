#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include "gameui.h"
#include "tier3/tier3.h"
#include "vgui/IPanel.h"
#include "vgui_controls/PHandle.h"
#include "engineinterface.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/AnimationController.h"
#include "tier1/KeyValues.h"
#include "hackmgr/dlloverride.h"
#include "filesystem.h"
#include "video/ivideoservices.h"
#include "matsys_controls/matsyscontrols.h"
#include "ienginevgui.h"
#include "vguisystemmoduleloader.h"
#include "steam/steam_api.h"
#include "rmlui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGameUI g_GameUI;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUI, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUI, IGameUIEx, GAMEUI_EX_INTERFACE_VERSION, g_GameUI);

IGameUIFuncs *gameuifuncs = NULL;
IVEngineClient *engine = NULL;
IEngineVGui *enginevguifuncs = NULL;

CSysModule* videoServicesDLL = NULL;

ConVarBase *name = NULL;
ConVarBase*	r_showenvcubemap=NULL;
ConVarBase*	r_eyegloss=NULL;
ConVarBase*	r_eyemove=NULL;
ConVarBase*	r_eyeshift_x=NULL;
ConVarBase*	r_eyeshift_y=NULL;
ConVarBase*	r_eyeshift_z=NULL;
ConVarBase*	r_eyesize=NULL;
ConVarBase*	mat_softwareskin=NULL;
ConVarBase*	r_nohw=NULL;
ConVarBase*	r_nosw=NULL;
ConVarBase*	r_teeth=NULL;
ConVarBase*	r_flex=NULL;
ConVarBase*	r_eyes=NULL;
ConVarBase*	r_skin=NULL;
ConVarBase*	r_maxmodeldecal=NULL;
ConVarBase*	r_modelwireframedecal=NULL;
ConVarBase*	mat_normals=NULL;
ConVarBase*	r_eyeglintlodpixels=NULL;
ConVarBase*	r_rootlod=NULL;
ConVarBase *cl_hud_minmode=NULL;

CGameUI::CGameUI()
{
	m_bActivatedUI = false;
	m_VPanel = vgui::INVALID_VPANEL;
	m_bNeedsRepaint = true;
}

CGameUI::~CGameUI()
{
}

// ------------------------------------------------------------------------------------------- //
// ConVar stuff.
// ------------------------------------------------------------------------------------------- //
class CGameUIConVarAccessor : public CDefaultAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		if(pCommand->IsFlagSet(FCVAR_GAMEDLL))
			DevMsg("gameui dll tried to register server con var/command named %s\n", pCommand->GetName());
	#endif

		return CDefaultAccessor::RegisterConCommandBase( pCommand );
	}
};

CGameUIConVarAccessor g_GameUIConVarAccessor;

const char *CGameUI::GetName()
{
	return "Rml Root";
}

const char *CGameUI::GetModuleName()
{
	return vgui::GetControlsModuleName();
}

void *CGameUI::QueryInterface(vgui::EInterfaceID id)
{
	if (id == vgui::ICLIENTPANEL_STANDARD_INTERFACE)
		return static_cast<IClientPanel *>(this);

	return NULL;
}

void CGameUI::Initialize( CreateInterfaceFn appFactory )
{
	MEM_ALLOC_CREDIT();
	ConnectTier1Libraries( &appFactory, 1 );
	ConnectTier2Libraries( &appFactory, 1 );

	if( g_pCVar == NULL )
	{
		Error( "CGameUI::Initialize() failed to get necessary interfaces\n" );
	}

	ConVar_Register( FCVAR_CLIENTDLL, &g_GameUIConVarAccessor );

	name = g_pCVar->FindVarBase("name");
	r_showenvcubemap = g_pCVar->FindVarBase("r_showenvcubemap");
	r_eyegloss = g_pCVar->FindVarBase("r_eyegloss");
	r_eyemove = g_pCVar->FindVarBase("r_eyemove");
	r_eyeshift_x = g_pCVar->FindVarBase("r_eyeshift_x");
	r_eyeshift_y = g_pCVar->FindVarBase("r_eyeshift_y");
	r_eyeshift_z = g_pCVar->FindVarBase("r_eyeshift_z");
	r_eyesize = g_pCVar->FindVarBase("r_eyesize");
	mat_softwareskin = g_pCVar->FindVarBase("mat_softwareskin");
	r_nohw = g_pCVar->FindVarBase("r_nohw");
	r_nosw = g_pCVar->FindVarBase("r_nosw");
	r_teeth = g_pCVar->FindVarBase("r_teeth");
	r_flex = g_pCVar->FindVarBase("r_flex");
	r_eyes = g_pCVar->FindVarBase("r_eyes");
	r_skin = g_pCVar->FindVarBase("r_skin");
	r_maxmodeldecal = g_pCVar->FindVarBase("r_maxmodeldecal");
	r_modelwireframedecal = g_pCVar->FindVarBase("r_modelwireframedecal");
	mat_normals = g_pCVar->FindVarBase("mat_normals");
	r_eyeglintlodpixels = g_pCVar->FindVarBase("r_eyeglintlodpixels");
	r_rootlod = g_pCVar->FindVarBase("r_rootlod");
	cl_hud_minmode = g_pCVar->FindVarBase("cl_hud_minmode");

	ConnectTier3Libraries( &appFactory, 1 );

	if( g_pFullFileSystem == NULL )
	{
		Error( "CGameUI::Initialize() failed to get necessary interfaces\n" );
	}

	HackMgr_SwapVideoServices( appFactory, videoServicesDLL );

	engine = (IVEngineClient *)appFactory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );
	enginevguifuncs = (IEngineVGui *)appFactory( VENGINE_VGUI_VERSION, NULL );
	gameuifuncs = (IGameUIFuncs *)appFactory( VENGINE_GAMEUIFUNCS_VERSION, NULL );

	if ( !engine || !enginevguifuncs || !gameuifuncs )
	{
		Error( "CGameUI::Initialize() failed to get necessary interfaces\n" );
	}

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	vgui::VGui_InitInterfacesList( "GameUI", &appFactory, 1 );
	vgui::VGui_InitMatSysInterfacesList( "GameUI", &appFactory, 1 );

	g_pVGuiLocalize->AddFile( "Resource/vgui_%language%.txt", "MOD", false );
	g_pVGuiLocalize->AddFile( "Resource/platform_%language%.txt", "MOD", false );
	g_pVGuiLocalize->AddFile( "Resource/gameui_%language%.txt", "MOD", false );
	g_pVGuiLocalize->AddFile( "Resource/valve_%language%.txt", "MOD", false );
	g_pVGuiLocalize->AddFile( "Resource/deck_%language%.txt", "MOD", false );
	g_pVGuiLocalize->AddFile( "servers/serverbrowser_%language%.txt", "MOD", false );

	vgui::VPANEL rootpanel = enginevguifuncs->GetPanel(PANEL_GAMEUIDLL);

	m_VPanel = vgui::ivgui()->AllocPanel();
	if(m_VPanel != vgui::INVALID_VPANEL) {
		vgui::ipanel()->Init(m_VPanel, this);
		vgui::ipanel()->SetParent(m_VPanel, rootpanel);

		vgui::ipanel()->SetPos( m_VPanel, 0, 0 );
		vgui::ipanel()->SetSize( m_VPanel, 640, 480 );

		vgui::ipanel()->SetMouseInputEnabled( m_VPanel, false );
		vgui::ipanel()->SetKeyBoardInputEnabled( m_VPanel, false );
	}

	AllowEngineShowGameUI();
	ActivateGameUI();

	g_RmlRenderInterface.Initialize();

	Rml::SetRenderInterface(&g_RmlRenderInterface);
	Rml::SetSystemInterface(&g_RmlSystemInterface);
	Rml::SetFileInterface(&g_RmlFileInterface);

	Rml::Factory::RegisterContextInstancer(&g_RmlFactory);

	if(!Rml::Initialise())
	{
		Error( "CGameUI::Initialize() failed to Initialise Rml\n" );
	}

	Rml::LoadFontFace("Resource/LatoLatin-Bold.ttf");
	Rml::LoadFontFace("Resource/LatoLatin-BoldItalic.ttf");
	Rml::LoadFontFace("Resource/LatoLatin-Italic.ttf");
	Rml::LoadFontFace("Resource/LatoLatin-Regular.ttf");
	Rml::LoadFontFace("Resource/NotoEmoji-Regular.ttf");
}

void CGameUI::PostInit()
{
}

void CGameUI::Connect( CreateInterfaceFn gameFactory )
{
}

void CGameUI::Start()
{
}

void CGameUI::Shutdown()
{
	// notify all the modules of Shutdown
	g_VModuleLoader.ShutdownPlatformModules();

	// unload the modules them from memory
	g_VModuleLoader.UnloadPlatformModules();

	if ( videoServicesDLL )
		Sys_UnloadModule( videoServicesDLL );

	ConVar_Unregister();
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

void CGameUI::GetClipRect(int &x0, int &y0, int &x1, int &y1)
{
	if(m_VPanel == vgui::INVALID_VPANEL)
		return;

	vgui::ipanel()->GetClipRect(m_VPanel, x0, y0, x1, y1);
}

void CGameUI::PaintTraverse(bool forceRepaint, bool allowForce)
{
	if(m_VPanel == vgui::INVALID_VPANEL)
		return;

	if ( !vgui::ipanel()->IsVisible( m_VPanel ) )
	{
		return;
	}

	if ( !forceRepaint &&
		 allowForce &&
		 m_bNeedsRepaint )
	{
		forceRepaint = true;
		m_bNeedsRepaint = false;
	}

	float oldAlphaMultiplier = vgui::surface()->DrawGetAlphaMultiplier();

	vgui::surface()->DrawSetAlphaMultiplier( 1 );

	if ( forceRepaint )
	{
		vgui::surface()->PushMakeCurrent( m_VPanel, false );

		

		vgui::surface()->PopMakeCurrent( m_VPanel );
	}

	int wide, tall;
	vgui::ipanel()->GetSize(m_VPanel, wide, tall);
	int x, y;
	vgui::ipanel()->GetPos(m_VPanel, x, y);

	g_RmlRenderInterface.BeginRender(x, y, wide, tall);

	// traverse and paint all our children
	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( m_VPanel );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];
		bool bVisible = vgui::ipanel()->IsVisible( child );

		if ( vgui::surface()->ShouldPaintChildPanel( child ) )
		{
			if ( bVisible )
			{
				vgui::ipanel()->PaintTraverse( child, forceRepaint, allowForce );
			}
		}
		else
		{
			// Invalidate the child panel so that it gets redrawn
			vgui::surface()->Invalidate( child );

			// keep traversing the tree, just don't allow anyone to paint after here
			if ( bVisible )
			{
				vgui::ipanel()->PaintTraverse( child, false, false );
			}
		}
	}

	g_RmlRenderInterface.EndRender();

	vgui::surface()->DrawSetAlphaMultiplier( oldAlphaMultiplier );

	vgui::surface()->SwapBuffers( m_VPanel );
}

void CGameUI::Repaint()
{
	if(m_VPanel != vgui::INVALID_VPANEL)
		vgui::surface()->Invalidate(m_VPanel);
	m_bNeedsRepaint = true;
}

bool CGameUI::RequestInfo(KeyValues *outputData)
{
	if(V_strcmp(outputData->GetName(), "alpha") == 0) {
		outputData->SetInt("alpha", 255);
		return true;
	}
	return false;
}

void CGameUI::DeletePanel()
{
	if(m_VPanel != vgui::INVALID_VPANEL) {
		vgui::ivgui()->FreePanel(m_VPanel);
		m_VPanel = vgui::INVALID_VPANEL;
	}
}

void CGameUI::OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel)
{
	const char *title = params->GetName();
	if(V_strcmp(title, "Delete") == 0) {
		DeletePanel();
	} else {
		static int s_bDebugMessages = -1;
		if ( s_bDebugMessages == -1 )
		{
			s_bDebugMessages = CommandLine()->FindParm( "-vguimessages" ) ? 1 : 0;
		}
		if ( s_bDebugMessages == 1 )
		{
			vgui::ivgui()->DPrintf( "Message '%s' not handled by panel '%s'\n", params->GetName(), GetName() );
		}
	}
}

void CGameUI::Think()
{

}

void CGameUI::OnTick()
{

}

void CGameUI::RunFrame()
{
	if( m_VPanel != vgui::INVALID_VPANEL ) {
		int wide, tall;
		vgui::surface()->GetScreenSize(wide, tall);
		vgui::ipanel()->SetSize(m_VPanel,wide,tall);
	}

	vgui::GetAnimationController()->UpdateAnimations( engine->Time() );

	// Run frames
	g_VModuleLoader.RunFrame();
}

void CGameUI::OnGameUIActivated()
{
	m_bActivatedUI = true;

	if( IsInLevel() )
		engine->ClientCmd_Unrestricted( "setpause nomsg" );

	if( m_VPanel != vgui::INVALID_VPANEL )
		vgui::ipanel()->SetVisible( m_VPanel, true );
}

void CGameUI::OnGameUIHidden()
{
	m_bActivatedUI = false;

	if( IsInLevel() )
		engine->ClientCmd_Unrestricted( "unpause nomsg" );

	if( m_VPanel != vgui::INVALID_VPANEL )
		vgui::ipanel()->SetVisible( m_VPanel, false );
}

void CGameUI::OnLevelLoadingStarted(bool bShowProgressDialog)
{
}

void CGameUI::OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason)
{
}

bool CGameUI::UpdateProgressBar(float progress, const char *statusText)
{
	return true;
}

bool CGameUI::SetShowProgressText( bool show )
{
	return false;
}

void CGameUI::ShowNewGameDialog( int chapter )
{
}

void CGameUI::SetLoadingBackgroundDialog( vgui::VPANEL panel )
{
}

void CGameUI::BonusMapUnlock( const char *pchFileName, const char *pchMapName )
{
}

void CGameUI::BonusMapComplete( const char *pchFileName, const char *pchMapName )
{
}

void CGameUI::BonusMapChallengeUpdate( const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest )
{
}

void CGameUI::BonusMapChallengeNames( char *pchFileName, char *pchMapName, char *pchChallengeName )
{
}

void CGameUI::BonusMapChallengeObjectives( int &iBronze, int &iSilver, int &iGold )
{
}

void CGameUI::BonusMapDatabaseSave( void )
{
}

int CGameUI::BonusMapNumAdvancedCompleted( void )
{
	return 0;
}

void CGameUI::BonusMapNumMedals( int piNumMedals[ 3 ] )
{
}

void CGameUI::OnConnectToServer(const char *game, int IP, int connectionPort, int queryPort)
{
}

void CGameUI::SetProgressOnStart()
{
}
void CGameUI::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{
}

void CGameUI::OnConfirmQuit( void )
{
}

bool CGameUI::IsMainMenuVisible( void )
{
	if( m_VPanel == vgui::INVALID_VPANEL )
		return m_bActivatedUI;

	return vgui::ipanel()->IsVisible( m_VPanel );
}

void CGameUI::SetMainMenuOverride( vgui::VPANEL panel )
{
}

void CGameUI::SendMainMenuCommand( const char *pszCommand )
{
}

bool CGameUI::IsPanelVisible()
{
	if( m_VPanel == vgui::INVALID_VPANEL )
		return false;

	return vgui::ipanel()->IsVisible( m_VPanel );
}

//-----------------------------------------------------------------------------
// Purpose: Makes the loading background dialog visible, if one has been set
//-----------------------------------------------------------------------------
void CGameUI::ShowLoadingBackgroundDialog()
{
#if 0
	if ( g_hLoadingBackgroundDialog != vgui::INVALID_VPANEL )
	{
		vgui::VPANEL panel = m_pPanel->GetVPanel();

		vgui::ipanel()->SetParent( g_hLoadingBackgroundDialog, panel );
		vgui::ipanel()->MoveToFront( g_hLoadingBackgroundDialog );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Hides the loading background dialog, if one has been set
//-----------------------------------------------------------------------------
void CGameUI::HideLoadingBackgroundDialog()
{
#if 0
	if ( g_hLoadingBackgroundDialog != vgui::INVALID_VPANEL )
	{
		if ( engine->IsInGame() )
		{
			vgui::ivgui()->PostMessage( g_hLoadingBackgroundDialog, new KeyValues( "LoadedIntoGame" ), vgui::INVALID_VPANEL );
		}
		else
		{
			vgui::ipanel()->SetVisible( g_hLoadingBackgroundDialog, false );
			vgui::ipanel()->MoveToBack( g_hLoadingBackgroundDialog );
		}

		vgui::ivgui()->PostMessage( g_hLoadingBackgroundDialog, new KeyValues("HideAsLoadingPanel"), vgui::INVALID_VPANEL );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a loading background dialog has been set
//-----------------------------------------------------------------------------
bool CGameUI::HasLoadingBackgroundDialog()
{
	//return ( vgui::INVALID_VPANEL != g_hLoadingBackgroundDialog );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to activate the gameUI
//-----------------------------------------------------------------------------
void CGameUI::ActivateGameUI()
{
	engine->ExecuteClientCmd("gameui_activate");
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to hide the gameUI
//-----------------------------------------------------------------------------
void CGameUI::HideGameUI()
{
	engine->ExecuteClientCmd("gameui_hide");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::PreventEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_preventescape");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::AllowEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_allowescape");
}

void CGameUI::PreventEngineShowGameUI()
{
	engine->ExecuteClientCmd("gameui_preventescapetoshow");
}

void CGameUI::AllowEngineShowGameUI()
{
	engine->ExecuteClientCmd("gameui_allowescapetoshow");
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're currently playing the game
//-----------------------------------------------------------------------------
bool CGameUI::IsInLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && !engine->IsLevelMainMenuBackground())
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're at the main menu and a background level is loaded
//-----------------------------------------------------------------------------
bool CGameUI::IsInBackgroundLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && engine->IsLevelMainMenuBackground())
	{
		return true;
	}
	return false;
}
