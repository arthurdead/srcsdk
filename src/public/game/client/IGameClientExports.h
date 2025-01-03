//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMECLIENTEXPORTS_H
#define IGAMECLIENTEXPORTS_H
#pragma once

#include "interface.h"

namespace vgui
{
	class Panel;
}

//-----------------------------------------------------------------------------
// Purpose: Exports a set of functions for the GameUI interface to interact with the game client
//-----------------------------------------------------------------------------
abstract_class IGameClientExports : public IBaseInterface
{
public:
	// ingame voice manipulation
	virtual bool IsPlayerGameVoiceMuted(int playerIndex) = 0;
	virtual void MutePlayerGameVoice(int playerIndex) = 0;
	virtual void UnmutePlayerGameVoice(int playerIndex) = 0;

	// notification of gameui state changes
	virtual void OnGameUIActivated() = 0;
	virtual void OnGameUIHidden() = 0;

	//=============================================================================
	// HPE_BEGIN
	// [dwenger] Necessary for stats display
	//=============================================================================

	virtual void CreateAchievementsPanel( vgui::Panel* pParent ) = 0;
	virtual void DisplayAchievementPanel( ) = 0;
	virtual void ShutdownAchievementPanel( ) = 0;
	virtual int GetAchievementsPanelMinWidth( void ) const = 0;

	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual const char *GetHolidayString() = 0;
};

abstract_class IGameClientExportsEx : public IGameClientExports
{
public:
	// if true, the gameui applies the blur effect
	virtual bool ClientWantsBlurEffect( void ) = 0;
};

#define GAMECLIENTEXPORTS_INTERFACE_VERSION "GameClientExports001"
#define GAMECLIENTEXPORTS_EX_INTERFACE_VERSION "GameClientExportsEx001"

#endif // IGAMECLIENTEXPORTS_H
