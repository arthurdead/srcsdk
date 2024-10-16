//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "server_class.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ServerClass *g_pServerClassHead=NULL;

ServerClass::ServerClass( const char *pNetworkName, SendTable *pTable )
{
	m_pNetworkName = pNetworkName;
	m_pTable = pTable;
	m_InstanceBaselineIndex = INVALID_STRING_INDEX;
	// g_pServerClassHead is sorted alphabetically, so find the correct place to insert
	if ( !g_pServerClassHead )
	{
		g_pServerClassHead = this;
		m_pNext = NULL;
	}
	else
	{
		ServerClass *p1 = g_pServerClassHead;
		ServerClass *p2 = p1->m_pNext;

		// use _stricmp because Q_stricmp isn't hooked up properly yet
		if ( V_stricmp( p1->GetName(), pNetworkName ) > 0)
		{
			m_pNext = g_pServerClassHead;
			g_pServerClassHead = this;
			p1 = NULL;
		}

		while( p1 )
		{
			if ( p2 == NULL || V_stricmp( p2->GetName(), pNetworkName ) > 0)
			{
				m_pNext = p2;
				p1->m_pNext = this;
				break;
			}
			p1 = p2;
			p2 = p2->m_pNext;
		}	
	}
}