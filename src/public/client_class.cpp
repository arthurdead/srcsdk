//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "client_class.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ClientClass *g_pClientClassHead=NULL;

ClientClass::ClientClass( const char *pNetworkName, CreateClientClassFn createFn, RecvTable *pRecvTable )
{
	m_pNetworkName	= pNetworkName;
	m_pCreateFn		= createFn;
	m_pCreateEventFn= NULL;
	m_pRecvTable	= pRecvTable;
	
	// Link it in
	m_pNext				= g_pClientClassHead;
	g_pClientClassHead	= this;
}

ClientClass::ClientClass( const char *pNetworkName, CreateEventFn createEventFn, RecvTable *pRecvTable )
{
	m_pNetworkName	= pNetworkName;
	m_pCreateFn		= NULL;
	m_pCreateEventFn= createEventFn;
	m_pRecvTable	= pRecvTable;
	
	// Link it in
	m_pNext				= g_pClientClassHead;
	g_pClientClassHead	= this;
}

ClientClass::ClientClass( const char *pNetworkName, RecvTable *pRecvTable )
{
	m_pNetworkName	= pNetworkName;
	m_pCreateFn		= NULL;
	m_pCreateEventFn= NULL;
	m_pRecvTable	= pRecvTable;
	
	// Link it in
	m_pNext				= g_pClientClassHead;
	g_pClientClassHead	= this;
}
