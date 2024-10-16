//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_PHYSICSPROP_H
#define C_PHYSICSPROP_H
#pragma once

#include "c_breakableprop.h"
#include "props_shared.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PhysicsProp : public C_BreakableProp, public ISpecialPhysics
{
	DECLARE_CLASS( C_PhysicsProp, C_BreakableProp );
public:
	DECLARE_CLIENTCLASS();

	// Inherited from IClientUnknown
public:
	virtual IClientModelRenderable*	GetClientModelRenderable();

	// Inherited from IClientModelRenderable
public:
	virtual bool GetRenderData( void *pData, ModelDataCategory_t nCategory );

	// Other public methods
public:
	C_PhysicsProp();
	virtual ~C_PhysicsProp();

	virtual void OnDataChanged( DataUpdateType_t type );
	virtual bool OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );

	bool GetPropDataAngles( const char *pKeyName, QAngle &vecAngles );
	float GetCarryDistanceOffset( void );

	virtual int GetPhysicsMode()
	{
		Assert( m_iPhysicsMode != PHYSICS_CLIENTSIDE );
		Assert( m_iPhysicsMode != PHYSICS_AUTODETECT );
		return m_iPhysicsMode;
	}

	virtual float GetMass()
	{
		return m_fMass;
	}

	virtual bool IsAsleep()
	{
		return !m_bAwake;
	}

	virtual void ComputeWorldSpaceSurroundingBox( Vector *mins, Vector *maxs );

protected:
	// Networked vars.
	bool m_bAwake;
	bool m_bAwakeLastTime;
	bool m_bCanUseStaticLighting;

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float, m_fMass );
	CNetworkVector( m_collisionMins );
	CNetworkVector( m_collisionMaxs );
};

typedef C_PhysicsProp CSharedPhysicsProp;

#endif // C_PHYSICSPROP_H 
