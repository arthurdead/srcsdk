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
pFileSystem->GetSearchPath("BASE_PATH", false, basePath, sizeof(basePath));
V_AppendSlash(basePath, sizeof(basePath));
V_strcat(basePath, "platform", sizeof(basePath));
V_AppendSlash(basePath, sizeof(basePath));

pFileSystem->RemoveSearchPath(basePath, "PLATFORM");

HACKMGR_EXECUTE_ON_LOAD_END
