//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// TODO: 
// - Merge common code FindPath and FindPathDistance
// - Fix case with finding the shortest path to neutral obstacle (might try to go from other side)
// - Cache paths computed for unit type in same frame?
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"
#include "recast/recast_mgr.h"
#include "mathlib/extent.h"
#include "worldsize.h"

#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#else
#include "recast/recast_imapmesh.h"
#include "recast/recast_recastdebugdraw.h"
#include "recast/recast_debugdrawmesh.h"
#include "recast/recast_detourdebugdraw.h"
#include "recast_imgr.h"
#endif // CLIENT_DLL

#include "recast/recast_tilecache_helpers.h"
#include "recast/recast_common.h"

#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourCommon.h"

#include "game_loopback/igameserverloopback.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar recast_findpath_debug( "recast_findpath_debug", "0", FCVAR_REPLICATED|FCVAR_CHEAT, "" );
ConVar recast_areaslope_debug( "recast_areaslope_debug", "0", FCVAR_REPLICATED|FCVAR_CHEAT, "" );

// Defaults
static ConVar recast_findpath_use_caching( "recast_findpath_use_caching", "1", FCVAR_REPLICATED|FCVAR_CHEAT );

CRecastQueryFilter::CRecastQueryFilter()
{
	// Change costs.
	setAreaCost(POLYAREA_GROUND, 1.0f);
	setAreaCost(POLYAREA_WATER, 10.0f);
	setAreaCost(POLYAREA_ROAD, 1.0f);
	setAreaCost(POLYAREA_DOOR, 1.0f);
	setAreaCost(POLYAREA_GRASS, 2.0f);
	setAreaCost(POLYAREA_JUMP, 1.5f);
}

CRecastQueryFilter::CRecastQueryFilter( PolyFlags flags, bool include )
	: CRecastQueryFilter()
{
	if(include)
		setIncludeFlags( flags );
	else
		setExcludeFlags( flags );
}

CRecastQueryFilter::CRecastQueryFilter( PolyFlags include, PolyFlags exclude )
	: CRecastQueryFilter()
{
	setIncludeFlags( include );
	setExcludeFlags( exclude );
}

CRecastQueryFilter defaultQueryFilter( (PolyFlags)POLYFLAGS_MASK_ALL, (PolyFlags)(POLYFLAGS_DISABLED|POLYFLAGS_MASK_OBSTACLES) );
CRecastQueryFilter allQueryFilter( (PolyFlags)(POLYFLAGS_MASK_ALL|POLYFLAGS_MASK_OBSTACLES), (PolyFlags)POLYFLAGS_DISABLED );
CRecastQueryFilter obstacleQueryFilter( (PolyFlags)POLYFLAGS_MASK_OBSTACLES, (PolyFlags)POLYFLAGS_DISABLED );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct ai_hull_t
{
	ai_hull_t( const char *pName, const Vector &_mins, const Vector &_maxs )
		: mins( _mins ), maxs( _maxs ), name( pName )
	{
		Vector vecSize;
		VectorSubtract( maxs, mins, vecSize );
		radius = vecSize.Length() * 0.5f;
		radius2d = vecSize.Length2D() * 0.5f;

		width = vecSize.y;
		height = vecSize.z;
		length = vecSize.x;
	}

	const char*	name;

	Vector mins;
	Vector maxs;

	float radius;
	float radius2d;

	float width;
	float height;
	float length;
};

static const ai_hull_t hull[]{
	ai_hull_t("HUMAN", Vector(-13, -13, 0), Vector(13, 13, 72) ), // Combine, Stalker, Zombie...
	ai_hull_t("SMALL_CENTERED", Vector(-20, -20, -20), Vector(20, 20, 20) ), // Scanner
	ai_hull_t("WIDE_HUMAN", Vector(-15, -15, 0), Vector(15, 15, 72) ), // Vortigaunt
	ai_hull_t("TINY", Vector(-12, -12, 0), Vector(12, 12, 24) ), // Headcrab
	ai_hull_t("WIDE_SHORT", Vector(-35, -35, 0), Vector(35, 35, 32) ), // Bullsquid
	ai_hull_t("MEDIUM", Vector(-16, -16, 0), Vector(16, 16, 64) ), // Cremator
	ai_hull_t("TINY_CENTERED", Vector(-8, -8, -4), Vector(8, 8, 4) ), // Manhack 
	ai_hull_t("LARGE", Vector(-40, -40, 0), Vector(40, 40, 100) ), // Antlion Guard
	ai_hull_t("LARGE_CENTERED", Vector(-38, -38, -38), Vector(38, 38, 38) ), // Mortar Synth
	ai_hull_t("MEDIUM_TALL", Vector(-18, -18, 0), Vector(18, 18, 100) ), // Hunter
	ai_hull_t("TINY_FLUID", Vector(-6.5, -6.5, 0), Vector(6.5, 6.5, 13) ), // Blob
	ai_hull_t("MEDIUMBIG", Vector(-17, -17, 0), Vector(17, 17, 69) ), // Infested drone
};

COMPILE_TIME_ASSERT(ARRAYSIZE(hull) == (RECAST_NAVMESH_NUM-1));
COMPILE_TIME_ASSERT(RECAST_NAVMESH_PLAYER == ARRAYSIZE(hull));

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
NavMeshType_t NAI_Hull::LookupId(const char *szName)
{
	if(V_strnicmp(szName, "RECAST_NAVMESH_", 15) == 0) {
		szName += 15;
	}

	if(V_stricmp(szName, "PLAYER") == 0) {
		return ARRAYSIZE(hull);
	}

	for(int i = 0; i < ARRAYSIZE(hull); ++i) {
		if(V_stricmp(szName, hull[i].name) == 0) {
			return (NavMeshType_t)i;
		}
	}

	return RECAST_NAVMESH_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const char *NAI_Hull::Name(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return "RECAST_NAVMESH_INVALID";

	if(type == ARRAYSIZE(hull))
		return "RECAST_NAVMESH_PLAYER";

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].name;

	return "RECAST_NAVMESH_INVALID";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::CRecastMesh( NavMeshType_t type ) :
	m_navMesh(NULL),
	m_tileCache(NULL),
	m_cacheBuildTimeMs(0),
	m_cacheCompressedSize(0),
	m_cacheRawSize(0),
	m_cacheLayerCount(0),
	m_cacheBuildMemUsage(0),
	m_navQuery(NULL),
	m_navQueryLimitedNodes(NULL),
	m_talloc(NULL),
	m_tcomp(NULL),
	m_tmproc(NULL)
{
	m_Type = type;
	m_MapType = NAI_Hull::MapMeshType( m_Type );

	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12000.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 600.0f;
	m_detailSampleMaxError = 100.0f;
	m_partitionType = RECAST_PARTITION_WATERSHED;

	m_maxTiles = 0;
	m_maxPolysPerTile = 0;
	m_tileSize = 48;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::~CRecastMesh()
{
	Reset();

	if( m_navQuery )
	{
		dtFreeNavMeshQuery( m_navQuery );
		m_navQuery = NULL;
	}

	if( m_navQueryLimitedNodes )
	{
		dtFreeNavMeshQuery( m_navQueryLimitedNodes );
		m_navQueryLimitedNodes = NULL;
	}

	if( m_talloc )
	{
		delete m_talloc;
		m_talloc = NULL;
	}

	if( m_tcomp )
	{
		delete m_tcomp;
		m_tcomp = NULL;
	}

	if( m_tmproc )
	{
		delete m_tmproc;
		m_tmproc = NULL;
	}
}

MapMeshType_t NAI_Hull::MapMeshType(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_TINY_FLUID)
		return RECAST_MAPMESH_NPC_FLUID;

	if(type == ARRAYSIZE(hull))
		return RECAST_MAPMESH_PLAYER;

	return RECAST_MAPMESH_NPC;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
unsigned int NAI_Hull::TraceMask(NavMeshType_t type)
{
	return TraceMask( MapMeshType( type ) );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
unsigned int NAI_Hull::TraceMask(MapMeshType_t type)
{ 
	switch (type) {
	case RECAST_MAPMESH_NPC:
		return MASK_NPCWORLDSTATIC;
	case RECAST_MAPMESH_NPC_FLUID:
		return MASK_NPCWORLDSTATIC_FLUID;
	case RECAST_MAPMESH_PLAYER:
		return MASK_PLAYERWORLDSTATIC;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::Mins(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return vec3_origin;

	if(type == ARRAYSIZE(hull))
		return VEC_HULL_MIN;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].mins;

	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector &NAI_Hull::Maxs(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return vec3_origin;

	if(type == ARRAYSIZE(hull))
		return VEC_HULL_MAX;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].maxs;

	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Height(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return 0.0f;

	if(type == ARRAYSIZE(hull))
		return VIEW_VECTORS->m_flHeight;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].height;

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Length(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return 0.0f;

	if(type == ARRAYSIZE(hull))
		return VIEW_VECTORS->m_flLength;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].length;

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float NAI_Hull::Width(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return 0.0f;

	if(type == ARRAYSIZE(hull))
		return VIEW_VECTORS->m_flWidth;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].width;

	return 0.0f;
}

float NAI_Hull::Radius(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return 0.0f;

	if(type == ARRAYSIZE(hull))
		return VIEW_VECTORS->m_flRadius;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].radius;

	return 0.0f;
}

float NAI_Hull::Radius2D(NavMeshType_t type)
{
	if(type == RECAST_NAVMESH_INVALID)
		return 0.0f;

	if(type == ARRAYSIZE(hull))
		return VIEW_VECTORS->m_flRadius2D;

	if(type >= 0 && type < ARRAYSIZE(hull))
		return hull[type].radius2d;

	return 0.0f;
}

void CRecastMesh::Init()
{
	HidingSpot::m_nextID[ m_Type ] = 1;
	HidingSpot::m_masterMarker[ m_Type ] = 0;

	m_navQuery = dtAllocNavMeshQuery();
	m_navQueryLimitedNodes = dtAllocNavMeshQuery();

	m_talloc = new LinearAllocator( 96000 );
	m_tcomp = new FastLZCompressor;
	m_tmproc = new MeshProcess( m_Type );

	// Per type
	m_agentMaxSlope = 45.573; // Default slope for units

	m_tileSize = 48;

	m_agentHeight = NAI_Hull::Height( m_Type );
	m_agentRadius = NAI_Hull::Radius2D( m_Type );

	//m_agentMaxClimb = 18.0f;
	m_agentMaxClimb = 25.0f;

	m_cellSize = ( m_agentRadius / 3.0f );

	m_cellHeight = ( m_cellSize / 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::PostLoad()
{
	// allow hiding spots to compute information
	FOR_EACH_VEC( TheHidingSpots[ m_Type ], hit )
	{
		HidingSpot *spot = TheHidingSpots[ m_Type ][ hit ];
		spot->PostLoad();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::Update( float dt )
{
	if( !IsLoaded() )
	{
		return;
	}

	// Will update obstacles, invalidates the last found path
	m_pathfindData.cacheValid = false;

	dtStatus status = m_tileCache->update( dt, m_navMesh );
	if( !dtStatusSucceed( status ) )
	{
		Warning("CRecastMesh::Update failed: \n");
		if( status & DT_OUT_OF_MEMORY )
			Warning("\tOut of memory. Consider increasing LinearAllocator buffer size.\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Reset()
{
	// Cleanup Nav mesh data
	if( m_navMesh )
	{
		dtFreeNavMesh(m_navMesh);
		m_navMesh = 0;
	}

	if( m_tileCache )
	{
		dtFreeTileCache(m_tileCache);
		m_tileCache = 0;
	}

	HidingSpot::m_nextID[ m_Type ] = 1;
	HidingSpot::m_masterMarker[ m_Type ] = 0;

	// free all the HidingSpots
	FOR_EACH_VEC( TheHidingSpots[ m_Type ], hit )
	{
		delete TheHidingSpots[ m_Type ][ hit ];
	}

	TheHidingSpots[ m_Type ].RemoveAll();

	return true;
}

#ifdef CLIENT_DLL
ConVar recast_draw_trimeshslope("recast_draw_trimeshslope", "0", FCVAR_CHEAT, "" );
ConVar recast_draw_navmesh("recast_draw_navmesh", "0", FCVAR_CHEAT, "" );
ConVar recast_draw_server("recast_draw_server", "1", FCVAR_CHEAT, "" );

//-----------------------------------------------------------------------------
// Purpose: Draws the mesh
//-----------------------------------------------------------------------------
void CRecastMesh::DebugRender()
{
	DebugDrawMesh dd;

	if( recast_draw_trimeshslope.GetBool() )
	{
		const float texScale = 1.0f / (m_cellSize * 10.0f);

		IRecastMgr *pRecastMgr = GetGameServerLoopback() ? GetGameServerLoopback()->GetRecastMgr() : NULL;
		if( pRecastMgr )
		{
			IMapMesh *pMapMesh = pRecastMgr->GetMapMesh( m_MapType );
			if( pMapMesh && pMapMesh->GetNorms() )
			{
				duDebugDrawTriMeshSlope(&dd, pMapMesh->GetVerts(), pMapMesh->GetNumVerts(),
					pMapMesh->GetTris(), pMapMesh->GetNorms(), pMapMesh->GetNumTris(),
					m_agentMaxSlope, texScale);
			}
		}
	}

	if( recast_draw_navmesh.GetBool() )
	{
		dtNavMesh *navMesh = NULL;
		dtNavMeshQuery *navQuery = NULL;
		if( recast_draw_server.GetBool() )
		{
			IRecastMgr *pRecastMgr = GetGameServerLoopback() ? GetGameServerLoopback()->GetRecastMgr() : NULL;
			if( pRecastMgr ) {
				navMesh = pRecastMgr->GetNavMesh( m_Type );
				navQuery = pRecastMgr->GetNavMeshQuery( m_Type );
			}
		}
		else
		{
			navMesh = m_navMesh;
			navQuery = m_navQuery;
		}

		if( navMesh != NULL && navQuery != NULL )
		{
			char m_navMeshDrawFlags = DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_CLOSEDLIST;

			duDebugDrawNavMeshWithClosedList(&dd, *navMesh, *navQuery, m_navMeshDrawFlags);
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtPolyRef CRecastMesh::GetPolyRef( const Vector &vPoint, float fBeneathLimit, float fExtent2D )
{
	if( !IsLoaded() )
		return 0;

	float pos[3];
	pos[0] = vPoint[0];
	pos[1] = vPoint[2];
	pos[2] = vPoint[1];

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = fExtent2D;
	polyPickExt[1] = fBeneathLimit;
	polyPickExt[2] = fExtent2D;

	dtPolyRef ref;
	dtStatus status = m_navQuery->findNearestPoly(pos, polyPickExt, &defaultQueryFilter, &ref, NULL);
	if( !dtStatusSucceed( status ) )
	{
		return 0;
	}
	return ref;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::IsValidPolyRef( dtPolyRef polyRef ) const
{
	if( !IsLoaded() )
		return false;

	return polyRef >= 0 && m_navQuery->isValidPolyRef( (dtPolyRef)polyRef, &defaultQueryFilter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::ClosestPointOnMesh( const Vector &vPoint, Vector &out, float fBeneathLimit, float fRadius )
{
	if( !IsLoaded() )
		return false;

	float pos[3];
	float center[3];
	pos[0] = vPoint[0];
	pos[1] = vPoint[2];
	pos[2] = vPoint[1];

	center[0] = pos[0];
	center[1] = pos[1] - (fBeneathLimit / 2.0f) + 32.0f;
	center[2] = pos[2];

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = fRadius;
	polyPickExt[1] = (fBeneathLimit + 32.0f) / 2.0f;
	polyPickExt[2] = fRadius;

	dtPolyRef closestRef;
	dtStatus status = m_navQuery->findNearestPoly(center, polyPickExt, &defaultQueryFilter, &closestRef, NULL, pos);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	float closest[3];
	status = m_navQuery->closestPointOnPoly(closestRef, pos, closest, NULL/*, &posOverPoly*/);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	out = Vector( closest[0], closest[2], closest[1] );
	return true;
}

Vector CRecastMesh::ClosestPointOnMesh( const Vector &vPoint, float fBeneathLimit, float fRadius )
{
	Vector out;
	if(!ClosestPointOnMesh(vPoint, out, fBeneathLimit, fRadius))
		return vec3_origin;
	return out;
}

// Returns a random number [0..1)
static float frand()
{
//	return ((float)(rand() & 0xffff)/(float)0xffff);
//	return (float)rand()/(float)RAND_MAX;
	return random_valve->RandomFloat(0.0f, 1.0f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CRecastMesh::RandomPointWithRadius( const Vector &vCenter, float fRadius, const Vector *pStartPoint )
{
	if( !IsLoaded() )
		return vec3_origin;

	float pos[3];
	float center[3];
	center[0] = vCenter[0];
	center[1] = vCenter[2];
	center[2] = vCenter[1];

	if( pStartPoint )
	{
		pos[0] = (*pStartPoint)[0];
		pos[1] = (*pStartPoint)[2];
		pos[2] = (*pStartPoint)[1];
	}
	else
	{
		dtVcopy(pos, center);
	}

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = 256.0f;
	polyPickExt[1] = MAX_COORD_FLOAT;
	polyPickExt[2] = 256.0f;

	dtPolyRef closestRef;
	dtStatus status = m_navQuery->findNearestPoly( pos, polyPickExt, &defaultQueryFilter, &closestRef, NULL );
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	dtPolyRef eRef;
	float epos[3];
	status = m_navQuery->findRandomPointAroundCircle( closestRef, center, fRadius, &defaultQueryFilter, frand, &eRef, epos );
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	return Vector( epos[0], epos[2], epos[1] );
}

static void calcTriNormal(const float* v0, const float* v1, const float* v2, float* norm)
{
	float e0[3], e1[3];
	rcVsub(e0, v1, v0);
	rcVsub(e1, v2, v0);
	rcVcross(norm, e0, e1);
	rcVnormalize(norm);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CRecastMesh::IsAreaFlat( const Vector &vCenter, const Vector &vExtents, float fSlope )
{
	dtStatus status;

	float center[3];
	center[0] = vCenter.x;
	center[1] = vCenter.z;
	center[2] = vCenter.y;

	float extents[3];
	extents[0] = vExtents.x;
	extents[1] = vExtents.z;
	extents[2] = vExtents.y;

	dtPolyRef polys[RECASTMESH_MAX_POLYS];
	int npolys = 0;

	status = m_navQuery->queryPolygons( center, extents, &defaultQueryFilter, polys, &npolys, RECASTMESH_MAX_POLYS );
	if( !dtStatusSucceed( status ) )
	{
		return 0;
	}

	float norm[3];
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;

	for( int i = 0; i < npolys; i++ )
	{
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		m_navMesh->getTileAndPolyByRefUnsafe( polys[i], &tile, &poly );

		if (poly->getType() == DT_POLYTYPE_GROUND)
		{
			for( int j = 2; j < poly->vertCount; ++j )
			{
				const float* va = &tile->verts[poly->verts[0]*3];
				const float* vb = &tile->verts[poly->verts[j-1]*3];
				const float* vc = &tile->verts[poly->verts[j]*3];

				calcTriNormal( va, vb, vc, norm);

				if( recast_areaslope_debug.GetBool() )
				{
					Vector vPoint( (va[0] + vb[0] + vc[0]) / 3.0f, (va[2] + vb[2] + vc[2]) / 3.0f, (va[1] + vb[1] + vc[1]) / 3.0f );
					char buf[256];
					V_snprintf( buf, sizeof( buf ), "%f", norm[1] );
					NDebugOverlay::Text( vPoint, buf, false, recast_areaslope_debug.GetFloat() );
				}

				if( norm[1] < fSlope )
					return false;
			}
		}
	}

	return true;
}

typedef struct originalPolyFlags_t {
	dtPolyRef ref;
	unsigned short flags;
} originalPolyFlags_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void markObstaclePolygonsWalkableNavmesh(dtNavMesh* nav, NavmeshFlags* flags, dtPolyRef start, unsigned short obstacleFlag, CUtlVector< originalPolyFlags_t > &enabledPolys)
{
	dtStatus status;

	// If already visited, skip.
	if (flags->getFlags(start))
		return;
		
	PolyRefArray openList;
	openList.push(start);

	// Mark as visited, so start is not revisited again
	flags->setFlags(start, 1);

	while (openList.size())
	{
		const dtPolyRef ref = openList.pop();

		unsigned short polyFlags = 0;
		status = nav->getPolyFlags( ref, &polyFlags );
		if( !dtStatusSucceed( status ) )
		{
			continue;
		}

		// Only mark the polygons of the obstacle
		if( (polyFlags & obstacleFlag) == 0 )
		{
			continue;
		}

#if defined(_DEBUG)
		for( int k = 0; k < enabledPolys.Count(); k++ )
		{
			AssertMsg( enabledPolys[k].ref != ref, ("Already visited poly!")  );
		}
#endif // _DEBUG

		// Mark walkable and remember
		enabledPolys.AddToTail();
		enabledPolys.Tail().ref = ref;
		enabledPolys.Tail().flags = polyFlags;

		nav->setPolyFlags( ref, (polyFlags | POLYFLAGS_WALK) );

		// Get current poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtMeshTile* tile = 0;
		const dtPoly* poly = 0;
		nav->getTileAndPolyByRefUnsafe(ref, &tile, &poly);

		// Visit linked polygons.
		for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = tile->links[i].next)
		{
			const dtPolyRef neiRef = tile->links[i].ref;
			// Skip invalid and already visited.
			if (!neiRef || flags->getFlags(neiRef))
				continue;
			// Mark as visited
			flags->setFlags(neiRef, 1);
			// Visit neighbours
			openList.push(neiRef);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtStatus CRecastMesh::DoFindPath( dtNavMeshQuery *navQuery, dtPolyRef startRef, dtPolyRef endRef, 
	float spos[3], float epos[3], bool bHasTargetAndIsObstacle, pathfind_resultdata_t &findpathData )
{
	VPROF_BUDGET( "CRecastMesh::DoFindPath", "RecastNav" );

	dtStatus status;

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(spos[0], spos[2], spos[1]), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, 255, 5.0f);
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1]), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 0, 255, 255, 5.0f);
	}

	if( !CanUseCachedPath( startRef, endRef, findpathData ) )
	{
		findpathData.cacheValid = false;

		CUtlVector< originalPolyFlags_t > enabledPolys;

		if( bHasTargetAndIsObstacle )
		{
			unsigned short obstacleFlag = 0;
			status = m_navMesh->getPolyFlags( endRef, &obstacleFlag );
			if( dtStatusSucceed( status ) )
			{
				NavmeshFlags *pNavMeshFlags = new NavmeshFlags;
				pNavMeshFlags->init( m_navMesh );

				// Find the obstacle flag
				obstacleFlag &= POLYFLAGS_MASK_OBSTACLES;
				// Make this ref and all linked refs with this flag walkable
				markObstaclePolygonsWalkableNavmesh( m_navMesh, pNavMeshFlags, endRef, obstacleFlag, enabledPolys );

				delete pNavMeshFlags;
			}
		}

		status = navQuery->findPath( startRef, endRef, spos, epos, &defaultQueryFilter, findpathData.polys, &findpathData.npolys, RECASTMESH_MAX_POLYS );

		// Restore obstacle polyflags again (if any)
		for( int i = 0; i < enabledPolys.Count(); i++ )
		{
			m_navMesh->setPolyFlags( enabledPolys[i].ref, enabledPolys[i].flags );
		}

		findpathData.isPartial = (status & DT_PARTIAL_RESULT) != 0;

		if( recast_findpath_debug.GetBool() )
		{
			if( findpathData.isPartial )
				Warning( "Found a partial path to goal\n" );

			if( status & DT_OUT_OF_NODES )
				Warning( "Ran out of nodes during path find\n" );

			if( status & DT_BUFFER_TOO_SMALL )
				Warning( "Buffer is too small to hold path find result\n" );
		}

		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	// Store information for caching purposes
	findpathData.cacheValid = true;
	findpathData.startRef = startRef;
	findpathData.endRef = endRef;

	if( findpathData.npolys )
	{	
		status = navQuery->findStraightPath(spos, epos, findpathData.polys, findpathData.npolys,
										findpathData.straightPath, findpathData.straightPathFlags,
										findpathData.straightPathPolys, &findpathData.nstraightPath, 
										RECASTMESH_MAX_POLYS, findpathData.straightPathOptions);
		return status;
	}

	return DT_FAILURE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtStatus CRecastMesh::ComputeAdjustedStartAndEnd( dtNavMeshQuery *navQuery, float spos[3], float epos[3], dtPolyRef &startRef, dtPolyRef &endRef, 
	float fBeneathLimit, bool bHasTargetAndIsObstacle, bool bDisallowBigPicker, const Vector *pStartTestPos )
{
	VPROF_BUDGET( "CRecastMesh::ComputeAdjustedStartAndEnd", "RecastNav" );

	dtStatus status;

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = 32.0f;
	polyPickExt[1] = fBeneathLimit;
	polyPickExt[2] = 32.0f;

	float polyPickExtEndBig[3];
	polyPickExtEndBig[0] = 512.0f;
	polyPickExtEndBig[1] = fBeneathLimit;
	polyPickExtEndBig[2] = 512.0f;

	// Find the start area. Optional use a different test position for finding the best area.
	// Don't care for now if start ref is an obstacle area or not
	if( pStartTestPos )
	{
		float spostest[3];
		spostest[0] = (*pStartTestPos)[0];
		spostest[1] = (*pStartTestPos)[2];
		spostest[2] = (*pStartTestPos)[1];
		status = navQuery->findNearestPoly(spos, polyPickExt, &defaultQueryFilter, &startRef, 0, spostest);
	}
	else
	{
		status = navQuery->findNearestPoly(spos, polyPickExt, &defaultQueryFilter, &startRef, 0);
	}

	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	// Try any filter next time
	if( !startRef )
	{
		if( pStartTestPos )
		{
			float spostest[3];
			spostest[0] = (*pStartTestPos)[0];
			spostest[1] = (*pStartTestPos)[2];
			spostest[2] = (*pStartTestPos)[1];
			status = navQuery->findNearestPoly(spos, polyPickExt, &allQueryFilter, &startRef, 0, spostest);
		}
		else
		{
			status = navQuery->findNearestPoly(spos, polyPickExt, &allQueryFilter, &startRef, 0);
		}
	}
	
	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	// Determine end point and area
	// Note: Special case for targets which act as obstacle on the nav mesh. These have a special flag to identify their polygon. We temporary mark
	// these polygons of the obstacle as walkable. Obstacles next to this obstacle have different flags, so they all have their own polygons.

	// Find the end area
	status = navQuery->findNearestPoly(epos, polyPickExt, bHasTargetAndIsObstacle ? &obstacleQueryFilter : &defaultQueryFilter, &endRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	if( !endRef && !bDisallowBigPicker )
	{
		// Try again, bigger picker querying more tiles
		// This is mostly the case at the map borders, where we have no nav polygons. It's useful to have some tolerance, rather than just bug
		// out with a "can't move there" notification.
		status = navQuery->findNearestPoly(epos, polyPickExtEndBig, bHasTargetAndIsObstacle ? &obstacleQueryFilter : &defaultQueryFilter, &endRef, 0);
		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	if( !endRef && bHasTargetAndIsObstacle )
	{
		// Work-around: in case of some small physics obstacles may not generate as an obstacle on the mesh
		// Todo: figure out something better?
		status = navQuery->findNearestPoly(epos, polyPickExt, &defaultQueryFilter, &endRef, 0);
		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	if( endRef )
	{
		float epos2[3];
		dtVcopy(epos2, epos);
		status = navQuery->closestPointOnPoly(endRef, epos2, epos, 0);
	}
	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we can use the previous computed path.
//			This may not always give the correct result, because due the start
//			and end positions it may have computed a different path. But this is
//			good enough, because it will mostly be triggered for selection of
//			units.
//-----------------------------------------------------------------------------
bool CRecastMesh::CanUseCachedPath( dtPolyRef startRef, dtPolyRef endRef, pathfind_resultdata_t &pathResultData )
{
	if( !recast_findpath_use_caching.GetBool() || !pathResultData.cacheValid )
		return false;
	if( pathResultData.startRef != startRef || pathResultData.endRef != endRef )
		return false;
	return true;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Builds a waypoint list from the path finding results.
//-----------------------------------------------------------------------------
AI_Waypoint_t *CRecastMesh::ConstructWaypointsFromStraightPath( pathfind_resultdata_t &findpathData )
{
	AI_Waypoint_t *pResultPath = NULL;

	WaypointFlags_t fWaypointFlags = bits_WP_TO_GOAL;

	for (int i = findpathData.nstraightPath - 1; i >= 0; i--)
	{
		const dtOffMeshConnection *pOffmeshCon = m_navMesh->getOffMeshConnectionByRef( findpathData.straightPathPolys[i] );

		Vector pos( findpathData.straightPath[i*3], findpathData.straightPath[i*3+2], findpathData.straightPath[i*3+1] );

		AI_Waypoint_t *pNewPath = new AI_Waypoint_t( pos, 0.0f, NAV_GROUND, fWaypointFlags );
		fWaypointFlags = bits_WP_NO_FLAGS;
		pNewPath->SetNext( pResultPath );

		// For now, offmesh connections are always considered as edges.
		if( pOffmeshCon )
		{
		#if 0
			if( pResultPath )
				pResultPath->SpecialGoalStatus = CHS_EDGEDOWNDEST;
			pNewPath->SpecialGoalStatus = CHS_EDGEDOWN;
		#endif
		}
		pResultPath = pNewPath;
	}

	return pResultPath;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_Waypoint_t * CRecastMesh::FindPath( const Vector &vStart, const Vector &vEnd, float fBeneathLimit, CBaseEntity *pTarget, 
	bool *bIsPartial, const Vector *pStartTestPos )
{
	VPROF_BUDGET( "CRecastMesh::FindPath", "RecastNav" );

	if( !IsLoaded() )
		return NULL;

	AI_Waypoint_t *pResultPath = NULL;

	dtStatus status;

	dtPolyRef startRef = 0, endRef = 0;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];
	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	bool bHasTargetAndIsObstacle = pTarget && pTarget->GetNavObstacleRef() != NAV_OBSTACLE_INVALID_INDEX;

	status = ComputeAdjustedStartAndEnd( m_navQuery, spos, epos, startRef, endRef, fBeneathLimit, bHasTargetAndIsObstacle, pTarget != NULL, pStartTestPos );
	if( !dtStatusSucceed( status ) )
	{
		return NULL;
	}

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1] + 16.0f), 
			-Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, 255, 5.0f);
	}

	DoFindPath( m_navQuery, startRef, endRef, spos, epos, bHasTargetAndIsObstacle, m_pathfindData );

	if( m_pathfindData.cacheValid )
	{
		if( bIsPartial ) 
		{
			*bIsPartial = m_pathfindData.isPartial;
		}

		pResultPath = ConstructWaypointsFromStraightPath( m_pathfindData );
	}

	return pResultPath;
}

AI_Waypoint_t * CRecastMesh::FindPath( dtPolyRef startRef, const Vector &vStart, dtPolyRef endRef, const Vector &vEnd, CBaseEntity *pTarget, 
	bool *bIsPartial )
{
	VPROF_BUDGET( "CRecastMesh::FindPath", "RecastNav" );

	if( !IsLoaded() )
		return NULL;

	AI_Waypoint_t *pResultPath = NULL;

	dtStatus status;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];
	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	bool bHasTargetAndIsObstacle = pTarget && pTarget->GetNavObstacleRef() != NAV_OBSTACLE_INVALID_INDEX;

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1] + 16.0f), 
			-Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, 255, 5.0f);
	}

	DoFindPath( m_navQuery, startRef, endRef, spos, epos, bHasTargetAndIsObstacle, m_pathfindData );

	if( m_pathfindData.cacheValid )
	{
		if( bIsPartial ) 
		{
			*bIsPartial = m_pathfindData.isPartial;
		}

		pResultPath = ConstructWaypointsFromStraightPath( m_pathfindData );
	}

	return pResultPath;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Finds the path distance between two points
// Returns: The distance, or -1 if not found.
//-----------------------------------------------------------------------------
float CRecastMesh::FindPathDistance( const Vector &vStart, const Vector &vEnd, CSharedBaseEntity *pTarget, float fBeneathLimit, bool bLimitedSearch )
{
	VPROF_BUDGET( "CRecastMesh::FindPathDistance", "RecastNav" );

	if( !IsLoaded() )
		return -1;

	dtStatus status;

	dtPolyRef startRef = 0, endRef = 0;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];

	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	// Faster search for nearby units. Mainly intended for unit sensing code, so it quickly filters out "unreachable" enemies
	dtNavMeshQuery *navQuery = bLimitedSearch ? m_navQueryLimitedNodes : m_navQuery;

	bool bHasTargetAndIsObstacle = pTarget && pTarget->GetNavObstacleRef() != NAV_OBSTACLE_INVALID_INDEX;

	status = ComputeAdjustedStartAndEnd( navQuery, spos, epos, startRef, endRef, fBeneathLimit, bHasTargetAndIsObstacle, pTarget != NULL, NULL );
	if( !dtStatusSucceed( status ) )
	{
		return 0.0f;
	}

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1] + 16.0f), 
			-Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, 255, 5.0f);
	}

	status = DoFindPath( navQuery, startRef, endRef, spos, epos, bHasTargetAndIsObstacle, m_pathfindData );
	if( dtStatusSucceed( status ) && !m_pathfindData.isPartial )
	{
		float fPathDistance = 0;
		Vector vPrev = vEnd;
		for (int i = m_pathfindData.nstraightPath - 1; i >= 0; i--)
		{
			Vector pos( m_pathfindData.straightPath[i*3], m_pathfindData.straightPath[i*3+2], m_pathfindData.straightPath[i*3+1] );
			fPathDistance += pos.DistTo(vPrev);
			vPrev = pos;
		}
		return fPathDistance;
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Tests if route between start and end is covered by the navigation
// mesh.
//-----------------------------------------------------------------------------
bool CRecastMesh::TestRoute( const Vector &vStart, const Vector &vEnd )
{
	if( !IsLoaded() )
		return false;

	dtStatus status;

	dtPolyRef startRef;
	dtPolyRef endRef;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];

	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	float polyPickExt[3];
	polyPickExt[0] = 128.0f;
	polyPickExt[1] = 600.0f;
	polyPickExt[2] = 128.0f;

	status = m_navQuery->findNearestPoly(spos, polyPickExt, &defaultQueryFilter, &startRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}
	status = m_navQuery->findNearestPoly(epos, polyPickExt, &defaultQueryFilter, &endRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	dtRaycastHit rayHit;
	rayHit.maxPath = 0;
	rayHit.pathCost = rayHit.t = 0;
	status = m_navQuery->raycast( startRef, spos, epos, &defaultQueryFilter, 0, &rayHit );
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	return rayHit.t == FLT_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a temporary obstacle to the navigation mesh (radius version)
//-----------------------------------------------------------------------------
dtObstacleRef CRecastMesh::AddTempObstacle( const Vector &vPos, float radius, float height, unsigned char areaId )
{
	if( !IsLoaded() )
		return 0;
	float pos[3] = {vPos.x, vPos.z, vPos.y};

	dtObstacleRef result;
	dtStatus status = m_tileCache->addObstacle( pos, radius, height, areaId, &result );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::AddTempObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::AddTempObstacle: out of memory\n");
		}
		return 0;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a temporary obstacle to the navigation mesh (convex hull version)
//-----------------------------------------------------------------------------
dtObstacleRef CRecastMesh::AddTempObstacle( const Vector &vPos, const Vector *convexHull, const int numConvexHull, float height, unsigned char areaId )
{
	if( !IsLoaded() )
		return 0;

	dtStatus status;
	float pos[3] = {vPos.x, vPos.z, vPos.y};

	float *verts = (float *)stackalloc( numConvexHull * 3 * sizeof( float ) );
	for( int i = 0; i < numConvexHull; i++ )
	{
		verts[(i*3)+0] = convexHull[i].x;
		verts[(i*3)+1] = convexHull[i].z;
		verts[(i*3)+2] = convexHull[i].y;
	}

	dtObstacleRef result;
	status = m_tileCache->addPolygonObstacle( pos, verts, numConvexHull, height, areaId, &result );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::AddTempObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::AddTempObstacle: out of memory\n");
		}
		return 0;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Removes a temporary obstacle from the navigation mesh
//-----------------------------------------------------------------------------
bool CRecastMesh::RemoveObstacle( const dtObstacleRef ref )
{
	if( !IsLoaded() )
		return false;

	dtStatus status = m_tileCache->removeObstacle( ref );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::RemoveObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::RemoveObstacle: out of memory\n");
		}
		return false;
	}
	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return Z of area at (x,y) of 'pos'
 * Trilinear interpolation of Z values at quad edges.
 * NOTE: pos->z is not used.
 */

float CRecastMesh::GetPolyZ( dtPolyRef polyRef, float x, float y ) const RESTRICT
{
	if( !IsValidPolyRef( polyRef ) )
		return MAX_COORD_FLOAT;

	float pos[3];
	pos[0] = x;
	pos[1] = 0.0f;
	pos[2] = y;

	dtStatus status = m_navQuery->getPolyHeight( polyRef, pos, &pos[1] );
	if( !dtStatusSucceed( status ) )
	{
		return MAX_COORD_FLOAT;
	}

	return pos[1];
}

PolyFlags CRecastMesh::GetPolyFlags( dtPolyRef polyRef ) const
{
	if( !IsValidPolyRef( polyRef ) )
		return (PolyFlags)0;

	unsigned short flags;
	dtStatus status = m_navMesh->getPolyFlags( polyRef, &flags );
	if( !dtStatusSucceed( status ) )
	{
		return (PolyFlags)0;
	}

	return (PolyFlags)flags;
}

bool CRecastMesh::GetPolyExtent( dtPolyRef polyRef, Extent *extent ) const
{
	if( !IsValidPolyRef( polyRef ) )
		return false;

	const dtMeshTile* tile = NULL;
	const dtPoly* poly = NULL;
	m_navMesh->getTileAndPolyByRefUnsafe( polyRef, &tile, &poly );

	Vector lo = Vector( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
	Vector hi = Vector( 0.0, 0.0, 0.0 );

	for( int i =0; i < DT_VERTS_PER_POLYGON; ++i ) {
		const float *vec = &tile->verts[poly->verts[i]];

		if(vec[0] < lo.x)
			lo.x = vec[0];
		if(vec[1] < lo.y)
			lo.y = vec[0];
		if(vec[2] < lo.z)
			lo.z = vec[0];

		if(vec[0] > hi.x)
			hi.x = vec[0];
		if(vec[1] > hi.y)
			hi.y = vec[0];
		if(vec[2] > hi.z)
			hi.z = vec[0];
	}

	extent->Init( lo, hi );

	return true;
}

HidingSpotVector TheHidingSpots[RECAST_NAVMESH_NUM];
unsigned int HidingSpot::m_nextID[RECAST_NAVMESH_NUM] = {1};
unsigned int HidingSpot::m_masterMarker[RECAST_NAVMESH_NUM] = {0};

//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot factory
 */
HidingSpot *CRecastMesh::CreateHidingSpot( void ) const
{
	return new HidingSpot( m_Type );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Construct a Hiding Spot.  Assign a unique ID which may be overwritten if loaded.
 */
HidingSpot::HidingSpot( NavMeshType_t type )
{
	m_navMeshType = type;

	m_pos = Vector( 0, 0, 0 );
	m_id = m_nextID[type]++;
	m_flags = 0;
	m_poly = 0;

	TheHidingSpots[type].AddToTail( this );
}

//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	fileBuffer.PutUnsignedInt( m_id );
	fileBuffer.PutFloat( m_pos.x );
	fileBuffer.PutFloat( m_pos.y );
	fileBuffer.PutFloat( m_pos.z );
	fileBuffer.PutUnsignedChar( m_flags );
}

//--------------------------------------------------------------------------------------------------------------
void HidingSpot::Load( CUtlBuffer &fileBuffer, unsigned int version )
{
	m_id = fileBuffer.GetUnsignedInt();
	m_pos.x = fileBuffer.GetFloat();
	m_pos.y = fileBuffer.GetFloat();
	m_pos.z = fileBuffer.GetFloat();
	m_flags = fileBuffer.GetUnsignedChar();

	// update next ID to avoid ID collisions by later spots
	if (m_id >= m_nextID[m_navMeshType])
		m_nextID[m_navMeshType] = m_id+1;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Hiding Spot post-load processing
 */
bool HidingSpot::PostLoad( void )
{
	// set our area
	CRecastMesh *pMesh = RecastMgr().GetMesh( m_navMeshType );
	if( !pMesh )
	{
		DevWarning( "A Hiding Spot has invalid Nav Mesh at setpos %.0f %.0f %.0f\n", m_pos.x, m_pos.y, m_pos.z );
		return false;
	}

	float halfHeight = NAI_Hull::Height( m_navMeshType ) * 0.5f;

	m_poly = pMesh->GetPolyRef( m_pos + Vector( 0, 0, halfHeight ) );
	if ( m_poly == 0 )
	{
		DevWarning( "A Hiding Spot is off of the Nav Mesh at setpos %.0f %.0f %.0f\n", m_pos.x, m_pos.y, m_pos.z );
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given a HidingSpot ID, return the associated HidingSpot
 */
HidingSpot *GetHidingSpotByID( NavMeshType_t type, unsigned int id )
{
	FOR_EACH_VEC( TheHidingSpots[type], it )
	{
		HidingSpot *spot = TheHidingSpots[type][ it ];

		if (spot->GetID() == id)
			return spot;
	}

	return NULL;
}

extern ConVar sv_stepsize;

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine how much walkable area we can see from the spot, and how far away we can see.
 */
void ClassifySniperSpot( HidingSpot *spot )
{
	Vector eye = spot->GetPosition();

	CRecastMesh *pMesh = RecastMgr().GetMesh( spot->m_navMeshType );
	if( !pMesh )
	{
		return;
	}

	dtPolyRef hidingArea = pMesh->GetPolyRef( spot->GetPosition() );

	const float halfHeight = NAI_Hull::Height( spot->m_navMeshType ) * 0.5f;

#if 0
	if (hidingArea != 0 && (pMesh->GetPolyFlags( hidingArea ) & NAV_MESH_STAND))
	{
		// we will be standing at this hiding spot
		eye.z += NAI_Hull::Height( spot->m_navMeshType );
	}
	else
#endif
	{
		// we are crouching when at this hiding spot
		eye.z += halfHeight;
	}

	Extent sniperExtent;
	float farthestRangeSq = 0.0f;
	const float minSniperRangeSq = 1000.0f * 1000.0f;
	bool found = false;

	// to make compiler stop warning me
	sniperExtent.lo = Vector( 0.0f, 0.0f, 0.0f );
	sniperExtent.hi = Vector( 0.0f, 0.0f, 0.0f );

	const float stepSize = sv_stepsize.GetFloat();

	auto func = 
	[&found,&sniperExtent,minSniperRangeSq,&farthestRangeSq,eye,stepSize,halfHeight]
	( CRecastMesh *pMesh, dtPolyRef polyref ) -> bool {
		const dtMeshTile* tile = NULL;
		const dtPoly* poly = NULL;

		pMesh->GetNavMesh()->getTileAndPolyByRefUnsafe(polyref, &tile, &poly);

		Extent areaExtent;
		if(!pMesh->GetPolyExtent( polyref, &areaExtent ))
			return true;

		// scan this area
		Vector walkable;
		for( walkable.y = areaExtent.lo.y + stepSize/2.0f; walkable.y < areaExtent.hi.y; walkable.y += stepSize )
		{
			for( walkable.x = areaExtent.lo.x + stepSize/2.0f; walkable.x < areaExtent.hi.x; walkable.x += stepSize )
			{
				walkable.z = pMesh->GetPolyZ( polyref, walkable ) + halfHeight;

				// check line of sight
				trace_t result;
				UTIL_TraceLine( eye, walkable, CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP, NULL, COLLISION_GROUP_NONE, &result );

				if (result.fraction == 1.0f && !result.startsolid)
				{
					// can see this spot

					// keep track of how far we can see
					float rangeSq = (eye - walkable).LengthSqr();
					if (rangeSq > farthestRangeSq)
					{
						farthestRangeSq = rangeSq;

						if (rangeSq >= minSniperRangeSq)
						{
							// this is a sniper spot
							// determine how good of a sniper spot it is by keeping track of the snipable area
							if (found)
							{
								if (walkable.x < sniperExtent.lo.x)
									sniperExtent.lo.x = walkable.x;
								if (walkable.x > sniperExtent.hi.x)
									sniperExtent.hi.x = walkable.x;

								if (walkable.y < sniperExtent.lo.y)
									sniperExtent.lo.y = walkable.y;
								if (walkable.y > sniperExtent.hi.y)
									sniperExtent.hi.y = walkable.y;
							}
							else
							{
								sniperExtent.lo = walkable;
								sniperExtent.hi = walkable;
								found = true;
							}
						}
					}
				}
			}
		}

		return true;
	};

	pMesh->ForAllPolys( func );

	if (found)
	{
		// if we can see a large snipable area, it is an "ideal" spot
		float snipableArea = sniperExtent.Area();

		const float minIdealSniperArea = 200.0f * 200.0f;
		const float longSniperRangeSq = 1500.0f * 1500.0f;

		if (snipableArea >= minIdealSniperArea || farthestRangeSq >= longSniperRangeSq)
			spot->m_flags |= HidingSpot::IDEAL_SNIPER_SPOT;
		else
			spot->m_flags |= HidingSpot::GOOD_SNIPER_SPOT;
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return radio chatter place for given coordinate
 */
unsigned int CRecastMesh::GetPlace( const Vector &pos ) const
{
	return UNDEFINED_PLACE;
}
