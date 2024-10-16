//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_BLOOD_H
#define FX_BLOOD_H
#pragma once

#include "particles_simple.h"

class CBloodSprayEmitter : public CSimpleEmitter
{
public:
	
	CBloodSprayEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	static CBloodSprayEmitter *Create( const char *pDebugName );

	inline void SetGravity( float flGravity )
	{
		m_flGravity = flGravity;
	}

	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta );

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );

private:

	float m_flGravity;

	CBloodSprayEmitter( const CBloodSprayEmitter & );
};

#endif // FX_BLOOD_H
