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
void RecvProxy_EHandle( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_MoveParent( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_InterpolationAmountChanged( const CRecvProxyData *pData, void *pStruct, void *pOut );

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
	RecvVarProxyFn proxyFn=RecvProxy_EHandle );

// Send a string_t as a string property.
RecvProp RecvPropStringT(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE );

#endif // RECVPROXY_H

