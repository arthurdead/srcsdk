//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MGR_H
#define RECAST_MGR_H

#pragma once

#include "tier1/utldict.h"
#include "tier1/utlmap.h"
#include "DetourTileCache.h"
#include "recast_imgr.h"
#include "ehandle.h"

#ifdef GAME_DLL
class CBaseEntity;
#else
#define CBaseEntity C_BaseEntity
class C_BaseEntity;
#endif

class CRecastMesh;
class CMapMesh;

typedef struct NavObstacle_t
{
	int meshIndex;
	dtObstacleRef ref;
} NavObstacleRef_t;

typedef struct NavObstacleArray_t
{
	NavObstacleArray_t() {}
	NavObstacleArray_t( const NavObstacleArray_t &ref )
	{
		obs =  ref.obs;
		areaId = ref.areaId;
	}

	CUtlVector< NavObstacle_t > obs;
	unsigned char areaId;
} NavObstacleArray_t;

#define NAV_OBSTACLE_INVALID_INDEX -1

class CRecastMgr : public IRecastMgr
{
public:
	CRecastMgr();
	~CRecastMgr();

	virtual void Init();

	virtual void Update( float dt );

	// Load methods
	virtual bool InitDefaultMeshes();
	virtual bool InsertMesh( const char *name, float agentRadius, float agentHeight, float agentMaxClimb, float agentMaxSlope );
	virtual bool Load();
	virtual void Reset();
	
	// Accessors for Units
	bool HasMeshes();
	CRecastMesh *GetMesh( const char *name );
	CRecastMesh *FindBestMeshForRadiusHeight( float radius, float height );
	CRecastMesh *FindBestMeshForEntity( CBaseEntity *pEntity );
	bool IsMeshLoaded( const char *name );

	const char *FindBestMeshNameForRadiusHeight( float radius, float height );
	const char *FindBestMeshNameForEntity( CBaseEntity *pEntity );

	// Used for debugging purposes on client. Don't use for anything else!
	virtual dtNavMesh* GetNavMesh( const char *meshName );
	virtual dtNavMeshQuery* GetNavMeshQuery( const char *meshName );
	virtual IMapMesh* GetMapMesh();
	
#ifndef CLIENT_DLL
	// Generation methods
	virtual bool LoadMapMesh( bool bLog = true, bool bDynamicOnly = false, 
		const Vector &vMinBounds = vec3_origin, const Vector &vMaxBounds = vec3_origin );
	virtual bool Build( bool loadDefaultMeshes = true );
	virtual bool Save();

	virtual bool IsMeshBuildDisabled( const char *meshName );

	// Rebuilds mesh partial. Clears and rebuilds tiles touching the bounds.
	virtual bool RebuildPartial( const Vector &vMins, const Vector& vMaxs );
	virtual void UpdateRebuildPartial();

	// threaded mesh building
	static void ThreadedBuildMesh( CRecastMesh *&pMesh );
	static void ThreadedRebuildPartialMesh( CRecastMesh *&pMesh );
#endif // CLIENT_DLL

	// Obstacle management
	virtual bool AddEntRadiusObstacle( CBaseEntity *pEntity, float radius, float height );
	virtual bool AddEntBoxObstacle( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height );
	virtual bool RemoveEntObstacles( CBaseEntity *pEntity );

	// Debug
#ifdef CLIENT_DLL
	void DebugRender();
#endif // CLIENT_DLL

	void DebugListMeshes();

private:
	CRecastMesh *GetMesh( int index );
	int FindMeshIndex( const char *name );

#ifndef CLIENT_DLL
	const char *GetFilename( void ) const;
	virtual bool BuildMesh( CMapMesh *m_pMapMesh, const char *name );
#endif // CLIENT_DLL

	NavObstacleArray_t &FindOrCreateObstacle( CBaseEntity *pEntity );
	unsigned char DetermineAreaID( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs );

private:
	bool m_bLoaded;

#ifndef CLIENT_DLL
	// Map mesh used for generation. May not be set.
	CMapMesh *m_pMapMesh;
	// Pending partial updates
	struct PartialMeshUpdate_t
	{
		Vector vMins;
		Vector vMaxs;
	};
	CUtlVector< PartialMeshUpdate_t > m_pendingPartialMeshUpdates;
#endif // CLIENT_DLL

	CUtlDict< CRecastMesh *, int > m_Meshes;

	CUtlMap< EHANDLE, NavObstacleArray_t > m_Obstacles;
};

CRecastMgr &RecastMgr();

inline bool CRecastMgr::HasMeshes()
{
	return m_bLoaded;
}

inline CRecastMesh *CRecastMgr::GetMesh( const char *name )
{
	int idx = FindMeshIndex( name );
	if( idx != -1 )
		return GetMesh( idx );
	return NULL;
}

#endif // RECAST_MGR_H