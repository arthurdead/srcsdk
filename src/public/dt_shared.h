//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_SHARED_H
#define DATATABLE_SHARED_H

#pragma once

#include "dt_common.h"

// ------------------------------------------------------------------------ //
// Client version
// ------------------------------------------------------------------------ //

#if defined (CLIENT_DLL)

#include "dt_recv.h"

#define PROPINFO(varName)							RECVINFO(varName)						
#define PROPINFO_DT(varName)						RECVINFO_DT(varName)					
#define PROPINFO_DT_NAME(varName, remoteVarName)	RECVINFO_DTNAME(varName,remoteVarName)	
#define PROPINFO_NAME(varName,remoteVarName)		RECVINFO_NAME(varName, remoteVarName)	

#define DataTableProxy_StaticDataTable DataTableRecvProxy_StaticDataTable

#define DataTableProxyFn DataTableRecvVarProxyFn

#define REFERENCE_DATATABLE(tableName)	REFERENCE_RECV_TABLE(tableName)

#define DataTableProp	RecvProp
#define DataTable	RecvTable

#define DEFINE_NETWORK_FIELD DEFINE_RECV_FIELD

#endif

// ------------------------------------------------------------------------ //
// Server version
// ------------------------------------------------------------------------ //

#if !defined (CLIENT_DLL)

#include "dt_send.h"

#define PROPINFO(varName)							SENDINFO(varName)			
#define PROPINFO_DT(varName)						SENDINFO_DT(varName)		
#define PROPINFO_DT_NAME(varName, remoteVarName)	SENDINFO_DT_NAME(varName, remoteVarName)
#define PROPINFO_NAME(varName,remoteVarName)		SENDINFO_NAME(varName,remoteVarName)

#define DataTableProxy_StaticDataTable SendProxy_DataTableToDataTable

#define DataTableProxyFn SendTableProxyFn

#define REFERENCE_DATATABLE(tableName)	REFERENCE_SEND_TABLE(tableName)

#define DataTableProp	SendPropInfo
#define DataTable	SendTableInfo

#define DEFINE_NETWORK_FIELD DEFINE_SEND_FIELD

#endif

// Use these functions to setup your data tables.
DataTableProp PropFloat(
	const char *pVarName,					// Variable name.
	int offset,						// Offset into container structure.
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits to use when encoding.
	DTFlags_t flags=SPROP_NONE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	);

DataTableProp PropVector(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	DTFlags_t flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	);

DataTableProp PropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	DTFlags_t flags=SPROP_NONE
	);

DataTableProp PropInt(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	int nBits=-1,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	DTFlags_t flags=SPROP_NONE
	);

DataTableProp PropBool(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE	// Handled by SENDINFO macro.
	);

DataTableProp PropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	DTFlags_t flags=SPROP_NONE
	);

DataTableProp PropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE );

DataTableProp PropDataTable(
	const char *pVarName,
	int offset,
	DTFlags_t flags,
	DataTable *pTable,
	DataTableProxyFn varProxy=DataTableProxy_StaticDataTable
	);

#endif // DATATABLE_SHARED_H

