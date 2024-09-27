#include "scenetokenprocessor.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSceneTokenProcessor::CurrentToken( void )
{
	return m_szToken;
}

// wordbreak parsing set
static characterset_t	g_SceneBreakSet;

static void InitializeSceneCharacterSets()
{
	static bool s_CharacterSetInitialized = false;
	if (!s_CharacterSetInitialized)
	{
		CharacterSetBuild( &g_SceneBreakSet, "{}()':" );
		s_CharacterSetInitialized = true;
	}
}

static const char* SceneParseFileInternal( const char* pFileBytes, char* pTokenOut, size_t nMaxTokenLen )
{
	pTokenOut[0] = 0;

	if (!pFileBytes)
		return 0;

	InitializeSceneCharacterSets();

	int c;
	unsigned int len = 0;
	
// skip whitespace
skipwhite:

	while ( (c = *pFileBytes) <= ' ')
	{
		if (c == 0)
			return 0;                    // end of file;
		pFileBytes++;
	}
	
// skip // comments
	if (c=='/' && pFileBytes[1] == '/')
	{
		while (*pFileBytes && *pFileBytes != '\n')
			pFileBytes++;
		goto skipwhite;
	}
	
// skip c-style comments
	if (c=='/' && pFileBytes[1] == '*' )
	{
		// Skip "/*"
		pFileBytes += 2;

		while ( *pFileBytes  )
		{
			if ( *pFileBytes == '*' &&
				 pFileBytes[1] == '/' )
			{
				pFileBytes += 2;
				break;
			}

			pFileBytes++;
		}

		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		pFileBytes++;
		while (1)
		{
			c = *pFileBytes++;
			if (c=='\"' || !c)
			{
				pTokenOut[len] = 0;
				return pFileBytes;
			}
			pTokenOut[len] = c;
			len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		}
	}

// parse single characters
	if ( IN_CHARACTERSET( g_SceneBreakSet, c ) )
	{
		pTokenOut[len] = c;
		len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		pTokenOut[len] = 0;
		return pFileBytes+1;
	}

// parse a regular word
	do
	{
		pTokenOut[len] = c;
		pFileBytes++;
		len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		c = *pFileBytes;
		if ( IN_CHARACTERSET( g_SceneBreakSet, c ) )
			break;
	} while (c>32);
	
	pTokenOut[len] = 0;
	return pFileBytes;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crossline - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::GetToken( bool crossline )
{
	// NOTE: crossline is ignored here, may need to implement if needed
	m_pBuffer = SceneParseFileInternal( m_pBuffer, m_szToken, sizeof( m_szToken ) );
	if ( m_szToken[0] )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::TokenAvailable( void )
{
	char const *search_p = m_pBuffer;

	while ( *search_p <= 32)
	{
		if (*search_p == '\n')
			return false;
		search_p++;
		if ( !*search_p )
			return false;

	}

	if (*search_p == ';' || *search_p == '#' ||		 // semicolon and # is comment field
		(*search_p == '/' && *((search_p)+1) == '/')) // also make // a comment field
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::Error( const char *fmt, ... )
{
	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof(string), fmt, argptr );
	va_end( argptr );

	Warning( "%s", string );
	Assert(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::SetBuffer( char *buffer )
{
	m_pBuffer = buffer;
}

CSceneTokenProcessor g_TokenProcessor;
