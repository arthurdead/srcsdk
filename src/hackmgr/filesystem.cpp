#include "hackmgr/hackmgr.h"
#include "filesystem.h"
#include "createinterface.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_GAME_PATHS 20

IFileSystem *GetFilesystem()
{
	if(!GetFilesystemInterfaceFactory()) {
		return NULL;
	}

	int status = IFACE_OK;
	IFileSystem *pFileSystem = (IFileSystem *)GetFilesystemInterfaceFactory()(FILESYSTEM_INTERFACE_VERSION, &status);
	if(!pFileSystem || status != IFACE_OK) {
		return NULL;
	}

#ifdef __MINGW32__
	g_pFullFileSystem = pFileSystem;
	g_pBaseFileSystem = (IBaseFileSystem *)pFileSystem->QueryInterface( BASEFILESYSTEM_INTERFACE_VERSION );
#endif

	return pFileSystem;
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

IFileSystem *pFileSystem = GetFilesystem();
if(!pFileSystem)
	return;

char basePath[MAX_PATH];
pFileSystem->GetSearchPath("BASE_PATH", false, basePath, sizeof(basePath));
V_AppendSlash(basePath, sizeof(basePath));
V_strcat(basePath, "platform", sizeof(basePath));
V_AppendSlash(basePath, sizeof(basePath));

pFileSystem->RemoveSearchPath(basePath, "PLATFORM");

//TODO!!!!! hate all this :)
char gamePath[MAX_PATH * MAX_GAME_PATHS];
pFileSystem->GetSearchPath("GAME", false, gamePath, sizeof(gamePath));

int last_len = 0;
int i = 0;
for(;; ++i) {
	if(gamePath[i] == ';') {
		gamePath[i] = '\0';
		const char *path = gamePath+last_len;
		pFileSystem->AddSearchPath(path, "GAME_NOBSP", PATH_ADD_TO_TAIL);
		last_len = i+1;
	} else if(gamePath[i] == '\0') {
		break;
	}
}

const char *path = gamePath+last_len;
if(path[0] != '\0') {
	pFileSystem->AddSearchPath(path, "GAME_NOBSP", PATH_ADD_TO_TAIL);
}

HACKMGR_EXECUTE_ON_LOAD_END
