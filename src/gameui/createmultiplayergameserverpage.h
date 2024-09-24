//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMESERVERPAGE_H
#define CREATEMULTIPLAYERGAMESERVERPAGE_H
#pragma once

#include <vgui_controls/PropertyPage.h>
#include "gameui_cvartogglecheckbutton.h"

//-----------------------------------------------------------------------------
// Purpose: server options page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameServerPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CCreateMultiplayerGameServerPage, vgui::PropertyPage );

public:
	CCreateMultiplayerGameServerPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameServerPage();

	// returns currently entered information about the server
	void SetMap(const char *name);
	bool IsRandomMapSelected();
	const char *GetMapName();

	// CS Bots
	void EnableBots( KeyValues *data );
	int GetBotQuota( void );
	bool GetBotsEnabled( void );

protected:
	virtual void OnApplyChanges();
	MESSAGE_FUNC( OnCheckButtonChecked, "CheckButtonChecked" );

private:
	void LoadMapList();
	void LoadMaps( const char *pszPathID );

	vgui::ComboBox *m_pMapList;
	vgui::CheckButton *m_pEnableBotsCheck;
	CGameUICvarToggleCheckButton *m_pEnableTutorCheck;
	KeyValues *m_pSavedData;

	enum { DATA_STR_LENGTH = 64 };
	char m_szMapName[DATA_STR_LENGTH];
};


#endif // CREATEMULTIPLAYERGAMESERVERPAGE_H
