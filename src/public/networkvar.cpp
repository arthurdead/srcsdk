//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "networkvar.h"

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)


bool g_bUseNetworkVars = true;

#endif

int InternalCheckDeclareClass( const char *pClassName, const char *pClassNameMatch, void *pTestPtr, void *pBasePtr )
{
	// This makes sure that casting from ThisClass to BaseClass works right. You'll get a compiler error if it doesn't
	// work at all, and you'll get a runtime error if you use multiple inheritance.
	Assert( pTestPtr == pBasePtr );
	
	// This is triggered by IMPLEMENT_SERVER_CLASS. It does DLLClassName::CheckDeclareClass( #DLLClassName ).
	// If they didn't do a DECLARE_CLASS in DLLClassName, then it'll be calling its base class's version
	// and the class names won't match.
	AssertMsg( V_strcmp(pClassName, pClassNameMatch) == 0, "V_strcmp(%s, %s) == 0", pClassName, pClassNameMatch );
	return 0;
}

