//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Default schedules.
//
//=============================================================================//

#ifndef AI_DEFAULT_H
#define AI_DEFAULT_H
#pragma once

//=========================================================
// These are the schedule types
//=========================================================
enum 
{
	#define AI_SCHED_ENUM(name, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "ai_default_sched_enum.inc"

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_SCHEDULE

};

#endif // AI_DEFAULT_H
