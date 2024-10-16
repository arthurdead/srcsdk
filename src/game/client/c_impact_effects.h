//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_IMPACT_EFFECTS_H
#define C_IMPACT_EFFECTS_H
#pragma once

#include "particles_simple.h"

//-----------------------------------------------------------------------------
// Purpose: DustParticle emitter 
//-----------------------------------------------------------------------------
class CDustParticle : public CSimpleEmitter
{
public:
	
	CDustParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CDustParticle *Create( const char *pDebugName="dust" );

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta );

	//Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );

	//Alpha
	virtual float UpdateAlpha( const SimpleParticle *pParticle );

private:
	CDustParticle( const CDustParticle & ); // not defined, not accessible
};

void GetColorForSurface( trace_t *trace, Vector *color );

#endif // C_IMPACT_EFFECTS_H
