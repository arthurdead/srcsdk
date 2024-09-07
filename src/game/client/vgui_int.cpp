//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "itextmessage.h"
#include "vguicenterprint.h"
#include "iloadingdisc.h"
#include "ifpspanel.h"
#include "imessagechars.h"
#include "inetgraphpanel.h"
#include "idebugoverlaypanel.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include "tier0/vprof.h"
#include "iclientmode.h"
#include <vgui_controls/Panel.h>
#include <KeyValues.h>
#include "filesystem.h"
#include "matsys_controls/matsyscontrols.h"

using namespace vgui;

#include <vgui/IInputInternal.h>
vgui::IInputInternal *g_InputInternal = NULL;

#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool IsWidescreen( void );

void GetVGUICursorPos( int& x, int& y )
{
	vgui::input()->GetCursorPos(x, y);
}

void SetVGUICursorPos( int x, int y )
{
	if ( !g_bTextMode )
	{
		vgui::input()->SetCursorPos(x, y);
	}
}

class CHudTextureHandleProperty : public vgui::IPanelAnimationPropertyConverter
{
public:
	virtual void GetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );
		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		// lookup texture name for id
		if ( pHandle->Get() )
		{
			kv->SetString( entry->name(), pHandle->Get()->szShortName );
		}
		else
		{
			kv->SetString( entry->name(), "" );
		}
	}
	
	virtual void SetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );
		
		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		const char *texturename = kv->GetString( entry->name() );
		if ( texturename && texturename[ 0 ] )
		{
			CHudTexture *currentTexture = HudIcons().GetIcon( texturename );
			pHandle->Set( currentTexture );
		}
		else
		{
			pHandle->Set( NULL );
		}
	}

	virtual void InitFromDefault( Panel *panel, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );

		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		const char *texturename = entry->defaultvalue();
		if ( texturename && texturename[ 0 ] )
		{
			CHudTexture *currentTexture = HudIcons().GetIcon( texturename );
			pHandle->Set( currentTexture );
		}
		else
		{
			pHandle->Set( NULL );
		}
	}
};

static CHudTextureHandleProperty textureHandleConverter;

bool IsWidescreen( void )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	int screenWidth, screenHeight;
	g_pMaterialSystem->GetBackBufferDimensions( screenWidth, screenHeight );
	float aspectRatio = ( float )screenWidth / ( float )screenHeight;
	// Has to be nearly 16:10 or higher to be considered widescreen.  We do this elsewhere in the code.
	bool bIsWidescreen = ( aspectRatio >= 1.59f ); 
	return bIsWidescreen;
}

static void VGui_VideoMode_AdjustForModeChange( void )
{
	// Kill all our panels. We need to do this in case any of them
	//	have pointers to objects (eg: iborders) that will get freed
	//	when schemes get destroyed and recreated (eg: mode change).
	netgraphpanel->Destroy();
	debugoverlaypanel->Destroy();
#if defined( TRACK_BLOCKING_IO )
	iopanel->Destroy();
#endif
	fps->Destroy();
	messagechars->Destroy();
	loadingdisc->Destroy();

	// Recreate our panels.
	VPANEL gameToolParent = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
	VPANEL toolParent = enginevgui->GetPanel( PANEL_TOOLS );
#if defined( TRACK_BLOCKING_IO )
	VPANEL gameDLLPanel = enginevgui->GetPanel( PANEL_GAMEDLL );
#endif

	loadingdisc->Create( gameToolParent );
	messagechars->Create( gameToolParent );

	// Debugging or related tool
	fps->Create( toolParent );
#if defined( TRACK_BLOCKING_IO )
	iopanel->Create( gameDLLPanel );
#endif
	netgraphpanel->Create( toolParent );
	debugoverlaypanel->Create( gameToolParent );
}

static void VGui_OneTimeInit()
{
	static bool initialized = false;
	if ( initialized )
		return;
	initialized = true;

	vgui::Panel::AddPropertyConverter( "CHudTextureHandle", &textureHandleConverter );


    g_pMaterialSystem->AddModeChangeCallBack( &VGui_VideoMode_AdjustForModeChange );
}

bool VGui_Startup( CreateInterfaceFn appSystemFactory )
{
	if ( !vgui::VGui_InitInterfacesList( "CLIENT", &appSystemFactory, 1 ) )
		return false;

	if ( !vgui::VGui_InitMatSysInterfacesList( "CLIENT", &appSystemFactory, 1 ) )
		return false;

	g_InputInternal = (IInputInternal *)appSystemFactory( VGUI_INPUTINTERNAL_INTERFACE_VERSION,  NULL );
	if ( !g_InputInternal )
	{
		return false; // c_vguiscreen.cpp needs this!
	}

	VGui_OneTimeInit();

	// Create any root panels for .dll
	VGUI_CreateClientDLLRootPanel();

	// Make sure we have a panel
	VPANEL root = VGui_GetClientDLLRootPanel();
	if ( root == vgui::INVALID_VPANEL )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_CreateGlobalPanels( void )
{
	VPANEL gameToolParent = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
	VPANEL toolParent = enginevgui->GetPanel( PANEL_TOOLS );
#if defined( TRACK_BLOCKING_IO )
	VPANEL gameDLLPanel = enginevgui->GetPanel( PANEL_GAMEDLL );
#endif
	// Part of game
	GetCenterPrint()->Create( gameToolParent );
	loadingdisc->Create( gameToolParent );
	messagechars->Create( gameToolParent );

	// Debugging or related tool
	fps->Create( toolParent );
#if defined( TRACK_BLOCKING_IO )
	iopanel->Create( gameDLLPanel );
#endif
	netgraphpanel->Create( toolParent );
	debugoverlaypanel->Create( gameToolParent );
}

void VGui_Shutdown()
{
	netgraphpanel->Destroy();
	debugoverlaypanel->Destroy();
#if defined( TRACK_BLOCKING_IO )
	iopanel->Destroy();
#endif
	fps->Destroy();

	messagechars->Destroy();
	loadingdisc->Destroy();
	GetCenterPrint()->Destroy();

	if ( GetFullscreenClientMode() )
	{
		GetFullscreenClientMode()->VGui_Shutdown();
	}

	VGUI_DestroyClientDLLRootPanel();

	// Make sure anything "marked for deletion"
	//  actually gets deleted before this dll goes away
	vgui::ivgui()->RunFrame();
}

static ConVar cl_showpausedimage( "cl_showpausedimage", "1", 0, "Show the 'Paused' image when game is paused." );

//-----------------------------------------------------------------------------
// Things to do before rendering vgui stuff...
//-----------------------------------------------------------------------------
void VGui_PreRender()
{
	VPROF( "VGui_PreRender" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	loadingdisc->SetLoadingVisible( engine->IsDrawingLoadingImage() && !engine->IsPlayingDemo() );
	loadingdisc->SetPausedVisible( !enginevgui->IsGameUIVisible() && cl_showpausedimage.GetBool() && engine->IsPaused() && !engine->IsTakingScreenshot() && !engine->IsPlayingDemo() );
}

void VGui_PostRender()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cl_panelanimation - 
//-----------------------------------------------------------------------------
CON_COMMAND( cl_panelanimation, "Shows panel animation variables: <panelname | blank for all panels>." )
{
	if ( args.ArgC() == 2 )
	{
		PanelAnimationDumpVars( args[1] );
	}
	else
	{
		PanelAnimationDumpVars( NULL );
	}
}

void GetHudSize( int& w, int &h )
{
	vgui::surface()->GetScreenSize( w, h );

	VPANEL hudParent = enginevgui->GetPanel( PANEL_CLIENTDLL );
	if ( hudParent != vgui::INVALID_VPANEL )
	{
		vgui::ipanel()->GetSize( hudParent, w, h );
	}
}

static vrect_t g_TrueScreenSize;
static vrect_t g_ScreenSpaceBounds;

void VGui_GetTrueScreenSize( int &w, int &h )
{
	w = g_TrueScreenSize.width;
	h = g_TrueScreenSize.height;
}

void VGUI_SetScreenSpaceBounds( int x, int y, int w, int h )
{
	vrect_t &r = g_ScreenSpaceBounds;
	r.x = x;
	r.y = y;
	r.width = w;
	r.height = h;
}

void VGUI_UpdateScreenSpaceBounds( int sx, int sy, int sw, int sh )
{
	g_TrueScreenSize.x = sx;
	g_TrueScreenSize.y = sy;
	g_TrueScreenSize.width = sw;
	g_TrueScreenSize.height = sh;

	VGUI_SetScreenSpaceBounds( sx, sy, sw, sh );
}

static int g_nCachedScreenSize[ 2 ] = { -1, -1 };

void VGui_OnScreenSizeChanged()
{
	vgui::surface()->GetScreenSize( g_nCachedScreenSize[ 0 ], g_nCachedScreenSize[ 1 ] );
}

void VGui_GetPanelBounds( int &x, int &y, int &w, int &h )
{
	x = y = 0;
	vgui::surface()->GetScreenSize( w, h );
}

void VGui_GetEngineRenderBounds( int &x, int &y, int &w, int &h, int &insetX, int &insetY )
{
	insetX = insetY = 0;

	x = y = 0;
	vgui::surface()->GetScreenSize( w, h );
}

void VGui_GetHudBounds( int &x, int &y, int &w, int &h )
{
	x = y = 0;
	vgui::surface()->GetScreenSize( w, h );
}

int VGUI_FindSlotForRootPanel( vgui::Panel *pRoot )
{
	CUtlVector< Panel * > list;
	VGui_GetPanelList( list );
	int slot =  list.Find( pRoot ) ;
	if ( slot == list.InvalidIndex() )
		return 0;
	return slot;
}
