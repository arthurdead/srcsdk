#ifndef HACKMGR_SHADERSYSTEM_H
#define HACKMGR_SHADERSYSTEM_H

#pragma once

#include "hackmgr.h"

class IShaderDLLInternal;

HACKMGR_API void HackMgr_ToggleShaderDLLAsMod(IShaderDLLInternal *pModDLL, bool value);

#endif