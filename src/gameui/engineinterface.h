//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Includes all the headers/declarations necessary to access the
//			engine interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENGINEINTERFACE_H
#define ENGINEINTERFACE_H

#pragma once

// these stupid set of includes are required to use the cdll_int interface
#include "mathlib/vector.h"
//#include "wrect.h"

// engine interface
#include "cdll_int.h"
#include "eiface.h"
#include "icvar.h"
#include "tier2/tier2.h"
#include "matchmaking/imatchframework.h"
#include "matchmaking/imatchext.h"

class IEngineVGui;
class IGameUIFuncs;
class IEngineSound;

// engine interface singleton accessors
extern IVEngineClient *engine;
extern IEngineVGui *enginevguifuncs;
extern IGameUIFuncs *gameuifuncs;
extern IEngineSound *enginesound;
extern IAchievementMgr *achievementmgr; 
extern CSteamAPIContext *steamapicontext;
extern IMatchExt *g_pMatchExt;

#endif // ENGINEINTERFACE_H
