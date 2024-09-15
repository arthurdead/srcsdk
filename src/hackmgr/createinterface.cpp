#include "hackmgr/hackmgr.h"
#include "createinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CSysModule *engine_DLL = NULL;
static CreateInterfaceFn engine_createinterface = NULL;

static CSysModule *filesystem_DLL = NULL;
static CreateInterfaceFn filesystem_createinterface = NULL;

static CSysModule *launcher_DLL = NULL;
static CreateInterfaceFn launcher_createinterface = NULL;

static CSysModule *materials_DLL = NULL;
static CreateInterfaceFn materials_createinterface = NULL;

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
	return do_load(engine_createinterface, engine_DLL, "engine" DLL_EXT_STRING);
}

CreateInterfaceFn GetFilesystemInterfaceFactory()
{
	return do_load(filesystem_createinterface, filesystem_DLL, "filesystem_stdio" DLL_EXT_STRING);
}

CreateInterfaceFn GetLauncherInterfaceFactory()
{
	return do_load(launcher_createinterface, launcher_DLL, "launcher" DLL_EXT_STRING);
}

CreateInterfaceFn GetMaterialSystemInterfaceFactory()
{
	return do_load(materials_createinterface, materials_DLL, "materialsystem" DLL_EXT_STRING);
}

CreateInterfaceFn GetVstdlibInterfaceFactory()
{
#ifdef __linux__
	return do_load(vstdlib_createinterface, vstdlib_DLL, "libvstdlib" DLL_EXT_STRING);
#else
	return do_load(vstdlib_createinterface, vstdlib_DLL, "vstdlib" DLL_EXT_STRING);
#endif
}