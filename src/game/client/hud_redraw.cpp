//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
//
// hud_redraw.cpp
//
#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "itextmessage.h"
#include "vgui_basepanel.h"
#include "hud_crosshair.h"
#include <vgui/ISurface.h>

#if defined( REPLAY_ENABLED )
#include "replay/ienginereplay.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//For progress bar orientations
const int CHud::HUDPB_HORIZONTAL = 0;
const int CHud::HUDPB_VERTICAL = 1;
const int CHud::HUDPB_HORIZONTAL_INV = 2;

// Called when a ConVar changes value
static void FovChanged_Callback( IConVarRef pConVar, const char *pOldString, float flOldValue )
{
	if ( engine->IsInGame() )
	{
		engine->ServerCmd( VarArgs( "fov %f\n", pConVar.GetFloat() ) );
	}
}

static ConVar fov_watcher( "_fov", "0", 0, "Automates fov command to server.", FovChanged_Callback );

//-----------------------------------------------------------------------------
// Purpose: Think
//-----------------------------------------------------------------------------
void CHud::Think(void)
{
#if defined( REPLAY_ENABLED )
	// Don't draw this
	extern IEngineClientReplay *g_EngineClientReplay;
	const bool bPlayingReplay = g_pEngineClientReplay && g_pEngineClientReplay->IsPlayingReplayDemo();
#endif

	CUtlVector< CHudElement * > & list = GetHudList();
	CUtlVector< vgui::Panel * > & hudPanelList = GetHudPanelList();

	int c = list.Count();
	Assert( c == hudPanelList.Count() );

	m_bEngineIsInGame = engine->IsInGame() && ( engine->IsLevelMainMenuBackground() == false );

	// Determine the visibility of all hud elements
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		CHudElement *element = list[i];
		vgui::Panel *pPanel = hudPanelList[ i ];

		// Visible?
		bool visible = element->ShouldDraw();

#if defined( REPLAY_ENABLED )
		visible = visible && !bPlayingReplay;
#endif

		element->SetActive( visible );

		// If it's a vgui panel, hide/show as appropriate
		if ( pPanel && pPanel->IsVisible() != visible )
		{
			pPanel->SetVisible( visible );
		}
		else if ( !pPanel )
		{
			// All HUD elements should now derive from vgui!!!
			Assert( 0 );
		}

		if ( visible )
		{
			element->ProcessInput();
		}
	}

	// Let the active weapon at the keybits
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->HandleInput();
		}
	}

	if ( ( m_flScreenShotTime > 0 ) && ( m_flScreenShotTime < gpGlobals->curtime ) )
	{
		engine->ClientCmd( "screenshot" );

		m_flScreenShotTime = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:  The percentage passed in is expected and clamped to 0.0f to 1.0f
// Input  : x - 
//			y - 
//			width - 
//			height - 
//			percentage - 
//			clr - 
//			type - 
//-----------------------------------------------------------------------------
void CHud::DrawProgressBar( int x, int y, int width, int height, float percentage, Color& clr, unsigned char type )
{
	//Clamp our percentage
	percentage = MIN( 1.0f, percentage );
	percentage = MAX( 0.0f, percentage );

	Color lowColor;
	lowColor.SetColor( clr.r() / 2, clr.g() / 2, clr.b() / 2, clr.a() );

	//Draw a vertical progress bar
	if ( type == HUDPB_VERTICAL )
	{
		int	barOfs = height * percentage;

		surface()->DrawSetColor( lowColor );
		surface()->DrawFilledRect( x, y, x + width, y + barOfs );

		surface()->DrawSetColor( clr );
		surface()->DrawFilledRect( x, y + barOfs, x + width, y + height );
	}
	else if ( type == HUDPB_HORIZONTAL )
	{
		int	barOfs = width * percentage;

		surface()->DrawSetColor( lowColor );
		surface()->DrawFilledRect( x, y, x + barOfs, y +  height );

		surface()->DrawSetColor( clr );
		surface()->DrawFilledRect( x + barOfs, y, x + width, y + height );
	}
	else if ( type == HUDPB_HORIZONTAL_INV )
	{
		int	barOfs = width * percentage;

		surface()->DrawSetColor( clr );
		surface()->DrawFilledRect( x, y, x + barOfs, y + height );

		surface()->DrawSetColor( lowColor );
		surface()->DrawFilledRect( x + barOfs, y, x + width, y +  height );
	}
}

//-----------------------------------------------------------------------------
// Purpose:  The percentage passed in is expected and clamped to 0.0f to 1.0f
// Input  : x - 
//			y - 
//			*icon - 
//			percentage - 
//			clr - 
//			type - 
//-----------------------------------------------------------------------------
void CHud::DrawIconProgressBar( int x, int y, CHudTexture *icon, CHudTexture *icon2, float percentage, Color& clr, int type )
{
	if ( icon == NULL )
		return;

	//Clamp our percentage
	percentage = MIN( 1.0f, percentage );
	percentage = MAX( 0.0f, percentage );

	int	height = icon->Height();
	int	width  = icon->Width();

	//Draw a vertical progress bar
	if ( type == HUDPB_VERTICAL )
	{
		int	barOfs = height * percentage;

		icon2->DrawSelfCropped( 
			x, y,  // Pos
			0, 0, width, barOfs, // Cropped subrect
			clr );

		icon->DrawSelfCropped( 
			x, y + barOfs, 
			0, barOfs, width, height - barOfs, // Cropped subrect
			clr );
	}
	else if ( type == HUDPB_HORIZONTAL )
	{
		int	barOfs = width * percentage;

		icon2->DrawSelfCropped( 
			x, y,  // Pos
			0, 0, barOfs, height, // Cropped subrect
			clr );

		icon->DrawSelfCropped( 
			x + barOfs, y, 
			barOfs, 0, width - barOfs, height, // Cropped subrect
			clr );
	}
}


