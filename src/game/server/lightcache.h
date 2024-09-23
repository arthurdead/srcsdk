#ifndef LIGHTCACHE_H
#define LIGHTCACHE_H

#pragma once

#include "mathlib/vector.h"

//- lighting ----------------------------------------------------------------------------------------
float GetLightIntensity( const Vector &pos );			// returns a 0..1 light intensity for the given point

#endif
