//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: provides an interface for dlls to query information about players from the game dll
//
//=============================================================================//
#ifndef IPLAYERINFO_H
#define IPLAYERINFO_H
#pragma once

#include "mathlib/vector.h"

class CUserCmd;
struct edict_t;
class CGlobalVars;

abstract_class IPlayerInfo
{
public:
	// returns the players name (UTF-8 encoded)
	virtual const char *GetName() = 0;
	// returns the userid (slot number)
	virtual int		GetUserID() = 0;
	// returns the string of their network (i.e Steam) ID
	virtual const char *GetNetworkIDString() = 0;
	// returns the team the player is on
	virtual int GetTeamIndex() = 0;
	// changes the player to a new team (if the game dll logic allows it)
	virtual void ChangeTeam( int iTeamNum ) = 0;
	// returns the number of kills this player has (exact meaning is mod dependent)
	virtual int	GetFragCount() = 0;
	// returns the number of deaths this player has (exact meaning is mod dependent)
	virtual int	GetDeathCount() = 0;
	// returns if this player slot is actually valid
	virtual bool IsConnected() = 0;
	// returns the armor/health of the player (exact meaning is mod dependent)
	virtual int	GetArmorValue() = 0;

	// extensions added to V2

	// various player flags
	virtual bool IsHLTV() = 0;
	virtual bool IsPlayer() = 0;
	virtual bool IsFakeClient() = 0;
	virtual bool IsDead() = 0;
	virtual bool IsInAVehicle() = 0;
	virtual bool IsObserver() = 0;

	// player position and size
	virtual const Vector GetAbsOrigin() = 0;
	virtual const QAngle GetAbsAngles() = 0;
	virtual const Vector GetPlayerMins() = 0;
	virtual const Vector GetPlayerMaxs() = 0;
	// the name of the weapon currently being carried
	virtual const char *GetWeaponName() = 0;
	// the name of the player model in use
	virtual const char *GetModelName() = 0;
	// current player health
	virtual const int GetHealth() = 0;
	// max health value
	virtual const int GetMaxHealth() = 0;
	// the last user input from this player
	virtual CUserCmd GetLastUserCommand() = 0;

	virtual bool IsReplay() = 0;
};


#define INTERFACEVERSION_PLAYERINFOMANAGER			"PlayerInfoManager002"
abstract_class IPlayerInfoManager
{
public:
	virtual IPlayerInfo *GetPlayerInfo( edict_t *pEdict ) = 0;
	virtual CGlobalVars *GetGlobalVars() = 0;
};


#endif // IPLAYERINFO_H
