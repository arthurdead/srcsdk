#include "hackmgr/hackmgr.h"
#include "hackmgr_internal.h"
#include "tier1/KeyValues.h"
#include "createinterface.h"
#include "filesystem.h"
#include "tier0/icommandline.h"
#include "filesystem_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_API void HackMgr_DependantModuleLoaded(const char *name)
{
}

static bool game_paused = false;

HACKMGR_API bool HackMgr_IsGamePaused()
{
	return game_paused;
}

HACKMGR_API void HackMgr_SetGamePaused(bool value)
{
	game_paused = value;
}

HACKMGR_INIT_PRIO(101) AddressManager addresses;

AddressManager::AddressManager()
{
	init();
}

AddressManager::~AddressManager()
{
	if(kv) {
		kv->deleteThis();
	}
}

void AddressManager::init()
{
	if(initialized)
		return;

	IFileSystem *pFileSystem = GetFilesystem();
	if(!pFileSystem)
		return;

	kv = new KeyValues("HackMgr");
	if(!kv->LoadFromFile(pFileSystem, "hackmgr.vdf", "GAMEBIN")) {
		kv->deleteThis();
		kv = NULL;
		return;
	}

	KeyValuesDumpAsDevMsg(kv, 0, 0);

	KeyValues *Functions = kv->FindKey("Functions");
	if(Functions) {
		KeyValues *it = Functions->GetFirstTrueSubKey();
		while(it) {
			__builtin_printf("%s\n", it->GetName());

			it = it->GetNextTrueSubKey();
		}
	}

	DebuggerBreak();

	initialized = true;
}

generic_func_t AddressManager::LookupFunction(const char *name)
{
	init();

	return nullptr;
}
