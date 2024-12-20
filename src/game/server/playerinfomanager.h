//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implementation of player info manager
//
//=============================================================================//
#ifndef PLAYERINFOMANAGER_H
#define PLAYERINFOMANAGER_H
#pragma once


#include "game/server/iplayerinfo.h"

//-----------------------------------------------------------------------------
// Purpose: interface for plugins to get player info
//-----------------------------------------------------------------------------
class CPlayerInfoManager: public IPlayerInfoManager
{
public:
	virtual IPlayerInfo *GetPlayerInfo( edict_t *pEdict );
	virtual IPlayerInfo *GetPlayerInfo( int index );
	virtual CGlobalVars *GetGlobalVars();
	// accessor to hook into aliastoweaponid
	virtual int			AliasToWeaponId(const char *weaponName);
	// accessor to hook into weaponidtoalias
	virtual const char *WeaponIdToAlias(int weaponId);
};

#endif