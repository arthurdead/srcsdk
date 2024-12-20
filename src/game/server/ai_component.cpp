#include "cbase.h"
#include "ai_component.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma push_macro("new")
#pragma push_macro("delete")
#undef new
#undef delete

void *CAI_Component::operator new( size_t nBytes )
{
	MEM_ALLOC_CREDIT();
	void *pResult = MemAlloc_Alloc( nBytes );
	memset( pResult, 0, nBytes );
	return pResult;
};

void *CAI_Component::operator new( size_t nBytes, int nBlockUse, const char *pFileName, int nLine )
{
	MEM_ALLOC_CREDIT();
	void *pResult = MemAlloc_Alloc( nBytes, pFileName, nLine );
	memset( pResult, 0, nBytes );
	return pResult;
}

#pragma pop_macro("delete")
#pragma pop_macro("new")
