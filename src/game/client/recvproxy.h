//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#ifndef RECVPROXY_H
#define RECVPROXY_H

#pragma once


#include "dt_recv.h"

class CRecvProxyData;


// This converts the int stored in pData to an EHANDLE in pOut.
void RecvProxy_IntToEHandle( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_IntToMoveParent( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_ShortSubOne( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_InterpolationAmountChanged( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntToModelIndex16_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_IntToModelIndex32_BackCompatible( const CRecvProxyData *pData, void *pStruct, void *pOut );

RecvProp RecvPropTime(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );

RecvProp RecvPropPredictableId(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );

RecvProp RecvPropEHandle(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,
	RecvVarProxyFn proxyFn=RecvProxy_IntToEHandle );

RecvProp RecvPropBool(
	const char *pVarName, 
	int offset, 
	int sizeofVar );

RecvProp RecvPropIntWithMinusOneFlag(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,
	RecvVarProxyFn proxyFn=RecvProxy_IntSubOne );

// Send a string_t as a string property.
RecvProp RecvPropStringT(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );

#endif // RECVPROXY_H

