//====== Copyright � 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "imaterialproxydict.h"
#include "materialsystem/imaterialproxy.h"
#include "tier1/UtlStringMap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CMaterialProxyDict : public IMaterialProxyDict
{
public:
	IMaterialProxy *	CreateProxy( const char *proxyName );
	void				Add( const char *pMaterialProxyName, MaterialProxyFactory_t *pMaterialProxyFactory );
	bool ProxyExists( const char *proxyName );
public:
	CUtlStringMap<MaterialProxyFactory_t *> m_StringToProxyFactoryMap;
};

void CMaterialProxyDict::Add( const char *pMaterialProxyName, MaterialProxyFactory_t *pMaterialProxyFactory )
{
	Assert( pMaterialProxyName );
	m_StringToProxyFactoryMap[pMaterialProxyName] = pMaterialProxyFactory;
}

INIT_PRIORITY(101) static CMaterialProxyDict g_MaterialProxyDict;
IMaterialProxyDict &GetMaterialProxyDict()
{
	return g_MaterialProxyDict;
}

IMaterialProxy *CMaterialProxyDict::CreateProxy( const char *pMaterialProxyName )
{
	UtlSymId_t sym = m_StringToProxyFactoryMap.Find( pMaterialProxyName );
	if ( sym == m_StringToProxyFactoryMap.InvalidIndex() )
	{
		return NULL;
	}
	MaterialProxyFactory_t *pMaterialProxyFactory = m_StringToProxyFactoryMap[sym];
	Assert( pMaterialProxyFactory );
	return (*pMaterialProxyFactory)();
}

bool CMaterialProxyDict::ProxyExists( const char *pMaterialProxyName )
{
	UtlSymId_t sym = m_StringToProxyFactoryMap.Find( pMaterialProxyName );
	if ( sym == m_StringToProxyFactoryMap.InvalidIndex() )
	{
		return false;
	}
	return true;
}

void DumpProxies_f()
{
	for ( int i = 0; i < g_MaterialProxyDict.m_StringToProxyFactoryMap.GetNumStrings(); ++i )
	{
		Msg( "%s\n", g_MaterialProxyDict.m_StringToProxyFactoryMap.String(i) );
	}
}

static ConCommand dumpmaterialsproxies( "dumpmaterialsproxies", DumpProxies_f, "Lists all material proxies.", FCVAR_CLIENTDLL );

