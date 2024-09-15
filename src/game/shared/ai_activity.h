//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_ACTIVITY_H
#define AI_ACTIVITY_H
#pragma once

#define ACTIVITY_NOT_AVAILABLE		-1

typedef enum
{
	#define ACTIVITY_ENUM(name, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "ai_activity_enum.inc"

	#define ACTIVITY_ENUM_ALIAS(name, value) \
		name = value,

	#include "ai_activity_enum.inc"

	// this is the end of the global activities, private per-monster activities start here.
	LAST_SHARED_ACTIVITY,
} Activity;


#endif // AI_ACTIVITY_H

