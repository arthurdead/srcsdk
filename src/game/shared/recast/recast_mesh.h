//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MESH_H
#define RECAST_MESH_H

#pragma once

#include <Recast.h>
#include "DetourNavMeshQuery.h"
#include "DetourTileCache.h"
#include "mathlib/vector.h"
#include "tier1/utlvector.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"
#include "ai_waypoint.h"
#include "recast/recast_imgr.h"
#include "mathlib/extent.h"
#include "worldsize.h"
#include "DetourCommon.h"
#include "DetourTileCacheBuilder.h"
#include "recast_mgr.h"

#ifdef GAME_DLL
class CBaseEntity;
#else
#define CBaseEntity C_BaseEntity
class C_BaseEntity;
#endif

class CMapMesh;
class NavmeshFlags;

class dtNavMesh;
class dtTileCache;

// Number of nodes used during pathfinding.
// For very large maps we might need a larger number.
#define RECAST_NAVQUERY_MAX_NODES 4096
// Mainly selecting the nearest enemy. The ones who return a path to an enemy
#define RECAST_NAVQUERY_LIMITED_NODES 128

enum RecastPartitionType
{
	RECAST_PARTITION_WATERSHED,
	RECAST_PARTITION_MONOTONE,
	RECAST_PARTITION_LAYERS,
};

/// These are just sample areas to use consistent values across the samples.
/// The user should specify these based on his needs.
enum PolyAreas : unsigned char
{
	POLYAREA_NONE = 0,

	POLYAREA_GROUND,
	POLYAREA_WATER,
	POLYAREA_ROAD,
	POLYAREA_DOOR,
	POLYAREA_GRASS,
	POLYAREA_JUMP,

	// Remainder are obstacle areas, up to the last valid area id 63
	// We use multiple ids so no neighboring obstacle gets the same id.
	POLYAREA_OBSTACLE_START,
	POLYAREA_OBSTACLE_END = POLYAREA_OBSTACLE_START + 14,

	POLYAREA_RESERVED1 = DT_TILECACHE_WALKABLE_AREA,
};

enum PolyFlags : unsigned short
{
	POLYFLAGS_NONE =                 0,
	POLYFLAGS_RESERVED1 =      (1 << 0),
	POLYFLAGS_DISABLED =       (1 << 1), // Disabled polygon

	POLYFLAGS_WALK =           (1 << 2), // Ability to walk (ground, grass, road)
	POLYFLAGS_SWIM =           (1 << 3), // Ability to swim (water).
	POLYFLAGS_DOOR =           (1 << 4), // Ability to move through doors.
	POLYFLAGS_JUMP =           (1 << 5), // Ability to jump.

	POLYFLAGS_MASK_ALL = 0x3C,

	POLYFLAGS_OBSTACLE_START = (1 << 6),
	POLYFLAGS_OBSTACLE_END =   (1 << 15),

	POLYFLAGS_MASK_OBSTACLES = 0xFFE0,
};

#define RECASTMESH_MAX_POLYS 256

#ifndef DT_VIRTUAL_QUERYFILTER
	#error
#endif

class CRecastQueryFilter : public dtQueryFilter
{
public:
	typedef dtQueryFilter BaseClass;

	CRecastQueryFilter();
	explicit CRecastQueryFilter( PolyFlags flags, bool include );
	CRecastQueryFilter( PolyFlags include, PolyFlags exclude );
};

extern CRecastQueryFilter defaultQueryFilter;
extern CRecastQueryFilter allQueryFilter;
extern CRecastQueryFilter obstacleQueryFilter;

class CRecastMesh;
class CRecastMgr;

//=============================================================================
//	>> CAI_Hull
//=============================================================================
namespace NAI_Hull
{
	const Vector &Mins(NavMeshType_t type);
	const Vector &Maxs(NavMeshType_t type);

	float Radius(NavMeshType_t type);
	float Radius2D(NavMeshType_t type);

	float Width(NavMeshType_t type);
	float Height(NavMeshType_t type);
	float Length(NavMeshType_t type);

	const char *Name(NavMeshType_t type);

	NavMeshType_t LookupId(const char *szName);

	MapMeshType_t MapMeshType(NavMeshType_t type);

	unsigned int TraceMask(NavMeshType_t type);
	unsigned int TraceMask(MapMeshType_t type);
}

//--------------------------------------------------------------------------------------------------------------
/**
 * A HidingSpot is a good place for a bot to crouch and wait for enemies
 */
class HidingSpot
{
public:
	virtual ~HidingSpot()	{ }

	enum 
	{ 
		IN_COVER			= 0x01,							// in a corner with good hard cover nearby
		GOOD_SNIPER_SPOT	= 0x02,							// had at least one decent sniping corridor
		IDEAL_SNIPER_SPOT	= 0x04,							// can see either very far, or a large area, or both
		EXPOSED				= 0x08							// spot in the open, usually on a ledge or cliff
	};

	bool HasGoodCover( void ) const			{ return (m_flags & IN_COVER) ? true : false; }	// return true if hiding spot in in cover
	bool IsGoodSniperSpot( void ) const		{ return (m_flags & GOOD_SNIPER_SPOT) ? true : false; }
	bool IsIdealSniperSpot( void ) const	{ return (m_flags & IDEAL_SNIPER_SPOT) ? true : false; }
	bool IsExposed( void ) const			{ return (m_flags & EXPOSED) ? true : false; }	

	int GetFlags( void ) const		{ return m_flags; }

	void Save( CUtlBuffer &fileBuffer, unsigned int version ) const;
	void Load( CUtlBuffer &fileBuffer, unsigned int version );
	bool PostLoad( void );

	const Vector &GetPosition( void ) const		{ return m_pos; }	// get the position of the hiding spot
	unsigned int GetID( void ) const			{ return m_id; }
	dtPolyRef GetPoly( void ) const		{ return m_poly; }	// return nav area this hiding spot is within

	void Mark( void )							{ m_marker = m_masterMarker[m_navMeshType]; }
	bool IsMarked( void ) const					{ return (m_marker == m_masterMarker[m_navMeshType]) ? true : false; }
	static void ChangeMasterMarker( NavMeshType_t type )		{ ++m_masterMarker[type]; }

public:
	void SetFlags( int flags )				{ m_flags |= flags; }	// FOR INTERNAL USE ONLY
	void SetPosition( const Vector &pos )	{ m_pos = pos; }		// FOR INTERNAL USE ONLY

private:
	friend class CRecastMesh;
	friend class CRecastMgr;
	friend void ClassifySniperSpot( HidingSpot *spot );

	HidingSpot( NavMeshType_t type );										// must use factory to create

	NavMeshType_t m_navMeshType;

	Vector m_pos;											// world coordinates of the spot
	unsigned int m_id;										// this spot's unique ID
	unsigned int m_marker;									// this spot's unique marker
	dtPolyRef m_poly;										// the nav area containing this hiding spot

	unsigned char m_flags;									// bit flags

	static unsigned int m_nextID[RECAST_NAVMESH_NUM];							// used when allocating spot ID's
	static unsigned int m_masterMarker[RECAST_NAVMESH_NUM];						// used to mark spots
};
typedef CUtlVectorUltraConservative< HidingSpot * > HidingSpotVector;
extern HidingSpotVector TheHidingSpots[RECAST_NAVMESH_NUM];

extern HidingSpot *GetHidingSpotByID( NavMeshType_t type, unsigned int id );

//--------------------------------------------------------------------------------------------------------------
/**
 * Stores a pointer to an interesting "spot", and a parametric distance along a path
 */
struct SpotOrder
{
	float t;						// parametric distance along ray where this spot first has LOS to our path
	union
	{
		HidingSpot *spot;			// the spot to look at
		unsigned int id;			// spot ID for save/load
	};
};
typedef CUtlVector< SpotOrder > SpotOrderVector;

class CRecastMesh
{
public:
	CRecastMesh( NavMeshType_t type );
	~CRecastMesh();

	const char *GetName() const;
	NavMeshType_t GetType() const;
	MapMeshType_t GetMapType() const;
	void Init();
	bool IsLoaded() const;

	virtual void Update( float dt );

	// Load/build 
	virtual bool Load( CUtlBuffer &fileBuffer, CMapMesh *pMapMesh = NULL );
	bool Reset();

#ifndef CLIENT_DLL
	virtual bool Build( CMapMesh *pMapMesh );
	virtual bool Save( CUtlBuffer &fileBuffer );

	virtual bool RebuildPartial( CMapMesh *pMapMesh, const Vector &vMins, const Vector &vMaxs );
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
	virtual void DebugRender();
#endif // CLIENT_DLL

	// Mesh Querying
	dtPolyRef GetPolyRef( const Vector &vPoint, float fBeneathLimit = 120.0f, float fExtent2D = 256.0f );
	bool IsValidPolyRef( dtPolyRef polyRef ) const;
	Vector ClosestPointOnMesh( const Vector &vPoint, float fBeneathLimit = 120.0f, float fRadius = 256.0f );
	bool ClosestPointOnMesh( const Vector &vPoint, Vector &out, float fBeneathLimit = 120.0f, float fRadius = 256.0f );
	Vector RandomPointWithRadius( const Vector &vCenter, float fRadius, const Vector *pStartPoint = NULL );
	float IsAreaFlat( const Vector &vCenter, const Vector &vExtents, float fSlope = 0 );

	Place GetPlace( const Vector &pos ) const;							// return Place at given coordinate

	PolyFlags GetPolyFlags( dtPolyRef polyRef ) const;

	bool GetPolyExtent( dtPolyRef polyRef, Extent *extent ) const;						// return a computed extent (XY is in m_nwCorner and m_seCorner, Z is computed)

	inline float GetPolyZ( dtPolyRef polyRef, const Vector * RESTRICT pPos ) const RESTRICT;			// return Z of area at (x,y) of 'pos'
	inline float GetPolyZ( dtPolyRef polyRef, const Vector &pos ) const RESTRICT;						// return Z of area at (x,y) of 'pos'
	float GetPolyZ( dtPolyRef polyRef, float x, float y ) const RESTRICT;				// return Z of area at (x,y) of 'pos'

	//-------------------------------------------------------------------------------------
	/**
	 * Apply the functor to all navigation areas.
	 * If functor returns false, stop processing and return false.
	 */
	template < typename Functor >
	bool ForAllPolys( Functor &func )
	{
		float extents[3];
		extents[0] = MAX_COORD_FLOAT;
		extents[1] = MAX_COORD_FLOAT;
		extents[2] = MAX_COORD_FLOAT;

		dtPolyRef polyRefs[RECASTMESH_MAX_POLYS];
		int polyCount;

		dtStatus status = m_navQuery->queryPolygons( m_navMesh->getParams()->orig, extents, &defaultQueryFilter, polyRefs, &polyCount, ARRAYSIZE(polyRefs) );
		if( !dtStatusSucceed( status ) )
		{
			return false;
		}

		for(int i = 0; i < polyCount; ++i) {
			if(!func( this, polyRefs[i] ))
				return false;
		}

		return true;
	}

#ifndef CLIENT_DLL
	// Path find functions
	AI_Waypoint_t *FindPath( const Vector &vStart, const Vector &vEnd, float fBeneathLimit = 120.0f, CBaseEntity *pTarget = NULL, 
		bool *bIsPartial = NULL, const Vector *pStartTestPos = NULL );
	AI_Waypoint_t *FindPath( dtPolyRef startRef, const Vector &vStart, dtPolyRef endRef, const Vector &vEnd, CBaseEntity *pTarget = NULL, 
		bool *bIsPartial = NULL );
#endif // CLIENT_DLL
	bool TestRoute( const Vector &vStart, const Vector &vEnd );
	float FindPathDistance( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget = NULL, float fBeneathLimit = 120.0f, bool bLimitedSearch = false );

	// Obstacle management
	dtObstacleRef AddTempObstacle( const Vector &vPos, float radius, float height, unsigned char areaId );
	dtObstacleRef AddTempObstacle( const Vector &vPos, const Vector *convexHull, const int numConvexHull, float height, unsigned char areaId );
	bool RemoveObstacle( const dtObstacleRef ref );

	// Accessors for debugging purposes only
	dtNavMesh *GetNavMesh() { return m_navMesh; }
	dtNavMeshQuery *GetNavMeshQuery() { return m_navQuery; }

	// Accessors for intended unit radius and height using this mesh
	float GetAgentRadius() { return m_agentRadius; }
	float GetAgentHeight() { return m_agentHeight; }
	float GetAgentMaxClimb() { return m_agentMaxClimb; }
	float GetAgentMaxSlope() { return m_agentMaxSlope; }

	// Getters/setters for various build settings:
	float GetCellSize() { return m_cellSize; }
	void SetCellSize( float cellSize ) { m_cellSize = cellSize; }
	float GetCellHeight() { return m_cellHeight; }
	void SetCellHeight( float cellHeight ) { m_cellHeight = cellHeight; }
	float GetTileSize() { return m_tileSize; }
	void SetTileSize( float tileSize ) { m_tileSize = tileSize; }

private:
	virtual void PostLoad();

	// Building
	bool DisableUnreachablePolygons( const CUtlVector< Vector > &samplePositions );
	bool RemoveUnreachablePoly( CMapMesh *pMapMesh );

	HidingSpot *CreateHidingSpot( void ) const;					// Hiding Spot factory

private:
	// Result data for path finding
	typedef struct pathfind_resultdata_t {
		pathfind_resultdata_t() : cacheValid(false), startRef(0), endRef(0), npolys(0), straightPathOptions(0), nstraightPath(0) {}

		// For caching purposes, keep the start/end information around for which the path was computed
		bool cacheValid;
		dtPolyRef startRef;
		dtPolyRef endRef;

		// Data from findPath
		dtPolyRef polys[RECASTMESH_MAX_POLYS];
		int npolys;
		bool isPartial;

		// Data from findStraightPath
		int straightPathOptions;
		float straightPath[RECASTMESH_MAX_POLYS*3];
		unsigned char straightPathFlags[RECASTMESH_MAX_POLYS];
		dtPolyRef straightPathPolys[RECASTMESH_MAX_POLYS];
		int nstraightPath;
	} pathfind_resultdata_t;

	bool CanUseCachedPath( dtPolyRef startRef, dtPolyRef endRef, pathfind_resultdata_t &findpathData );
	dtStatus ComputeAdjustedStartAndEnd( dtNavMeshQuery *navQuery, float spos[3], float epos[3], dtPolyRef &startRef, dtPolyRef &endRef, 
		float fBeneathLimit, bool bHasTargetAndIsObstacle = false, bool bDisallowBigPicker = false, const Vector *pStartTestPos = NULL );
	dtStatus DoFindPath( dtNavMeshQuery *navQuery, dtPolyRef startRef, dtPolyRef endRef, float spos[3], float epos[3], 
		bool bHasTargetAndIsObstacle, pathfind_resultdata_t &findpathData );

#ifndef CLIENT_DLL
	AI_Waypoint_t *ConstructWaypointsFromStraightPath( pathfind_resultdata_t &findpathData );
#endif // CLIENT_DLL

protected:

	float m_cellSize;
	float m_cellHeight;

	float m_agentHeight;
	float m_agentRadius;

	float m_agentMaxClimb;
	float m_agentMaxSlope;

	float m_regionMinSize;
	float m_regionMergeSize;
	float m_edgeMaxLen;
	float m_edgeMaxError;
	float m_vertsPerPoly;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	RecastPartitionType m_partitionType;

	int m_maxTiles;
	int m_maxPolysPerTile;
	float m_tileSize;

	float m_cacheBuildTimeMs;
	int m_cacheCompressedSize;
	int m_cacheRawSize;
	int m_cacheLayerCount;
	int m_cacheBuildMemUsage;

private:
	NavMeshType_t m_Type;
	MapMeshType_t m_MapType;
	
	// Data used during build
	rcConfig m_cfg;

	struct dtTileCacheAlloc* m_talloc;
	struct dtTileCacheCompressor* m_tcomp;
	struct dtTileCacheMeshProcess *m_tmproc;

	// Data used for path finding
	dtNavMesh* m_navMesh;
	dtTileCache* m_tileCache;
	dtNavMeshQuery* m_navQuery;
	dtNavMeshQuery* m_navQueryLimitedNodes;

	pathfind_resultdata_t m_pathfindData;

	HidingSpotVector m_HidingSpots;
};

//--------------------------------------------------------------------------------------------------------------
/**
* Return Z of area at (x,y) of 'pos'
* Trilinear interpolation of Z values at quad edges.
* NOTE: pos->z is not used.
*/
inline float CRecastMesh::GetPolyZ( dtPolyRef polyRef, const Vector * RESTRICT pos ) const RESTRICT
{
	return GetPolyZ( polyRef, pos->x, pos->y );
}

inline float CRecastMesh::GetPolyZ( dtPolyRef polyRef, const Vector & pos ) const RESTRICT
{
	return GetPolyZ( polyRef, pos.x, pos.y );
}

inline const char *CRecastMesh::GetName() const
{
	return NAI_Hull::Name( m_Type );
}

inline NavMeshType_t CRecastMesh::GetType() const
{
	return m_Type;
}

inline MapMeshType_t CRecastMesh::GetMapType() const
{
	return m_MapType;
}

inline bool CRecastMesh::IsLoaded() const
{
	return m_tileCache != NULL;
}

#endif // RECAST_MESH_H