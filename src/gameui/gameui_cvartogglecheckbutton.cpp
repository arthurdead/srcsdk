//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "gameui_cvartogglecheckbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#undef CvarToggleCheckButton
template class vgui::CvarToggleCheckButton<CGameUIConVarRef>;
