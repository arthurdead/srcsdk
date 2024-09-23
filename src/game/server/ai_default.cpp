//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Default schedules.
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "soundent.h"
#include "scripted.h"
#include "ai_schedule.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "stringregistry.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CAI_Schedule *CAI_BaseNPC::ScheduleInList( const char *pName, CAI_Schedule **pList, int listCount )
{
	int i;

	if ( !pName )
	{
		DevMsg( "%s set to unnamed schedule!\n", GetClassname() );
		return NULL;
	}


	for ( i = 0; i < listCount; i++ )
	{
		if ( !pList[i]->GetName() )
		{
			DevMsg( "Unnamed schedule!\n" );
			continue;
		}
		if ( stricmp( pName, pList[i]->GetName() ) == 0 )
			return pList[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given and schedule name, return the schedule ID
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetScheduleID(const char* schedName)
{
	return GetSchedulingSymbols()->ScheduleSymbolToId(schedName);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitDefaultScheduleSR(void)
{
	#define ADD_DEF_SCHEDULE( name, localId ) idSpace.AddSchedule(name, localId, "CAI_BaseNPC" )

	CAI_ClassScheduleIdSpace &idSpace = CAI_BaseNPC::AccessClassScheduleIdSpaceDirect();

	#define AI_SCHED_ENUM(name, ...) \
		ADD_DEF_SCHEDULE( #name, name );

	#include "ai_default_sched_enum.inc"
}

bool CAI_BaseNPC::LoadDefaultSchedules(void)
{
#ifdef _DEBUG
#define AI_SCHEDULE_PARSER_STRICT_LOAD 0
#else
#define AI_SCHEDULE_PARSER_STRICT_LOAD 0
#endif

// For loading default schedules in memory  (see ai_default.cpp)

#if AI_SCHEDULE_PARSER_STRICT_LOAD == 1
#define AI_LOAD_DEF_SCHEDULE_BUFFER( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,(char *)g_psz##name,NULL,&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			return false; \
	} while (false)

#define AI_LOAD_DEF_SCHEDULE_FILE( classname, name ) \
	do \
	{ \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,NULL,UTIL_VarArgs("scripts/schedules/%s.sch",#name),&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			return false; \
	} while (false)
#else
#define AI_LOAD_DEF_SCHEDULE_BUFFER( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,(char *)g_psz##name,NULL,&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			fValid = false; \
	} while (false)

#define AI_LOAD_DEF_SCHEDULE_FILE( classname, name ) \
	do \
	{ \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,NULL,UTIL_VarArgs("scripts/schedules/%s.sch",#name),&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			fValid = false; \
	} while (false)
#endif

	#define AI_SCHED_ENUM(name, ...) \
		AI_LOAD_DEF_SCHEDULE_FILE( CAI_BaseNPC, name );
	#define AI_SCHED_ENUM_NO_LOAD(name, ...)

	bool fValid = true;

	//AI_LOAD_DEF_SCHEDULE_BUFFER( CAI_BaseNPC, Error );

	AI_LOAD_DEF_SCHEDULE_BUFFER( CAI_BaseNPC, SCHED_NONE );

	#include "ai_default_sched_enum.inc"

	return fValid;
}

int CAI_BaseNPC::TranslateSchedule( int scheduleType )
{
	// FIXME: Where should this go now?
#if 0
	if (scheduleType >= LAST_SHARED_SCHEDULE)
	{
		char errMsg[256];
		Q_snprintf(errMsg,sizeof(errMsg),"ERROR: Subclass Schedule (%s) Hitting Base Class!\n",ScheduleName(scheduleType));
		DevMsg( errMsg );
		AddTimedOverlay( errMsg, 5);
		return SCHED_FAIL;
	}
#endif

	switch( scheduleType )
	{
	// Hande some special cases
	case SCHED_AISCRIPT:
		{
			Assert( m_hCine != NULL );
			if ( !m_hCine )
			{
				DevWarning( 2, "Script failed for %s\n", GetClassname() );
				CineCleanup();
				return SCHED_IDLE_STAND;
			}
//			else
//				DevMsg( 2, "Starting script %s for %s\n", STRING( m_hCine->m_iszPlay ), GetClassname() );

			switch ( m_hCine->m_fMoveTo )
			{
				case CINE_MOVETO_WAIT:
				case CINE_MOVETO_TELEPORT:
				{
					return SCHED_SCRIPTED_WAIT;
				}

				case CINE_MOVETO_WALK:
				{
					return SCHED_SCRIPTED_WALK;
				}

				case CINE_MOVETO_RUN:
				{
					return SCHED_SCRIPTED_RUN;
				}

				case CINE_MOVETO_CUSTOM:
				{
					return SCHED_SCRIPTED_CUSTOM_MOVE;
				}

				case CINE_MOVETO_WAIT_FACING:
				{
					return SCHED_SCRIPTED_FACE;
				}
			}
		}
		break;

	case SCHED_IDLE_STAND:
		{
			// FIXME: crows are set into IDLE_STAND as an failure schedule, not sure if ALERT_STAND or COMBAT_STAND is a better choice
			// Assert( m_NPCState == NPC_STATE_IDLE );
		}
		break;
	case SCHED_IDLE_WANDER:
		{
			// FIXME: citizen interaction only, no idea what the state is.
			// Assert( m_NPCState == NPC_STATE_IDLE );
		}
		break;

	case SCHED_IDLE_WALK:
		{
			switch( m_NPCState )
			{
			case NPC_STATE_ALERT:
				return SCHED_ALERT_WALK;
			case NPC_STATE_COMBAT:
				return SCHED_COMBAT_WALK;
			}
		}
		break;

	case SCHED_ALERT_FACE:
		{
			// FIXME: default AI can pick this when in idle state
			// Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_ALERT_SCAN:
	case SCHED_ALERT_STAND:
		{
			// FIXME: rollermines use this when they're being held
			// Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_ALERT_WALK:
		{
			Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_COMBAT_FACE:
		{
			// FIXME: failure schedule for SCHED_PATROL which can be called when in alert
			// Assert( m_NPCState == NPC_STATE_COMBAT );
		}
		break;
	case SCHED_COMBAT_STAND:
		{
			// FIXME: never used?
		}
		break;
	case SCHED_COMBAT_WALK:
		{
			Assert( m_NPCState == NPC_STATE_COMBAT );
		}
		break;
	}

	return scheduleType;
}

//=========================================================
// GetScheduleOfType - returns a pointer to one of the
// NPC's available schedules of the indicated type.
//=========================================================
CAI_Schedule *CAI_BaseNPC::GetScheduleOfType( int scheduleType )
{
	// allow the derived classes to pick an appropriate version of this schedule or override
	// base schedule types.
	AI_PROFILE_SCOPE_BEGIN(CAI_BaseNPC_TranslateSchedule);
	scheduleType = TranslateSchedule( scheduleType );
	AI_PROFILE_SCOPE_END();

	// Get a pointer to that schedule
	CAI_Schedule *schedule = GetSchedule(scheduleType);

	if (!schedule)
	{
		DevMsg( "GetScheduleOfType(): No CASE for Schedule Type %d!\n", scheduleType );
		return GetSchedule(SCHED_IDLE_STAND);
	}
	return schedule;
}

CAI_Schedule *CAI_BaseNPC::GetSchedule(int schedule)
{
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		Warning("ERROR: %s missing schedule!\n", GetSchedulingErrorName());
		return g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}

	return g_AI_SchedulesManager.GetScheduleFromID( schedule );
}

bool CAI_BaseNPC::IsCurSchedule( int schedId, bool fIdeal )	
{ 
	if ( !m_pSchedule )
		return ( schedId == SCHED_NONE || schedId == AI_RemapToGlobal(SCHED_NONE) );

	schedId = ( AI_IdIsLocal( schedId ) ) ? 
							GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedId) : 
							schedId;
	if ( fIdeal )
		return ( schedId == m_IdealSchedule );

	return ( m_pSchedule->GetId() == schedId ); 
}


const char* CAI_BaseNPC::ConditionName(int conditionID)
{
	if ( AI_IdIsLocal( conditionID ) )
		conditionID = GetClassScheduleIdSpace()->ConditionLocalToGlobal(conditionID);
	return GetSchedulingSymbols()->ConditionIdToSymbol(conditionID);
}

const char *CAI_BaseNPC::TaskName(int taskID)
{
	if ( AI_IdIsLocal( taskID ) )
		taskID = GetClassScheduleIdSpace()->TaskLocalToGlobal(taskID);
	return GetSchedulingSymbols()->TaskIdToSymbol( taskID );
}



// This hooks the main game systems callbacks to allow the AI system to manage memory
class CAI_SystemHook : public CAutoGameSystem
{
public:
	CAI_SystemHook( char const *name ) : CAutoGameSystem( name )
	{
	}

	// UNDONE: Schedule / strings stuff should probably happen once each GAME, not each level
	void LevelInitPreEntity()
	{
		extern float g_AINextDisabledMessageTime;
		g_AINextDisabledMessageTime = 0;

		CAI_BaseNPC::gm_iNextThinkRebalanceTick = 0;
	}

	virtual void LevelInitPostEntity()
	{
		g_AI_SensedObjectsManager.Init();
	}

	void LevelShutdownPreEntity()
	{
	}

	void LevelShutdownPostEntity( void )
	{
		g_AI_SquadManager.DeleteAllSquads();
		CBaseCombatCharacter::ResetVisibilityCache( NULL );
		g_AI_SensedObjectsManager.Term();
	}
};


static CAI_SystemHook g_AISystemHook( "CAI_SystemHook" );


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_DEFINE_SCHEDULE_BUFFER(Error, R"(

Tasks
{
	TASK_DEBUG_BREAK
}

)");

AI_DEFINE_SCHEDULE_BUFFER(SCHED_NONE, R"(

Tasks
{
	TASK_DEBUG_BREAK
}

)");
