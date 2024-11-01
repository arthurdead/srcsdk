#include "hackmgr/hackmgr.h"
#include "eiface.h"
#include "server_class.h"
#include "tier1/utlstring.h"
#include "dbg_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_API bool HackMgr_Server_PreInit(IServerGameDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals, bool bDedicated)
{
	Install_HackMgrSpew();

	return true;
}
