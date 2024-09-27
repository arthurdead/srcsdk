//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMEBOTPAGE_H
#define CREATEMULTIPLAYERGAMEBOTPAGE_H
#pragma once

#include <vgui_controls/PropertyPage.h>
#include "gameui_cvartogglecheckbutton.h"

class CDescription;
class mpcontrol_t;

//-----------------------------------------------------------------------------
// Purpose: advanced bot properties page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameBotPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CCreateMultiplayerGameBotPage, vgui::PropertyPage );

public:
	CCreateMultiplayerGameBotPage( vgui::Panel *parent, const char *name, KeyValues *botKeys );
	~CCreateMultiplayerGameBotPage();

protected:
	virtual void OnResetChanges();
	virtual void OnApplyChanges();

private:
	CGameUICvarToggleCheckButton *m_joinAfterPlayer;

	CGameUICvarToggleCheckButton *m_allowRogues;

	CGameUICvarToggleCheckButton *m_allowPistols;
	CGameUICvarToggleCheckButton *m_allowShotguns;
	CGameUICvarToggleCheckButton *m_allowSubmachineGuns;
	CGameUICvarToggleCheckButton *m_allowMachineGuns;
	CGameUICvarToggleCheckButton *m_allowRifles;
	CGameUICvarToggleCheckButton *m_allowGrenades;
#ifdef CS_SHIELD_ENABLED
	CGameUICvarToggleCheckButton *m_allowShields;
#endif // CS_SHIELD_ENABLED
	CGameUICvarToggleCheckButton *m_allowSnipers;

	CGameUICvarToggleCheckButton *m_deferToHuman;

	vgui::ComboBox *m_joinTeamCombo;
	void SetJoinTeamCombo( const char *team );

	vgui::ComboBox *m_chatterCombo;
	void SetChatterCombo( const char *team );

	vgui::TextEntry *m_prefixEntry;

	KeyValues *m_pSavedData;
};


#endif // CREATEMULTIPLAYERGAMEBOTPAGE_H
