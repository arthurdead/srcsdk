//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef BONETOWORLDARRAY_H
#define BONETOWORLDARRAY_H
#pragma once

#include "tier0/tslist.h"
#include "mathlib/mathlib.h"
#include "studio.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
template <int NUM_ARRAYS>
class CBoneToWorldArrays
{
public:
	enum
	{
		ALIGNMENT = 128,
	};

	CBoneToWorldArrays();

	~CBoneToWorldArrays();

	int NumArrays();

	matrix3x4_t *Alloc( bool bBlock = true );

	void Free( matrix3x4_t *p );

private:
	CTSListBase m_Free;
	matrix3x4_t *m_pBase;
};

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template <int NUM_ARRAYS>
CBoneToWorldArrays<NUM_ARRAYS>::CBoneToWorldArrays()
{
	const int SIZE_ARRAY = AlignValue( sizeof(matrix3x4_t) * MAXSTUDIOBONES, ALIGNMENT );
	m_pBase = (matrix3x4_t *)_aligned_malloc( SIZE_ARRAY * NUM_ARRAYS, ALIGNMENT );
	for ( int i = 0; i < NUM_ARRAYS; i++ )
	{
		matrix3x4_t *pArray = (matrix3x4_t *)((byte *)m_pBase + SIZE_ARRAY * i);
		Assert( (size_t)pArray % ALIGNMENT == 0 );
		Free( pArray );
	}
}

template <int NUM_ARRAYS>
CBoneToWorldArrays<NUM_ARRAYS>::~CBoneToWorldArrays()
{
	_aligned_free( m_pBase );
}

template <int NUM_ARRAYS>
int CBoneToWorldArrays<NUM_ARRAYS>::NumArrays()
{
	return NUM_ARRAYS;
}

template <int NUM_ARRAYS>
matrix3x4_t *CBoneToWorldArrays<NUM_ARRAYS>::Alloc( bool bBlock )
{
	TSLNodeBase_t *p;
	while ( ( p = m_Free.Pop() ) == NULL && bBlock )
	{
		ThreadPause();
	}
	return (matrix3x4_t *)p;
}

template <int NUM_ARRAYS>
void CBoneToWorldArrays<NUM_ARRAYS>::Free( matrix3x4_t *p )
{
	m_Free.Push( (TSLNodeBase_t *) p );
}

#include "tier0/memdbgoff.h"

#endif // BONETOWORLDARRAY_H
