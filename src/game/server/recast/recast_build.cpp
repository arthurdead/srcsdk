//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#include "recast/recast_mapmesh.h"
#include "recast/recast_tilecache_helpers.h"
#include "recast/recast_mgr_ent.h"
#include "vstdlib/jobthread.h"
#include "collisionutils.h"

#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"

#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

#include "ChunkyTriMesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar recast_build_single( "recast_build_single", "", 0, "Limit building to a single mesh named in this convar" );
static ConVar recast_build_threaded( "recast_build_threaded", "1", FCVAR_ARCHIVE );
static ConVar recast_build_numthreads( "recast_build_numthreads", "8", FCVAR_ARCHIVE );
static ConVar recast_build_remove_unreachable_polys( "recast_build_remove_unreachable_polys", "1", 0 );

static ConVar recast_build_partial_debug( "recast_build_partial_debug", "0", 0, "debug partial mesh rebuilding" );

// This value specifies how many layers (or "floors") each navmesh tile is expected to have.
static const int EXPECTED_LAYERS_PER_TILE = 4;

static const int MAX_LAYERS = 32;

class BuildContext : public rcContext
{
public:
	virtual void doLog(const rcLogCategory category, const char* msg, const int len) 
	{
		switch( category )
		{
		case RC_LOG_PROGRESS:
			Msg( "%s\n", msg );
			break;
		case RC_LOG_WARNING:
			Warning( "%s\n", msg );
			break;
		case RC_LOG_ERROR:
			Warning( "%s\n", msg );
			break;
		}
	}
};

static int calcLayerBufferSize(const int gridWidth, const int gridHeight)
{
	const int headerSize = dtAlign4(sizeof(dtTileCacheLayerHeader));
	const int gridSize = gridWidth * gridHeight;
	return headerSize + gridSize*4;
}

struct TileCacheData
{
	unsigned char* data;
	int dataSize;
};

struct RasterizationContext
{
	RasterizationContext() :
		solid(0),
		triareas(0),
		lset(0),
		chf(0),
		ntiles(0)
	{
		memset(tiles, 0, sizeof(TileCacheData)*MAX_LAYERS);
	}
	
	~RasterizationContext()
	{
		rcFreeHeightField(solid);
		delete [] triareas;
		rcFreeHeightfieldLayerSet(lset);
		rcFreeCompactHeightfield(chf);
		for (int i = 0; i < MAX_LAYERS; ++i)
		{
			dtFree(tiles[i].data);
			tiles[i].data = 0;
		}
	}
	
	rcHeightfield* solid;
	unsigned char* triareas;
	rcHeightfieldLayerSet* lset;
	rcCompactHeightfield* chf;
	TileCacheData tiles[MAX_LAYERS];
	int ntiles;
};

static int rasterizeTileLayers(BuildContext* ctx, CMapMesh* geom,
							   const int tx, const int ty,
							   const rcConfig& cfg,
							   TileCacheData* tiles,
							   const int maxTiles)
{
	if (!geom || !geom->GetChunkyMesh())
	{
		ctx->log(RC_LOG_ERROR, "buildTile: Input mesh is not specified.");
		return 0;
	}
	
	FastLZCompressor comp;
	RasterizationContext rc;
	
	const float* verts = geom->GetVerts();
	const int nverts = geom->GetNumVerts();
	const rcChunkyTriMesh* chunkyMesh = geom->GetChunkyMesh();
	
	// Tile bounds.
	const float tcs = cfg.tileSize * cfg.cs;
	
	rcConfig tcfg;
	memcpy(&tcfg, &cfg, sizeof(tcfg));

	tcfg.bmin[0] = cfg.bmin[0] + tx*tcs;
	tcfg.bmin[1] = cfg.bmin[1];
	tcfg.bmin[2] = cfg.bmin[2] + ty*tcs;
	tcfg.bmax[0] = cfg.bmin[0] + (tx+1)*tcs;
	tcfg.bmax[1] = cfg.bmax[1];
	tcfg.bmax[2] = cfg.bmin[2] + (ty+1)*tcs;
	tcfg.bmin[0] -= tcfg.borderSize*tcfg.cs;
	tcfg.bmin[2] -= tcfg.borderSize*tcfg.cs;
	tcfg.bmax[0] += tcfg.borderSize*tcfg.cs;
	tcfg.bmax[2] += tcfg.borderSize*tcfg.cs;
	
	// Allocate voxel heightfield where we rasterize our input data to.
	rc.solid = rcAllocHeightfield();
	if (!rc.solid)
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return 0;
	}
	if (!rcCreateHeightfield(ctx, *rc.solid, tcfg.width, tcfg.height, tcfg.bmin, tcfg.bmax, tcfg.cs, tcfg.ch))
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return 0;
	}
	
	// Allocate array that can hold triangle flags.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	rc.triareas = new unsigned char[chunkyMesh->maxTrisPerChunk];
	if (!rc.triareas)
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
		return 0;
	}
	
	float tbmin[2], tbmax[2];
	tbmin[0] = tcfg.bmin[0];
	tbmin[1] = tcfg.bmin[2];
	tbmax[0] = tcfg.bmax[0];
	tbmax[1] = tcfg.bmax[2];
	int cid[512];// TODO: Make grow when returning too many items.
	const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, 512);
	if (!ncid)
	{
		return 0; // empty
	}
	
	for (int i = 0; i < ncid; ++i)
	{
		const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
		const int* tris = &chunkyMesh->tris[node.i*3];
		const int ntris = node.n;
		
		memset(rc.triareas, 0, ntris*sizeof(unsigned char));
		rcMarkWalkableTriangles(ctx, tcfg.walkableSlopeAngle,
								verts, nverts, tris, ntris, rc.triareas);
		
		rcRasterizeTriangles(ctx, verts, nverts, tris, rc.triareas, ntris, *rc.solid, tcfg.walkableClimb);
	}
	
	// Once all geometry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(ctx, tcfg.walkableClimb, *rc.solid);
	rcFilterLedgeSpans(ctx, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid);
	rcFilterWalkableLowHeightSpans(ctx, tcfg.walkableHeight, *rc.solid);
	
	
	rc.chf = rcAllocCompactHeightfield();
	if (!rc.chf)
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return 0;
	}
	if (!rcBuildCompactHeightfield(ctx, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid, *rc.chf))
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return 0;
	}
	
	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(ctx, tcfg.walkableRadius, *rc.chf))
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return 0;
	}
	
#if 0
	// (Optional) Mark areas.
	const ConvexVolume* vols = geom->getConvexVolumes();
	for (int i  = 0; i < geom->getConvexVolumeCount(); ++i)
	{
		rcMarkConvexPolyArea(ctx, vols[i].verts, vols[i].nverts,
							 vols[i].hmin, vols[i].hmax,
							 (unsigned char)vols[i].area, *rc.chf);
	}
#endif // 0

	rc.lset = rcAllocHeightfieldLayerSet();
	if (!rc.lset)
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'lset'.");
		return 0;
	}
	if (!rcBuildHeightfieldLayers(ctx, *rc.chf, tcfg.borderSize, tcfg.walkableHeight, *rc.lset))
	{
		ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build heighfield layers.");
		return 0;
	}
	
	rc.ntiles = 0;
	for (int i = 0; i < rcMin(rc.lset->nlayers, MAX_LAYERS); ++i)
	{
		TileCacheData* tile = &rc.tiles[rc.ntiles++];
		const rcHeightfieldLayer* layer = &rc.lset->layers[i];
		
		// Store header
		dtTileCacheLayerHeader header;
		header.magic = DT_TILECACHE_MAGIC;
		header.version = DT_TILECACHE_VERSION;
		
		// Tile layer location in the navmesh.
		header.tx = tx;
		header.ty = ty;
		header.tlayer = i;
		dtVcopy(header.bmin, layer->bmin);
		dtVcopy(header.bmax, layer->bmax);
		
		// Tile info.
		header.width = (unsigned char)layer->width;
		header.height = (unsigned char)layer->height;
		header.minx = (unsigned char)layer->minx;
		header.maxx = (unsigned char)layer->maxx;
		header.miny = (unsigned char)layer->miny;
		header.maxy = (unsigned char)layer->maxy;
		header.hmin = (unsigned short)layer->hmin;
		header.hmax = (unsigned short)layer->hmax;

		dtStatus status = dtBuildTileCacheLayer(&comp, &header, layer->heights, layer->areas, layer->cons,
												&tile->data, &tile->dataSize);
		if (dtStatusFailed(status))
		{
			return 0;
		}
	}

	// Transfer ownsership of tile data from build context to the caller.
	int n = 0;
	for (int i = 0; i < rcMin(rc.ntiles, maxTiles); ++i)
	{
		tiles[n++] = rc.tiles[i];
		rc.tiles[i].data = 0;
		rc.tiles[i].dataSize = 0;
	}
	
	return n;
}

//-----------------------------------------------------------------------------
// Purpose: Marks polygons as disabled based on a number of sample points and
//			writes back the data, removing any empty tiles.
//-----------------------------------------------------------------------------
bool CRecastMesh::RemoveUnreachablePoly( CMapMesh *pMapMesh )
{
	if( !pMapMesh->GetSampleOrigins().Count() )
	{
		Warning( "CRecastMesh: No sample points on the map, skipping removable of unreachable polygons\n" );
		return true;
	}

	dtStatus status;

	DisableUnreachablePolygons( pMapMesh->GetSampleOrigins() );

	int removedTiles = 0, updatedTiles = 0;

	// Go through all tiles
	const dtNavMesh * navMesh = m_navMesh;

	for( int i = 0; i < m_navMesh->getMaxTiles(); ++i )
	{
		m_talloc->reset();

		const dtMeshTile* tile = navMesh->getTile(i);
		if( !tile->header ) 
			continue;

		// Find compressed tile
		dtCompressedTile *compTile = NULL;
		for (int j = 0; j < m_tileCache->getTileCount(); ++j)
		{
			const dtCompressedTile* compressedTile = m_tileCache->getTile(j);
			if (!compressedTile || !compressedTile->header || !compressedTile->dataSize) continue;

			if( compressedTile->header->tx == tile->header->x && 
				compressedTile->header->ty == tile->header->y &&
				compressedTile->header->tlayer == tile->header->layer ) {
				compTile = (dtCompressedTile *)compressedTile;
				break;
			}
		}

		if( !compTile ) {
			Warning("Did not find matching compressed tile!\n");
			continue;
		}

		// Build the layer data, so we can access the areas of the tile
		dtTileCacheLayer *pLayer = NULL;
		status = dtDecompressTileCacheLayer( m_talloc, m_tcomp, compTile->data, compTile->dataSize, &pLayer );
		if (dtStatusFailed(status)) {
			Warning("Could not decompress tile! =>\n" );
			if( status & DT_FAILURE )
				Warning("\tFailure status\n");
			if( status & DT_OUT_OF_MEMORY )
				Warning("\tOut of memory status\n");
			if( status & DT_WRONG_MAGIC )
				Warning("\tWrong magic status\n");
			if( status & DT_WRONG_VERSION )
				Warning("\tWrong version status\n");
			if( status & DT_INVALID_PARAM )
				Warning("\tInvalid param status\n");
			continue;
		}
	
		// Go through each poly in the tile
		bool bAllPolygonsDisabled = true, bHasPolygonsDisabled = false;
		for( int i = 0; i < tile->header->polyCount; ++i )
		{
			const dtPoly* p = &tile->polys[i];
			if( p->flags & SAMPLE_POLYFLAGS_DISABLED ) {

				// TODO: Maybe optimize by creating a custom version of dtMarkPolyArea, but probably fast enough
				CUtlVector< float > verts;
				for( int k = 0; k < p->vertCount; k++ ) 
				{
					verts.AddToTail( tile->verts[ p->verts[k] * 3 ] );
					verts.AddToTail( tile->verts[ p->verts[k]*3+1 ] );
					verts.AddToTail( tile->verts[ p->verts[k]*3+2 ] );
				}

				// Mark areas for this polygon as null
				dtMarkPolyArea( *pLayer, tile->header->bmin, m_tileCache->getParams()->cs, m_tileCache->getParams()->ch, 
					verts.Base(), p->vertCount, DT_TILECACHE_NULL_AREA );

				bHasPolygonsDisabled = true;
			} else {
				bAllPolygonsDisabled = false;
			}
		}

		if( bAllPolygonsDisabled ) {
			// If all polygons are disabled, simply remove the tile. This can greatly reduce the file size.
			m_tileCache->removeTile( m_tileCache->getTileRef( compTile ), &compTile->data, &compTile->dataSize );
			removedTiles++;
		} else if( bHasPolygonsDisabled ) {
			// Write back the updated tile with disabled areas
			status = dtBuildTileCacheLayer( m_tcomp, pLayer->header, pLayer->heights, pLayer->areas, pLayer->cons, &compTile->data, &compTile->dataSize );
			if (dtStatusFailed(status)) {
				Warning("Could not rebuild compressed tile cache layer! =>\n" );
				if( status & DT_FAILURE )
					Warning("\tFailure status\n");
				if( status & DT_OUT_OF_MEMORY )
					Warning("\tOut of memory status\n");
			}

			updatedTiles++;
		}

		dtFreeTileCacheLayer( m_talloc, pLayer );
	}

	Msg( "Removed %d tiles, Updated tiles: %d, total tiles: %d\n", removedTiles, updatedTiles, m_navMesh->getMaxTiles() );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Build( CMapMesh *pMapMesh )
{
	double fStartTime = Plat_FloatTime();

	Reset(); // Clean any existing data

	BuildContext ctx;

	ctx.enableLog( true );

	dtStatus status;
	
	V_memset(&m_cfg, 0, sizeof(m_cfg));

	// Init cache
	rcCalcBounds( pMapMesh->GetVerts(), pMapMesh->GetNumVerts(), m_cfg.bmin, m_cfg.bmax );
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);
	int gw = 0, gh = 0;
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cellSize, &gw, &gh);
	const int ts = (int)m_tileSize;
	const int tw = (gw + ts-1) / ts;
	const int th = (gh + ts-1) / ts;

	// Max tiles and max polys affect how the tile IDs are caculated.
	// There are 22 bits available for identifying a tile and a polygon.
	int tileBits = rcMin((int)dtIlog2(dtNextPow2(tw*th*EXPECTED_LAYERS_PER_TILE)), 14);
	if (tileBits > 14) tileBits = 14;
	int polyBits = 22 - tileBits;
	m_maxTiles = 1 << tileBits;
	m_maxPolysPerTile = 1 << polyBits;

	// Generation params.
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.tileSize = (int)m_tileSize;
	m_cfg.borderSize = m_cfg.walkableRadius + 3; // Reserve enough padding.
	m_cfg.width = m_cfg.tileSize + m_cfg.borderSize*2;
	m_cfg.height = m_cfg.tileSize + m_cfg.borderSize*2;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;
	
	// Tile cache params.
	dtTileCacheParams tcparams;
	memset(&tcparams, 0, sizeof(tcparams));
	rcVcopy(tcparams.orig, m_cfg.bmin);
	tcparams.cs = m_cellSize;
	tcparams.ch = m_cellHeight;
	tcparams.width = (int)m_tileSize;
	tcparams.height = (int)m_tileSize;
	tcparams.walkableHeight = m_agentHeight;
	tcparams.walkableRadius = m_agentRadius;
	tcparams.walkableClimb = m_agentMaxClimb;
	tcparams.maxSimplificationError = m_edgeMaxError;
	tcparams.maxTiles = tw*th*EXPECTED_LAYERS_PER_TILE;
	tcparams.maxObstacles = 2048;

	dtFreeTileCache(m_tileCache);

	m_tileCache = dtAllocTileCache();
	if (!m_tileCache)
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not allocate tile cache.");
		return false;
	}
	status = m_tileCache->init(&tcparams, m_talloc, m_tcomp, m_tmproc);
	if (dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not init tile cache.");
		return false;
	}
	
	dtFreeNavMesh(m_navMesh);
	
	m_navMesh = dtAllocNavMesh();
	if (!m_navMesh)
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not allocate navmesh.");
		return false;
	}

	dtNavMeshParams params;
	memset(&params, 0, sizeof(params));
	rcVcopy(params.orig, m_cfg.bmin);
	params.tileWidth = m_tileSize*m_cellSize;
	params.tileHeight = m_tileSize*m_cellSize;
	params.maxTiles = m_maxTiles;
	params.maxPolys = m_maxPolysPerTile;
	
	status = m_navMesh->init(&params);
	if (dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not init navmesh.");
		return false;
	}
	
	status = m_navQuery->init( m_navMesh, RECAST_NAVQUERY_MAX_NODES );
	if (dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not init Detour navmesh query");
		return false;
	}
	
	status = m_navQueryLimitedNodes->init( m_navMesh, RECAST_NAVQUERY_LIMITED_NODES );
	if (dtStatusFailed(status))
	{
		ctx.log(RC_LOG_ERROR, "buildTiledNavigation: Could not init Detour navmesh query");
		return false;
	}

	// Preprocess tiles.
	
	ctx.resetTimers();
	
	m_cacheLayerCount = 0;
	m_cacheCompressedSize = 0;
	m_cacheRawSize = 0;
	
	for (int y = 0; y < th; ++y)
	{
		for (int x = 0; x < tw; ++x)
		{
			TileCacheData tiles[MAX_LAYERS];
			memset(tiles, 0, sizeof(tiles));
			int ntiles = rasterizeTileLayers(&ctx, pMapMesh, x, y, m_cfg, tiles, MAX_LAYERS);

			for (int i = 0; i < ntiles; ++i)
			{
				TileCacheData* tile = &tiles[i];
				status = m_tileCache->addTile(tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0);
				if (dtStatusFailed(status))
				{
					dtFree(tile->data);
					tile->data = 0;
					continue;
				}
				
				m_cacheLayerCount++;
				m_cacheCompressedSize += tile->dataSize;
				m_cacheRawSize += calcLayerBufferSize(tcparams.width, tcparams.height);
			}
		}
	}

	// Build initial meshes
	ctx.startTimer(RC_TIMER_TOTAL);
	for (int y = 0; y < th; ++y)
		for (int x = 0; x < tw; ++x)
			m_tileCache->buildNavMeshTilesAt(x,y, m_navMesh);
	ctx.stopTimer(RC_TIMER_TOTAL);

	// Disable unreachable polys
	if( recast_build_remove_unreachable_polys.GetBool() )
	{
		RemoveUnreachablePoly( pMapMesh );
	}
	
	m_cacheBuildTimeMs = ctx.getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;
	m_cacheBuildMemUsage = ((LinearAllocator *)m_talloc)->high;

	const dtNavMesh* nav = m_navMesh;
	int navmeshMemUsage = 0;
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = nav->getTile(i);
		if (tile->header)
			navmeshMemUsage += tile->dataSize;
	}

	PostLoad();

	DevMsg( "CRecastMesh: Generated navigation mesh %s in %f seconds\n", m_Name.Get(), Plat_FloatTime() - fStartTime );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Partial rebuild. Destroys tiles at bounds touching the mesh and 
//			rebuilds those tiles.
//-----------------------------------------------------------------------------
bool CRecastMesh::RebuildPartial( CMapMesh *pMapMesh, const Vector &vMins, const Vector &vMaxs )
{
	double fStartTime = Plat_FloatTime();

	BuildContext ctx;
	dtStatus status;

	const dtTileCacheParams &tcparams = *m_tileCache->getParams();

	float bmin[3];
	float bmax[3];
	
	bmin[0] = vMins.x;
	bmin[1] = vMins.z;
	bmin[2] = vMins.y;
	bmax[0] = vMaxs.x;
	bmax[1] = vMaxs.z;
	bmax[2] = vMaxs.y;

	const float tw = tcparams.width * tcparams.cs;
	const float th = tcparams.height * tcparams.cs;
	const int tx0 = (int)dtMathFloorf((bmin[0]-tcparams.orig[0]) / tw);
	const int tx1 = (int)dtMathFloorf((bmax[0]-tcparams.orig[0]) / tw);
	const int ty0 = (int)dtMathFloorf((bmin[2]-tcparams.orig[2]) / th);
	const int ty1 = (int)dtMathFloorf((bmax[2]-tcparams.orig[2]) / th);
	
	TileCacheData tiles[MAX_LAYERS];
	int ntiles;
	dtCompressedTileRef results[128];

	for (int ty = ty0; ty <= ty1; ++ty)
	{
		for (int tx = tx0; tx <= tx1; ++tx)
		{
			int nCount = m_tileCache->getTilesAt( tx, ty, results, 128 );
			for( int i = 0; i < nCount; i++ )
			{
				unsigned char* data; 
				int dataSize;
				m_tileCache->removeTile( results[i], &data, &dataSize );
			}

			memset(tiles, 0, sizeof(tiles));
			ntiles = rasterizeTileLayers(&ctx, pMapMesh, tx, ty, m_cfg, tiles, MAX_LAYERS);

			for (int i = 0; i < ntiles; ++i)
			{
				TileCacheData* tile = &tiles[i];
				status = m_tileCache->addTile(tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0);
				if (dtStatusFailed(status))
				{
					dtFree(tile->data);
					tile->data = 0;
					continue;
				}
			}
		}
	}

	// Build initial meshes
	for (int ty = ty0; ty <= ty1; ++ty)
	{
		for (int tx = tx0; tx <= tx1; ++tx)
		{
			m_tileCache->buildNavMeshTilesAt(tx,ty, m_navMesh);
		}
	}

	if( recast_build_partial_debug.GetBool() )
		DevMsg( "CRecastMesh: Generated partial mesh update %s in %f seconds\n", m_Name.Get(), Plat_FloatTime() - fStartTime );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Loads the map mesh (geometry)
//-----------------------------------------------------------------------------
bool CRecastMgr::LoadMapMesh( bool bLog, bool bDynamicOnly, const Vector &vMinBounds, const Vector &vMaxBounds )
{
	if( bDynamicOnly && !m_pMapMesh )
	{
		Warning("CRecastMesh::LoadMapMesh: load dynamic specified, but no existing static map mesh data!\n");
		return false;
	}

	double fStartTime = Plat_FloatTime();

	// Create a new map mesh if we are loading everything
	if( !bDynamicOnly )
	{
		if( m_pMapMesh )
		{
			delete m_pMapMesh;
			m_pMapMesh = NULL;
		}

		m_pMapMesh = new CMapMesh( bLog );
	}
	else
	{
		m_pMapMesh->SetLog( bLog );
	}

	m_pMapMesh->SetBounds( vMinBounds, vMaxBounds );
	if( !m_pMapMesh->Load( bDynamicOnly ) )
	{
		Warning("CRecastMesh::LoadMapMesh: failed to load map data!\n");
		return false;
	}

	if( bLog )
		DevMsg( "CRecastMgr: Loaded map mesh in %f seconds\n", Plat_FloatTime() - fStartTime );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::BuildMesh( CMapMesh *pMapMesh, const char *name )
{
	CRecastMesh *pMesh = GetMesh( name );
	if( !pMesh )
	{
		pMesh = new CRecastMesh();
		pMesh->Init( name );
		m_Meshes.Insert( pMesh->GetName(), pMesh );
	}
	if( pMesh->Build( pMapMesh ) )
	{
		return true;
	}
	return false;
}

static void PreThreadedBuildMesh()
{
}

static void PostThreadedBuildMesh()
{
}

void CRecastMgr::ThreadedBuildMesh( CRecastMesh *&pMesh )
{
	pMesh->Build( (CMapMesh *)RecastMgr().GetMapMesh() );
}

void CRecastMgr::ThreadedRebuildPartialMesh( CRecastMesh *&pMesh )
{
	CMapMesh *pMapMesh = (CMapMesh *)RecastMgr().GetMapMesh();
	pMesh->RebuildPartial( pMapMesh, pMapMesh->GetMinBounds(), pMapMesh->GetMaxBounds() );
}

static char* s_MeshNames[] = 
{
	"human",
	"medium",
	"large",
	"verylarge",
	"air"
};

//-----------------------------------------------------------------------------
// Purpose: Checks if building this mesh is disabled
//-----------------------------------------------------------------------------
bool CRecastMgr::IsMeshBuildDisabled( const char *meshName )
{
	CRecastMgrEnt *pMgrEnt = GetRecastMgrEnt();
	if( pMgrEnt )
	{
		if( pMgrEnt->HasSpawnFlags( SF_DISABLE_MESH_HUMAN ) && V_strcmp( "human", meshName ) == 0 )
			return true;
		else if( pMgrEnt->HasSpawnFlags( SF_DISABLE_MESH_MEDIUM ) && V_strcmp( "medium", meshName ) == 0 )
			return true;
		else if( pMgrEnt->HasSpawnFlags( SF_DISABLE_MESH_LARGE ) && V_strcmp( "large", meshName ) == 0 )
			return true;
		else if( pMgrEnt->HasSpawnFlags( SF_DISABLE_MESH_VERYLARGE ) && V_strcmp( "verylarge", meshName ) == 0 )
			return true;
		else if( pMgrEnt->HasSpawnFlags( SF_DISABLE_MESH_AIR ) && V_strcmp( "air", meshName ) == 0 )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Builds all navigation meshes
//-----------------------------------------------------------------------------
bool CRecastMgr::Build( bool loadDefaultMeshes )
{
	double fStartTime = Plat_FloatTime();

	// Load map mesh
	if( !LoadMapMesh() )
	{
		Warning("CRecastMesh::Build: failed to load map data!\n");
		return false;
	}

	// Insert all meshes first
	if( loadDefaultMeshes )
	{
		InitDefaultMeshes();
	}
	CUtlVector<CRecastMesh *> meshesToBuild;
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		if( IsMeshBuildDisabled( m_Meshes[i]->GetName() ) )
			continue;
		meshesToBuild.AddToTail( m_Meshes[i] );
	}

	// Create meshes
	if( recast_build_threaded.GetBool() )
	{
		// Build threaded
		CParallelProcessor<CRecastMesh *, CFuncJobItemProcessor<CRecastMesh *> > processor("BuildNavMesh");
		processor.m_ItemProcessor.Init( &ThreadedBuildMesh, &PreThreadedBuildMesh, &PostThreadedBuildMesh );
		processor.Run( meshesToBuild.Base(), meshesToBuild.Count(), recast_build_numthreads.GetInt(), g_pThreadPool );
	}
	else
	{
		if( V_strlen( recast_build_single.GetString() ) > 0 )
		{
			BuildMesh( m_pMapMesh, recast_build_single.GetString() );
		}
		else
		{
			for( int i = 0; i < meshesToBuild.Count(); i++ )
			{
				BuildMesh( m_pMapMesh, meshesToBuild[i]->GetName() );
			}
		}
	}

	m_bLoaded = true;
	DevMsg( "CRecastMgr: Finished generating %d meshes in %f seconds\n", m_Meshes.Count(), Plat_FloatTime() - fStartTime );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Performs the actual partial rebuilds of the mesh, merging multiple
//			updates in one.
//-----------------------------------------------------------------------------
void CRecastMgr::UpdateRebuildPartial()
{
	if( !m_pendingPartialMeshUpdates.Count() )
		return;

	PartialMeshUpdate_t &curUpdate = m_pendingPartialMeshUpdates.Head();

	bool bDidMerge;
	int nTests = 0, nMerges = 0;
	do {
		bDidMerge = false;
		for( int i = m_pendingPartialMeshUpdates.Count() - 1; i >= 1; i-- )
		{
			if( IsBoxIntersectingBox( curUpdate.vMins, curUpdate.vMaxs,
				m_pendingPartialMeshUpdates[i].vMins, m_pendingPartialMeshUpdates[i].vMaxs ) )
			{
				VectorMin( curUpdate.vMins, m_pendingPartialMeshUpdates[i].vMins, curUpdate.vMins );
				VectorMax( curUpdate.vMaxs, m_pendingPartialMeshUpdates[i].vMaxs, curUpdate.vMaxs );
				m_pendingPartialMeshUpdates.Remove( i );
				bDidMerge = true;
				nMerges++;
			}
		}
		nTests++;
	} while( bDidMerge && nTests < 100 );

	if( recast_build_partial_debug.GetBool() && nMerges > 0 )
	{
		DevMsg( "CRecastMgr::UpdateRebuildPartial: Merged multiple updates (%d) into one\n", nMerges+1 );
	}

	// Load map mesh
	if( !LoadMapMesh( recast_build_partial_debug.GetBool(), true, curUpdate.vMins, curUpdate.vMaxs ) )
	{
		Warning( "CRecastMgr::UpdateRebuildPartial: failed to load map data!\n" );
		return;
	}

	// Perform the update
	CUtlVector<CRecastMesh *> meshesToBuild;
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		if( IsMeshBuildDisabled( m_Meshes[i]->GetName() ) )
			continue;
		meshesToBuild.AddToTail( m_Meshes[i] );
	}

	CParallelProcessor<CRecastMesh *, CFuncJobItemProcessor<CRecastMesh *> > processor("BuildNavMesh");
	processor.m_ItemProcessor.Init( &ThreadedRebuildPartialMesh, &PreThreadedBuildMesh, &PostThreadedBuildMesh );
	processor.Run( meshesToBuild.Base(), meshesToBuild.Count(), recast_build_numthreads.GetInt(), g_pThreadPool );

	m_pendingPartialMeshUpdates.Remove( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Rebuilds a part of the navigation mesh
//-----------------------------------------------------------------------------
bool CRecastMgr::RebuildPartial( const Vector &vMins, const Vector& vMaxs )
{
	PartialMeshUpdate_t update;
	update.vMins = vMins;
	update.vMaxs = vMaxs;
	m_pendingPartialMeshUpdates.AddToTail( update );

	return true;
}

CON_COMMAND( recast_mesh_setcellsize, "" )
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL

	CRecastMesh *pMesh = RecastMgr().GetMesh( args[1] );
	if( !pMesh )
	{
		Warning("recast_mesh_setcellsize: could not find mesh \"%s\"\n", args[1] );
		return;
	}
	pMesh->SetCellSize( atof( args[2] ) );
}

CON_COMMAND( recast_mesh_setcellheight, "" )
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL

	CRecastMesh *pMesh = RecastMgr().GetMesh( args[1] );
	if( !pMesh )
	{
		Warning("recast_mesh_setcellheight: could not find mesh \"%s\"\n", args[1] );
		return;
	}
	pMesh->SetCellHeight( atof( args[2] ) );
}

CON_COMMAND( recast_mesh_settilesize, "" )
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL

	CRecastMesh *pMesh = RecastMgr().GetMesh( args[1] );
	if( !pMesh )
	{
		Warning("recast_mesh_settilesize: could not find mesh \"%s\"\n", args[1] );
		return;
	}
	pMesh->SetTileSize( atof( args[2] ) );
}