#ifndef IGAMECLIENTLOOPBACK_H
#define IGAMECLIENTLOOPBACK_H

#pragma once

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"
#include "mathlib/vector.h"

#ifndef SWDS

#define GAMECLIENTLOOPBACK_INTERFACE_VERSION "IGameClientLoopback001"

//-----------------------------------------------------------------------------
// Lightcache entry handle
//-----------------------------------------------------------------------------
FORWARD_DECLARE_HANDLE( LightCacheHandle_t );

abstract_class IGameClientLoopback : public IAppSystem
{
public:
	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	// If pBoxColors is specified (it's an array of 6), then it'll copy the light contribution at each box side.
	virtual void		ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color, Vector *pBoxColors=NULL ) = 0;

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void		ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color ) = 0;

	// Get the lighting intensivty for a specified point
	// If bClamp is specified, the resulting Vector is restricted to the 0.0 to 1.0 for each element
	virtual Vector				GetLightForPoint(const Vector &pos, bool bClamp) = 0;

	// Just get the leaf ambient light - no caching, no samples
	virtual Vector			GetLightForPointFast(const Vector &pos, bool bClamp) = 0;

	// Returns the color of the ambient light
	virtual void		GetAmbientLightColor( Vector& color ) = 0;
};

#endif

#endif
