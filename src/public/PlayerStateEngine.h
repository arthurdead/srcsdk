//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERSTATE_ENGINE_H
#define PLAYERSTATE_ENGINE_H
#pragma once

#include "edict.h"
#include "networkvar.h"

class CPlayerStateEngine
{
public:
	friend class CPlayerStateGame;

	DECLARE_CLASS_NOBASE( CPlayerStateEngine );
	virtual void NetworkStateChanged() = 0;
	virtual void NetworkStateChanged( void *pProp ) = 0;
	
	// This virtual method is necessary to generate a vtable in all cases
	// (DECLARE_PREDICTABLE will generate a vtable also)!
	virtual ~CPlayerStateEngine() {}

protected:
	// true if the player is dead
	bool deadflag_DO_NOT_USE;	

public:
	// Viewing angle (player only)
	QAngle		v_angle;		
	
// The client .dll only cares about deadflag
//  the game and engine .dlls need to worry about the rest of this data
	// Player's network name
	string_t	netname;
	// 0:nothing, 1:force view angles, 2:add avelocity
	int			fixangle;
	// delta angle for fixangle == FIXANGLE_RELATIVE
	QAngle		anglechange;
	// flag to single the HLTV/Replay fake client, not transmitted
	bool		hltv;
	bool		replay;
	int			frags;
	int			deaths;
};

#endif // PLAYERSTATE_H
