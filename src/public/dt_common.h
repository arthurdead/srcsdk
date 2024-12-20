//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_COMMON_H
#define DATATABLE_COMMON_H

#pragma once

#include "basetypes.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include <stddef.h>
#include "mathlib/vector.h"

#ifdef GNUC
#undef offsetof
#define offsetof(s,m)	__builtin_offsetof(s,m)
#endif

// Max number of properties in a datatable and its children.
#define MAX_DATATABLES		1024	// must be a power of 2.
#define MAX_DATATABLE_PROPS	4096

#define MAX_ARRAY_ELEMENTS	2048		// a network array should have more that 1024 elements

#define HIGH_DEFAULT		-121121.121121f

#define BITS_FULLRES	-1	// Use the full resolution of the type being encoded.
#define BITS_WORLDCOORD	-2	// Encode as a world coordinate.

#define DT_MAX_STRING_BITS			9
#define DT_MAX_STRING_BUFFERSIZE	(1<<DT_MAX_STRING_BITS)	// Maximum length of a string that can be sent.

#define STRINGBUFSIZE(className, varName)	sizeof( ((className*)0)->varName )

// Gets the size of a variable in a class.
#define PROPSIZEOF(className, varName)		sizeof(((className*)0)->varName)


// SendProp::m_Flags.
#define SPROP_UNSIGNED			(1<<0)	// Unsigned integer data.

#define SPROP_COORD				(1<<1)	// If this is set, the float/vector is treated like a world coordinate.
										// Note that the bit count is ignored in this case.

#define SPROP_NOSCALE			(1<<2)	// For floating point, don't scale into range, just take value as is.

#define SPROP_ROUNDDOWN			(1<<3)	// For floating point, limit high value to range minus one bit unit

#define SPROP_ROUNDUP			(1<<4)	// For floating point, limit low value to range minus one bit unit

#define SPROP_NORMAL			(1<<5)	// If this is set, the vector is treated like a normal (only valid for vectors)
							
#define SPROP_EXCLUDE			(1<<6)	// This is an exclude prop (not excludED, but it points at another prop to be excluded).

#define SPROP_XYZE				(1<<7)	// Use XYZ/Exponent encoding for vectors.

#define SPROP_INSIDEARRAY		(1<<8)	// This tells us that the property is inside an array, so it shouldn't be put into the
										// flattened property list. Its array will point at it when it needs to.

#define SPROP_PROXY_ALWAYS_YES	(1<<9)	// Set for datatable props using one of the default datatable proxies like
										// SendProxy_DataTableToDataTable that always send the data to all clients.

#define SPROP_CHANGES_OFTEN		(1<<10)	// this is an often changed field, moved to head of sendtable so it gets a small index

#define SPROP_IS_A_VECTOR_ELEM	(1<<11)	// Set automatically if SPROP_VECTORELEM is used.

#define SPROP_COLLAPSIBLE		(1<<12)	// Set automatically if it's a datatable with an offset of 0 that doesn't change the pointer
										// (ie: for all automatically-chained base classes).
										// In this case, it can get rid of this SendPropDataTable altogether and spare the
										// trouble of walking the hierarchy more than necessary.

#define SPROP_COORD_MP					(1<<13) // Like SPROP_COORD, but special handling for multiplayer games
#define SPROP_COORD_MP_LOWPRECISION 	(1<<14) // Like SPROP_COORD, but special handling for multiplayer games where the fractional component only gets a 3 bits instead of 5
#define SPROP_COORD_MP_INTEGRAL			(1<<15) // SPROP_COORD_MP, but coordinates are rounded to integral boundaries

#define SPROP_VARINT					SPROP_NORMAL	// reuse existing flag so we don't break demo. note you want to include SPROP_UNSIGNED if needed, its more efficient

//#define DT_CELL_COORD_SUPPORTED
#ifdef DT_CELL_COORD_SUPPORTED
#define SPROP_CELL_COORD				(1<<17) // Like SPROP_COORD, but special encoding for cell coordinates that can't be negative, bit count indicate maximum value
#define SPROP_CELL_COORD_LOWPRECISION 	(1<<18) // Like SPROP_CELL_COORD, but special handling where the fractional component only gets a 3 bits instead of 5
#define SPROP_CELL_COORD_INTEGRAL		(1<<19) // SPROP_CELL_COORD, but coordinates are rounded to integral boundaries
#endif

#define SPROP_NUMFLAGBITS_NETWORKED		16

// This is server side only, it's used to mark properties whose SendProxy_* functions encode against gpGlobals->tickcount (the only ones that currently do this are
//  m_flAnimTime and m_flSimulationTime.  MODs shouldn't need to mess with this probably
#define SPROP_ENCODED_AGAINST_TICKCOUNT	(1<<16)

// See SPROP_NUMFLAGBITS_NETWORKED for the ones which are networked
#define SPROP_NUMFLAGBITS				17

// Used by the SendProp and RecvProp functions to disable debug checks on type sizes.
#define SIZEOF_IGNORE		-1


// Use this to extern send and receive datatables, and reference them.
#define EXTERN_SEND_TABLE(tableName)	namespace tableName {extern SendTable g_SendTable;}
#define EXTERN_RECV_TABLE(tableName)	namespace tableName {extern RecvTable g_RecvTable;}

#define REFERENCE_SEND_TABLE(tableName)	tableName::g_SendTable
#define REFERENCE_RECV_TABLE(tableName)	tableName::g_RecvTable

#define DT_VARNAME(varName) \
	#varName

#define __DT_VARNAME_VECTORELEM_x "x"
#define __DT_VARNAME_VECTORELEM_y "y"
#define __DT_VARNAME_VECTORELEM_z "z"
#define __DT_VARNAME_VECTORELEM_0 "x"
#define __DT_VARNAME_VECTORELEM_1 "y"
#define __DT_VARNAME_VECTORELEM_2 "z"

//catch wrong use of macros
#ifdef _DEBUG
#define DT_VARNAME_VECTORELEM(varName, i) \
	#varName "~" __DT_VARNAME_VECTORELEM_##i

#define DT_VARNAME_ARRAYELEM(varName, i) \
	#varName "<" #i ">"

#define DT_VARNAME_STRUCTELEM(structVarName, varName) \
	#structVarName "~" #varName

#define DT_VARNAME_NESTEDSTRUCTELEM(structVarName, structVarName2, varName) \
	#structVarName "~" #structVarName2 "~" #varName

#define DT_VARNAME_STRUCTELEM_ARRAYELEM(structVarName, varName, i) \
	#structVarName "~" #varName "<" #i ">"
#else
#define DT_VARNAME_VECTORELEM(varName, i) \
	#varName "." __DT_VARNAME_VECTORELEM_##i

#define DT_VARNAME_ARRAYELEM(varName, i) \
	#varName "[" #i "]"

#define DT_VARNAME_STRUCTELEM(structVarName, varName) \
	#structVarName "." #varName

#define DT_VARNAME_NESTEDSTRUCTELEM(structVarName, structVarName2, varName) \
	#structVarName "." #structVarName2 "." #varName

#define DT_VARNAME_STRUCTELEM_ARRAYELEM(structVarName, varName, i) \
	#structVarName "." #varName "[" #i "]"
#endif

class SendProp;

// The day we do this, we break all mods until they recompile.
//#define DT_INT64_SUPPORTED
//#define DT_QUATERNION_SUPPORTED

typedef enum
{
	DPT_Int=0,
	DPT_Float,
	DPT_Vector,
	DPT_VectorXY, // Only encodes the XY of a vector, ignores Z
	DPT_String,
	DPT_Array,	// An array of the base types (can't be of datatables).
	DPT_DataTable,

#ifdef DT_QUATERNION_SUPPORTED
	DPT_Quaternion,
#endif

#ifdef DT_INT64_SUPPORTED
	DPT_Int64,
#endif

	DPT_NUMSendPropTypes

} SendPropType;


class DVariant
{
public:
	DVariant()				{m_Type = DPT_Float;}
	DVariant(float val)		{m_Type = DPT_Float; m_Float = val;}
	
	const char *ToString();

	union
	{
		float	m_Float;
		int		m_Int;
		unsigned int		m_UInt;
		char		m_Char;
		signed char		m_SChar;
		unsigned char		m_UChar;
		bool		m_Bool;
		short		m_Short;
		unsigned short		m_UShort;
		const char	*m_pString;
		void	*m_pData;	// For DataTables.
#if defined DT_QUATERNION_SUPPORTED || defined DT_INT64_SUPPORTED
		Quaternion	m_Quaternion;
#endif
		Vector2D	m_Vector2D;
		Vector	m_Vector;
		QAngle m_Angles;
	#if defined DT_INT64_SUPPORTED || defined DT_QUATERNION_SUPPORTED
		int64	m_Int64;
		uint64	m_UInt64;
	#else
		int	m_IntPair[2];
		uint	m_UIntPair[2];
	#endif
		color32 m_Color32;
		ColorRGBExp32 m_Color32E;
		color24 m_Color24;
	};
	SendPropType	m_Type;
};

#if !defined DT_INT64_SUPPORTED && !defined DT_QUATERNION_SUPPORTED
COMPILE_TIME_ASSERT( sizeof(DVariant) == 16 );
#elif defined DT_INT64_SUPPORTED || defined DT_QUATERNION_SUPPORTED
COMPILE_TIME_ASSERT( sizeof(DVariant) == 24 );
#endif

// This can be used to set the # of bits used to transmit a number between 0 and nMaxElements-1.
inline int NumBitsForCount( int nMaxElements )
{
	int nBits = 0;
	while ( nMaxElements > 0 )
	{
		++nBits;
		nMaxElements >>= 1;
	}
	return nBits;
}

// Format and allocate a string.
char* AllocateStringHelper( PRINTF_FORMAT_STRING const char *pFormat, ... );

// Allocates a string for a data table name. Data table names must be unique, so this will
// assert if you try to allocate a duplicate.
char* AllocateUniqueDataTableName( bool bSendTable, PRINTF_FORMAT_STRING const char *pFormat, ... );



#endif // DATATABLE_COMMON_H
