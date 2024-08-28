#include "filesystem.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void add_game_bin_folder()
{
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
	DebuggerBreak();
	if(num > 0) {
		const char *p = szSearchPaths;
		const char *lastp = p;
		while(*p != '\0') {
			if(*p == ';') {
				DebuggerBreak();
				if(V_strnicmp(lastp, szTargetPath, p-lastp) == 0) {
					found = true;
					break;
				}
				lastp = p+1;
			}

			++p;
		}
	}

	if(!found) {
		pFileSystem->AddSearchPath(szTargetPath, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
	}
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
[[using gnu: constructor(0)]] void call_add_game_bin_folder()
{
	add_game_bin_folder();
}
#pragma GCC diagnostic pop
#else
class call_add_game_bin_folder_t final
{
public:
	call_add_game_bin_folder_t()
	{ add_game_bin_folder(); }
};

call_add_game_bin_folder_t call_add_game_bin_folder;
#endif
