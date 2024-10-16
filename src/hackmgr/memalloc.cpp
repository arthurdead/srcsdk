#include "tier0/memalloc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef malloc
#undef realloc
#undef free

#ifdef LINUX

static MemAllocFailHandler_t s_AllocFailHandler = NULL;

class CMemAlloc : public IMemAlloc
{
public:
	// Release versions
	virtual void *Alloc( size_t nSize )
	{ return ::malloc(nSize); }
	virtual void *Realloc( void *pMem, size_t nSize )
	{ return ::realloc(pMem, nSize); }
	virtual void Free( void *pMem )
	{ ::free(pMem); }
	virtual void *Expand_NoLongerSupported( void *pMem, size_t nSize )
	{ return NULL; }

	// Debug versions
	virtual void *Alloc( size_t nSize, const char *pFileName, int nLine )
	{ return ::malloc(nSize); }
	virtual void *Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine )
	{ return ::realloc(pMem, nSize); }
	virtual void  Free( void *pMem, const char *pFileName, int nLine )
	{ ::free(pMem); }
	virtual void *Expand_NoLongerSupported( void *pMem, size_t nSize, const char *pFileName, int nLine )
	{ return NULL; }

	// Returns size of a particular allocation
	virtual size_t GetSize( void *pMem )
	{ return ::malloc_usable_size(pMem); }

	// Force file + line information for an allocation
	virtual void PushAllocDbgInfo( const char *pFileName, int nLine )
	{
	}
	virtual void PopAllocDbgInfo()
	{
	}

	// FIXME: Remove when we have our own allocator
	// these methods of the Crt debug code is used in our codebase currently
	virtual long CrtSetBreakAlloc( long lNewBreakAlloc )
	{
		return 0;
	}
	virtual	int CrtSetReportMode( int nReportType, int nReportMode )
	{
		return 0;
	}
	virtual int CrtIsValidHeapPointer( const void *pMem )
	{
		return pMem != NULL;
	}
	virtual int CrtIsValidPointer( const void *pMem, unsigned int size, int access )
	{
		return pMem != NULL;
	}
	virtual int CrtCheckMemory( void )
	{
		return 0;
	}
	virtual int CrtSetDbgFlag( int nNewFlag )
	{
		return 0;
	}
	virtual void CrtMemCheckpoint( _CrtMemState *pState )
	{
	}

	// FIXME: Make a better stats interface
	virtual void DumpStats()
	{
	}
	virtual void DumpStatsFileBase( char const *pchFileBase )
	{
	}

	// FIXME: Remove when we have our own allocator
	virtual void* CrtSetReportFile( int nRptType, void* hFile )
	{
		return NULL;
	}
	virtual void* CrtSetReportHook( void* pfnNewHook )
	{
		return NULL;
	}
	virtual int CrtDbgReport( int nRptType, const char * szFile,
			int nLine, const char * szModule, const char * pMsg )
	{
		return 0;
	}

	virtual int heapchk()
	{ return 0; }

	virtual bool IsDebugHeap()
	{ return false; }

	virtual void GetActualDbgInfo( const char *&pFileName, int &nLine )
	{
		pFileName = NULL;
		nLine = -1;
	}
	virtual void RegisterAllocation( const char *pFileName, int nLine, int nLogicalSize, int nActualSize, unsigned nTime )
	{
	}
	virtual void RegisterDeallocation( const char *pFileName, int nLine, int nLogicalSize, int nActualSize, unsigned nTime )
	{
	}

	virtual int GetVersion()
	{
		return 0;
	}

	virtual void CompactHeap()
	{
	}

	// Function called when malloc fails or memory limits hit to attempt to free up memory (can come in any thread)
	virtual MemAllocFailHandler_t SetAllocFailHandler( MemAllocFailHandler_t pfnMemAllocFailHandler )
	{
		MemAllocFailHandler_t old = s_AllocFailHandler;
		s_AllocFailHandler = pfnMemAllocFailHandler;
		return old;
	}

	virtual void DumpBlockStats( void * )
	{
	}

#if defined( _MEMTEST )	
	virtual void SetStatsExtraInfo( const char *pMapName, const char *pComment )
	{
	}
#endif

	// Returns 0 if no failure, otherwise the size_t of the last requested chunk
	//  "I'm sure this is completely thread safe!" Brian Deen 7/19/2012.
	virtual size_t MemoryAllocFailed()
	{
		return 0;
	}

	// handles storing allocation info for coroutines
	virtual uint32 GetDebugInfoSize()
	{
		return 0;
	}
	virtual void SaveDebugInfo( void *pvDebugInfo )
	{
	}
	virtual void RestoreDebugInfo( const void *pvDebugInfo )
	{
	}
	virtual void InitDebugInfo( void *pvDebugInfo, const char *pchRootFileName, int nLine )
	{
	}

	// Replacement for ::GlobalMemoryStatus which accounts for unused memory in our system
	virtual void GlobalMemoryStatus( size_t *pUsedMemory, size_t *pFreeMemory )
	{
	}
};

static CMemAlloc s_MemAlloc;
DLL_EXPORT_ATTR SELECTANY IMemAlloc *g_pMemAlloc = &s_MemAlloc;

#endif
