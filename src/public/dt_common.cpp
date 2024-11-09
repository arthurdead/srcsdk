//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "dt_common.h"
#include "utldict.h"

#include "tier0/memdbgon.h"

static CUtlDict<int,int> *g_STDict = 0;
static CUtlDict<int,int> *g_RTDict = 0;

const char *DVariant::ToString()
{
	static char text[128];

	switch ( m_Type )
	{
	case DPT_Int : 
		Q_snprintf( text, sizeof(text), "%i", m_Int );
		break;
	case DPT_Float :
		Q_snprintf( text, sizeof(text), "%.3f", m_Float );
		break;
	case DPT_Vector :
		Q_snprintf( text, sizeof(text), "(%.3f,%.3f,%.3f)", 
			m_Vector[0], m_Vector[1], m_Vector[2] );
		break;
	case DPT_VectorXY :
		Q_snprintf( text, sizeof(text), "(%.3f,%.3f)", 
			m_Vector[0], m_Vector[1] );
		break;
#ifdef DT_QUATERNION_SUPPORTED
	case DPT_Quaternion :
		Q_snprintf( text, sizeof(text), "(%.3f,%.3f,%.3f %.3f)", 
			m_Vector[0], m_Vector[1], m_Vector[2], m_Vector[3] );
		break;
#endif
	case DPT_String : 
		if ( m_pString ) 
			return m_pString;
		else
			return "NULL";
		break;
	case DPT_Array :
		Q_snprintf( text, sizeof(text), "Array" ); 
		break;
	case DPT_DataTable :
		Q_snprintf( text, sizeof(text), "DataTable" ); 
		break;
#ifdef DT_INT64_SUPPORTED
	case DPT_Int64:
		Q_snprintf( text, sizeof(text), "%I64d", m_Int64 );
		break;
#endif
	default :
		Q_snprintf( text, sizeof(text), "DVariant type %i unknown", m_Type ); 
		break;
	}

	return text;
}

char* AllocateStringHelper2( const char *pFormat, va_list marker )
{
	char str[512];
	_vsnprintf( str, sizeof( str ), pFormat, marker );
	str[ ARRAYSIZE(str) - 1 ] = 0;
	
	int len = strlen( str ) + 1;
	char *pRet = new char[len];
	memcpy( pRet, str, len );

	return pRet;
}


char* AllocateStringHelper( const char *pFormat, ... )
{
	va_list marker;
	va_start( marker, pFormat );
	char *pRet = AllocateStringHelper2( pFormat, marker );
	va_end( marker );

	return pRet;
}


char* AllocateUniqueDataTableName( bool bSendTable, const char *pFormat, ... )
{
	// Setup the string.
	va_list marker;
	va_start( marker, pFormat );
	char *pRet = AllocateStringHelper2( pFormat, marker );
	va_end( marker );

	// Make sure it's unique.
#ifdef _DEBUG
	// Have to allocate them here because if they're declared as straight global variables,
	// their constructors won't have been called yet by the time we get in here.
	if ( !g_STDict )
	{
		g_STDict = new CUtlDict<int,int>;
		g_RTDict = new CUtlDict<int,int>;
	}

	CUtlDict<int,int> *pDict = bSendTable ? g_STDict : g_RTDict;
	if ( pDict->Find( pRet ) != pDict->InvalidIndex() )
	{
		// If it hits this, then they have 2 utlvectors in different data tables with the same name and the
		// same size limit. The names of 
		Assert( false );
	}
	pDict->Insert( pRet, 0 );
#endif

	return pRet;
}
