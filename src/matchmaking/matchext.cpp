#include "matchext.h"
#include "tier1/interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMatchExt g_MatchExt;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMatchExt, IMatchExt, IMATCHEXT_INTERFACE, g_MatchExt);

KeyValues * CMatchExt::GetAllMissions()
{
	return NULL;
}

KeyValues * CMatchExt::GetMapInfo( KeyValues *pSettings, KeyValues **ppMissionInfo )
{
	return NULL;
}

KeyValues * CMatchExt::GetMapInfoByBspName( KeyValues *pSettings, char const *szBspMapName, KeyValues **ppMissionInfo )
{
	return NULL;
}

KeyValues * CMatchExt::GetMissionDetails( const char *szMissionName )
{
	return NULL;
}