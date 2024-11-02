//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MAPENTITIES_SHARED_H
#define MAPENTITIES_SHARED_H
#pragma once

#include "tier0/platform.h"
#include "tier0/logging.h"
#include "tier1/utlvector.h"
#include "tier1/utllinkedlist.h"

DECLARE_LOGGING_CHANNEL( LOG_MAPPARSE );

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
class CPointTemplate;
typedef CPointTemplate CSharedPointTemplate;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
class C_PointTemplate;
typedef C_PointTemplate CSharedPointTemplate;
#endif

// This class provides hooks into the map-entity loading process that allows CS to do some tricks
// when restarting the round. The main trick it tries to do is recreate all 
abstract_class IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname ) = 0;
	virtual CSharedBaseEntity* CreateNextEntity( const char *pClassname ) = 0;
};

// -------------------------------------------------------------------------------------------- //
// Entity list management stuff.
// -------------------------------------------------------------------------------------------- //
// These are created for map entities in order as the map entities are spawned.
class CMapEntityRef
{
public:
	int		m_iEdict;			// Which edict slot this entity got. -1 if CreateEntityByName failed.
	int		m_iSerialNumber;	// The edict serial number. TODO used anywhere ?
};

extern CUtlLinkedList<CMapEntityRef, unsigned short> g_MapEntityRefs;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMapLoadEntityFilter : public IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity( const char *pClassname ) OVERRIDE;

	virtual CSharedBaseEntity* CreateNextEntity( const char *pClassname ) OVERRIDE;
};

// Use the filter so you can prevent certain entities from being created out of the map.
// CSPort does this when restarting rounds. It wants to reload most entities from the map, but certain
// entities like the world entity need to be left intact.
void MapEntity_ParseAllEntities( const char *pMapData, IMapEntityFilter *pFilter=NULL, bool bActivateEntities=false );

const char *MapEntity_ParseEntity( CSharedBaseEntity *&pEntity, bool &bCreated, const char *pEntData, IMapEntityFilter *pFilter );
void MapEntity_PrecacheEntity( const char *pEntData, int &nStringSize );


//-----------------------------------------------------------------------------
// Hierarchical spawn 
//-----------------------------------------------------------------------------
struct HierarchicalSpawn_t
{
	CSharedBaseEntity *m_pEntity;
	int			m_nDepth;
	CSharedBaseEntity	*m_pDeferredParent;			// attachment parents can't be set until the parents are spawned
	const char	*m_pDeferredParentAttachment; // so defer setting them up until the second pass
	bool m_bSpawn;
	bool m_bActivate;
};

struct HierarchicalSpawnMapData_t
{
	const char	*m_pMapData;
	int			m_iMapDataLength;
};

// Shared by mapentities.cpp and Foundry for spawning entities.
class CMapEntitySpawner
{
public:
	CMapEntitySpawner();
	~CMapEntitySpawner();
	
	void AddEntity( CSharedBaseEntity *pEntity, bool bCreated, const char *pMapData, int iMapDataLength );
	void HandleTemplates();
	void SpawnAndActivate( bool bActivateEntities );
	void PurgeRemovedEntities();

public:
	bool m_bFoundryMode;

private:

	HierarchicalSpawnMapData_t *m_pSpawnMapData;
	HierarchicalSpawn_t *m_pSpawnList;
	CUtlVector< CSharedPointTemplate* > m_PointTemplates;
	int m_nEntities;
};

void SpawnHierarchicalList( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities );
void MapEntity_ParseAllEntites_SpawnTemplates( CSharedPointTemplate **pTemplates, int iTemplateCount, CSharedBaseEntity **pSpawnedEntities, HierarchicalSpawnMapData_t *pSpawnMapData, int iSpawnedEntityCount );


#define MAPKEY_MAXLENGTH	2048

//-----------------------------------------------------------------------------
// Purpose: encapsulates the data string in the map file 
//			that is used to initialise entities.  The data
//			string contains a set of key/value pairs.
//-----------------------------------------------------------------------------
class CEntityMapData
{
private:
	char	*m_pEntData;
	int		m_nEntDataSize;
	char	*m_pCurrentKey;

public:
	explicit CEntityMapData( char *entBlock, int nEntBlockSize = -1 ) : 
		m_pEntData(entBlock), m_nEntDataSize(nEntBlockSize), m_pCurrentKey(entBlock) {}

	// find the keyName in the entdata and puts it's value into Value.  returns false if key is not found
	bool ExtractValue( const char *keyName, char *Value );

	// find the nth keyName in the endata and change its value to specified one
	// where n == nKeyInstance
	bool SetValue( const char *keyName, char *NewValue, int nKeyInstance = 0 );
	
	bool GetFirstKey( char *keyName, char *Value );
	bool GetNextKey( char *keyName, char *Value );

	const char *CurrentBufferPosition( void );
};

const char *MapEntity_ParseToken( const char *data, char *newToken );
const char *MapEntity_SkipToNextEntity( const char *pMapData, char *pWorkBuffer );
bool MapEntity_ExtractValue( const char *pEntData, const char *keyName, char Value[MAPKEY_MAXLENGTH] );


#endif // MAPENTITIES_SHARED_H
