//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RAGDOLL_SHARED_H
#define RAGDOLL_SHARED_H
#pragma once

#include "igamesystem.h"
#include "ehandle.h"
#include "vcollide_parse.h"

class IPhysicsObject;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsCollision;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
struct matrix3x4_t;

struct vcollide_t;
struct studiohdr_t;
class CStudioHdr;
class CBoneAccessor;

#include "mathlib/vector.h"
#include "bone_accessor.h"

#ifdef GAME_DLL
class CBaseAnimating;
typedef CBaseAnimating CSharedBaseAnimating;
#else
class C_BaseAnimating;
typedef C_BaseAnimating CSharedBaseAnimating;
#endif

// UNDONE: Remove and make dynamic?
#define RAGDOLL_MAX_ELEMENTS	128
#define RAGDOLL_INDEX_BITS		7			// NOTE 1<<RAGDOLL_INDEX_BITS >= RAGDOLL_MAX_ELEMENTS

#define CORE_DISSOLVE_FADE_START 0.2f
#define CORE_DISSOLVE_MODEL_FADE_START 0.1f
#define CORE_DISSOLVE_MODEL_FADE_LENGTH 0.05f
#define CORE_DISSOLVE_FADEIN_LENGTH 0.1f

struct ragdollelement_t
{
	Vector				originParentSpace;
	IPhysicsObject		*pObject;		// all valid elements have an object
	IPhysicsConstraint	*pConstraint;	// all valid elements have a constraint (except the root)
	int					parentIndex;
};

struct ragdoll_t
{
	int						listCount;
	bool					allowStretch;
	bool					unused;
	IPhysicsConstraintGroup *pGroup;
	// store these in separate arrays for save/load
	ragdollelement_t 	list[RAGDOLL_MAX_ELEMENTS];
	int					boneIndex[RAGDOLL_MAX_ELEMENTS];
	ragdollanimatedfriction_t animfriction;
};

struct ragdollparams_t
{
	void		*pGameData;
	vcollide_t	*pCollide;
	CStudioHdr	*pStudioHdr;
	int			modelIndex;
	Vector		forcePosition;
	Vector		forceVector;
	int			forceBoneIndex;
	const matrix3x4_t *pCurrentBones;
	float		jointFrictionScale;
	bool		allowStretch;
	bool		fixedConstraints;
};

typedef CHandle<CSharedBaseAnimating> CRagdollHandle;

class CRagdollEntry
{
public:
	CRagdollEntry( CSharedBaseAnimating *pRagdoll, float flForcedRetireTime ) : m_hRagdoll( pRagdoll ), m_flForcedRetireTime( flForcedRetireTime )
	{
	}
	CSharedBaseAnimating* Get() { return m_hRagdoll.Get(); }
	float GetForcedRetireTime() { return m_flForcedRetireTime; }

private:
	CRagdollHandle m_hRagdoll;
	float m_flForcedRetireTime;
};

//-----------------------------------------------------------------------------
// This hooks the main game systems callbacks to allow the AI system to manage memory
//-----------------------------------------------------------------------------
class CRagdollLRURetirement : public CAutoGameSystemPerFrame
{
public:
	CRagdollLRURetirement( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	// Methods of IGameSystem
	virtual void Update( float frametime );
	virtual void FrameUpdatePostEntityThink( void );

	// Move it to the top of the LRU
	void MoveToTopOfLRU( CSharedBaseAnimating *pRagdoll, bool bImportant = false, float flForcedRetireTime = 0.0f );
	void SetMaxRagdollCount( int iMaxCount ){ m_iMaxRagdolls = iMaxCount; }

	virtual void LevelInitPreEntity( void );
	int CountRagdolls( bool bOnlySimulatingRagdolls ) { return bOnlySimulatingRagdolls ? m_iSimulatedRagdollCount : m_iRagdollCount; }

private:
	CUtlLinkedList< CRagdollEntry > m_LRU; 
	CUtlLinkedList< CRagdollEntry > m_LRUImportantRagdolls; 

	int m_iMaxRagdolls;
	int m_iSimulatedRagdollCount;
	int m_iRagdollCount;
};

extern CRagdollLRURetirement s_RagdollLRU;


bool RagdollCreate( ragdoll_t &ragdoll, const ragdollparams_t &params, IPhysicsEnvironment *pPhysEnv );
bool RagdollCreateDestr( ragdoll_t &ragdoll, const ragdollparams_t &params, IPhysicsEnvironment *pPhysEnv );

void RagdollActivateDestr( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex, bool bForceWake = true );
void RagdollActivate( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex, bool bForceWake = true );
void RagdollSetupCollisions( ragdoll_t &ragdoll, vcollide_t *pCollide, int modelIndex );
void RagdollDestroy( ragdoll_t &ragdoll );

// Gets the bone matrix for a ragdoll object
// NOTE: This is different than the object's position because it is
// forced to be rigidly attached in parent space
bool RagdollGetBoneMatrix( const ragdoll_t &ragdoll, CBoneAccessor &pBoneToWorld, int objectIndex );

// Parse the ragdoll and obtain the mapping from each physics element index to a bone index
// returns num phys elements
int RagdollExtractBoneIndices( int *boneIndexOut, CStudioHdr *pStudioHdr, vcollide_t *pCollide );

// computes an exact bbox of the ragdoll's physics objects
void RagdollComputeExactBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs );
void RagdollComputeApproximateBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs );
bool RagdollIsAsleep( const ragdoll_t &ragdoll );
void RagdollSetupAnimatedFriction( IPhysicsEnvironment *pPhysEnv, ragdoll_t *ragdoll, int iModelIndex );

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const matrix3x4_t *pBoneToWorld );
void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, const matrix3x4_t *pPrevBones, const matrix3x4_t *pCurrentBones, float dt );

void RagdollSolveSeparation( ragdoll_t &ragdoll, CSharedBaseEntity *pEntity );

#endif // RAGDOLL_SHARED_H
