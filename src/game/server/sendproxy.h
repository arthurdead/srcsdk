//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#ifndef SENDPROXY_H
#define SENDPROXY_H

#pragma once

#include "dt_send.h"


class DVariant;

void SendProxy_EHandle( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_ModelIndex( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

inline SendPropEx SendPropModelIndex( const char *pVarName, int offset, int sizeofVar=SIZEOF_IGNORE, DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY )
{
	return SendPropInt( pVarName, offset, sizeofVar, 32, SPROP_NONE, SendProxy_ModelIndex, priority );
}

SendPropEx SendPropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	DTFlags_t flags = SPROP_NONE,
	SendVarProxyFn proxyFn=SendProxy_EHandle,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY );

SendPropEx SendPropTime(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY );

SendPropEx SendPropPredictableId(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY	);


// Send a string_t as a string property.
SendPropEx SendPropStringT( const char *pVarName, int offset, int sizeofVar=SIZEOF_IGNORE, DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY );

//-----------------------------------------------------------------------------
// Purpose: Proxy that only sends data to team members
//-----------------------------------------------------------------------------
void* SendProxy_OnlyToTeam( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );

extern void* SendProxy_SendMinimalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
extern void* SendProxy_SendFullDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );

#endif // SENDPROXY_H
