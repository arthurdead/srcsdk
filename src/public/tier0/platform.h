//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef PLATFORM_H
#define PLATFORM_H

#pragma once

#ifdef __WINE__
#undef WIN32
#undef _WIN32
#undef __WIN32
#undef __WIN32__
#undef __WINNT
#undef __WINNT__
#endif

#if defined(__x86_64__) || defined(_WIN64)
#define PLATFORM_64BITS 1
#endif

#if defined(__GCC__) || defined(__GNUC__)
#define COMPILER_GCC 1
#endif

#ifdef __clang__
#define COMPILER_CLANG 1
#endif

#include "wchartypes.h"
#include "basetypes.h"
#include "tier0/valve_off.h"

#ifdef _DEBUG
#if !defined( PLAT_COMPILE_TIME_ASSERT )
#define PLAT_COMPILE_TIME_ASSERT( pred )	switch(0){case 0:case pred:;}
#endif
#else
#if !defined( PLAT_COMPILE_TIME_ASSERT )
#define PLAT_COMPILE_TIME_ASSERT( pred )
#endif
#endif

// feature enables
#define NEW_SOFTWARE_LIGHTING

#ifdef POSIX
// need this for _alloca
#include <alloca.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#endif

#include <malloc.h>
#include <new>

// need this for memset
#include <string.h>

#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.

#ifdef _WIN32
	#define PLATFORM_WINDOWS 1 // Windows PC or Xbox 360
	#ifndef IS_WINDOWS_PC
	#define IS_WINDOWS_PC
	#endif
	#ifndef PLATFORM_WINDOWS_PC
	#define PLATFORM_WINDOWS_PC 1 // Windows PC
	#endif
	#ifdef _WIN64
		#define PLATFORM_WINDOWS_PC64 1
	#else
		#define PLATFORM_WINDOWS_PC32 1
	#endif
#elif defined(POSIX)

#else
	#error
#endif

typedef unsigned char uint8;
typedef signed char int8;

#if defined( _WIN32 )

	typedef __int16					int16;
	typedef unsigned __int16		uint16;
	typedef __int32					int32;
	typedef unsigned __int32		uint32;
	typedef __int64					int64;
	typedef unsigned __int64		uint64;

	#ifdef PLATFORM_64BITS
		typedef __int64 intp;				// intp is an integer that can accomodate a pointer
		typedef unsigned __int64 uintp;		// (ie, sizeof(intp) >= sizeof(int) && sizeof(intp) >= sizeof(void *)
	#else
		typedef __int32 intp;
		typedef unsigned __int32 uintp;
	#endif

	// Use this to specify that a function is an override of a virtual function.
	// This lets the compiler catch cases where you meant to override a virtual
	// function but you accidentally changed the function signature and created
	// an overloaded function. Usage in function declarations is like this:
	// int GetData() const OVERRIDE;
	#define OVERRIDE override

#else // _WIN32

	typedef short					int16;
	typedef unsigned short			uint16;
	typedef int						int32;
	typedef unsigned int			uint32;
	typedef long long				int64;
	typedef unsigned long long		uint64;
	#ifdef PLATFORM_64BITS
		typedef long long			intp;
		typedef unsigned long long	uintp;
	#else
		typedef int					intp;
		typedef unsigned int		uintp;
	#endif
	struct HWND__;
	typedef HWND__ *HWND;

	// Avoid redefinition warnings if a previous header defines this.
	#undef OVERRIDE
	#if __cplusplus >= 201103L
		#define OVERRIDE override
		#if defined(__clang__)
			// warning: 'override' keyword is a C++11 extension [-Wc++11-extensions]
			// Disabling this warning is less intrusive than enabling C++11 extensions
			#pragma GCC diagnostic ignored "-Wc++11-extensions"
		#endif
	#else
		#define OVERRIDE
	#endif

#endif // else _WIN32

//-----------------------------------------------------------------------------
// Set up platform type defines.
//-----------------------------------------------------------------------------

// From steam/steamtypes.h
// RTime32
// We use this 32 bit time representing real world time.
// It offers 1 second resolution beginning on January 1, 1970 (Unix time)
typedef uint32 RTime32;

typedef float				float32;
typedef double				float64;

// for when we don't care about how many bits we use
typedef unsigned int		uint;

#ifdef _MSC_VER
// Ensure that everybody has the right compiler version installed. The version
// number can be obtained by looking at the compiler output when you type 'cl'
// and removing the last two digits and the periods: 16.00.40219.01 becomes 160040219
#if _MSC_FULL_VER > 180000000
	#if _MSC_FULL_VER < 180030723
		#error You must install VS 2013 Update 3
	#endif
#elif _MSC_FULL_VER > 160000000
	#if _MSC_FULL_VER < 160040219
		#error You must install VS 2010 SP1
	#endif
#else
	#if _MSC_FULL_VER < 140050727
		#error You must install VS 2005 SP1
	#endif
#endif
#endif

// This can be used to ensure the size of pointers to members when declaring
// a pointer type for a class that has only been forward declared
#ifdef _MSC_VER
#define SINGLE_INHERITANCE __single_inheritance
#define MULTIPLE_INHERITANCE __multiple_inheritance
#define VIRTUAL_INHERITANCE __virtual_inheritance
#else
#define SINGLE_INHERITANCE
#define MULTIPLE_INHERITANCE
#define VIRTUAL_INHERITANCE
#endif

//TODO!!! the fuck do i call this?
#ifdef __MINGW32__
#define VIRTUAL_OVERLOAD 
#else
#define VIRTUAL_OVERLOAD virtual
#endif

#ifdef _MSC_VER
#define NO_VTABLE __declspec( novtable )
#else
#define NO_VTABLE
#endif

#ifdef _MSC_VER
	// This indicates that a function never returns, which helps with
	// generating accurate compiler warnings
	#define NORETURN				__declspec( noreturn )
#else
	#define NORETURN __attribute__((__noreturn__))
#endif

// This can be used to declare an abstract (interface only) class.
// Classes marked abstract should not be instantiated.  If they are, and access violation will occur.
//
// Example of use:
//
// abstract_class CFoo
// {
//      ...
// }
//
// MSDN __declspec(novtable) documentation: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/_langref_novtable.asp
//
// Note: NJS: This is not enabled for regular PC, due to not knowing the implications of exporting a class with no no vtable.
//       It's probable that this shouldn't be an issue, but an experiment should be done to verify this.
//
#define abstract_class class


// MSVC CRT uses 0x7fff while gcc uses MAX_INT, leading to mismatches between platforms
// As a result, we pick the least common denominator here.  This should be used anywhere
// you might typically want to use RAND_MAX
#define VALVE_RAND_MAX 0x7fff



/*
FIXME: Enable this when we no longer fear change =)

// need these for the limits
#include <limits.h>
#include <float.h>

// Maximum and minimum representable values
#define  INT8_MAX			SCHAR_MAX
#define  INT16_MAX			SHRT_MAX
#define  INT32_MAX			LONG_MAX
#define  INT64_MAX			(((int64)~0) >> 1)

#define  INT8_MIN			SCHAR_MIN
#define  INT16_MIN			SHRT_MIN
#define  INT32_MIN			LONG_MIN
#define  INT64_MIN			(((int64)1) << 63)

#define  UINT8_MAX			((uint8)~0)
#define  UINT16_MAX			((uint16)~0)
#define  UINT32_MAX			((uint32)~0)
#define  UINT64_MAX			((uint64)~0)

#define  UINT8_MIN			0
#define  UINT16_MIN			0
#define  UINT32_MIN			0
#define  UINT64_MIN			0

#ifndef  UINT_MIN
#define  UINT_MIN			UINT32_MIN
#endif

#define  FLOAT32_MAX		FLT_MAX
#define  FLOAT64_MAX		DBL_MAX

#define  FLOAT32_MIN FLT_MIN
#define  FLOAT64_MIN DBL_MIN
*/

// portability / compiler settings
#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

#elif defined POSIX
#if defined( OSX ) && defined( CARBON_WORKAROUND )
#define DWORD unsigned int
#else
typedef unsigned long DWORD;
#endif
typedef unsigned short WORD;
typedef long int LONG;
struct HINSTANCE__;
typedef HINSTANCE__ * HINSTANCE;
#define _MAX_PATH PATH_MAX
#undef __cdecl

//#define __cdecl __attribute__((__cdecl__))
//stupid dumbass steam developers
#define __cdecl

#define __stdcall __attribute__((__stdcall__))
#define __thiscall __attribute__((__thiscall__))
#define __declspec(...) __attribute__((__VA_ARGS__))

typedef const char* LPCSTR;
typedef char* PSTR, *LPSTR;

#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

typedef DWORD COLORREF;
typedef DWORD* LPCOLORREF;

typedef struct tagRGBQUAD RGBQUAD;

#endif // defined(_WIN32) && !defined(WINDED)


// Defines MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH  260
#endif

#ifdef _WIN32
#define MAX_UNICODE_PATH 32767
#else
#define MAX_UNICODE_PATH MAX_PATH
#endif

#define MAX_UNICODE_PATH_IN_UTF8 MAX_UNICODE_PATH*4

#ifdef GNUC
#undef offsetof
#define offsetof( type, var ) __builtin_offsetof( type, var ) 
#else
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif

#define V_CONCAT2_INTERNAL(x1,x2) x1##x2
#define V_CONCAT2(x1,x2) V_CONCAT2_INTERNAL(x1,x2)

#define V_CONCAT3_INTERNAL(x1,x2,x3) x1##x2##x3
#define V_CONCAT3(x1,x2,x3) V_CONCAT3_INTERNAL(x1,x2,x3)

// Stringify a number
#define V_STRINGIFY_INTERNAL(x) #x
// Extra level of indirection needed when passing in a macro to avoid getting the macro name instead of value
#define V_STRINGIFY(x) V_STRINGIFY_INTERNAL(x)

#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) //  need macro for constant expression

// Used to step into the debugger
#if defined( _WIN32 )
#if !defined _MSC_VER && !defined __MINGW32__
#define __debugbreak() __asm__ __volatile__ ("int $3")
#endif

#define DebuggerBreak()  __debugbreak()
#else
	// On OSX, SIGTRAP doesn't really stop the thread cold when debugging.
	// So if being debugged, use INT3 which is precise.
#ifdef OSX
#define DebuggerBreak()  if ( Plat_IsInDebugSession() ) { __asm ( "int $3" ); } else { raise(SIGTRAP); }
#else
//#define DebuggerBreak()  raise(SIGTRAP)
#define DebuggerBreak() __asm__ __volatile__ ("int $3");
#endif
#endif
#define	DebuggerBreakIfDebugging() if ( !Plat_IsInDebugSession() ) ; else DebuggerBreak()

#ifdef STAGING_ONLY
#define	DebuggerBreakIfDebugging_StagingOnly() if ( !Plat_IsInDebugSession() ) ; else DebuggerBreak()
#else
#define	DebuggerBreakIfDebugging_StagingOnly()
#endif

// Allows you to specify code that should only execute if we are in a staging build. Otherwise the code noops.
#ifdef STAGING_ONLY
#define STAGING_ONLY_EXEC( _exec ) do { _exec; } while (0)
#else
#define STAGING_ONLY_EXEC( _exec ) do { } while (0)
#endif

// C functions for external declarations that call the appropriate C++ methods
#ifndef EXPORT
	#if defined _WIN32 && !defined GNUC
		#define EXPORT	__declspec( dllexport )
	#else
		#define EXPORT	__attribute__ ((visibility("default")))
	#endif
#endif

#if defined __i386__ && !defined __linux__
	#define id386	1
#else
	#define id386	0
#endif  // __i386__

// decls for aligning data
#if defined _WIN32 && !defined GNUC
        #define DECL_ALIGN(x) __declspec(align(x))

#elif GNUC
	#define DECL_ALIGN(x) __attribute__((aligned(x)))
#else
        #define DECL_ALIGN(x) /* */
#endif

#ifdef _MSC_VER
// MSVC has the align at the start of the struct
#define ALIGN1 DECL_ALIGN(1)
#define ALIGN2 DECL_ALIGN(2)
#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#define ALIGN1_POST
#define ALIGN2_POST
#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#elif defined( GNUC )
// gnuc has the align decoration at the end
#define ALIGN1
#define ALIGN2
#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128

#define ALIGN1_POST DECL_ALIGN(1)
#define ALIGN2_POST DECL_ALIGN(2)
#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#else
#error
#endif

// Pull in the /analyze code annotations.
#include "annotations.h"

//-----------------------------------------------------------------------------
// Convert int<-->pointer, avoiding 32/64-bit compiler warnings:
//-----------------------------------------------------------------------------
#define INT_TO_POINTER( i ) (void *)( ( i ) + (char *)NULL )
#define POINTER_TO_INT( p ) ( (int)(uintp)( p ) )


//-----------------------------------------------------------------------------
// Stack-based allocation related helpers
//-----------------------------------------------------------------------------
#if defined( GNUC )
	#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )
#ifdef _LINUX
	#define mallocsize( _p )	( malloc_usable_size( _p ) )
#elif defined(OSX)
	#define mallocsize( _p )	( malloc_size( _p ) )
#elif defined _WIN32
	#define mallocsize( _p )		( _msize( _p ) )
#else
#error
#endif
#elif defined ( _WIN32 )
	#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
	#define mallocsize( _p )		( _msize( _p ) )
#endif

#define  stackfree( _p )			0

// Linux had a few areas where it didn't construct objects in the same order that Windows does.
// So when CVProfile::CVProfile() would access g_pMemAlloc, it would crash because the allocator wasn't initalized yet.
#ifdef GNUC
	#define INIT_PRIORITY(...) __attribute__((init_priority((__VA_ARGS__))))
#else
	#define INIT_PRIORITY(...)
#endif

#if defined(_MSC_VER)
	#define SELECTANY __declspec(selectany)
	#define RESTRICT __restrict
	#define RESTRICT_FUNC __declspec(restrict)
	#define FMTFUNCTION( a, b )
#elif defined(GNUC)
	#define SELECTANY __attribute__((weak))
	#define RESTRICT __restrict
	#define RESTRICT_FUNC
	// squirrel.h does a #define printf DevMsg which leads to warnings when we try
	// to use printf as the prototype format function. Using __printf__ instead.
	#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( __printf__, fmtargnumber, firstvarargnumber )))
#else
	#define SELECTANY static
	#define RESTRICT
	#define RESTRICT_FUNC
	#define FMTFUNCTION( a, b )
#endif

#if defined( _WIN32 ) && !defined GNUC
#define DLL_EXPORT_ATTR __declspec( dllexport )
#define DLL_IMPORT_ATTR __declspec( dllimport )
#elif defined GNUC && !defined __linux__
#define DLL_EXPORT_ATTR __attribute__ ((visibility("default"),dllexport))
#define DLL_IMPORT_ATTR __attribute__ ((visibility("default"),dllimport))
#elif defined GNUC && defined __linux__
#define DLL_EXPORT_ATTR __attribute__ ((visibility("default")))
#define DLL_IMPORT_ATTR __attribute__ ((visibility("default")))
#endif

#ifdef GNUC
#define SYMALIAS(name) __attribute__ ((alias(name)))
#else
#define SYMALIAS(name) 
#endif

#if defined( _WIN32 ) && !defined GNUC

// Used for dll exporting and importing
#define DLL_EXPORT				extern "C" __declspec( dllexport )
#define DLL_IMPORT				extern "C" __declspec( dllimport )

// Can't use extern "C" when DLL exporting a class
#define DLL_CLASS_EXPORT		__declspec( dllexport )
#define DLL_CLASS_IMPORT		__declspec( dllimport )

// Can't use extern "C" when DLL exporting a global
#define DLL_GLOBAL_EXPORT		extern __declspec( dllexport )
#define DLL_GLOBAL_IMPORT		extern __declspec( dllimport )

#define LIB_EXPORT				extern "C" 
#define LIB_IMPORT				extern "C" 

#define LIB_CLASS_EXPORT		
#define LIB_CLASS_IMPORT		

#define LIB_GLOBAL_EXPORT		extern 
#define LIB_GLOBAL_IMPORT		extern 

#define DLL_LOCAL

#define LIB_LOCAL

#elif defined( _WIN32 ) && defined GNUC

// Used for dll exporting and importing
#define  DLL_EXPORT   extern "C" __attribute__ ((visibility("default"),dllexport))
#define  DLL_IMPORT   extern "C" __attribute__ ((visibility("default"),dllimport))

// Can't use extern "C" when DLL exporting a class
#define  DLL_CLASS_EXPORT __attribute__ ((visibility("default"),dllexport))
#define  DLL_CLASS_IMPORT __attribute__ ((visibility("default"),dllimport))

// Can't use extern "C" when DLL exporting a global
#define  DLL_GLOBAL_EXPORT   extern __attribute__ ((visibility("default"), dllexport))
#define  DLL_GLOBAL_IMPORT   extern __attribute__ ((visibility("default"), dllimport))

#define  LIB_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  LIB_IMPORT   extern "C" __attribute__ ((visibility("default")))

#define  LIB_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  LIB_CLASS_IMPORT __attribute__ ((visibility("default")))

#define  LIB_GLOBAL_EXPORT   extern __attribute__ ((visibility("default")))
#define  LIB_GLOBAL_IMPORT   extern __attribute__ ((visibility("default")))

#define  DLL_LOCAL __attribute__ ((visibility("hidden")))

#define  LIB_LOCAL __attribute__ ((visibility("hidden")))

#elif defined GNUC
// Used for dll exporting and importing
#define  DLL_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  DLL_IMPORT   extern "C" __attribute__ ((visibility("default")))

// Can't use extern "C" when DLL exporting a class
#define  DLL_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  DLL_CLASS_IMPORT __attribute__ ((visibility("default")))

// Can't use extern "C" when DLL exporting a global
#define  DLL_GLOBAL_EXPORT   extern __attribute__ ((visibility("default")))
#define  DLL_GLOBAL_IMPORT   extern __attribute__ ((visibility("default")))

#define  LIB_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  LIB_IMPORT   extern "C" __attribute__ ((visibility("default")))

#define  LIB_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  LIB_CLASS_IMPORT __attribute__ ((visibility("default")))

#define  LIB_GLOBAL_EXPORT   extern __attribute__ ((visibility("default")))
#define  LIB_GLOBAL_IMPORT   extern __attribute__ ((visibility("default")))

#define  DLL_LOCAL __attribute__ ((visibility("hidden")))

#define  LIB_LOCAL __attribute__ ((visibility("hidden")))

#else
#error "Unsupported Platform."
#endif

// Used for standard calling conventions
#if defined( _WIN32 ) && !defined GNUC
	#undef CDECL
	#define  CDECL				__cdecl
	#define  STDCALL				__stdcall
	#define  FASTCALL				__fastcall
	#define  THISCALL				__thiscall
	#define  FORCEINLINE			__forceinline
	// GCC 3.4.1 has a bug in supporting forced inline of templated functions
	// this macro lets us not force inlining in that case
	#define  FORCEINLINE_TEMPLATE		__forceinline
#else
	#undef CDECL
	#define  CDECL __attribute__((__cdecl__))
	#define  STDCALL __attribute__((__stdcall__))
	#undef FASTCALL
	#define  FASTCALL __attribute__((__fastcall__))
	#define  THISCALL __attribute__((__thiscall__))
	#undef FORCEINLINE
	#ifdef _LINUX_DEBUGGABLE
		#define  FORCEINLINE
	#else
		#define  FORCEINLINE inline __attribute__ ((always_inline))
	#endif
	// GCC 3.4.1 has a bug in supporting forced inline of templated functions
	// this macro lets us not force inlining in that case
	#define FORCEINLINE_TEMPLATE	inline
//	#define  __stdcall			__attribute__ ((__stdcall__))
#endif

// Force a function call site -not- to inlined. (useful for profiling)
#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))

// Pass hints to the compiler to prevent it from generating unnessecary / stupid code
// in certain situations.  Several compilers other than MSVC also have an equivilent
// construct.
//
// Essentially the 'Hint' is that the condition specified is assumed to be true at
// that point in the compilation.  If '0' is passed, then the compiler assumes that
// any subsequent code in the same 'basic block' is unreachable, and thus usually
// removed.
#ifdef _MSC_VER
	#define HINT(THE_HINT)	__assume((THE_HINT))
#else
	#define HINT(THE_HINT)	__attribute__((__assume__((THE_HINT))))
#endif

// Marks the codepath from here until the next branch entry point as unreachable,
// and asserts if any attempt is made to execute it.
#ifdef GNUC
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE() { Assert(0); HINT(0); }
#endif

// In cases where no default is present or appropriate, this causes MSVC to generate
// as little code as possible, and throw an assertion in debug.
#define NO_DEFAULT default: UNREACHABLE();


#ifdef _WIN32

// Remove warnings from warning level 4.
#pragma warning(disable : 4514) // warning C4514: 'acosl' : unreferenced inline function has been removed
#pragma warning(disable : 4100) // warning C4100: 'hwnd' : unreferenced formal parameter
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4512) // warning C4512: 'InFileRIFF' : assignment operator could not be generated
#pragma warning(disable : 4611) // warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable : 4710) // warning C4710: function 'x' not inlined
#pragma warning(disable : 4702) // warning C4702: unreachable code
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4239) // nonstandard extension used : 'argument' ( conversion from class Vector to class Vector& )
#pragma warning(disable : 4097) // typedef-name 'BaseClass' used as synonym for class-name 'CFlexCycler::CBaseFlex'
#pragma warning(disable : 4324) // Padding was added at the end of a structure
#pragma warning(disable : 4244) // type conversion warning.
#pragma warning(disable : 4305)	// truncation from 'const double ' to 'float '
#pragma warning(disable : 4786)	// Disable warnings about long symbol names
#pragma warning(disable : 4250) // 'X' : inherits 'Y::Z' via dominance
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4481) // warning C4481: nonstandard extension used: override specifier 'override'
#pragma warning(disable : 4748) // warning C4748: /GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function

#if _MSC_VER >= 1300
#pragma warning(disable : 4511)	// Disable warnings about private copy constructors
#pragma warning(disable : 4121)	// warning C4121: 'symbol' : alignment of a member was sensitive to packing
#pragma warning(disable : 4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc (disabled due to std headers having exception syntax)
#endif

#if _MSC_VER >= 1400
#pragma warning(disable : 4996)	// functions declared deprecated
#endif


#endif // _WIN32

#if defined( LINUX ) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
  // based on some Jonathan Wakely macros on the net...
  #define GCC_DIAG_STR(s) #s
  #define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
  #define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
  #define GCC_DIAG_PRAGMA(x)	GCC_DIAG_DO_PRAGMA(GCC diagnostic x)

  #define GCC_DIAG_PUSH_OFF(x)	GCC_DIAG_PRAGMA(push) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
  #define GCC_DIAG_POP()		GCC_DIAG_PRAGMA(pop)
#else
  #define GCC_DIAG_PUSH_OFF(x)
  #define GCC_DIAG_POP()
#endif

#ifdef LINUX
#pragma GCC diagnostic ignored "-Wconversion-null"	// passing NULL to non-pointer argument 1
#pragma GCC diagnostic ignored "-Wpointer-arith"	// NULL used in arithmetic. Ie, vpanel == NULL where VPANEL is uint.
#pragma GCC diagnostic ignored "-Wswitch"				// enumeration values not handled in switch
#endif

#ifdef OSX
#pragma GCC diagnostic ignored "-Wconversion-null"			// passing NULL to non-pointer argument 1
#pragma GCC diagnostic ignored "-Wnull-arithmetic"			// NULL used in arithmetic. Ie, vpanel == NULL where VPANEL is uint.
#pragma GCC diagnostic ignored "-Wswitch-enum"				// enumeration values not handled in switch
#pragma GCC diagnostic ignored "-Wswitch"					// enumeration values not handled in switch
#endif


// When we port to 64 bit, we'll have to resolve the int, ptr vs size_t 32/64 bit problems...
#if !defined( _WIN64 )
#pragma warning( disable : 4267 )	// conversion from 'size_t' to 'int', possible loss of data
#pragma warning( disable : 4311 )	// pointer truncation from 'char *' to 'int'
#pragma warning( disable : 4312 )	// conversion from 'unsigned int' to 'memhandle_t' of greater size
#endif


#ifdef POSIX
#define _stricmp stricmp
#define strcmpi stricmp
#define stricmp strcasecmp
#define _vsnprintf vsnprintf
#define _alloca alloca
#ifdef _snprintf
#undef _snprintf
#endif
#define _snprintf snprintf
//#define GetProcAddress dlsym
#define _chdir chdir
#define _strnicmp strnicmp
#define strnicmp strncasecmp
#define _getcwd getcwd
#define _snwprintf swprintf
#define swprintf_s swprintf
#define wcsicmp _wcsicmp
#define _wcsicmp wcscmp
#define _finite finite
#define _tempnam tempnam
#define _unlink unlink
#define _access access
#define _mkdir(dir) mkdir( dir, S_IRWXU | S_IRWXG | S_IRWXO )
#define _wtoi(arg) wcstol(arg, NULL, 10)
#define _wtoi64(arg) wcstoll(arg, NULL, 10)

struct HINSTANCE__;
typedef HINSTANCE__ * HINSTANCE;

typedef HINSTANCE HMODULE;
typedef void *HANDLE;
#endif

#define fsel(c,x,y) ( (c) >= 0 ? (x) : (y) )

// integer conditional move
// if a >= 0, return x, else y
#define isel(a,x,y) ( ((a) >= 0) ? (x) : (y) )

// if x = y, return a, else b
#define ieqsel(x,y,a,b) (( (x) == (y) ) ? (a) : (b))

// if the nth bit of a is set (counting with 0 = LSB),
// return x, else y
// this is fast if nbit is a compile-time immediate 
#define ibitsel(a, nbit, x, y) ( ( ((a) & (1 << (nbit))) != 0 ) ? (x) : (y) )

//-----------------------------------------------------------------------------
// FP exception handling
//-----------------------------------------------------------------------------
//#define CHECK_FLOAT_EXCEPTIONS		1

#if defined( _MSC_VER )

	#if defined( PLATFORM_WINDOWS_PC64 )
		inline void SetupFPUControlWord()
		{
		}
	#else
		inline void SetupFPUControlWordForceExceptions()
		{
			// use local to get and store control word
			uint16 tmpCtrlW;
			__asm
			{
				fnclex						/* clear all current exceptions */
				fnstcw word ptr [tmpCtrlW]	/* get current control word */
				and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
				or [tmpCtrlW], 0230h		/* set to 53-bit, mask only inexact, underflow */
				fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
			}
		}

		#ifdef CHECK_FLOAT_EXCEPTIONS

			inline void SetupFPUControlWord()
			{
				SetupFPUControlWordForceExceptions();
			}

		#else

			inline void SetupFPUControlWord()
			{
				// use local to get and store control word
				uint16 tmpCtrlW;
				__asm
				{
					fnstcw word ptr [tmpCtrlW]	/* get current control word */
					and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
					or [tmpCtrlW], 023Fh		/* set to 53-bit, mask only inexact, underflow */
					fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
				}
			}

		#endif
	#endif

#else

	inline void SetupFPUControlWord()
	{
		__volatile unsigned short int __cw;
		__asm __volatile ("fnstcw %0" : "=m" (__cw));
		__cw = __cw & 0x0FCC0;	// keep infinity control, keep rounding mode
		__cw = __cw | 0x023F;	// set 53-bit, no exceptions
		__asm __volatile ("fldcw %0" : : "m" (__cw));
	}

#endif // _MSC_VER

//-----------------------------------------------------------------------------
// Purpose: Standard functions for handling endian-ness
//-----------------------------------------------------------------------------

//-------------------------------------
// Basic swaps
//-------------------------------------

template <typename T>
inline T WordSwapC( T w )
{
   uint16 temp;

   temp  = ((*((uint16 *)&w) & 0xff00) >> 8);
   temp |= ((*((uint16 *)&w) & 0x00ff) << 8);

   return *((T*)&temp);
}

template <typename T>
inline T DWordSwapC( T dw )
{
   uint32 temp;

   temp  =   *((uint32 *)&dw) 				>> 24;
   temp |= ((*((uint32 *)&dw) & 0x00FF0000) >> 8);
   temp |= ((*((uint32 *)&dw) & 0x0000FF00) << 8);
   temp |= ((*((uint32 *)&dw) & 0x000000FF) << 24);

   return *((T*)&temp);
}

template <typename T>
inline T QWordSwapC( T dw )
{
	// Assert sizes passed to this are already correct, otherwise
	// the cast to uint64 * below is unsafe and may have wrong results 
	// or even crash.
	PLAT_COMPILE_TIME_ASSERT( sizeof( dw ) == sizeof(uint64) );

	uint64 temp;

	temp  =   *((uint64 *)&dw) 				         >> 56;
	temp |= ((*((uint64 *)&dw) & 0x00FF000000000000ull) >> 40);
	temp |= ((*((uint64 *)&dw) & 0x0000FF0000000000ull) >> 24);
	temp |= ((*((uint64 *)&dw) & 0x000000FF00000000ull) >> 8);
	temp |= ((*((uint64 *)&dw) & 0x00000000FF000000ull) << 8);
	temp |= ((*((uint64 *)&dw) & 0x0000000000FF0000ull) << 24);
	temp |= ((*((uint64 *)&dw) & 0x000000000000FF00ull) << 40);
	temp |= ((*((uint64 *)&dw) & 0x00000000000000FFull) << 56);

	return *((T*)&temp);
}

//-------------------------------------
// Fast swaps
//-------------------------------------

#if defined( _MSC_VER ) && !defined( PLATFORM_WINDOWS_PC64 )

	#define WordSwap  WordSwapAsm
	#define DWordSwap DWordSwapAsm

	#pragma warning(push)
	#pragma warning (disable:4035) // no return value

	template <typename T>
	inline T WordSwapAsm( T w )
	{
	   __asm
	   {
		  mov ax, w
		  xchg al, ah
	   }
	}

	template <typename T>
	inline T DWordSwapAsm( T dw )
	{
	   __asm
	   {
		  mov eax, dw
		  bswap eax
	   }
	}

	#pragma warning(pop)

#else

	#define WordSwap  WordSwapC
	#define DWordSwap DWordSwapC

#endif

// No ASM implementation for this yet
#define QWordSwap QWordSwapC

//-------------------------------------
// The typically used methods.
//-------------------------------------

#if defined(__i386__) && !defined(VALVE_LITTLE_ENDIAN)
#define VALVE_LITTLE_ENDIAN 1
#endif

// If a swapped float passes through the fpu, the bytes may get changed.
// Prevent this by swapping floats as DWORDs.
#define SafeSwapFloat( pOut, pIn )	(*((uint*)pOut) = DWordSwap( *((uint*)pIn) ))

#if defined(VALVE_LITTLE_ENDIAN)

#define BigShort( val )				WordSwap( val )
#define BigWord( val )				WordSwap( val )
#define BigLong( val )				DWordSwap( val )
#define BigDWord( val )				DWordSwap( val )
#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define LittleQWord( val )			( val )
#define SwapShort( val )			BigShort( val )
#define SwapWord( val )				BigWord( val )
#define SwapLong( val )				BigLong( val )
#define SwapDWord( val )			BigDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )

#elif defined(VALVE_BIG_ENDIAN)

#define BigShort( val )				( val )
#define BigWord( val )				( val )
#define BigLong( val )				( val )
#define BigDWord( val )				( val )
#define LittleShort( val )			WordSwap( val )
#define LittleWord( val )			WordSwap( val )
#define LittleLong( val )			DWordSwap( val )
#define LittleDWord( val )			DWordSwap( val )
#define LittleQWord( val )			QWordSwap( val )
#define SwapShort( val )			LittleShort( val )
#define SwapWord( val )				LittleWord( val )
#define SwapLong( val )				LittleLong( val )
#define SwapDWord( val )			LittleDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else

// @Note (toml 05-02-02): this technique expects the compiler to
// optimize the expression and eliminate the other path. On any new
// platform/compiler this should be tested.
inline short BigShort( short val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline uint16 BigWord( uint16 val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline long BigLong( long val )			{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline uint32 BigDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline short LittleShort( short val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline uint16 LittleWord( uint16 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline long LittleLong( long val )		{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline uint32 LittleDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline uint64 LittleQWord( uint64 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : QWordSwap( val ); }
inline short SwapShort( short val )					{ return WordSwap( val ); }
inline uint16 SwapWord( uint16 val )				{ return WordSwap( val ); }
inline long SwapLong( long val )					{ return DWordSwap( val ); }
inline uint32 SwapDWord( uint32 val )				{ return DWordSwap( val ); }

// Pass floats by pointer for swapping to avoid truncation in the fpu
inline void BigFloat( float *pOut, const float *pIn )		{ int test = 1; ( *(char *)&test == 1 ) ? SafeSwapFloat( pOut, pIn ) : ( *pOut = *pIn ); }
inline void LittleFloat( float *pOut, const float *pIn )	{ int test = 1; ( *(char *)&test == 1 ) ? ( *pOut = *pIn ) : SafeSwapFloat( pOut, pIn ); }
inline void SwapFloat( float *pOut, const float *pIn )		{ SafeSwapFloat( pOut, pIn ); }

#endif

FORCEINLINE unsigned long LoadLittleDWord( const unsigned long *base, unsigned int dwordIndex )
{
	return LittleDWord( base[dwordIndex] );
}

FORCEINLINE void StoreLittleDWord( unsigned long *base, unsigned int dwordIndex, unsigned long dword )
{
	base[dwordIndex] = LittleDWord(dword);
}

#if defined( _WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	extern "C" unsigned __int64 __rdtsc();
	#pragma intrinsic(__rdtsc)
#endif

inline uint64 Plat_Rdtsc()
{
#if defined( _WIN64 )
	return ( uint64 )__rdtsc();
#elif defined( _WIN32 ) && !defined GNUC
  #if defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	return ( uint64 )__rdtsc();
  #else
    __asm rdtsc;
	__asm ret;
  #endif
#elif defined( __i386__ )
	uint64 val;
	__asm__ __volatile__ ( "rdtsc" : "=A" (val) );
	return val;
#elif defined( __x86_64__ )
	uint32 lo, hi;
	__asm__ __volatile__ ( "rdtsc" : "=a" (lo), "=d" (hi));
	return ( ( ( uint64 )hi ) << 32 ) | lo;
#else
	#error
#endif
}

// b/w compatibility
#define Sys_FloatTime Plat_FloatTime

// Protect against bad auto operator=
#define DISALLOW_OPERATOR_EQUAL( _classname )			\
	private:											\
		_classname &operator=( const _classname & );	\
	public:

// Define a reasonable operator=
#define IMPLEMENT_OPERATOR_EQUAL( _classname )			\
	public:												\
		_classname &operator=( const _classname &src )	\
		{												\
			memcpy( (void *)this, (void *)&src, sizeof(_classname) );	\
			return *this;								\
		}

#define UNORDEREDENUM_OPERATORS_BASE( Enum, Type ) \
	inline bool operator==( Enum lhs, Enum rhs ) \
	{ return (Type)lhs != (Type)rhs; } \
	inline bool operator!=( Enum lhs, Enum rhs ) \
	{ return (Type)lhs != (Type)rhs; } \
	bool operator==( Enum, Type ) = delete; \
	bool operator!=( Enum, Type ) = delete; \
	bool operator>( Enum, Type ) = delete; \
	bool operator>=( Enum, Type ) = delete; \
	bool operator<( Enum, Type ) = delete; \
	bool operator<=( Enum, Type ) = delete; \
	bool operator==( Type, Enum ) = delete; \
	bool operator!=( Type, Enum ) = delete; \
	bool operator>( Type, Enum ) = delete; \
	bool operator>=( Type, Enum ) = delete; \
	bool operator<( Type, Enum ) = delete; \
	bool operator<=( Type, Enum ) = delete; \
	Enum operator+( Type, Enum ) = delete; \
	Enum operator-( Type, Enum ) = delete; \
	Enum operator*( Type, Enum ) = delete; \
	Enum operator/( Type, Enum ) = delete; \
	Enum operator+( Enum, Type ) = delete; \
	Enum operator-( Enum, Type ) = delete; \
	Enum operator*( Enum, Type ) = delete; \
	Enum operator/( Enum, Type ) = delete; \
	Enum operator+=( Type &, Enum ) = delete; \
	Enum operator-=( Type &, Enum ) = delete; \
	Enum operator*=( Type &, Enum ) = delete; \
	Enum operator/=( Type &, Enum ) = delete; \
	Enum operator+=( Enum &, Type ) = delete; \
	Enum operator-=( Enum &, Type ) = delete; \
	Enum operator*=( Enum &, Type ) = delete; \
	Enum operator/=( Enum &, Type ) = delete; \
	Enum operator-( Enum ) = delete; \
	Enum operator+( Enum ) = delete; \
	Enum operator&( Enum, Type ) = delete; \
	Enum operator&=( Enum &, Type ) = delete; \
	Enum operator|( Enum, Type ) = delete; \
	Enum operator|=( Enum &, Type ) = delete; \
	Enum operator^( Enum, Type ) = delete; \
	Enum operator^=( Enum &, Type ) = delete; \
	Enum operator&( Type, Enum ) = delete; \
	Enum operator&=( Type &, Enum ) = delete; \
	Enum operator|( Type, Enum ) = delete; \
	Enum operator|=( Type &, Enum ) = delete; \
	Enum operator^( Type, Enum ) = delete; \
	Enum operator^=( Type &, Enum ) = delete;

#define UNORDEREDENUM_OPERATORS( Enum, Type ) \
	UNORDEREDENUM_OPERATORS_BASE( Enum, Type ) \
	Enum operator&( Enum, Enum ) = delete; \
	Enum operator&=( Enum &, Enum ) = delete; \
	Enum operator|( Enum, Enum ) = delete; \
	Enum operator|=( Enum &, Enum ) = delete; \
	Enum operator^( Enum, Enum ) = delete; \
	Enum operator^=( Enum &, Enum ) = delete; \
	Enum operator~( Enum ) = delete; \
	Enum operator<<( Enum, Enum ) = delete; \
	Enum operator<<=( Enum &, Enum ) = delete; \
	Enum operator>>( Enum, Enum ) = delete; \
	Enum operator>>=( Enum &, Enum ) = delete;

#define FLAGENUM_OPERATORS( Enum, Type ) \
	UNORDEREDENUM_OPERATORS_BASE( Enum, Type ) \
	inline Enum operator&( Enum lhs, Enum rhs ) \
	{ return (Enum)((Type)lhs & (Type)rhs); } \
	inline Enum &operator&=( Enum &lhs, Enum rhs ) \
	{ lhs = operator&(lhs, rhs); return lhs; } \
	inline Enum operator|( Enum lhs, Enum rhs ) \
	{ return (Enum)((Type)lhs | (Type)rhs); } \
	inline Enum &operator|=( Enum &lhs, Enum rhs ) \
	{ lhs = operator|(lhs, rhs); return lhs; } \
	inline Enum operator^( Enum lhs, Enum rhs ) \
	{ return (Enum)((Type)lhs ^ (Type)rhs); } \
	inline Enum &operator^=( Enum &lhs, Enum rhs ) \
	{ lhs = operator^(lhs, rhs); return lhs; } \
	inline Enum operator~( Enum lhs ) \
	{ return (Enum)(~(Type)lhs); } \
	inline Enum operator<<( Enum lhs, Enum rhs ) \
	{ return (Enum)((Type)lhs << (Type)rhs); } \
	inline Enum &operator<<=( Enum &lhs, Enum rhs ) \
	{ lhs = operator<<(lhs, rhs); return lhs; } \
	inline Enum operator>>( Enum lhs, Enum rhs ) \
	{ return (Enum)((Type)lhs << (Type)rhs); } \
	inline Enum &operator>>=( Enum &lhs, Enum rhs ) \
	{ lhs = operator>>(lhs, rhs); return lhs; }

#define Plat_FastMemset memset
#define Plat_FastMemcpy memcpy

//-----------------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------------

namespace valve_type_traits
{
	template <typename T>
	struct rem_ref;

	template <typename T>
	struct rem_ref
	{
		using type = T;
	};

	template <typename T>
	struct rem_ref<T &>
	{
		using type = T;
	};

	template <typename T>
	struct rem_ref<T &&>
	{
		using type = T;
	};

	template <typename T>
	using rem_ref_t = typename rem_ref<T>::type;

	template <typename T>
	struct rem_const;

	template <typename T>
	struct rem_const
	{
		using type = T;
	};

	template <typename T>
	struct rem_const<const T>
	{
		using type = T;
	};
	template <typename T>
	using rem_const_t = typename rem_const<T>::type;
}

template <typename T>
struct type_identity
{
};

template <typename T> struct is_integral_ { static constexpr inline const bool value = false; };
template<> struct is_integral_<bool> { static constexpr inline const bool value = true; };
template<> struct is_integral_<char> { static constexpr inline const bool value = true; };
template<> struct is_integral_<signed char> { static constexpr inline const bool value = true; };
template<> struct is_integral_<unsigned char> { static constexpr inline const bool value = true; };
template<> struct is_integral_<short> { static constexpr inline const bool value = true; };
template<> struct is_integral_<unsigned short> { static constexpr inline const bool value = true; };
template<> struct is_integral_<int> { static constexpr inline const bool value = true; };
template<> struct is_integral_<unsigned int> { static constexpr inline const bool value = true; };
template<> struct is_integral_<long> { static constexpr inline const bool value = true; };
template<> struct is_integral_<unsigned long> { static constexpr inline const bool value = true; };
template<> struct is_integral_<long long> { static constexpr inline const bool value = true; };
template<> struct is_integral_<unsigned long long> { static constexpr inline const bool value = true; };
template <typename T>
constexpr inline const bool is_integral_v = is_integral_<T>::value;

template <typename T> struct is_unsigned_ { static constexpr inline const bool value = false; };
template<> struct is_unsigned_<bool> { static constexpr inline const bool value = true; };
template<> struct is_unsigned_<unsigned char> { static constexpr inline const bool value = true; };
template<> struct is_unsigned_<unsigned short> { static constexpr inline const bool value = true; };
template<> struct is_unsigned_<unsigned int> { static constexpr inline const bool value = true; };
template<> struct is_unsigned_<unsigned long> { static constexpr inline const bool value = true; };
template<> struct is_unsigned_<unsigned long long> { static constexpr inline const bool value = true; };
template <typename T>
constexpr inline const bool is_unsigned_v = is_unsigned_<T>::value;

template <typename T> struct is_floating_point_ { static constexpr inline const bool value = false; };
template<> struct is_floating_point_<float> { static constexpr inline const bool value = true; };
template<> struct is_floating_point_<double> { static constexpr inline const bool value = true; };
template<> struct is_floating_point_<long double> { static constexpr inline const bool value = true; };
template <typename T>
constexpr inline const bool is_floating_point_v = is_floating_point_<T>::value;

template <typename T, typename U> struct is_same_ { static constexpr inline const bool value = false; };
template<typename T> struct is_same_<T, T> { static constexpr inline const bool value = true; };
template <typename T, typename U>
constexpr inline const bool is_same_v = is_same_<T, U>::value;

template <typename T>
inline valve_type_traits::rem_ref_t<T> &&Move(T &&x)
{ return static_cast<valve_type_traits::rem_ref_t<T> &&>(x); }

template <class T>
inline T* Construct( T* pMemory )
{
	return ::new( pMemory ) T;
}

template <class T, typename ARG1>
inline T* Construct( T* pMemory, ARG1 a1 )
{
	return ::new( pMemory ) T( a1 );
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2 )
{
	return ::new( pMemory ) T( a1, a2 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3 )
{
	return ::new( pMemory ) T( a1, a2, a3 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4 )
{
	return ::new( pMemory ) T( a1, a2, a3, a4 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5 )
{
	return ::new( pMemory ) T( a1, a2, a3, a4, a5 );
}

template <class T, class P>
inline void ConstructOneArg( T* pMemory, P const& arg)
{
	::new( pMemory ) T(arg);
}

template <class T, class P>
inline void ConstructOneArg( T* pMemory, P && arg)
{
	::new( pMemory ) T(Move(arg));
}

template <class T, class P1, class P2 >
inline void ConstructTwoArg( T* pMemory, P1 const& arg1, P2 const& arg2)
{
	::new( pMemory ) T(arg1, arg2);
}

template <class T, class P1, class P2, class P3 >
inline void ConstructThreeArg( T* pMemory, P1 const& arg1, P2 const& arg2, P3 const& arg3)
{
	::new( pMemory ) T(arg1, arg2, arg3);
}

template <class T>
inline T* CopyConstruct( T* pMemory, T const& src )
{
	return ::new( pMemory ) T(src);
}

template <class T>
inline T* MoveConstruct( T* pMemory, T && src )
{
	return ::new( pMemory ) T(Move(src));
}

template <class T>
inline void Destruct( T* pMemory )
{
	pMemory->~T();

#ifdef _DEBUG
	memset( reinterpret_cast<void*>( pMemory ), 0xDD, sizeof(T) );
#endif
}


//
// GET_OUTER()
//
// A platform-independent way for a contained class to get a pointer to its
// owner. If you know a class is exclusively used in the context of some
// "outer" class, this is a much more space efficient way to get at the outer
// class than having the inner class store a pointer to it.
//
//	class COuter
//	{
//		class CInner // Note: this does not need to be a nested class to work
//		{
//			void PrintAddressOfOuter()
//			{
//				printf( "Outer is at 0x%x\n", GET_OUTER( COuter, m_Inner ) );
//			}
//		};
//
//		CInner m_Inner;
//		friend class CInner;
//	};

#define GET_OUTER( OuterType, OuterMember ) \
   ( ( OuterType * ) ( (uint8 *)this - offsetof( OuterType, OuterMember ) ) )

/*	TEMPLATE_FUNCTION_TABLE()

    (Note added to platform.h so platforms that correctly support templated
	 functions can handle portions as templated functions rather than wrapped
	 functions)

	Helps automate the process of creating an array of function
	templates that are all specialized by a single integer.
	This sort of thing is often useful in optimization work.

	For example, using TEMPLATE_FUNCTION_TABLE, this:

	TEMPLATE_FUNCTION_TABLE(int, Function, ( int blah, int blah ), 10)
	{
		return argument * argument;
	}

	is equivilent to the following:

	(NOTE: the function has to be wrapped in a class due to code
	generation bugs involved with directly specializing a function
	based on a constant.)

	template<int argument>
	class FunctionWrapper
	{
	public:
		int Function( int blah, int blah )
		{
			return argument*argument;
		}
	}

	typedef int (*FunctionType)( int blah, int blah );

	class FunctionName
	{
	public:
		enum { count = 10 };
		FunctionType functions[10];
	};

	FunctionType FunctionName::functions[] =
	{
		FunctionWrapper<0>::Function,
		FunctionWrapper<1>::Function,
		FunctionWrapper<2>::Function,
		FunctionWrapper<3>::Function,
		FunctionWrapper<4>::Function,
		FunctionWrapper<5>::Function,
		FunctionWrapper<6>::Function,
		FunctionWrapper<7>::Function,
		FunctionWrapper<8>::Function,
		FunctionWrapper<9>::Function
	};
*/


#define TEMPLATE_FUNCTION_TABLE(RETURN_TYPE, NAME, ARGS, COUNT)			\
																		\
typedef RETURN_TYPE (FASTCALL *__Type_##NAME) ARGS;						\
																		\
template<const int nArgument>											\
struct __Function_##NAME												\
{																		\
	static RETURN_TYPE FASTCALL Run ARGS;								\
};																		\
																		\
template <const int i>														\
struct __MetaLooper_##NAME : __MetaLooper_##NAME<i-1>					\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<i>::Run; }	\
};																		\
																		\
template<>																\
struct __MetaLooper_##NAME<0>											\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<0>::Run; }	\
};																		\
																		\
class NAME																\
{																		\
private:																\
    static const __MetaLooper_##NAME<COUNT> m;							\
public:																	\
	enum { count = COUNT };												\
	static const __Type_##NAME* functions;								\
};																		\
const __MetaLooper_##NAME<COUNT> NAME::m;								\
const __Type_##NAME* NAME::functions = (__Type_##NAME*)&m;				\
template<const int nArgument>													\
RETURN_TYPE FASTCALL __Function_##NAME<nArgument>::Run ARGS


#define LOOP_INTERCHANGE(BOOLEAN, CODE)\
	if( (BOOLEAN) )\
	{\
		CODE;\
	} else\
	{\
		CODE;\
	}

//-----------------------------------------------------------------------------
// Dynamic libs support
//-----------------------------------------------------------------------------
#if 0 // defined( PLATFORM_WINDOWS_PC )

PLATFORM_INTERFACE void *Plat_GetProcAddress( const char *pszModule, const char *pszName );

template <typename FUNCPTR_TYPE>
class CDynamicFunction
{
public:
	CDynamicFunction( const char *pszModule, const char *pszName, FUNCPTR_TYPE pfnFallback = NULL )
	{
		m_pfn = pfnFallback;
		void *pAddr = Plat_GetProcAddress( pszModule, pszName );
		if ( pAddr )
		{
			m_pfn = (FUNCPTR_TYPE)pAddr;
		}
	}

	operator bool()			{ return m_pfn != NULL;	}
	bool operator !()		{ return !m_pfn;	}
	operator FUNCPTR_TYPE()	{ return m_pfn; }

private:
	FUNCPTR_TYPE m_pfn;
};
#endif

//-----------------------------------------------------------------------------

#include "tier0/valve_on.h"

#if defined(TIER0_DLL_EXPORT)
extern "C" int V_tier0_stricmp(const char *s1, const char *s2 );
#undef stricmp
#undef strcmpi
#define stricmp(s1,s2) V_tier0_stricmp( s1, s2 )
#define strcmpi(s1,s2) V_tier0_stricmp( s1, s2 )
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

#endif /* PLATFORM_H */
