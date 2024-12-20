//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_ACTIVITY_H
#define AI_ACTIVITY_H
#pragma once

#include "tier0/platform.h"
#include "studio.h"

enum Activity : unsigned short
{
	#define ACTIVITY_ENUM(name, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "ai_activity_enum.inc"

	// this is the end of the global activities, private per-monster activities start here.
	LAST_SHARED_ACTIVITY,

	#define ACTIVITY_ENUM_ALIAS(name, value) \
		name = value,

	#include "ai_activity_enum.inc"
};


#endif // AI_ACTIVITY_H

