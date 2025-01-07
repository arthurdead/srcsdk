//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast_tilecache_helpers.h"
#include "recast/recast_mesh.h"
#ifndef CLIENT_DLL
#include "recast/recast_offmesh_connection.h"
#endif // CLIENT_DLL

#include "DetourNavMeshBuilder.h"
#include "DetourCommon.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"

#include "fastlz/fastlz.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int FastLZCompressor::maxCompressedSize(const int bufferSize)
{
	return (int)(bufferSize* 1.05f);
}
	
dtStatus FastLZCompressor::compress(const unsigned char* buffer, const int bufferSize,
							unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
{

	*compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
	return DT_SUCCESS;
}
	
dtStatus FastLZCompressor::decompress(const unsigned char* compressed, const int compressedSize,
							unsigned char* buffer, const int maxBufferSize, int* bufferSize)
{
	*bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
	return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
}

LinearAllocator::LinearAllocator(const size_t cap) : buffer(0), capacity(0), top(0), high(0)
{
	resize(cap);
}
	
LinearAllocator::~LinearAllocator()
{
	dtFree(buffer);
}

void LinearAllocator::resize(const size_t cap)
{
	if (buffer) dtFree(buffer);
	buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
	capacity = cap;
}
	
void LinearAllocator::reset()
{
	high = dtMax(high, top);
	top = 0;
}
	
void* LinearAllocator::alloc(const size_t size)
{
	if (!buffer)
		return 0;
	if (top+size > capacity)
		return 0;
	unsigned char* mem = &buffer[top];
	top += size;
	return mem;
}

#pragma push_macro("free")
#undef free

void LinearAllocator::free(void* /*ptr*/)

#pragma pop_macro("free")

{
	// Empty
}


MeshProcess::MeshProcess( NavMeshType_t type )
{
	meshFlags = (RecastOfffMeshConnSF_t)(SF_OFFMESHCONN_TYPE_FLAGS_START << type);

	// One time parse all
	parseAll();
}

static void TestAgentRadius( NavMeshType_t type, float &fAgentRadius )
{
	float meshAgentRadius = NAI_Hull::Radius2D( type );
	if( meshAgentRadius > fAgentRadius ) 
	{
		fAgentRadius = meshAgentRadius;
	}
}

void MeshProcess::parseAll()
{
	offMeshConnVerts.Purge();
	offMeshConnRad.Purge();
	offMeshConnDir.Purge();
	offMeshConnArea.Purge();
	offMeshConnFlags.Purge();
	offMeshConnId.Purge();

#ifndef CLIENT_DLL
	// Pass in off-mesh connections.
	// TODO: Currently this is always server side, but we don't really need them client side atm.

	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "recast_offmesh_connection" );
	while( pEnt )
	{
		COffMeshConnection *pOffMeshConn = dynamic_cast< COffMeshConnection * >( pEnt );
		if( pOffMeshConn )
		{
			parseConnection( pOffMeshConn );
		}

		// Find next off mesh connection
		pEnt = gEntList.FindEntityByClassname( pEnt, "recast_offmesh_connection" );
	}
	//DevMsg("Parsed %d offmesh connections\n", offMeshConnId.Count());
#endif // CLIENT_DLL
}

#ifndef CLIENT_DLL
void MeshProcess::parseConnection( COffMeshConnection *pOffMeshConn )
{
	// Only parse connections for which we have the flag set
	RecastOfffMeshConnSF_t spawnFlags = pOffMeshConn->GetSpawnFlags();
	if( (spawnFlags & meshFlags) != SF_OFFMESHCONN_NONE ) 
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, pOffMeshConn->m_target );
		if( pTarget )
		{
			int vertOffset = offMeshConnVerts.Count();
			offMeshConnVerts.AddMultipleToTail( 6 );

			const Vector &vStart = pOffMeshConn->GetAbsOrigin();
			const Vector &vEnd = pTarget->GetAbsOrigin();

			offMeshConnVerts[vertOffset] = vStart.x;
			offMeshConnVerts[vertOffset+1] = vStart.z;
			offMeshConnVerts[vertOffset+2] = vStart.y;
			offMeshConnVerts[vertOffset+3] = vEnd.x;
			offMeshConnVerts[vertOffset+4] = vEnd.z;
			offMeshConnVerts[vertOffset+5] = vEnd.y;

			float rad = 0.0f;
			for(int i = 0; i < RECAST_NAVMESH_NUM; ++i) {
				if( (spawnFlags & (RecastOfffMeshConnSF_t)(SF_OFFMESHCONN_TYPE_FLAGS_START << i)) != SF_OFFMESHCONN_NONE )
					TestAgentRadius( i, rad );
			}

			offMeshConnRad.AddToTail( rad );

			offMeshConnId.AddToTail( 10000 + offMeshConnId.Count() );

			if( spawnFlags & SF_OFFMESHCONN_JUMPEDGE )
			{
				offMeshConnArea.AddToTail( POLYAREA_JUMP );
				offMeshConnFlags.AddToTail( POLYFLAGS_JUMP );
				offMeshConnDir.AddToTail( 0 ); // one direction
			}
			else
			{
				offMeshConnArea.AddToTail( POLYAREA_GROUND );
				offMeshConnFlags.AddToTail( POLYFLAGS_WALK );
				offMeshConnDir.AddToTail( DT_OFFMESH_CON_BIDIR ); // bi direction
			}

			//Msg("Parsed offmesh connection with rad %f, start %f %f %f, end %f %f %f\n", rad,
			//	vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z);
		}
		else
		{
			Warning("Offmesh connection at (%f %f %f) has no valid target (%s)\n", 
				pOffMeshConn->GetAbsOrigin().x, pOffMeshConn->GetAbsOrigin().y, pOffMeshConn->GetAbsOrigin().z,
				pOffMeshConn->m_target != NULL_STRING ? STRING( pOffMeshConn->m_target ) : "<null>");
		}
	}
}
#endif // CLIENT_DLL

void MeshProcess::process(struct dtNavMeshCreateParams* params,
						unsigned char* polyAreas, unsigned short* polyFlags)
{
	// Update poly flags from areas.
	for (int i = 0; i < params->polyCount; ++i)
	{
		if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
			polyAreas[i] = POLYAREA_GROUND;

		if (polyAreas[i] == POLYAREA_GROUND ||
			polyAreas[i] == POLYAREA_GRASS ||
			polyAreas[i] == POLYAREA_ROAD)
		{
			polyFlags[i] = POLYFLAGS_WALK;
		}
		else if (polyAreas[i] == POLYAREA_WATER)
		{
			polyFlags[i] = POLYFLAGS_SWIM;
		}
		else if (polyAreas[i] == POLYAREA_DOOR)
		{
			polyFlags[i] = POLYFLAGS_WALK | POLYFLAGS_DOOR;
		}
		else if (polyAreas[i] >= POLYAREA_OBSTACLE_START && polyAreas[i] <= POLYAREA_OBSTACLE_END )
		{
			polyFlags[i] = (POLYFLAGS_OBSTACLE_START << (polyAreas[i] - POLYAREA_OBSTACLE_START));
		}
	}

	params->offMeshConVerts = offMeshConnVerts.Base();
	params->offMeshConRad = offMeshConnRad.Base();
	params->offMeshConDir =  offMeshConnDir.Base();
	params->offMeshConAreas = offMeshConnArea.Base();
	params->offMeshConFlags = offMeshConnFlags.Base();
	params->offMeshConUserID = offMeshConnId.Base();
	params->offMeshConCount = offMeshConnId.Count();
}