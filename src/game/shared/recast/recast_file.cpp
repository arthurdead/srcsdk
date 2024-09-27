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
static const int NAVMESHSET_VERSION = 2;

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
		Msg( "Failed to read mesh header.\n" );
		return false;
	}

	m_cellSize = header.cellSize;
	m_cellHeight = header.cellHeight;
	m_tileSize = header.tileSize;

	Init();

	DevMsg( 2, "Loading mesh %s with cell size %f, cell height %f, tile size %f\n", 
		GetName(), m_cellSize, m_cellHeight, m_tileSize );

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
		Warning("Failed to init tile cache\n");
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
		Warning("Could not init Detour navmesh query\n");
		return false;
	}

	status = m_navQueryLimitedNodes->init( m_navMesh, RECAST_NAVQUERY_LIMITED_NODES );
	if (dtStatusFailed(status))
	{
		Warning("Could not init Detour navmesh query");
		return false;
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
	V_snprintf( filename, sizeof( filename ), "%s", STRING( engine->GetLevelName() ) );
	V_StripExtension( filename, filename, 256 );
	V_snprintf( filename, sizeof( filename ), "%s.%s", filename, EXT_NAVFILE );
#else
	V_snprintf( filename, sizeof( filename ), FORMAT_NAVFILE, STRING( gpGlobals->mapname ) );
#endif // CLIENT_DLL

	bool navIsInBsp = false;
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::READ_ONLY );
	if ( !g_pFullFileSystem->ReadFile( filename, "MOD", fileBuffer ) )	// this ignores .nav files embedded in the .bsp ...
	{
		navIsInBsp = true;
		if ( !g_pFullFileSystem->ReadFile( filename, "BSP", fileBuffer ) )	// ... and this looks for one if it's the only one around.
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
		Msg( "Invalid navigation file '%s'.\n", filename );
		return false;
	}

	// read file version number
	if ( !fileBuffer.IsValid() || header.version > NAVMESHSET_VERSION )
	{
		Msg( "Unknown navigation file version.\n" );
		return false;
	}

	// verify size
	char *bspFilename = RecastGetBspFilename( filename );
	if ( bspFilename == NULL )
	{
		Msg( "CRecastMgr::Load: Could not get bsp file name\n" );
		return false;
	}

	unsigned int bspSize = g_pFullFileSystem->Size( bspFilename );
	if ( bspSize != header.bspSize && !navIsInBsp )
	{
#ifndef CLIENT_DLL
		if ( engine->IsDedicatedServer() )
		{
			// Warning doesn't print to the dedicated server console, so we'll use Msg instead
			DevMsg( "The Navigation Mesh was built using a different version of this map.\n" );
		}
		else
#endif // CLIENT_DLL
		{
			DevWarning( "The Navigation Mesh was built using a different version of this map.\n" );
		}
	}

	int numLoaded = 0;

	// Read the meshes!
	for( int i = 0; i < header.numMeshes; i++ )
	{
		int lenName = 0;
		fileBuffer.Get(&lenName, sizeof(int));

		if( lenName > 2048 )
		{
			Msg( "Mesh name too long. Invalid mesh file?\n" );
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
	DevMsg( "CRecastMgr: Loaded %d nav meshes in %f seconds\n", numLoaded, Plat_FloatTime() - fStartTime );

	return true;
}

#ifndef CLIENT_DLL
bool CRecastMesh::Save( CUtlBuffer &fileBuffer )
{
	Msg( "Saving mesh %s with cell size %f, cell height %f, tile size %f\n", 
		GetName(), m_cellSize, m_cellHeight, m_tileSize );

	int nameLen = V_strlen( GetName() );
	fileBuffer.Put( GetName(), nameLen );

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
	DevMsg( "Size of bsp file '%s' is %u bytes.\n", bspFilename, bspSize );

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

	if ( !g_pFullFileSystem->WriteFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to save %d bytes to %s\n", fileBuffer.Size(), filename );
		return false;
	}

	unsigned int navSize = g_pFullFileSystem->Size( filename );
	DevMsg( "Size of nav file '%s' is %u bytes.\n", filename, navSize );

	return true;
}

#endif // CLIENT_DLL
