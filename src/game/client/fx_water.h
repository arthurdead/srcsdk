//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_WATER_H
#define FX_WATER_H
#pragma once

#include "particles_simple.h"
#include "fx.h"

class CSplashParticle : public CSimpleEmitter
{
public:
	
	CSplashParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ), m_bUseClipHeight( false ) {}
	
	// Create
	static CSplashParticle *Create( const char *pDebugName );

	// Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta );

	// Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );

	// Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle );

	void SetClipHeight( float flClipHeight );

	// Simulation
	void SimulateParticles( CParticleSimulateIterator *pIterator );

private:
	CSplashParticle( const CSplashParticle & );
	
	float	m_flClipHeight;
	bool	m_bUseClipHeight;
};

class WaterDebrisEffect : public CSimpleEmitter
{
public:
	WaterDebrisEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static WaterDebrisEffect* Create( const char *pDebugName );

	virtual float UpdateAlpha( const SimpleParticle *pParticle );

private:
	WaterDebrisEffect( const WaterDebrisEffect & );
};

extern void FX_WaterRipple( const Vector &origin, float scale, Vector *pColor, float flLifetime=1.5, float flAlpha=1 );
extern void FX_GunshotSplash( const Vector &origin, const Vector &normal, float scale );
extern void FX_GunshotSlimeSplash( const Vector &origin, const Vector &normal, float scale );

//-----------------------------------------------------------------------------
// Purpose: Retrieve and alter lighting for splashes
// Input  : position - point to check
//			*color - tint of the lighting at this point
//			*luminosity - adjusted luminosity at this point
//-----------------------------------------------------------------------------
void FX_GetSplashLighting( const Vector &position, Vector *color, float *luminosity );

#endif // FX_WATER_H
