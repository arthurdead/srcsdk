//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEPANEL_H
#define BASEPANEL_H
#pragma once

#include "basemodpanel.h"
inline BaseModUI::CBaseModPanel * BasePanel() { return &BaseModUI::CBaseModPanel::GetSingleton(); }

#endif // BASEPANEL_H
