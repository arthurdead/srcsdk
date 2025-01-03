//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ENGINE_IENGINETRACE_H
#define ENGINE_IENGINETRACE_H
#pragma once

#include "basehandle.h"
#include "utlvector.h" //need CUtlVector for IEngineTrace::GetBrushesIn*()
#include "mathlib/vector4d.h"
#include "bspflags.h"
#include "hackmgr/hackmgr.h"

DECLARE_LOGGING_CHANNEL( LOG_TRACE );

class Vector;
class IHandleEntity;
struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;
class ICollideable;
class QAngle;
class CTraceListData;
typedef CTraceListData ITraceListData;
class CPhysCollide;
struct cplane_t;
struct virtualmeshlist_t;

//-----------------------------------------------------------------------------
// The standard trace filter... NOTE: Most normal traces inherit from CTraceFilter!!!
//-----------------------------------------------------------------------------
enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY,				// NOTE: This does *not* test static props!!!
	TRACE_ENTITIES_ONLY,			// NOTE: This version will *not* test static props
	TRACE_EVERYTHING_FILTER_PROPS,	// NOTE: This version will pass the IHandleEntity for props through the filter, unlike all other filters
};

abstract_class ITraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pEntity, int contentsMask ) = 0;
	virtual TraceType_t	GetTraceType() const = 0;
};


//-----------------------------------------------------------------------------
// Classes are expected to inherit these + implement the ShouldHitEntity method
//-----------------------------------------------------------------------------

// This is the one most normal traces will inherit from
class CTraceFilter : public ITraceFilter
{
public:
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterEntitiesOnly : public ITraceFilter
{
public:
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
};


//-----------------------------------------------------------------------------
// Classes need not inherit from these
//-----------------------------------------------------------------------------
class CTraceFilterWorldOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_WORLD_ONLY;
	}
};

class CTraceFilterWorldAndPropsOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterHitAll : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{ 
		return true; 
	}
};

enum DebugTraceCounterBehavior_t
{
	kTRACE_COUNTER_SET = 0,
	kTRACE_COUNTER_INC,
};

//-----------------------------------------------------------------------------
// Enumeration interface for EnumerateLinkEntities
//-----------------------------------------------------------------------------
abstract_class IEntityEnumerator
{
public:
	// This gets called with each handle
	virtual bool EnumEntity( IHandleEntity *pHandleEntity ) = 0; 
};

struct BrushSideInfo_t
{
	Vector4D plane;			// The plane of the brush side
	unsigned short bevel;	// Bevel plane?
	unsigned short thin;	// Thin?
};

//-----------------------------------------------------------------------------
// Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_ENGINETRACE_SERVER	"EngineTraceServer003"
#define INTERFACEVERSION_ENGINETRACE_CLIENT	"EngineTraceClient003"
abstract_class IEngineTrace
{
private:
	// Returns the contents mask + entity at a particular world-space position
	virtual int		DO_NOT_USE_GetPointContents( const Vector &vecAbsPosition, IHandleEntity** ppEntity ) = 0;

public:
	// Returns the contents mask + entity at a particular world-space position
	HACKMGR_CLASS_API int		GetPointContents( const Vector &vecAbsPosition, int contentsMask = MASK_ALL, IHandleEntity** ppEntity = NULL );
	
	// Returns the contents mask of the world only @ the world-space position (static props are ignored)
	HACKMGR_CLASS_API int		GetPointContents_WorldOnly( const Vector &vecAbsPosition, int contentsMask = MASK_ALL );

	// Get the point contents, but only test the specific entity. This works
	// on static props and brush models.
	//
	// If the entity isn't a static prop or a brush model, it returns CONTENTS_EMPTY and sets
	// bFailed to true if bFailed is non-null.
	virtual int		GetPointContents_Collideable( ICollideable *pCollide, const Vector &vecAbsPosition ) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToEntity( const Ray_t &ray, unsigned int fMask, IHandleEntity *pEnt, trace_t *pTrace ) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToCollideable( const Ray_t &ray, unsigned int fMask, ICollideable *pCollide, trace_t *pTrace ) = 0;

	// A version that simply accepts a ray (can work as a traceline or tracehull)
	virtual void	TraceRay( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// A version that sets up the leaf and entity lists and allows you to pass those in for collision.
	virtual void	SetupLeafAndEntityListRay( const Ray_t &ray, CTraceListData &traceData ) = 0;
	virtual void    SetupLeafAndEntityListBox( const Vector &vecBoxMin, const Vector &vecBoxMax, CTraceListData &traceData ) = 0;
	virtual void	TraceRayAgainstLeafAndEntityList( const Ray_t &ray, CTraceListData &traceData, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// A version that sweeps a collideable through the world
	// abs start + abs end represents the collision origins you want to sweep the collideable through
	// vecAngles represents the collision angles of the collideable during the sweep
	virtual void	SweepCollideable( ICollideable *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
		const QAngle &vecAngles, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// Enumerates over all entities along a ray
	// If triggers == true, it enumerates all triggers along a ray
	virtual void	EnumerateEntities( const Ray_t &ray, bool triggers, IEntityEnumerator *pEnumerator ) = 0;

	// Same thing, but enumerate entitys within a box
	virtual void	EnumerateEntities( const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator ) = 0;

	// Convert a handle entity to a collideable.  Useful inside enumer
	virtual ICollideable *GetCollideable( IHandleEntity *pEntity ) = 0;

	// HACKHACK: Temp for performance measurments
	virtual int GetStatByIndex( int index, bool bClear ) = 0;


	//finds brushes in an AABB, prone to some false positives
	virtual void GetBrushesInAABB( const Vector &vMins, const Vector &vMaxs, CUtlVector<int> *pOutput, int iContentsMask = 0xFFFFFFFF ) = 0;

	//Creates a CPhysCollide out of all displacements wholly or partially contained in the specified AABB
	virtual CPhysCollide* GetCollidableFromDisplacementsInAABB( const Vector& vMins, const Vector& vMaxs ) = 0;

	//retrieve brush planes and contents, returns true if data is being returned in the output pointers, false if the brush doesn't exist
	virtual bool GetBrushInfo( int iBrush, CUtlVector<Vector4D> *pPlanesOut, int *pContentsOut ) = 0;

	// gets the number of displacements in the world
	HACKMGR_CLASS_API int GetNumDisplacements( );

	// gets a specific diplacement mesh
	HACKMGR_CLASS_API void GetDisplacementMesh( int nIndex, virtualmeshlist_t *pMeshTriList );
	
	//retrieve brush planes and contents, returns true if data is being returned in the output pointers, false if the brush doesn't exist
	HACKMGR_CLASS_API bool GetBrushInfo( int iBrush, CUtlVector<BrushSideInfo_t> *pBrushSideInfoOut, int *pContentsOut );

	virtual bool PointOutsideWorld( const Vector &ptTest ) = 0; //Tests a point to see if it's outside any playable area

	// Walks bsp to find the leaf containing the specified point
	virtual int GetLeafContainingPoint( const Vector &ptTest ) = 0;

	HACKMGR_CLASS_API ITraceListData *AllocTraceListData();
	HACKMGR_CLASS_API void FreeTraceListData(ITraceListData *);

	/// Used only in debugging: get/set/clear/increment the trace debug counter. See comment below for details.
	HACKMGR_CLASS_API int GetSetDebugTraceCounter( int value, DebugTraceCounterBehavior_t behavior );
};



#endif // ENGINE_IENGINETRACE_H
