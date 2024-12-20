//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERSTATE_GAME_H
#define PLAYERSTATE_GAME_H
#pragma once

#include "edict.h"
#include "networkvar.h"
// Only care about this stuff in game/client .dlls
#if defined( CLIENT_DLL )
#include "predictable_entity.h"
#endif
#ifdef GAME_DLL
#include "PlayerStateEngine.h"
#endif

class CPlayerStateGame
#ifdef GAME_DLL
	 : public CPlayerStateEngine, public INetworkableObject
#endif
{
public:
#ifdef GAME_DLL
	DECLARE_CLASS( CPlayerStateGame, CPlayerStateEngine );
	DECLARE_EMBEDDED_NETWORKVAR();
#else
	DECLARE_CLASS_NOBASE( CPlayerStateGame )
#endif

#ifdef GAME_DLL
	void NetworkStateChanged( void *pProp ) override
	{
		// Make sure it's a semi-reasonable pointer.
		Assert( ((char*)pProp - (char*)this) >= 0 );
		Assert( ((char*)pProp - (char*)this) < 32768 );

		// Good, they passed an offset so we can track this variable's change
		// and avoid sending the whole entity.
		NetworkStateChanged( ((char*)pProp - (char*)this) );
	}
#endif

	// This virtual method is necessary to generate a vtable in all cases
	// (DECLARE_PREDICTABLE will generate a vtable also)!
	virtual ~CPlayerStateGame() {}

	// true if the player is dead
#ifdef GAME_DLL
	CNetworkVarLocated( bool, deadflag, CPlayerStateEngine, deadflag_DO_NOT_USE );
#else
	CNetworkVar( bool, deadflag );
#endif

#ifdef CLIENT_DLL
	// Viewing angle (player only)
	QAngle		v_angle;		
#endif

#if defined( CLIENT_DLL )
	DECLARE_PREDICTABLE();
#endif
};

#endif // PLAYERSTATE_H
