//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Provides access to cvars that are bounded in the client DLL.
//
// $NoKeywords: $
//=============================================================================//

#ifndef CDLL_BOUNDED_CVARS_H
#define CDLL_BOUNDED_CVARS_H
#pragma once


#include "convar_serverbounded.h"


extern ConVar_ServerBounded *cl_predict;
extern ConVar_ServerBounded *cl_interp;

// Returns cl_interp_ratio / cl_updaterate.
float GetClientInterpAmount();


extern bool g_bForceCLPredictOff;	// If this is set to true, then prediction is forced off. Used temporarily for kill cam.



#endif // CDLL_BOUNDED_CVARS_H

