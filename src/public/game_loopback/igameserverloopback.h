#ifndef IGAMESERVERLOOPBACK_H
#define IGAMESERVERLOOPBACK_H

#pragma once

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"

#ifndef SWDS

#define GAMESERVERLOOPBACK_INTERFACE_VERSION "IGameServerLoopback001"

class IRecastMgr;

abstract_class IGameServerLoopback : public IAppSystem
{
public:
	// Way of accessing the server recast mesh on the client (for debugging/visualization)
	virtual IRecastMgr *GetRecastMgr() = 0;
};

#endif

#endif
