#ifndef MATCHFRAMEWORK_H
#define MATCHFRAMEWORK_H

#pragma once

#include "matchmaking/imatchext.h"

class CMatchExt : public IMatchExt
{
public:
	// Get server map information for the session settings
	virtual KeyValues * GetAllMissions();
	virtual KeyValues * GetMapInfo( KeyValues *pSettings, KeyValues **ppMissionInfo = NULL );
	virtual KeyValues * GetMapInfoByBspName( KeyValues *pSettings, char const *szBspMapName, KeyValues **ppMissionInfo = NULL );
};

#endif
