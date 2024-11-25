//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Implementation of save/loading the Recast meshes
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#include "recast/recast_file.h"
#include "recast_tilecache_helpers.h"
#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#endif // CLIENT_DLL

#include "datacache/imdlcache.h"
#include <filesystem.h>

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FORMAT_BSPFILE "maps" CORRECT_PATH_SEPARATOR_S "%s.bsp"
#define FORMAT_NAVFILE "maps" CORRECT_PATH_SEPARATOR_S "%s.recast"
#define EXT_NAVFILE "recast"

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 3;

struct NavMgrHeader
{
	int magic;
	int version;
	int numMeshes;
	unsigned int bspSize;
};

struct TileCacheSetHeader
{
	int cellSize;
	int cellHeight;
	int tileSize;
	int numTiles;
	dtNavMeshParams meshParams;
	dtTileCacheParams cacheParams;
};

struct TileCacheTileHeader
{
	dtCompressedTileRef tileRef;
	int dataSize;
};

static char *RecastGetBspFilename( const char *navFilename )
{
	static char bspFilename[256];

#ifdef CLIENT_DLL
	V_snprintf( bspFilename, sizeof( bspFilename ), FORMAT_BSPFILE, engine->GetLevelName() );
#else
	V_snprintf( bspFilename, sizeof( bspFilename ), FORMAT_BSPFILE, STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	int len = V_strlen( bspFilename );
	if (len < 3)
		return NULL;

	bspFilename[ len-3 ] = 'b';
	bspFilename[ len-2 ] = 's';
	bspFilename[ len-1 ] = 'p';

	return bspFilename;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Load( CUtlBuffer &fileBuffer, CMapMesh *pMapMesh )
{
	// Read header.
	TileCacheSetHeader header;
	fileBuffer.Get( &header, sizeof( header ) );
	if( !fileBuffer.IsValid() )
	{
		Log_Error(LOG_RECAST, "Failed to read mesh header.\n" );
		return false;
	}

	m_cellSize = header.cellSize;
	m_cellHeight = header.cellHeight;
	m_tileSize = header.tileSize;

	Init();

	if(developer->GetInt() > 1) {
		Log_Msg( LOG_RECAST, "Loading mesh %s\n", 
			GetName() );
		Log_Msg( LOG_RECAST, "  cell size %f, cell height %f, tile size %f\n", 
			m_cellSize, m_cellHeight, m_tileSize );
		Log_Msg( LOG_RECAST, "  agent radius %f, agent height %f, agent climb %f, agent slope %f\n", 
			m_agentRadius, m_agentHeight, m_agentMaxClimb, m_agentMaxSlope );
	}

	m_navMesh = dtAllocNavMesh();
	if (!m_navMesh)
	{
		return false;
	}
	dtStatus status = m_navMesh->init(&header.meshParams);
	if (dtStatusFailed(status))
	{
		return false;
	}

	m_tileCache = dtAllocTileCache();
	if (!m_tileCache)
	{
		return false;
	}

	status = m_tileCache->init(&header.cacheParams, m_talloc, m_tcomp, m_tmproc);
	if (dtStatusFailed(status))
	{
		Log_Error(LOG_RECAST,"Failed to init tile cache\n");
		return false;
	}
		
	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		TileCacheTileHeader tileHeader;
		fileBuffer.Get( &tileHeader, sizeof(tileHeader) );
		if( !fileBuffer.IsValid() )
			return false;

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) 
			break;
		V_memset(data, 0, tileHeader.dataSize);
		fileBuffer.Get( data, tileHeader.dataSize );
		if( !fileBuffer.IsValid() )
			return false;
		
		dtCompressedTileRef tile = 0;
		m_tileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);

		if (tile)
			m_tileCache->buildNavMeshTile(tile, m_navMesh);
	}

	// Init nav query
	status = m_navQuery->init( m_navMesh, RECAST_NAVQUERY_MAX_NODES );
	if( dtStatusFailed(status) )
	{
		Log_Error(LOG_RECAST,"Could not init Detour navmesh query\n");
		return false;
	}

	status = m_navQueryLimitedNodes->init( m_navMesh, RECAST_NAVQUERY_LIMITED_NODES );
	if (dtStatusFailed(status))
	{
		Log_Error(LOG_RECAST,"Could not init Detour navmesh query");
		return false;
	}

	uint infoCount;
	fileBuffer.Get( &infoCount, sizeof(uint) );

	for(int i = 0; i < infoCount; ++i) {
		dtPolyRef src_ref;
		fileBuffer.Get( &src_ref, sizeof(dtPolyRef) );

		PolyVisibilityInfo &info = m_polyVisibility[ m_polyVisibility.Insert( src_ref ) ];

		uint visCount;
		fileBuffer.Get( &visCount, sizeof(uint) );

		for(int j = 0; j < visCount; ++j) {
			dtPolyRef tgt_ref;
			fileBuffer.Get( &tgt_ref, sizeof(dtPolyRef) );

			uint vertCount;
			fileBuffer.Get( &vertCount, sizeof(uint) );

			auto vis_hndl = info.visible.Insert( tgt_ref );

			for(int k = 0; k < vertCount; ++k) {
				uint32 src_vert;
				fileBuffer.Get( &src_vert, sizeof(uint32) );
				uint32 dst_vert;
				fileBuffer.Get( &dst_vert, sizeof(uint32) );

				info.visible[ vis_hndl ].verts.AddToTail( PolyVisibilityInfo::VisibilityConnectionInfo::vertPair{src_vert, dst_vert} );
			}
		}

		fileBuffer.Get( &visCount, sizeof(uint) );

		for(int j = 0; j < visCount; ++j) {
			dtPolyRef tgt_ref;
			fileBuffer.Get( &tgt_ref, sizeof(dtPolyRef) );

			uint vertCount;
			fileBuffer.Get( &vertCount, sizeof(uint) );

			auto not_vis_hndl = info.not_visible.Insert( tgt_ref );

			for(int k = 0; k < vertCount; ++k) {
				uint32 src_vert;
				fileBuffer.Get( &src_vert, sizeof(uint32) );
				uint32 dst_vert;
				fileBuffer.Get( &dst_vert, sizeof(uint32) );

				info.not_visible[ not_vis_hndl ].verts.AddToTail( PolyVisibilityInfo::VisibilityConnectionInfo::vertPair{src_vert, dst_vert} );
			}
		}
	}

	PostLoad();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::Load()
{
	if( m_bLoaded )
		return true;

	double fStartTime = Plat_FloatTime();
	m_bLoaded = false;

	Reset();

	MDLCACHE_CRITICAL_SECTION();

	char filename[256];
#ifdef CLIENT_DLL
	V_snprintf( filename, sizeof( filename ), "%s", engine->GetLevelName() );
	V_StripExtension( filename, filename, 256 );
	V_snprintf( filename, sizeof( filename ), "%s.%s", filename, EXT_NAVFILE );
#else
	V_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	bool navIsInBsp = false;
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( g_pFullFileSystem->ReadFile( filename, "BSP", fileBuffer ) )
	{
		navIsInBsp = true;
	}
	else
	{
		if ( !g_pFullFileSystem->ReadFile( filename, "GAME_NOBSP", fileBuffer ) )
		{
			InitMeshes();
			return false;
		}
	}

	// Read header of nav mesh file
	NavMgrHeader header;
	fileBuffer.Get( &header, sizeof( header ) );
	if ( !fileBuffer.IsValid() || header.magic != NAVMESHSET_MAGIC )
	{
		Log_Error(LOG_RECAST, "Invalid navigation file '%s'.\n", filename );
		return false;
	}

	// read file version number
	if ( !fileBuffer.IsValid() || header.version != NAVMESHSET_VERSION )
	{
		Log_Error(LOG_RECAST, "Unknown navigation file version.\n" );
		return false;
	}

	// verify size
	char *bspFilename = RecastGetBspFilename( filename );
	if ( bspFilename == NULL )
	{
		Log_Error(LOG_RECAST, "CRecastMgr::Load: Could not get bsp file name\n" );
		return false;
	}

	unsigned int bspSize = g_pFullFileSystem->Size( bspFilename );
	if ( bspSize != header.bspSize && !navIsInBsp )
	{
		Log_Warning( LOG_RECAST,"The Navigation Mesh was built using a different version of this map.\n" );
	}

	int numLoaded = 0;

	// Read the meshes!
	for( int i = 0; i < header.numMeshes; i++ )
	{
		int lenName = 0;
		fileBuffer.Get(&lenName, sizeof(int));

		if( lenName > 2048 )
		{
			Log_Error( LOG_RECAST, "Mesh name too long. Invalid mesh file?\n" );
			return false;
		}

		char *szName = (char *)stackalloc( sizeof( char ) * (lenName + 1) );
		fileBuffer.Get( szName, lenName );
		szName[lenName] = 0;

		NavMeshType_t type = NAI_Hull::LookupId( szName );
		if( type == RECAST_NAVMESH_INVALID )
		{
			Msg( "Mesh name %s does not correspond to any type\n", szName );
			return false;
		}

		CRecastMesh *pMesh = new CRecastMesh( type );
		if( !pMesh->Load( fileBuffer ) ) {
			delete pMesh;
			return false;
		}

		m_Meshes[type] = pMesh;

		numLoaded++;
	}
	
	m_bLoaded = true;
	Log_Msg( LOG_RECAST,"CRecastMgr: Loaded %d nav meshes in %f seconds\n", numLoaded, Plat_FloatTime() - fStartTime );

	return true;
}

#ifndef CLIENT_DLL
bool CRecastMesh::Save( CUtlBuffer &fileBuffer )
{
	if(developer->GetInt() > 1) {
		Log_Msg( LOG_RECAST, "Saving mesh %s\n", 
			GetName() );
		Log_Msg( LOG_RECAST, "  cell size %f, cell height %f, tile size %f\n", 
			m_cellSize, m_cellHeight, m_tileSize );
		Log_Msg( LOG_RECAST, "  agent radius %f, agent height %f, agent climb %f, agent slope %f\n", 
			m_agentRadius, m_agentHeight, m_agentMaxClimb, m_agentMaxSlope );
	}

	const char *szName = GetName();
	int nameLen = V_strlen( szName );
	fileBuffer.Put( &nameLen, sizeof(int) );
	fileBuffer.Put( szName, nameLen );

	// Store header.
	TileCacheSetHeader header;
	header.cellSize = m_cellSize;
	header.cellHeight = m_cellHeight;
	header.tileSize = m_tileSize;
	header.numTiles = 0;

	if(m_tileCache) {
		for (int i = 0; i < m_tileCache->getTileCount(); ++i)
		{
			const dtCompressedTile* tile = m_tileCache->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) 
				continue;
			header.numTiles++;
		}
	}

	if(m_tileCache)
		memcpy(&header.cacheParams, m_tileCache->getParams(), sizeof(dtTileCacheParams));
	else
		memset(&header.cacheParams, 0, sizeof(dtTileCacheParams));
	if(m_navMesh)
		memcpy(&header.meshParams, m_navMesh->getParams(), sizeof(dtNavMeshParams));
	else
		memset(&header.meshParams, 0, sizeof(dtNavMeshParams));
	fileBuffer.Put( &header, sizeof( header ) );

	// Store tiles.
	if(m_tileCache) {
		for (int i = 0; i < m_tileCache->getTileCount(); ++i)
		{
			const dtCompressedTile* tile = m_tileCache->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) continue;

			TileCacheTileHeader tileHeader;
			tileHeader.tileRef = m_tileCache->getTileRef(tile);
			tileHeader.dataSize = tile->dataSize;
			fileBuffer.Put( &tileHeader, sizeof( tileHeader ) );

			fileBuffer.Put( tile->data, tile->dataSize );
		}
	}

	uint visCount = m_polyVisibility.Count();
	fileBuffer.Put( &visCount, sizeof(uint) );

	FOR_EACH_HASHTABLE( m_polyVisibility, it )
	{
		dtPolyRef src_ref = m_polyVisibility.Key( it );
		fileBuffer.Put( &src_ref, sizeof(dtPolyRef) );

		const PolyVisibilityInfo &info = m_polyVisibility.Element( it );

		visCount = info.visible.Count();
		fileBuffer.Put( &visCount, sizeof(uint) );

		FOR_EACH_HASHTABLE( info.visible, it2 )
		{
			dtPolyRef tgt_ref = info.visible.Key( it2 );

			const PolyVisibilityInfo::VisibilityConnectionInfo &conn_info = info.visible.Element(it2);

			fileBuffer.Put( &tgt_ref, sizeof(dtPolyRef) );

			visCount = conn_info.verts.Count();
			fileBuffer.Put( &visCount, sizeof(uint) );

			for(int k = 0; k < visCount; ++k) {
				const PolyVisibilityInfo::VisibilityConnectionInfo::vertPair &vert = conn_info.verts[k];
				fileBuffer.Put( &vert.src_vert, sizeof(uint32) );
				fileBuffer.Put( &vert.dst_vert, sizeof(uint32) );
			}
		}

		FOR_EACH_HASHTABLE( info.not_visible, it2 )
		{
			dtPolyRef tgt_ref = info.not_visible.Key( it2 );

			const PolyVisibilityInfo::VisibilityConnectionInfo &conn_info = info.not_visible.Element(it2);

			fileBuffer.Put( &tgt_ref, sizeof(dtPolyRef) );

			visCount = conn_info.verts.Count();
			fileBuffer.Put( &visCount, sizeof(uint) );

			for(int k = 0; k < visCount; ++k) {
				const PolyVisibilityInfo::VisibilityConnectionInfo::vertPair &vert = conn_info.verts[k];
				fileBuffer.Put( &vert.src_vert, sizeof(uint32) );
				fileBuffer.Put( &vert.dst_vert, sizeof(uint32) );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return the filename for this map's "nav map" file
//-----------------------------------------------------------------------------
const char *CRecastMgr::GetFilename( void ) const
{
	// persistant return value
	static char filename[256];
	V_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );

	return filename;
}

//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
bool CRecastMgr::Save()
{
	const char *filename = GetFilename();
	if( filename == NULL )
		return false;

	CUtlBuffer fileBuffer( 4096, 1024*1024 );

	char *bspFilename = RecastGetBspFilename( filename );
	if (bspFilename == NULL)
	{
		return false;
	}

	unsigned int bspSize  = g_pFullFileSystem->Size( bspFilename );
	Log_Msg( LOG_RECAST, "Size of bsp file '%s' is %u bytes.\n", bspFilename, bspSize );

	CUtlVector< CRecastMesh * > meshesToSave;
	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		if(!m_Meshes[i])
			continue;
		// Mesh is not build, but could still be there from previous build when it was not disabled.
		if( IsMeshBuildDisabled( m_Meshes[i]->GetType() ) )
			continue;
		meshesToSave.AddToTail( m_Meshes[i] );
	}

	NavMgrHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numMeshes = meshesToSave.Count();
	header.bspSize = bspSize;
	fileBuffer.Put( &header, sizeof( header ) );

	for( int i = 0; i < meshesToSave.Count(); i++ )
	{
		meshesToSave[i]->Save( fileBuffer );
	}

	if ( !g_pFullFileSystem->WriteFile( filename, "GAME", fileBuffer ) )
	{
		Log_Warning( LOG_RECAST, "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
		return false;
	}

	unsigned int navSize = g_pFullFileSystem->Size( filename );
	Log_Msg( LOG_RECAST, "Size of nav file '%s' is %u bytes.\n", filename, navSize );

	return true;
}

#endif // CLIENT_DLL
