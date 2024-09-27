#include "hackmgr/hackmgr.h"
#include "tier0/dbg.h"

#include "dbg_legacy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef printf

DLL_EXPORT SELECTANY bool HushAsserts()
{
#ifdef DBGFLAG_ASSERT
	static bool s_bHushAsserts = !!CommandLine()->FindParm( "-hushasserts" );
	return s_bHushAsserts;
#else
	return true;
#endif
}

DEFINE_DLL_LOGGING_CHANNEL_NO_TAGS( LOG_TIER0, "Tier0" );
DEFINE_DLL_LOGGING_CHANNEL_NO_TAGS( LOG_TRACE, "EngineTrace" );
DEFINE_DLL_LOGGING_CHANNEL_NO_TAGS( LOG_FOUNDRY, "Foundry" );

static LoggingChannelID_t s_NextChannelID = 1;

DLL_EXPORT bool LoggingSystem_IsChannelEnabled( LoggingChannelID_t channelID, LoggingSeverity_t severity )
{
	return true;
}

DLL_EXPORT LoggingChannelID_t LoggingSystem_RegisterLoggingChannel( const char *pName, RegisterTagsFunc registerTagsFunc, int flags, LoggingSeverity_t severity, Color color )
{
	if( registerTagsFunc )
		registerTagsFunc();

	return s_NextChannelID++;
}

DLL_EXPORT LoggingResponse_t LoggingSystem_Log( LoggingChannelID_t channelID, LoggingSeverity_t severity, const char *pMessageFormat, ... )
{
	tchar formattedMessage[MAX_LOGGING_MESSAGE_LENGTH];

	va_list args;
	va_start( args, pMessageFormat );
	_vsntprintf( formattedMessage, MAX_LOGGING_MESSAGE_LENGTH, pMessageFormat, args );
	va_end( args );

	{
		_tprintf( _T("%s"), formattedMessage );
#ifdef _WIN32
		Plat_DebugString( formattedMessage );
#endif
	}

	switch (severity) {
	case LS_MESSAGE:
		return LR_CONTINUE;
	case LS_WARNING:
		return LR_CONTINUE;
	case LS_ASSERT: {
	#ifndef WIN32
		// Non-win32
		bool bRaiseOnAssert = getenv( "RAISE_ON_ASSERT" ) || !!CommandLine()->FindParm( "-raiseonassert" );
	#elif defined( _DEBUG )
		// Win32 debug
		bool bRaiseOnAssert = true;
	#else
		// Win32 release
		bool bRaiseOnAssert = !!CommandLine()->FindParm( "-raiseonassert" );
	#endif

		return bRaiseOnAssert ? LR_DEBUGGER : LR_CONTINUE;
	}
	case LS_FATAL_ERROR:
		return LR_ABORT;
	case LS_ERROR:
		return LR_CONTINUE;
	}

	return LR_CONTINUE;
}

DLL_EXPORT LoggingResponse_t LoggingSystem_LogAssert( const char *pMessageFormat, ... ) 
{
	tchar formattedMessage[MAX_LOGGING_MESSAGE_LENGTH];

	va_list args;
	va_start( args, pMessageFormat );
	_vsntprintf( formattedMessage, MAX_LOGGING_MESSAGE_LENGTH, pMessageFormat, args );
	va_end( args );

	{
		_tprintf( _T("%s"), formattedMessage );
#ifdef _WIN32
		Plat_DebugString( formattedMessage );
#endif
	}

#ifndef WIN32
	// Non-win32
	bool bRaiseOnAssert = getenv( "RAISE_ON_ASSERT" ) || !!CommandLine()->FindParm( "-raiseonassert" );
#elif defined( _DEBUG )
	// Win32 debug
	bool bRaiseOnAssert = true;
#else
	// Win32 release
	bool bRaiseOnAssert = !!CommandLine()->FindParm( "-raiseonassert" );
#endif

	return bRaiseOnAssert ? LR_DEBUGGER : LR_CONTINUE;
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)



HACKMGR_EXECUTE_ON_LOAD_END
