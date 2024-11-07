#include "tier0/platform.h"
#include "hackmgr/engine_target.h"

#include "steam/steam_api_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if HACKMGR_ENGINE_TARGET == HACKMGR_ENGINE_TARGET_SDK2013SP

#ifdef _WIN32

DLL_EXPORT void *S_CALLTYPE SteamInternal_ContextInit( void *pContextInitData )
{
	return NULL;
}

DLL_EXPORT void *S_CALLTYPE SteamInternal_CreateInterface( const char *ver )
{
	DebuggerBreak();
	return NULL;
}

DLL_EXPORT void *S_CALLTYPE SteamInternal_FindOrCreateUserInterface( HSteamUser hSteamUser, const char *pszVersion )
{
	DebuggerBreak();
	return NULL;
}

DLL_EXPORT void *S_CALLTYPE SteamInternal_FindOrCreateGameServerInterface( HSteamUser hSteamUser, const char *pszVersion )
{
	DebuggerBreak();
	return NULL;
}

#endif

#endif
