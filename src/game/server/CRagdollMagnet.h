//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Used to influence the initial force for a dying NPC's ragdoll. 
//			Passive entity. Just represents position in the world, radius, force
//
// $NoKeywords: $
//=============================================================================//
#ifndef CRAGDOLLMAGNET_H
#define CRAGDOLLMAGNET_H
#pragma once

#include "baseentity.h"

#define SF_RAGDOLLMAGNET_BAR	0x00000002	// this is a bar magnet.

class CRagdollMagnet : public CPointEntity
{
public:
	DECLARE_CLASS( CRagdollMagnet, CPointEntity );
	DECLARE_MAPENTITY();

	Vector GetForceVector( CBaseEntity *pNPC, int *pBone = NULL );
	float GetRadius( void ) { return m_radius; }
	Vector GetAxisVector( void ) { return m_axis - GetAbsOrigin(); }
	float DistToPoint( const Vector &vecPoint );

	bool IsEnabled( void ) { return !m_bDisabled; }
	
	int IsBarMagnet( void ) { return (m_spawnflags & SF_RAGDOLLMAGNET_BAR); }

	static CRagdollMagnet *FindBestMagnet( CBaseEntity *pNPC );

	void Enable( bool bEnable ) { m_bDisabled = !bEnable; }

	// Inputs
	void InputEnable( inputdata_t &&inputdata );
	void InputDisable( inputdata_t &&inputdata );

	const char	*BoneTarget() { return STRING(m_BoneTarget); }

	COutputVector m_OnUsed;

private:
	bool	m_bDisabled;
	float	m_radius;
	float	m_force;
	Vector	m_axis;
	string_t	m_BoneTarget;
};

#endif //CRAGDOLLMAGNET_H