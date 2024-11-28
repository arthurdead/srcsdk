//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMECONSOLE_H
#define GAMECONSOLE_H
#pragma once

#include "GameUI/IGameConsole.h"
#include "vgui_controls/consoledialog.h"
#include <Color.h>
#include "utlvector.h"
#include "vgui_controls/Frame.h"

//-----------------------------------------------------------------------------
// Purpose: Game/dev console dialog
//-----------------------------------------------------------------------------
class CGameConsoleDialog : public vgui::CConsoleDialog
{
	DECLARE_CLASS_SIMPLE( CGameConsoleDialog, vgui::CConsoleDialog );

public:
	CGameConsoleDialog();

private:
	MESSAGE_FUNC( OnClosedByHittingTilde, "ClosedByHittingTilde" );
	MESSAGE_FUNC_CHARPTR( OnCommandSubmitted, "CommandSubmitted", command );

	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	virtual void OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: VGui implementation of the game/dev console
//-----------------------------------------------------------------------------
class CGameConsole : public IGameConsole
{
public:
	CGameConsole();
	~CGameConsole();

#ifdef __MINGW32__
private:
	void __DTOR__();
#endif

public:
	// sets up the console for use
	void Initialize();

	// activates the console, makes it visible and brings it to the foreground
	virtual void Activate();
	// hides the console
	virtual void Hide();
	// clears the console
	virtual void Clear();

	// returns true if the console is currently in focus
	virtual bool IsConsoleVisible();

	// activates the console after a delay
	void ActivateDelayed(float time);

	void SetParent( vgui::VPANEL parent );

	static void OnCmdCondump();

private:
	bool m_bInitialized;
	CGameConsoleDialog *m_pConsole;
};

extern CGameConsole g_GameConsole;

#endif // GAMECONSOLE_H
