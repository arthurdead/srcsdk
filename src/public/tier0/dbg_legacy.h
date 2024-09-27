#ifndef DBG_LEGACY_H
#define DBG_LEGACY_H

#pragma once

#include "platform.h"
#include "annotations.h"

class Color;

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define DBG_INTERFACE	DLL_EXPORT
#define DBG_OVERLOAD	DLL_GLOBAL_EXPORT
#define DBG_CLASS		DLL_CLASS_EXPORT
#else
#define DBG_INTERFACE	DLL_IMPORT
#define DBG_OVERLOAD	DLL_GLOBAL_IMPORT
#define DBG_CLASS		DLL_CLASS_IMPORT
#endif

#else // BUILD_AS_DLL

#define DBG_INTERFACE	extern
#define DBG_OVERLOAD	
#define DBG_CLASS		
#endif // BUILD_AS_DLL

/* Various types of spew messages */
// I'm sure you're asking yourself why SPEW_ instead of DBG_ ?
// It's because DBG_ is used all over the place in windows.h
// For example, DBG_CONTINUE is defined. Feh.
enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,

	SPEW_TYPE_COUNT
};

enum SpewRetval_t
{
	SPEW_DEBUGGER = 0,
	SPEW_CONTINUE,
	SPEW_ABORT
};

/* type of externally defined function used to display debug spew */
typedef SpewRetval_t (*SpewOutputFunc_t)( SpewType_t spewType, const tchar *pMsg );

/* Used to redirect spew output */
DBG_INTERFACE void   SpewOutputFunc( SpewOutputFunc_t func );

/* Used to get the current spew output function */
DBG_INTERFACE SpewOutputFunc_t GetSpewOutputFunc( void );

/* This is the default spew fun, which is used if you don't specify one */
DBG_INTERFACE SpewRetval_t DefaultSpewFunc( SpewType_t type, const tchar *pMsg );

/* Same as the default spew func, but returns SPEW_ABORT for asserts */
DBG_INTERFACE SpewRetval_t DefaultSpewFuncAbortOnAsserts( SpewType_t type, const tchar *pMsg );

/* Should be called only inside a SpewOutputFunc_t, returns groupname, level, color */
DBG_INTERFACE const tchar* GetSpewOutputGroup( void );
DBG_INTERFACE int GetSpewOutputLevel( void );
DBG_INTERFACE const Color* GetSpewOutputColor( void );

/* Used to manage spew groups and subgroups */
DBG_INTERFACE void   SpewActivate( const tchar* pGroupName, int level );
DBG_INTERFACE bool   IsSpewActive( const tchar* pGroupName, int level );

/* Used to display messages, should never be called directly. */
DBG_INTERFACE void   _SpewInfo( SpewType_t type, const tchar* pFile, int line );
DBG_INTERFACE SpewRetval_t   _SpewMessage( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE SpewRetval_t   _DSpewMessage( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE SpewRetval_t   ColorSpewMessage( SpewType_t type, const Color *pColor, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );

#if !defined( _RETAIL )

/* These are always compiled in */
DBG_INTERFACE void Msg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DMsg( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void MsgV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

DBG_INTERFACE void Warning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DWarning( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void WarningV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

DBG_INTERFACE void Log( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DLog( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void LogV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

#ifdef Error
// p4.cpp does a #define Error Warning and in that case the Error prototype needs to
// be consistent with the Warning prototype.
DBG_INTERFACE void Error( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
#else
DBG_INTERFACE void NORETURN Error( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void NORETURN ErrorV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

#endif

#else

inline void Msg( ... ) {}
inline void DMsg( ... ) {}
inline void MsgV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void Warning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) {}
inline void WarningV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void DWarning( ... ) {}
inline void Log( ... ) {}
inline void DLog( ... ) {}
inline void LogV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void Error( ... ) {}
inline void ErrorV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}

#endif

// You can use this macro like a runtime assert macro.
// If the condition fails, then Error is called with the message. This macro is called
// like AssertMsg, where msg must be enclosed in parenthesis:
//
// ErrorIfNot( bCondition, ("a b c %d %d %d", 1, 2, 3) );
#define ErrorIfNot( condition, msg ) \
	if ( condition )		\
		;					\
	else 					\
	{						\
		Error msg;			\
	}

#if !defined( _RETAIL )

/* A couple of super-common dynamic spew messages, here for convenience */
/* These looked at the "developer" group */
DBG_INTERFACE void DevMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void DevWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void DevLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

/* default level versions (level 1) */
DBG_OVERLOAD void DevMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void DevWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void DevLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* These looked at the "console" group */
DBG_INTERFACE void ConColorMsg( int level, const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void ConMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

/* default console version (level 1) */
DBG_OVERLOAD void ConColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_OVERLOAD void ConMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void ConWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void ConLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* developer console version (level 2) */
DBG_INTERFACE void ConDColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConDMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void ConDWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void ConDLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* These looked at the "network" group */
DBG_INTERFACE void NetMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void NetWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void NetLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

void ValidateSpew( class CValidator &validator );

#else

inline void DevMsg( ... ) {}
inline void DevWarning( ... ) {}
inline void DevLog( ... ) {}
inline void ConMsg( ... ) {}
inline void ConLog( ... ) {}
inline void NetMsg( ... ) {}
inline void NetWarning( ... ) {}
inline void NetLog( ... ) {}

#endif

#endif
