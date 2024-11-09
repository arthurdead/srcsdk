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
#include "bitstring.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_SCHEDULE, "AI Schedule" );

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
			Log_Error(LOG_SCHEDULE,"ERROR:  Mistake in default schedule definitions, AI Disabled.\n");
			Assert(0);
		}

		if (!CAI_Agent::LoadDefaultSchedules())
		{
			Log_Error(LOG_SCHEDULE,"ERROR:  Mistake in default schedule definitions, AI Disabled.\n");
			Assert(0);
		}

// UNDONE: enable this after the schedules are all loaded (right now some load in monster spawns)
#if 0
		// If not in developer mode, free the string memory.  Otherwise
		// keep it around for debugging information
		if (!developer->GetInt())
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

#ifdef _DEBUG
#define AI_SCHEDULE_PARSER_STRICT_BOOL 1
#define AI_SCHEDULE_PARSER_STRICT_NULL 1
#define AI_SCHEDULE_PARSER_STRICT_NAMES 1
#define AI_SCHEDULE_PARSER_STRICT_REGISTRATION 0
#define AI_SCHEDULE_PARSER_STRICT_ENUM 0
#else
#define AI_SCHEDULE_PARSER_STRICT_BOOL 0
#define AI_SCHEDULE_PARSER_STRICT_NULL 0
#define AI_SCHEDULE_PARSER_STRICT_NAMES 0
#define AI_SCHEDULE_PARSER_STRICT_REGISTRATION 0
#define AI_SCHEDULE_PARSER_STRICT_ENUM 0
#endif

template <typename T>
static int schedule_parse_enum(
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
			Log_Error( LOG_SCHEDULE,"ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			return 0;
		}

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

	#if AI_SCHEDULE_PARSER_STRICT_NAMES == 1
		if(with_prefix && strnicmp(sched_tempbuffer, enum_prefix, enum_prefix_len)) {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting, '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			return 0;
		}
	#endif

		T state = func(sched_tempbuffer);
		if(state == invalid_value)
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		#if AI_SCHEDULE_PARSER_STRICT_ENUM == 1
			return 0;
		#endif
		}

		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = state;
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		if(state != invalid_value) {
			return 1;
		} else {
		#if AI_SCHEDULE_PARSER_STRICT_ENUM == 1
			return 0;
		#else
			return 2;
		#endif
		}
	}
	else if(!strnicmp(sched_tempbuffer, enum_prefix, enum_prefix_len))
	{
		T state = func(with_prefix ? sched_tempbuffer : sched_tempbuffer+enum_prefix_len);
		if(state == invalid_value)
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		#if AI_SCHEDULE_PARSER_STRICT_ENUM == 1
			return 0;
		#endif
		}

		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = state;
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		if(state != invalid_value) {
			return 1;
		} else {
		#if AI_SCHEDULE_PARSER_STRICT_ENUM == 1
			return 0;
		#else
			return 2;
		#endif
		}
	}
	else
	{
		Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		return 0;
	}
}

template <typename T, typename U>
static int schedule_parse_namespace_id(
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
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			return 0;
		}

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

	#if AI_SCHEDULE_PARSER_STRICT_NAMES == 1
		if(strnicmp(sched_tempbuffer, enum_prefix, enum_prefix_len)) {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting, '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			return 0;
		}
	#endif

		// Convert generic ID to sub-class specific enum
		T taskValueGlobalID = (pGlobalNamespace->*symid)( sched_tempbuffer );
		if(taskValueGlobalID == -1)
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
			return 0;
		#endif
		}

		U taskValueLocalID = -1;

		if(taskValueGlobalID != -1) {
			taskValueLocalID = (pIdSpace) ? (pIdSpace->*globtolocal)(taskValueGlobalID) : AI_RemapFromGlobal( taskValueGlobalID );
			if(taskValueLocalID == -1)
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
				return 0;
			#endif
			}
		}

		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = taskValueLocalID;
		if(taskValueLocalID != -1) {
			return 1;
		} else {
		#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
			return 0;
		#else
			return 2;
		#endif
		}
	}
	else if (!strnicmp(enum_prefix,sched_tempbuffer,enum_prefix_len))
	{
		// Convert generic ID to sub-class specific enum
		T taskValueGlobalID = (pGlobalNamespace->*symid)( sched_tempbuffer );
		if(taskValueGlobalID == -1)
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
			return false;
		#endif
		}

		U taskValueLocalID = -1;

		if(taskValueGlobalID != -1) {
			taskValueLocalID = (pIdSpace) ? (pIdSpace->*globtolocal)(taskValueGlobalID) : AI_RemapFromGlobal( taskValueGlobalID );
			if(taskValueLocalID == -1)
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Unknown '%s' '%s'.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
			#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
				return false;
			#endif
			}
		}

		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = datatype;
		(sched_tempTask[sched_tempTaskNum].data[dataNum].*datavar) = taskValueLocalID;
		if(taskValueLocalID != -1) {
			return 1;
		} else {
		#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
			return 0;
		#else
			return 2;
		#endif
		}
	} else {
		Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,tag,sched_tempbuffer);
		return 0;
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

static int sched_parse_task_value(
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
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Num' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				return 0;
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
			return 1;
		} else {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Num' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_INT) {
		if (!stricmp("Int",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Int' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				return 0;
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
			return 1;
		} else {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Int' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_FLOAT) {
		if (!stricmp("Float",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Float' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				return 0;
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
			return 1;
		} else {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Float' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_BOOL) {
		if (!stricmp("Bool",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'Bool' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				return 0;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		if (!stricmp("True",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = true;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return 1;
		} else if (!stricmp("False",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = false;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return 1;
		}
	#if AI_SCHEDULE_PARSER_STRICT_BOOL == 0
		else if (!stricmp("1",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = true;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return 1;
		} else if (!stricmp("0",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].bData = false;
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_BOOL;
			return 1;
		} else
	#endif
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Bool' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
		}
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_STRING) {
		if (!stricmp("String",sched_tempbuffer))
		{
			// Skip the ";", but make sure it's present
			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
			if (stricmp(sched_tempbuffer,":"))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting ':' after 'String' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
				return 0;
			}

			pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		}

		int len = V_strlen(sched_tempbuffer);
		if(len >= sizeof(TaskData_t::szStr)) {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): String '%s' is too large.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
		}

		V_strncpy(sched_tempTask[sched_tempTaskNum].data[dataNum].szStr, sched_tempbuffer, len);
		sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_STRING;
		return 1;
	} else if(paramCheck->nTypes[dataNum] == TASK_DATA_CHECK_NULL) {
		if(!stricmp("Void",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return 1;
		} else if(!stricmp("Null",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return 1;
		} else if(!stricmp("None",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return 1;
		} else if(!stricmp("empty",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return 1;
		} else
	#if AI_SCHEDULE_PARSER_STRICT_NULL == 0
		if(!stricmp("0",sched_tempbuffer)) {
			sched_tempTask[sched_tempTaskNum].data[dataNum].nType = TASK_DATA_NONE;
			return 1;
		} else
	#endif
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Null' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
			return 0;
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
		Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) has invalid param check type on %i.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,dataNum);
		return 0;
	}
}

bool CAI_SchedulesManager::LoadSchedules( const char *pclassname, const char *pStartFile, const char *pfilename, CAI_ClassScheduleIdSpace *pIdSpace, CAI_GlobalScheduleNamespace *pGlobalNamespace )
{
	bool free_file = false;

	if(!pStartFile && pfilename && pfilename[0] != '\0') {
		pStartFile = (const char *)UTIL_LoadFileForMe(pfilename, NULL, "MOD");
		if(!pStartFile) {
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s): failed to open file '%s'.\n",pclassname,pfilename);
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
		Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s): no file.\n",pclassname);
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

			//need to allow 'Error'
		#if AI_SCHEDULE_PARSER_STRICT_NAMES == 1 && 0
			if(strnicmp("SCHED_",sched_pSchedName,6))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (#%i): Malformed, Expecting 'Schedule' got '%s' instead.\n",pclassname,pfilename,sched_tempSchedNum,sched_pSchedName);
				return false;
			}
		#endif
		} else {
			if(strnicmp("SCHED_",sched_pSchedName,6))
			{
				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (#%i): Malformed, Expecting 'Schedule' got '%s' instead.\n",pclassname,pfilename,sched_tempSchedNum,sched_pSchedName);
				return false;
			}
		}

		//TODO Arthurdead!!!
	#if 0
		if(!stricmp(sched_pSchedName,"SCHED_NONE") || !stricmp(sched_pSchedName,"Error"))
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s): '%s' (#%i) cannot be modified.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
			return false;
		}
	#endif

		SchedGlobalId_t scheduleID = pGlobalNamespace->ScheduleSymbolToId(sched_pSchedName);
		if (scheduleID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s) (%s): '%s' (#%i) was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
		#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
			return false;
		#endif
		}
		//TODO Arthurdead!!!
	#if 0
		else if(scheduleID == SCHED_NONE)
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s): '%s' (#%i) cannot be modified.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
			return false;
		}
	#endif

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		if (stricmp(sched_tempbuffer,"{"))
		{
			Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
			return false;
		}

		sched_tempInterruptMask.ClearAll();
		sched_tempTaskNum = 0;
		memset((void *)sched_tempTask, 0, sizeof(sched_tempTask));

		pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
		bool first = true;
		for(;;) {
			if (!stricmp(sched_tempbuffer,"Tasks")) {
				pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
				if (stricmp(sched_tempbuffer,"{"))
				{
					Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
					return false;
				}

				// ==========================
				// Now read in the tasks
				// ==========================
				// Store in temp array until number of tasks is known
				sched_pCurrTaskName[0] = '\0';

				pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
				for(;;) {
				#if AI_SCHEDULE_PARSER_STRICT_NAMES == 1
					if(strnicmp("TASK_",sched_pCurrTaskName,5))
					{
						Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Task' got '%s' instead.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
						return false;
					}
				#endif

					// Convert generic ID to sub-class specific enum
					TaskGlobalId_t taskGlobalID = pGlobalNamespace->TaskSymbolToId( sched_pCurrTaskName );
					TaskLocalId_t taskLocalID = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskGlobalID) : AI_RemapFromGlobal( taskGlobalID );

					// If not a valid condition, send a warning message
					if (taskLocalID == -1)
					{
						Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum);
					#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
						return false;
					#endif
					}

					const TaskParamCheck_t *paramCheck = NULL;

					sched_tempTask[sched_tempTaskNum].iTask = taskLocalID;

					if (taskLocalID != -1)
					{
						Assert( AI_IdIsLocal( taskLocalID ) );

						if(pIdSpace) {
							paramCheck = pIdSpace->TaskParamsCheck( taskLocalID );
						}
						if(!paramCheck) {
							paramCheck = pGlobalNamespace->TaskParamsCheck( taskGlobalID );
						}
						if ( !paramCheck )
						{
							Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Task '%s' (#%i) has no params check.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum);
						#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
							return false;
						#endif
						}
					}

					sched_tempTask[sched_tempTaskNum].numData = paramCheck ? paramCheck->numTotal : 0;

					if(paramCheck && paramCheck->numTotal > 1) {
						bool parenthesis = false;

						pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						if(!stricmp("=",sched_tempbuffer)) {
							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

							if(stricmp("[",sched_tempbuffer)) {
								Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '[' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
								return false;
							}

							parenthesis = false;
						} else if(!stricmp("(",sched_tempbuffer)) {
							parenthesis = true;
						} else if(!stricmp("[",sched_tempbuffer)) {
							parenthesis = false;
						} else {
							Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '[' or '(' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
							return false;
						}

						pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						for(int i = 0; i < paramCheck->numTotal; ++i) {
							int parsed = sched_parse_task_value(pfile,pclassname,pfilename,
								pIdSpace,pGlobalNamespace,
								paramCheck, i
							);

						#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1 && AI_SCHEDULE_PARSER_STRICT_ENUM == 1
							if(parsed != 1) {
								return false;
							}
						#else
							if(parsed == 0) {
								return false;
							}
						#endif

							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );

							if(!stricmp(",",sched_tempbuffer)) {
								pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
							}

							if(!stricmp(parenthesis ? ")" : "]",sched_tempbuffer)) {
								Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): %i parameters are expected but found %i.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,paramCheck->numTotal, i+1);
								return false;
							}
						}

						if(stricmp(parenthesis ? ")" : "]",sched_tempbuffer)) {
							Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting '%s' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,parenthesis?")":"]",sched_tempbuffer);
							return false;
						}

						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					} else if(paramCheck && paramCheck->numTotal == 1 && paramCheck->nTypes[0] != TASK_DATA_CHECK_NULL) {
						pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						if(!stricmp("=",sched_tempbuffer)) {
							pfile = engine->ParseFile(pfile, sched_tempbuffer, SCHED_BUFFER_STRIDE );
						}

						int parsed = sched_parse_task_value(
							pfile,pclassname,pfilename,
							pIdSpace,pGlobalNamespace,
							paramCheck, 0
						);

					#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1 && AI_SCHEDULE_PARSER_STRICT_ENUM == 1
						if(parsed != 1) {
							return false;
						}
					#else
						if(parsed == 0) {
							return false;
						}
					#endif

						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
					} else if(!paramCheck || (paramCheck->numTotal == 0 || (paramCheck->numTotal == 1 && paramCheck->nTypes[0] == TASK_DATA_CHECK_NULL))) {
						pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );

						if(!paramCheck || (paramCheck->numTotal == 1 && paramCheck->nTypes[0] == TASK_DATA_CHECK_NULL)) {
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
							}
						#if AI_SCHEDULE_PARSER_STRICT_NULL == 0
							if(!stricmp("0",sched_pCurrTaskName)) {
								sched_tempTask[sched_tempTaskNum].data[0].nType = TASK_DATA_NONE;
								pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
							}
						#endif
							else if(equal) {
								Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i) (%s #%i): Malformed, Expecting 'Null' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName,sched_tempTaskNum,sched_tempbuffer);
								return false;
							}
						}
					}

					sched_tempTaskNum++;
					if(sched_tempTaskNum >= ARRAYSIZE(sched_tempTask)) {
						Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Task limit reached.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum);
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
					Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting '{' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
					return false;
				}

				// ==========================
				// Now read in the interrupts
				// ==========================
				sched_pCurrTaskName[0] = '\0';

				pfile = engine->ParseFile(pfile, sched_pCurrTaskName, SCHED_BUFFER_STRIDE );
				for(;;) {
				#if AI_SCHEDULE_PARSER_STRICT_NAMES == 1
					if(strnicmp("COND_",sched_pCurrTaskName,5))
					{
						Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Condition' got '%s' instead.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
						return false;
					}
				#endif

					// Convert generic ID to sub-class specific enum
					AiCondGlobalId_t condID = pGlobalNamespace->ConditionSymbolToId(sched_pCurrTaskName);

					// If not a valid condition, send a warning message
					if (condID == -1)
					{
						Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Condition '%s' was not registered.\n", pclassname, pfilename,sched_pSchedName,sched_tempSchedNum,sched_pCurrTaskName);
					#if AI_SCHEDULE_PARSER_STRICT_REGISTRATION == 1
						return false;
					#endif
					}

					// Otherwise, add to this schedules list of conditions
					if(condID != -1)
					{
						int interrupt = AI_RemapFromGlobal(condID);
						Assert( AI_IdIsGlobal( condID ) && interrupt >= 0 && interrupt < MAX_CONDITIONS );
						sched_tempInterruptMask.Set(interrupt);
					}

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
				if(first) {
					if (!stricmp(sched_tempbuffer,"}"))
					{
						break;
					}

					first = false;
				}

				Log_Error( LOG_SCHEDULE, "ERROR: LoadSchd (%s) (%s) (%s #%i): Malformed, Expecting 'Tasks' or 'Interrupts' got '%s' instead.\n",pclassname,pfilename,sched_pSchedName,sched_tempSchedNum,sched_tempbuffer);
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
		if(scheduleID != -1)
		{
			int old_num = 0;

			CAI_Schedule *new_schedule = GetScheduleByName(sched_pSchedName);
			if(!new_schedule) {
				new_schedule = CreateSchedule(sched_pSchedName,scheduleID);
				old_num = 0;
			} else {
				old_num = new_schedule->m_iNumTasks;
			}

			int mem_num = (old_num+sched_tempTaskNum);

			if(!new_schedule->m_pTaskList) {
				new_schedule->m_pTaskList = (Task_t *)malloc(sizeof(Task_t) * mem_num);
			} else {
				new_schedule->m_pTaskList = (Task_t *)realloc(new_schedule->m_pTaskList, sizeof(Task_t) * mem_num);
			}

			int actual_num = old_num;

			for (int k=0;k<sched_tempTaskNum;k++)
			{
				int old_i = old_num+k;
				int new_i = k;

				if(sched_tempTask[new_i].iTask == -1) {
					continue;
				}

				memcpy(&new_schedule->m_pTaskList[old_i], &sched_tempTask[new_i], sizeof(Task_t));
				actual_num++;

				Assert( AI_IdIsLocal( new_schedule->m_pTaskList[old_i].iTask ) );
			}

			new_schedule->m_iNumTasks = actual_num;

			if(actual_num == 0) {
				free(new_schedule->m_pTaskList);
			} else if(mem_num != actual_num) {
				new_schedule->m_pTaskList = (Task_t *)realloc(new_schedule->m_pTaskList, sizeof(Task_t) * actual_num);
			}

			CAI_ScheduleBits tmp;
			new_schedule->m_InterruptMask.Or(sched_tempInterruptMask, &tmp);
			new_schedule->m_InterruptMask = tmp;
		}

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

	Log_Error( LOG_SCHEDULE, "Couldn't find schedule (%s)\n", CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol(schedID) );

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
