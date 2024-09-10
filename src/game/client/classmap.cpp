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

class classentry_t
{
public:
	classentry_t()
	{
		mapname[ 0 ] = 0;
		factory = 0;
		size = -1;
	}

	char const *GetMapName() const
	{
		return mapname;
	}

	void SetMapName( char const *newname )
	{
		Q_strncpy( mapname, newname, sizeof( mapname ) );
	}

	DISPATCHFUNCTION	factory;
	int					size;
private:
	char				mapname[ 40 ];
};

class CClassMap : public IClassMap
{
public:
	virtual void			Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory /*= 0*/ );
	virtual const char		*ClassnameToMapName( const char *classname );
	virtual const char		*MapNameToClassname( const char *mapname );
	virtual C_BaseEntity	*CreateEntity( const char *mapname );
	virtual int				GetClassSize( const char *classname );

	CUtlDict< classentry_t, unsigned short > m_ClassDict;
};

INIT_PRIORITY(101) static CClassMap g_Classmap;
IClassMap& GetClassMap( void )
{
	return g_Classmap;
}

void CClassMap::Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory = 0 )
{
	const char *map = ClassnameToMapName( classname );
	if ( map && !Q_strcasecmp( mapname, map ) )
		return;

	if ( map )
	{
		int index = m_ClassDict.Find( classname );
		Assert( index != m_ClassDict.InvalidIndex() );
		m_ClassDict.RemoveAt( index );
	}

	classentry_t element;
	element.SetMapName( mapname );
	element.factory = factory;
	element.size = size;
	m_ClassDict.Insert( classname, element );
}

const char *CClassMap::ClassnameToMapName( const char *classname )
{
	unsigned short index;
	static classentry_t lookup; 

	index = m_ClassDict.Find( classname );
	if ( index == m_ClassDict.InvalidIndex() )
		return NULL;

	lookup = m_ClassDict.Element( index );
	return lookup.GetMapName();
}

const char *CClassMap::MapNameToClassname( const char *mapname )
{
	int c = m_ClassDict.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		classentry_t *lookup = &m_ClassDict[ i ];
		if ( !lookup )
			continue;

		if ( Q_stricmp( lookup->GetMapName(), mapname ) )
			continue;

		return m_ClassDict.GetElementName( i );
	}

	return NULL;
}

C_BaseEntity *CClassMap::CreateEntity( const char *mapname )
{
	int c = m_ClassDict.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		classentry_t *lookup = &m_ClassDict[ i ];
		if ( !lookup )
			continue;

		if ( Q_stricmp( lookup->GetMapName(), mapname ) )
			continue;

		if ( !lookup->factory )
		{
#if defined( _DEBUG )
			Msg( "No factory for %s/%s\n", lookup->GetMapName(), m_ClassDict.GetElementName( i ) );
#endif
			continue;
		}

		return ( *lookup->factory )( mapname );
	}

	return NULL;
}

int CClassMap::GetClassSize( const char *classname )
{
	int c = m_ClassDict.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		classentry_t *lookup = &m_ClassDict[ i ];
		if ( !lookup )
			continue;

		if ( Q_strcmp( lookup->GetMapName(), classname ) )
			continue;

		return lookup->size;
	}

	return -1;
}

void DumpEntityFactories_f()
{
	CClassMap &classMap = (CClassMap &)GetClassMap();
	for ( int i = classMap.m_ClassDict.First(); i != classMap.m_ClassDict.InvalidIndex(); i = classMap.m_ClassDict.Next( i ) )
	{
		Msg( "%s\n", classMap.m_ClassDict.Element( i ).GetMapName() );
	}
}

static ConCommand dumpentityfactories( "dumpcliententityfactories", DumpEntityFactories_f, "Lists all entity factory names.", FCVAR_CLIENTDLL );
