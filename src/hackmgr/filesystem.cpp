#include "hackmgr/hackmgr.h"
#include "filesystem.h"
#include "createinterface.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

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
	V_strcat(szTargetPath, CORRECT_PATH_SEPARATOR_S "user" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
} else {
	V_strcat(szTargetPath, "user" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
}

//TODO!!! remove both config and platform

HACKMGR_EXECUTE_ON_LOAD_END
