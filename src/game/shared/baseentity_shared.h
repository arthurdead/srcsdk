//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEENTITY_SHARED_H
#define BASEENTITY_SHARED_H

#include "mathlib/vector.h"

// Simple shared header file for common base entities

// entity capabilities
// These are caps bits to indicate what an object's capabilities (currently used for +USE, save/restore and level transitions)
#define		FCAP_MUST_SPAWN				0x00000001		// Spawn after restore
#define		FCAP_ACROSS_TRANSITION		0x00000002		// should transfer between transitions 
// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define		FCAP_FORCE_TRANSITION		0x00000004		// ALWAYS goes across transitions
#define		FCAP_NOTIFY_ON_TRANSITION	0x00000008		// Entity will receive Inside/Outside transition inputs when a transition occurs

#define		FCAP_IMPULSE_USE			0x00000010		// can be used by the player
#define		FCAP_CONTINUOUS_USE			0x00000020		// can be used by the player
#define		FCAP_ONOFF_USE				0x00000040		// can be used by the player
#define		FCAP_DIRECTIONAL_USE		0x00000080		// Player sends +/- 1 when using (currently only tracktrains)
// NOTE: Normally +USE only works in direct line of sight.  Add these caps for additional searches
#define		FCAP_USE_ONGROUND			0x00000100
#define		FCAP_USE_IN_RADIUS			0x00000200

#define		FCAP_MASTER					0x10000000		// Can be used to "master" other entities (like multisource)
#define		FCAP_WCEDIT_POSITION		0x40000000		// Can change position and update Hammer in edit mode

// How many bits are used to transmit parent attachment indices?
#define NUM_PARENTATTACHMENT_BITS	6

// Maximum number of vphysics objects per entity
#define VPHYSICS_MAX_OBJECT_LIST_COUNT	1024

// Shared EntityMessage between game and client .dlls
#define BASEENTITY_MSG_REMOVE_DECALS	1

extern float k_flMaxEntityPosCoord;
extern float k_flMaxEntityEulerAngle;
extern float k_flMaxEntitySpeed;
extern float k_flMaxEntitySpinRate;

extern bool CheckEmitReasonablePhysicsSpew();

// Returns:
//   -1 - velocity is really, REALLY bad and probably should be rejected.
//   0  - velocity was suspicious and clamped.
//   1  - velocity was OK and not modified
extern int CheckEntityVelocity( Vector &v );

bool IsEntityCoordinateReasonable ( const vec_t c );

bool IsEntityPositionReasonable( const Vector &v );

bool IsEntityQAngleReasonable( const QAngle &q );

// Angular velocity in exponential map form
bool IsEntityAngularVelocityReasonable( const Vector &q );

// Angular velocity of each Euler angle.
bool IsEntityQAngleVelReasonable( const QAngle &q );

#endif

#ifndef BASEENTITY_SHARED_INLINES_H
#define BASEENTITY_SHARED_INLINES_H

#if defined( CLIENT_DLL )
#include "c_baseentity.h"
typedef C_BaseEntity CSharedBaseEntity;
#else
#include "baseentity.h"
typedef CBaseEntity CSharedBaseEntity;
#endif

// CBaseEntity inlines
inline bool CSharedBaseEntity::IsPlayerSimulated( void ) const
{
	return m_bIsPlayerSimulated;
}

inline CSharedBasePlayer *CSharedBaseEntity::GetSimulatingPlayer( void )
{
	return m_hPlayerSimulationOwner.Get();
}

inline MoveType_t CSharedBaseEntity::GetMoveType() const
{
	return (MoveType_t)(unsigned char)m_MoveType;
}

inline MoveCollide_t CSharedBaseEntity::GetMoveCollide() const
{
	return (MoveCollide_t)(unsigned char)m_MoveCollide;
}

//-----------------------------------------------------------------------------
// Collision group accessors
//-----------------------------------------------------------------------------
inline int CSharedBaseEntity::GetCollisionGroup() const
{
	return m_CollisionGroup;
}

inline uint64	CSharedBaseEntity::GetFlags( void ) const
{
	return m_fFlags;
}

inline bool CSharedBaseEntity::IsAlive( void )
{
	return m_lifeState == LIFE_ALIVE; 
}

inline CSharedBaseEntity	*CSharedBaseEntity::GetOwnerEntity() const
{
	return m_hOwnerEntity.Get();
}

inline CSharedBaseEntity	*CSharedBaseEntity::GetEffectEntity() const
{
	return m_hEffectEntity.Get();
}

inline int CSharedBaseEntity::GetPredictionRandomSeed( bool bUseUnSyncedServerPlatTime )
{
#ifdef GAME_DLL
	return bUseUnSyncedServerPlatTime ? m_nPredictionRandomSeedServer : m_nPredictionRandomSeed;
#else
	return m_nPredictionRandomSeed;
#endif
}

inline CSharedBasePlayer *CSharedBaseEntity::GetPredictionPlayer( void )
{
	return m_pPredictionPlayer;
}

inline void CSharedBaseEntity::SetPredictionPlayer( CSharedBasePlayer *player )
{
	m_pPredictionPlayer = player;
}


inline bool CSharedBaseEntity::IsSimulatedEveryTick() const
{
	return m_bSimulatedEveryTick;
}

inline bool CSharedBaseEntity::IsAnimatedEveryTick() const
{
	return m_bAnimatedEveryTick;
}

inline void CSharedBaseEntity::SetSimulatedEveryTick( bool sim )
{
	if ( m_bSimulatedEveryTick != sim )
	{
		m_bSimulatedEveryTick = sim;
#ifdef CLIENT_DLL
		Interp_UpdateInterpolationAmounts( GetVarMapping() );
#endif
	}
}

inline void CSharedBaseEntity::SetAnimatedEveryTick( bool anim )
{
	if ( m_bAnimatedEveryTick != anim )
	{
		m_bAnimatedEveryTick = anim;
#ifdef CLIENT_DLL
		Interp_UpdateInterpolationAmounts( GetVarMapping() );
#endif
	}
}

inline float CSharedBaseEntity::GetAnimTime() const
{
	return m_flAnimTime;
}

inline float CSharedBaseEntity::GetSimulationTime() const
{
	return m_flSimulationTime;
}

inline void CSharedBaseEntity::SetAnimTime( float at )
{
	m_flAnimTime = at;
}

inline void CSharedBaseEntity::SetSimulationTime( float st )
{
	m_flSimulationTime = st;
}

inline int CSharedBaseEntity::GetEffects( void ) const
{ 
	return m_fEffects; 
}

inline void CSharedBaseEntity::RemoveEffects( int nEffects ) 
{ 
#if !defined( CLIENT_DLL )
#ifdef HL2_EPISODIC
	if ( nEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT) )
	{
		// Hack for now, to avoid player emitting radius with his flashlight
		if ( !IsPlayer() )
		{
			RemoveEntityFromDarknessCheck( this );
		}
	}
#endif // HL2_EPISODIC
#endif // !CLIENT_DLL

	m_fEffects &= ~nEffects;
	if ( nEffects & EF_NODRAW )
	{
#ifndef CLIENT_DLL
		NetworkProp()->MarkPVSInformationDirty();
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
	}
}

inline void CSharedBaseEntity::ClearEffects( void ) 
{ 
#if !defined( CLIENT_DLL )
#ifdef HL2_EPISODIC
	if ( m_fEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT) )
	{
		// Hack for now, to avoid player emitting radius with his flashlight
		if ( !IsPlayer() )
		{
			RemoveEntityFromDarknessCheck( this );
		}
	}
#endif // HL2_EPISODIC
#endif // !CLIENT_DLL

	m_fEffects = 0;
#ifndef CLIENT_DLL
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
}

inline bool CSharedBaseEntity::IsEffectActive( int nEffects ) const
{ 
	return (m_fEffects & nEffects) != 0; 
}

// convenience functions for fishing out the vectors of this object
// equivalent to GetVectors(), but doesn't need an intermediate stack 
// variable (which might cause an LHS anyway)
inline Vector	CSharedBaseEntity::Forward() const RESTRICT  ///< get my forward (+x) vector
{
	const matrix3x4_t &mat = EntityToWorldTransform();
	return Vector( mat[0][0], mat[1][0], mat[2][0] );
}

inline Vector	CSharedBaseEntity::Left() const RESTRICT     ///< get my left    (+y) vector
{
	const matrix3x4_t &mat = EntityToWorldTransform();
	return Vector( mat[0][1], mat[1][1], mat[2][1] );
}

inline Vector	CSharedBaseEntity::Up() const  RESTRICT      ///< get my up      (+z) vector
{
	const matrix3x4_t &mat = EntityToWorldTransform();
	return Vector( mat[0][2], mat[1][2], mat[2][2] );
}

inline bool IsEntityCoordinateReasonable ( const vec_t c )
{
	float r = k_flMaxEntityPosCoord;
	return c > -r && c < r;
}

inline bool IsEntityPositionReasonable( const Vector &v )
{
	float r = k_flMaxEntityPosCoord;
	return
		v.x > -r && v.x < r &&
		v.y > -r && v.y < r &&
		v.z > -r && v.z < r;
}

inline bool IsEntityQAngleReasonable( const QAngle &q )
{
	float r = k_flMaxEntityEulerAngle;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

// Angular velocity in exponential map form
inline bool IsEntityAngularVelocityReasonable( const Vector &q )
{
	float r = k_flMaxEntitySpinRate;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

// Angular velocity of each Euler angle.
inline bool IsEntityQAngleVelReasonable( const QAngle &q )
{
	float r = k_flMaxEntitySpinRate;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

#endif // BASEENTITY_SHARED_H
