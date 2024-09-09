#include "hackmgr/hackmgr.h"
#include "hackmgr_internal.h"
#include "tier1/KeyValues.h"
#include "createinterface.h"
#include "filesystem.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_API void DependOnHackMgr()
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

	if(!GetFilesystemInterfaceFactory()) {
		return;
	}

	int status = IFACE_OK;
	IFileSystem *pFileSystem = (IFileSystem *)GetFilesystemInterfaceFactory()(FILESYSTEM_INTERFACE_VERSION, &status);
	if(!pFileSystem || status != IFACE_OK) {
		return;
	}

	const char *pGameDir = CommandLine()->ParmValue("-game", "hl2");

	char szTargetPath[MAX_PATH];
	V_strncpy(szTargetPath, pGameDir, sizeof(szTargetPath));
	int len = V_strlen(szTargetPath);
	if(szTargetPath[len] != CORRECT_PATH_SEPARATOR &&
		szTargetPath[len] != INCORRECT_PATH_SEPARATOR) {
		V_strcat(szTargetPath, CORRECT_PATH_SEPARATOR_S "bin" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
	} else {
		V_strcat(szTargetPath, "bin" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
	}
	V_strcat(szTargetPath, "hackmgr.vdf", sizeof(szTargetPath));

	kv = new KeyValues("HackMgr");
	if(!kv->LoadFromFile(pFileSystem, szTargetPath, NULL)) {
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
