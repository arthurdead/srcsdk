//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Sets up the tasks for default AI.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "stringregistry.h"
#include "ai_basenpc.h"
#include "ai_task.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char * g_ppszTaskFailureText[] =
{
	"No failure",                                    // NO_TASK_FAILURE
	"No Target",                                     // FAIL_NO_TARGET
	"Weapon owned by someone else",                  // FAIL_WEAPON_OWNED
	"Weapon/Item doesn't exist",                     // FAIL_ITEM_NO_FIND
	"Schedule not found",                            // FAIL_SCHEDULE_NOT_FOUND
	"Don't have an enemy",                           // FAIL_NO_ENEMY
	"Found no backaway node",                        // FAIL_NO_BACKAWAY_NODE
	"Couldn't find cover",                           // FAIL_NO_COVER
	"Couldn't find flank",                           // FAIL_NO_FLANK
	"Couldn't find shoot position",                  // FAIL_NO_SHOOT
	"Don't have a route",                            // FAIL_NO_ROUTE
	"Don't have a route: no goal",                   // FAIL_NO_ROUTE_GOAL
	"Don't have a route: blocked",                   // FAIL_NO_ROUTE_BLOCKED
	"Don't have a route: illegal move",              // FAIL_NO_ROUTE_ILLEGAL
	"Couldn't walk to target",                       // FAIL_NO_WALK
	"Node already locked",                           // FAIL_ALREADY_LOCKED
	"No sound present",                              // FAIL_NO_SOUND
	"No scent present",                              // FAIL_NO_SCENT
	"Bad activity",                                  // FAIL_BAD_ACTIVITY
	"No goal entity",                                // FAIL_NO_GOAL
	"No player",                                     // FAIL_NO_PLAYER
	"Can't reach any nodes",                         // FAIL_NO_REACHABLE_NODE
	"No AI Network to Use",                          // FAIL_NO_AI_NETWORK
	"Bad position to Target",                        // FAIL_BAD_POSITION
	"Route Destination No Longer Valid",             // FAIL_BAD_PATH_GOAL
	"Stuck on top of something",                     // FAIL_STUCK_ONTOP
	"Item has been taken",		                     // FAIL_ITEM_TAKEN
	"Too frozen",									 // FAIL_FROZEN
	"UNIMPLEMENTED!!!!"
};

COMPILE_TIME_ASSERT(ARRAYSIZE(g_ppszTaskFailureText) == NUM_FAIL_CODES);

const char *TaskFailureToString( AI_TaskFailureCode_t code )
{
	const char *pszResult;
	if ( code < 0 || code >= NUM_FAIL_CODES )
		pszResult = (const char *)code;
	else
		pszResult = g_ppszTaskFailureText[code];
	return pszResult;
}

//-----------------------------------------------------------------------------
// Purpose: Given and task name, return the task ID
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetTaskID(const char* taskName)
{
	return GetSchedulingSymbols()->TaskSymbolToId( taskName );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the task name string registry
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitDefaultTaskSR(void)
{
	#define ADD_DEF_TASK( name, params ) idSpace.AddTask(#name, name, params, "CAI_BaseNPC" )
	#define ADD_DEF_TASK_ALIAS( sname, ename, params ) idSpace.AddTask(#sname, ename, params, "CAI_BaseNPC" )

	CAI_ClassScheduleIdSpace &idSpace = CAI_BaseNPC::AccessClassScheduleIdSpaceDirect();

	#define AI_TASK_ENUM(name, params, ...) \
		ADD_DEF_TASK( name, params );

	#include "ai_default_task_enum.inc"
}
