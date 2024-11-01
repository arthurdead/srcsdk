//===== Copyright © 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: common routines to operate on matchmaking sessions and members
// Assumptions: caller should include all required headers before including mm_helpers.h
//
//===========================================================================//

#ifndef __COMMON__MM_HELPERS_H_
#define __COMMON__MM_HELPERS_H_
#pragma once

#include "tier1/KeyValues.h"
#include "tier1/fmtstr.h"

typedef uint64 XUID;

//
// Contains inline functions to deal with common tasks involving matchmaking and sessions
//

inline KeyValues * SessionMembersFindPlayer( KeyValues *pSessionSettings, XUID xuidPlayer, KeyValues **ppMachine = NULL )
{
	if ( ppMachine )
		*ppMachine = NULL;

	if ( !pSessionSettings )
		return NULL;

	KeyValues *pMembers = pSessionSettings->FindKey( "Members" );
	if ( !pMembers )
		return NULL;

	int numMachines = pMembers->GetInt( "numMachines" );
	for ( int k = 0; k < numMachines; ++ k )
	{
		KeyValues *pMachine = pMembers->FindKey( CFmtStr( "machine%d", k ) );
		if ( !pMachine )
			continue;

		int numPlayers = pMachine->GetInt( "numPlayers" );
		for ( int j = 0; j < numPlayers; ++ j )
		{
			KeyValues *pPlayer = pMachine->FindKey( CFmtStr( "player%d", j ) );
			if ( !pPlayer )
				continue;

			if ( pPlayer->GetUint64( "xuid" ) == xuidPlayer )
			{
				if ( ppMachine )
					*ppMachine = pMachine;

				return pPlayer;
			}
		}
	}

	return NULL;
}

#endif // __COMMON__MM_HELPERS_H_

