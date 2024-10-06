//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include <stdio.h>

#include "gameconsole.h"
#include "vgui/ISurface.h"

#include "KeyValues.h"
#include "vgui/VGUI.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Panel.h"
#include "convar.h"
#include "gameui.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui/KeyCode.h"
#include "loadingdialog.h"
#include "engineinterface.h"
#include "IGameUIFuncs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CGameConsole g_GameConsole;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameConsole, IGameConsole, GAMECONSOLE_INTERFACE_VERSION, g_GameConsole);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameConsole::CGameConsole()
{
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameConsole::~CGameConsole()
{
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: sets up the console for use
//-----------------------------------------------------------------------------
void CGameConsole::Initialize()
{
	m_pConsole = vgui::SETUP_PANEL( new CGameConsoleDialog() ); // we add text before displaying this so set it up now!

	// set the console to taking up most of the right-half of the screen
	int swide, stall;
	vgui::surface()->GetScreenSize(swide, stall);
	int offset = vgui::scheme()->GetProportionalScaledValue(16);

	m_pConsole->SetBounds(
		swide / 2 - (offset * 4),
		offset,
		(swide / 2) + (offset * 3),
		stall - (offset * 8));

	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: activates the console, makes it visible and brings it to the foreground
//-----------------------------------------------------------------------------
void CGameConsole::Activate()
{
	if (!m_bInitialized)
		return;

	vgui::surface()->RestrictPaintToSinglePanel(vgui::INVALID_VPANEL);
	m_pConsole->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: hides the console
//-----------------------------------------------------------------------------
void CGameConsole::Hide()
{
	if (!m_bInitialized)
		return;

	m_pConsole->Hide();
}

//-----------------------------------------------------------------------------
// Purpose: clears the console
//-----------------------------------------------------------------------------
void CGameConsole::Clear()
{
	if (!m_bInitialized)
		return;

	m_pConsole->Clear();
}


//-----------------------------------------------------------------------------
// Purpose: returns true if the console is currently in focus
//-----------------------------------------------------------------------------
bool CGameConsole::IsConsoleVisible()
{
	if (!m_bInitialized)
		return false;
	
	return m_pConsole->IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: activates the console after a delay
//-----------------------------------------------------------------------------
void CGameConsole::ActivateDelayed(float time)
{
	if (!m_bInitialized)
		return;

	m_pConsole->PostMessage(m_pConsole, new KeyValues("Activate"), time);
}

void CGameConsole::SetParent( vgui::VPANEL parent )
{
	if (!m_bInitialized)
		return;

	m_pConsole->SetParent( parent );
}

//-----------------------------------------------------------------------------
// Purpose: static command handler
//-----------------------------------------------------------------------------
void CGameConsole::OnCmdCondump()
{
	g_GameConsole.m_pConsole->DumpConsoleTextToFile();
}

CON_COMMAND( condump, "dump the text currently in the console to condumpXX.log" )
{
	g_GameConsole.OnCmdCondump();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameConsoleDialog::CGameConsoleDialog() : BaseClass( NULL, "GameConsole", false )
{
	AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: generic vgui command handler
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnCommand(const char *command)
{
	if ( !Q_stricmp( command, "Close" ) )
	{
		if ( g_GameUI.IsInBackgroundLevel() )
		{
			// Tell the engine we've hid the console, so that it unpauses the game
			// even though we're still sitting at the menu.
			engine->ClientCmd_Unrestricted( "unpause" );
		}
	}

	BaseClass::OnCommand(command);
}


//-----------------------------------------------------------------------------
// HACK: Allow F key bindings to operate even when typing in the text entry field
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnKeyCodeTyped(KeyCode code)
{
	BaseClass::OnKeyCodeTyped(code);

	// check for processing
	if ( m_pConsolePanel->TextEntryHasFocus() )
	{
		// HACK: Allow F key bindings to operate even here
		if ( code >= KEY_F1 && code <= KEY_F12 )
		{
			// See if there is a binding for the FKey
			const char *binding = gameuifuncs->GetBindingForButtonCode( code );
			if ( binding && binding[0] )
			{
				// submit the entry as a console commmand
				char szCommand[256];
				Q_strncpy( szCommand, binding, sizeof( szCommand ) );
				engine->ClientCmd_Unrestricted( szCommand );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Submits a command
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnCommandSubmitted( const char *pCommand )
{
	engine->ClientCmd_Unrestricted( pCommand );
}


//-----------------------------------------------------------------------------
// Submits a command
//-----------------------------------------------------------------------------
void CGameConsoleDialog::OnClosedByHittingTilde()
{
	if ( !g_GameUI.HasLoadingBackgroundDialog() )
	{
		if( g_GameUI.IsInLevel() )
			g_GameUI.HideGameUI();
		else
			g_GameUI.ActivateGameUI();
	}
	else
	{
	#if 0
		vgui::surface()->RestrictPaintToSinglePanel( LoadingDialog()->GetVPanel() );
	#endif
	}
}
