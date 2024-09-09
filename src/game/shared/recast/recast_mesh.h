//====== Copyright � Sandern Corporation, All rights reserved. ===========//
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

enum SamplePartitionType
{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

/// These are just sample areas to use consistent values across the samples.
/// The use should specify these base on his needs.
enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	//SAMPLE_POLYAREA_WATER,
	//SAMPLE_POLYAREA_ROAD,
	//SAMPLE_POLYAREA_DOOR,
	//SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
	// Remainder are obstacle areas, up to the last valid area id 63
	// We use multiple ids so no neighboring obstacle gets the same id.
	SAMPLE_POLYAREA_OBSTACLE_START,
	SAMPLE_POLYAREA_OBSTACLE_END = SAMPLE_POLYAREA_OBSTACLE_START + 14
};

enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
	//SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
	//SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP		= 0x02,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED	= 0x04,		// Disabled polygon
	SAMPLE_POLYFLAGS_OBSTACLE_START = 0x10,
	SAMPLE_POLYFLAGS_OBSTACLE_END = 0x8000,
	SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
};

#define RECASTMESH_MAX_POLYS 256

class CRecastMesh
{
public:
	CRecastMesh();
	~CRecastMesh();

	const char *GetName();
	void Init( const char *name );
	void Init( const char *name, float agentRadius, float agentHeight, float agentMaxClimb, float agentMaxSlope );
	bool IsLoaded();

	static bool ComputeMeshSettings( const char *pMeshName, 
		float &fAgentRadius, float &fAgentHeight, float &fAgentMaxClimb, float &fAgentMaxSlope,
		float &fCellSize, float &fCellHeight, float &fTileSize );

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
	int GetPolyRef( const Vector &vPoint, float fBeneathLimit = 120.0f, float fExtent2D = 256.0f );
	bool IsValidPolyRef( int polyRef );
	Vector ClosestPointOnMesh( const Vector &vPoint, float fBeneathLimit = 120.0f, float fRadius = 256.0f );
	Vector RandomPointWithRadius( const Vector &vCenter, float fRadius, const Vector *pStartPoint = NULL );
	float IsAreaFlat( const Vector &vCenter, const Vector &vExtents, float fSlope = 0 );

#ifndef CLIENT_DLL
	// Path find functions
	AI_Waypoint_t *FindPath( const Vector &vStart, const Vector &vEnd, float fBeneathLimit = 120.0f, CBaseEntity *pTarget = NULL, 
		bool *bIsPartial = NULL, const Vector *pStartTestPos = NULL );
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

	void SharedInit(  const char *name );

	// Building
	bool DisableUnreachablePolygons( const CUtlVector< Vector > &samplePositions );
	bool RemoveUnreachablePoly( CMapMesh *pMapMesh );

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
	int m_partitionType;

	int m_maxTiles;
	int m_maxPolysPerTile;
	float m_tileSize;

	float m_cacheBuildTimeMs;
	int m_cacheCompressedSize;
	int m_cacheRawSize;
	int m_cacheLayerCount;
	int m_cacheBuildMemUsage;

	unsigned short m_allObstacleFlags;

private:
	CUtlString m_Name;
	
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

	dtQueryFilter m_defaultFilter;
	dtQueryFilter m_obstacleFilter;
	dtQueryFilter m_allFilter;
	pathfind_resultdata_t m_pathfindData;
};

inline const char *CRecastMesh::GetName()
{
	return m_Name.Get();
}

inline bool CRecastMesh::IsLoaded()
{
	return m_tileCache != NULL;
}

#endif // RECAST_MESH_H