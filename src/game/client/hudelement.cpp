#include "hudelement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma push_macro("new")
#pragma push_macro("delete")
#undef new
#undef delete

// memory handling, uses calloc so members are zero'd out on instantiation
void *CHudElement::operator new( size_t stAllocateBlock )	
{												
	Assert( stAllocateBlock != 0 );				
	void *pMem = malloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void* CHudElement::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )  
{ 
	Assert( stAllocateBlock != 0 );
	void *pMem = MemAlloc_Alloc( stAllocateBlock, pFileName, nLine );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void CHudElement::operator delete( void *pMem )				
{												
#if defined( _DEBUG )
	int size = _msize( pMem );					
	memset( pMem, 0xcd, size );					
#endif
	free( pMem );								
}

void CHudElement::operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )				
{
	operator delete( pMem );
}

#pragma pop_macro("delete")
#pragma pop_macro("new")
