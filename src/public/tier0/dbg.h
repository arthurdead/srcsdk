//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef DBG_H
#define DBG_H

#pragma once

#include "basetypes.h"
#include "dbgflag.h"
#include "platform.h"
#include <stdio.h>
#include <stdarg.h>
#include "icommandline.h"
#include "consoleio.h"
#include "hackmgr/hackmgr.h"
#include "Color.h"

#include "logging.h"

class CValidator;

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define DBG_INTERFACE	DLL_EXPORT
#define DBG_OVERLOAD	DLL_GLOBAL_EXPORT
#define DBG_GLOBAL	DLL_GLOBAL_EXPORT
#define DBG_CLASS		DLL_CLASS_EXPORT
#else
#define DBG_INTERFACE	DLL_IMPORT
#define DBG_OVERLOAD	DLL_GLOBAL_IMPORT
#define DBG_GLOBAL	DLL_GLOBAL_IMPORT
#define DBG_CLASS		DLL_CLASS_IMPORT
#endif

#else // BUILD_AS_DLL

#define DBG_INTERFACE	extern
#define DBG_OVERLOAD	extern
#define DBG_GLOBAL	extern
#define DBG_CLASS		
#endif // BUILD_AS_DLL

#if HACKMGR_ENGINE_TARGET == HACKMGR_ENGINE_TARGET_SDK2013MP
	#define DBG_INTERFACE_ABI_1 DBG_INTERFACE
#else
	#define DBG_INTERFACE_ABI_1 HACKMGR_API
#endif

//-----------------------------------------------------------------------------
// Usage model for the Dbg library
//
// 1. Spew.
// 
//   Spew can be used in a static and a dynamic mode. The static
//   mode allows us to display assertions and other messages either only
//   in debug builds, or in non-release builds. The dynamic mode allows us to
//   turn on and off certain spew messages while the application is running.
// 
//   Static Spew messages:
//
//     Assertions are used to detect and warn about invalid states
//     Spews are used to display a particular status/warning message.
//
//     To use an assertion, use
//
//     Assert( (f == 5) );
//     AssertMsg( (f == 5), ("F needs to be %d here!\n", 5) );
//     AssertFunc( (f == 5), BadFunc() );
//     AssertEquals( f, 5 );
//     AssertFloatEquals( f, 5.0f, 1e-3 );
//
//     The first will simply report that an assertion failed on a particular
//     code file and line. The second version will display a print-f formatted message 
//	   along with the file and line, the third will display a generic message and
//     will also cause the function BadFunc to be executed, and the last two
//	   will report an error if f is not equal to 5 (the last one asserts within
//	   a particular tolerance).
//
//     To use a warning, use
//      
//     Warning("Oh I feel so %s all over\n", "yummy");
//
//     Warning will do its magic in only Debug builds. To perform spew in *all*
//     builds, use RelWarning.
//
//	   Three other spew types, Msg, Log, and Error, are compiled into all builds.
//	   These error types do *not* need two sets of parenthesis.
//
//	   Msg( "Isn't this exciting %d?", 5 );
//	   Error( "I'm just thrilled" );
//
//   Dynamic Spew messages
//
//     It is possible to dynamically turn spew on and off. Dynamic spew is 
//     identified by a spew group and priority level. To turn spew on for a 
//     particular spew group, use SpewActivate( "group", level ). This will 
//     cause all spew in that particular group with priority levels <= the 
//     level specified in the SpewActivate function to be printed. Use DSpew 
//     to perform the spew:
//
//     DWarning( "group", level, "Oh I feel even yummier!\n" );
//
//     Priority level 0 means that the spew will *always* be printed, and group
//     '*' is the default spew group. If a DWarning is encountered using a group 
//     whose priority has not been set, it will use the priority of the default 
//     group. The priority of the default group is initially set to 0.      
//
//   Spew output
//   
//     The output of the spew system can be redirected to an externally-supplied
//     function which is responsible for outputting the spew. By default, the 
//     spew is simply printed using printf.
//
//     To redirect spew output, call SpewOutput.
//
//     SpewOutputFunc( OutputFunc );
//
//     This will cause OutputFunc to be called every time a spew message is
//     generated. OutputFunc will be passed a spew type and a message to print.
//     It must return a value indicating whether the debugger should be invoked,
//     whether the program should continue running, or whether the program 
//     should abort. 
//
// 2. Code activation
//
//   To cause code to be run only in debug builds, use DBG_CODE:
//   An example is below.
//
//   DBG_CODE(
//				{
//					int x = 5;
//					++x;
//				}
//           ); 
//
//   Code can be activated based on the dynamic spew groups also. Use
//  
//   DBG_DCODE( "group", level,
//              { int x = 5; ++x; }
//            );
//
// 3. Breaking into the debugger.
//
//   To cause an unconditional break into the debugger in debug builds only, use DBG_BREAK
//
//   DBG_BREAK();
//
//	 You can force a break in any build (release or debug) using
//
//	 DebuggerBreak();
//-----------------------------------------------------------------------------

DBG_INTERFACE void _ExitOnFatalAssert( const tchar* pFile, int line );
DBG_INTERFACE bool ShouldUseNewAssertDialog();

// Returns true if they want to break in the debugger.

//this is broken on my PC idk why
#ifndef __linux__
DBG_INTERFACE bool DoNewAssertDialog( const tchar *pFile, int line, const tchar *pExpression );
#else
HACKMGR_API bool HackMgr_DoNewAssertDialog( const tchar *pFile, int line, const tchar *pExpression );
#define DoNewAssertDialog HackMgr_DoNewAssertDialog
#endif

// Allows the assert dialogs to be turned off from code
DBG_INTERFACE bool AreAllAssertsDisabled();
DBG_INTERFACE void SetAllAssertsDisabled( bool bAssertsEnabled );

// Provides a callback that is called on asserts regardless of spew levels
typedef void (*AssertFailedNotifyFunc_t)( const char *pchFile, int nLine, const char *pchMessage );
DBG_INTERFACE void SetAssertFailedNotifyFunc( AssertFailedNotifyFunc_t func );
DBG_INTERFACE void CallAssertFailedNotifyFunc( const char *pchFile, int nLine, const char *pchMessage );

/* True if -hushasserts was passed on command line. */
DBG_INTERFACE_ABI_1 bool HushAsserts();

#if defined( USE_SDL )
struct SDL_Window;
DBG_INTERFACE void SetAssertDialogParent( SDL_Window *window );
DBG_INTERFACE SDL_Window * GetAssertDialogParent();
#endif

/* Used to define macros, never use these directly. */

#ifdef _PREFAST_
	// When doing /analyze builds define _AssertMsg to be __analysis_assume. This tells
	// the compiler to assume that the condition is true, which helps to suppress many
	// warnings. This define is done in debug and release builds.
	// The unfortunate !! is necessary because otherwise /analyze is incapable of evaluating
	// all of the logical expressions that the regular compiler can handle.
	// Include _msg in the macro so that format errors in it are detected.
	#define _AssertMsg( _exp, _msg, _executeExp, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	// Force asserts on for /analyze so that we get a __analysis_assume of all of the constraints.
	#define DBGFLAG_ASSERT
	#define DBGFLAG_ASSERTFATAL
	#define DBGFLAG_ASSERTDEBUG
#else
	#define  _AssertMsg( _exp, _msg, _executeExp, _bFatal )	\
		do {																\
			if (!(_exp)) 													\
			{ 																\
				LoggingResponse_t _ret = Log_Assert( "%s (%d) : %s\n", __TFILE__, __LINE__, static_cast<const char*>( _msg ) );	\
				CallAssertFailedNotifyFunc( __TFILE__, __LINE__, _msg );					\
				_executeExp; 												\
				if ( _ret == LR_DEBUGGER)									\
				{															\
					if ( !ShouldUseNewAssertDialog() || DoNewAssertDialog( __TFILE__, __LINE__, _msg ) ) \
					{														\
						DebuggerBreak();									\
					}														\
					if ( _bFatal )											\
					{														\
						_ExitOnFatalAssert( __TFILE__, __LINE__ );			\
					}														\
				}															\
			}																\
		} while (0)

	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) \
		do {																\
			static bool fAsserted;											\
			if (!fAsserted )												\
			{ 																\
				_AssertMsg( _exp, _msg, (fAsserted = true), _bFatal );		\
			}																\
		} while (0)
#endif

/* Spew macros... */

// AssertFatal macros
// AssertFatal is used to detect an unrecoverable error condition.
// If enabled, it may display an assert dialog (if DBGFLAG_ASSERTDLG is turned on or running under the debugger),
// and always terminates the application

#ifdef DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), true )
#define  AssertFatalOnce( _exp )								_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), true )
#define  AssertFatalMsg( _exp, _msg, ... )						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), true )
#define  AssertFatalMsgOnce( _exp, _msg )						_AssertMsgOnce( _exp, _msg, true )
#define  AssertFatalFunc( _exp, _f )							_AssertMsg( _exp, _T("Assertion Failed: " _T(#_exp), _f, true )
#define  AssertFatalEquals( _exp, _expectedValue )				AssertFatalMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )   AssertFatalMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  VerifyFatal( _exp )									AssertFatal( _exp )
#define  VerifyEqualsFatal( _exp, _expectedValue )				AssertFatalEquals( _exp, _expectedValue )

#define  AssertFatalMsg1( _exp, _msg, a1 )									AssertFatalMsg( _exp, _msg, a1 )
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								AssertFatalMsg( _exp, _msg, a1, a2 )
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							AssertFatalMsg( _exp, _msg, a1, a2, a3 )
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertFatalMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									((void)0)
#define  AssertFatalOnce( _exp )								((void)0)
#define  AssertFatalMsg( _exp, _msg )							((void)0)
#define  AssertFatalMsgOnce( _exp, _msg )						((void)0)
#define  AssertFatalFunc( _exp, _f )							((void)0)
#define  AssertFatalEquals( _exp, _expectedValue )				((void)0)
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  VerifyFatal( _exp )									(_exp)
#define  VerifyEqualsFatal( _exp, _expectedValue )				(_exp)

#define  AssertFatalMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERTFATAL

// Assert macros
// Assert is used to detect an important but survivable error.
// It's only turned on when DBGFLAG_ASSERT is true.

#ifdef DBGFLAG_ASSERT

#define  Assert( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsg( _exp, _msg, ... )  						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), false )
#define  AssertOnce( _exp )       							_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), false )
#define  AssertMsgOnce( _exp, _msg )  						_AssertMsgOnce( _exp, _msg, false )
#define  AssertFunc( _exp, _f )   							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), _f, false )
#define  AssertEquals( _exp, _expectedValue )              	AssertMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFloatEquals( _exp, _expectedValue, _tol )  	AssertMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  Verify( _exp )           							Assert( _exp )
#define  VerifyMsg1( _exp, _msg, a1 )						AssertMsg1( _exp, _msg, a1 )
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					AssertMsg2( _exp, _msg, a1, a2 )
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				AssertMsg3( _exp, _msg, a1, a2, a3 )
#define  VerifyEquals( _exp, _expectedValue )           	AssertEquals( _exp, _expectedValue )
#define  DbgVerify( _exp )           						Assert( _exp )

#define  AssertMsg1( _exp, _msg, a1 )									AssertMsg( _exp, _msg, a1 )
#define  AssertMsg2( _exp, _msg, a1, a2 )								AssertMsg( _exp, _msg, a1, a2 )
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							AssertMsg( _exp, _msg, a1, a2, a3 )
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERT

#define  Assert( _exp )										((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg, ... )						((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define	 VerifyMsg1( _exp, _msg, a1 )						(_exp)
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					(_exp)
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)
#define  DbgVerify( _exp )									(_exp)

#define  AssertMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERT

// The Always version of the assert macros are defined even when DBGFLAG_ASSERT is not, 
// so they will be available even in release.
#define  AssertAlways( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsgAlways( _exp, _msg )  							_AssertMsg( _exp, _msg, ((void)0), false )

// Macros to help decorate warnings or errors with the location in code
#define FILE_LINE_FUNCTION_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):" __FUNCTION__ ":"
#define FILE_LINE_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):"
#define FUNCTION_LINE_STRING __FUNCTION__ "(" V_STRINGIFY(__LINE__) "): "

// Handy define for inserting clickable messages into the build output.
// Use like this:
// #pragma MESSAGE("Some message")
#define MESSAGE(msg) message(__FILE__ "(" V_STRINGIFY(__LINE__) "): " msg)

DECLARE_LOGGING_CHANNEL( LOG_TIER0 );

//////////////////////////////////////////////////////////////////////////
// Legacy Logging System
//////////////////////////////////////////////////////////////////////////

// Channels which map the legacy logging system to the new system.

// Channel for all default Msg/Warning/Error commands.
DECLARE_LOGGING_CHANNEL( LOG_GENERAL );
// Channel for all asserts.
DECLARE_LOGGING_CHANNEL( LOG_ASSERT );
// Channel for all ConMsg and ConColorMsg commands.
DECLARE_LOGGING_CHANNEL( LOG_CONSOLE );
// Channel for all DevMsg and DevWarning commands with level < 2.
DECLARE_LOGGING_CHANNEL( LOG_DEVELOPER );
// Channel for ConDMsg commands.
DECLARE_LOGGING_CHANNEL( LOG_DEVELOPER_CONSOLE );
// Channel for all DevMsg and DevWarning commands with level >= 2.
DECLARE_LOGGING_CHANNEL( LOG_DEVELOPER_VERBOSE );

//TODO!!!!! remove
#define TIER0_ENABLE_LEGACY_DBG
#ifdef TIER0_ENABLE_LEGACY_DBG
#include "dbg_legacy.h"
#else
//use the logging system instead

inline void Msg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void DMsg( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void MsgV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) { DebuggerBreak(); }

inline void Warning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void DWarning( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void WarningV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) { DebuggerBreak(); }

inline void Log( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void DLog( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void LogV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) { DebuggerBreak(); }

inline void Error( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void ErrorV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) { DebuggerBreak(); }

inline void DevMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void DevWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void DevLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }

inline void DevMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void DevWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void DevLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }

inline void ConColorMsg( int level, const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void ConLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }

inline void ConColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void ConLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }

inline void ConDColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConDMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void ConDWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void ConDLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }

inline void NetMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) { DebuggerBreak(); }
inline void NetWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
inline void NetLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) { DebuggerBreak(); }
#endif

DBG_INTERFACE void COM_TimestampedLog( PRINTF_FORMAT_STRING char const *fmt, ... ) FMTFUNCTION( 1, 2 );

/* Code macros, debugger interface */

#ifdef DBGFLAG_ASSERT

#define DBG_CODE( _code )            if (0) ; else { _code }
#define DBG_CODE_NOSCOPE( _code )	 _code
#define DBG_DCODE( _g, _l, _code )   if (IsSpewActive( _g, _l )) { _code } else {}
#define DBG_BREAK()                  DebuggerBreak()	/* defined in platform.h */ 

#else /* not _DEBUG */

#define DBG_CODE( _code )            ((void)0)
#define DBG_CODE_NOSCOPE( _code )	 
#define DBG_DCODE( _g, _l, _code )   ((void)0)
#define DBG_BREAK()                  ((void)0)

#endif /* _DEBUG */

//-----------------------------------------------------------------------------

#ifndef _RETAIL
class CScopeMsg
{
public:
	CScopeMsg( const char *pszScope, LoggingChannelID_t nChannel )
	{
		m_Channel = nChannel;
		m_pszScope = pszScope;
		Log_Msg( nChannel, "%s { ", pszScope );
	}
	~CScopeMsg()
	{
		Log_Msg( m_Channel, "} %s", m_pszScope );
	}
	const char *m_pszScope;
	LoggingChannelID_t m_Channel;
};
#define SCOPE_MSG( msg, ... ) CScopeMsg scopeMsg( msg __VA_OPT__(, __VA_ARGS__) )
#else
#define SCOPE_MSG( msg, ... )
#endif


//-----------------------------------------------------------------------------
// Macro to assist in asserting constant invariants during compilation

// This implementation of compile time assert has zero cost (so it can safely be
// included in release builds) and can be used at file scope or function scope.
// We're using an ancient version of GCC that can't quite handle some
// of our complicated templates properly.  Use some preprocessor trickery
// to workaround this
#if defined __GNUC__ && 0
	#define COMPILE_TIME_ASSERT( pred ) typedef int UNIQUE_ID[ (pred) ? 1 : -1 ]
#else
	#if _MSC_VER >= 1600 || defined __GNUC__
	// If available use static_assert instead of weird language tricks. This
	// leads to much more readable messages when compile time assert constraints
	// are violated.
	#define COMPILE_TIME_ASSERT( pred ) static_assert( pred, "Compile time assert constraint is not true: " #pred )
	#else
	// Due to gcc bugs this can in rare cases (some template functions) cause redeclaration
	// errors when used multiple times in one scope. Fix by adding extra scoping.
	#define COMPILE_TIME_ASSERT( pred ) typedef char compile_time_assert_type[(pred) ? 1 : -1];
	#endif
#endif
// ASSERT_INVARIANT used to be needed in order to allow COMPILE_TIME_ASSERTs at global
// scope. However the new COMPILE_TIME_ASSERT macro supports that by default.
#define ASSERT_INVARIANT( pred )	COMPILE_TIME_ASSERT( pred )


#ifdef _DEBUG
template<typename DEST_POINTER_TYPE, typename SOURCE_POINTER_TYPE>
inline DEST_POINTER_TYPE assert_cast(SOURCE_POINTER_TYPE* pSource)
{
    Assert( static_cast<DEST_POINTER_TYPE>(pSource) == dynamic_cast<DEST_POINTER_TYPE>(pSource) );
    return static_cast<DEST_POINTER_TYPE>(pSource);
}
#else
#define assert_cast static_cast
#endif

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:

// Have to use these stubs so we don't have to include windows.h here.

DBG_INTERFACE void _AssertValidReadPtr( void* ptr, int count = 1 );
DBG_INTERFACE void _AssertValidWritePtr( void* ptr, int count = 1 );
DBG_INTERFACE void _AssertValidReadWritePtr( void* ptr, int count = 1 );
DBG_INTERFACE void AssertValidStringPtr( const tchar* ptr, int maxchar = 0xFFFFFF );

#ifdef DBGFLAG_ASSERT

FORCEINLINE void AssertValidReadPtr( const void* ptr, int count = 1 )	    { _AssertValidReadPtr( (void*)ptr, count ); }
FORCEINLINE void AssertValidWritePtr( const void* ptr, int count = 1 )		{ _AssertValidWritePtr( (void*)ptr, count ); }
FORCEINLINE void AssertValidReadWritePtr( const void* ptr, int count = 1 )	{ _AssertValidReadWritePtr( (void*)ptr, count ); }

#else

FORCEINLINE void AssertValidReadPtr( const void* ptr, int count = 1 )			 { }
FORCEINLINE void AssertValidWritePtr( const void* ptr, int count = 1 )		     { }
FORCEINLINE void AssertValidReadWritePtr( const void* ptr, int count = 1 )	     { }
#define AssertValidStringPtr AssertValidReadPtr

#endif

#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

//-----------------------------------------------------------------------------
// Macro to protect functions that are not reentrant

#ifdef _DEBUG
class CReentryGuard
{
public:
	CReentryGuard(int *pSemaphore)
	 : m_pSemaphore(pSemaphore)
	{
		++(*m_pSemaphore);
	}
	
	~CReentryGuard()
	{
		--(*m_pSemaphore);
	}
	
private:
	int *m_pSemaphore;
};

#define ASSERT_NO_REENTRY() \
	static int fSemaphore##__LINE__; \
	Assert( !fSemaphore##__LINE__ ); \
	CReentryGuard ReentryGuard##__LINE__( &fSemaphore##__LINE__ )
#else
#define ASSERT_NO_REENTRY()
#endif

//-----------------------------------------------------------------------------
//
// Purpose: Inline string formatter
//

#include "tier0/valve_off.h"
class CDbgFmtMsg
{
public:
	CDbgFmtMsg(PRINTF_FORMAT_STRING const tchar *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{ 
		va_list arg_ptr;

		va_start(arg_ptr, pszFormat);
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, pszFormat, arg_ptr);
		va_end(arg_ptr);

		m_szBuf[sizeof(m_szBuf)-1] = 0;
	}

	operator const tchar *() const				
	{ 
		return m_szBuf; 
	}

private:
	tchar m_szBuf[256];
};
#include "tier0/valve_on.h"

//-----------------------------------------------------------------------------
//
// Purpose: Embed debug info in each file.
//
#if defined( _WIN32 )

	#ifdef _DEBUG
		#pragma comment(compiler)
	#endif

#endif

//-----------------------------------------------------------------------------
//
// Purpose: Wrap around a variable to create a simple place to put a breakpoint
//

#ifdef _DEBUG

template< class Type >
class CDataWatcher
{
public:
	const Type& operator=( const Type &val ) 
	{ 
		return Set( val ); 
	}
	
	const Type& operator=( const CDataWatcher<Type> &val ) 
	{ 
		return Set( val.m_Value ); 
	}
	
	const Type& Set( const Type &val )
	{
		// Put your breakpoint here
		m_Value = val;
		return m_Value;
	}
	
	Type& GetForModify()
	{
		return m_Value;
	}
	
	const Type& operator+=( const Type &val ) 
	{
		return Set( m_Value + val ); 
	}
	
	const Type& operator-=( const Type &val ) 
	{
		return Set( m_Value - val ); 
	}
	
	const Type& operator/=( const Type &val ) 
	{
		return Set( m_Value / val ); 
	}
	
	const Type& operator*=( const Type &val ) 
	{
		return Set( m_Value * val ); 
	}
	
	const Type& operator^=( const Type &val ) 
	{
		return Set( m_Value ^ val ); 
	}
	
	const Type& operator|=( const Type &val ) 
	{
		return Set( m_Value | val ); 
	}
	
	const Type& operator++()
	{
		return (*this += 1);
	}
	
	Type operator--()
	{
		return (*this -= 1);
	}
	
	Type operator++( int ) // postfix version..
	{
		Type val = m_Value;
		(*this += 1);
		return val;
	}
	
	Type operator--( int ) // postfix version..
	{
		Type val = m_Value;
		(*this -= 1);
		return val;
	}
	
	// For some reason the compiler only generates type conversion warnings for this operator when used like 
	// CNetworkVarBase<unsigned tchar> = 0x1
	// (it warns about converting from an int to an unsigned char).
	template< class C >
	const Type& operator&=( C val ) 
	{ 
		return Set( m_Value & val ); 
	}
	
	operator const Type&() const 
	{
		return m_Value; 
	}
	
	const Type& Get() const 
	{
		return m_Value; 
	}
	
	const Type* operator->() const 
	{
		return &m_Value; 
	}
	
	Type m_Value;
	
};

#else

template< class Type >
class CDataWatcher
{
private:
	CDataWatcher(); // refuse to compile in non-debug builds
};

#endif

//-----------------------------------------------------------------------------

#endif /* DBG_H */
