//===== Copyright © 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef BITTOOLS_H
#define BITTOOLS_H
#pragma once

#include "tier0/platform.h"

#define FAST_BIT_SCAN 1

#ifdef _WIN32
#ifdef __MINGW32__
#include <immintrin.h>
#else
#include <intrin.h>
#endif
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif

namespace bittools
{
	template<int N, int C = 0>
	struct RecurseBit
	{
		enum {result = RecurseBit<N/2, C+1>::result};
	};
	
	template<int C>
	struct RecurseBit<0, C>
	{
		enum {result = C};
	};
	
	template<int N, int C = 1>
	struct RecursePow2
	{
		enum {result = RecursePow2<N/2, C*2>::result};
	};

	template<int C>
	struct RecursePow2<0, C>
	{
		enum {result = C};
	};
	
}

#define ROUND_TO_POWER_OF_2( n ) ( bittools::RecursePow2< (n) - 1 >::result )
#define MINIMUM_BITS_NEEDED( n ) ( bittools::RecurseBit< (n) - 1 >::result )

int UTIL_CountNumBitsSet( unsigned int nVar );
int UTIL_CountNumBitsSet( uint64 nVar );
inline int UTIL_CountNumBitsSet( unsigned short nVar )
{ return UTIL_CountNumBitsSet(static_cast<unsigned int>(nVar)); }
inline int UTIL_CountNumBitsSet( unsigned char nVar )
{ return UTIL_CountNumBitsSet(static_cast<unsigned int>(nVar)); }
inline int UTIL_CountNumBitsSet( bool nVar )
{ return nVar ? 1 : 0; }

unsigned int CountLeadingZeros(unsigned int x);
unsigned int CountTrailingZeros(unsigned int elem);

//-----------------------------------------------------------------------------
// Support functions
//-----------------------------------------------------------------------------

#define LOG2_BITS_PER_INT	5
#define BITS_PER_INT		32

int FirstBitInWord( unsigned int elem, int offset );

//-------------------------------------

inline unsigned GetEndMask( int numBits ) 
{ 
	static unsigned bitStringEndMasks[] = 
	{
		0xffffffff,
		0x00000001,
		0x00000003,
		0x00000007,
		0x0000000f,
		0x0000001f,
		0x0000003f,
		0x0000007f,
		0x000000ff,
		0x000001ff,
		0x000003ff,
		0x000007ff,
		0x00000fff,
		0x00001fff,
		0x00003fff,
		0x00007fff,
		0x0000ffff,
		0x0001ffff,
		0x0003ffff,
		0x0007ffff,
		0x000fffff,
		0x001fffff,
		0x003fffff,
		0x007fffff,
		0x00ffffff,
		0x01ffffff,
		0x03ffffff,
		0x07ffffff,
		0x0fffffff,
		0x1fffffff,
		0x3fffffff,
		0x7fffffff,
	};

	return bitStringEndMasks[numBits % BITS_PER_INT]; 
}

inline int GetBitForBitnum( int bitNum ) 
{ 
	static int bitsForBitnum[] = 
	{
		( 1 << 0 ),
		( 1 << 1 ),
		( 1 << 2 ),
		( 1 << 3 ),
		( 1 << 4 ),
		( 1 << 5 ),
		( 1 << 6 ),
		( 1 << 7 ),
		( 1 << 8 ),
		( 1 << 9 ),
		( 1 << 10 ),
		( 1 << 11 ),
		( 1 << 12 ),
		( 1 << 13 ),
		( 1 << 14 ),
		( 1 << 15 ),
		( 1 << 16 ),
		( 1 << 17 ),
		( 1 << 18 ),
		( 1 << 19 ),
		( 1 << 20 ),
		( 1 << 21 ),
		( 1 << 22 ),
		( 1 << 23 ),
		( 1 << 24 ),
		( 1 << 25 ),
		( 1 << 26 ),
		( 1 << 27 ),
		( 1 << 28 ),
		( 1 << 29 ),
		( 1 << 30 ),
		( 1 << 31 ),
	};

	return bitsForBitnum[ (bitNum) & (BITS_PER_INT-1) ]; 
}

inline int GetBitForBitnumByte( int bitNum ) 
{ 
	static int bitsForBitnum[] = 
	{
		( 1 << 0 ),
		( 1 << 1 ),
		( 1 << 2 ),
		( 1 << 3 ),
		( 1 << 4 ),
		( 1 << 5 ),
		( 1 << 6 ),
		( 1 << 7 ),
	};

	return bitsForBitnum[ bitNum & 7 ]; 
}

inline int CalcNumIntsForBits( int numBits )	{ return (numBits + (BITS_PER_INT-1)) / BITS_PER_INT; }

template <int bits> struct BitCountToEndMask_t { };
template <> struct BitCountToEndMask_t< 0> { enum { MASK = 0xffffffff }; };
template <> struct BitCountToEndMask_t< 1> { enum { MASK = 0x00000001 }; };
template <> struct BitCountToEndMask_t< 2> { enum { MASK = 0x00000003 }; };
template <> struct BitCountToEndMask_t< 3> { enum { MASK = 0x00000007 }; };
template <> struct BitCountToEndMask_t< 4> { enum { MASK = 0x0000000f }; };
template <> struct BitCountToEndMask_t< 5> { enum { MASK = 0x0000001f }; };
template <> struct BitCountToEndMask_t< 6> { enum { MASK = 0x0000003f }; };
template <> struct BitCountToEndMask_t< 7> { enum { MASK = 0x0000007f }; };
template <> struct BitCountToEndMask_t< 8> { enum { MASK = 0x000000ff }; };
template <> struct BitCountToEndMask_t< 9> { enum { MASK = 0x000001ff }; };
template <> struct BitCountToEndMask_t<10> { enum { MASK = 0x000003ff }; };
template <> struct BitCountToEndMask_t<11> { enum { MASK = 0x000007ff }; };
template <> struct BitCountToEndMask_t<12> { enum { MASK = 0x00000fff }; };
template <> struct BitCountToEndMask_t<13> { enum { MASK = 0x00001fff }; };
template <> struct BitCountToEndMask_t<14> { enum { MASK = 0x00003fff }; };
template <> struct BitCountToEndMask_t<15> { enum { MASK = 0x00007fff }; };
template <> struct BitCountToEndMask_t<16> { enum { MASK = 0x0000ffff }; };
template <> struct BitCountToEndMask_t<17> { enum { MASK = 0x0001ffff }; };
template <> struct BitCountToEndMask_t<18> { enum { MASK = 0x0003ffff }; };
template <> struct BitCountToEndMask_t<19> { enum { MASK = 0x0007ffff }; };
template <> struct BitCountToEndMask_t<20> { enum { MASK = 0x000fffff }; };
template <> struct BitCountToEndMask_t<21> { enum { MASK = 0x001fffff }; };
template <> struct BitCountToEndMask_t<22> { enum { MASK = 0x003fffff }; };
template <> struct BitCountToEndMask_t<23> { enum { MASK = 0x007fffff }; };
template <> struct BitCountToEndMask_t<24> { enum { MASK = 0x00ffffff }; };
template <> struct BitCountToEndMask_t<25> { enum { MASK = 0x01ffffff }; };
template <> struct BitCountToEndMask_t<26> { enum { MASK = 0x03ffffff }; };
template <> struct BitCountToEndMask_t<27> { enum { MASK = 0x07ffffff }; };
template <> struct BitCountToEndMask_t<28> { enum { MASK = 0x0fffffff }; };
template <> struct BitCountToEndMask_t<29> { enum { MASK = 0x1fffffff }; };
template <> struct BitCountToEndMask_t<30> { enum { MASK = 0x3fffffff }; };
template <> struct BitCountToEndMask_t<31> { enum { MASK = 0x7fffffff }; };

inline int BitForBitnum(int bitnum)
{
	return GetBitForBitnum(bitnum);
}

#endif //BITTOOLS_H

