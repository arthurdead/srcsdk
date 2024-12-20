//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// Particle system entities can derive from this to handle some of the mundane
// functionality of hooking into the engine's entity system.

#ifndef PARTICLE_BASEEFFECT_H
#define PARTICLE_BASEEFFECT_H

#pragma once

#include "predictable_entity.h"
#include "baseentity_shared.h"

#if defined( CLIENT_DLL )
class C_BaseParticleEntity;
typedef C_BaseParticleEntity CSharedBaseParticleEntity;

#include "particlemgr.h"

#else
class CBaseParticleEntity;
typedef CBaseParticleEntity CSharedBaseParticleEntity;
#endif 

#if defined( CLIENT_DLL )
	#define CBaseParticleEntity C_BaseParticleEntity
#endif

class CBaseParticleEntity : public CSharedBaseEntity
#if defined( CLIENT_DLL )
, public IParticleEffect
#endif
{
public:
	DECLARE_CLASS( CBaseParticleEntity, CSharedBaseEntity );
	CBaseParticleEntity();
	virtual ~CBaseParticleEntity();
private:
	CBaseParticleEntity( const CBaseParticleEntity & ); // not defined, not accessible
public:

#if defined( CLIENT_DLL )
	#undef CBaseParticleEntity
#endif

	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	// CBaseEntity overrides.
public:
#if !defined( CLIENT_DLL )
	virtual EdictStateFlags_t		UpdateTransmitState( void );	
#else
// Default IParticleEffect overrides.
public:

	virtual bool	ShouldSimulate() const { return m_bSimulate; }
	virtual void	SetShouldSimulate( bool bSim ) { m_bSimulate = bSim; }

	virtual void	SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	RenderParticles( CParticleRenderIterator *pIterator );
	virtual const Vector & GetSortOrigin();
public:
	CParticleEffectBinding	m_ParticleEffect;
#endif

	virtual void	Activate();
	virtual void	Think();	

#if defined( CLIENT_DLL )
	// NOTE: Ths enclosed particle effect binding will do all the drawing
	virtual bool	ShouldDraw() { return false; }

	int				AllocateToolParticleEffectId();
	int				GetToolParticleEffectId() const;

private:
	int				m_nToolParticleEffectId;
	bool			m_bSimulate;
#endif

public:
	void			FollowEntity(CSharedBaseEntity *pEntity);
	
	// UTIL_Remove will be called after the specified amount of time.
	// If you pass in -1, the entity will never go away automatically.
	void			SetLifetime(float lifetime);
};


#if defined( CLIENT_DLL )

inline int C_BaseParticleEntity::GetToolParticleEffectId() const
{
	return m_nToolParticleEffectId;
}

inline int C_BaseParticleEntity::AllocateToolParticleEffectId()
{
	m_nToolParticleEffectId = ParticleMgr()->AllocateToolParticleEffectId();
	return m_nToolParticleEffectId;
}

#endif // CLIENT_DLL

#endif


