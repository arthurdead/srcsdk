#ifndef FLASHLIGHT_MOD_H
#define FLASHLIGHT_MOD_H

#pragma once

#include "hackmgr.h"
#include "materialsystem/imaterialsystem.h"

HACKMGR_API FlashlightStateMod_t &HackMgr_GetFlashlightMod(FlashlightState_t &flashlightState);
HACKMGR_API void HackMgr_RemoveFlashlightMod(FlashlightState_t &flashlightState);

#endif