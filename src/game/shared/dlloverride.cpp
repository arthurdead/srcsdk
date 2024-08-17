#include "filesystem.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
extern const char *UTIL_VarArgs( PRINTF_FORMAT_STRING const char *format, ... ) FMTFUNCTION( 1, 2 );
#else
extern char	*VarArgs( PRINTF_FORMAT_STRING const char *format, ... );
#endif

//#pragma message(FILE_LINE_STRING " !!FIXME!! replace all this with Sys_LoadGameModule")
static class DllOverride {
public:
	DllOverride() {
		Sys_LoadInterface("filesystem_stdio" DLL_EXT_STRING, FILESYSTEM_INTERFACE_VERSION, nullptr, (void **)&g_pFullFileSystem);
		const char *pGameDir = CommandLine()->ParmValue("-game", "hl2");
	#ifdef GAME_DLL
		pGameDir = UTIL_VarArgs("%s/bin", pGameDir);
	#else
		pGameDir = VarArgs("%s/bin", pGameDir);
	#endif
		g_pFullFileSystem->AddSearchPath(pGameDir, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
	}
} g_DllOverride;
