//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "tier0/platform.h"
#include "stdlib.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CDECL srand(unsigned int)
{
}

int CDECL rand()
{
	return RandomInt( 0, VALVE_RAND_MAX );
}

#endif // !_STATIC_LINKED || _SHARED_LIB
