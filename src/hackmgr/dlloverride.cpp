#include "hackmgr/hackmgr.h"
#include "filesystem.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

IFileSystem *pFileSystem = NULL;
if(!Sys_LoadInterface("filesystem_stdio" DLL_EXT_STRING, FILESYSTEM_INTERFACE_VERSION, nullptr, (void **)&pFileSystem) || !pFileSystem) {
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

bool found = false;

char szSearchPaths[MAX_PATH * 2];
int num = pFileSystem->GetSearchPath("EXECUTABLE_PATH", false, szSearchPaths, ARRAYSIZE(szSearchPaths));
if(num > 0) {
	const char *p = szSearchPaths;
	const char *lastp = p;
	do {
		if(*p == ';' || *p == '\0') {
			if(V_strnicmp(lastp, szTargetPath, p-lastp) == 0) {
				found = true;
				break;
			}
			lastp = p+1;
		}
	} while(*p++ != '\0');
}

if(!found) {
	pFileSystem->AddSearchPath(szTargetPath, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
}

HACKMGR_EXECUTE_ON_LOAD_END
