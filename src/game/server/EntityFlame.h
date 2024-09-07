//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYFLAME_H
#define ENTITYFLAME_H
#pragma once

#include "baseentity.h"
#include "ai_localnavigator.h"

#define FLAME_DAMAGE_INTERVAL			0.2f // How often to deal damage.
#define FLAME_DIRECT_DAMAGE_PER_SEC		5.0f
#define FLAME_RADIUS_DAMAGE_PER_SEC		4.0f

#define FLAME_DIRECT_DAMAGE ( FLAME_DIRECT_DAMAGE_PER_SEC * FLAME_DAMAGE_INTERVAL )
#define FLAME_RADIUS_DAMAGE ( FLAME_RADIUS_DAMAGE_PER_SEC * FLAME_DAMAGE_INTERVAL )

#define FLAME_MAX_LIFETIME_ON_DEAD_NPCS	10.0f

class CEntityFlame : public CBaseEntity 
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CEntityFlame, CBaseEntity );

	CEntityFlame( void );

	static CEntityFlame	*Create( CBaseEntity *pTarget, float flLifetime, float flSize, bool useHitboxes = true );
	static CEntityFlame	*Create( CBaseEntity *pTarget, float flLifetime )
	{ return Create(pTarget, flLifetime, 0.0f, true); }
	static CEntityFlame	*Create( CBaseEntity *pTarget, float flLifetime, bool useHitboxes )
	{ return Create(pTarget, flLifetime, 0.0f, useHitboxes); }
	static CEntityFlame	*Create( CBaseEntity *pTarget, bool useHitboxes )
	{ return Create(pTarget, 0.0f, 0.0f, useHitboxes); }
	static CEntityFlame	*Create( CBaseEntity *pTarget )
	{ return Create(pTarget, 0.0f, 0.0f, true); }

	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetLifetime( float lifetime );
	void	SetUseHitboxes( bool use );
	void	SetNumHitboxFires( int iNumHitBoxFires );
	void	SetHitboxFireScale( float flHitboxFireScale );

	float	GetRemainingLife( void ) const;
	int		GetNumHitboxFires( void );
	float	GetHitboxFireScale( void );

	virtual void Precache();
	virtual void UpdateOnRemove();
	virtual void Spawn();
	virtual void Activate();

	void	SetSize( float size ) { m_flSize = size; }

	void	UseCheapEffect( bool bCheap );

	void	SetAttacker( CBaseEntity *pAttacker ) { m_hAttacker = pAttacker; }
	CBaseEntity *GetAttacker( void ) const;

	DECLARE_MAPENTITY();

protected:

	void InputIgnite( inputdata_t &inputdata );

	void	FlameThink( void );

	CNetworkHandle( CBaseEntity, m_hEntAttached );		// The entity that we are burning (attached to).
	CNetworkVar( bool, m_bCheapEffect );

	CNetworkVar( float, m_flSize );
	CNetworkVar( bool, m_bUseHitboxes );
	CNetworkVar( int, m_iNumHitboxFires );
	CNetworkVar( float, m_flHitboxFireScale );

	CNetworkVar( float, m_flLifetime );
	string_t	m_iszPlayingSound;	// Track the sound so we can StopSound later

	EHANDLE m_hAttacker;

	int m_iDangerSound;
	bool	m_bPlayingSound;
	Obstacle_t m_hObstacle;
};

#endif // ENTITYFLAME_H
