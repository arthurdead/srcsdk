#ifndef PLATFORM_FUNCS_H
#define PLATFORM_FUNCS_H

#pragma once

#include "tier0/platform.h"
#include "hackmgr/hackmgr.h"

//-----------------------------------------------------------------------------
// DLL export for platform utilities
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define PLATFORM_INTERFACE	DLL_EXPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_EXPORT
#define PLATFORM_CLASS		DLL_CLASS_EXPORT
#else
#define PLATFORM_INTERFACE	DLL_IMPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_IMPORT
#define PLATFORM_CLASS		DLL_CLASS_IMPORT
#endif

#else	// BUILD_AS_DLL

#define PLATFORM_INTERFACE	extern
#define PLATFORM_OVERLOAD
#define PLATFORM_CLASS

#endif	// BUILD_AS_DLL

#if HACKMGR_ENGINE_TARGET == HACKMGR_ENGINE_TARGET_SDK2013MP
	#define PLATFORM_INTERFACE_ABI_1 PLATFORM_INTERFACE
#else
	#define PLATFORM_INTERFACE_ABI_1 HACKMGR_API
#endif

#if 0
	#define PLATFORM_INTERFACE_ABI_2 PLATFORM_INTERFACE
	#define PLATFORM_OVERLOAD_ABI_2 PLATFORM_OVERLOAD
#else
	#define PLATFORM_INTERFACE_ABI_2 HACKMGR_API
	#define PLATFORM_OVERLOAD_ABI_2 HACKMGR_OVERLOAD_API
#endif

PLATFORM_INTERFACE bool vtune( bool resume );

// When in benchmark mode, the timer returns a simple incremented value each time you call it.
//
// It should not be changed after startup unless you really know what you're doing. The only place
// that should do this is the benchmark code itself so it can output a legit duration.
PLATFORM_INTERFACE void				Plat_SetBenchmarkMode( bool bBenchmarkMode );	
PLATFORM_INTERFACE bool				Plat_IsInBenchmarkMode();

PLATFORM_INTERFACE_ABI_1 void				Plat_ExitProcess( int nCode );

PLATFORM_INTERFACE double			Plat_FloatTime();		// Returns time in seconds since the module was loaded.
PLATFORM_INTERFACE unsigned int		Plat_MSTime();			// Time in milliseconds.
PLATFORM_INTERFACE char *			Plat_ctime( const time_t *timep, char *buf, size_t bufsize );
PLATFORM_INTERFACE struct tm *		Plat_gmtime( const time_t *timep, struct tm *result );
PLATFORM_INTERFACE time_t			Plat_timegm( struct tm *timeptr );
PLATFORM_INTERFACE struct tm *		Plat_localtime( const time_t *timep, struct tm *result );

// Get the local calendar time.
// Same as time() followed by localtime(), but non-crash-prone and threadsafe.
PLATFORM_INTERFACE void				Plat_GetLocalTime( struct tm *pNow );

// Get a time string (same as ascstring, but threadsafe).
PLATFORM_INTERFACE void				Plat_GetTimeString( struct tm *pTime, char *pOut, int nMaxBytes );

// Processor Information:
struct CPUInformation
{
	int	 m_Size;		// Size of this structure, for forward compatability.

	bool m_bRDTSC : 1,	// Is RDTSC supported?
		 m_bCMOV  : 1,  // Is CMOV supported?
		 m_bFCMOV : 1,  // Is FCMOV supported?
		 m_bSSE	  : 1,	// Is SSE supported?
		 m_bSSE2  : 1,	// Is SSE2 Supported?
		 m_b3DNow : 1,	// Is 3DNow! Supported?
		 m_bMMX   : 1,	// Is MMX supported?
		 m_bHT	  : 1;	// Is HyperThreading supported?

	uint8 m_nLogicalProcessors;		// Number op logical processors.
	uint8 m_nPhysicalProcessors;	// Number of physical processors
	
	bool m_bSSE3 : 1,
		 m_bSSSE3 : 1,
		 m_bSSE4a : 1,
		 m_bSSE41 : 1,
		 m_bSSE42 : 1;	

	int64 m_Speed;						// In cycles per second.

	tchar* m_szProcessorID;				// Processor vendor Identification.

	uint32 m_nModel;
	uint32 m_nFeatures[3];
};

// Have to return a pointer, not a reference, because references are not compatible with the
// extern "C" implied by PLATFORM_INTERFACE.
PLATFORM_INTERFACE const CPUInformation* GetCPUInformation();

PLATFORM_INTERFACE float GetCPUUsage();

PLATFORM_INTERFACE void GetCurrentDate( int *pDay, int *pMonth, int *pYear );

// ---------------------------------------------------------------------------------- //
// Performance Monitoring Events - L2 stats etc...
// ---------------------------------------------------------------------------------- //
PLATFORM_INTERFACE void InitPME();
PLATFORM_INTERFACE void ShutdownPME();

//-----------------------------------------------------------------------------
// Thread related functions
//-----------------------------------------------------------------------------

// Sets a hardware data breakpoint on the given address. Currently Win32-only.
// Specify 1, 2, or 4 bytes for nWatchBytes; pass 0 to unregister the address.
PLATFORM_INTERFACE void	Plat_SetHardwareDataBreakpoint( const void *pAddress, int nWatchBytes, bool bBreakOnRead );

// Apply current hardware data breakpoints to a newly created thread.
PLATFORM_INTERFACE void	Plat_ApplyHardwareDataBreakpointsToNewThread( unsigned long dwThreadID );

//-----------------------------------------------------------------------------
// Process related functions
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE const tchar *Plat_GetCommandLine();
#ifndef _WIN32
// helper function for OS's that don't have a ::GetCommandLine() call
PLATFORM_INTERFACE void Plat_SetCommandLine( const char *cmdLine );
#endif
PLATFORM_INTERFACE const char *Plat_GetCommandLineA();

//-----------------------------------------------------------------------------
// Security related functions
//-----------------------------------------------------------------------------
// Ensure that the hardware key's drivers have been installed.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyDriver();

// Ok, so this isn't a very secure way to verify the hardware key for now.  It
// is primarially depending on the fact that all the binaries have been wrapped
// with the secure wrapper provided by the hardware keys vendor.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKey();

// The same as above, but notifies user with a message box when the key isn't in
// and gives him an opportunity to correct the situation.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyPrompt();

// Can be called in real time, doesn't perform the verify every frame.  Mainly just
// here to allow the game to drop out quickly when the key is removed, rather than
// allowing the wrapper to pop up it's own blocking dialog, which the engine doesn't
// like much.
PLATFORM_INTERFACE bool Plat_FastVerifyHardwareKey();

//-----------------------------------------------------------------------------
// Just logs file and line to simple.log
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void* Plat_SimpleLog( const tchar* file, int line );

//-----------------------------------------------------------------------------
// Returns true if debugger attached, false otherwise
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool Plat_IsInDebugSession();
PLATFORM_INTERFACE void Plat_DebugString( const char * );

//-----------------------------------------------------------------------------
// Message Box
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE_ABI_2 void Plat_MessageBox( const char *pTitle, const tchar *pMessage );

//-----------------------------------------------------------------------------
// Returns true if running on a 64 bit (windows) OS
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool Is64BitOS();

// Watchdog timer support. Call Plat_BeginWatchdogTimer( nn ) to kick the timer off.  if you don't call
// Plat_EndWatchdogTimer within nn seconds, the program will kick off an exception.  This is for making
// sure that hung dedicated servers abort (and restart) instead of staying hung. Calling
// Plat_EndWatchdogTimer more than once or when there is no active watchdog is fine. Only does anything
// under linux right now. It should be possible to implement this functionality in windows via a
// thread, if desired.
PLATFORM_INTERFACE void Plat_BeginWatchdogTimer( int nSecs );
PLATFORM_INTERFACE void Plat_EndWatchdogTimer( void );
PLATFORM_INTERFACE int Plat_GetWatchdogTime( void );

typedef void (*Plat_WatchDogHandlerFunction_t)(void);
PLATFORM_INTERFACE void Plat_SetWatchdogHandlerFunction( Plat_WatchDogHandlerFunction_t function );

//-----------------------------------------------------------------------------
// Include additional dependant header components.
//-----------------------------------------------------------------------------
#include "tier0/fasttimer.h"

#endif
