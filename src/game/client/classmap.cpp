//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "iclassmap.h"
#include "utldict.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_ENTITYFACTORY, "EntityFactory Client" );

class CClassMap : public IClassMap
{
public:
	CClassMap();

	virtual void			Add( const char *mapname, IEntityFactory *factory );
	virtual C_BaseEntity	*CreateEntity( const char *mapname );
	virtual int				GetClassSize( const char *classname );
	virtual IEntityFactory *FindFactory( const char *pClassName );

	CUtlDict< IEntityFactory *, unsigned short > m_ClassDict;
};

INIT_PRIORITY(101) static CClassMap g_Classmap;
IClassMap& GetClassMap( void )
{
	return g_Classmap;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CClassMap::CClassMap() : m_ClassDict( true, 0, 128 )
{
}

void CClassMap::Add( const char *mapname, IEntityFactory *factory )
{
	Assert( FindFactory( mapname ) == NULL );
	m_ClassDict.Insert( mapname, factory );
}

C_BaseEntity *CClassMap::CreateEntity( const char *mapname )
{
	IEntityFactory *pFactory = FindFactory( mapname );
	if ( !pFactory )
	{
		Log_Error(LOG_ENTITYFACTORY,"Attempted to create unknown entity type %s!\n", mapname );
		return NULL;
	}
#if defined(TRACK_ENTITY_MEMORY) && defined(USE_MEM_DEBUG)
	MEM_ALLOC_CREDIT_( m_ClassDict.GetElementName( m_ClassDict.Find( mapname ) ) );
#endif
	return pFactory->Create( mapname );
}

//-----------------------------------------------------------------------------
// Finds a new factory
//-----------------------------------------------------------------------------
IEntityFactory *CClassMap::FindFactory( const char *pClassName )
{
	unsigned short nIndex = m_ClassDict.Find( pClassName );
	if ( nIndex == m_ClassDict.InvalidIndex() )
		return NULL;
	return m_ClassDict[nIndex];
}

int CClassMap::GetClassSize( const char *classname )
{
	IEntityFactory *pFactory = FindFactory( classname );
	if ( !pFactory )
	{
		return -1;
	}
	return pFactory->GetEntitySize();
}

void DumpEntityFactories_f()
{
	CClassMap &classMap = (CClassMap &)GetClassMap();
	for ( int i = classMap.m_ClassDict.First(); i != classMap.m_ClassDict.InvalidIndex(); i = classMap.m_ClassDict.Next( i ) )
	{
		Log_Msg( LOG_ENTITYFACTORY,"%s - %s\n", classMap.m_ClassDict.GetElementName( i ), classMap.m_ClassDict.Element( i )->DllClassname() );
	}
}

static ConCommand dumpentityfactories( "dumpentityfactories_client", DumpEntityFactories_f, "Lists all entity factory names.", FCVAR_CLIENTDLL );
