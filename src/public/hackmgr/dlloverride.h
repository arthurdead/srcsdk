#ifndef HACKMGR_DLLOVERRIDE_H
#define HACKMGR_DLLOVERRIDE_H

#pragma once

#include "hackmgr.h"

class IVideoServices;
class IPhysics;

HACKMGR_API bool HackMgr_IsSafeToSwapVideoServices();
HACKMGR_API void HackMgr_SetEngineVideoServicesPtr(IVideoServices *pOldInter, IVideoServices *pNewInter);

HACKMGR_API bool HackMgr_IsSafeToSwapPhysics();
HACKMGR_API void HackMgr_SetEnginePhysicsPtr(IPhysics *pOldInter, IPhysics *pNewInter);

#endif