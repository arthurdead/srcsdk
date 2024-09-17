#include "hackmgr/hackmgr.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DLL_EXPORT SELECTANY bool HushAsserts()
{
#ifdef DBGFLAG_ASSERT
	static bool s_bHushAsserts = !!CommandLine()->FindParm( "-hushasserts" );
	return s_bHushAsserts;
#else
	return true;
#endif
}
