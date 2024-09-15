//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "convar.h"
#include "ai_basenpc.h"
#include "ai_agent.h"
#include "tier1/strtools.h"
#include "ai_activity.h"
#include "ai_schedule.h"
#include "ai_default.h"
#ifndef AI_USES_NAV_MESH
#include "ai_hint.h"
#endif
#include "bitstring.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_SchedulesManager g_AI_SchedulesManager;
CAI_SchedulesManager g_AI_AgentSchedulesManager;

//-----------------------------------------------------------------------------
// Purpose:	Delete all the string registries
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_SchedulesManager::DestroyStringRegistries(void)
{
	CAI_Agent::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::gm_SquadSlotNamespace.Clear();

	delete CAI_BaseNPC::m_pActivitySR;
	CAI_BaseNPC::m_pActivitySR = NULL;
	CAI_BaseNPC::m_iNumActivities = 0;

	delete CAI_BaseNPC::m_pEventSR;
	CAI_BaseNPC::m_pEventSR = NULL;
	CAI_BaseNPC::m_iNumEvents = 0;
}

void CAI_SchedulesManager::CreateStringRegistries( void )
{
	CAI_Agent::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::GetSchedulingSymbols()->Clear();
	CAI_BaseNPC::gm_SquadSlotNamespace.Clear();

	CAI_BaseNPC::m_pActivitySR = new CStringRegistry();
	CAI_BaseNPC::m_pEventSR = new CStringRegistry();
}

//-----------------------------------------------------------------------------
// Purpose: Load all the schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitSchedulingTables()
{
	CAI_BaseNPC::gm_ClassScheduleIdSpace.Init( "CAI_BaseNPC", CAI_BaseNPC::GetSchedulingSymbols() );
	CAI_BaseNPC::InitDefaultScheduleSR();
	CAI_BaseNPC::InitDefaultConditionSR();
	CAI_BaseNPC::InitDefaultTaskSR();
	CAI_BaseNPC::InitDefaultActivitySR();
	CAI_BaseNPC::InitDefaultSquadSlotSR();
}

void CAI_Agent::InitSchedulingTables()
{
	CAI_Agent::gm_ClassScheduleIdSpace.Init( "CAI_Agent", CAI_Agent::GetSchedulingSymbols() );
	CAI_Agent::InitDefaultScheduleSR();
	CAI_Agent::InitDefaultConditionSR();
	CAI_Agent::InitDefaultTaskSR();
}

bool CAI_SchedulesManager::LoadAllSchedules(void)
{
	// If I haven't loaded schedules yet
	if (!CAI_SchedulesManager::allSchedules)
	{
		// Init defaults
		CAI_BaseNPC::InitSchedulingTables();
		CAI_Agent::InitSchedulingTables();

		if (!CAI_BaseNPC::LoadDefaultSchedules())
		{
			CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
			DevMsg("ERROR:  Mistake in default schedule definitions, AI Disabled.\n");
		}

		if (!CAI_Agent::LoadDefaultSchedules())
		{
			DevMsg("ERROR:  Mistake in default schedule definitions, AI Disabled.\n");
		}

// UNDONE: enable this after the schedules are all loaded (right now some load in monster spawns)
#if 0
		// If not in developer mode, free the string memory.  Otherwise
		// keep it around for debugging information
		if (!g_pDeveloper->GetInt())
		{
			ClearStringRegistries();
		}
#endif

	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Creates and returns schedule of the given name
//			This should eventually be replaced when we convert to
//			non-hard coded schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::CreateSchedule(const char *name, int schedule_id)
{
	// Allocate schedule
	CAI_Schedule *pSched = new CAI_Schedule(name,schedule_id,CAI_SchedulesManager::allSchedules);
	CAI_SchedulesManager::allSchedules = pSched;

	// Return schedule
	return pSched;
}

//-----------------------------------------------------------------------------
// Purpose: Given text name of a NPC state returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
NPC_STATE CAI_SchedulesManager::GetStateID(const char *state_name)
{
	if		(!stricmp(state_name,"NONE"))		{	return NPC_STATE_NONE;		}
	else if (!stricmp(state_name,"IDLE"))		{	return NPC_STATE_IDLE;		}
	else if (!stricmp(state_name,"COMBAT"))		{	return NPC_STATE_COMBAT;	}
	else if (!stricmp(state_name,"PRONE"))		{	return NPC_STATE_PRONE;		}
	else if (!stricmp(state_name,"ALERT"))		{	return NPC_STATE_ALERT;		}
	else if (!stricmp(state_name,"SCRIPT"))		{	return NPC_STATE_SCRIPT;	}
	else if (!stricmp(state_name,"PLAYDEAD"))	{	return NPC_STATE_PLAYDEAD;	}
	else if (!stricmp(state_name,"DEAD"))		{	return NPC_STATE_DEAD;		}
	else											return NPC_STATE_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: Given text name of a memory bit returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetMemoryID(const char *state_name)
{
	if		(!stricmp(state_name,"PROVOKED"))		{	return bits_MEMORY_PROVOKED;		}
	else if (!stricmp(state_name,"INCOVER"))		{	return bits_MEMORY_INCOVER;			}
	else if (!stricmp(state_name,"SUSPICIOUS"))		{	return bits_MEMORY_SUSPICIOUS;		}
	else if (!stricmp(state_name,"PATH_FAILED"))	{	return bits_MEMORY_PATH_FAILED;		}
	else if (!stricmp(state_name,"FLINCHED"))		{	return bits_MEMORY_FLINCHED;		}
	else if (!stricmp(state_name,"TOURGUIDE"))		{	return bits_MEMORY_TOURGUIDE;		}
#ifndef AI_USES_NAV_MESH
	else if (!stricmp(state_name,"LOCKED_HINT"))	{	return bits_MEMORY_LOCKED_HINT;		}
#else
	else if (!stricmp(state_name,"LOCKED_AREA"))	{	return bits_MEMORY_LOCKED_AREA;		}
#endif
	else if (!stricmp(state_name,"TURNING"))		{	return bits_MEMORY_TURNING;			}
	else if (!stricmp(state_name,"TURNHACK"))		{	return bits_MEMORY_TURNHACK;		}
	else if (!stricmp(state_name,"CUSTOM4"))		{	return bits_MEMORY_CUSTOM4;			}
	else if (!stricmp(state_name,"CUSTOM3"))		{	return bits_MEMORY_CUSTOM3;			}
	else if (!stricmp(state_name,"CUSTOM2"))		{	return bits_MEMORY_CUSTOM2;			}
	else if (!stricmp(state_name,"CUSTOM1"))		{	return bits_MEMORY_CUSTOM1;			}
	else												return MEMORY_CLEAR;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
pathType_t CAI_SchedulesManager::GetPathID( const char *token )
{
	if		( !stricmp( token, "TRAVEL" ) )	{	return PATH_TRAVEL;		}
	else if ( !stricmp( token, "LOS" ) )		{	return PATH_LOS;		}
	else if ( !stricmp( token, "FLANK" ) )		{	return PATH_FLANK;		}
	else if ( !stricmp( token, "FLANK_LOS" ) )	{	return PATH_FLANK_LOS;	}
	else if ( !stricmp( token, "COVER" ) )		{	return PATH_COVER;		}
	else if ( !stricmp( token, "COVER_LOS" ) )	{	return PATH_COVER_LOS;	}

	return PATH_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
goalType_t CAI_SchedulesManager::GetGoalID( const char *token )
{
	if		( !stricmp( token, "ENEMY" ) )			{	return GOAL_ENEMY;			}
	else if ( !stricmp( token, "ENEMY_LKP" ) )		{	return GOAL_ENEMY_LKP;		}
	else if ( !stricmp( token, "TARGET" ) )			{	return GOAL_TARGET;			}
	else if ( !stricmp( token, "SAVED_POSITION" ) )	{	return GOAL_SAVED_POSITION;	}

	return GOAL_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Read data on schedules
//			As I'm parsing a human generated file, give a lot of error output
// Output:  true  - if data successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------

#define SCHED_BUFFER_STRIDE 1024

static char sched_global_buffer[SCHED_BUFFER_STRIDE * 3];
static char *const sched_tempbuffer = sched_global_buffer;
static char *const sched_pSchedName = sched_global_buffer + SCHED_BUFFER_STRIDE;
static char *const sched_pCurrTaskName = sched_global_buffer + (SCHED_BUFFER_STRIDE * 2);

static int sched_tempSchedNum = 0;

static int sched_tempTaskNum = 0;
static Task_t sched_tempTask[50];
static CAI_ScheduleBits sched_tempInterruptMask;

template <typename T>
static bool schedule_parse_enum(
	const char *&pfile, const char *pclassname, const char *pfilename,
	const char *tag, const char *enum_prefix, bool with_prefix, T(*func)(const char *), T invalid_value,
	int dataNum, TaskDataType_t datatype, T (TaskData_t::*datavar)
	)
{
	int enum_prefix_len = V_strlen(enum_prefix);

	if (!stricmp(tag,sched_tempbuffer))
	{
		// Skip the ";", but make sure it's present
		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		if (stricmp(sched_tempbuffer,":"))
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

		T state = func(sched_tempbuffer);
		if(state == invalid_value)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = state;
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		return true;
	}
	else if(!strnicmp(sched_tempbuffer, enum_prefix, enum_prefix_len))
	{
		T state = func(with_prefix ? sched_tempbuffer : sched_tempbuffer+enum_prefix_len);
		if(state == invalid_value)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = state;
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		return true;
	}
	else
	{
		DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		Assert(0);
		return false;
	}
}

template <typename T, typename U>
static bool schedule_parse_namespace_id(
	const char *&pfile, const char *pclassname, const char *pfilename,
	const char *tag, const char *enum_prefix,
	T (CAI_GlobalScheduleNamespace::*symid)( const char *pszTask ) const, U (CAI_ClassScheduleIdSpace::*globtolocal)( T ) const,
	CAI_ClassScheduleIdSpace *pIdSpace, CAI_GlobalScheduleNamespace *pGlobalNamespace,
	int dataNum, TaskDataType_t datatype, T (TaskData_t::*datavar)
	)
{
	int enum_prefix_len = V_strlen(enum_prefix);

	if (!stricmp(tag,sched_tempbuffer))
	{
		// Skip the ";", but make sure it's present
		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		if (stricmp(sched_tempbuffer,":"))
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

		// Convert generic ID to sub-class specific enum
		T taskValueGlobalID = (pGlobalNamespace->*symid)( sched_tempbuffer );
		if(taskValueGlobalID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		U taskValueLocalID = (pIdSpace) ? (pIdSpace->*globtolocal)(taskValueGlobalID) : AI_RemapFromGlobal( taskValueGlobalID );
		if(taskValueLocalID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = taskValueLocalID;
		return true;
	}
	else if (!strnicmp(enum_prefix,sched_tempbuffer,enum_prefix_len))
	{
		// Convert generic ID to sub-class specific enum
		T taskValueGlobalID = (pGlobalNamespace->*symid)( sched_tempbuffer );
		if(taskValueGlobalID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		U taskValueLocalID = (pIdSpace) ? (pIdSpace->*globtolocal)(taskValueGlobalID) : AI_RemapFromGlobal( taskValueGlobalID );
		if(taskValueLocalID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			Assert(0);
			return false;
		}

		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = taskValueLocalID;
		return true;
	} else {
		DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		Assert(0);
		return false;
	}
}

/*
#ifndef AI_USES_NAV_MESH
	else if ( !stricmp( "HintFlags",token ) )
	{
		// Skip the ":", but make sure it's present
		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		if (stricmp(token,":"))
		{
			DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'HINTFLAG'\n",prefix,new_schedule->GetName());
			Assert(0);
			return false;
		}

		// Load the flags and make sure they are valid
		pfile = engine->ParseFile( pfile, token, sizeof( token ) );
		tempTask[taskNum].flTaskData = CAI_HintManager::GetFlags( token );
		if (tempTask[taskNum].flTaskData == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown hint flag type  %s!\n", prefix,new_schedule->GetName(), token);
			Assert(0);
			return false;
		}
	}
#endif
*/

static bool sched_parse_task_value(
	const char *&pfile, const char *pclassname, const char *pfilename,
	CAI_ClassScheduleIdSpace *pIdSpace, CAI_GlobalScheduleNamespace *pGlobalNamespace,
	const TaskParamCheck_t *paramCheck, int dataNum
	)
{
	if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_NUM) {
		int tag_len = 0;

		if (!stricmp("Int",sched_tempbuffer))
		{
			tag_len = 3;
		}
		else if (!stricmp("Float",sched_tempbuffer))
		{
			tag_len = 5;
		}

		if (tag_len > 0)
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Num' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		bool all_num = true;
		bool dot = false;

		int len = V_strlen(sched_tempbuffer);
		for(int i = 0; i < len; ++i) {
			if(sched_tempbuffer[i] == '.') {
				dot = true;
			} else if(sched_tempbuffer[i] < '0' || sched_tempbuffer[i] > '9') {
				all_num = false;
				break;
			}
		}

		if(all_num) {
			if(dot) {
				sched_tempTask[sched_tempTaskNum].data[dataNum].flData = V_atof(sched_tempbuffer);
				sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_FLOAT;
			} else {
				sched_tempTask[sched_tempTaskNum].data[dataNum].iData = V_atoi(sched_tempbuffer);
				sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_INT;
			}
			return true;
		} else {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Num' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_INT) {
		if (!stricmp("Int",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Int' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		bool all_num = true;

		int len = V_strlen(sched_tempbuffer);
		for(int i = 0; i < len; ++i) {
			if(sched_tempbuffer[i] < '0' || sched_tempbuffer[i] > '9') {
				all_num = false;
				break;
			}
		}

		if(all_num) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].iData = V_atoi(sched_tempbuffer);
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_INT;
			return true;
		} else {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Int' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_FLOAT) {
		if (!stricmp("Float",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Float' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		bool all_num = true;

		int len = V_strlen(sched_tempbuffer);
		for(int i = 0; i < len; ++i) {
			if((sched_tempbuffer[i] < '0' || sched_tempbuffer[i] > '9') && sched_tempbuffer[i] != '.') {
				all_num = false;
				break;
			}
		}

		if(all_num) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].flData = V_atof(sched_tempbuffer);
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_FLOAT;
			return true;
		} else {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Float' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_BOOL) {
		if (!stricmp("Bool",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Bool' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		if (!stricmp("True",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = true;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return true;
		} else if (!stricmp("False",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = false;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return true;
		} else if (!stricmp("1",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = true;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return true;
		} else if (!stricmp("0",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = false;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return true;
		} else {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Bool' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_STRING) {
		if (!stricmp("String",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'String' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		int len = V_strlen(sched_tempbuffer);
		if(len >= sizeof(TaskData_t::szStr)) {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): String '%s' is too large.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}

		V_strncpy(sched_tempTask[sched_tempTaskNum].data[dataNum].szStr, sched_tempbuffer, len);
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_STRING;
		return true;
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_NULL) {
		if(!stricmp("Void",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return true;
		} else if(!stricmp("Null",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return true;
		} else if(!stricmp("None",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return true;
		} else if(!stricmp("empty",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return true;
		} else {
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Null' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			Assert(0);
			return false;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_TASK_ID) {
		return schedule_parse_namespace_id(pfile, pclassname, pfilename,
			"Task", "TASK_",
			&CAI_GlobalScheduleNamespace::TaskSymbolToId, &CAI_ClassScheduleIdSpace::TaskGlobalToLocal,
			pIdSpace, pGlobalNamespace,
			dataNum, TASK_DATA_TASK_ID, &TaskData_t::taskId
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_SCHEDULE_ID) {
		return schedule_parse_namespace_id(pfile, pclassname, pfilename,
			"Schedule", "SCHED_",
			&CAI_GlobalScheduleNamespace::ScheduleSymbolToId, &CAI_ClassScheduleIdSpace::ScheduleGlobalToLocal,
			pIdSpace, pGlobalNamespace,
			dataNum, TASK_DATA_SCHEDULE_ID, &TaskData_t::schedId
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_ACTIVITY) {
		return schedule_parse_enum(pfile, pclassname, pfilename,
			"Activity", "ACT_", true,
			&CAI_BaseNPC::GetActivityID, ACT_INVALID,
			dataNum, TASK_DATA_ACTIVITY, &TaskData_t::activity
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_NPCSTATE) {
		return schedule_parse_enum(pfile, pclassname, pfilename,
			"State", "NPC_STATE_", false,
			&CAI_SchedulesManager::GetStateID, NPC_STATE_INVALID,
			dataNum, TASK_DATA_NPCSTATE, &TaskData_t::nNpcState
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_MEMORY_ID) {
		return schedule_parse_enum(pfile, pclassname, pfilename,
			"Memory", "MEMORY_", false,
			&CAI_SchedulesManager::GetMemoryID, 0,
			dataNum, TASK_DATA_MEMORY_ID, &TaskData_t::nMemoryId
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_PATH_TYPE) {
		return schedule_parse_enum(pfile, pclassname, pfilename,
			"Path", "PATH_", false,
			&CAI_SchedulesManager::GetPathID, PATH_NONE,
			dataNum, TASK_DATA_PATH_TYPE, &TaskData_t::pathType
		);
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_GOAL_TYPE) {
		return schedule_parse_enum(pfile, pclassname, pfilename,
			"Goal", "GOAL_", false,
			&CAI_SchedulesManager::GetGoalID, GOAL_NONE,
			dataNum, TASK_DATA_GOAL_TYPE, &TaskData_t::goalType
		);
	} else {
		DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) has invalid param check type on %i.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,dataNum);
		Assert(0);
		return false;
	}
}

bool CAI_SchedulesManager::LoadSchedules( const char *pclassname, const char *pStartFile, const char *pfilename, CAI_ClassScheduleIdSpace *pIdSpace, CAI_GlobalScheduleNamespace *pGlobalNamespace )
{
	bool free_file = false;

	if(!pStartFile && pfilename && pfilename[0] != '\0') {
		pStartFile = (const char *)UTIL_LoadFileForMe(pfilename, NULL, "MOD");
		if(!pStartFile) {
			DevMsg( "ERROR: LoadSchd (%s): failed to open file '%s'.\n",pclassname,pfilename);
			Assert(0);
			return false;
		}
		free_file = true;
	}

	struct free_file_on_end_t {
		~free_file_on_end_t() {
			if(free_file)
				UTIL_FreeFile((byte *)pStartFile);
		}
		const char *pStartFile;
		bool free_file;
	} free_file_on_end;
	free_file_on_end.pStartFile = pStartFile;
	free_file_on_end.free_file = free_file;

	if(!pfilename && pStartFile && pStartFile[0] != '\0') {
		pfilename = "MEMORY";
	}

	if(!pStartFile || pStartFile[0] == '\0' || !pfilename || pfilename[0] == '\0') {
		DevMsg( "ERROR: LoadSchd (%s): no file.\n",pclassname);
		Assert(0);
		return false;
	}

	const char *pfile = pStartFile;

	sched_tempSchedNum = 0;

	sched_global_buffer[0] = '\0';
	sched_tempbuffer[0] = '\0';
	sched_pSchedName[0] = '\0';

	pfile = engine->ParseFile(pfile, sched_pSchedName, SCHED_BUFFER_STRIDE );
	for(;;) {
		if(!stricmp("Schedule",sched_pSchedName))
		{
			pfile = engine->ParseFile(pfile, sched_pSchedName, SCHED_BUFFER_STRIDE );

			if(!stricmp("=",sched_pSchedName))
			{
				pfile = engine->ParseFile(pfile, sched_pSchedName, SCHED_BUFFER_STRIDE );
			}
		}

		if(strnicmp("SCHED_",sched_pSchedName,6))
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (#%i): Malformed, Expecting 'Schedule' got '%s' instead.\n",pclassname,pfilename,sched_tempSchedNum,sched_pSchedName);
			Assert(0);
			return false;
		}

		//TODO Arthurdead!!!
	#if 0
		if(!stricmp(sched_pSchedName,"SCHED_NONE"))
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s): '%s' (#%i) cannot be modified.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
			Assert(0);
			return false;
		}
	#endif

		SchedGlobalId_t scheduleID = pGlobalNamespace->ScheduleSymbolToId(sched_pSchedName);
		if (scheduleID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s): '%s' (#%i) was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
			Assert(0);
			return false;
		}

		//TODO Arthurdead!!!
	#if 0
		if(scheduleID == SCHED_NONE)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s): '%s' (#%i) cannot be modified.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
			Assert(0);
			return false;
		}
	#endif

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		if (stricmp(sched_tempbuffer,"{"))
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
			Assert(0);
			return false;
		}

		sched_tempInterruptMask.ClearAll();
		sched_tempTaskNum = 0;
		memset(sched_tempTask, 0, sizeof(sched_tempTask));

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		for(;;) {
			if (!stricmp(sched_tempbuffer,"Tasks")) {
				pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
				if (stricmp(sched_tempbuffer,"{"))
				{
					DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
					Assert(0);
					return false;
				}

				// ==========================
				// Now read in the tasks
				// ==========================
				// Store in temp array until number of tasks is known
				sched_pCurrTaskName[0] = '\0';

				pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
				for(;;) {
					if(strnicmp("TASK_",sched_pCurrTaskName,5))
					{
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Task' got '%s' instead.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
						Assert(0);
						return false;
					}

					// Convert generic ID to sub-class specific enum
					TaskGlobalId_t taskGlobalID = pGlobalNamespace->TaskSymbolToId( sched_pCurrTaskName );
					TaskLocalId_t taskLocalID = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskGlobalID) : AI_RemapFromGlobal( taskGlobalID );

					// If not a valid condition, send a warning message
					if (taskLocalID == -1)
					{
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum);
						Assert(0);
						return false;
					}

					Assert( AI_IdIsLocal( taskLocalID ) );

					sched_tempTask[sched_tempTaskNum].iTask = taskLocalID;

					const TaskParamCheck_t *paramCheck = (pIdSpace) ? pIdSpace->TaskParamsCheck( taskLocalID ) : pGlobalNamespace->TaskParamsCheck( taskGlobalID );
					if ( !paramCheck )
					{
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) has no params check.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum);
						Assert(0);
						return false;
					}

					sched_tempTask[sched_tempTaskNum].numData = paramCheck->numTotal;

					if(paramCheck->numTotal > 1) {
						bool parenthesis = false;

						pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						if(!stricmp("=",sched_tempbuffer)) {
							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

							if(stricmp("[",sched_tempbuffer)) {
								DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '[' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
								Assert(0);
								return false;
							}

							parenthesis = false;
						} else if(!stricmp("(",sched_tempbuffer)) {
							parenthesis = true;
						} else if(!stricmp("[",sched_tempbuffer)) {
							parenthesis = false;
						} else {
							DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '[' or '(' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
							Assert(0);
							return false;
						}

						pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						for(int i = 0; i < paramCheck->numTotal; ++i) {
							if(!sched_parse_task_value(pfile,pclassname,pfilename,
								pIdSpace,pGlobalNamespace,
								paramCheck, i
							)) {
								return false;
							}

							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

							if(!stricmp(",",sched_tempbuffer)) {
								pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
							}

							if(!stricmp(parenthesis ? ")" : "]",sched_tempbuffer)) {
								DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): %i parameters are expected but found %i.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,paramCheck->numTotal, i+1);
								Assert(0);
								return false;
							}
						}

						if(stricmp(parenthesis ? ")" : "]",sched_tempbuffer)) {
							DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,parenthesis?")":"]",sched_tempbuffer);
							Assert(0);
							return false;
						}

						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					} else if(paramCheck->numTotal == 1) {
						if(paramCheck[0].nTypes[0] != TASK_DATA_CHECK_NULL) {
							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
							if(!stricmp("=",sched_tempbuffer)) {
								pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
							}

							if(!sched_parse_task_value(
								pfile,pclassname,pfilename,
								pIdSpace,pGlobalNamespace,
								paramCheck, 0
							)) {
								return false;
							}

							pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
						} else {
							pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );

							bool equal = !stricmp("=",sched_pCurrTaskName);

							if(equal) {
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							}

							if(!stricmp("Void",sched_pCurrTaskName)) {
								sched_tempTask[sched_tempTaskNum].data[0].nType = TASK_DATA_NONE;
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							} else if(!stricmp("Null",sched_pCurrTaskName)) {
								sched_tempTask[sched_tempTaskNum].data[0].nType = TASK_DATA_NONE;
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							} else if(!stricmp("None",sched_pCurrTaskName)) {
								sched_tempTask[sched_tempTaskNum].data[0].nType = TASK_DATA_NONE;
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							} else if(!stricmp("empty",sched_pCurrTaskName)) {
								sched_tempTask[sched_tempTaskNum].data[0].nType = TASK_DATA_NONE;
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							} else if(equal) {
								DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Null' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
								Assert(0);
								return false;
							}
						}
					} else if(paramCheck->numTotal == 0 ) {
						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					}

					sched_tempTaskNum++;
					if(sched_tempTaskNum >= ARRAYSIZE(sched_tempTask)) {
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Task limit reached.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
						Assert(0);
						return false;
					}

					if (!stricmp(sched_pCurrTaskName,","))
					{
						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					}

					if (!stricmp(sched_pCurrTaskName,"}"))
					{
						break;
					}
				}

				pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			} else if (!stricmp(sched_tempbuffer,"Interrupts")) {
				pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
				if (stricmp(sched_tempbuffer,"{"))
				{
					DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
					Assert(0);
					return false;
				}

				// ==========================
				// Now read in the interrupts
				// ==========================
				sched_pCurrTaskName[0] = '\0';

				pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
				for(;;) {
					if(strnicmp("COND_",sched_pCurrTaskName,5))
					{
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Condition' got '%s' instead.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
						Assert(0);
						return false;
					}

					// Convert generic ID to sub-class specific enum
					AiCondGlobalId_t condID = pGlobalNamespace->ConditionSymbolToId(sched_pCurrTaskName);

					// If not a valid condition, send a warning message
					if (condID == -1)
					{
						DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Condition '%s' was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
						Assert(0);
						return false;
					}

					// Otherwise, add to this schedules list of conditions

					int interrupt = AI_RemapFromGlobal(condID);
					Assert( AI_IdIsGlobal( condID ) && interrupt >= 0 && interrupt < MAX_CONDITIONS );
					sched_tempInterruptMask.Set(interrupt);

					// Read the next token
					pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );

					if (!stricmp(sched_pCurrTaskName,","))
					{
						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					}

					if (!stricmp(sched_pCurrTaskName,"}"))
					{
						break;
					}
				}

				pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			} else {
				DevMsg( "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Tasks' or 'Interrupts' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
				Assert(0);
				return false;
			}

			if (!stricmp(sched_tempbuffer,"}"))
			{
				break;
			}
		}

		// -----------------------------
		// Check for duplicate schedule
		// -----------------------------
		int old_num = 0;

		CAI_Schedule *new_schedule = GetScheduleByName(sched_pSchedName);
		if(!new_schedule) {
			new_schedule = CreateSchedule(sched_pSchedName,scheduleID);

			old_num = 0;

			// Now copy the tasks into the new schedule
			new_schedule->m_iNumTasks = sched_tempTaskNum;
		} else {
			old_num = new_schedule->m_iNumTasks;

			new_schedule->m_iNumTasks += sched_tempTaskNum;
		}

		if(!new_schedule->m_pTaskList) {
			new_schedule->m_pTaskList = (Task_t *)malloc(sizeof(Task_t) * new_schedule->m_iNumTasks);
		} else {
			new_schedule->m_pTaskList = (Task_t *)realloc(new_schedule->m_pTaskList, sizeof(Task_t) * new_schedule->m_iNumTasks);
		}

		for (int i=old_num;i<new_schedule->m_iNumTasks;i++)
		{
			memcpy(&new_schedule->m_pTaskList[i], &sched_tempTask[i], sizeof(Task_t));

			Assert( AI_IdIsLocal( new_schedule->m_pTaskList[i].iTask ) );
		}

		CAI_ScheduleBits tmp;
		new_schedule->m_InterruptMask.Or(sched_tempInterruptMask, &tmp);
		new_schedule->m_InterruptMask = tmp;

		pfile = engine->ParseFile(pfile, sched_pSchedName, SCHED_BUFFER_STRIDE );

		if (sched_pSchedName[0] == '\0')
		{
			break;
		}

		sched_tempSchedNum++;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Given a schedule ID, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleFromID( int schedID )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (schedule->m_iScheduleID == schedID)
			return schedule;
	}

	DevMsg( "Couldn't find schedule (%s)\n", CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol(schedID) );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given a schedule name, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleByName( const char *name )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (FStrEq(schedule->GetName(),name))
			return schedule;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Delete all the schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_SchedulesManager::DeleteAllSchedules(void)
{
	m_CurLoadSig++;

	if ( m_CurLoadSig < 0 )
		m_CurLoadSig = 0;

	CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules;
	CAI_Schedule *next;

	while (schedule)
	{
		next = schedule->nextSchedule;
		delete schedule;
		schedule = next;
	}
	CAI_SchedulesManager::allSchedules = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------

CAI_Schedule::CAI_Schedule(const char *name, int schedule_id, CAI_Schedule *pNext)
{
	m_iScheduleID = schedule_id;

	int len = strlen(name);
	m_pName = new char[len+1];
	Q_strncpy(m_pName,name,len+1);

	m_pTaskList = NULL;
	m_iNumTasks = 0;

	// ---------------------------------
	//  Add to linked list of schedules
	// ---------------------------------
	nextSchedule = pNext;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAI_Schedule::~CAI_Schedule( void )
{
	delete[] m_pName;
	free(m_pTaskList);
}
