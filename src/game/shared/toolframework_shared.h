#ifndef TOOLFRAMEWORK_SHARED_H
#define TOOLFRAMEWORK_SHARED_H

#pragma once

#include "toolframework/itoolentity.h"
#include "icliententitylist.h"
#include "iserverentitylist.h"

#ifdef GAME_DLL
#include "toolframework_server.h"
#else
#include "toolframework_client.h"
#endif

extern IServerTools	*servertools;
extern IServerToolsEx	*servertools_ex;
extern IClientTools	*clienttools;
extern IClientToolsEx	*clienttools_ex;

#endif
