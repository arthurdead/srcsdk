//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared util code between client and server.
//
//=============================================================================//

#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#pragma once

#include "mathlib/vector.h"
#include "cmodel.h"
#include "utlvector.h"
#include "networkvar.h"
#include "engine/IEngineTrace.h"
#include "engine/IStaticPropMgr.h"
#include "shared_classnames.h"
#include "shareddefs.h"
#include "predictable_entity.h"
#ifdef GAME_DLL
#include "enginecallback.h"
#endif
#include "debugoverlay_shared.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
#endif

#ifdef PORTAL
#include "portal_util_shared.h"
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;
class CGameTrace;

#if defined( CLIENT_DLL )
class C_BasePlayer;
typedef C_BasePlayer CSharedBasePlayer;
#else
class CBasePlayer;
typedef CBasePlayer CSharedBasePlayer;
#endif

typedef CGameTrace trace_t;

extern ConVarBase *developer;	// developer mode


//-----------------------------------------------------------------------------
// Language IDs.
//-----------------------------------------------------------------------------
#define LANGUAGE_ENGLISH				0


//-----------------------------------------------------------------------------
// Pitch + yaw
//-----------------------------------------------------------------------------
float		UTIL_VecToYaw			(const Vector &vec);
float		UTIL_VecToPitch			(const Vector &vec);
float		UTIL_VecToYaw			(const matrix3x4_t& matrix, const Vector &vec);
float		UTIL_VecToPitch			(const matrix3x4_t& matrix, const Vector &vec);
Vector		UTIL_YawToVector		( float yaw );

//-----------------------------------------------------------------------------
// Shared random number generators for shared/predicted code:
// whenever generating random numbers in shared/predicted code, these functions
// have to be used. Each call should specify a unique "sharedname" string that
// seeds the random number generator. In loops make sure the "additionalSeed"
// is increased with the loop counter, otherwise it will always return the
// same random number
//-----------------------------------------------------------------------------
float	SharedRandomFloat( const char *sharedname, float flMinVal, float flMaxVal, int additionalSeed = 0 );
int		SharedRandomInt( const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed = 0 );
Vector	SharedRandomVector( const char *sharedname, float minVal, float maxVal, int additionalSeed = 0 );
QAngle	SharedRandomAngle( const char *sharedname, float minVal, float maxVal, int additionalSeed = 0 );

//-----------------------------------------------------------------------------
// Standard collision filters...
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass );
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask );


//-----------------------------------------------------------------------------
// Converts an IHandleEntity to an CBaseEntity
//-----------------------------------------------------------------------------
inline const CSharedBaseEntity *EntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#endif
}

inline CSharedBaseEntity *EntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#endif
}

typedef bool (*ShouldHitFunc_t)( IHandleEntity *pHandleEntity, int contentsMask );

//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterSimple );

	CTraceFilterSimple( const IHandleEntity *passentity, int collisionGroup, ShouldHitFunc_t pExtraShouldHitCheckFn = NULL );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity( const IHandleEntity *pPassEntity ) { m_pPassEnt = pPassEntity; }
	virtual void SetCollisionGroup( int iCollisionGroup ) { m_collisionGroup = iCollisionGroup; }

	const IHandleEntity *GetPassEntity( void ){ return m_pPassEnt;}

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
	ShouldHitFunc_t m_pExtraShouldHitCheckFunction;

};

class CTraceFilterSkipTwoEntities : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoEntities, CTraceFilterSimple );
	
	CTraceFilterSkipTwoEntities( const IHandleEntity *passentity = NULL, const IHandleEntity *passentity2 = NULL, int collisionGroup = COLLISION_GROUP_NONE );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity2( const IHandleEntity *pPassEntity2 ) { m_pPassEnt2 = pPassEntity2; }

private:
	const IHandleEntity *m_pPassEnt2;
};

class CTraceFilterSimpleList : public CTraceFilterSimple
{
public:
	CTraceFilterSimpleList( int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

	void	AddEntityToIgnore( IHandleEntity *pEntity );
	void	AddEntitiesToIgnore( int nCount, IHandleEntity **ppEntities );
protected:
	CUtlVector<IHandleEntity*>	m_PassEntities;
};

class CTraceFilterOnlyHitThis : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterOnlyHitThis );

	CTraceFilterOnlyHitThis( const IHandleEntity *hitentity );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	const IHandleEntity *m_pHitEnt;
};

class CTraceFilterOnlyNPCsAndPlayer : public CTraceFilterSimple
{
public:
	CTraceFilterOnlyNPCsAndPlayer( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterNoNPCsOrPlayer : public CTraceFilterSimple
{
public:
	CTraceFilterNoNPCsOrPlayer( const IHandleEntity *passentity = NULL, int collisionGroup = COLLISION_GROUP_NONE )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

//-----------------------------------------------------------------------------
// Purpose: Custom trace filter used for NPC LOS traces
//-----------------------------------------------------------------------------
class CTraceFilterLOS : public CTraceFilterSkipTwoEntities
{
public:
	CTraceFilterLOS( IHandleEntity *pHandleEntity, int collisionGroup, IHandleEntity *pHandleEntity2 = NULL );
	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterSkipClassname : public CTraceFilterSimple
{
public:
	CTraceFilterSkipClassname( const IHandleEntity *passentity, const char *pchClassname, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:

	const char *m_pchClassname;
};

class CTraceFilterSkipTwoClassnames : public CTraceFilterSkipClassname
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoClassnames, CTraceFilterSkipClassname );

	CTraceFilterSkipTwoClassnames( const IHandleEntity *passentity, const char *pchClassname, const char *pchClassname2, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	const char *m_pchClassname2;
};

class CTraceFilterSimpleClassnameList : public CTraceFilterSimple
{
public:
	CTraceFilterSimpleClassnameList( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

	void	AddClassnameToIgnore( const char *pchClassname );
private:
	CUtlVector<const char*>	m_PassClassnames;
};

class CTraceFilterChain : public CTraceFilter
{
public:
	CTraceFilterChain( ITraceFilter *pTraceFilter1, ITraceFilter *pTraceFilter2 );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	ITraceFilter	*m_pTraceFilter1;
	ITraceFilter	*m_pTraceFilter2;
};

#ifndef SWDS
// helper
void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration );
#endif

#ifndef SWDS
extern bool g_bTextMode;

extern ConVarBase* r_visualizetraces;
#endif

inline bool VisualizeTraces()
{
#ifndef SWDS
	if( !NDebugOverlay::IsEnabled() )
		return false;

	return r_visualizetraces->GetBool();
#else
	return false;
#endif
}

inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
#endif
}

inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
#endif
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 255, 0, true, -1.0f );
	}
#endif
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );

	enginetrace->TraceRay( ray, mask, pFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 255, 0, true, -1.0f );
	}
#endif
}

inline void UTIL_TraceRay( const Ray_t &ray, unsigned int mask, 
						  const IHandleEntity *ignore, int collisionGroup, trace_t *ptr, ShouldHitFunc_t pExtraShouldHitCheckFn = NULL )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup, pExtraShouldHitCheckFn );

	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
#endif
}

inline void UTIL_TraceRay( const Ray_t &ray, unsigned int mask, 
						  ITraceFilter *pFilter, trace_t *ptr )
{
	enginetrace->TraceRay( ray, mask, pFilter, ptr );

#ifndef SWDS
	if( VisualizeTraces() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns z value of floor below given point (up to 384 inches below)
//-----------------------------------------------------------------------------
float GetLongFloorZ(const Vector &origin);

unsigned int UTIL_MaskForEntity( CSharedBaseEntity *pEntity, bool brush_only = false );
unsigned int UTIL_CollisionGroupForEntity( CSharedBaseEntity *pEntity );

// Sweeps a particular entity through the world
void UTIL_TraceEntity( CSharedBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr );
void UTIL_TraceEntity( CSharedBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, ITraceFilter *pFilter, trace_t *ptr );
void UTIL_TraceEntity( CSharedBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *ignore, int collisionGroup, trace_t *ptr );

bool UTIL_EntityHasMatchingRootParent( CSharedBaseEntity *pRootParent, CSharedBaseEntity *pEntity );

inline int UTIL_PointContents( const Vector &vec, int contentsMask )
{
	return enginetrace->GetPointContents( vec, contentsMask, NULL );
}

// Sweeps against a particular model, using collision rules 
void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CSharedBaseEntity *pentModel, int collisionGroup, trace_t *ptr );

void UTIL_ClipTraceToPlayers( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *filter, trace_t *tr );

// Particle effect tracer
void		UTIL_ParticleTracer( const char *pszTracerEffectName, const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = 0, bool bWhiz = false );

// Old style, non-particle system, tracers
void		UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = TRACER_DONT_USE_ATTACHMENT, float flVelocity = 0, bool bWhiz = false, const char *pCustomTracerName = NULL, int iParticleID = 0 );

bool		UTIL_ShouldShowBlood( int bloodColor );
void		UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount );

void		UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount );
void		UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor );
void		UTIL_DecalTrace( trace_t *pTrace, char const *decalName );
bool		UTIL_IsSpaceEmpty( CSharedBaseEntity *pMainEnt, const Vector &vMin, const Vector &vMax );
bool		UTIL_IsSpaceEmpty( CSharedBaseEntity *pMainEnt, const Vector &vMin, const Vector &vMax, unsigned int mask, ITraceFilter *pFilter );

// ----------------------------------------------------------------------------- //
// This is a cache of preloaded keyvalues.
// ----------------------------------------------------------------------------- // 

KeyValues* CacheKeyValuesForFile( const char *pFilename, const char *pPathID );

void ClearKeyValuesCache();

// 
// Read a possibly-encrypted KeyValues file in. 
// If pICEKey is NULL, then it appends .txt to the filename and loads it as an unencrypted file.
// If pICEKey is non-NULL, then it appends .ctx to the filename and loads it as an encrypted file.
//
// (This should be moved into a more appropriate place).
//
KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const char *pSearchPath, const unsigned char *pICEKey, bool bForceReadEncryptedFile, bool *bReadEncrypted );
KeyValues* ReadKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const char *pSearchPath );

void		UTIL_DecodeICE( unsigned char * buffer, int size, const unsigned char *key );
void		UTIL_EncodeICE( unsigned char * buffer, unsigned int size, const unsigned char *key );
unsigned short UTIL_GetAchievementEventMask( void );	

#define FL_AXIS_DIRECTION_NONE	( 0 )
#define FL_AXIS_DIRECTION_X		( 1 << 0 )
#define FL_AXIS_DIRECTION_NX	( 1 << 1 )
#define FL_AXIS_DIRECTION_Y		( 1 << 2 )
#define FL_AXIS_DIRECTION_NY	( 1 << 3 )
#define FL_AXIS_DIRECTION_Z		( 1 << 4 )
#define FL_AXIS_DIRECTION_NZ	( 1 << 5 )

bool		UTIL_FindClosestPassableSpace( const Vector &vCenter, const Vector &vExtents, const Vector &vIndecisivePush, ITraceFilter *pTraceFilter, unsigned int fMask, unsigned int iIterations, Vector &vCenterOut, int nAxisRestrictionFlags = FL_AXIS_DIRECTION_NONE );
bool		UTIL_FindClosestPassableSpace( CSharedBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask, unsigned int iIterations, Vector &vOriginOut, Vector *pStartingPosition = NULL, int nAxisRestrictionFlags = FL_AXIS_DIRECTION_NONE );
bool		UTIL_FindClosestPassableSpace( CSharedBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask, Vector *pStartingPosition = NULL, int nAxisRestrictionFlags = FL_AXIS_DIRECTION_NONE );

void		UTIL_StringToVector( float *pVector, const char *pString );
void		UTIL_StringToIntArray( int *pVector, int count, const char *pString );
void		UTIL_StringToFloatArray( float *pVector, int count, const char *pString );
void		UTIL_StringToColor32( color32 *color, const char *pString );

// Version of UTIL_StringToIntArray that doesn't set all untouched array elements to 0.
void		UTIL_StringToIntArray_PreserveArray( int *pVector, int count, const char *pString );

// Version of UTIL_StringToFloatArray that doesn't set all untouched array elements to 0.
void		UTIL_StringToFloatArray_PreserveArray( float *pVector, int count, const char *pString );

CSharedBasePlayer *UTIL_PlayerByIndex( int entindex );

// For not just using one big ai net
extern CSharedBaseEntity*	FindPickerEntity( CSharedBasePlayer* pPlayer );
extern CSharedBaseEntity *FindPickerEntityClass(CSharedBasePlayer *pPlayer, const char *classname);

//=============================================================================
// HPE_BEGIN:
// [menglish] Added UTIL function for events in client win_panel which transmit the player as a user ID
//=============================================================================
CSharedBasePlayer *UTIL_PlayerByUserId( int userID );
//=============================================================================
// HPE_END
//=============================================================================

// decodes a buffer using a 64bit ICE key (inplace)
void		UTIL_DecodeICE( unsigned char * buffer, int size, const unsigned char *key);


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a position and a ray, return the shortest distance between the two.
 * If 'pos' is beyond either end of the ray, the returned distance is negated.
 */
inline float DistanceToRay( const Vector &pos, const Vector &rayStart, const Vector &rayEnd, float *along = NULL, Vector *pointOnRay = NULL )
{
	Vector to = pos - rayStart;
	Vector dir = rayEnd - rayStart;
	float length = dir.NormalizeInPlace();

	float rangeAlong = DotProduct( dir, to );
	if (along)
	{
		*along = rangeAlong;
	}

	float range;

	if (rangeAlong < 0.0f)
	{
		// off start point
		range = -(pos - rayStart).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayStart;
		}
	}
	else if (rangeAlong > length)
	{
		// off end point
		range = -(pos - rayEnd).Length();

		if (pointOnRay)
		{
			*pointOnRay = rayEnd;
		}
	}
	else // within ray bounds
	{
		Vector onRay = rayStart + rangeAlong * dir;
		range = (pos - onRay).Length();

		if (pointOnRay)
		{
			*pointOnRay = onRay;
		}
	}

	return range;
}


//--------------------------------------------------------------------------------------------------------------
/**
* Macro for creating an interface that when inherited from automatically maintains a list of instances
* that inherit from that interface.
*/

// interface for entities that want to a auto maintained global list
#define DECLARE_AUTO_LIST( interfaceName ) \
	class interfaceName; \
	abstract_class interfaceName \
	{ \
	public: \
		interfaceName( bool bAutoAdd = true ); \
		virtual ~interfaceName(); \
		static void AddToAutoList( interfaceName *pElement ) { m_##interfaceName##AutoList.AddToTail( pElement ); } \
		static void RemoveFromAutoList( interfaceName *pElement ) { m_##interfaceName##AutoList.FindAndFastRemove( pElement ); } \
		static const CUtlVector< interfaceName* >& AutoList( void ) { return m_##interfaceName##AutoList; } \
	private: \
		static CUtlVector< interfaceName* > m_##interfaceName##AutoList; \
	};

// Creates the auto add/remove constructor/destructor...
// Pass false to the constructor to not auto add
#define IMPLEMENT_AUTO_LIST( interfaceName ) \
	CUtlVector< class interfaceName* > interfaceName::m_##interfaceName##AutoList; \
	interfaceName::interfaceName( bool bAutoAdd ) \
	{ \
		if ( bAutoAdd ) \
		{ \
			AddToAutoList( this ); \
		} \
	} \
	interfaceName::~interfaceName() \
	{ \
		RemoveFromAutoList( this ); \
	}

//--------------------------------------------------------------------------------------------------------------
// This would do the same thing without requiring casts all over the place. Yes, it's a template, but 
// DECLARE_AUTO_LIST requires a CUtlVector<T> anyway. TODO ask about replacing the macros with this.
//template<class T>
//class AutoList {
//public:
//	typedef CUtlVector<T*> AutoListType;
//	static AutoListType& All() { return m_autolist; }
//protected:
//	AutoList() { m_autolist.AddToTail(static_cast<T*>(this)); }
//	virtual ~AutoList() { m_autolist.FindAndFastRemove(static_cast<T*>(this)); }
//private:
//	static AutoListType m_autolist;
//};

//--------------------------------------------------------------------------------------------------------------
/**
 * Simple class for tracking intervals of game time.
 * Upon creation, the timer is invalidated.  To measure time intervals, start the timer via Start().
 */
class IntervalTimer
{
public:
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif
	DECLARE_DATADESC();
	DECLARE_CLASS_NOBASE( IntervalTimer );
	DECLARE_EMBEDDED_NETWORKVAR();

	IntervalTimer( void )
	{
		m_timestamp = -1.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now();
	}		

	void Start( void )
	{
		m_timestamp = Now();
	}

	void StartFromTime( float startTime )
	{
		m_timestamp = startTime;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}		

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	/// if not started, elapsed time is very large
	float GetElapsedTime( void ) const
	{
		return (HasStarted()) ? (Now() - m_timestamp) : 99999.9f;
	}

	bool IsLessThen( float duration ) const
	{
		return (Now() - m_timestamp < duration) ? true : false;
	}

	bool IsGreaterThen( float duration ) const
	{
		return (Now() - m_timestamp > duration) ? true : false;
	}

	float GetStartTime( void ) const
	{
		return m_timestamp;
	}

protected:
	CNetworkVar( float, m_timestamp );
	float Now( void ) const;		// work-around since client header doesn't like inlined gpGlobals->curtime
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE(DT_IntervalTimer);
#else
EXTERN_SEND_TABLE(DT_IntervalTimer);
#endif

//--------------------------------------------------------------------------------------------------------------
/**
 * Simple class for counting down a short interval of time.
 * Upon creation, the timer is invalidated.  Invalidated countdown timers are considered to have elapsed.
 */
class CountdownTimer
{
public:
#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif
	DECLARE_CLASS_NOBASE( CountdownTimer );
	DECLARE_EMBEDDED_NETWORKVAR();

	CountdownTimer( void )
	{
		m_timestamp = -1.0f;
		m_duration = 0.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now() + m_duration;
	}		

	void Start( float duration )
	{
		m_timestamp = Now() + duration;
		m_duration = duration;
	}

	void StartFromTime( float startTime, float duration )
	{
		m_timestamp = startTime + duration;
		m_duration = duration;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}		

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	bool IsElapsed( void ) const
	{
		return (Now() > m_timestamp);
	}

	float GetElapsedTime( void ) const
	{
		return Now() - m_timestamp + m_duration;
	}

	float GetRemainingTime( void ) const
	{
		return (m_timestamp - Now());
	}

	/// return original countdown time
	float GetCountdownDuration( void ) const
	{
		return (m_timestamp > 0.0f) ? m_duration.Get() : 0.0f;
	}

	/// 1.0 for newly started, 0.0 for elapsed
	float GetRemainingRatio( void ) const
	{
		if ( HasStarted() )
		{
			float left = GetRemainingTime() / m_duration;
			if ( left < 0.0f )
				return 0.0f;
			if ( left > 1.0f )
				return 1.0f;
			return left;
		}

		return 0.0f;
	}

private:
	CNetworkVar( float, m_duration );
	CNetworkVar( float, m_timestamp );
	virtual float Now( void ) const;		// work-around since client header doesn't like inlined gpGlobals->curtime
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE(DT_CountdownTimer);
#else
EXTERN_SEND_TABLE(DT_CountdownTimer);
#endif

class RealTimeCountdownTimer : public CountdownTimer
{
	virtual float Now( void ) const OVERRIDE
	{
		return Plat_FloatTime();
	}
};

/**
* Simple class for tracking change in values over time.
*/
#define TIMELINE_ARRAY_SIZE 64
#define TIMELINE_INTERVAL_START 0.25f

enum TimelineCompression_t
{
	TIMELINE_COMPRESSION_SUM,
	TIMELINE_COMPRESSION_COUNT_PER_INTERVAL,
	TIMELINE_COMPRESSION_AVERAGE,
	TIMELINE_COMPRESSION_AVERAGE_BLEND,

	TIMELINE_COMPRESSION_TOTAL
};

class CTimeline : public IntervalTimer
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTimeline, IntervalTimer );
	DECLARE_EMBEDDED_NETWORKVAR();

	CTimeline( void )
	{
		ClearValues();
	}

	void ClearValues( void );
	void ClearAndStart( void ) { ClearValues(); Start(); }
	void StopRecording( void ) { m_bStopped = true; }
	void RecordValue( float flValue );
	void RecordFinalValue( float flValue ) { RecordValue( flValue ); StopRecording(); }

	int Count( void ) const
	{
		return m_nBucketCount;
	}

	float GetValue( int i ) const;

	float GetValueAtInterp( float fInterp ) const;

	float GetValueTime( int i ) const
	{
		Assert( i >= 0 && i < m_nBucketCount );
		return static_cast<float>( i ) * m_flInterval;
	}

	float GetInterval( void ) const
	{
		return m_flInterval;
	}

	void SetCompressionType( TimelineCompression_t nCompressionType )
	{
		m_nCompressionType = nCompressionType;
	}

	TimelineCompression_t GetCompressionType( void ) const
	{
		return m_nCompressionType;
	}

private:
	int GetCurrentBucket( void )
	{
		return static_cast<float>( Now() - m_timestamp ) / m_flInterval;
	}

	void Compress( void );

	CNetworkArray( float, m_flValues, TIMELINE_ARRAY_SIZE );
	CNetworkArray( int, m_nValueCounts, TIMELINE_ARRAY_SIZE );
	CNetworkVar( int, m_nBucketCount );
	CNetworkVar( float, m_flInterval );
	CNetworkVar( float, m_flFinalValue );
	CNetworkVar( TimelineCompression_t, m_nCompressionType );
	CNetworkVar( bool, m_bStopped );
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE(DT_Timeline);
#else
EXTERN_SEND_TABLE(DT_Timeline);
#endif

int UTIL_CountNumBitsSet( unsigned int nVar );
int UTIL_CountNumBitsSet( uint64 nVar );

bool UTIL_GetModDir( char *lpszTextOut, unsigned int nSize );

/*UTIL_CalcFrustumThroughPolygon - Given a frustum and a polygon, calculate how the current frustum would clip the polygon, then generate a new frustum that runs along the edge of the clipped polygon.
-returns number of planes in the output frustum, 0 if the polygon was completely clipped by the input frustum
-vFrustumOrigin can be thought of as the camera origin if your frustum is a view frustum
-planes should face inward
-iPreserveCount will preserve N planes at the end of your input frustum and ensure they're at the end of your output frustum. Assuming your input frustum is of type "Frustum", a value of 2 would preserve your near and far planes
-to ensure that your output frustum can hold the entire complex frustum we generate. Make it of size (iPolyVertCount + iCurrentFrustumPlanes + iPreserveCount). Otherwise the output frustum will be simplified to fit your maximum output by eliminating bounding planes with the clipped area.
-a lack of input frustum is considered valid input*/
int UTIL_CalcFrustumThroughConvexPolygon( const Vector *pPolyVertices, int iPolyVertCount, const Vector &vFrustumOrigin, const VPlane *pInputFrustumPlanes, int iInputFrustumPlanes, VPlane *pOutputFrustumPlanes, int iMaxOutputPlanes, int iPreserveCount );

const char* ReadAndAllocStringValue( KeyValues *pSub, const char *pName, const char *pFilename = NULL );

int UTIL_StringFieldToInt( const char *szValue, const char **pValueStrings, int iNumStrings );

class CFlaggedEntitiesEnum : public IPartitionEnumerator
{
public:
	CFlaggedEntitiesEnum( CSharedBaseEntity **pList, int listMax, int flagMask );

	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	int GetCount() { return m_count; }
	bool AddToList( CSharedBaseEntity *pEntity );

private:
	CSharedBaseEntity		**m_pList;
	int				m_listMax;
	int				m_flagMask;
	int				m_count;
};

class CHurtableEntitiesEnum : public IPartitionEnumerator
{
public:
	CHurtableEntitiesEnum( CSharedBaseEntity **pList, int listMax );

	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	int GetCount() { return m_count; }
	bool AddToList( CSharedBaseEntity *pEntity );

private:
	CSharedBaseEntity		**m_pList;
	int				m_listMax;
	int				m_count;
};

int			UTIL_EntitiesAlongRay( const Ray_t &ray, CFlaggedEntitiesEnum *pEnum  );

inline int UTIL_EntitiesAlongRay( CSharedBaseEntity **pList, int listMax, const Ray_t &ray, int flagMask )
{
	CFlaggedEntitiesEnum rayEnum( pList, listMax, flagMask );
	return UTIL_EntitiesAlongRay( ray, &rayEnum );
}

//-----------------------------------------------------------------------------
// Holidays
//-----------------------------------------------------------------------------

// Used at level change and round start to re-calculate which holiday is active
void				UTIL_CalculateHolidays();

int				UTIL_GetNumHolidays();

EHolidayIndex				UTIL_GetHolidayIndex( EHolidayFlag eHoliday );
EHolidayFlag				UTIL_GetHolidayFlag( EHolidayIndex eHoliday );

bool				UTIL_IsHolidaysActive( EHolidayFlags eHolidays );
EHolidayFlags	UTIL_GetHolidaysForString( const char* pszHolidayName );

const char		   *UTIL_GetHolidaysString( EHolidayFlags eHolidays );

// This will return the first active holiday string it can find. In the case of multiple
// holidays overlapping, the list order will act as priority.
const char		   *UTIL_GetActiveHolidaysString();

#ifdef CLIENT_DLL
unsigned int UTIL_HolidaysToVisionFilters( EHolidayFlags eHolidays );

unsigned int UTIL_GetActiveHolidaysVisionFilter();
#endif

extern bool FindInList( const char **pStrings, const char *pToFind, int size );

#endif // UTIL_SHARED_H
