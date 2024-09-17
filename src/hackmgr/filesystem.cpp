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

char basePath[MAX_PATH];
int len = pFileSystem->GetSearchPath("BASE_PATH", false, basePath, sizeof(basePath));

if(basePath[len-1] == INCORRECT_PATH_SEPARATOR) {
	basePath[len-1] = CORRECT_PATH_SEPARATOR;
} else if(basePath[len-1] != CORRECT_PATH_SEPARATOR) {
	basePath[len++] = CORRECT_PATH_SEPARATOR;
	basePath[len] = '\0';
}

char platformPath[MAX_PATH];
V_sprintf_safe(platformPath, "%splatform" CORRECT_PATH_SEPARATOR_S, basePath);

pFileSystem->RemoveSearchPath(platformPath, "PLATFORM");

HACKMGR_EXECUTE_ON_LOAD_END
