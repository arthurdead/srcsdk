//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game.  All iteration through
//			entities is done through this object.
//-----------------------------------------------------------------------------
#include "cbase.h"
#include "cliententitylist.h"
#include "tier1/fmtstr.h"
#include "collisionutils.h"
#include "entityoutput.h"
#include "c_ai_basenpc.h"
#include "c_world.h"
#include "recast/recast_mgr_ent.h"
#include "c_playerresource.h"
#include "c_effects.h"
#include "collisionproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// Create interface
static CClientEntityList s_EntityList;
CBaseEntityList *g_pEntityList = &s_EntityList;
IClientEntityList *entitylist = &s_EntityList;
IClientEntityListEx *entitylist_ex = &s_EntityList;

// Expose list to engine
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientEntityList, IClientEntityList, VCLIENTENTITYLIST_INTERFACE_VERSION, s_EntityList );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientEntityList, IClientEntityListEx, VCLIENTENTITYLIST_EX_INTERFACE_VERSION, s_EntityList );

// Store local pointer to interface for rest of client .dll only 
//  (CClientEntityList instead of IClientEntityList )
CClientEntityList *cl_entitylist = &s_EntityList; 

bool PVSNotifierMap_LessFunc( C_BaseEntity* const &a, C_BaseEntity* const &b )
{
	return a < b;
}

//-----------------------------------------------------------------------------
// Convenience methods to convert between entindex + ClientEntityHandle_t
//-----------------------------------------------------------------------------
ClientEntityHandle_t CClientEntityList::EntIndexToHandle( int entnum )
{
	if ( entnum < -1 )
		return NULL_EHANDLE;
	C_BaseEntity *pUnk = GetListedEntity( entnum );
	return pUnk ? pUnk->GetRefEHandle() : NULL_EHANDLE; 
}

								 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::CClientEntityList( void ) : 
	m_PVSNotifierMap( 0, 0, PVSNotifierMap_LessFunc )
{
	m_iMaxUsedServerIndex = -1;
	m_iMaxServerEnts = 0;
	Release();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClientEntityList::~CClientEntityList( void )
{
	Release();
}

//-----------------------------------------------------------------------------
// Purpose: Clears all entity lists and releases entities
//-----------------------------------------------------------------------------
void CClientEntityList::Release( void )
{
	// Free all the entities.
	ClientEntityHandle_t iter = FirstHandle();
	while( iter != InvalidHandle() )
	{
		C_BaseEntity *pEnt = GetBaseEntityFromHandle( iter );
		if( pEnt )
		{
			UTIL_RemoveImmediate( pEnt );
		}

		RemoveEntity( iter );
		iter = FirstHandle();
	}

#ifdef _DEBUG
	for ( UtlHashHandle_t handle = g_EntsByClassname.GetFirstHandle(); g_EntsByClassname.IsValidHandle(handle);	handle = g_EntsByClassname.GetNextHandle(handle) )
	{
		EntsByStringList_t &element = g_EntsByClassname[handle];
		Assert( element.pHead == NULL );
	}
#endif

	g_EntsByClassname.RemoveAll();

	m_iNumServerEnts = 0;
	m_iMaxServerEnts = 0;
	m_iNumClientNonNetworkable = 0;
	m_iMaxUsedServerIndex = -1;
}

#if defined( STAGING_ONLY )

// Defined in tier1 / interface.cpp for Windows and native for POSIX platforms.
extern "C" int backtrace( void **buffer, int size );

extern "C" char ** backtrace_symbols (void *const *buffer, int size);

static struct
{
	int entnum;
	float time;
	C_BaseEntity *pBaseEntity;
	void *backtrace_addrs[ 32 ];
	char **backtrace_syms;
} g_RemoveEntityBacktraces[ 1024 ];
static uint32 g_RemoveEntityBacktracesIndex = 0;

static void OnRemoveEntityBacktraceHook( int entnum, C_BaseEntity *pBaseEntity )
{
	int index = g_RemoveEntityBacktracesIndex++;
	if ( g_RemoveEntityBacktracesIndex >= ARRAYSIZE( g_RemoveEntityBacktraces ) )
		g_RemoveEntityBacktracesIndex = 0;

	g_RemoveEntityBacktraces[ index ].entnum = entnum;
	g_RemoveEntityBacktraces[ index ].time = gpGlobals->curtime;
	g_RemoveEntityBacktraces[ index ].pBaseEntity = pBaseEntity;

	memset( g_RemoveEntityBacktraces[ index ].backtrace_addrs, 0, sizeof( g_RemoveEntityBacktraces[ index ].backtrace_addrs ) );
	backtrace( g_RemoveEntityBacktraces[ index ].backtrace_addrs, ARRAYSIZE( g_RemoveEntityBacktraces[ index ].backtrace_addrs ) );
#ifdef POSIX
	if( g_RemoveEntityBacktraces[ index ].backtrace_syms )
		free( g_RemoveEntityBacktraces[ index ].backtrace_syms );
	g_RemoveEntityBacktraces[ index ].backtrace_syms = backtrace_symbols( g_RemoveEntityBacktraces[ index ].backtrace_addrs, ARRAYSIZE( g_RemoveEntityBacktraces[ index ].backtrace_addrs ) );
#endif
}

// Should help us track down CL_PreserveExistingEntity Host_Error() issues:
//  1. Set cl_removeentity_backtrace_capture to 1.
//  2. When error hits, run "cl_removeentity_backtrace_dump [entnum]".
//  3. In debugger, track down what functions the spewed addresses refer to.
static ConVar cl_removeentity_backtrace_capture( "cl_removeentity_backtrace_capture",
#ifdef _DEBUG
	"1", 
#else
	"0", 
#endif
	0,
	"For debugging. Capture backtraces for CClientEntityList::OnRemoveEntity calls." );

void do_removeentity_backtrace_dump(int entnum, bool do_trap)
{
	for ( int i = 0; i < ARRAYSIZE( g_RemoveEntityBacktraces ); i++ )
	{
		if ( g_RemoveEntityBacktraces[ i ].time && 
			( entnum == -1 || g_RemoveEntityBacktraces[ i ].entnum == entnum ) )
		{
			Msg( "%d: time:%.2f pBaseEntity:%p\n", g_RemoveEntityBacktraces[i].entnum,
				g_RemoveEntityBacktraces[ i ].time, g_RemoveEntityBacktraces[ i ].pBaseEntity );
			for ( int j = 0; j < ARRAYSIZE( g_RemoveEntityBacktraces[ i ].backtrace_addrs ); j++ )
			{
			#ifdef POSIX
				if(g_RemoveEntityBacktraces[ i ].backtrace_syms)
					Msg( "  %p - %s\n", g_RemoveEntityBacktraces[ i ].backtrace_addrs[ j ], g_RemoveEntityBacktraces[ i ].backtrace_syms[ j ] ? g_RemoveEntityBacktraces[ i ].backtrace_syms[ j ] : "NULL" );
				else
			#endif
				Msg( "  %p\n", g_RemoveEntityBacktraces[ i ].backtrace_addrs[ j ] );
			}

			if(do_trap) {
				DebuggerBreak();
			}

			if(entnum != -1) {
				break;
			}
		}
	}
}

CON_COMMAND( cl_removeentity_backtrace_dump, "Dump backtraces for client OnRemoveEntity calls." )
{
	if ( !cl_removeentity_backtrace_capture.GetBool() )
	{
		Msg( "cl_removeentity_backtrace_dump error: cl_removeentity_backtrace_capture not enabled. Backtraces not captured.\n" );
		return;
	}

	int entnum = ( args.ArgC() >= 2 ) ? atoi( args[ 1 ] ) : -1;

	do_removeentity_backtrace_dump( entnum, false );
}

#endif // STAGING_ONLY

IClientNetworkable* CClientEntityList::GetClientNetworkable( int entnum )
{
	Assert( entnum >= 0 );
	Assert( entnum < MAX_EDICTS );

	IClientNetworkable *pNetworkable = m_EntityCacheInfo[entnum].m_pNetworkable;

#if defined( STAGING_ONLY ) && 0
	if(!pNetworkable) {
		do_removeentity_backtrace_dump(entnum, true);
	}
#endif // STAGING_ONLY

	return pNetworkable;
}

EntityCacheInfo_t *CClientEntityList::GetClientNetworkableArray()
{
	return m_EntityCacheInfo;
}

void CClientEntityList::SetDormant( int entityIndex, bool bDormant )
{
	Assert( entityIndex >= 0 );
	Assert( entityIndex < MAX_EDICTS );
	m_EntityCacheInfo[entityIndex].m_bDormant = bDormant;
}


int CClientEntityList::NumberOfEntities( bool bIncludeNonNetworkable )
{
	if ( bIncludeNonNetworkable == true )
		 return m_iNumServerEnts + m_iNumClientNonNetworkable;

	return m_iNumServerEnts;
}


void CClientEntityList::SetMaxEntities( int maxents )
{
	m_iMaxServerEnts = maxents;
}


int CClientEntityList::GetMaxEntities( void )
{
	return m_iMaxServerEnts;
}


//-----------------------------------------------------------------------------
// Convenience methods to convert between entindex + ClientEntityHandle_t
//-----------------------------------------------------------------------------
int CClientEntityList::HandleToEntIndex( ClientEntityHandle_t handle )
{
	if ( handle == NULL )
		return -1;
	C_BaseEntity *pEnt = GetBaseEntityFromHandle( handle );
	return pEnt ? pEnt->entindex() : -1; 
}


//-----------------------------------------------------------------------------
// Purpose: Because m_iNumServerEnts != last index
// Output : int
//-----------------------------------------------------------------------------
int CClientEntityList::GetHighestEntityIndex( void )
{
	return m_iMaxUsedServerIndex;
}

void CClientEntityList::RecomputeHighestEntityUsed( void )
{
	m_iMaxUsedServerIndex = -1;

	// Walk backward looking for first valid index
	int i;
	for ( i = MAX_EDICTS - 1; i >= 0; i-- )
	{
		if ( GetListedEntity( i ) != NULL )
		{
			m_iMaxUsedServerIndex = i;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Add a raw C_BaseEntity to the entity list.
// Input  : index - 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------

C_BaseEntity* CClientEntityList::GetBaseEntity( int entnum )
{
	return LookupEntityByNetworkIndex( entnum );
}

C_BaseEntity* CClientEntityList::GetListedEntity( int entnum )
{
	return LookupEntityByNetworkIndex( entnum );
}

ICollideable* CClientEntityList::GetCollideable( int entnum )
{
	C_BaseEntity *pEnt = LookupEntityByNetworkIndex( entnum );
	return pEnt ? pEnt->GetCollideable() : NULL;
}

IClientUnknown* CClientEntityList::GetClientUnknownFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}

IClientUnknown* CClientEntityList::GetClientUnknownFromHandle( CBaseHandle hEnt )
{
	return LookupEntity( hEnt );
}

IClientEntity* CClientEntityList::GetClientEntity( int entnum )
{
	return LookupEntityByNetworkIndex( entnum );
}

IClientNetworkable* CClientEntityList::GetClientNetworkableFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}

IClientNetworkable* CClientEntityList::GetClientNetworkableFromHandle( CBaseHandle hEnt )
{
	return LookupEntity( hEnt );
}

IClientEntity* CClientEntityList::GetClientEntityFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}

IClientEntity* CClientEntityList::GetClientEntityFromHandle( CBaseHandle hEnt )
{
	return LookupEntity( hEnt );
}

C_BaseEntity* CClientEntityList::GetClientRenderableFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}


C_BaseEntity* CClientEntityList::GetBaseEntityFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}


ICollideable* CClientEntityList::GetCollideableFromHandle( ClientEntityHandle_t hEnt )
{
	C_BaseEntity *pEnt = LookupEntity( hEnt );
	return pEnt ? pEnt->GetCollideable() : NULL;
}


C_BaseEntity* CClientEntityList::GetClientThinkableFromHandle( ClientEntityHandle_t hEnt )
{
	return LookupEntity( hEnt );
}


void CClientEntityList::AddPVSNotifier( C_BaseEntity *pUnknown )
{
	IPVSNotify *pNotify = pUnknown->GetPVSNotifyInterface();
	if ( pNotify )
	{
		unsigned short index = m_PVSNotifyInfos.AddToTail();
		CPVSNotifyInfo *pInfo = &m_PVSNotifyInfos[index];
		pInfo->m_pNotify = pNotify;
		pInfo->m_pRenderable = pUnknown;
		pInfo->m_InPVSStatus = 0;
		pInfo->m_PVSNotifiersLink = index;

		m_PVSNotifierMap.Insert( pUnknown, index );
	}
}


void CClientEntityList::RemovePVSNotifier( C_BaseEntity *pUnknown )
{
	IPVSNotify *pNotify = pUnknown->GetPVSNotifyInterface();
	if ( pNotify )
	{
		unsigned short index = m_PVSNotifierMap.Find( pUnknown );
		if ( !m_PVSNotifierMap.IsValidIndex( index ) )
		{
			Warning( "PVS notifier not in m_PVSNotifierMap\n" );
			Assert( false );
			return;
		}

		unsigned short indexIntoPVSNotifyInfos = m_PVSNotifierMap[index];
		
		Assert( m_PVSNotifyInfos[indexIntoPVSNotifyInfos].m_pNotify == pNotify );
		Assert( m_PVSNotifyInfos[indexIntoPVSNotifyInfos].m_pRenderable == pUnknown );
		
		m_PVSNotifyInfos.Remove( indexIntoPVSNotifyInfos );
		m_PVSNotifierMap.RemoveAt( index );
		return;
	}

	// If it didn't report itself as a notifier, let's hope it's not in the notifier list now
	// (which would mean that it reported itself as a notifier earlier, but not now).
#ifdef _DEBUG
	unsigned short index = m_PVSNotifierMap.Find( pUnknown );
	Assert( !m_PVSNotifierMap.IsValidIndex( index ) );
#endif
}

void CClientEntityList::AddListenerEntity( IClientEntityListener *pListener )
{
	if ( m_entityListeners.Find( pListener ) >= 0 )
	{
		AssertMsg( 0, "Can't add listeners multiple times\n" );
		return;
	}
	m_entityListeners.AddToTail( pListener );
}

void CClientEntityList::RemoveListenerEntity( IClientEntityListener *pListener )
{
	m_entityListeners.FindAndRemove( pListener );
}

void CClientEntityList::OnAddEntity( C_BaseEntity *pEnt, EHANDLE handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// Update our counters.
		m_iNumServerEnts++;
		if ( entnum > m_iMaxUsedServerIndex )
		{
			m_iMaxUsedServerIndex = entnum;
		}


		// Cache its networkable pointer.
		Assert( dynamic_cast< IClientUnknown* >( pEnt ) );
		Assert( ((IClientUnknown*)pEnt)->GetClientNetworkable() ); // Server entities should all be networkable.
		pCache->m_pNetworkable = pEnt;
		pCache->m_bDormant = true;
	}

	// If this thing wants PVS notifications, hook it up.
	AddPVSNotifier( pEnt );

	// Store it in a special list for fast iteration if it's a C_BaseEntity.
	pCache->m_BaseEntitiesIndex = m_BaseEntities.AddToTail( pEnt );

	if ( entnum < 0 || entnum >= MAX_EDICTS )
	{
		 m_iNumClientNonNetworkable++;
	}

	// Make sure no one is trying to build an entity without game strings.
	Assert( MAKE_STRING( pEnt->GetClassname() ) == FindPooledString( pEnt->GetClassname() ) && 
		( pEnt->GetEntityNameAsTStr() == NULL_STRING || pEnt->GetEntityNameAsTStr() == FindPooledString( pEnt->GetEntityNameAsTStr().ToCStr() ) ) );

	Assert( pEnt->m_pPrevByClass == NULL && pEnt->m_pNextByClass == NULL && pEnt->m_ListByClass == g_EntsByClassname.InvalidHandle() );

	EntsByStringList_t dummyEntry = { pEnt->GetClassnameStr(), 0 };
	UtlHashHandle_t hEntry = g_EntsByClassname.Insert( dummyEntry );

	EntsByStringList_t *pEntry = &g_EntsByClassname[hEntry];
	pEnt->m_ListByClass = hEntry;
	if ( pEntry->pHead )
	{
		pEntry->pHead->m_pPrevByClass = pEnt;
		pEnt->m_pNextByClass = pEntry->pHead;
		Assert( pEnt->m_pPrevByClass == NULL );
	}
	pEntry->pHead = pEnt;

	//DevMsg(2,"Created %s\n", pBaseEnt->GetClassname() );
	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityCreated( pEnt );
	}
}

void CClientEntityList::OnRemoveEntity( C_BaseEntity *pEnt, EHANDLE handle )
{
	int entnum = handle.GetEntryIndex();
	EntityCacheInfo_t *pCache = &m_EntityCacheInfo[entnum];

	if ( entnum >= 0 && entnum < MAX_EDICTS )
	{
		// This is a networkable ent. Clear out our cache info for it.
		pCache->m_pNetworkable = NULL;
		m_iNumServerEnts--;

		if ( entnum >= m_iMaxUsedServerIndex )
		{
			RecomputeHighestEntityUsed();
		}
	}

	// If this is a PVS notifier, remove it.
	RemovePVSNotifier( pEnt );

#if defined( STAGING_ONLY )
	if ( cl_removeentity_backtrace_capture.GetBool() )
	{
		OnRemoveEntityBacktraceHook( entnum, pEnt );
	}
#endif // STAGING_ONLY

	if ( entnum < 0 || entnum >= MAX_EDICTS )
	{
		 m_iNumClientNonNetworkable--;
	}

	//DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
	for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
	{
		m_entityListeners[i]->OnEntityDeleted( pEnt );
	}

	if ( pCache->m_BaseEntitiesIndex != m_BaseEntities.InvalidIndex() )
		m_BaseEntities.Remove( pCache->m_BaseEntitiesIndex );

	pCache->m_BaseEntitiesIndex = m_BaseEntities.InvalidIndex();

	if ( pEnt->m_ListByClass != g_EntsByClassname.InvalidHandle() )
	{
		EntsByStringList_t *pEntry = &g_EntsByClassname[pEnt->m_ListByClass];
		if ( pEntry->pHead == pEnt )
		{
			pEntry->pHead = pEnt->m_pNextByClass;
			// Don't remove empty list, on the assumption that the number of classes that are not referenced again is small
			// Plus during map load we get a lot of precache others that hit this [8/8/2008 tom]
		}

		Assert( g_EntsByClassname[pEnt->m_ListByClass].pHead != pEnt );
		if ( pEnt->m_pNextByClass )
		{
			pEnt->m_pNextByClass->m_pPrevByClass = pEnt->m_pPrevByClass;
		}

		if ( pEnt->m_pPrevByClass )
		{
			pEnt->m_pPrevByClass->m_pNextByClass = pEnt->m_pNextByClass;
		}

		pEnt->m_pPrevByClass = pEnt->m_pNextByClass = NULL;
		pEnt->m_ListByClass = g_EntsByClassname.InvalidHandle();
	}
}


// Use this to iterate over all the C_BaseEntities.
C_BaseEntity* CClientEntityList::FirstBaseEntity() const
{
	const CEntInfo *pList = FirstEntInfo();
	while ( pList )
	{
		if ( pList->m_pBaseEnt )
		{
			return pList->m_pBaseEnt;
		}
		pList = pList->m_pNext;
	}

	return NULL;

}

C_BaseEntity* CClientEntityList::NextBaseEntity( C_BaseEntity *pEnt ) const
{
	if ( pEnt == NULL )
		return FirstBaseEntity();

	// Run through the list until we get a C_BaseEntity.
	const CEntInfo *pList = GetEntInfoPtr( pEnt->GetRefEHandle() );
	if ( pList )
	{
		pList = NextEntInfo(pList);
	}

	while ( pList )
	{
		if ( pList->m_pBaseEnt )
		{
			return pList->m_pBaseEnt;
		}
		pList = pList->m_pNext;
	}
	
	return NULL; 
}

// call this before and after each frame to delete all of the marked entities.
void CClientEntityList::CleanupDeleteList( void )
{
	bool lastallowed = C_BaseEntity::s_bImmediateRemovesAllowed;
	C_BaseEntity::s_bImmediateRemovesAllowed = true;
	C_BaseEntity::PurgeRemovedEntities();
	C_BaseEntity::s_bImmediateRemovesAllowed = lastallowed;
}

// -------------------------------------------------------------------------------------------------- //
// C_AllBaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_AllBaseEntityIterator::C_AllBaseEntityIterator()
{
	Restart();
}


void C_AllBaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

	
C_BaseEntity* C_AllBaseEntityIterator::Next()
{
	if ( m_CurBaseEntity == ClientEntityList().m_BaseEntities.InvalidIndex() )
		return NULL;

	C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );
	return pRet;
}


// -------------------------------------------------------------------------------------------------- //
// C_BaseEntityIterator
// -------------------------------------------------------------------------------------------------- //
C_BaseEntityIterator::C_BaseEntityIterator()
{
	Restart();
}

void C_BaseEntityIterator::Restart()
{
	m_CurBaseEntity = ClientEntityList().m_BaseEntities.Head();
}

C_BaseEntity* C_BaseEntityIterator::Next()
{
	// Skip dormant entities
	while ( m_CurBaseEntity != ClientEntityList().m_BaseEntities.InvalidIndex() )
	{
		C_BaseEntity *pRet = ClientEntityList().m_BaseEntities[m_CurBaseEntity];
		m_CurBaseEntity = ClientEntityList().m_BaseEntities.Next( m_CurBaseEntity );

		if (!pRet->IsDormant())
			return pRet;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given classname.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Classname to search for.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByClassname( C_BaseEntity *pStartEntity, const char *szName, IEntityFindFilter *pFilter )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *pEntity = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !pEntity )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( pEntity->ClassMatches(szName) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity(pEntity) )
				continue;

			return pEntity;
		}
	}

	return NULL;
}

C_BaseEntity *CClientEntityList::FindEntityByClassnameFast( C_BaseEntity *pStartEntity, string_t iszClassname )
{
	if ( pStartEntity )
	{
		return pStartEntity->m_pNextByClass;
	}

	EntsByStringList_t key = { iszClassname };
	UtlHashHandle_t hEntry = g_EntsByClassname.Find( key );
	if ( hEntry != g_EntsByClassname.InvalidHandle() )
	{
		return g_EntsByClassname[hEntry].pHead;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Finds an entity given a procedural name.
// Input  : szName - The procedural name to search for, should start with '!'.
//			pSearchingEntity - 
//			pActivator - The activator entity if this was called from an input
//				or Use handler.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityProcedural( const char *szName, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller )
{
	//
	// Check for the name escape character.
	//
	if ( szName[0] == '!' )
	{
		const char *pName = szName + 1;

		//
		// It is a procedural name, look for the ones we understand.
		//
		if ( FStrEq( pName, "player" ) )
		{
			return C_BasePlayer::GetLocalPlayer(); 
		}
		else if ( FStrEq( pName, "activator" ) )
		{
			return pActivator;
		}
		else if ( FStrEq( pName, "caller" ) )
		{
			return pCaller;
		}
		else if ( FStrEq( pName, "picker" ) )
		{
			// TODO: Player could be activator instead
			C_BasePlayer *pPlayer = ToBasePlayer(pSearchingEntity);
			return FindPickerEntity( pPlayer ? pPlayer : NULL ); 
		}
		else if ( FStrEq( pName, "self" ) )
		{
			return pSearchingEntity;
		}
		else if ( FStrEq( pName, "parent" ) )
		{
			return pSearchingEntity ? pSearchingEntity->GetParent() : NULL;
		}
		else if ( FStrEq( pName, "owner" ) )
		{
			return pSearchingEntity ? pSearchingEntity->GetOwnerEntity() : NULL;
		}
		else if (strchr(pName, ':'))
		{
			char name[128];
			Q_strncpy(name, pName, strchr(pName, ':')-pName+1);

			C_BaseEntity *pEntity = FindEntityProcedural(UTIL_VarArgs("!%s", name), pSearchingEntity, pActivator, pCaller);
			if (pEntity && pEntity->IsNPC())
			{
				const char *target = (Q_strstr(pName, ":") + 1);
				if (target[0] != '!')
					target = UTIL_VarArgs("!%s", target);

				return pEntity->MyNPCPointer()->FindNamedEntity(target);
			}
		}
		else if (pSearchingEntity && pSearchingEntity->IsCombatCharacter())
		{
			// Perhaps the entity itself has the answer?
			// This opens up new possibilities. The weird filter is there so it doesn't go through this twice.
			CNullEntityFilter pFilter;
			return pSearchingEntity->MyCombatCharacterPointer()->FindNamedEntity(szName, &pFilter);
		}
		else 
		{
			Warning( "Invalid entity search name %s\n", szName );
			Assert(0);
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given name.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Name to search for.
//			pActivator - Activator entity if this was called from an input
//				handler or Use handler.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByName( C_BaseEntity *pStartEntity, const char *szName, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	if ( !szName || szName[0] == 0 )
		return NULL;

	if ( szName[0] == '!' )
	{
		//
		// Avoid an infinite loop, only find one match per procedural search!
		//
		if (pStartEntity == NULL)
			return FindEntityProcedural( szName, pSearchingEntity, pActivator, pCaller );

		return NULL;
	}
	
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( ent->NameMatches( szName ) )
		{
			if ( pFilter && !pFilter->ShouldFindEntity(ent) )
				continue;

			return ent;
		}
	}

	return NULL;
}

C_BaseEntity *CClientEntityList::FindEntityByNameFast( C_BaseEntity *pStartEntity, string_t iszName )
{
	if ( iszName == NULL_STRING || STRING(iszName)[0] == 0 )
		return NULL;

	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( IDENT_STRINGS( ent->GetEntityNameAsTStr(), iszName ) )
		{
			return ent;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pStartEntity - 
//			szModelName - 
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByModel( C_BaseEntity *pStartEntity, const char *szModelName )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( ent->GetModelName() == NULL_STRING )
			continue;

		if ( FStrEq( STRING(ent->GetModelName()), szModelName ) )
			return ent;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given target.
// Input  : pStartEntity - 
//			szName - 
//-----------------------------------------------------------------------------
// FIXME: obsolete, remove
C_BaseEntity	*CClientEntityList::FindEntityByOutputTarget( C_BaseEntity *pStartEntity, string_t iTarget )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		datamap_t *dmap = ent->GetMapDataDesc();
		while ( dmap )
		{
			int fields = dmap->dataNumFields;
			for ( int i = 0; i < fields; i++ )
			{
				typedescription_t *dataDesc = &dmap->dataDesc[i];
				if ( ( dataDesc->fieldType == FIELD_CUSTOM ) && ( dataDesc->flags & FTYPEDESC_OUTPUT ) )
				{
					CBaseEntityOutput *pOutput = (CBaseEntityOutput *)((int)ent + (int)dataDesc->fieldOffset);
					if ( pOutput->GetActionForTarget( iTarget ) )
						return ent;
				}
			}

			dmap = dmap->baseMap;
		}
	}

	return NULL;
}
C_BaseEntity	*CClientEntityList::FindEntityByTarget( C_BaseEntity *pStartEntity, const char *szName )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		if ( ent->m_target == NULL_STRING )
			continue;

		if ( FStrEq( STRING(ent->m_target), szName ) )
			return ent;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Used to iterate all the entities within a sphere.
// Input  : pStartEntity - 
//			vecCenter - 
//			flRadius - 
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityInSphere( C_BaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( pStartEntity->GetRefEHandle() )->m_pNext : FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		Vector vecRelativeCenter;
		ent->CollisionProp()->WorldToCollisionSpace( vecCenter, &vecRelativeCenter );
		if ( !IsBoxIntersectingSphere( ent->CollisionProp()->OBBMins(),	ent->CollisionProp()->OBBMaxs(), vecRelativeCenter, flRadius ) )
			continue;

		return ent;
	}

	// nothing found
	return NULL; 
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by name within a radius
// Input  : szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller )
{
	C_BaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	C_BaseEntity *pSearch = NULL;
	while ((pSearch = FindEntityByName( pSearch, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
	{
		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByNameWithin( C_BaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller )
{
	//
	// Check for matching class names within the search radius.
	//
	C_BaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller );
	}

	while ((pEntity = FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
	{
		float flDist2 = (pEntity->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by class name withing given search radius.
// Input  : szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
{
	C_BaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	C_BaseEntity *pSearch = NULL;
	while ((pSearch = FindEntityByClassname( pSearch, szName )) != NULL)
	{
		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}

C_BaseEntity *CClientEntityList::FindEntityByClassnameNearestFast( string_t iszName, const Vector &vecSrc, float flRadius )
{
	C_BaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	C_BaseEntity *pSearch = NULL;
	while ((pSearch = FindEntityByClassnameFast( pSearch, iszName )) != NULL)
	{
		float flDist2 = (pSearch->GetAbsOrigin() - vecSrc).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by class name withing given search radius.
// Input  : szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByClassnameNearest2D( const char *szName, const Vector &vecSrc, float flRadius )
{
	C_BaseEntity *pEntity = NULL;

	//
	// Check for matching class names within the search radius.
	//
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}

	C_BaseEntity *pSearch = NULL;
	while ((pSearch = FindEntityByClassname( pSearch, szName )) != NULL)
	{
		float flDist2 = (pSearch->GetAbsOrigin().AsVector2D() - vecSrc.AsVector2D()).LengthSqr();

		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within radius distance by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByClassnameWithin( C_BaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
{
	//
	// Check for matching class names within the search radius.
	//
	C_BaseEntity *pEntity = pStartEntity;
	float flMaxDist2 = flRadius * flRadius;
	if (flMaxDist2 == 0)
	{
		return FindEntityByClassname( pEntity, szName );
	}

	while ((pEntity = FindEntityByClassname( pEntity, szName )) != NULL)
	{
		Vector vecRelativeCenter;
		pEntity->CollisionProp()->WorldToCollisionSpace( vecSrc, &vecRelativeCenter );
		if ( IsBoxIntersectingSphere( pEntity->CollisionProp()->OBBMins(),	pEntity->CollisionProp()->OBBMaxs(), vecRelativeCenter, flRadius ) )
		{
			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within an extent by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecMins - Search mins.
//			vecMaxs - Search maxs.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityByClassnameWithin( C_BaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs )
{
	//
	// Check for matching class names within the search radius.
	//
	C_BaseEntity *pEntity = pStartEntity;

	while ((pEntity = FindEntityByClassname( pEntity, szName )) != NULL)
	{
		// check if the aabb intersects the search aabb.
		Vector entMins, entMaxs;
		pEntity->CollisionProp()->WorldSpaceAABB( &entMins, &entMaxs );
		if ( IsBoxIntersectingBox( vecMins, vecMaxs, entMins, entMaxs ) )
		{
			return pEntity;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by target name or class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityGeneric( C_BaseEntity *pStartEntity, const char *szName, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	C_BaseEntity *pEntity = NULL;

	pEntity = FindEntityByName( pStartEntity, szName, pSearchingEntity, pActivator, pCaller, pFilter );
	if (!pEntity)
	{
		pEntity = FindEntityByClassname( pStartEntity, szName, pFilter );
	}

	return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by target name or class name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityGenericWithin( C_BaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller )
{
	C_BaseEntity *pEntity = NULL;

	pEntity = FindEntityByNameWithin( pStartEntity, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
	if (!pEntity)
	{
		pEntity = FindEntityByClassnameWithin( pStartEntity, szName, vecSrc, flRadius );
	}

	return pEntity;
} 

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by target name or class name within a radius.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, C_BaseEntity *pSearchingEntity, C_BaseEntity *pActivator, C_BaseEntity *pCaller )
{
	C_BaseEntity *pEntity = NULL;

	pEntity = FindEntityByNameNearest( szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
	if (!pEntity)
	{
		pEntity = FindEntityByClassnameNearest( szName, vecSrc, flRadius );
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn) with the
//			given classname.
// Input  : origin - 
//			facing - 
//			threshold - 
//			classname - 
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, const char *classname)
{
	float bestDot = threshold;
	C_BaseEntity *best_ent = NULL;

	const CEntInfo *pInfo = FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		// FIXME: why is this skipping pointsize entities?
		if (ent->IsPointSized() )
			continue;

		// Make vector to entity
		Vector	to_ent = (ent->GetAbsOrigin() - origin);

		VectorNormalize( to_ent );
		float dot = DotProduct (facing , to_ent );
		if (dot > bestDot) 
		{
			if (FClassnameIs(ent,classname))
			{
				bestDot	= dot;
				best_ent = ent;
			}
		}
	}
	return best_ent;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn)
// Input  : origin - 
//			facing - 
//			threshold - 
//-----------------------------------------------------------------------------
C_BaseEntity *CClientEntityList::FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold)
{
	float bestDot = threshold;
	C_BaseEntity *best_ent = NULL;

	const CEntInfo *pInfo = FirstEntInfo();

	for ( ;pInfo; pInfo = pInfo->m_pNext )
	{
		C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pBaseEnt;
		if ( !ent )
		{
			DevWarning( "NULL entity in global entity list!\n" );
			continue;
		}

		const char *classname = ent->GetClassname();

		// Make vector to entity
		Vector	to_ent = ent->WorldSpaceCenter() - origin;
		VectorNormalize(to_ent);

		float dot = DotProduct( facing, to_ent );
		if (dot <= bestDot) 
			continue;

		bestDot	= dot;
		best_ent = ent;
	}
	return best_ent;
}

static const char *s_StandardEnts[] = {
	"gamerules_proxy",
	"player_manager",
	"team_manager",
	"soundent",
	"worldspawn",
	"env_debughistory",
	"recast_mgr",
};

bool IsStandardEntityClassname(const char *pClassname)
{
	return FindInList(s_StandardEnts, pClassname, ARRAYSIZE(s_StandardEnts));
}

C_BaseEntity *GetSingletonOfClassname(const char *pClassname)
{
	if(V_strcmp(pClassname, "worldspawn") == 0) {
		return GetClientWorldEntity();
	} else if(V_strcmp(pClassname, "player_manager") == 0) {
		return g_PR;
	} else if(V_strcmp(pClassname, "gamerules_proxy") == 0) {
		return g_pGameRulesProxy;
	} else if(V_strcmp(pClassname, "team_manager") == 0) {
		return NULL;
	} else if(V_strcmp(pClassname, "recast_mgr") == 0) {
		return GetRecastMgrEnt();
	} else if(V_strcmp(pClassname, "client_snowfallmgr") == 0) {
		return s_pSnowFallMgr;
	} else {
		return NULL;
	}
}
