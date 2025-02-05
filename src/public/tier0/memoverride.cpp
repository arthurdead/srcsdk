//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Insert this file into all projects using the memory system
// It will cause that project to use the shader memory allocator
//
// $NoKeywords: $
//=============================================================================//

#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)

#include "platform.h"

#ifdef QT_DLL
#define NO_MEMOVERRIDE_NEW_DELETE
#endif

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "basetypes.h"
#include "dbg.h"
#include "minidump.h"
#include "memalloc.h"

#include "module_name.h"

#include "memdbgoff.h"

#if defined( _WIN32 )
#include "winlite.h"
#endif

#ifdef POSIX
typedef void(*_CRT_DUMP_CLIENT)();
typedef void(*_CRT_ALLOC_HOOK)();
typedef void(*_CRT_REPORT_HOOK)();
typedef HANDLE _HFILE;
typedef int errno_t;
struct _locale_t {};
#define _CRTDBG_ALLOC_MEM_DF 0
#endif

inline void __cdecl VPurecallHandler()
{
	DebuggerBreakIfDebugging();	// give the debugger a chance to catch first
	WriteMiniDump();
	Plat_ExitProcess( EXIT_FAILURE );
}

#if defined( _WIN32 )

// set Windows pure virtual handler
_purecall_handler OldPurecallHandler = _set_purecall_handler( VPurecallHandler );

#elif defined( POSIX )

// set OSX/Linux pure virtual handler
extern "C" void __cxa_pure_virtual() { VPurecallHandler(); }

#endif

#if defined(USE_MEM_DEBUG)
#pragma optimize( "", off )
#define inline
#endif

#if defined(USE_LIGHT_MEM_DEBUG) || defined(USE_MEM_DEBUG)
const char *g_pszModule = V_STRINGIFY( MEMOVERRIDE_MODULE );
#endif

inline void *AllocUnattributed( size_t nSize )
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	return MemAlloc_Alloc(nSize);
#else
	return MemAlloc_Alloc(nSize, ::g_pszModule, 0);
#endif
}

inline void *ReallocUnattributed( void *pMem, size_t nSize )
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	return g_pMemAlloc->Realloc(pMem, nSize);
#else
	return g_pMemAlloc->Realloc(pMem, nSize, ::g_pszModule, 0);
#endif
}

#undef inline


//-----------------------------------------------------------------------------
// Standard functions in the CRT that we're going to override to call our allocator
//-----------------------------------------------------------------------------

// this magic only works under win32
// under linux this malloc() overrides the libc malloc() and so we
// end up in a recursion (as MemAlloc_Alloc() calls malloc)
#if defined _MSC_VER
#ifndef _CRTNOALIAS
#define _CRTNOALIAS
#endif
#define ALLOC_CALL _CRTNOALIAS _CRTRESTRICT 
#define FREE_CALL _CRTNOALIAS 
#else
#define ALLOC_CALL
#define FREE_CALL
#endif

extern "C"
{

ALLOC_CALL void *malloc( size_t nSize )
{
	return AllocUnattributed( nSize );
}

FREE_CALL void free( void *pMem )
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	g_pMemAlloc->Free(pMem);
#else
	g_pMemAlloc->Free(pMem, ::g_pszModule, 0 );
#endif
}

ALLOC_CALL void *realloc( void *pMem, size_t nSize )
{
	return ReallocUnattributed( pMem, nSize );
}

ALLOC_CALL void *calloc( size_t nCount, size_t nElementSize )
{
	void *pMem = AllocUnattributed( nElementSize * nCount );
	memset(pMem, 0, nElementSize * nCount);
	return pMem;
}

} // end extern "C"

//-----------------------------------------------------------------------------
// Non-standard MSVC functions that we're going to override to call our allocator
//-----------------------------------------------------------------------------
extern "C"
{

// 64-bit
#ifdef _WIN64

#if ( defined ( _MSC_VER ) && _MSC_VER >= 1900 )
_CRTRESTRICT
#endif
void* __cdecl _malloc_base( size_t nSize )
{
	return AllocUnattributed( nSize );
}

#else

#if ( defined ( _MSC_VER ) && _MSC_VER >= 1900 )
_CRTRESTRICT
#endif
void *_malloc_base( size_t nSize )
{
	return AllocUnattributed( nSize );
}

#endif

#if ( defined ( _MSC_VER ) && _MSC_VER >= 1900 )
_CRTRESTRICT void *_calloc_base(size_t nCount, size_t nSize)
{
	void *pMem = AllocUnattributed(nCount*nSize);
	memset(pMem, 0, nCount*nSize);
	return pMem;
}
#else
void *_calloc_base( size_t nSize )
{
	void *pMem = AllocUnattributed( nSize );
	memset(pMem, 0, nSize);
	return pMem;
}
#endif

#if ( defined ( _MSC_VER ) && _MSC_VER >= 1900 )
_CRTRESTRICT
#endif
void *_realloc_base( void *pMem, size_t nSize )
{
	return ReallocUnattributed( pMem, nSize );
}

#if ( defined ( _MSC_VER ) && _MSC_VER >= 1900 )
_CRTRESTRICT
#endif
void *_recalloc_base( void *pMem, size_t nCount, size_t nSize )
{
	void *pMemOut = ReallocUnattributed( pMem, nSize * nCount );
	if (!pMem)
	{
		memset(pMemOut, 0, nSize * nCount);
	}
	return pMemOut;
}

void _free_base( void *pMem )
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	g_pMemAlloc->Free(pMem);
#else
	g_pMemAlloc->Free(pMem, ::g_pszModule, 0 );
#endif
}

void *__cdecl _expand_base( void *pMem, size_t nNewSize, int nBlockUse )
{
	Assert( 0 );
	return nullptr;
}

// crt
void * __cdecl _malloc_crt(size_t size)
{
	return AllocUnattributed( size );
}

void * __cdecl _calloc_crt(size_t count, size_t size)
{
#if (defined( _MSC_VER ) && _MSC_VER >= 1900)
	return _calloc_base(count, size);
#else
	return _calloc_base( count * size );
#endif
}

void * __cdecl _realloc_crt(void *ptr, size_t size)
{
	return _realloc_base( ptr, size );
}

void * __cdecl _recalloc_crt(void *ptr, size_t count, size_t size)
{
	return _recalloc_base( ptr, count, size );
}

ALLOC_CALL void * __cdecl _recalloc ( void * memblock, size_t count, size_t size )
{
	void *pMem = ReallocUnattributed( memblock, size * count );
	if (!memblock)
	{
		memset(pMem, 0, size * count);
	}
	return pMem;
}

size_t __cdecl _msize_base( void *pMem ) noexcept
{
	return g_pMemAlloc->GetSize(pMem);
}

#ifndef __MINGW32__
size_t _msize( void *pMem )
{
	return _msize_base(pMem);
}
#endif

size_t msize( void *pMem )
{
	return g_pMemAlloc->GetSize(pMem);
}

void *__cdecl _heap_alloc( size_t nSize )
{
	return AllocUnattributed( nSize );
}

void *__cdecl _nh_malloc( size_t nSize, int )
{
	return AllocUnattributed( nSize );
}

void *__cdecl _expand( void *pMem, size_t nSize )
{
	Assert( 0 );
	return nullptr;
}

unsigned int _amblksiz = 16; //BYTES_PER_PARA;

HANDLE _crtheap = (HANDLE)1;	// PatM Can't be 0 or CRT pukes
int __active_heap = 1;

size_t __cdecl _get_sbh_threshold( void )
{
	return 0;
}

int __cdecl _set_sbh_threshold( size_t )
{
	return 0;
}

#ifndef __MINGW32__
int _heapchk()
{
	return g_pMemAlloc->heapchk();
}
#endif

int _heapmin()
{
	return 1;
}

int __cdecl _heapadd( void *, size_t )
{
	return 0;
}

int __cdecl _heapset( unsigned int )
{
	return 0;
}

size_t __cdecl _heapused( size_t *, size_t * )
{
	return 0;
}

typedef struct _heapinfo _HEAPINFO;

int __cdecl _heapwalk( _HEAPINFO * )
{
	return 0;
}

} // end extern "C"


//-----------------------------------------------------------------------------
// Debugging functions that we're going to override to call our allocator
// NOTE: These have to be here for release + debug builds in case we
// link to a debug static lib!!!
//-----------------------------------------------------------------------------

extern "C"
{

void *malloc_db( size_t nSize, const char *pFileName, int nLine )
{
	return MemAlloc_Alloc(nSize, pFileName, nLine);
}

void free_db( void *pMem, const char *pFileName, int nLine )
{
	g_pMemAlloc->Free(pMem, pFileName, nLine);
}

void *realloc_db( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
	return g_pMemAlloc->Realloc(pMem, nSize, pFileName, nLine);
}

} // end extern "C"

//-----------------------------------------------------------------------------
// These methods are standard MSVC heap initialization + shutdown methods
//-----------------------------------------------------------------------------
extern "C"
{

	int __cdecl _heap_init()
	{
		return g_pMemAlloc != nullptr;
	}

	void __cdecl _heap_term()
	{
	}

}


//-----------------------------------------------------------------------------
// Prevents us from using an inappropriate new or delete method,
// ensures they are here even when linking against debug or release static libs
//-----------------------------------------------------------------------------
#ifndef NO_MEMOVERRIDE_NEW_DELETE
#if !defined( _OSX )

void *__cdecl operator new( size_t nSize )
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return MemAlloc_Alloc(nSize, pFileName, nLine );
}

void *__cdecl operator new[] ( size_t nSize )
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new[] ( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return MemAlloc_Alloc(nSize, pFileName, nLine);
}

#else

void *__cdecl operator new( size_t nSize ) throw (std::bad_alloc)
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return MemAlloc_Alloc(nSize, pFileName, nLine );
}

void *__cdecl operator new[] ( size_t nSize ) throw (std::bad_alloc)
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new[] ( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return MemAlloc_Alloc(nSize, pFileName, nLine);
}
#endif // !_OSX

void __cdecl operator delete( void *pMem ) throw()
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	g_pMemAlloc->Free(pMem);
#else
	g_pMemAlloc->Free(pMem, ::g_pszModule, 0 );
#endif
}

void __cdecl operator delete[] ( void *pMem ) throw()
{
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	g_pMemAlloc->Free(pMem);
#else
	g_pMemAlloc->Free(pMem, ::g_pszModule, 0 );
#endif
}
#endif


//-----------------------------------------------------------------------------
// Override some debugging allocation methods in MSVC
// NOTE: These have to be here for release + debug builds in case we
// link to a debug static lib!!!
//-----------------------------------------------------------------------------

// This here just hides the internal file names, etc of allocations
// made in the c runtime library
#define CRT_INTERNAL_FILE_NAME "C-runtime internal"

class CAttibCRT
{
public:
	CAttibCRT(int nBlockUse) : m_nBlockUse(nBlockUse)
	{
		if (m_nBlockUse == _CRT_BLOCK)
		{
			g_pMemAlloc->PushAllocDbgInfo(CRT_INTERNAL_FILE_NAME, 0);
		}
	}
	
	~CAttibCRT()
	{
		if (m_nBlockUse == _CRT_BLOCK)
		{
			g_pMemAlloc->PopAllocDbgInfo();
		}
	}
	
private:
	int m_nBlockUse;
};


#define AttribIfCrt() CAttibCRT _attrib(nBlockUse)


extern "C"
{

void *__cdecl _nh_malloc_dbg( size_t nSize, int nFlag, int nBlockUse,
								const char *pFileName, int nLine )
{
	AttribIfCrt();
	return MemAlloc_Alloc(nSize, pFileName, nLine);
}

#undef _malloc_dbg
void *__cdecl _malloc_dbg( size_t nSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	return MemAlloc_Alloc(nSize, pFileName, nLine);
}

#undef _calloc_dbg
void *__cdecl _calloc_dbg( size_t nNum, size_t nSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	void *pMem = MemAlloc_Alloc(nSize * nNum, pFileName, nLine);
	memset(pMem, 0, nSize * nNum);
	return pMem;
}

void *__cdecl _calloc_dbg_impl( size_t nNum, size_t nSize, int nBlockUse, 
								const char * szFileName, int nLine, int * errno_tmp )
{
	return _calloc_dbg( nNum, nSize, nBlockUse, szFileName, nLine );
}

#undef _realloc_dbg
void *__cdecl _realloc_dbg( void *pMem, size_t nNewSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	return g_pMemAlloc->Realloc(pMem, nNewSize, pFileName, nLine);
}

#undef _expand_dbg
void *__cdecl _expand_dbg( void *pMem, size_t nNewSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	Assert( 0 );
	return nullptr;
}

#undef _free_dbg
void __cdecl _free_dbg( void *pMem, int nBlockUse )
{
	AttribIfCrt();
#if !defined(USE_LIGHT_MEM_DEBUG) && !defined(USE_MEM_DEBUG)
	g_pMemAlloc->Free(pMem);
#else
	g_pMemAlloc->Free(pMem, ::g_pszModule, 0 );
#endif
}

#undef _msize_dbg
size_t __cdecl _msize_dbg( void *pMem, int nBlockUse )
{
#ifdef _WIN32
	return _msize(pMem);
#elif POSIX
	Assert( "_msize_dbg unsupported" );
	return 0;
#endif
}


// aligned base
ALLOC_CALL void *__cdecl _aligned_malloc_base( size_t size, size_t align )
{
	return MemAlloc_AllocAligned( size, align );
}

ALLOC_CALL void *__cdecl _aligned_realloc_base( void *ptr, size_t size, size_t align )
{
	return MemAlloc_ReallocAligned( ptr, size, align );
}

ALLOC_CALL void *__cdecl _aligned_recalloc_base( void *ptr, size_t size, size_t align )
{
	Error( "Unsupported function\n" );
	return nullptr;
}

FREE_CALL void __cdecl _aligned_free_base( void *ptr )
{
	MemAlloc_FreeAligned( ptr );
}

// aligned
ALLOC_CALL void * __cdecl _aligned_malloc( size_t size, size_t align )
{
	return _aligned_malloc_base(size, align);
}

ALLOC_CALL void *__cdecl _aligned_realloc(void *memblock, size_t size, size_t align)
{
	return _aligned_realloc_base(memblock, size, align);
}

ALLOC_CALL void * __cdecl _aligned_recalloc( void * memblock, size_t count, size_t size, size_t align )
{
	return _aligned_recalloc_base(memblock, count * size, align);
}

FREE_CALL void __cdecl _aligned_free( void *memblock )
{
	_aligned_free_base(memblock);
}

// aligned offset base
ALLOC_CALL void * __cdecl _aligned_offset_malloc_base( size_t size, size_t align, size_t offset )
{
	Assert( 0 );
	return nullptr;
}

ALLOC_CALL void * __cdecl _aligned_offset_realloc_base( void * memblock, size_t size, size_t align, size_t offset)
{
	Assert( 0 );
	return nullptr;
}

ALLOC_CALL void * __cdecl _aligned_offset_recalloc_base( void * memblock, size_t size, size_t align, size_t offset)
{
	Assert( 0 );
	return nullptr;
}

// aligned offset
ALLOC_CALL void *__cdecl _aligned_offset_malloc(size_t size, size_t align, size_t offset)
{
	return _aligned_offset_malloc_base( size, align, offset );
}

ALLOC_CALL void *__cdecl _aligned_offset_realloc(void *memblock, size_t size, size_t align, size_t offset)
{
	return _aligned_offset_realloc_base( memblock, size, align, offset );
}

ALLOC_CALL void * __cdecl _aligned_offset_recalloc( void * memblock, size_t count, size_t size, size_t align, size_t offset )
{
	return _aligned_offset_recalloc_base( memblock, count * size, align, offset );
}

// aligned offset debug
void * __cdecl _aligned_offset_recalloc_dbg( void * memblock, size_t count, size_t size, size_t align, size_t offset, const char * f_name, int line_n )
{
	Assert( 0 );
	void *pMem = ReallocUnattributed( memblock, size * count );
	if (!memblock)
	{
		memset(pMem, 0, size * count);
	}

	return pMem;
}

void * __cdecl _aligned_recalloc_dbg( void *memblock, size_t count, size_t size, size_t align, const char * f_name, int line_n )
{
	return _aligned_offset_recalloc_dbg(memblock, count, size, align, 0, f_name, line_n);
}

void * __cdecl _recalloc_dbg ( void * memblock, size_t count, size_t size, int nBlockUse, const char * szFileName, int nLine )
{
	return _aligned_offset_recalloc_dbg(memblock, count, size, 0, 0, szFileName, nLine);
}

} // end extern "C"


//-----------------------------------------------------------------------------
// Override some the _CRT debugging allocation methods in MSVC
//-----------------------------------------------------------------------------
extern "C"
{

#undef _CrtDumpMemoryLeaks
int _CrtDumpMemoryLeaks(void)
{
	return 0;
}

#undef _CrtSetDumpClient
_CRT_DUMP_CLIENT _CrtSetDumpClient( _CRT_DUMP_CLIENT dumpClient )
{
	return nullptr;
}

#undef _CrtSetDbgFlag
int _CrtSetDbgFlag( int nNewFlag )
{
	return g_pMemAlloc->CrtSetDbgFlag( nNewFlag );
}

// 64-bit port.
#define AFNAME(var) __p_ ## var
#define AFRET(var)  &var

#undef _crtDbgFlag
int _crtDbgFlag = _CRTDBG_ALLOC_MEM_DF;
int* AFNAME(_crtDbgFlag)(void)
{
	return AFRET(_crtDbgFlag);
}

#undef _crtBreakAlloc
long _crtBreakAlloc;      /* Break on this allocation */
long* AFNAME(_crtBreakAlloc) (void)
{
	return AFRET(_crtBreakAlloc);
}

void __cdecl _CrtSetDbgBlockType( void *pMem, int nBlockUse )
{
	DebuggerBreak();
}

#undef _CrtSetAllocHook
_CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook( _CRT_ALLOC_HOOK pfnNewHook )
{
	DebuggerBreak();
	return nullptr;
}

#undef _CrtSetBreakAlloc
long __cdecl _CrtSetBreakAlloc( long lNewBreakAlloc )
{
	return g_pMemAlloc->CrtSetBreakAlloc( lNewBreakAlloc );
}

#undef _CrtIsValidHeapPointer
int __cdecl _CrtIsValidHeapPointer( const void *pMem )
{
	return g_pMemAlloc->CrtIsValidHeapPointer( pMem );
}

#undef _CrtIsValidPointer
int __cdecl _CrtIsValidPointer( const void *pMem, unsigned int size, int access )
{
	return g_pMemAlloc->CrtIsValidPointer( pMem, size, access );
}

#undef _CrtCheckMemory
int __cdecl _CrtCheckMemory( void )
{
	// FIXME: Remove this when we re-implement the heap
	return g_pMemAlloc->CrtCheckMemory( );
}

#undef _CrtIsMemoryBlock
int __cdecl _CrtIsMemoryBlock( const void *pMem, unsigned int nSize,
	long *plRequestNumber, char **ppFileName, int *pnLine )
{
	DebuggerBreak();
	return 1;
}

#undef _CrtMemDifference
int __cdecl _CrtMemDifference( _CrtMemState *pState, const _CrtMemState * oldState, const _CrtMemState * newState )
{
	DebuggerBreak();
	return FALSE;
}

#undef _CrtMemDumpStatistics
void __cdecl _CrtMemDumpStatistics( const _CrtMemState *pState )
{
	DebuggerBreak();	
}

#undef _CrtMemCheckpoint
void __cdecl _CrtMemCheckpoint( _CrtMemState *pState )
{
	// FIXME: Remove this when we re-implement the heap
	g_pMemAlloc->CrtMemCheckpoint( pState );
}

#undef _CrtMemDumpAllObjectsSince
void __cdecl _CrtMemDumpAllObjectsSince( const _CrtMemState *pState )
{
	DebuggerBreak();
}

#undef _CrtDoForAllClientObjects
void __cdecl _CrtDoForAllClientObjects( void (*pfn)(void *, void *), void * pContext )
{
	DebuggerBreak();
}


//-----------------------------------------------------------------------------
// Methods in dbgrpt.cpp 
//-----------------------------------------------------------------------------
long _crtAssertBusy = -1;

#undef _CrtSetReportMode
int __cdecl _CrtSetReportMode( int nReportType, int nReportMode )
{
	return g_pMemAlloc->CrtSetReportMode( nReportType, nReportMode );
}

#undef _CrtSetReportFile
_HFILE __cdecl _CrtSetReportFile( int nRptType, _HFILE hFile )
{
	return (_HFILE)g_pMemAlloc->CrtSetReportFile( nRptType, hFile );
}

#undef _CrtSetReportHook
_CRT_REPORT_HOOK __cdecl _CrtSetReportHook( _CRT_REPORT_HOOK pfnNewHook )
{
	return (_CRT_REPORT_HOOK)g_pMemAlloc->CrtSetReportHook( (void *)pfnNewHook );
}

int __cdecl _CrtDbgReport( int nRptType, const char * szFile,
		int nLine, const char * szModule, const char * szFormat, ... )
{
	static char output[1024];
	va_list args;
	va_start( args, szFormat );
	//V_tier0_sprintf_s( output, sizeof( output ), szFormat, args );
	snprintf( output, sizeof( output ), szFormat, args );
	va_end( args );

	return g_pMemAlloc->CrtDbgReport( nRptType, szFile, nLine, szModule, output );
}

// wrapper which passes no debug info; not available in debug
void __cdecl _invalid_parameter_noinfo(void)
{
	Assert(0);
}

int __cdecl __crtMessageWindowW( int nRptType, const wchar_t * szFile, const wchar_t * szLine,
								 const wchar_t * szModule, const wchar_t * szUserMessage )
{
	//Assert(0);
	return 0;
}

int __cdecl _CrtDbgReportV( int nRptType, const wchar_t *szFile, int nLine, 
							const wchar_t *szModule, const wchar_t *szFormat, va_list arglist )
{
	//Assert(0);
	return 0;
}

int __cdecl _CrtDbgReportW( int nRptType, const wchar_t *szFile, int nLine, 
							const wchar_t *szModule, const wchar_t *szFormat, ...)
{
	//Assert(0);
	return 0;
}

#if ( defined(_MSC_VER) && _MSC_VER >= 1900)
int __cdecl _VCrtDbgReportA(int nRptType, void *pReturnAddr, const char* szFile, int nLine,
							const char *szModule, const char *szFormat, va_list arglist)
{
	Assert(0);
	return 0;
}
#else
int __cdecl _VCrtDbgReportA( int nRptType, const wchar_t * szFile, int nLine, 
							 const wchar_t * szModule, const wchar_t * szFormat, va_list arglist )
{
	Assert(0);
	return 0;
}
#endif

#undef _CrtSetReportHook2
int __cdecl _CrtSetReportHook2( int mode, _CRT_REPORT_HOOK pfnNewHook )
{
	_CrtSetReportHook( pfnNewHook );
	return 0;
}

int __crtDebugCheckCount = FALSE;

#undef _CrtSetCheckCount
int __cdecl _CrtSetCheckCount( int fCheckCount )
{
	int oldCheckCount = __crtDebugCheckCount;
	return oldCheckCount;
}

#undef _CrtGetCheckCount
int __cdecl _CrtGetCheckCount( void )
{
	return __crtDebugCheckCount;
}

#undef _CrtGetReportHook
_CRT_REPORT_HOOK __cdecl _CrtGetReportHook( void )
{
	return nullptr;
}

#undef _CrtReportBlockType
int __cdecl _CrtReportBlockType(const void * pUserData)
{
	return 0;
}

} // end extern "C"

// Most files include this file, so when it's used it adds an extra .ValveDbg section,
// to help identify debug binaries.
#ifdef _MSC_VER
	#pragma data_seg("ValveDBG") 
#endif
volatile const char* DBG = "*** DEBUG STUB ***";

// Extras added prevent dbgheap.obj from being included - DAL
extern "C"
{

size_t __crtDebugFillThreshold = 0;

extern "C" void * __cdecl _heap_alloc_base (size_t size) 
{
	Assert(0);
	return nullptr;
}


void * __cdecl _heap_alloc_dbg( size_t nSize, int nBlockUse, const char * szFileName, int nLine)
{
		return _heap_alloc(nSize);
}

// 64-bit
#ifdef _WIN64
static void * __cdecl realloc_help( void * pUserData, size_t * pnNewSize, int nBlockUse,const char * szFileName,
				int nLine, int fRealloc )
{
		Assert(0); // Shouldn't be needed
		return nullptr;
}
#else
static void * __cdecl realloc_help( void * pUserData, size_t nNewSize, int nBlockUse, const char * szFileName,
				  int nLine, int fRealloc)
{
		Assert(0); // Shouldn't be needed
		return nullptr;
}
#endif

void __cdecl _free_nolock( void * pUserData)
{
		// I don't think the second param is used in memoverride
		_free_dbg(pUserData, 0);
}

void __cdecl _free_dbg_nolock( void * pUserData, int nBlockUse)
{
		_free_dbg(pUserData, 0);
}

#undef _CrtGetAllocHook
_CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook ( void)
{
		Assert(0); 
		return nullptr;
}

static int __cdecl CheckBytes( unsigned char * pb, unsigned char bCheck, size_t nSize)
{
		int bOkay = TRUE;
		return bOkay;
}


#undef _CrtGetDumpClient
_CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient ( void)
{
		Assert(0); 
		return nullptr;
}

struct _CrtMemBlockHeader;

static void __cdecl _printMemBlockData( _locale_t plocinfo, _CrtMemBlockHeader * pHead)
{
}

static void __cdecl _CrtMemDumpAllObjectsSince_stat( const _CrtMemState * state, _locale_t plocinfo)
{
}

#undef _aligned_malloc_dbg
void * __cdecl _aligned_malloc_dbg( size_t size, size_t align, const char * f_name, int line_n)
{
	return _aligned_malloc(size, align);
}

#undef _aligned_realloc_dbg
void * __cdecl _aligned_realloc_dbg( void *memblock, size_t size, size_t align,
			   const char * f_name, int line_n)
{
	return _aligned_realloc(memblock, size, align);
}

#undef _aligned_offset_malloc_dbg
void * __cdecl _aligned_offset_malloc_dbg( size_t size, size_t align, size_t offset,
			  const char * f_name, int line_n)
{
	return _aligned_offset_malloc(size, align, offset);
}

#undef _aligned_offset_realloc_dbg
void * __cdecl _aligned_offset_realloc_dbg( void * memblock, size_t size, size_t align, 
				 size_t offset, const char * f_name, int line_n)
{
	return _aligned_offset_realloc(memblock, size, align, offset);
}

#undef _aligned_free_dbg
void __cdecl _aligned_free_dbg( void * memblock)
{
	_aligned_free(memblock);
}

#undef _CrtSetDebugFillThreshold
size_t __cdecl _CrtSetDebugFillThreshold( size_t _NewDebugFillThreshold)
{
	Assert(0);
	return 0;
}

//===========================================
// NEW!!! 64-bit

char * __cdecl _strdup ( const char * string )
{
	size_t nSize = strlen(string) + 1;
	char *pCopy = (char*)AllocUnattributed( nSize );
	if ( pCopy )
		memcpy( pCopy, string, nSize );
	return pCopy;
}

#undef _tfullpath_dbg
_TSCHAR * __cdecl _tfullpath_dbg ( _TSCHAR *UserBuf, const _TSCHAR *path, size_t maxlen, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return nullptr;
}

tchar * __cdecl _tfullpath ( tchar *UserBuf, const tchar *path, size_t maxlen )
{
	Assert(0);
	return nullptr;
}

_TSCHAR * __cdecl _tgetdcwd_lk_dbg ( int drive, _TSCHAR *pnbuf, int maxlen, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return nullptr;
}

_TSCHAR * __cdecl _tgetdcwd_nolock ( int drive, _TSCHAR *pnbuf, int maxlen )
{
	Assert(0);
	return nullptr;
}

errno_t __cdecl _tdupenv_s_helper_dbg ( _TSCHAR **pBuffer, size_t *pBufferSizeInTChars, const _TSCHAR *varname, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

errno_t __cdecl _tdupenv_s_helper ( _TSCHAR **pBuffer, size_t *pBufferSizeInTChars, const _TSCHAR *varname )
{
	Assert(0);
	return 0;
}

#undef _ttempnam_dbg
_TSCHAR * __cdecl _ttempnam_dbg ( const _TSCHAR *dir, const _TSCHAR *pfx, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

tchar * __cdecl _ttempnam ( const tchar *dir, const tchar *pfx )
{
	Assert(0);
	return 0;
}

#undef _wcsdup_dbg
wchar_t * __cdecl _wcsdup_dbg ( const wchar_t * string, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

wchar_t * __cdecl _wcsdup ( const wchar_t * string )
{
	Assert(0);
	return 0;
}

} // end extern "C"

#if defined _MSC_VER && _MSC_VER >= 1400

//-----------------------------------------------------------------------------
// 	XBox Memory Allocator Override
//-----------------------------------------------------------------------------

#define MAX_LANG_LEN        64  /* max language name length */
#define MAX_CTRY_LEN        64  /* max country name length */
#define MAX_MODIFIER_LEN    0   /* max modifier name length - n/a */
#define MAX_LC_LEN          (MAX_LANG_LEN+MAX_CTRY_LEN+MAX_MODIFIER_LEN+3)

#if _MSC_VER >= 1700 // VS 11 (VS 2012)
// Copied from C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\crt\src\mtdll.h
#ifndef _SETLOC_STRUCT_DEFINED
struct _is_ctype_compatible {
		unsigned long id;
		int is_clike;
};

typedef struct setloc_struct {
	/* getqloc static variables */
	wchar_t *pchLanguage;
	wchar_t *pchCountry;
	int iLocState;
	int iPrimaryLen;
	BOOL bAbbrevLanguage;
	BOOL bAbbrevCountry;
	UINT        _cachecp;
	wchar_t     _cachein[MAX_LC_LEN];
	wchar_t     _cacheout[MAX_LC_LEN];
	/* _setlocale_set_cat (LC_CTYPE) static variable */
	struct _is_ctype_compatible _Loc_c[5];
	wchar_t _cacheLocaleName[LOCALE_NAME_MAX_LENGTH];
} _setloc_struct, *_psetloc_struct;
#define _SETLOC_STRUCT_DEFINED
#endif  /* _SETLOC_STRUCT_DEFINED */

_CRTIMP extern unsigned long __cdecl __threadid(void);
#define _threadid   (__threadid())
_CRTIMP extern uintptr_t __cdecl __threadhandle(void);
#define _threadhandle   (__threadhandle())

/* Structure for each thread's data */

struct _tiddata {
	unsigned long   _tid;       /* thread ID */


	uintptr_t _thandle;         /* thread handle */

	int     _terrno;            /* errno value */
	unsigned long   _tdoserrno; /* _doserrno value */
	unsigned int    _fpds;      /* Floating Point data segment */
	unsigned long   _holdrand;  /* rand() seed value */
	char *      _token;         /* ptr to strtok() token */
	wchar_t *   _wtoken;        /* ptr to wcstok() token */
	unsigned char * _mtoken;    /* ptr to _mbstok() token */

	/* following pointers get malloc'd at runtime */
	char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
	wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
	char *      _namebuf0;      /* ptr to tmpnam() buffer */
	wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
	char *      _namebuf1;      /* ptr to tmpfile() buffer */
	wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
	char *      _asctimebuf;    /* ptr to asctime() buffer */
	wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
	void *      _gmtimebuf;     /* ptr to gmtime() structure */
	char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */
	unsigned char _con_ch_buf[MB_LEN_MAX];
								/* ptr to putch() buffer */
	unsigned short _ch_buf_used;   /* if the _con_ch_buf is used */

	/* following fields are needed by _beginthread code */
	void *      _initaddr;      /* initial user thread address */
	void *      _initarg;       /* initial user thread argument */

	/* following three fields are needed to support signal handling and
	 * runtime errors */
	void *      _pxcptacttab;   /* ptr to exception-action table */
	void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
	int         _tfpecode;      /* float point exception code */

#if ( defined( _MSC_VER ) && _MSC_VER >= 1900)
	void *ptmbcinfo_dummy;
	void *ptlocinfo_dummy;
#else
	/* pointer to the copy of the multibyte character information used by
	 * the thread */
	pthreadmbcinfo  ptmbcinfo;

	/* pointer to the copy of the locale informaton used by the thead */
	pthreadlocinfo  ptlocinfo;
#endif

	int         _ownlocale;     /* if 1, this thread owns its own locale */

	/* following field is needed by NLG routines */
	unsigned long   _NLG_dwCode;

	/*
	 * Per-Thread data needed by C++ Exception Handling
	 */
	void *      _terminate;     /* terminate() routine */
	void *      _unexpected;    /* unexpected() routine */
	void *      _translator;    /* S.E. translator */
	void *      _purecall;      /* called when pure virtual happens */
	void *      _curexception;  /* current exception */
	void *      _curcontext;    /* current exception context */
	int         _ProcessingThrow; /* for uncaught_exception */
	void *      _curexcspec;    /* for handling exceptions thrown from std::unexpected */
#if defined (_M_X64) || defined (_M_ARM)
	void *      _pExitContext;
	void *      _pUnwindContext;
	void *      _pFrameInfoChain;
#if defined (_WIN64)
	unsigned __int64    _ImageBase;
	unsigned __int64    _ThrowImageBase;
#else  /* defined (_WIN64) */
	unsigned __int32    _ImageBase;
	unsigned __int32    _ThrowImageBase;
#endif  /* defined (_WIN64) */
	void *      _pForeignException;
#elif defined (_M_IX86)
	void *      _pFrameInfoChain;
#endif  /* defined (_M_IX86) */
	_setloc_struct _setloc_data;

	void *      _reserved1;     /* nothing */
	void *      _reserved2;     /* nothing */
	void *      _reserved3;     /* nothing */
#ifdef _M_IX86
	void *      _reserved4;     /* nothing */
	void *      _reserved5;     /* nothing */
#endif  /* _M_IX86 */

	int _cxxReThrow;        /* Set to True if it's a rethrown C++ Exception */

	unsigned long __initDomain;     /* initial domain used by _beginthread[ex] for managed function */
};
#else // _MSC_VER >= 1700 // VS 11 (VS 2012)
struct _is_ctype_compatible {
		unsigned long id;
		int is_clike;
};
typedef struct setloc_struct {
	/* getqloc static variables */
	char *pchLanguage;
	char *pchCountry;
	int iLcidState;
	int iPrimaryLen;
	BOOL bAbbrevLanguage;
	BOOL bAbbrevCountry;
	LCID lcidLanguage;
	LCID lcidCountry;
	/* expand_locale static variables */
	LC_ID       _cacheid;
	UINT        _cachecp;
	char        _cachein[MAX_LC_LEN];
	char        _cacheout[MAX_LC_LEN];
	/* _setlocale_set_cat (LC_CTYPE) static variable */
	struct _is_ctype_compatible _Lcid_c[5];
} _setloc_struct, *_psetloc_struct;

struct _tiddata {
	unsigned long   _tid;       /* thread ID */


	uintptr_t _thandle;         /* thread handle */

	int     _terrno;            /* errno value */
	unsigned long   _tdoserrno; /* _doserrno value */
	unsigned int    _fpds;      /* Floating Point data segment */
	unsigned long   _holdrand;  /* rand() seed value */
	char *      _token;         /* ptr to strtok() token */
	wchar_t *   _wtoken;        /* ptr to wcstok() token */
	unsigned char * _mtoken;    /* ptr to _mbstok() token */

	/* following pointers get malloc'd at runtime */
	char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
	wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
	char *      _namebuf0;      /* ptr to tmpnam() buffer */
	wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
	char *      _namebuf1;      /* ptr to tmpfile() buffer */
	wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
	char *      _asctimebuf;    /* ptr to asctime() buffer */
	wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
	void *      _gmtimebuf;     /* ptr to gmtime() structure */
	char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */
	unsigned char _con_ch_buf[MB_LEN_MAX];
								/* ptr to putch() buffer */
	unsigned short _ch_buf_used;   /* if the _con_ch_buf is used */

	/* following fields are needed by _beginthread code */
	void *      _initaddr;      /* initial user thread address */
	void *      _initarg;       /* initial user thread argument */

	/* following three fields are needed to support signal handling and
	 * runtime errors */
	void *      _pxcptacttab;   /* ptr to exception-action table */
	void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
	int         _tfpecode;      /* float point exception code */

	/* pointer to the copy of the multibyte character information used by
	 * the thread */
	pthreadmbcinfo  ptmbcinfo;

	/* pointer to the copy of the locale informaton used by the thead */
	pthreadlocinfo  ptlocinfo;
	int         _ownlocale;     /* if 1, this thread owns its own locale */

	/* following field is needed by NLG routines */
	unsigned long   _NLG_dwCode;

	/*
	 * Per-Thread data needed by C++ Exception Handling
	 */
	void *      _terminate;     /* terminate() routine */
	void *      _unexpected;    /* unexpected() routine */
	void *      _translator;    /* S.E. translator */
	void *      _purecall;      /* called when pure virtual happens */
	void *      _curexception;  /* current exception */
	void *      _curcontext;    /* current exception context */
	int         _ProcessingThrow; /* for uncaught_exception */
	void *              _curexcspec;    /* for handling exceptions thrown from std::unexpected */
#if defined (_M_IA64) || defined (_M_AMD64)
	void *      _pExitContext;
	void *      _pUnwindContext;
	void *      _pFrameInfoChain;
	unsigned __int64    _ImageBase;
#if defined (_M_IA64)
	unsigned __int64    _TargetGp;
#endif  /* defined (_M_IA64) */
	unsigned __int64    _ThrowImageBase;
	void *      _pForeignException;
#elif defined (_M_IX86)
	void *      _pFrameInfoChain;
#endif  /* defined (_M_IX86) */
	_setloc_struct _setloc_data;

	void *      _encode_ptr;    /* EncodePointer() routine */
	void *      _decode_ptr;    /* DecodePointer() routine */

	void *      _reserved1;     /* nothing */
	void *      _reserved2;     /* nothing */
	void *      _reserved3;     /* nothing */

	int _cxxReThrow;        /* Set to True if it's a rethrown C++ Exception */

	unsigned long __initDomain;     /* initial domain used by _beginthread[ex] for managed function */
};
#endif // _MSC_VER >= 1700 // VS 11 (VS 2012)

typedef struct _tiddata * _ptiddata;

#if (defined( _MSC_VER ) && _MSC_VER >= 1900)
//Do we need anything in here?
#else
class _LocaleUpdate
{
	_locale_tstruct localeinfo;
	_ptiddata ptd;
	bool updated;
	public:
	_LocaleUpdate(_locale_t plocinfo)
		: updated(false)
	{
		/*
		if (plocinfo == nullptr)
		{
			ptd = _getptd();
			localeinfo.locinfo = ptd->ptlocinfo;
			localeinfo.mbcinfo = ptd->ptmbcinfo;

			__UPDATE_LOCALE(ptd, localeinfo.locinfo);
			__UPDATE_MBCP(ptd, localeinfo.mbcinfo);
			if (!(ptd->_ownlocale & _PER_THREAD_LOCALE_BIT))
			{
				ptd->_ownlocale |= _PER_THREAD_LOCALE_BIT;
				updated = true;
			}
		}
		else
		{
			localeinfo=*plocinfo;
		}
		*/
	}
	~_LocaleUpdate()
	{
//        if (updated)
//	        ptd->_ownlocale = ptd->_ownlocale & ~_PER_THREAD_LOCALE_BIT;
	}
	_locale_t GetLocaleT()
	{
		return &localeinfo;
	}
};
#endif //_MSC_VER

#pragma warning(push)
#pragma warning(disable: 4483)
#if _MSC_FULL_VER >= 140050415
#define _NATIVE_STARTUP_NAMESPACE  __identifier("<CrtImplementationDetails>")
#else  /* _MSC_FULL_VER >= 140050415 */
#define _NATIVE_STARTUP_NAMESPACE __CrtImplementationDetails
#endif  /* _MSC_FULL_VER >= 140050415 */

namespace _NATIVE_STARTUP_NAMESPACE
{
	class NativeDll
	{
	private:
		static const unsigned int ProcessDetach   = 0;
		static const unsigned int ProcessAttach   = 1;
		static const unsigned int ThreadAttach    = 2;
		static const unsigned int ThreadDetach    = 3;
		static const unsigned int ProcessVerifier = 4;

	public:

		inline static bool IsInDllMain()
		{
			return false;
		}

		inline static bool IsInProcessAttach()
		{
			return false;
		}

		inline static bool IsInProcessDetach()
		{
			return false;
		}

		inline static bool IsInVcclrit()
		{
			return false;
		}

		inline static bool IsSafeForManagedCode()
		{
			if (!IsInDllMain())
			{
				return true;
			}

			if (IsInVcclrit())
			{
				return true;
			}

			return !IsInProcessAttach() && !IsInProcessDetach();
		}
	};
}
#pragma warning(pop)

#endif // _MSC_VER >= 1400

#endif // !STEAM && !NO_MALLOC_OVERRIDE
