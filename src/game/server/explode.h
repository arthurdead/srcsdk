//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef EXPLODE_H
#define EXPLODE_H

#pragma once

#include "mathlib/vector.h"
#include "ehandle.h"
#include "shareddefs.h"
#include "takedamageinfo.h"

class CBaseEntity;

enum SFEnvExplosion_t : unsigned int
{
	SF_ENVEXPLOSION_NODAMAGE =	0x00000001, // when set, ENV_EXPLOSION will not actually inflict damage
	SF_ENVEXPLOSION_REPEATABLE =	0x00000002, // can this entity be refired?
	SF_ENVEXPLOSION_NOFIREBALL =	0x00000004, // don't draw the fireball
	SF_ENVEXPLOSION_NOSMOKE =		0x00000008, // don't draw the smoke
	SF_ENVEXPLOSION_NODECAL =		0x00000010, // don't make a scorch mark
	SF_ENVEXPLOSION_NOSPARKS =	0x00000020, // don't make sparks
	SF_ENVEXPLOSION_NOSOUND =		0x00000040, // don't play explosion sound.
	SF_ENVEXPLOSION_RND_ORIENT =	0x00000080,	// randomly oriented sprites
	SF_ENVEXPLOSION_NOFIREBALLSMOKE = 0x0100,
	SF_ENVEXPLOSION_NOPARTICLES = 0x00000200,
	SF_ENVEXPLOSION_NODLIGHTS =	0x00000400,
	SF_ENVEXPLOSION_NOCLAMPMIN =	0x00000800, // don't clamp the minimum size of the fireball sprite
	SF_ENVEXPLOSION_NOCLAMPMAX =	0x00001000, // don't clamp the maximum size of the fireball sprite
	SF_ENVEXPLOSION_SURFACEONLY =	0x00002000, // don't damage the player if he's underwater.
	SF_ENVEXPLOSION_GENERIC_DAMAGE =	0x00004000, // don't do BLAST damage
	SF_ENVEXPLOSION_ICE =			0x00008000, // freeze stuff and do ice type effects
};

FLAGENUM_OPERATORS( SFEnvExplosion_t, unsigned int )

extern modelindex_t g_sModelIndexFireball;
extern modelindex_t g_sModelIndexSmoke;

void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, float flExplosionForce = 0.0f, bool bSurfaceOnly = false, bool bSilent = false, DamageTypes_t iCustomDamageType = DMG_INVALID );

void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, SFEnvExplosion_t nSpawnFlags, 
					 float flExplosionForce = 0.0f, CBaseEntity *pInflictor = NULL, DamageTypes_t iCustomDamageType = DMG_INVALID,  const EHANDLE *ignoredEntity = NULL, Class_T ignoredClass = CLASS_NONE);

// this version lets you specify classes or entities to be ignored
void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, 
					 const EHANDLE *ignoredEntity, Class_T ignoredClass,
					 float flExplosionForce = 0.0f, bool bSurfaceOnly = false, bool bSilent = false, DamageTypes_t iCustomDamageType = DMG_INVALID );

#endif			//EXPLODE_H
