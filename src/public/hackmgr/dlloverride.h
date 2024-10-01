#ifndef HACKMGR_DLLOVERRIDE_H
#define HACKMGR_DLLOVERRIDE_H

#pragma once

#include "hackmgr.h"

#ifndef SWDS
class IVideoServices;
#endif
class IPhysics;

#ifndef SWDS
HACKMGR_API bool HackMgr_IsSafeToSwapVideoServices();
HACKMGR_API void HackMgr_SetEngineVideoServicesPtr(IVideoServices *pOldInter, IVideoServices *pNewInter);
#endif

HACKMGR_API bool HackMgr_IsSafeToSwapPhysics();
HACKMGR_API void HackMgr_SetEnginePhysicsPtr(IPhysics *pOldInter, IPhysics *pNewInter);

#if defined CLIENT_DLL || defined GAME_DLL
extern void HackMgr_SwapVphysics( CreateInterfaceFn &physicsFactory, CreateInterfaceFn appFactory, CSysModule *&vphysicsDLL );
#endif

#if defined CLIENT_DLL || defined GAMEUI_EXPORTS || defined GAMEPADUI_DLL
extern void HackMgr_SwapVideoServices( CreateInterfaceFn appFactory, CSysModule *&videoServicesDLL );
#endif

#endif