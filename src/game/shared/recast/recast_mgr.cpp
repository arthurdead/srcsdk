//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"
#include "game_loopback/igameserverloopback.h"
#include "filesystem.h"
#include "collisionproperty.h"

#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#include "player.h"
#include "props.h"
#include "ai_basenpc.h"
#else
#include "c_baseplayer.h"
#include "c_ai_basenpc.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_RECAST, "Recast Server" );
#else
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_RECAST, "Recast Client" );
#endif

ConVar recast_debug_mesh( "recast_debug_mesh", "HUMAN", FCVAR_REPLICATED|FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Accessor
//-----------------------------------------------------------------------------
static CRecastMgr s_RecastMgr;
CRecastMgr &RecastMgr()
{
	return s_RecastMgr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMgr::CRecastMgr() : m_bLoaded(false), m_Obstacles( 0, 0, DefLessFunc( EHANDLE ) )
{
#ifndef CLIENT_DLL
	for(int i = 0; i < RECAST_MAPMESH_NUM; ++i)
		m_pMapMeshes[i] = NULL;
#endif // CLIENT_DLL

	for(int i = 0; i < RECAST_NAVMESH_NUM; ++i)
		m_Meshes[i] = NULL;

	m_placeCount = 0;
	m_placeName = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMgr::~CRecastMgr()
{
	// !!!!bug!!! why does this crash in linux on server exit
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		delete [] m_placeName[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Init()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Reset()
{
	m_bLoaded = false;

	for(int i = 0; i < RECAST_NAVMESH_NUM; ++i) {
		if( m_Meshes[i] )
		{
			delete m_Meshes[i];
			m_Meshes[i] = NULL;
		}
	}

	m_Obstacles.Purge();

#ifndef CLIENT_DLL
	m_pendingPartialMeshUpdates.Purge();

	for(int i = 0; i < RECAST_MAPMESH_NUM; ++i) {
		if( m_pMapMeshes[i] )
		{
			delete m_pMapMeshes[i];
			m_pMapMeshes[i] = NULL;
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::InitMeshes()
{
	// Ensures default meshes exists, even if they don't have a mesh loaded.
	for( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = GetMesh( (NavMeshType_t)i );
		if( !pMesh )
		{
			pMesh = new CRecastMesh( (NavMeshType_t)i );
			pMesh->Init();
			m_Meshes[i] = pMesh;
		}
	}

	return true;
}

CRecastMesh *CRecastMgr::GetMesh( NavMeshType_t type )
{
	if(type == RECAST_NAVMESH_INVALID)
		return NULL;
	int idx = FindMeshIndex( type );
	if( idx != -1 )
		return GetMeshByIndex( idx );
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Update( float dt )
{
	VPROF_BUDGET( "CRecastMgr::Update", "RecastNav" );

#ifndef CLIENT_DLL
	UpdateRebuildPartial();
#endif // CLIENT_DLL

	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		if(!pMesh)
			continue;
		pMesh->Update( dt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::GetMeshByIndex( int index )
{
	return m_Meshes[index];
}

//-----------------------------------------------------------------------------
// Purpose: Determines best nav mesh radius/height
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::FindBestMeshForRadiusHeight( float radius, float height )
{
	int bestIdx = -1;
	float fBestRadiusDiff = 0;
	float fBestHeightDiff = 0;
	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		if(!pMesh)
			continue;

		if( !pMesh->IsLoaded() )
		{
			continue;
		}

		// Only consider fitting meshes
		if( radius > pMesh->GetAgentRadius() || height > pMesh->GetAgentHeight() )
		{
			continue;
		}

		// From these meshes, pick the best fitting one
		float fRadiusDiff = fabs( pMesh->GetAgentRadius() - radius );
		float fHeightDiff = fabs( pMesh->GetAgentHeight() - height );
		if( bestIdx == -1 || (fRadiusDiff + fHeightDiff <= fBestRadiusDiff + fBestHeightDiff) )
		{
			bestIdx = i;
			fBestRadiusDiff = fRadiusDiff;
			fBestHeightDiff = fHeightDiff;
		}
	}
	if(bestIdx == -1)
		return NULL;
	return GetMeshByIndex( bestIdx );
}

//-----------------------------------------------------------------------------
// Purpose: Determines best nav mesh for entity
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::FindBestMeshForEntity( CSharedBaseEntity *pEntity )
{
	if( !pEntity )
		return NULL;

	CShared_AI_BaseNPC *pNPC = pEntity->MyNPCPointer();
	if(pNPC) {
		return FindBestMeshForRadiusHeight( pNPC->BoundingRadius2D(), pNPC->GetHullHeight() );
	} else if(pEntity->IsPlayer()) {
		CSharedBasePlayer *pPlayer = ToBasePlayer( pEntity );

		float flRadius2D = VIEW_VECTORS->m_flRadius2D * pPlayer->GetModelScale();

		float flHeight = VIEW_VECTORS->m_flHeight * pPlayer->GetModelScale();

		return FindBestMeshForRadiusHeight( flRadius2D, flHeight );
	} else {
		return FindBestMeshForRadiusHeight( pEntity->BoundingRadius2D(), pEntity->CollisionProp()->Height() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRecastMgr::FindMeshIndex( NavMeshType_t type )
{
	if(type == RECAST_NAVMESH_INVALID)
		return -1;

	return type;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::IsMeshLoaded( NavMeshType_t type )
{
	CRecastMesh *pMesh = GetMesh( type );
	return pMesh != NULL && pMesh->IsLoaded();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
NavMeshType_t CRecastMgr::FindBestMeshTypeForRadiusHeight( float radius, float height )
{
	CRecastMesh *pMesh = FindBestMeshForRadiusHeight( radius, height );
	return pMesh ? pMesh->GetType() : RECAST_NAVMESH_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
NavMeshType_t CRecastMgr::FindBestMeshTypeForEntity( CSharedBaseEntity *pEntity )
{
	CRecastMesh *pMesh = FindBestMeshForEntity( pEntity );
	return pMesh ? pMesh->GetType() : RECAST_NAVMESH_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtNavMesh* CRecastMgr::GetNavMesh( NavMeshType_t type )
{
	int idx = FindMeshIndex( type );
	if( m_Meshes[idx] )
	{
		return m_Meshes[idx]->GetNavMesh();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtNavMeshQuery* CRecastMgr::GetNavMeshQuery( NavMeshType_t type )
{
	int idx = FindMeshIndex( type );
	if( m_Meshes[idx] )
	{
		return m_Meshes[idx]->GetNavMeshQuery();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IMapMesh* CRecastMgr::GetMapMesh( MapMeshType_t type )
{
#ifdef CLIENT_DLL
	return NULL;
#else
	return m_pMapMeshes[ type ];
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Adds an obstacle with radius and height for entity
//-----------------------------------------------------------------------------
bool CRecastMgr::AddEntRadiusObstacle( CSharedBaseEntity *pEntity, float radius, float height )
{
	if( !pEntity )
		return false;

	NavObstacleArray_t &obstacle = FindOrCreateObstacle( pEntity );
	obstacle.areaId = DetermineAreaID( pEntity, -Vector( radius, radius, radius ), Vector( radius, radius, radius ) );

	bool bSuccess = true;
	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		if(!pMesh)
			continue;

		// TODO: better check. Needs to filter out obstacles for air mesh
		if( pMesh->GetAgentHeight() - 50.0f > height )
			continue;

		obstacle.obs.AddToTail();
		obstacle.obs.Tail().meshIndex = i;
		obstacle.obs.Tail().ref = pMesh->AddTempObstacle( pEntity->GetAbsOrigin(), radius + 1.0f, height, obstacle.areaId );
		if( obstacle.obs.Tail().ref == 0 )
			bSuccess = false;
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Adds an obstacle based on bounds and height for entity
// TODO: 
//-----------------------------------------------------------------------------
bool CRecastMgr::AddEntBoxObstacle( CSharedBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height )
{
	if( !pEntity )
		return false;

	matrix3x4_t transform; // model to world transformation
	AngleMatrix( QAngle(0, pEntity->GetAbsAngles()[YAW], 0), pEntity->GetAbsOrigin(), transform );

	Vector convexHull[4];

	NavObstacleArray_t &obstacle = FindOrCreateObstacle( pEntity );
	obstacle.areaId = DetermineAreaID( pEntity, mins, maxs );

	bool bSuccess = true;
	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		if(!pMesh)
			continue;

		// TODO: better check. Needs to filter out obstacles for air mesh
		if( pMesh->GetAgentHeight() - 50.0f > height )
			continue;

		float erodeDist = pMesh->GetAgentRadius() + 8.0f;

		VectorTransform( mins + Vector(-erodeDist, -erodeDist, 0), transform, convexHull[0] );
		VectorTransform( Vector(mins.x, maxs.y, mins.z) + Vector(-erodeDist, erodeDist, 0), transform, convexHull[1] );
		VectorTransform( Vector(maxs.x, maxs.y, mins.z) + Vector(erodeDist, erodeDist, 0), transform, convexHull[2] );
		VectorTransform( Vector(maxs.x, mins.y, mins.z) + Vector(erodeDist, -erodeDist, 0), transform, convexHull[3] );

		/*for( int j = 0; j < 4; j++ )
		{
			int next = (j+1) == 4 ? 0 : j + 1;
			NDebugOverlay::Line( convexHull[j], convexHull[next], 0, 255, 0, true, 10.0f );
		}*/

		obstacle.obs.AddToTail();
		obstacle.obs.Tail().meshIndex = i;
		obstacle.obs.Tail().ref = pMesh->AddTempObstacle( pEntity->GetAbsOrigin(), convexHull, 4, height, obstacle.areaId );

		if( obstacle.obs.Tail().ref == 0 )
			bSuccess = false;
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Removes any obstacle associated with the entity
//-----------------------------------------------------------------------------
bool CRecastMgr::RemoveEntObstacles( CSharedBaseEntity *pEntity )
{
	if( !pEntity )
		return false;

	bool bSuccess = true;
	int idx = m_Obstacles.Find( pEntity );
	if( m_Obstacles.IsValidIndex( idx ) )
	{
		for( int i = 0; i < m_Obstacles[idx].obs.Count(); i++ )
		{
			CRecastMesh *pMesh = m_Meshes[ m_Obstacles[idx].obs[i].meshIndex ];
			if(!pMesh)
				bSuccess = false;
			else if( !pMesh->RemoveObstacle( m_Obstacles[idx].obs[i].ref ) )
				bSuccess = false;
		}

		m_Obstacles.RemoveAt( idx );
		pEntity->SetNavObstacleRef( NAV_OBSTACLE_INVALID_INDEX );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines the area id in such a way potential touching obstacles
//			don't have the same area id. There are about 16 available area ids.
//-----------------------------------------------------------------------------
unsigned char CRecastMgr::DetermineAreaID( CSharedBaseEntity *pEntity, const Vector &mins, const Vector &maxs )
{
	unsigned char areaId = POLYAREA_OBSTACLE_START;

	// Determine areaId
	bool usedPolyAreas[20];
	V_memset( usedPolyAreas, 0, ARRAYSIZE(usedPolyAreas) * sizeof(bool) );
	CSharedBaseEntity *pEnts[256];
	int n = UTIL_EntitiesInBox( pEnts, 256, pEntity->GetAbsOrigin() + mins - Vector(64.0f, 64.0f, 64.0f), pEntity->GetAbsOrigin() + maxs + Vector(64.0f, 64.0f, 64.0f), 0 );
	for( int i = 0; i < n; i++ )
	{
		CSharedBaseEntity *pEnt = pEnts[i];
		if( !pEnt || pEnt == pEntity || pEnt->GetNavObstacleRef() == NAV_OBSTACLE_INVALID_INDEX )
		{
			continue;
		}

		unsigned char otherAreaId = m_Obstacles.Element( pEnt->GetNavObstacleRef() ).areaId;

		// Sanity check, would indicate an uninitialized obstacle if this happens.
		if( otherAreaId < POLYAREA_OBSTACLE_START || otherAreaId > POLYAREA_OBSTACLE_END )
		{
			Warning( "Obstacle has valid ref, but invalid area id? %d\n", otherAreaId );
			continue;
		}
		usedPolyAreas[otherAreaId] = true;
	}

	while( areaId != POLYAREA_OBSTACLE_END - 1 && usedPolyAreas[areaId] )
	{
		areaId++;
	}

	return areaId;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
NavObstacleArray_t &CRecastMgr::FindOrCreateObstacle( CSharedBaseEntity *pEntity )
{
	int idx = m_Obstacles.Find( pEntity );
	if( !m_Obstacles.IsValidIndex( idx ) )
	{
		idx = m_Obstacles.Insert( pEntity );
		pEntity->SetNavObstacleRef( idx );
	}
	return m_Obstacles[idx];
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
void CRecastMgr::DebugRender()
{
	NavMeshType_t type = NAI_Hull::LookupId( recast_debug_mesh.GetString() );
	if(type == RECAST_NAVMESH_INVALID)
		return;

	if( !m_Meshes[type] )
	{
		// Might be visualizing a server mesh that does not exist on the client
		// Insert dummy mesh on the fly.
		IRecastMgr *pRecastMgr = g_pGameServerLoopback ? g_pGameServerLoopback->GetRecastMgr() : NULL;
		if( pRecastMgr && pRecastMgr->GetNavMesh( type ) )
		{
			CRecastMesh *pMesh = new CRecastMesh( type );
			pMesh->Init();
			m_Meshes[type] = pMesh;
		}
	}

	if(m_Meshes[type])
		m_Meshes[type]->DebugRender();
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Prints list of navigation meshes
//-----------------------------------------------------------------------------
void CRecastMgr::DebugListMeshes()
{
	for ( int i = 0; i < RECAST_NAVMESH_NUM; i++ )
	{
		CRecastMesh *pMesh = m_Meshes[i];
		if(!pMesh)
			continue;

		Log_Msg( LOG_RECAST, "%d: %s (agent radius: %f, height: %f, climb: %f, slope: %f, cell size: %f, cell height: %f)\n", i, 
			pMesh->GetName(),
			pMesh->GetAgentRadius(), pMesh->GetAgentHeight(), pMesh->GetAgentMaxClimb(), pMesh->GetAgentMaxSlope(),
			pMesh->GetCellSize(), pMesh->GetCellHeight());
	}
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_loadmapmesh, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	for(int i = 0; i < RECAST_MAPMESH_NUM; ++i)
		s_RecastMgr.LoadMapMesh( (MapMeshType_t)i );
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_reload, "Reload the Recast Navigation Mesh from disk on server", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_reload, "Reload the Recast Navigation Mesh from disk on client", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL

	s_RecastMgr.Load();
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_build, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	s_RecastMgr.Build();
	s_RecastMgr.Save();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if( pPlayer )
	{
		engine->ClientCommand( pPlayer->edict(), "cl_recast_reload\n" );
	}
}

CON_COMMAND_F( recast_save, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	s_RecastMgr.Save();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if( pPlayer )
	{
		engine->ClientCommand( pPlayer->edict(), "cl_recast_reload\n" );
	}
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_listmeshes, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_listmeshes, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL

	s_RecastMgr.DebugListMeshes();
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_readd_phys_props, "", FCVAR_CHEAT )
{
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
	{
		if( FClassnameIs( pEntity, gm_isz_class_PropPhysics ) )
		{
			CBaseProp *pProp = dynamic_cast< CBaseProp * >( pEntity );
			if( pProp )
				pProp->UpdateNavObstacle( true );
		}
	}
}
#endif // CLIENT_DLL

//--------------------------------------------------------------------------------------------------------------
//
// The 'place directory' is used to save and load places from
// nav files in a size-efficient manner that also allows for the 
// order of the place ID's to change without invalidating the
// nav files.
//
// The place directory is stored in the nav file as a list of 
// place name strings.  Each nav area then contains an index
// into that directory, or zero if no place has been assigned to 
// that area.
//
PlaceDirectory::PlaceDirectory( void )
{
	Reset();
}

void PlaceDirectory::Reset( void )
{
	m_directory.RemoveAll();
	m_hasUnnamedAreas = false;
}

/// return true if this place is already in the directory
bool PlaceDirectory::IsKnown( Place place ) const
{
	return m_directory.HasElement( place );
}

/// return the directory index corresponding to this Place (0 = no entry)
PlaceDirectory::IndexType PlaceDirectory::GetIndex( Place place ) const
{
	if (place == UNDEFINED_PLACE)
		return 0;

	int i = m_directory.Find( place );

	if (i < 0)
	{
		AssertMsg( false, "PlaceDirectory::GetIndex failure" );
		return 0;
	}

	return (IndexType)(i+1);
}

/// add the place to the directory if not already known
void PlaceDirectory::AddPlace( Place place )
{
	if (place == UNDEFINED_PLACE)
	{
		m_hasUnnamedAreas = true;
		return;
	}

	Assert( place < 1000 );

	if (IsKnown( place ))
		return;

	m_directory.AddToTail( place );
}

/// given an index, return the Place
Place PlaceDirectory::IndexToPlace( IndexType entry ) const
{
	if (entry == 0)
		return UNDEFINED_PLACE;

	int i = entry-1;

	if (i >= m_directory.Count())
	{
		AssertMsg( false, "PlaceDirectory::IndexToPlace: Invalid entry" );
		return UNDEFINED_PLACE;
	}

	return m_directory[ i ];
}

/// store the directory
void PlaceDirectory::Save( CUtlBuffer &fileBuffer )
{
	// store number of entries in directory
	IndexType count = (IndexType)m_directory.Count();
	fileBuffer.PutUnsignedShort( count );

	// store entries		
	for( int i=0; i<m_directory.Count(); ++i )
	{
		const char *placeName = RecastMgr().PlaceToName( m_directory[i] );

		// store string length followed by string itself
		unsigned short len = (unsigned short)(strlen( placeName ) + 1);
		fileBuffer.PutUnsignedShort( len );
		fileBuffer.Put( placeName, len );
	}

	fileBuffer.PutUnsignedChar( m_hasUnnamedAreas );
}

/// load the directory
void PlaceDirectory::Load( CUtlBuffer &fileBuffer, int version )
{
	// read number of entries
	IndexType count = fileBuffer.GetUnsignedShort();

	m_directory.RemoveAll();

	// read each entry
	char placeName[256];
	unsigned short len;
	for( int i=0; i<count; ++i )
	{
		len = fileBuffer.GetUnsignedShort();
		fileBuffer.Get( placeName, MIN( sizeof( placeName ), len ) );

		Place place = RecastMgr().NameToPlace( placeName );
		if (place == UNDEFINED_PLACE)
		{
			Warning( "Warning: NavMesh place %s is undefined?\n", placeName );
		}
		AddPlace( place );
	}

	if ( version > 11 )
	{
		m_hasUnnamedAreas = fileBuffer.GetUnsignedChar() != 0;
	}
}

PlaceDirectory placeDirectory;

//--------------------------------------------------------------------------------------------------------------
/**
 * Load the place names from a file
 */
void CRecastMgr::LoadPlaceDatabase( void )
{
	m_placeCount = 0;

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	g_pFullFileSystem->ReadFile("NavPlace.db", "GAME", buf);

	if (!buf.Size())
		return;

	const int maxNameLength = 128;
	char buffer[ maxNameLength ];

	CUtlVector<char*> placeNames;

	// count the number of places
	while( true )
	{
		buf.GetLine( buffer, maxNameLength );

		if ( !buf.IsValid() )
			break;

		int len = V_strlen( buffer );
		if ( len >= 2 )
		{
			if ( buffer[len-1] == '\n' || buffer[len-1] == '\r' )
				buffer[len-1] = 0;
			
			if ( buffer[len-2] == '\r' )
				buffer[len-2] = 0;

			char *pName = new char[ len + 1 ];
			V_strncpy( pName, buffer, len+1 );
			placeNames.AddToTail( pName );
		}
	}

	// allocate place name array
	m_placeCount = placeNames.Count();
	m_placeName = new char * [ m_placeCount ];
	
	for ( unsigned int i=0; i < m_placeCount; i++ )
	{
		m_placeName[i] = placeNames[i];
	}
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Given a place, return its name.
 * Reserve zero as invalid.
 */
const char *CRecastMgr::PlaceToName( Place place ) const
{
	if (place >= 1 && place <= m_placeCount)
		return m_placeName[ (int)place - 1 ];

	return NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a place name, return a place ID or zero if no place is defined
 * Reserve zero as invalid.
 */
Place CRecastMgr::NameToPlace( const char *name ) const
{
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if (FStrEq( m_placeName[i], name ))
			return i+1;
	}

	return UNDEFINED_PLACE;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
 */
Place CRecastMgr::PartialNameToPlace( const char *name ) const
{
	Place found = UNDEFINED_PLACE;
	bool isAmbiguous = false;
	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if (!strnicmp( m_placeName[i], name, strlen( name ) ))
		{
			// check for exact match in case of subsets of other strings
			if (!stricmp( m_placeName[i], name ))
			{
				found = NameToPlace( m_placeName[i] );
				isAmbiguous = false;
				break;
			}

			if (found != UNDEFINED_PLACE)
			{
				isAmbiguous = true;
			}
			else
			{
				found = NameToPlace( m_placeName[i] );
			}
		}
	}

	if (isAmbiguous)
		return UNDEFINED_PLACE;

	return found;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Given a partial place name, fill in possible place names for ConCommand autocomplete
 */
int CRecastMgr::PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int numMatches = 0;
	partial += Q_strlen( "nav_use_place " );
	int partialLength = Q_strlen( partial );

	for( unsigned int i=0; i<m_placeCount; ++i )
	{
		if ( !Q_strnicmp( m_placeName[i], partial, partialLength ) )
		{
			// Add the place name to the autocomplete array
			Q_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "nav_use_place %s", m_placeName[i] );

			// Make sure we don't try to return too many place names
			if ( numMatches == COMMAND_COMPLETION_MAXITEMS )
				return numMatches;
		}
	}

	return numMatches;
}

//--------------------------------------------------------------------------------------------------------------
typedef const char * SortStringType;
int StringSort (const SortStringType *s1, const SortStringType *s2)
{
	return strcmp( *s1, *s2 );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Output a list of names to the console
 */
void CRecastMgr::PrintAllPlaces( void ) const
{
	if (m_placeCount == 0)
	{
		Log_Warning( LOG_RECAST, "There are no entries in the Place database.\n" );
		return;
	}

	unsigned int i;

	CUtlVector< SortStringType > placeNames;
	for ( i=0; i<m_placeCount; ++i )
	{
		placeNames.AddToTail( m_placeName[i] );
	}
	placeNames.Sort( StringSort );

	for( i=0; i<(unsigned int)placeNames.Count(); ++i )
	{
		if (NameToPlace( placeNames[i] ) == GetNavPlace())
			Log_Msg( LOG_RECAST, "--> %-26s", placeNames[i] );
		else
			Log_Msg( LOG_RECAST, "%-30s", placeNames[i] );

		if ((i+1) % 3 == 0)
			Log_Msg( LOG_RECAST, "\n" );
	}

	Log_Msg( LOG_RECAST, "\n" );
}
