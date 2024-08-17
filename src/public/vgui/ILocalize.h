//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ILOCALIZE_H
#define ILOCALIZE_H

#pragma once

#include "tier1/ilocalize.h"

namespace vgui
{
	class ILocalize : public ::ILocalize { };		// backwards compatability with vgui::ILocalize declarations
}

#define VGUI_LOCALIZE_INTERFACE_VERSION "VGUI_Localize005"

#endif // ILOCALIZE_H
