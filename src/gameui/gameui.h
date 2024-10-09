//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the interface that the GameUI dll exports
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEUI_INTERFACE_H
#define GAMEUI_INTERFACE_H
#pragma once

#include "GameUI/IGameUI.h"
#include <vgui/IClientPanel.h>
#include "rmlcontext.h"

class CGameUI : public IGameUIEx, public vgui::IClientPanel
{
public:
	CGameUI();
	~CGameUI();

	// initialization/shutdown
	virtual void Initialize( CreateInterfaceFn appFactory );
	virtual void PostInit();

	// connect to other interfaces at the same level (gameui.dll/server.dll/client.dll)
	virtual void Connect( CreateInterfaceFn gameFactory );

	virtual void Start();
	virtual void Shutdown();
	virtual void RunFrame();

	// notifications
	virtual void OnGameUIActivated();
	virtual void OnGameUIHidden();

	virtual void OnLevelLoadingStarted(bool bShowProgressDialog);
	virtual void OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason);

	// level loading progress, returns true if the screen needs updating
	virtual bool UpdateProgressBar(float progress, const char *statusText);
	// Shows progress desc, returns previous setting... (used with custom progress bars )
	virtual bool SetShowProgressText( bool show );

	// !!!!!!!!!members added after "GameUI011" initial release!!!!!!!!!!!!!!!!!!!
	virtual void ShowNewGameDialog( int chapter );

	// inserts specified panel as background for level load dialog
	virtual void SetLoadingBackgroundDialog( vgui::VPANEL panel );

	// Bonus maps interfaces
	virtual void BonusMapUnlock( const char *pchFileName = NULL, const char *pchMapName = NULL );
	virtual void BonusMapComplete( const char *pchFileName = NULL, const char *pchMapName = NULL );
	virtual void BonusMapChallengeUpdate( const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest );
	virtual void BonusMapChallengeNames( char *pchFileName, char *pchMapName, char *pchChallengeName );
	virtual void BonusMapChallengeObjectives( int &iBronze, int &iSilver, int &iGold );
	virtual void BonusMapDatabaseSave( void );
	virtual int BonusMapNumAdvancedCompleted( void );
	virtual void BonusMapNumMedals( int piNumMedals[ 3 ] );

	virtual void OnConnectToServer(const char *game, int IP, int connectionPort, int queryPort);

	virtual void SetProgressOnStart();
	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure );

	virtual void OnConfirmQuit( void );

	virtual bool IsMainMenuVisible( void );

	// Client DLL is providing us with a panel that it wants to replace the main menu with
	virtual void SetMainMenuOverride( vgui::VPANEL panel );
	// Client DLL is telling us that a main menu command was issued, probably from its custom main menu panel
	virtual void SendMainMenuCommand( const char *pszCommand );

	virtual bool IsPanelVisible();

	virtual vgui::VPANEL GetVPanel() { return m_VPanel; }

	// straight interface to Panel functions
	virtual void Think();
	virtual void PerformApplySchemeSettings() {}
	virtual void PaintTraverse(bool forceRepaint, bool allowForce);
	virtual void Repaint();
	virtual vgui::VPANEL IsWithinTraverse(int x, int y, bool traversePopups) { return vgui::INVALID_VPANEL; }
	virtual void GetInset(int &top, int &left, int &right, int &bottom) {}
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1);
	virtual void OnChildAdded(vgui::VPANEL child) {}
	virtual void OnSizeChanged(int newWide, int newTall) {}

	virtual void InternalFocusChanged(bool lost) {}
	virtual bool RequestInfo(KeyValues *outputData);
	virtual void RequestFocus(int direction) {}
	virtual bool RequestFocusPrev(vgui::VPANEL existingPanel) { return false; }
	virtual bool RequestFocusNext(vgui::VPANEL existingPanel) { return false; }
	virtual void OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel);
	virtual vgui::VPANEL GetCurrentKeyFocus() { return vgui::INVALID_VPANEL; }
	virtual int GetTabPosition() { return -1; }

	// for debugging purposes
	virtual const char *GetName();
	virtual const char *GetClassName() { return "CGameUI"; }

	// get scheme handles from panels
	virtual vgui::HScheme GetScheme() { return vgui::INVALID_SCHEME; }
	// gets whether or not this panel should scale with screen resolution
	virtual bool IsProportional() { return false; }
	// auto-deletion
	virtual bool IsAutoDeleteSet() { return false; }
	// deletes this
	virtual void DeletePanel();

	// interfaces
	virtual void *QueryInterface(vgui::EInterfaceID id);

	// returns a pointer to the vgui controls baseclass Panel *
	virtual vgui::Panel *GetPanel() { return NULL; }

	// returns the name of the module this panel is part of
	virtual const char *GetModuleName();

	virtual void OnTick();

	void ShowLoadingBackgroundDialog();
	void HideLoadingBackgroundDialog();
	bool HasLoadingBackgroundDialog();

	// Engine wrappers for activating / hiding the gameUI
	void ActivateGameUI();
	void HideGameUI();

	// Toggle allowing the engine to hide the game UI with the escape key
	void PreventEngineHideGameUI();
	void AllowEngineHideGameUI();

	void PreventEngineShowGameUI();
	void AllowEngineShowGameUI();

	// state
	bool IsInLevel();
	bool IsInBackgroundLevel();

private:
	bool m_bActivatedUI;
	vgui::VPANEL m_VPanel;
	bool m_bNeedsRepaint;
};

extern CGameUI g_GameUI;

#endif // GAMEUI_INTERFACE_H
