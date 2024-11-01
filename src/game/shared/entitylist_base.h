//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYLIST_BASE_H
#define ENTITYLIST_BASE_H
#pragma once


#include "const.h"
#include "basehandle.h"
#include "utllinkedlist.h"
#include "ihandleentity.h"
#include "ehandle.h"
#include "string_t.h"
#include "tier1/utlhash.h"

COMPILE_TIME_ASSERT(GAME_NUM_ENT_ENTRIES > ENGINE_NUM_ENT_ENTRIES);

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

class CEntInfo
{
public:
	CSharedBaseEntity *m_pBaseEnt;
	int				m_SerialNumber;
	CEntInfo		*m_pPrev;
	CEntInfo		*m_pNext;

	void			ClearLinks();
};

// Derive a class from this if you want to filter entity list searches
abstract_class IEntityFindFilter
{
public:
	virtual bool ShouldFindEntity( CSharedBaseEntity *pEntity ) = 0;
	virtual CSharedBaseEntity *GetFilterResult( void ) = 0;
};

// Returns false every time. Created for some sick hack involving FindEntityProcedural looking at FindNamedEntity.
class CNullEntityFilter : public IEntityFindFilter
{
public:
	virtual bool ShouldFindEntity( CSharedBaseEntity *pEntity ) { return false; }
	virtual CSharedBaseEntity *GetFilterResult( void ) { return NULL; }
};

//-----------------------------------------------------------------------------
// Entity hash tables
//-----------------------------------------------------------------------------

struct EntsByStringList_t
{
	string_t iszStr;
	CSharedBaseEntity *pHead;
};

class CEntsByStringHashFuncs
{
public:
	CEntsByStringHashFuncs( int ) {}

	bool operator()( const EntsByStringList_t &lhs, const EntsByStringList_t &rhs ) const
	{
		return lhs.iszStr == rhs.iszStr;
	}

	unsigned int operator()( const EntsByStringList_t &item ) const
	{
		COMPILE_TIME_ASSERT( sizeof(char *) == sizeof(int) );
		return HashInt( (int)item.iszStr.ToCStr() );
	}
};

typedef CUtlHash<EntsByStringList_t	, CEntsByStringHashFuncs, CEntsByStringHashFuncs > CEntsByStringTable;

extern CEntsByStringTable g_EntsByClassname;

class CBaseEntityList
{
public:
	CBaseEntityList();
	~CBaseEntityList();
	
	// Add and remove entities. iForcedSerialNum should only be used on the client. The server
	// gets to dictate what the networkable serial numbers are on the client so it can send
	// ehandles over and they work.
	EHANDLE AddNetworkableEntity( CSharedBaseEntity *pEnt, int index, int iForcedSerialNum = -1 );
	EHANDLE AddNonNetworkableEntity( CSharedBaseEntity *pEnt );
	void RemoveEntity( CBaseHandle handle );

	// Get an ehandle from a networkable entity's index (note: if there is no entity in that slot,
	// then the ehandle will be invalid and produce NULL).
	EHANDLE GetNetworkableHandle( int iEntity ) const;

	// ehandles use this in their Get() function to produce a pointer to the entity.
	CSharedBaseEntity* LookupEntity( const CBaseHandle &handle ) const;
	CSharedBaseEntity* LookupEntityByNetworkIndex( int edictIndex ) const;

	// Use these to iterate over all the entities.
	EHANDLE FirstHandle() const;
	EHANDLE NextHandle( CBaseHandle hEnt ) const;
	static EHANDLE InvalidHandle();

	const CEntInfo *FirstEntInfo() const;
	const CEntInfo *NextEntInfo( const CEntInfo *pInfo ) const;
	const CEntInfo *GetEntInfoPtr( const CBaseHandle &hEnt ) const;
	const CEntInfo *GetEntInfoPtrByIndex( int index ) const;

	// Used by Foundry when an entity is respawned/edited.
	// We force the new entity's ehandle to be the same so anyone pointing at it still gets a valid CBaseEntity out of their ehandle.
	void ForceEntSerialNumber( int iEntIndex, int iSerialNumber );

// Overridables.
protected:

	// These are notifications to the derived class. It can cache info here if it wants.
	virtual void OnAddEntity( CSharedBaseEntity *pEnt, EHANDLE handle ) = 0;
	
	// It is safe to delete the entity here. We won't be accessing the pointer after
	// calling OnRemoveEntity.
	virtual void OnRemoveEntity( CSharedBaseEntity *pEnt, EHANDLE handle ) = 0;

public:
	virtual CSharedBaseEntity *FindEntityByClassname( CSharedBaseEntity *pStartEntity, const char *szName, IEntityFindFilter *pFilter = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityByName( CSharedBaseEntity *pStartEntity, const char *szName, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityByName( CSharedBaseEntity *pStartEntity, string_t iszName, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL )
	{
		return FindEntityByName( pStartEntity, STRING(iszName), pSearchingEntity, pActivator, pCaller, pFilter );
	}
	virtual CSharedBaseEntity *FindEntityInSphere( CSharedBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius ) = 0;
	virtual CSharedBaseEntity *FindEntityByTarget( CSharedBaseEntity *pStartEntity, const char *szName ) = 0;
	virtual CSharedBaseEntity *FindEntityByModel( CSharedBaseEntity *pStartEntity, const char *szModelName ) = 0;
	virtual CSharedBaseEntity	*FindEntityByOutputTarget( CSharedBaseEntity *pStartEntity, string_t iTarget ) = 0;

	virtual CSharedBaseEntity *FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityByNameWithin( CSharedBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius ) = 0;
	virtual CSharedBaseEntity *FindEntityByClassnameNearest2D( const char *szName, const Vector &vecSrc, float flRadius ) = 0;
	virtual CSharedBaseEntity *FindEntityByClassnameWithin( CSharedBaseEntity *pStartEntity , const char *szName, const Vector &vecSrc, float flRadius ) = 0;
	virtual CSharedBaseEntity *FindEntityByClassnameWithin( CSharedBaseEntity *pStartEntity , const char *szName, const Vector &vecMins, const Vector &vecMaxs ) = 0;

	virtual CSharedBaseEntity *FindEntityGeneric( CSharedBaseEntity *pStartEntity, const char *szName, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityGenericWithin( CSharedBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL ) = 0;
	virtual CSharedBaseEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL ) = 0;
	
	virtual CSharedBaseEntity *FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold) = 0;
	virtual CSharedBaseEntity *FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, const char *classname) = 0;

	virtual CSharedBaseEntity *FindEntityProcedural( const char *szName, CSharedBaseEntity *pSearchingEntity = NULL, CSharedBaseEntity *pActivator = NULL, CSharedBaseEntity *pCaller = NULL ) = 0;

	// Fast versions that require a (real) string_t, and won't do wildcarding
	virtual CSharedBaseEntity *FindEntityByClassnameFast( CSharedBaseEntity *pStartEntity, string_t iszClassname ) = 0;
	virtual CSharedBaseEntity *FindEntityByClassnameNearestFast( string_t iszClassname, const Vector &vecSrc, float flRadius ) = 0;
	virtual CSharedBaseEntity *FindEntityByNameFast( CSharedBaseEntity *pStartEntity, string_t iszName ) = 0;

	// call this before and after each frame to delete all of the marked entities.
	virtual void CleanupDeleteList( void ) = 0;

	// returns the next entity after pCurrentEnt;  if pCurrentEnt is NULL, return the first entity
	virtual CSharedBaseEntity *NextEnt( CSharedBaseEntity *pCurrentEnt ) = 0;
	virtual CSharedBaseEntity *FirstEnt() = 0;

private:

	EHANDLE AddEntityAtSlot( CSharedBaseEntity *pEnt, int iSlot, int iForcedSerialNum );
	void RemoveEntityAtSlot( int iSlot );

	
private:
	
	class CEntInfoList
	{
	public:
		CEntInfoList();

		const CEntInfo	*Head() const { return m_pHead; }
		const CEntInfo	*Tail() const { return m_pTail; }
		CEntInfo		*Head() { return m_pHead; }
		CEntInfo		*Tail() { return m_pTail; }
		void			AddToHead( CEntInfo *pElement ) { LinkAfter( NULL, pElement ); }
		void			AddToTail( CEntInfo *pElement ) { LinkBefore( NULL, pElement ); }

		void LinkBefore( CEntInfo *pBefore, CEntInfo *pElement );
		void LinkAfter( CEntInfo *pBefore, CEntInfo *pElement );
		void Unlink( CEntInfo *pElement );
		bool IsInList( CEntInfo *pElement );
	
	private:
		CEntInfo		*m_pHead;
		CEntInfo		*m_pTail;
	};

	int GetEntInfoIndex( const CEntInfo *pEntInfo ) const;


	// The first MAX_EDICTS entities are networkable. The rest are client-only or server-only.
	CEntInfo m_EntPtrArray[GAME_NUM_ENT_ENTRIES];
	CEntInfoList	m_activeList;
	CEntInfoList	m_freeNonNetworkableList;
};


// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline int CBaseEntityList::GetEntInfoIndex( const CEntInfo *pEntInfo ) const
{
	Assert( pEntInfo );
	int index = (int)(pEntInfo - m_EntPtrArray);
	Assert( index >= 0 && index < GAME_NUM_ENT_ENTRIES );
	return index;
}

inline EHANDLE CBaseEntityList::GetNetworkableHandle( int iEntity ) const
{
	Assert( iEntity >= 0 && iEntity < MAX_EDICTS );
	if ( m_EntPtrArray[iEntity].m_pBaseEnt != NULL )
		return EHANDLE( iEntity, m_EntPtrArray[iEntity].m_SerialNumber );
	else
		return NULL_EHANDLE;
}


inline CSharedBaseEntity* CBaseEntityList::LookupEntity( const CBaseHandle &handle ) const
{
	if ( handle.m_Index == INVALID_EHANDLE_INDEX )
		return NULL;

	const CEntInfo *pInfo = &m_EntPtrArray[ handle.GetEntryIndex() ];
	if ( pInfo->m_SerialNumber == handle.GetSerialNumber() )
		return pInfo->m_pBaseEnt;
	else
		return NULL;
}


inline CSharedBaseEntity* CBaseEntityList::LookupEntityByNetworkIndex( int edictIndex ) const
{
	// (Legacy support).
	if ( edictIndex < 0 )
		return NULL;

	Assert( edictIndex < GAME_NUM_ENT_ENTRIES );
	return m_EntPtrArray[edictIndex].m_pBaseEnt;
}

inline EHANDLE CBaseEntityList::FirstHandle() const
{
	if ( !m_activeList.Head() )
		return NULL_EHANDLE;

	int index = GetEntInfoIndex( m_activeList.Head() );
	return EHANDLE( index, m_EntPtrArray[index].m_SerialNumber );
}

inline EHANDLE CBaseEntityList::NextHandle( CBaseHandle hEnt ) const
{
	int iSlot = hEnt.GetEntryIndex();
	CEntInfo *pNext = m_EntPtrArray[iSlot].m_pNext;
	if ( !pNext )
		return NULL_EHANDLE;

	int index = GetEntInfoIndex( pNext );

	return EHANDLE( index, m_EntPtrArray[index].m_SerialNumber );
}
	
inline EHANDLE CBaseEntityList::InvalidHandle()
{
	return NULL_EHANDLE;
}

inline const CEntInfo *CBaseEntityList::FirstEntInfo() const
{
	return m_activeList.Head();
}

inline const CEntInfo *CBaseEntityList::NextEntInfo( const CEntInfo *pInfo ) const
{
	return pInfo->m_pNext;
}

inline const CEntInfo *CBaseEntityList::GetEntInfoPtr( const CBaseHandle &hEnt ) const
{
	int iSlot = hEnt.GetEntryIndex();
	return &m_EntPtrArray[iSlot];
}

inline const CEntInfo *CBaseEntityList::GetEntInfoPtrByIndex( int index ) const
{
	return &m_EntPtrArray[index];
}

inline void CBaseEntityList::ForceEntSerialNumber( int iEntIndex, int iSerialNumber )
{
	m_EntPtrArray[iEntIndex].m_SerialNumber = iSerialNumber;
}

extern CBaseEntityList *g_pEntityList;

#endif // ENTITYLIST_BASE_H
