#ifndef IGAMELOOPBACK_H
#define IGAMELOOPBACK_H

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"

#define GAMELOOPBACK_INTERFACE_VERSION "IGameLoopback001"

abstract_class IGameLoopback : public IAppSystem
{
public:

};

#define GAMECLIENTLOOPBACK_INTERFACE_VERSION "IGameClientLoopback001"

abstract_class IGameClientLoopback : public IAppSystem
{
public:

};

#define GAMESERVERLOOPBACK_INTERFACE_VERSION "IGameServerLoopback001"

class IRecastMgr;

abstract_class IGameServerLoopback : public IAppSystem
{
public:
	// Way of accessing the server recast mesh on the client (for debugging/visualization)
	virtual IRecastMgr *GetRecastMgr() = 0;
};

#endif
