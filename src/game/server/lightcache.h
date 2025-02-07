#ifndef LIGHTCACHE_H
#define LIGHTCACHE_H

#pragma once

#include "mathlib/vector.h"

class CBaseEntity;

//- lighting ----------------------------------------------------------------------------------------
float GetLightIntensity( int playerIndex, const Vector &pos );			// returns a 0..1 light intensity for the given point
float GetLightIntensity( const CBaseEntity *pTarget, const Vector &pos );
float GetLightIntensity( const Vector &pos );
void UpdateLightIntensity( const Vector &pos );
void UpdateLightIntensity( int playerIndex, const Vector &pos );
void UpdateLightIntensity( const CBaseEntity *pTarget, const Vector &pos );
void SaveLightIntensity();

#endif
