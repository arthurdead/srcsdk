//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//

#ifndef _IPLAYERMANAGER_H_
#define _IPLAYERMANAGER_H_

#pragma once

#include "tier0/platform.h"

class IPlayer;
class IPlayerFriend;
class IPlayerLocal;
typedef uint64 XUID;

abstract_class IPlayerManager
{
public:
	//
	// EnableServersUpdate
	//	controls whether friends data is being updated in the background
	//
	virtual void EnableFriendsUpdate( bool bEnable ) = 0;

private:
	//
	// GetLocalPlayer
	//	returns a local player interface for a given controller index
	//
	virtual IPlayerLocal * DO_NOT_USE_GetLocalPlayer( int iController ) = 0;

public:
	IPlayerLocal * GetLocalPlayer()
	{ return DO_NOT_USE_GetLocalPlayer(0); }

	//
	// GetNumFriends
	//	returns number of friends discovered and for which data is available
	//
	virtual int GetNumFriends() = 0;

	//
	// GetFriend
	//	returns player interface to the given friend or NULL if friend not found or not available
	//
	virtual IPlayerFriend * GetFriendByIndex( int index ) = 0;
	virtual IPlayerFriend * GetFriendByXUID( XUID xuid ) = 0;

	//
	// FindPlayer
	//	returns player interface by player's XUID or NULL if player not found or not available
	//
	virtual IPlayer * FindPlayer( XUID xuid ) = 0;
};


#endif
