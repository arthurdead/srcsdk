//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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
#include "tier1/utlbuffer.h"
#include "tier1/convar.h"

DECLARE_LOGGING_CHANNEL( LOG_RECAST );

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
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

/**
 * A place is a named group of navigation areas
 */
typedef unsigned int Place;
#define UNDEFINED_PLACE 0				// ie: "no place"
#define ANY_PLACE 0xFFFF

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
class PlaceDirectory
{
public:
	typedef unsigned short IndexType;	// Loaded/Saved as UnsignedShort.  Change this and you'll have to version.

	PlaceDirectory( void );
	void Reset( void );
	bool IsKnown( Place place ) const;						/// return true if this place is already in the directory
	IndexType GetIndex( Place place ) const;				/// return the directory index corresponding to this Place (0 = no entry)
	void AddPlace( Place place );							/// add the place to the directory if not already known
	Place IndexToPlace( IndexType entry ) const;			/// given an index, return the Place
	void Save( CUtlBuffer &fileBuffer );					/// store the directory
	void Load( CUtlBuffer &fileBuffer, int version );		/// load the directory
	const CUtlVector< Place > *GetPlaces( void ) const
	{
		return &m_directory;
	}

	bool HasUnnamedPlaces( void ) const 
	{
		return m_hasUnnamedAreas;
	}

private:
	CUtlVector< Place > m_directory;
	bool m_hasUnnamedAreas;
};

extern PlaceDirectory placeDirectory;

class CRecastMgr : public IRecastMgr
{
public:
	CRecastMgr();
	~CRecastMgr();

	virtual void Init();

	virtual void Update( float dt );

	// Load methods
	virtual bool InitMeshes();
	virtual bool Load();
	virtual void Reset();
	
	// Accessors for Units
	bool HasMeshes();
	CRecastMesh *GetMesh( NavMeshType_t type );
	CRecastMesh *FindBestMeshForRadiusHeight( float radius, float height );
	CRecastMesh *FindBestMeshForEntity( CSharedBaseEntity *pEntity );
	bool IsMeshLoaded( NavMeshType_t type );

	NavMeshType_t FindBestMeshTypeForRadiusHeight( float radius, float height );
	NavMeshType_t FindBestMeshTypeForEntity( CSharedBaseEntity *pEntity );

	// Used for debugging purposes on client. Don't use for anything else!
	virtual dtNavMesh* GetNavMesh( NavMeshType_t type );
	virtual dtNavMeshQuery* GetNavMeshQuery( NavMeshType_t type );
	virtual IMapMesh* GetMapMesh( MapMeshType_t type );
	
#ifndef CLIENT_DLL
	// Generation methods
	virtual bool LoadMapMesh( MapMeshType_t type, bool bLog = true, bool bDynamicOnly = false, 
		const Vector &vMinBounds = vec3_origin, const Vector &vMaxBounds = vec3_origin );
	virtual bool Build( bool loadDefaultMeshes = true );
	virtual bool Save();

	virtual bool IsMeshBuildDisabled( NavMeshType_t type );

	// Rebuilds mesh partial. Clears and rebuilds tiles touching the bounds.
	virtual bool RebuildPartial( const Vector &vMins, const Vector& vMaxs );
	virtual void UpdateRebuildPartial();

	// threaded mesh building
	static void ThreadedBuildMesh( CRecastMesh *&pMesh );
	static void ThreadedRebuildPartialMesh( CRecastMesh *&pMesh );
#endif // CLIENT_DLL

	// Obstacle management
	virtual bool AddEntRadiusObstacle( CSharedBaseEntity *pEntity, float radius, float height );
	virtual bool AddEntBoxObstacle( CSharedBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height );
	virtual bool RemoveEntObstacles( CSharedBaseEntity *pEntity );

	// Debug
#ifdef CLIENT_DLL
	void DebugRender();
#endif // CLIENT_DLL

	void DebugListMeshes();

	const char *PlaceToName( Place place ) const;						// given a place, return its name
	Place NameToPlace( const char *name ) const;						// given a place name, return a place ID or zero if no place is defined
	Place PartialNameToPlace( const char *name ) const;					// given the first part of a place name, return a place ID or zero if no place is defined, or the partial match is ambiguous
	void PrintAllPlaces( void ) const;									// output a list of names to the console
	int PlaceNameAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );	// Given a partial place name, fill in possible place names for ConCommand autocomplete

	//-------------------------------------------------------------------------------------
	// Edit mode
	//
	unsigned int GetNavPlace( void ) const			{ return m_navPlace; }
	void SetNavPlace( unsigned int place )			{ m_navPlace = place; }

private:
	CRecastMesh *GetMeshByIndex( int index );
	int FindMeshIndex( NavMeshType_t type );

#ifndef CLIENT_DLL
	const char *GetFilename( void ) const;
	virtual bool BuildMesh( CMapMesh *pMapMesh, NavMeshType_t type );
#endif // CLIENT_DLL

	NavObstacleArray_t &FindOrCreateObstacle( CSharedBaseEntity *pEntity );
	unsigned char DetermineAreaID( CSharedBaseEntity *pEntity, const Vector &mins, const Vector &maxs );

private:
	bool m_bLoaded;

#ifndef CLIENT_DLL
	// Map mesh used for generation. May not be set.
	CMapMesh *m_pMapMeshes[ RECAST_MAPMESH_NUM ];
	// Pending partial updates
	struct PartialMeshUpdate_t
	{
		Vector vMins;
		Vector vMaxs;
	};
	CUtlVector< PartialMeshUpdate_t > m_pendingPartialMeshUpdates;
#endif // CLIENT_DLL

	CRecastMesh *m_Meshes[RECAST_NAVMESH_NUM];

	CUtlMap< EHANDLE, NavObstacleArray_t > m_Obstacles;

	unsigned int m_navPlace;									// current navigation place for editing

	//----------------------------------------------------------------------------------
	// Place directory
	//
	char **m_placeName;											// master directory of place names (ie: "places")
	unsigned int m_placeCount;									// number of "places" defined in placeName[]
	void LoadPlaceDatabase( void );								// load the place names from a file
};

CRecastMgr &RecastMgr();

inline bool CRecastMgr::HasMeshes()
{
	return m_bLoaded;
}

inline CRecastMesh *CRecastMgr::GetMesh( NavMeshType_t type )
{
	if(type == RECAST_NAVMESH_INVALID)
		return NULL;
	int idx = FindMeshIndex( type );
	if( idx != -1 )
		return GetMeshByIndex( idx );
	return NULL;
}

#endif // RECAST_MGR_H