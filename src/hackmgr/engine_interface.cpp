#include "hackmgr/hackmgr.h"
#include "engine_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CreateInterfaceFn engine_createinterface = NULL;

CreateInterfaceFn GetEngineInterface()
{
	if(!engine_createinterface) {
		engine_createinterface = Sys_GetFactory("engine" DLL_EXT_STRING);
	}
	return engine_createinterface;
}
