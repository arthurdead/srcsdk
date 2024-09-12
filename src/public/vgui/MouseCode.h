//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: names mouse button inputs
// NOTE: Button codes also contain key codes, but we won't worry about that
//
// $NoKeywords: $
//===========================================================================//

#ifndef MOUSECODE_H
#define MOUSECODE_H

#pragma once

#include "inputsystem/ButtonCode.h"

namespace vgui
{
typedef ButtonCode_t MouseCode;

inline int MouseButtonBit(MouseCode code)
{
	if (code < MOUSE_FIRST || code > MOUSE_LAST) {
		Assert(false);
		return 0;
	}
	return 1 << (code - MOUSE_FIRST);
}
}

#endif // MOUSECODE_H
