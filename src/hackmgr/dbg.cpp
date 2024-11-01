#define TIER0_ENABLE_LEGACY_DBG

#include "tier0/platform.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "hackmgr/hackmgr.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "tier1/UtlStringMap.h"
#include "tier1/utlstring.h"
#include "tier1/utlmap.h"
#include "vstdlib/cvar.h"
#include "dbg_internal.h"

#include "dbg_legacy.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef printf

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_DEVELOPER, "Developer" );
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_CONSOLE, "Console" );

#ifdef _WIN32
PLATFORM_INTERFACE bool SetupWin32ConsoleIO();
typedef ConsoleColorContext_t Win32ConsoleColorContext_t;
PLATFORM_INTERFACE void InitWin32ConsoleColorContext( Win32ConsoleColorContext_t *pContext );
PLATFORM_INTERFACE uint16 SetWin32ConsoleColor( Win32ConsoleColorContext_t *pContext, int nRed, int nGreen, int nBlue, int nIntensity );
PLATFORM_INTERFACE void RestoreWin32ConsoleColor( Win32ConsoleColorContext_t *pContext, uint16 prevColor );
#endif

DLL_EXPORT bool SetupConsoleIO()
{
#ifdef _WIN32
	return SetupWin32ConsoleIO();
#else
	return false;
#endif
}

DLL_EXPORT void InitConsoleColorContext( ConsoleColorContext_t *pContext )
{
#ifdef _WIN32
	InitWin32ConsoleColorContext( pContext );
#endif
}

DLL_EXPORT uint16 SetConsoleColor( ConsoleColorContext_t *pContext, int nRed, int nGreen, int nBlue, int nIntensity )
{
#ifdef _WIN32
	return SetWin32ConsoleColor( pContext, nRed, nGreen, nBlue, nIntensity );
#else
	return -1;
#endif
}

DLL_EXPORT void RestoreConsoleColor( ConsoleColorContext_t *pContext, uint16 prevColor )
{
#ifdef _WIN32
	RestoreWin32ConsoleColor( pContext, prevColor );
#endif
}

#ifdef __MINGW32__
LIB_EXPORT SYMALIAS("InitConsoleColorContext") void _imp__InitConsoleColorContext( ConsoleColorContext_t *pContext );
LIB_EXPORT SYMALIAS("SetConsoleColor") uint16 _imp__SetConsoleColor( ConsoleColorContext_t *pContext, int nRed, int nGreen, int nBlue, int nIntensity );
LIB_EXPORT SYMALIAS("RestoreConsoleColor") void _imp__RestoreConsoleColor( ConsoleColorContext_t *pContext, uint16 prevColor );
#endif

#ifdef _WIN32
DLL_EXPORT void InitWin32ConsoleColorContext( Win32ConsoleColorContext_t *pContext )
{

}

DLL_EXPORT uint16 SetWin32ConsoleColor( Win32ConsoleColorContext_t *pContext, int nRed, int nGreen, int nBlue, int nIntensity )
{
	return -1;
}

DLL_EXPORT void RestoreWin32ConsoleColor( Win32ConsoleColorContext_t *pContext, uint16 prevColor )
{

}
#endif

#if 0
DLL_EXPORT bool HushAsserts()
{
#ifdef DBGFLAG_ASSERT
	static bool s_bHushAsserts = !!CommandLine()->FindParm( "-hushasserts" );
	return s_bHushAsserts;
#else
	return true;
#endif
}
#endif

INIT_PRIORITY(101) static CLoggingSystem s_LoggingSystem;

CLoggingSystem::CLoggingSystem()
{
	m_nChannelCount = 0;
}

CLoggingSystem::~CLoggingSystem()
{

}

CLoggingSystem::LoggingChannel_t *CLoggingSystem::GetChannel( LoggingChannelID_t channelID )
{
	Assert( channelID != INVALID_LOGGING_CHANNEL_ID );
	return &m_RegisteredChannels[channelID];
}

const CLoggingSystem::LoggingChannel_t *CLoggingSystem::GetChannel( LoggingChannelID_t channelID ) const
{
	Assert( channelID != INVALID_LOGGING_CHANNEL_ID );
	return &m_RegisteredChannels[channelID];
}

LoggingChannelID_t CLoggingSystem::RegisterLoggingChannel( const char *pChannelName, RegisterTagsFunc registerTagsFunc, LoggingChannelFlags_t flags, LoggingSeverity_t severity, Color spewColor )
{
	Assert( m_nChannelCount < MAX_LOGGING_CHANNEL_COUNT );

	for ( int i = 0; i < m_nChannelCount; ++i )
	{
		if ( V_stricmp( m_RegisteredChannels[i].m_Name, pChannelName ) == 0 )
		{
			if ( registerTagsFunc != NULL )
			{
				registerTagsFunc();
			}

			if ( m_RegisteredChannels[i].m_SpewColor == UNSPECIFIED_LOGGING_COLOR )
			{
				m_RegisteredChannels[i].m_SpewColor = spewColor;
			}

			return m_RegisteredChannels[i].m_ID;
		}
	}

	m_RegisteredChannels[m_nChannelCount].m_ID = m_nChannelCount;
	m_RegisteredChannels[m_nChannelCount].m_Flags = ( LoggingChannelFlags_t )flags;
	m_RegisteredChannels[m_nChannelCount].m_MinimumSeverity = severity;
	m_RegisteredChannels[m_nChannelCount].m_SpewColor = spewColor;
	V_strncpy( m_RegisteredChannels[m_nChannelCount].m_Name, pChannelName, MAX_LOGGING_IDENTIFIER_LENGTH );

	if ( registerTagsFunc != NULL ) 
	{
		registerTagsFunc();
	}

	return m_nChannelCount++;
}

DLL_EXPORT bool LoggingSystem_IsChannelEnabled( LoggingChannelID_t channelID, LoggingSeverity_t severity )
{
	return s_LoggingSystem.IsChannelEnabled( channelID, severity );
}

DLL_EXPORT LoggingChannelID_t LoggingSystem_RegisterLoggingChannel( const char *pName, RegisterTagsFunc registerTagsFunc, LoggingChannelFlags_t flags, LoggingSeverity_t severity, Color color )
{
	return s_LoggingSystem.RegisterLoggingChannel( pName, registerTagsFunc, flags, severity, color );
}

static Color clr_msg(255, 255, 255, 255);
static Color clr_warning(255, 255, 0, 255);
static Color clr_assert(255, 0, 255, 255);
static Color clr_fatal_error(255, 0, 0, 255);
static Color clr_error(255, 0, 0, 255);

static bool RaiseOnAssert()
{
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

	return bRaiseOnAssert;
}

LoggingResponse_t CLoggingSystem::LogDirect( LoggingChannelID_t channelID, LoggingSeverity_t severity, Color color, const tchar *pMessage )
{
	Assert( channelID != INVALID_LOGGING_CHANNEL_ID );
	if ( channelID == INVALID_LOGGING_CHANNEL_ID )
		return LR_DEBUGGER;

	LoggingChannel_t *pChannel = GetChannel( channelID );

	if(color == UNSPECIFIED_LOGGING_COLOR) {
		if(pChannel->m_SpewColor != UNSPECIFIED_LOGGING_COLOR) {
			color = pChannel->m_SpewColor;
		} else {
			switch (severity) {
			case LS_MESSAGE:
				color = clr_msg;
				break;
			case LS_WARNING:
				color = clr_warning;
				break;
			case LS_ASSERT:
				color = clr_assert;
				break;
			case LS_FATAL_ERROR:
				color = clr_fatal_error;
				break;
			case LS_ERROR:
				color = clr_error;
				break;
			}
		}
	}

	{
	#ifdef __linux__
		_tprintf( _T("\033[38;2;%i;%i;%im%s\033[0m"), color.r(), color.g(), color.b(), pMessage );
	#else
		_tprintf( _T("%s"), pMessage );
	#endif

#ifdef _WIN32
		Plat_DebugString( pMessage );
#endif

		g_pCVar->ConsoleColorPrintf( color, "%s", pMessage );
	}

	switch (severity) {
	case LS_MESSAGE:
		return LR_CONTINUE;
	case LS_WARNING:
		return LR_CONTINUE;
	case LS_ASSERT:
		return RaiseOnAssert() ? LR_DEBUGGER : LR_CONTINUE;
	case LS_FATAL_ERROR:
		return LR_ABORT;
	case LS_ERROR:
		return LR_CONTINUE;
	}

	return LR_DEBUGGER;
}

DLL_EXPORT LoggingResponse_t LoggingSystem_Log( LoggingChannelID_t channelID, LoggingSeverity_t severity, const char *pMessageFormat, ... )
{
	if ( !s_LoggingSystem.IsChannelEnabled( channelID, severity ) )
		return LR_CONTINUE;

	tchar formattedMessage[MAX_LOGGING_MESSAGE_LENGTH];

	va_list args;
	va_start( args, pMessageFormat );
	_vsntprintf( formattedMessage, MAX_LOGGING_MESSAGE_LENGTH, pMessageFormat, args );
	va_end( args );

	return s_LoggingSystem.LogDirect( channelID, severity, UNSPECIFIED_LOGGING_COLOR, formattedMessage );
}

DLL_GLOBAL_EXPORT LoggingResponse_t LoggingSystem_Log( LoggingChannelID_t channelID, LoggingSeverity_t severity, Color spewColor, const char *pMessageFormat, ... )
{
	if ( !s_LoggingSystem.IsChannelEnabled( channelID, severity ) )
		return LR_CONTINUE;

	tchar formattedMessage[MAX_LOGGING_MESSAGE_LENGTH];

	va_list args;
	va_start( args, pMessageFormat );
	_vsntprintf( formattedMessage, MAX_LOGGING_MESSAGE_LENGTH, pMessageFormat, args );
	va_end( args );

	return s_LoggingSystem.LogDirect( channelID, severity, spewColor, formattedMessage );
}

DLL_EXPORT LoggingResponse_t LoggingSystem_LogAssert( const char *pMessageFormat, ... ) 
{
	if ( !s_LoggingSystem.IsChannelEnabled( LOG_ASSERT, LS_ASSERT ) )
		return LR_CONTINUE;

	tchar formattedMessage[MAX_LOGGING_MESSAGE_LENGTH];

	va_list args;
	va_start( args, pMessageFormat );
	_vsntprintf( formattedMessage, MAX_LOGGING_MESSAGE_LENGTH, pMessageFormat, args );
	va_end( args );

	return s_LoggingSystem.LogDirect( LOG_ASSERT, LS_ASSERT, UNSPECIFIED_LOGGING_COLOR, formattedMessage );
}

#ifdef __MINGW32__
LIB_EXPORT SYMALIAS("LoggingSystem_IsChannelEnabled") bool _imp__LoggingSystem_IsChannelEnabled( LoggingChannelID_t channelID, LoggingSeverity_t severity );
LIB_EXPORT SYMALIAS("LoggingSystem_RegisterLoggingChannel") LoggingChannelID_t _imp__LoggingSystem_RegisterLoggingChannel( const char *pName, RegisterTagsFunc registerTagsFunc, LoggingChannelFlags_t flags, LoggingSeverity_t severity, Color color );
LIB_EXPORT SYMALIAS("LoggingSystem_Log") LoggingResponse_t _imp__LoggingSystem_Log( LoggingChannelID_t channelID, LoggingSeverity_t severity, const char *pMessageFormat, ... );
LIB_EXPORT SYMALIAS("LoggingSystem_LogAssert") LoggingResponse_t _imp__LoggingSystem_LogAssert( const char *pMessageFormat, ... );
#endif

//the normal DoNewAssertDialog is broken on my PC
#ifdef __linux__
static const char *zenity_path = NULL;

struct ignored_assert
{
	int line;
};

static CUtlStringMap<ignored_assert> ignored_asserts;

static char zenity_buffer[2048];
HACKMGR_API bool HackMgr_DoNewAssertDialog( const tchar *pFile, int line, const tchar *pExpression )
{
	if(AreAllAssertsDisabled()) {
		return false;
	}

	UtlSymId_t id = ignored_asserts.Find(pFile);
	if(id != ignored_asserts.InvalidIndex()) {
		const ignored_assert &info = ignored_asserts[id];
		if(info.line == -1 || info.line == line) {
			return false;
		}
	}

	int stdout_pipes[2];
	if(pipe(stdout_pipes) == -1) {
		return true;
	}

	V_snprintf(zenity_buffer, sizeof(zenity_buffer), "File: %s\nLine: %i\nExpr: %s\n", pFile, line, pExpression);

	pid_t pid(vfork());
	if(pid == 0) {
		if(dup2(stdout_pipes[1], STDOUT_FILENO) == -1) {
			_exit(1);
		}

		close(stdout_pipes[0]);
		close(stdout_pipes[1]);

		const char *argv[]{
			zenity_path,
			"--error",
			"--no-markup",
			"--title","Assertion Failed",
			"--text",zenity_buffer,
			"--ok-label","Break",
			"--extra-button","Ignore",
			"--extra-button","Ignore This File",
			"--extra-button","Always Ignore",
			"--extra-button","Ignore All Asserts",
			NULL
		};
		execve(zenity_path, const_cast<char **>(argv), environ);
		_exit(1);
	} else if(pid == -1) {
		close(stdout_pipes[1]);
		return true;
	}

	close(stdout_pipes[1]);

	char *const buffer_start(zenity_buffer);

	char *buffer_end(buffer_start + sizeof(zenity_buffer));
	char *buffer_it(buffer_start);

	int status;
	waitpid(pid, &status, 0);

	while(read(stdout_pipes[0], buffer_it, 1) == 1 && buffer_it < buffer_end) {
		++buffer_it;
	}

	close(stdout_pipes[0]);

	if(!WIFEXITED(status)) {
		return true;
	}

	--buffer_it;
	*buffer_it = '\0';

	if(V_strcmp(buffer_start,"Break") == 0 || V_strcmp(buffer_start,"Corefile") == 0) {
		return true;
	} else if(V_strcmp(buffer_start,"Ignore") == 0) {
		return false;
	} else if(V_strcmp(buffer_start,"Always Ignore") == 0) {
		ignored_asserts[pFile].line = line;
		return false;
	} else if(V_strcmp(buffer_start,"Ignore This File") == 0) {
		ignored_asserts[pFile].line = -1;
		return false;
	} else if(V_strcmp(buffer_start,"Ignore All Asserts") == 0) {
		SetAllAssertsDisabled(true);
		return false;
	}

	return true;
}
#endif

DLL_EXPORT void Plat_MessageBox( const char *pTitle, const tchar *pMessage )
{
#ifdef __linux__
	pid_t pid(vfork());
	if(pid == 0) {
		const char *argv[]{
			zenity_path,
			"--error",
			"--no-markup",
			"--title",pTitle,
			"--text",pMessage,
			NULL
		};
		execve(zenity_path, const_cast<char **>(argv), environ);
		_exit(1);
	} else if(pid == -1) {
		return;
	}

	int status;
	waitpid(pid, &status, 0);
#else
	MessageBox( NULL, pMessage, pTitle, MB_OK );
#endif
}

#ifdef __MINGW32__
LIB_EXPORT SYMALIAS("Plat_MessageBox") void _imp__Plat_MessageBox( const char *pTitle, const tchar *pMessage );
#endif

static SpewRetval_t HackMgr_SpewOutput( SpewType_t spewType, const tchar *pMsg )
{
	Color color = clr_fatal_error;
	SpewRetval_t ret = SPEW_DEBUGGER;

	switch(spewType) {
	case SPEW_MESSAGE:
		ret = SPEW_CONTINUE;
		break;
	case SPEW_WARNING:
		ret = SPEW_CONTINUE;
		break;
	case SPEW_ASSERT:
		ret = RaiseOnAssert() ? SPEW_DEBUGGER : SPEW_CONTINUE;
		break;
	case SPEW_ERROR:
		ret = SPEW_ABORT;
		break;
	case SPEW_LOG:
		ret = SPEW_CONTINUE;
		break;
	}

	const tchar *group = GetSpewOutputGroup();
	int level = GetSpewOutputLevel();

	const Color *spew_clr = GetSpewOutputColor();
	if(spew_clr && *spew_clr != Color(0, 0, 0, 0) && *spew_clr != Color(255, 255, 255, 255)) {
		color = *spew_clr;
	} else {
		switch(spewType) {
		case SPEW_MESSAGE:
			color = clr_msg;
			break;
		case SPEW_WARNING:
			color = clr_warning;
			break;
		case SPEW_ASSERT:
			color = clr_assert;
			break;
		case SPEW_ERROR:
			color = clr_fatal_error;
			break;
		case SPEW_LOG:
			color = clr_msg;
			break;
		}
	}

	{
	#ifdef __linux__
		_tprintf( _T("\033[38;2;%i;%i;%im%s\033[0m"), color.r(), color.g(), color.b(), pMsg );
	#else
		_tprintf( _T("%s"), pMsg );
	#endif

#ifdef _WIN32
		Plat_DebugString( pMsg );
#endif

		g_pCVar->ConsoleColorPrintf( color, "%s", pMsg );
	}

	return ret;
}

void Install_HackMgrSpew()
{
	SpewOutputFunc(HackMgr_SpewOutput);
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

#ifdef __linux__
/*zenity_path = getenv("SYSTEM_ZENITY");
if(!zenity_path) {
	zenity_path = getenv("STEAM_ZENITY");
}*/
if(!zenity_path) {
	zenity_path = "/usr/bin/zenity";
}
#endif

if(!VStdLib_GetICVarFactory()) {
	return;
}

int status = IFACE_OK;
g_pCVar = (ICvar *)VStdLib_GetICVarFactory()(CVAR_INTERFACE_VERSION, &status);
if(!g_pCVar || status != IFACE_OK) {
	return;
}

HACKMGR_EXECUTE_ON_LOAD_END
