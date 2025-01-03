//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef PARTICLE_PARSE_H
#define PARTICLE_PARSE_H
#pragma once

#include "utlvector.h"
#include "utlstring.h"
#include "ifilelist.h"
#include "irecipientfilter.h"

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

//-----------------------------------------------------------------------------
// Particle attachment methods
//-----------------------------------------------------------------------------
enum ParticleAttachment_t
{
	PATTACH_ABSORIGIN = 0,			// Create at absorigin, but don't follow
	PATTACH_ABSORIGIN_FOLLOW,		// Create at absorigin, and update to follow the entity
	PATTACH_CUSTOMORIGIN,			// Create at a custom origin, but don't follow
	PATTACH_CUSTOMORIGIN_FOLLOW,	// Create at a custom origin, follow relative position to specified entity
	PATTACH_POINT,					// Create on attachment point, but don't follow
	PATTACH_POINT_FOLLOW,			// Create on attachment point, and update to follow the entity
	PATTACH_EYES_FOLLOW,			// Create on eyes of the attached entity, and update to follow the entity

	PATTACH_WORLDORIGIN,			// Used for control points that don't attach to an entity

	PATTACH_ROOTBONE_FOLLOW,		// Create at the root bone of the entity, and update to follow

	PATTACH_BONE_FOLLOW,

	MAX_PATTACH_TYPES,
};

extern int GetAttachTypeFromString( const char *pszString );

#define PARTICLE_DISPATCH_FROM_ENTITY		(1<<0)
#define PARTICLE_DISPATCH_RESET_PARTICLES	(1<<1)

struct te_particle_effects_colors_t
{
	Vector m_vecColor1;
	Vector m_vecColor2;
};

struct te_particle_effects_control_point_t
{
	ParticleAttachment_t m_eParticleAttachment;
	Vector m_vecOffset;
};

//-----------------------------------------------------------------------------
// Particle parsing methods
//-----------------------------------------------------------------------------
// Parse the particle manifest file & register the effects within it
// Only needs to be called once per game, unless tools change particle definitions
void ParseParticleEffects( bool bLoadSheets, bool bPrecache );
void ParseParticleEffectsMap( const char *pMapName, bool bLoadSheets, IFileList *pFilesToReload = NULL ); 

// Get a list of the files inside the particle manifest file
void GetParticleManifest( CUtlVector<CUtlString>& list );

// Precaches standard particle systems (only necessary on server)
// Should be called once per level
void PrecacheStandardParticleSystems( );

class IFileList;
void ReloadParticleEffectsInList( IFileList *pFilesToReload );

//-----------------------------------------------------------------------------
// Particle spawning methods
//-----------------------------------------------------------------------------
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity = false, IRecipientFilter *filter = NULL );
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity = NULL, int iAttachmentPoint = -1, bool bResetAllParticlesOnEntity = false, IRecipientFilter *filter = NULL );
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CSharedBaseEntity *pEntity = NULL );
void DispatchParticleEffect( const char *pszParticleName, const Vector &vecOrigin, const QAngle &vecAngles, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity = NULL );
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CSharedBaseEntity *pEntity = NULL, bool bUseColor = false, const Vector& color1 = vec3_origin, const Vector& color2 = vec3_origin );
void DispatchParticleEffectLink( const char *pszParticleName, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity = NULL, CSharedBaseEntity *pOtherEntity = NULL, int iAttachmentPoint = -1, bool bResetAllParticlesOnEntity = false, int nSplitScreenPlayerSlot = -1 );
void DispatchParticleEffect( int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CSharedBaseEntity *pEntity = NULL, bool bUseColor = false, const Vector& color1 = vec3_origin, const Vector& color2 = vec3_origin );

void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity, const char *pszAttachmentName, Vector vecColor1, Vector vecColor2, bool bUseColors=true, bool bResetAllParticlesOnEntity = false );
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, Vector vecColor1, Vector vecColor2, bool bUseColors=true, CSharedBaseEntity *pEntity = NULL, int iAttachType = PATTACH_CUSTOMORIGIN );
void DispatchParticleEffect( const char *pszParticleName, const Vector& vecStart, ParticleAttachment_t iAttachType, CSharedBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity = false );

void StopParticleEffects( CSharedBaseEntity *pEntity );
void StopParticleEffect( CSharedBaseEntity *pEntity, const char *pszParticleName );


#endif // PARTICLE_PARSE_H
