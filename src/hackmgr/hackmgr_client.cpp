#include "hackmgr/hackmgr.h"
#include "cdll_int.h"
#include "client_class.h"
#include "tier1/utlstring.h"
#include "dbg_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_API bool HackMgr_Client_PreInit(IBaseClientDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals)
{
	Install_HackMgrSpew();

	return true;
}
