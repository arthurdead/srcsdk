#ifndef LIGHTCACHE_H
#define LIGHTCACHE_H

#pragma once

#include "mathlib/vector.h"

//- lighting ----------------------------------------------------------------------------------------
float GetLightIntensity( int playerIndex, const Vector &pos );			// returns a 0..1 light intensity for the given point
void UpdateLightIntensity( int playerIndex, const Vector &pos );
void SaveLightIntensity();

#endif
