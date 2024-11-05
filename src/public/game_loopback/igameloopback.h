#ifndef IGAMELOOPBACK_H
#define IGAMELOOPBACK_H

#pragma once

#include "tier0/platform.h"
#include "appframework/IAppSystem.h"

#ifndef SWDS

#define GAMELOOPBACK_INTERFACE_VERSION "IGameLoopback001"

abstract_class IGameLoopback : public IAppSystem
{
public:

};

#endif

#endif
