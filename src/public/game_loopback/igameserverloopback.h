#ifndef IGAMESERVERLOOPBACK_H
#define IGAMESERVERLOOPBACK_H

#pragma once

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"

struct map_datamap_t;

#ifndef SWDS

#define GAMESERVERLOOPBACK_INTERFACE_VERSION "IGameServerLoopback001"

class IRecastMgr;

abstract_class IGameServerLoopback : public IAppSystem
{
public:
	// Way of accessing the server recast mesh on the client (for debugging/visualization)
	virtual IRecastMgr *GetRecastMgr() = 0;

	virtual map_datamap_t *GetMapDatamaps() = 0;

#ifdef _DEBUG
	virtual const char *GetEntityClassname( int entnum, int iSerialNum ) = 0;
#endif
};

#endif

#endif
