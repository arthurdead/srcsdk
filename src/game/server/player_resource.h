//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYER_RESOURCE_H
#define PLAYER_RESOURCE_H
#pragma once

#include "shareddefs.h"
#include "baseentity.h"

class CPlayerResource : public CLogicalEntity
{
	DECLARE_CLASS( CPlayerResource, CLogicalEntity );
public:
	DECLARE_SERVERCLASS();

	CPlayerResource();

	virtual void Spawn( void );
	virtual void ResourceThink( void );
	virtual void UpdatePlayerData( void );
	virtual void			UpdateOnRemove( void );

protected:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	CNetworkArray( int, m_iPing, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iScore, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iDeaths, MAX_PLAYERS+1 );
	CNetworkArray( int, m_bConnected, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iTeam, MAX_PLAYERS+1 );
	CNetworkArray( int, m_bAlive, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iHealth, MAX_PLAYERS+1 );
		
	int	m_nUpdateCounter;
};

extern CPlayerResource *g_pPlayerResource;

#endif // PLAYER_RESOURCE_H
