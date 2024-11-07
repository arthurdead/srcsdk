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

#include "cdll_int.h"
#include "IGameUIFuncs.h"
#include "ienginevgui.h"

extern IVEngineClient *engine;
extern IGameUIFuncs *gameuifuncs;
extern IEngineVGui *enginevguifuncs;

#endif // ENGINEINTERFACE_H
