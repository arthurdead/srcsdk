//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_TILECACHE_HELPER_H
#define RECAST_TILECACHE_HELPER_H

#pragma once

#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "tier1/utlvector.h"
#include "recast_imgr.h"

enum RecastOfffMeshConnSF_t : unsigned short;

class CMapMesh;

struct FastLZCompressor : public dtTileCacheCompressor
{
	virtual int maxCompressedSize(const int bufferSize);
	
	virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
							  unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize);
	
	virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
								unsigned char* buffer, const int maxBufferSize, int* bufferSize);
};

struct LinearAllocator : public dtTileCacheAlloc
{
	unsigned char* buffer;
	size_t capacity;
	size_t top;
	size_t high;
	
	LinearAllocator(const size_t cap);
	
	~LinearAllocator();

	void resize(const size_t cap);
	
	virtual void reset();
	
	virtual void* alloc(const size_t size);

	#pragma push_macro("free")
	#undef free

	virtual void free(void* /*ptr*/);

	#pragma pop_macro("free")
};

class COffMeshConnection;

struct MeshProcess : public dtTileCacheMeshProcess
{
	MeshProcess( NavMeshType_t type );

	virtual void process(struct dtNavMeshCreateParams* params,
						 unsigned char* polyAreas, unsigned short* polyFlags);

private:
	void parseAll();

#ifndef CLIENT_DLL
	void parseConnection( COffMeshConnection *pOffMeshConn );
#endif // CLIENT_DLL

private:
	RecastOfffMeshConnSF_t meshFlags;

	CUtlVector< float > offMeshConnVerts;
	CUtlVector< float > offMeshConnRad;
	CUtlVector< unsigned char > offMeshConnDir;
	CUtlVector< unsigned char > offMeshConnArea;
	CUtlVector< unsigned short > offMeshConnFlags;
	CUtlVector< unsigned int > offMeshConnId;
};

#endif // RECAST_TILECACHE_HELPER_H
