//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the tasks for default AI.
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_TASK_H
#define AI_TASK_H
#pragma once

#include "ai_activity.h"
#include "ai_schedule.h"
#include "strtools.h"
#include "activitylist.h"

class CStringRegistry;

class CAI_BaseNPC;

// ----------------------------------------------------------------------
// Failure messages
//
// UNDONE: do this diffently so when not in developer mode we can 
//		   not use any memory for these text strings
// ----------------------------------------------------------------------

// Codes are either one of the enumerated types below, or a string (similar to Windows resource IDs)
typedef int AI_TaskFailureCode_t;

enum AI_BaseTaskFailureCodes_t
{
	NO_TASK_FAILURE,
	FAIL_NO_TARGET,
	FAIL_WEAPON_OWNED,
	FAIL_ITEM_NO_FIND,
	FAIL_SCHEDULE_NOT_FOUND,
	FAIL_NO_ENEMY,
	FAIL_NO_BACKAWAY_POSITION,
	FAIL_NO_COVER,
	FAIL_NO_FLANK,
	FAIL_NO_SHOOT,
	FAIL_NO_ROUTE,
	FAIL_NO_ROUTE_GOAL,
	FAIL_NO_ROUTE_BLOCKED,
	FAIL_NO_ROUTE_ILLEGAL,
	FAIL_NO_WALK,
	FAIL_ALREADY_LOCKED,
	FAIL_NO_SOUND,
	FAIL_NO_SCENT,
	FAIL_BAD_ACTIVITY,
	FAIL_NO_GOAL,
	FAIL_NO_PLAYER,
	FAIL_NOT_REACHABLE,
	FAIL_NO_NAV_MESH,
	FAIL_BAD_POSITION,
	FAIL_BAD_PATH_GOAL,
	FAIL_STUCK_ONTOP,
	FAIL_ITEM_TAKEN,
	FAIL_FROZEN,

	FAIL_UNIMPLEMENTED,

	NUM_FAIL_CODES,
};

inline bool IsPathTaskFailure( AI_TaskFailureCode_t code )
{
	return ( code >= FAIL_NO_ROUTE && code <= FAIL_NO_ROUTE_ILLEGAL );
}

const char *TaskFailureToString( AI_TaskFailureCode_t code );
inline int MakeFailCode( const char *pszGeneralError ) { return (int)pszGeneralError; }


enum TaskStatus_e 
{
	TASKSTATUS_NEW =			 	0,			// Just started
	TASKSTATUS_RUN_MOVE_AND_TASK =	1,			// Running task & movement
	TASKSTATUS_RUN_MOVE	=			2,			// Just running movement
	TASKSTATUS_RUN_TASK	=			3,			// Just running task
	TASKSTATUS_COMPLETE	=			4,			// Completed, get next task
};

typedef int TaskId_t;
typedef int TaskGlobalId_t;
typedef int TaskLocalId_t;

enum TaskDataType_t
{
	TASK_DATA_NONE,
	TASK_DATA_FLOAT,
	TASK_DATA_INT,
	TASK_DATA_BOOL,
	TASK_DATA_STRING,
	TASK_DATA_ACTIVITY,
	TASK_DATA_TASK_ID,
	TASK_DATA_SCHEDULE_ID,
	TASK_DATA_NPCSTATE,
	TASK_DATA_MEMORY_ID,
	TASK_DATA_PATH_TYPE,
	TASK_DATA_GOAL_TYPE,
};

// an array of tasks is a task list
// an array of schedules is a schedule list
struct TaskData_t
{
	TaskData_t() = default;
	TaskData_t(const TaskData_t &) = default;
	TaskData_t &operator=(const TaskData_t &) = default;
	TaskData_t(TaskData_t &&) = default;
	TaskData_t &operator=(TaskData_t &&) = default;
	~TaskData_t() = default;

	TaskData_t(float flVal)
		: nType(TASK_DATA_FLOAT), flData(flVal)
	{
	}

	TaskData_t(int iVal)
		: nType(TASK_DATA_INT), iData(iVal)
	{
	}

	TaskData_t(bool bVal)
		: nType(TASK_DATA_BOOL), bData(bVal)
	{
	}

	TaskData_t(Activity actVal)
		: nType(TASK_DATA_ACTIVITY), activity(actVal)
	{
	}

	union {
		float	flData;
		int iData;
		bool bData;
		char szStr[64] = {0};
		Activity activity;
		TaskGlobalId_t taskId;
		SchedGlobalId_t schedId;
		NPC_STATE nNpcState;
		int nMemoryId;
		pathType_t pathType;
		goalType_t goalType;
	};

	bool CanBeBool() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return false;
		case TASK_DATA_FLOAT:
			return true;
		case TASK_DATA_INT:
			return true;
		case TASK_DATA_BOOL:
			return true;
		case TASK_DATA_STRING:
			return true;
		case TASK_DATA_ACTIVITY:
			return true;
		case TASK_DATA_NPCSTATE:
			return true;
		default:
			return false;
		}
	}

	bool AsBool() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return false;
		case TASK_DATA_FLOAT:
			return flData > 0.0f;
		case TASK_DATA_INT:
			return iData > 0;
		case TASK_DATA_BOOL:
			return bData;
		case TASK_DATA_STRING:
			if(!V_stricmp(szStr, "true")) {
				return true;
			} else if(!V_stricmp(szStr, "false")) {
				return false;
			} else {
				return V_atoi(szStr) > 0;
			}
		case TASK_DATA_ACTIVITY:
			return activity != ACT_INVALID;
		case TASK_DATA_NPCSTATE:
			return nNpcState >= NPC_STATE_IDLE && nNpcState <= NPC_STATE_COMBAT;
		default:
			return false;
		}
	}

	bool CanBeInt() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return true;
		case TASK_DATA_FLOAT:
			return true;
		case TASK_DATA_INT:
			return true;
		case TASK_DATA_BOOL:
			return true;
		case TASK_DATA_STRING:
			return true;
		case TASK_DATA_ACTIVITY:
			return true;
		case TASK_DATA_NPCSTATE:
			return true;
		case TASK_DATA_TASK_ID:
			return true;
		case TASK_DATA_SCHEDULE_ID:
			return true;
		case TASK_DATA_MEMORY_ID:
			return true;
		case TASK_DATA_PATH_TYPE:
			return true;
		case TASK_DATA_GOAL_TYPE:
			return true;
		default:
			return false;
		}
	}

	int AsInt() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return -1;
		case TASK_DATA_FLOAT:
			return (int)flData;
		case TASK_DATA_INT:
			return iData;
		case TASK_DATA_BOOL:
			return bData ? 1 : 0;
		case TASK_DATA_STRING:
			if(!V_stricmp(szStr, "true")) {
				return 1;
			} else if(!V_stricmp(szStr, "false")) {
				return 0;
			} else {
				return V_atoi(szStr);
			}
		case TASK_DATA_ACTIVITY:
			return (int)activity;
		case TASK_DATA_NPCSTATE:
			return (int)nNpcState;
		case TASK_DATA_TASK_ID:
			return (int)taskId;
		case TASK_DATA_SCHEDULE_ID:
			return (int)schedId;
		case TASK_DATA_MEMORY_ID:
			return (int)nMemoryId;
		case TASK_DATA_PATH_TYPE:
			return (int)pathType;
		case TASK_DATA_GOAL_TYPE:
			return (int)goalType;
		default:
			return -1;
		}
	}

	bool CanBeActivity() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return false;
		case TASK_DATA_INT:
			return true;
		case TASK_DATA_STRING:
			return true;
		case TASK_DATA_ACTIVITY:
			return true;
		default:
			return false;
		}
	}

	Activity AsActivity() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return ACT_INVALID;
		case TASK_DATA_INT:
			return (Activity)iData;
		case TASK_DATA_STRING:
			return ActivityList_IndexForName( szStr );
		case TASK_DATA_ACTIVITY:
			return activity;
		default:
			return ACT_INVALID;
		}
	}

	bool CanBeFloat() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return true;
		case TASK_DATA_FLOAT:
			return true;
		case TASK_DATA_INT:
			return true;
		case TASK_DATA_BOOL:
			return true;
		case TASK_DATA_STRING:
			return true;
		default:
			return false;
		}
	}

	float AsFloat() const
	{
		switch(nType) {
		case TASK_DATA_NONE:
			return -1.0f;
		case TASK_DATA_FLOAT:
			return flData;
		case TASK_DATA_INT:
			return (float)iData;
		case TASK_DATA_BOOL:
			return bData ? 1.0f : 0.0f;
		case TASK_DATA_STRING:
			if(!V_stricmp(szStr, "true")) {
				return 1.0f;
			} else if(!V_stricmp(szStr, "false")) {
				return 0.0f;
			} else {
				return V_atof(szStr);
			}
		default:
			return -1.0f;
		}
	}

	TaskDataType_t nType = TASK_DATA_NONE;
};

//=========================================================
// These are the shared tasks
//=========================================================
enum sharedtasks_e
{
	#define AI_TASK_ENUM(name, params, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "ai_default_task_enum.inc"

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_TASK
};

#define TASK_MAX_PARAMETERS 5

struct Task_t
{
	Task_t() = default;

	Task_t(TaskId_t iTask_)
		: iTask(iTask_), numData(0)
	{
	}

	Task_t(TaskId_t iTask_, TaskData_t data_)
		: iTask(iTask_), numData(1)
	{
		data[0] = data_;
	}

	TaskId_t iTask = TASK_INVALID;
	TaskData_t data[TASK_MAX_PARAMETERS];
	int numData = 0;
};

enum TaskDataTypeCheck_t
{
	TASK_DATA_CHECK_NULL,
	TASK_DATA_CHECK_FLOAT,
	TASK_DATA_CHECK_INT,
	TASK_DATA_CHECK_NUM,
	TASK_DATA_CHECK_BOOL,
	TASK_DATA_CHECK_STRING,
	TASK_DATA_CHECK_ACTIVITY,
	TASK_DATA_CHECK_TASK_ID,
	TASK_DATA_CHECK_SCHEDULE_ID,
	TASK_DATA_CHECK_NPCSTATE,
	TASK_DATA_CHECK_MEMORY_ID,
	TASK_DATA_CHECK_PATH_TYPE,
	TASK_DATA_CHECK_GOAL_TYPE,
};

//TODO Arthurdead!!!! support optional parameters??

struct TaskParamCheck_t
{
	TaskParamCheck_t(const TaskParamCheck_t &) = default;
	TaskParamCheck_t &operator=(const TaskParamCheck_t &) = default;
	TaskParamCheck_t(TaskParamCheck_t &&) = default;
	TaskParamCheck_t &operator=(TaskParamCheck_t &&) = default;
	~TaskParamCheck_t() = default;

	TaskParamCheck_t()
		: numTotal(0)
	{
		nTypes[0] = TASK_DATA_CHECK_NULL;
	}

	TaskParamCheck_t(TaskDataTypeCheck_t type)
		: numTotal(1)
	{
		nTypes[0] = type;
	}

	template <int types_num>
	TaskParamCheck_t(const TaskDataTypeCheck_t (&types)[types_num])
		: numTotal(types_num)
	{
		COMPILE_TIME_ASSERT(types_num < TASK_MAX_PARAMETERS);

		for(int i = 0; i < types_num; ++i) {
			nTypes[i] = types[i];
		}
	}

	int numTotal;
	TaskDataTypeCheck_t nTypes[TASK_MAX_PARAMETERS];
};

typedef void(CAI_BaseNPC::*TaskFunc_t)(const Task_t *pTask);

//-----------------

#define DECLARE_TASK( id, params ) \
	taskIds.PushBack( #id, id, params );

#define ADD_CUSTOM_TASK_NAMED(derivedClass,taskName,taskEN,params)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddTask( taskName, taskEN, params, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_TASK(derivedClass,taskEN,params) ADD_CUSTOM_TASK_NAMED(derivedClass,#taskEN,taskEN,params)


#endif // AI_TASK_H
