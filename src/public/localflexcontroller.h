//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef LOCALFLEXCONTROLLER_H
#define LOCALFLEXCONTROLLER_H

#pragma once

#include "tier0/platform.h"

enum LocalFlexController_t : unsigned char;
UNORDEREDENUM_OPERATORS( LocalFlexController_t, unsigned char )

typedef LocalFlexController_t FlexWeight_t;

inline const LocalFlexController_t INVALID_FLEXCONTROLLER = (LocalFlexController_t)-1;

#endif	// LOCALFLEXCONTROLLER_H
