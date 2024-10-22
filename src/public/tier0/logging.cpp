#include "logging.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef printf

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_ASSERT, "Assert" );

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_TIER0, "Tier0" );

CSimpleLoggingListener::CSimpleLoggingListener( bool bQuietPrintf, bool bQuietDebugger ) : 
  m_bQuietPrintf( bQuietPrintf ), 
	  m_bQuietDebugger( bQuietDebugger ) 
{ 
}

void CSimpleLoggingListener::Log( const LoggingContext_t *pContext, const tchar *pMessage )
{
#ifndef _CERT
	if ( !m_bQuietPrintf )
	{
		_tprintf( _T("%s"), pMessage );
	}
#endif

	if ( !m_bQuietDebugger && Plat_IsInDebugSession() )
	{
		Plat_DebugString( pMessage );
	}
}

void CSimpleGUIoggingListener::Log( const LoggingContext_t *pContext, const tchar *pMessage )
{
	if ( Plat_IsInDebugSession() )
	{
		Plat_DebugString( pMessage );
	}
	if ( pContext->m_Severity == LS_FATAL_ERROR )
	{
		if ( Plat_IsInDebugSession() )
			DebuggerBreak();

		Plat_MessageBox( "Error", pMessage );
	}
}

CColorizedLoggingListener::CColorizedLoggingListener( bool bQuietPrintf, bool bQuietDebugger ) : CSimpleLoggingListener( bQuietPrintf, bQuietDebugger )
{
	InitConsoleColorContext( &m_ColorContext );
}

void CColorizedLoggingListener::Log( const LoggingContext_t *pContext, const tchar *pMessage )
{
	if ( !m_bQuietPrintf )
	{
		int nPrevColor = -1;

		if ( pContext->m_Color != UNSPECIFIED_LOGGING_COLOR )
		{
			nPrevColor = SetConsoleColor( &m_ColorContext,
				pContext->m_Color.r(), pContext->m_Color.g(), pContext->m_Color.b(), 
				MAX( MAX( pContext->m_Color.r(), pContext->m_Color.g() ), pContext->m_Color.b() ) > 128 );
		}

		_tprintf( _T("%s"), pMessage );

		if ( nPrevColor >= 0 )
		{
			RestoreConsoleColor( &m_ColorContext, nPrevColor );
		}
	}

	if ( !m_bQuietDebugger && Plat_IsInDebugSession() )
	{
		Plat_DebugString( pMessage );
	}
}

LoggingResponse_t CDefaultLoggingResponsePolicy::OnLog( const LoggingContext_t *pContext )
{
	if ( pContext->m_Severity == LS_ASSERT && !CommandLine()->FindParm( "-noassert" ) ) 
	{
		return LR_DEBUGGER;
	}
	else if ( pContext->m_Severity == LS_FATAL_ERROR )
	{
		return LR_ABORT;
	}
	else
	{
		return LR_CONTINUE;
	}
}

LoggingResponse_t CNonFatalLoggingResponsePolicy::OnLog( const LoggingContext_t *pContext )
{
	if ( ( pContext->m_Severity == LS_ASSERT && !CommandLine()->FindParm( "-noassert" ) ) || pContext->m_Severity == LS_FATAL_ERROR )
	{
		return LR_DEBUGGER;
	}
	else
	{
		return LR_CONTINUE;
	}
}
