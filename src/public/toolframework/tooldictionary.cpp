//========= Copyright Valve Corporation, All rights reserved. ============//
#include "toolframework/tooldictionary.h"
#include "toolframework/itooldictionary.h"
#include "utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int	CToolDictionary::GetToolCount() const
{
	return m_Tools.Count();
}

IToolSystem	*CToolDictionary::GetTool( int index )
{
	if ( index < 0 || index >= m_Tools.Count() )
	{
		return NULL;
	}
	return m_Tools[ index ];
}

void *CToolDictionary::QueryInterface( const char *pInterfaceName )
{
	return NULL;
}

void CToolDictionary::RegisterTool( IToolSystem *tool )
{
	m_Tools.AddToTail( tool );
}

CToolDictionary g_ToolDictionary;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( IToolDictionary, CToolDictionary, VTOOLDICTIONARY_INTERFACE_VERSION, g_ToolDictionary );

void RegisterTool( IToolSystem *tool )
{
	g_ToolDictionary.RegisterTool( tool );
}
