//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef DT_UTLVECTOR_COMMON_H
#define DT_UTLVECTOR_COMMON_H
#pragma once


#include "utlvector.h"


typedef void (*EnsureCapacityFn)( void *pVoid, int offsetToUtlVector, int len );
typedef void (*ResizeUtlVectorFn)( void *pVoid, int offsetToUtlVector, int len );

template< class T >
void UtlVector_InitializeAllocatedElements( T *pBase, int count )
{
	memset( reinterpret_cast<void*>( pBase ), 0, count * sizeof( T ) );
}

template< class T, class A >
class UtlVectorTemplate
{
public:
	static void ResizeUtlVector( void *pStruct, int offsetToUtlVector, int len )
	{
		CUtlVector<T,A> *pVec = (CUtlVector<T,A>*)((char*)pStruct + offsetToUtlVector);
		if ( pVec->Count() < len )
			pVec->AddMultipleToTail( len - pVec->Count() );
		else if ( pVec->Count() > len )
			pVec->RemoveMultiple( len, pVec->Count()-len );

		// Ensure capacity
		pVec->EnsureCapacity( len );

		int nNumAllocated = pVec->NumAllocated();

		// This is important to do because EnsureCapacity doesn't actually call the constructors
		// on the elements, but we need them to be initialized, otherwise it'll have out-of-range
		// values which will piss off the datatable encoder.
		UtlVector_InitializeAllocatedElements( pVec->Base() + pVec->Count(), nNumAllocated - pVec->Count() );
	}

	static void EnsureCapacity( void *pStruct, int offsetToUtlVector, int len )
	{
		CUtlVector<T,A> *pVec = (CUtlVector<T,A>*)((char*)pStruct + offsetToUtlVector);

		pVec->EnsureCapacity( len );
		
		int nNumAllocated = pVec->NumAllocated();

		// This is important to do because EnsureCapacity doesn't actually call the constructors
		// on the elements, but we need them to be initialized, otherwise it'll have out-of-range
		// values which will piss off the datatable encoder.
		UtlVector_InitializeAllocatedElements( pVec->Base() + pVec->Count(), nNumAllocated - pVec->Count() );
	}
};

template< class T, class A >
inline ResizeUtlVectorFn GetResizeUtlVectorTemplate( CUtlVector<T,A> &vec )
{
	return &UtlVectorTemplate<T,A>::ResizeUtlVector;
}

template< class T, class A >
inline EnsureCapacityFn GetEnsureCapacityTemplate( CUtlVector<T,A> &vec )
{
	return &UtlVectorTemplate<T,A>::EnsureCapacity;
}


#endif // DT_UTLVECTOR_COMMON_H
