//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dt_shared.h"

#if !defined (CLIENT_DLL)
#include "sendproxy.h"
#else
#include "recvproxy.h"
#endif


// ------------------------------------------------------------------------ //
// Just wrappers to make shared code look easier...
// ------------------------------------------------------------------------ //

// Use these functions to setup your data tables.
DataTableProp PropFloat(
	const char *pVarName,					// Variable name.
	int offset,						// Offset into container structure.
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	)
{
#if !defined (CLIENT_DLL)
	return SendPropFloat( pVarName, offset, sizeofVar, nBits, flags, fLowValue, fHighValue );
#else
	return RecvPropFloat( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropVector(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits (for each floating-point component) to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	)
{
#if !defined (CLIENT_DLL)
	return SendPropVector( pVarName, offset, sizeofVar, nBits, flags, fLowValue, fHighValue );
#else
	return RecvPropVector( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags
	)
{
#if !defined (CLIENT_DLL)
	return SendPropAngle( pVarName, offset, sizeofVar, nBits, flags );
#else
	return RecvPropFloat( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropInt(
	const char *pVarName,
	int offset,
	int sizeofVar,	// Handled by SENDINFO macro.
	int nBits,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	int flags
	)
{
#if !defined (CLIENT_DLL)
	return SendPropInt( pVarName, offset, sizeofVar, nBits, flags );
#else
	return RecvPropInt( pVarName, offset, sizeofVar, flags );
#endif
}

DataTableProp PropBool(
	const char *pVarName,
	int offset,
	int sizeofVar	// Handled by SENDINFO macro.
	)
{
#if !defined (CLIENT_DLL)
	return SendPropBool( pVarName, offset, sizeofVar );
#else
	return RecvPropBool( pVarName, offset, sizeofVar );
#endif
}

DataTableProp PropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	int flags
	)
{
#if !defined (CLIENT_DLL)
	return SendPropString( pVarName, offset, bufferLen, flags );
#else
	return RecvPropString( pVarName, offset, bufferLen, flags );
#endif
}

DataTableProp PropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
#if !defined (CLIENT_DLL)
	return SendPropEHandle( pVarName, offset, sizeofVar );
#else
	return RecvPropEHandle( pVarName, offset, sizeofVar );
#endif
}

DataTableProp PropDataTable(
	const char *pVarName,
	int offset,
	int flags,
	DataTable *pTable,
	DataTableProxyFn varProxy
	)
{
#if !defined (CLIENT_DLL)
	return SendPropDataTable( pVarName, offset, pTable, varProxy );
#else
	return RecvPropDataTable( pVarName, offset, flags, pTable, varProxy );
#endif
}
