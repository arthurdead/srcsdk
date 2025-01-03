#include "hackmgr/hackmgr.h"
#include "createinterface.h"
#include "eiface.h"
#include "dlloverride_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CSysModule *engine_DLL = NULL;
static CreateInterfaceFn engine_createinterface = NULL;

static CSysModule *filesystem_DLL = NULL;
static CreateInterfaceFn filesystem_createinterface = NULL;

static CSysModule *launcher_DLL = NULL;
static CreateInterfaceFn launcher_createinterface = NULL;

#ifndef SWDS
static CSysModule *materials_DLL = NULL;
static CreateInterfaceFn materials_createinterface = NULL;
#endif

static CSysModule *vstdlib_DLL = NULL;
static CreateInterfaceFn vstdlib_createinterface = NULL;

static CreateInterfaceFn do_load(CreateInterfaceFn &func, CSysModule *&dll, const char *name)
{
	if(!func) {
		dll = Sys_LoadModule(name);
		if(dll) {
			func = Sys_GetFactory(dll);
		}
	}
	return func;
}

CreateInterfaceFn GetEngineInterfaceFactory()
{
	if(!IsDedicatedServer()) {
		return do_load(engine_createinterface, engine_DLL, "engine" DLL_EXT_STRING);
	} else {
		return do_load(engine_createinterface, engine_DLL, "engine_srv" DLL_EXT_STRING);
	}
}

CreateInterfaceFn GetFilesystemInterfaceFactory()
{
	if(!IsDedicatedServer()) {
		return do_load(filesystem_createinterface, filesystem_DLL, "filesystem_stdio" DLL_EXT_STRING);
	} else {
		if( !filesystem_createinterface ) {
			CAppSystemGroup *ParentAppSystemGroup = GetLauncherAppSystem();
			if( !ParentAppSystemGroup )
				return NULL;

			filesystem_DLL = NULL;
			filesystem_createinterface = Launcher_AppSystemCreateInterface;
		}

		return filesystem_createinterface;
	}
}

CreateInterfaceFn GetLauncherInterfaceFactory()
{
	if(!IsDedicatedServer()) {
		return do_load(launcher_createinterface, launcher_DLL, "launcher" DLL_EXT_STRING);
	} else {
		return do_load(launcher_createinterface, launcher_DLL, "dedicated_srv" DLL_EXT_STRING);
	}
}

#ifndef SWDS
CreateInterfaceFn GetMaterialSystemInterfaceFactory()
{
	return do_load(materials_createinterface, materials_DLL, "materialsystem" DLL_EXT_STRING);
}
#endif

CreateInterfaceFn GetVstdlibInterfaceFactory()
{
	if(!IsDedicatedServer()) {
	#ifdef __linux__
		return do_load(vstdlib_createinterface, vstdlib_DLL, "libvstdlib" DLL_EXT_STRING);
	#else
		return do_load(vstdlib_createinterface, vstdlib_DLL, "vstdlib" DLL_EXT_STRING);
	#endif
	} else {
	#ifdef __linux__
		return do_load(vstdlib_createinterface, vstdlib_DLL, "libvstdlib_srv" DLL_EXT_STRING);
	#else
		return do_load(vstdlib_createinterface, vstdlib_DLL, "vstdlib_srv" DLL_EXT_STRING);
	#endif
	}
}

bool IsDedicatedServer()
{
	return false;
}
