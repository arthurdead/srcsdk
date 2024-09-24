//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEUI_CVARTOGGLECHECKBUTTON_H
#define GAMEUI_CVARTOGGLECHECKBUTTON_H
#pragma once

#include <vgui_controls/cvartogglecheckbutton.h>
#include "gameui_util.h"

extern template class vgui::CvarToggleCheckButton<CGameUIConVarRef>;

typedef vgui::CvarToggleCheckButton<CGameUIConVarRef> CGameUICvarToggleCheckButton;

#ifdef _DEBUG
#define CvarToggleCheckButton ****!!!USE_CGameUICvarToggleCheckButton!!!!***
#define CCvarToggleCheckButton ****!!!USE_CGameUICvarToggleCheckButton!!!!***
#endif

#endif // CVARTOGGLECHECKBUTTON_H
