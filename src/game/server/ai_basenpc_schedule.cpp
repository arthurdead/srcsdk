//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions and data pertaining to the NPCs' AI scheduling system.
//			Implements default NPC tasks and schedules.
//
//=============================================================================//


#include "cbase.h"
#include "ai_default.h"
#include "animation.h"
#include "scripted.h"
#include "soundent.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "bitstring.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "ai_navigator.h"
#include "ai_tacticalservices.h"
#include "ai_moveprobe.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_speech.h"
#include "game.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "ndebugoverlay.h"
#include "tier0/vcrmode.h"
#include "env_debughistory.h"
#include "global_event_log.h"
#include "ai_behavior.h"
#include "collisionproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ai_task_pre_script;
extern ConVar ai_use_efficiency;
extern ConVar ai_use_think_optimizations;
#define ShouldUseEfficiency() ( ai_use_think_optimizations.GetBool() && ai_use_efficiency.GetBool() )

ConVar	ai_simulate_task_overtime( "ai_simulate_task_overtime", "0" );

#define MAX_TASKS_RUN 10

struct TaskTimings
{
	const char *pszTask;
	CFastTimer selectSchedule;
	CFastTimer startTimer;
	CFastTimer runTimer;
};

TaskTimings g_AITaskTimings[MAX_TASKS_RUN];
int			g_nAITasksRun;

void CAI_BaseNPC::DumpTaskTimings()
{
	DevMsg(" Tasks timings:\n" );
	for ( int i = 0; i < g_nAITasksRun; ++i )
	{
		DevMsg( "   %32s -- select %5.2f, start %5.2f, run %5.2f\n", g_AITaskTimings[i].pszTask,
			 g_AITaskTimings[i].selectSchedule.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].startTimer.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].runTimer.GetDuration().GetMillisecondsF() );
			
	}
}


//=========================================================
// FHaveSchedule - Returns true if NPC's GetCurSchedule()
// is anything other than NULL.
//=========================================================
bool CAI_BaseNPC::FHaveSchedule( void )
{
	if ( GetCurSchedule() == NULL )
	{
		return false;
	}

	return true;
}

//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CAI_BaseNPC::ClearSchedule( const char *szReason )
{
	if (szReason && m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg( this, AIMF_IGNORE_SELECTED, "  Schedule cleared: %s\n", szReason );
	}

	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = 0;
	m_ScheduleState.bScheduleWasInterrupted = true;
	SetTaskStatus( TASKSTATUS_NEW );
	m_IdealSchedule = SCHED_NONE;
	m_pSchedule =  NULL;
	ResetScheduleCurTaskIndex();
	m_InverseIgnoreConditions.SetAll();
}

//=========================================================
// FScheduleDone - Returns true if the caller is on the
// last task in the schedule
//=========================================================
bool CAI_BaseNPC::FScheduleDone ( void )
{
	Assert( GetCurSchedule() != NULL );
	
	if ( GetScheduleCurTaskIndex() == GetCurSchedule()->NumTasks() )
	{
		return true;
	}

	return false;
}

//=========================================================

bool CAI_BaseNPC::SetSchedule( int localScheduleID ) 			
{ 
	CAI_Schedule *pNewSchedule = GetScheduleOfType( localScheduleID );
	if ( pNewSchedule )
	{
		// ken: I'm don't know of any remaining cases, but if you find one, hunt it down as to why the schedule is getting slammed while they're in the middle of script
		if (m_hCine != NULL)
		{
			if (!(localScheduleID == SCHED_SLEEP || localScheduleID == SCHED_WAIT_FOR_SCRIPT || localScheduleID == SCHED_SCRIPTED_WALK || localScheduleID == SCHED_SCRIPTED_RUN || localScheduleID == SCHED_SCRIPTED_CUSTOM_MOVE || localScheduleID == SCHED_SCRIPTED_WAIT || localScheduleID == SCHED_SCRIPTED_FACE) )
			{
				Assert( 0 );
				// ExitScriptedSequence();
			}
		}
		

		m_IdealSchedule = GetGlobalScheduleId( localScheduleID );
		SetSchedule( pNewSchedule ); 
		return true;
	}
	return false;
}

//=========================================================
// SetSchedule - replaces the NPC's schedule pointer
// with the passed pointer, and sets the ScheduleIndex back
// to 0
//=========================================================
#define SCHEDULE_HISTORY_SIZE	10
void CAI_BaseNPC::SetSchedule( CAI_Schedule *pNewSchedule )
{
	Assert( pNewSchedule != NULL );

	OnSetSchedule();
	
	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = gpGlobals->curtime;
	m_ScheduleState.bScheduleWasInterrupted = false;
	
	m_pSchedule = pNewSchedule ;
	ResetScheduleCurTaskIndex();
	SetTaskStatus( TASKSTATUS_NEW );
	m_failSchedule = SCHED_NONE;
	bool bCondInPVS = HasCondition( COND_IN_PVS );
	m_Conditions.ClearAll();
	if ( bCondInPVS )
		SetCondition( COND_IN_PVS );
	m_bConditionsGathered = false;
	GetNavigator()->ClearGoal();
	m_InverseIgnoreConditions.SetAll();
	Forget( bits_MEMORY_TURNING );

/*
#if _DEBUG
	if ( !ScheduleFromName( pNewSchedule->GetName() ) )
	{
		DevMsg( "Schedule %s not in table!!!\n", pNewSchedule->GetName() );
	}
#endif
*/	
// this is very useful code if you can isolate a test case in a level with a single NPC. It will notify
// you of every schedule selection the NPC makes.

	if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg(this, AIMF_IGNORE_SELECTED, "Schedule: %s (time: %.2f)\n", pNewSchedule->GetName(), gpGlobals->curtime );
	}

	if ( m_pEvent != NULL )
	{
		if ( m_pScheduleEvent != NULL )
		{
			GlobalEventLog.RemoveEvent( m_pScheduleEvent );
		}
		m_pScheduleEvent = GlobalEventLog.CreateEvent( "Schedule", false, m_pEvent );

		GlobalEventLog.AddKeyValue( m_pScheduleEvent, false, "Schedule", pNewSchedule->GetName() );
	}

	ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d): Schedule: %s (time: %.2f)\n", GetDebugName(), entindex(), pNewSchedule->GetName(), gpGlobals->curtime ) );

#ifdef AI_MONITOR_FOR_OSCILLATION
	if( m_bSelected )
	{
		AIScheduleChoice_t choice;
		choice.m_flTimeSelected = gpGlobals->curtime;
		choice.m_pScheduleSelected = pNewSchedule;
		m_ScheduleHistory.AddToHead(choice);

		if( m_ScheduleHistory.Count() > SCHEDULE_HISTORY_SIZE )
		{
			m_ScheduleHistory.Remove( SCHEDULE_HISTORY_SIZE );
		}

		Assert( m_ScheduleHistory.Count() <= SCHEDULE_HISTORY_SIZE );

		// No analysis until the vector is full!
		if( m_ScheduleHistory.Count() == SCHEDULE_HISTORY_SIZE )
		{
			int		iNumSelections  = m_ScheduleHistory.Count();
			float	flTimeSpan		= m_ScheduleHistory.Head().m_flTimeSelected - m_ScheduleHistory.Tail().m_flTimeSelected;
			float	flSelectionsPerSecond = ((float)iNumSelections) / flTimeSpan;

			Msg( "%d selections in %f seconds   (avg. %f selections per second)\n", iNumSelections, flTimeSpan, flSelectionsPerSecond );

			if( flSelectionsPerSecond >=  8.0f )
			{
				DevMsg("\n\n %s is thrashing schedule selection:\n", GetDebugName() );

				for( int i = 0 ; i < m_ScheduleHistory.Count() ; i++ )
				{
					AIScheduleChoice_t choice = m_ScheduleHistory[i];
					Msg("--%s  %f\n", choice.m_pScheduleSelected->GetName(), choice.m_flTimeSelected );
				}

				Msg("\n");

				CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
			}
		}
	}
#endif//AI_MONITOR_FOR_OSCILLATION
}

//=========================================================
// NextScheduledTask - increments the ScheduleIndex
//=========================================================
void CAI_BaseNPC::NextScheduledTask ( void )
{
	Assert( GetCurSchedule() != NULL );

	SetTaskStatus( TASKSTATUS_NEW );
	IncScheduleCurTaskIndex();

	if ( FScheduleDone() )
	{
		// Reset memory of failed schedule 
		m_failedSchedule   = NULL;
		m_interuptSchedule = NULL;

		// just completed last task in schedule, so make it invalid by clearing it.
		SetCondition( COND_SCHEDULE_DONE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This function allows NPCs to modify the interrupt mask for the
//			current schedule. This enables them to use base schedules but with
//			different interrupt conditions. Implement this function in your
//			derived class, and Set or Clear condition bits as you please.
//
//			NOTE: Always call the base class in your implementation, but be
//				  aware of the difference between changing the bits before vs.
//				  changing the bits after calling the base implementation.
//
// Input  : pBitString - Receives the updated interrupt mask.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::BuildScheduleTestBits( void )
{
	//NOTENOTE: Always defined in the leaf classes
}


//=========================================================
// IsScheduleValid - returns true as long as the current
// schedule is still the proper schedule to be executing,
// taking into account all conditions
//=========================================================
bool CAI_BaseNPC::IsScheduleValid()
{
	if ( GetCurSchedule() == NULL || GetCurSchedule()->NumTasks() == 0 )
	{
		return false;
	}

	//Start out with the base schedule's set interrupt conditions
	GetCurSchedule()->GetInterruptMask( &m_CustomInterruptConditions );

	// Let the leaf class modify our interrupt test bits, but:
	// - Don't allow any modifications when scripted
	// - Don't modify interrupts for Schedules that set the COND_NO_CUSTOM_INTERRUPTS bit.
	if ( m_NPCState != NPC_STATE_SCRIPT && !IsInLockedScene() && !m_CustomInterruptConditions.IsBitSet( COND_NO_CUSTOM_INTERRUPTS ) )
	{
		BuildScheduleTestBits();
	}

	//Any conditions set here will always be forced on the interrupt conditions
	SetCustomInterruptCondition( COND_NPC_FREEZE );

	// This is like: m_CustomInterruptConditions &= m_Conditions;
	CAI_ScheduleBits testBits;
	m_CustomInterruptConditions.And( m_Conditions, &testBits  );

	if (!testBits.IsAllClear()) 
	{
		// If in developer mode save the interrupt text for debug output
		if (developer->GetInt()) 
		{
			// Reset memory of failed schedule 
			m_failedSchedule   = NULL;
			m_interuptSchedule = GetCurSchedule();

			// Find the first non-zero bit
			for (int i=0;i<MAX_CONDITIONS;i++)
			{
				if (testBits.IsBitSet(i))
				{
					m_interruptText = ConditionName( AI_RemapToGlobal( i ) );
					if (!m_interruptText)
					{
						m_interruptText = "(UNKNOWN CONDITION)";
						/*
						static const char *pError = "ERROR: Unknown condition!";
						DevMsg("%s (%s)\n", pError, GetDebugName());
						m_interruptText = pError;
						*/
					}

					if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
					{
						DevMsg( this, AIMF_IGNORE_SELECTED, "      Break condition -> %s\n", m_interruptText );
					}

					ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      Break condition -> %s\n", GetDebugName(), entindex(), m_interruptText ) );

					break;
				}
			}
			
			if ( HasCondition( COND_NEW_ENEMY ) )
			{
				if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
				{
					DevMsg( this, AIMF_IGNORE_SELECTED, "      New enemy: %s\n", GetEnemy() ? GetEnemy()->GetDebugName() : "<NULL>" );
				}
				
				ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      New enemy: %s\n", GetDebugName(), entindex(), GetEnemy() ? GetEnemy()->GetDebugName() : "<NULL>" ) );
			}
		}

		return false;
	}

	if ( HasCondition(COND_SCHEDULE_DONE) || 
		 HasCondition(COND_TASK_FAILED)   )
	{
#ifdef DEBUG
		if ( HasCondition ( COND_TASK_FAILED ) && m_failSchedule == SCHED_NONE )
		{
			// fail! Send a visual indicator.
			DevWarning( 2, "Schedule: %s Failed\n", GetCurSchedule()->GetName() );

			Vector tmp;
			CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 1.0f ), &tmp );
			tmp.z += 16;

			g_pEffects->Sparks( tmp );
		}
#endif // DEBUG

		// some condition has interrupted the schedule, or the schedule is done
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not SelectIdealState() should be called before
//			a NPC selects a new schedule. 
//
//			NOTE: This logic was a source of pure, distilled trouble in Half-Life.
//			If you change this function, please supply good comments.
//
// Output : Returns true if yes, false if no
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ShouldSelectIdealState( void )
{
/*

	HERE's the old Half-Life code that used to control this.

	if ( m_IdealNPCState != NPC_STATE_DEAD && 
		 (m_IdealNPCState != NPC_STATE_SCRIPT || m_IdealNPCState == m_NPCState) )
	{
		if (	(m_afConditions && !HasConditions(bits_COND_SCHEDULE_DONE)) ||
				(GetCurSchedule() && (GetCurSchedule()->iInterruptMask & bits_COND_SCHEDULE_DONE)) ||
				((m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL))	)
		{
			GetIdealState();
		}
	}
*/
	
	// Don't get ideal state if you are supposed to be dead.
	if ( m_IdealNPCState == NPC_STATE_DEAD )
		return false;

	// If I'm supposed to be in scripted state, but i'm not yet, do not allow 
	// SelectIdealState() to be called, because it doesn't know how to determine 
	// that a NPC should be in SCRIPT state and will stomp it with some other 
	// state. (Most likely ALERT)
	if ( (m_IdealNPCState == NPC_STATE_SCRIPT) && (m_NPCState != NPC_STATE_SCRIPT) )
		return false;

	// If the NPC has any current conditions, and one of those conditions indicates
	// that the previous schedule completed successfully, then don't run SelectIdealState(). 
	// Paths between states only exist for interrupted schedules, or when a schedule 
	// contains a task that suggests that the NPC change state.
	if ( !HasCondition(COND_SCHEDULE_DONE) )
		return true;

	// This seems like some sort of hack...
	// Currently no schedule that I can see in the AI uses this feature, but if a schedule
	// interrupt mask contains bits_COND_SCHEDULE_DONE, then force a call to SelectIdealState().
	// If we want to keep this feature, I suggest we create a new condition with a name that
	// indicates exactly what it does. 
	if ( GetCurSchedule() && GetCurSchedule()->HasInterrupt(COND_SCHEDULE_DONE) )
		return true;

	// Don't call SelectIdealState if a NPC in combat state has a valid enemy handle. Otherwise,
	// we need to change state immediately because something unexpected happened to the enemy 
	// entity (it was blown apart by someone else, for example), and we need the NPC to change
	// state. THE REST OF OUR CODE should be robust enough that this can go away!!
	if ( (m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL) )
		return true;

	if ( (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT) && (GetEnemy() != NULL) )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a new schedule based on current condition bits.
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetNewSchedule( void )
{
	int scheduleType;

	//
	// Schedule selection code here overrides all leaf schedule selection.
	//
	if (HasCondition(COND_NPC_FREEZE))
	{
		scheduleType = SCHED_NPC_FREEZE;
	}
	else
	{
		// I dunno how this trend got started, but we need to find the problem.
		// You may not be in combat state with no enemy!!! (sjb) 11/4/03
		if( m_NPCState == NPC_STATE_COMBAT && !GetEnemy() )
		{
			DevMsg("**ERROR: Combat State with no enemy! slamming to ALERT\n");
			SetState( NPC_STATE_ALERT );
		}

		AI_PROFILE_SCOPE_BEGIN( CAI_BaseNPC_SelectSchedule);

		if ( m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_DEAD || m_iInteractionState == NPCINT_MOVING_TO_MARK )
		{
			scheduleType = CAI_BaseNPC::SelectSchedule();
		}
		else
		{
			scheduleType = SelectSchedule();
		}

		m_IdealSchedule = GetGlobalScheduleId( scheduleType );

		AI_PROFILE_SCOPE_END();
	}

	return GetScheduleOfType( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetFailSchedule( void )
{
	int prevSchedule;
	int failedTask;

	if ( GetCurSchedule() )
		prevSchedule = GetLocalScheduleId( GetCurSchedule()->GetId() );
	else
		prevSchedule = SCHED_NONE;
		
	const Task_t *pTask = GetTask();
	if ( pTask )
		failedTask = pTask->iTask;
	else
		failedTask = TASK_INVALID;

	Assert( AI_IdIsLocal( prevSchedule ) );
	Assert( AI_IdIsLocal( failedTask ) );

	int scheduleType = SelectFailSchedule( prevSchedule, failedTask, m_ScheduleState.taskFailureCode );
	return GetScheduleOfType( scheduleType );
}


//=========================================================
// MaintainSchedule - does all the per-think schedule maintenance.
// ensures that the NPC leaves this function with a valid
// schedule!
//=========================================================

static bool ShouldStopProcessingTasks( CAI_BaseNPC *pNPC, int taskTime, int timeLimit )
{
#ifdef DEBUG
	if( ai_simulate_task_overtime.GetBool() )
		return true;
#endif

	// Always stop processing if we've queued up a navigation query on the last task
	if ( pNPC->IsNavigationDeferred() )
		return true;

	if ( AIStrongOpt() )
	{
		bool bInScript = ( pNPC->GetState() == NPC_STATE_SCRIPT || pNPC->IsCurSchedule( SCHED_SCENE_GENERIC, false ) );
		
		// We ran a costly task, don't do it again!
		if ( pNPC->HasMemory( bits_MEMORY_TASK_EXPENSIVE ) && bInScript == false )
			return true;
	}

	if ( taskTime > timeLimit )
	{
		if ( ShouldUseEfficiency() || 
			 pNPC->IsMoving() || 
			 ( pNPC->GetIdealActivity() != ACT_RUN && pNPC->GetIdealActivity() != ACT_WALK ) )
		{
			return true;
		}
	}
	return false;
}

//-------------------------------------

void CAI_BaseNPC::MaintainSchedule ( void )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_RunAI_MaintainSchedule);
	extern CFastTimer g_AIMaintainScheduleTimer;
	CTimeScope timeScope(&g_AIMaintainScheduleTimer);

	//---------------------------------

	CAI_Schedule	*pNewSchedule;
	int			i;
	bool		runTask = true;

#if defined( VPROF_ENABLED )
#if defined(DISABLE_DEBUG_HISTORY)
	bool bDebugTaskNames = ( developer->GetBool() || ( VProfAI() && g_VProfCurrentProfile.IsEnabled() ) );
#else
	bool bDebugTaskNames = true;
#endif
#else
	bool bDebugTaskNames = false;
#endif

	memset( (void *)g_AITaskTimings, 0, sizeof(g_AITaskTimings) );
	
	g_nAITasksRun = 0;

#ifdef _DEBUG
	const int timeLimit = 16;
#else
	const int timeLimit = 8;
#endif
	int taskTime = Plat_MSTime();

	// Reset this at the beginning of the frame
	Forget( bits_MEMORY_TASK_EXPENSIVE );

	// UNDONE: Tune/fix this MAX_TASKS_RUN... This is just here so infinite loops are impossible
	bool bStopProcessing = false;
	for ( i = 0; i < MAX_TASKS_RUN && !bStopProcessing; i++ )
	{
		if ( GetCurSchedule() != NULL && TaskIsComplete() )
		{
			// Schedule is valid, so advance to the next task if the current is complete.
			NextScheduledTask();

			// If we finished the current schedule, clear our ignored conditions so they
			// aren't applied to the next schedule selection.
			if ( HasCondition( COND_SCHEDULE_DONE ) )
			{
				// Put our conditions back the way they were after GatherConditions,
				// but add in COND_SCHEDULE_DONE.
				m_Conditions = m_ConditionsPreIgnore;
				SetCondition( COND_SCHEDULE_DONE );

				m_InverseIgnoreConditions.SetAll();
			}

			// --------------------------------------------------------
			//	If debug stepping advance when I complete a task
			// --------------------------------------------------------
			if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
			{
				m_nDebugCurIndex++;
				return;
			}
		}
		
		int curTiming = g_nAITasksRun;
		g_nAITasksRun++;

		// validate existing schedule 
		if ( !IsScheduleValid() || m_NPCState != m_IdealNPCState )
		{
			// Notify the NPC that his schedule is changing
			m_ScheduleState.bScheduleWasInterrupted = true;
			OnScheduleChange();

			if ( !HasCondition(COND_NPC_FREEZE) && ( !m_bConditionsGathered || m_bSkippedChooseEnemy ) )
			{
				// occurs if a schedule is exhausted within one think
				GatherConditions();
			}

			if ( ShouldSelectIdealState() )
			{
				NPC_STATE eIdealState = SelectIdealState();
				SetIdealState( eIdealState );
			}

			if ( HasCondition( COND_TASK_FAILED ) && m_NPCState == m_IdealNPCState )
			{
				// Get a fail schedule if the previous schedule failed during execution and 
				// the NPC is still in its ideal state. Otherwise, the NPC would immediately
				// select the same schedule again and fail again.
				if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
				{
					DevMsg( this, AIMF_IGNORE_SELECTED, "      (failed)\n" );
				}

				ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      (failed)\n", GetDebugName(), entindex() ) );

				pNewSchedule = GetFailSchedule();
				m_IdealSchedule = pNewSchedule->GetId();
				DevWarning( 2, "(%s #%i) Schedule (%s) Failed at %d!\n", GetDebugName(), entindex(), GetCurSchedule() ? GetCurSchedule()->GetName() : "GetCurSchedule() == NULL", GetScheduleCurTaskIndex() );
				SetSchedule( pNewSchedule );
			}
			else
			{
				// If the NPC is supposed to change state, it doesn't matter if the previous
				// schedule failed or completed. Changing state means selecting an entirely new schedule.
				SetState( m_IdealNPCState );
				
				g_AITaskTimings[curTiming].selectSchedule.Start();

				pNewSchedule = GetNewSchedule();

				g_AITaskTimings[curTiming].selectSchedule.End();

				SetSchedule( pNewSchedule );
			}
		}

		if (!GetCurSchedule())
		{
			g_AITaskTimings[curTiming].selectSchedule.Start();
			
			pNewSchedule = GetNewSchedule();
			
			g_AITaskTimings[curTiming].selectSchedule.End();

			if (pNewSchedule)
			{
				SetSchedule( pNewSchedule );
			}
		}

		if ( !GetCurSchedule() || GetCurSchedule()->NumTasks() == 0 )
		{
			DevMsg("ERROR: Missing or invalid schedule!\n");
			SetActivity ( ACT_IDLE );
			return;
		}
		
		AI_PROFILE_SCOPE_BEGIN_( GetCurSchedule() ? CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol( GetCurSchedule()->GetId() ) : "NULL SCHEDULE" );

		if ( GetTaskStatus() == TASKSTATUS_NEW )
		{	
			if ( GetScheduleCurTaskIndex() == 0 )
			{
				int globalId = GetCurSchedule()->GetId();
				int localId = GetLocalScheduleId( globalId ); // if localId == -1, then it came from a behavior
				OnStartSchedule( (localId != -1)? localId : globalId );
			}

			g_AITaskTimings[curTiming].startTimer.Start();
			const Task_t *pTask = GetTask();
			const char *pszTaskName = ( bDebugTaskNames ) ? TaskName( pTask->iTask ) : "ai_task";
			Assert( pTask != NULL );
			g_AITaskTimings[i].pszTask = pszTaskName;

			if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
			{
				DevMsg(this, AIMF_IGNORE_SELECTED, "  Task: %s\n", pszTaskName );
			}

			if ( m_pScheduleEvent != NULL )
			{
				CGlobalEvent	*pEvent = GlobalEventLog.CreateTempEvent( "New Task", m_pScheduleEvent );

				GlobalEventLog.AddKeyValue( pEvent, false, "Task", pszTaskName );
			}

			ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):  Task: %s\n", GetDebugName(), entindex(), pszTaskName ) );

			OnStartTask();
			
			m_ScheduleState.taskFailureCode    = NO_TASK_FAILURE;
			m_ScheduleState.timeCurTaskStarted = gpGlobals->curtime;
			
			AI_PROFILE_SCOPE_BEGIN_( pszTaskName );
			AI_PROFILE_SCOPE_BEGIN(CAI_BaseNPC_StartTask);

			StartTask( pTask );

			AI_PROFILE_SCOPE_END();
			AI_PROFILE_SCOPE_END();

			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
				StartTaskOverlay();

			g_AITaskTimings[curTiming].startTimer.End();
			// DevMsg( "%.2f StartTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
		}

		AI_PROFILE_SCOPE_END();

		// UNDONE: Twice?!!!
		MaintainActivity();
		
		AI_PROFILE_SCOPE_BEGIN_( GetCurSchedule() ? CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol( GetCurSchedule()->GetId() ) : "NULL SCHEDULE" );

		if ( !TaskIsComplete() && GetTaskStatus() != TASKSTATUS_NEW )
		{
			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) && runTask )
			{
				const Task_t *pTask = GetTask();
				const char *pszTaskName = ( bDebugTaskNames ) ? TaskName( pTask->iTask ) : "ai_task";
				Assert( pTask != NULL );
				g_AITaskTimings[i].pszTask = pszTaskName;
				// DevMsg( "%.2f RunTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
				g_AITaskTimings[curTiming].runTimer.Start();

				AI_PROFILE_SCOPE_BEGIN_( pszTaskName );
				AI_PROFILE_SCOPE_BEGIN(CAI_BaseNPC_RunTask);

				int j;
				for (j = 0; j < 8; j++)
				{
					RunTask( pTask );

					if ( GetTaskInterrupt() == 0 || TaskIsComplete() || HasCondition(COND_TASK_FAILED) )
						break;

					if ( ShouldUseEfficiency() && ShouldStopProcessingTasks( this, Plat_MSTime() - taskTime, timeLimit ) )
					{
						bStopProcessing = true;
						break;
					}
				}
				AssertMsg( j < 8, "Runaway task interrupt\n" );
					
				AI_PROFILE_SCOPE_END();
				AI_PROFILE_SCOPE_END();

				if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
				{
					if ( IsCurTaskContinuousMove() )
						Remember( bits_MEMORY_MOVED_FROM_SPAWN );
					RunTaskOverlay();
				}

				g_AITaskTimings[curTiming].runTimer.End();

				// don't do this again this frame
				// FIXME: RunTask() should eat some of the clock, depending on what it has done
				// runTask = false;

				if ( !TaskIsComplete() )
				{
					bStopProcessing = true;
				}
			}
			else
			{
				bStopProcessing = true;
			}
		}

		AI_PROFILE_SCOPE_END();

		// Decide if we should continue on this frame
		if ( !bStopProcessing && ShouldStopProcessingTasks( this, Plat_MSTime() - taskTime, timeLimit ) )
			bStopProcessing = true;
	}

	for ( i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->MaintainChannelSchedules();
	}

	// UNDONE: We have to do this so that we have an animation set to blend to if RunTask changes the animation
	// RunTask() will always change animations at the end of a script!
	// Don't do this twice
	MaintainActivity();

	// --------------------------------------------------------
	//	If I'm stopping to debug step, don't animate unless
	//  I'm in motion
	// --------------------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
	{
		if (!GetNavigator()->IsGoalActive() && 
			m_nDebugCurIndex >= CAI_BaseNPC::m_nDebugPauseIndex)
		{
			SetPlaybackRate( 0 );
		}
	}
}


//=========================================================

bool CAI_BaseNPC::FindCoverPos( CBaseEntity *pEntity, Vector *pResult )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_FindCoverPos);

	if ( !GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), 0, pResult ) )
	{
		if ( !GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), pResult ) ) 
		{
			return false;
		}
	}
	return true;
}

//=========================================================

bool CAI_BaseNPC::FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_FindCoverPosInRadius);

	if ( pEntity == NULL )
	{
		// Find cover from self if no enemy available
		pEntity = this;
	}

	Vector					coverPos			= vec3_invalid;
	CAI_TacticalServices *	pTacticalServices	= GetTacticalServices();
	const Vector &			enemyPos			= pEntity->GetAbsOrigin();
	Vector					enemyEyePos			= pEntity->EyePosition();

	if( ( !GetSquad() || GetSquad()->GetFirstMember() == this ) &&
		IsCoverPosition( enemyEyePos, goalPos + GetViewOffset() ) && 
		IsValidCover( goalPos ) )
	{
		coverPos = goalPos;
	}
	else if ( !pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 0, coverRadius * 0.5, &coverPos ) )
	{
		if ( !pTacticalServices->FindLateralCover( goalPos, enemyEyePos, 0, coverRadius * 0.5, 3, &coverPos ) )
		{
			if ( !pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, coverRadius * 0.5 - 0.1, coverRadius, &coverPos ) )
			{
				pTacticalServices->FindLateralCover( goalPos, enemyEyePos, 0, coverRadius, 5, &coverPos );
			}
		}
	}
	
	if ( coverPos == vec3_invalid )
		return false;
	*pResult = coverPos;
	return true;
}

//=========================================================

bool CAI_BaseNPC::FindCoverPos( CSound *pSound, Vector *pResult )
{
	if ( !GetTacticalServices()->FindCoverPos( pSound->GetSoundReactOrigin(), 
												pSound->GetSoundReactOrigin(), 
												MIN( pSound->Volume(), 120.0 ), 
												CoverRadius(), 
												pResult ) )
	{
		return GetTacticalServices()->FindLateralCover( pSound->GetSoundReactOrigin(), MIN( pSound->Volume(), 60.0 ), pResult );
	}

	return true;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================

//-----------------------------------------------------------------------------
// TASK_TURN_RIGHT / TASK_TURN_LEFT
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartTurn( float flDeltaYaw )
{
	float flCurrentYaw;
	
	flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );
	GetMotor()->SetIdealYaw( UTIL_AngleMod( flCurrentYaw + flDeltaYaw ) );
	SetTurnActivity();
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::FindCoverFromEnemy( bool bNodesOnly, float flMinDistance, float flMaxDistance )
{
	CBaseEntity *pEntity = GetEnemy();

	// Find cover from self if no enemy available
	if ( pEntity == NULL )
		pEntity = this;

	Vector coverPos = vec3_invalid;

	if ( bNodesOnly )
	{
		if ( flMaxDistance == FLT_MAX )
			flMaxDistance = CoverRadius();
		
		if ( !GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), flMinDistance, flMaxDistance, &coverPos ) )
			return false;
	}
	else
	{
		if ( !FindCoverPos( pEntity, &coverPos ) )
			return false;
	}

	AI_NavGoal_t goal( GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE );

	if ( !GetNavigator()->SetGoal( goal ) )
		return false;

	// FIXME: add to goal
	GetNavigator()->SetArrivalActivity( GetCoverActivity() );
	GetNavigator()->SetArrivalDirection( vec3_origin );
	
	return true;
}


//-----------------------------------------------------------------------------
// TASK_FIND_COVER_FROM_BEST_SOUND
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::FindCoverFromBestSound( Vector *pCoverPos )
{
	CSound *pBestSound;

	pBestSound = GetBestSound();

	if (pBestSound)
	{
		// UNDONE: Back away if cover fails?  Grunts do this.
		return FindCoverPos( pBestSound, pCoverPos );
	}
	else
	{
		DevMsg( 2, "Attempting to find cover from best sound, but best sound not founc.\n" );
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// TASK_FACE_REASONABLE
//-----------------------------------------------------------------------------
float CAI_BaseNPC::CalcReasonableFacing( bool bIgnoreOriginalFacing )
{
	float flReasonableYaw;

	if( !bIgnoreOriginalFacing && !HasMemory( bits_MEMORY_MOVED_FROM_SPAWN ) && !HasCondition( COND_SEE_ENEMY) )
	{
		flReasonableYaw = m_flOriginalYaw;
	}
	else
	{
		// If I'm facing a wall, change my original yaw and try to find a good direction to face.
		trace_t tr;
		Vector forward;
		QAngle angles( 0, 0, 0 );

		float idealYaw = GetMotor()->GetIdealYaw();
		
		flReasonableYaw = idealYaw;
		
		// Try just using the facing we have
		const float MIN_DIST = GetReasonableFacingDist();
		float longestTrace = 0;
		
		// Early out if we're overriding reasonable facing
		if ( !MIN_DIST )
			return flReasonableYaw;

		// Otherwise, scan out back and forth until something better is found
		const float SLICES = 8.0f;
		const float SIZE_SLICE = 360.0 / SLICES;
		const int SEARCH_MAX = (int)SLICES / 2;

		float zEye = GetAbsOrigin().z + m_vDefaultEyeOffset.z; // always use standing eye so as to not screw with crouch cover

		for( int i = 0 ; i <= SEARCH_MAX; i++ )
		{
			float offset = i * SIZE_SLICE;
			for ( int j = -1; j <= 1; j += 2)
			{
				angles.y = idealYaw + ( offset * j );
				AngleVectors( angles, &forward, NULL, NULL );
				float curTrace;
				if( ( curTrace = LineOfSightDist( forward, zEye ) ) > longestTrace && IsValidReasonableFacing(forward, curTrace) )
				{
					// Take this one.
					flReasonableYaw = angles.y;
					longestTrace = curTrace;
				}
				
				if ( longestTrace > MIN_DIST) // found one
					break;

				if ( i == 0 || i == SEARCH_MAX) // if trying forwards or backwards, skip the check of the other side...
					break;
			}
			
			if ( longestTrace > MIN_DIST ) // found one
				break;
		}
	}
	
	return flReasonableYaw;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CAI_BaseNPC::GetReasonableFacingDist( void )
{
	if ( GetTask() && GetTask()->iTask == TASK_FACE_ENEMY )
	{
		const float dist = 3.5*12;
		if ( GetEnemy() )
		{
			float distEnemy = ( GetEnemy()->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).Length() - 1.0; 
			return MIN( distEnemy, dist );
		}

		return dist;
	}
	return 5*12;
}

//-----------------------------------------------------------------------------
// TASK_SCRIPT_RUN_TO_TARGET / TASK_SCRIPT_WALK_TO_TARGET / TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartScriptMoveToTargetTask( TaskId_t task )
{
	Activity newActivity;

	if ( m_hTargetEnt == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if ( (m_hTargetEnt->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
	{
		TaskComplete();
	}
	else
	{
		//
		// Select the appropriate activity.
		//
		if ( task == TASK_SCRIPT_WALK_TO_TARGET )
		{
			newActivity = ACT_WALK;
		}
		else if ( task == TASK_SCRIPT_RUN_TO_TARGET )
		{
			newActivity = ACT_RUN;
		}
		else
		{
			newActivity = GetScriptCustomMoveActivity();
		}

		if ( ( newActivity != ACT_SCRIPT_CUSTOM_MOVE ) && TranslateActivity( newActivity ) == ACT_INVALID )
		{
			// This NPC can't do this!
			Assert( 0 );
		}
		else 
		{
			if (m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{

				AI_NavGoal_t goal( GOALTYPE_TARGETENT, newActivity );
				
				if ( GetState() == NPC_STATE_SCRIPT && 
					 ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY || 
					   m_strScriptArrivalSequence != NULL_STRING ) )
				{
					if ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY )
					{
						goal.arrivalActivity = m_ScriptArrivalActivity;
					}
					else
					{
						goal.arrivalSequence = LookupSequence( m_strScriptArrivalSequence.ToCStr() );
					}
				}
					
				if (!GetNavigator()->SetGoal( goal, AIN_DISCARD_IF_FAIL ))
				{
					if ( GetNavigator()->GetNavFailCounter() == 0 )
					{
						// no path was built, but OnNavFailed() did something so that next time it may work
						DevWarning("%s %s failed Urgent Movement, retrying\n", GetDebugName(), TaskName( task ) );
						return;
					}

					// FIXME: scripted sequences don't actually know how to handle failure, but we're failing.  This is serious
					DevWarning("%s %s failed Urgent Movement, abandoning schedule\n", GetDebugName(), TaskName( task ) );
					TaskFail(FAIL_NO_ROUTE);
				}
				else
				{
					GetNavigator()->SetArrivalDirection( m_hTargetEnt->GetAbsAngles() );
				}
			}
		}
	}

	m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
	m_strScriptArrivalSequence = NULL_STRING;

	TaskComplete();
}

//-----------------------------------------------------------------------------
// Start task!
//-----------------------------------------------------------------------------
void CAI_BaseNPC::StartTask( const Task_t *pTask )
{
	TaskId_t task = pTask->iTask;

	if( task >= 0 && task < LAST_SHARED_TASK ) {
		(this->*s_DefaultTasks[ task ][ 0 ])( pTask );
		return;
	}

	DevMsg( "No StartTask entry for %s\n", TaskName( task ) );
	TaskFail(FAIL_UNIMPLEMENTED);
}

//=========================================================
// RunTask 
//=========================================================
void CAI_BaseNPC::RunTask( const Task_t *pTask )
{
	VPROF_BUDGET( "CAI_BaseNPC::RunTask", VPROF_BUDGETGROUP_NPCS );

	TaskId_t task = pTask->iTask;

	if( task >= 0 && task < LAST_SHARED_TASK ) {
		(this->*s_DefaultTasks[ task ][ 1 ])( pTask );
		return;
	}

	DevMsg( "No RunTask entry for %s\n", TaskName( pTask->iTask ) );
	TaskFail(FAIL_UNIMPLEMENTED);
}

void CAI_BaseNPC::StartTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		if ( ShouldMoveAndShoot() )
		{
			m_MoveAndShootOverlay.StartShootWhileMove();
		}
		else
		{
			m_MoveAndShootOverlay.NoShootWhileMove();
		}
	}
}


//-----------------------------------------------------------------------------
// TASK_DIE.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RunDieTask()
{
	AutoMovement();

	if ( IsActivityFinished() && GetCycle() >= 1.0f )
	{
		m_lifeState = LIFE_DEAD;
		
		SetThink ( NULL );
		StopAnimation();

		if ( !BBoxFlat() )
		{
			// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
			// block the player on a slope or stairs, the corpse is made nonsolid. 
//					SetSolid( SOLID_NOT );
			UTIL_SetSize ( this, Vector ( -4, -4, 0 ), Vector ( 4, 4, 1 ) );
		}
		else // !!!HACKHACK - put NPC in a thin, wide bounding box until we fix the solid type/bounding volume problem
			UTIL_SetSize ( this, WorldAlignMins(), Vector ( WorldAlignMaxs().x, WorldAlignMaxs().y, WorldAlignMins().z + 1 ) );
	}
}


//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1 / TASK_RANGE_ATTACK2 / etc.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RunAttackTask( TaskId_t task )
{
	AutoMovement( );

	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if( vecEnemyLKP != vec3_origin )
	{
		if ( ( task == TASK_RANGE_ATTACK1 || task == TASK_RELOAD ) && 
			 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
			 FInAimCone( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	if ( IsActivityFinished() )
	{
		if ( task == TASK_RELOAD && GetShotRegulator() )
		{
			GetShotRegulator()->Reset( false );
		}

		TaskComplete();
	}
}

void CAI_BaseNPC::RunTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		m_MoveAndShootOverlay.RunShootWhileMove();
	}
}

void CAI_BaseNPC::EndTaskOverlay()
{
	m_MoveAndShootOverlay.EndShootWhileMove();
}

//=========================================================
// SetTurnActivity - measures the difference between the way
// the NPC is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CAI_BaseNPC::SetTurnActivity ( void )
{
	if ( IsCrouching() )
	{
		SetIdealActivity( ACT_IDLE ); // failure case
		return;
	}

	float flYD;
	flYD = GetMotor()->DeltaIdealYaw();

	// Allow AddTurnGesture() to decide this
	if (GetMotor()->AddTurnGesture( flYD ))
	{
		SetIdealActivity( ACT_IDLE );
		Remember( bits_MEMORY_TURNING );
		return;
	}

	if( flYD <= -80 && flYD >= -100 && SelectWeightedSequence( ACT_90_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree right.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_RIGHT );
		return;
	}
	if( flYD >= 80 && flYD <= 100 && SelectWeightedSequence( ACT_90_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree left.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_LEFT );
		return;
	}
	if( fabs( flYD ) >= 160 && SelectWeightedSequence ( ACT_180_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_180_LEFT );
		return;
	}

	if ( flYD <= -45 && SelectWeightedSequence ( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		SetIdealActivity( ACT_TURN_RIGHT );
		return;
	}
	if ( flYD >= 45 && SelectWeightedSequence ( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		SetIdealActivity( ACT_TURN_LEFT );
		return;
	}

	SetIdealActivity( ACT_IDLE ); // failure case

}


//-----------------------------------------------------------------------------
// Purpose: For a specific delta, add a turn gesture and set the yaw speed
// Input  : yaw delta
//-----------------------------------------------------------------------------


bool CAI_BaseNPC::UpdateTurnGesture( void )
{
	float flYD = GetMotor()->DeltaIdealYaw();
	return GetMotor()->AddTurnGesture( flYD );
}


//-----------------------------------------------------------------------------
// Purpose: For non-looping animations that may be replayed sequentially (like attacks)
//			Set the activity to ACT_RESET if this is a replay, otherwise just set ideal activity
// Input  : newIdealActivity - desired ideal activity
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ResetIdealActivity( Activity newIdealActivity )
{
	if ( m_Activity == newIdealActivity )
	{
		m_Activity = ACT_RESET;
	}

	SetIdealActivity( newIdealActivity );
}

			
void CAI_BaseNPC::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	if ( GetNavType() == NAV_FLY )
	{
		// UNDONE: Cache these per enemy instead?
		Vector offset = pEnemy->EyePosition() - pEnemy->GetAbsOrigin();
		chasePosition += offset;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the custom movement activity for the script that this NPC
//			is running.
// Output : Returns the activity, or ACT_INVALID is the sequence is unknown.
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetScriptCustomMoveActivity( void )
{
	Activity eActivity = ACT_WALK;

	if ( ( m_hCine != NULL ) && ( m_hCine->m_iszCustomMove != NULL_STRING ) )
	{
		// We have a valid script. Look up the custom movement activity.
		eActivity = ( Activity )LookupActivity( STRING( m_hCine->m_iszCustomMove ) );
		if ( eActivity == ACT_INVALID )
		{
			// Not an activity, at least make sure it's a valid sequence.
			if ( LookupSequence( STRING( m_hCine->m_iszCustomMove ) ) != ACT_INVALID )
			{
				eActivity = ACT_SCRIPT_CUSTOM_MOVE;
			}
			else
			{
				eActivity = ACT_WALK;
			}
		}
	}
	else if ( m_iszSceneCustomMoveSeq != NULL_STRING )
	{
		eActivity = ACT_SCRIPT_CUSTOM_MOVE;
	}

	return eActivity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetScriptCustomMoveSequence( void )
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	// If we have a scripted sequence entity, use it's custom move
	if ( m_hCine != NULL )
	{
		iSequence = LookupSequence( STRING( m_hCine->m_iszCustomMove ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			DevMsg( "SCRIPT_CUSTOM_MOVE: %s has no sequence:%s\n", GetClassname(), STRING(m_hCine->m_iszCustomMove) );
		}
	}
	else if ( m_iszSceneCustomMoveSeq != NULL_STRING )
	{
		// Otherwise, use the .vcd custom move
		iSequence = LookupSequence( STRING( m_iszSceneCustomMoveSeq ) );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			Warning( "SCRIPT_CUSTOM_MOVE: %s failed scripted custom move. Has no sequence called: %s\n", GetClassname(), STRING(m_iszSceneCustomMoveSeq) );
		}
	}

	// Failed? Use walk.
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
	{
		iSequence = SelectWeightedSequence( ACT_WALK );
	}

	return iSequence;
}

//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
const Task_t *CAI_BaseNPC::GetTask( void ) 
{
	int iScheduleIndex = GetScheduleCurTaskIndex();
	if ( !GetCurSchedule() ||  iScheduleIndex < 0 || iScheduleIndex >= GetCurSchedule()->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &GetCurSchedule()->GetTaskList()[ iScheduleIndex ];
}


void CAI_BaseNPC::TranslateAddOnAttachment( char *pchAttachmentName, int iCount )
{
#ifdef HL2_DLL
	if( Classify() == CLASS_ZOMBIE || ClassMatches( "npc_combine*" ) )
	{
		if ( Q_strcmp( pchAttachmentName, "addon_rear" ) == 0 || 
			 Q_strcmp( pchAttachmentName, "addon_front" ) == 0 || 
			 Q_strcmp( pchAttachmentName, "addon_rear_or_front" ) == 0 )
		{
			if ( iCount == 0 )
			{
				Q_strcpy( pchAttachmentName, "eyes" );
			}
			else
			{
				Q_strcpy( pchAttachmentName, "" );
			}

			return;
		}
	}
#endif

	if( Q_strcmp( pchAttachmentName, "addon_baseshooter" ) == 0 )
	{
		switch ( iCount )
		{
		case 0:
			Q_strcpy( pchAttachmentName, "anim_attachment_lh" );
			break;

		case 1:
			Q_strcpy( pchAttachmentName, "anim_attachment_rh" );
			break;

		default:
			Q_strcpy( pchAttachmentName, "" );
		}

		return;
	}

	Q_strcpy( pchAttachmentName, "" );
}

//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsInterruptable()
{
	if ( GetState() == NPC_STATE_SCRIPT )
	{
		if ( m_hCine )
		{
			if (!m_hCine->CanInterrupt() )
				return false;

			// are the in an script FL_FLY state?
			if ((GetFlags() & FL_FLY ) && !(m_hCine->m_savedFlags & FL_FLY))
			{
				return false;
			}
		}
	}
	
	return IsAlive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectInteractionSchedule( void )
{
	SetTarget( m_hForcedInteractionPartner.Get() );

	// If we have an interaction, we're the initiator. Move to our interaction point.
	if ( m_iInteractionPlaying != NPCINT_NONE )
		return SCHED_INTERACTION_MOVE_TO_PARTNER;

	// Otherwise, turn towards our partner and wait for him to reach us.
	//m_iInteractionState = NPCINT_MOVING_TO_MARK;
	return SCHED_INTERACTION_WAIT_FOR_PARTNER;
}

//-----------------------------------------------------------------------------
// Idle schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectIdleSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition ( COND_HEAR_DANGER ) ||
		 HasCondition ( COND_HEAR_COMBAT ) ||
		 HasCondition ( COND_HEAR_WORLD  ) ||
		 HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
		 HasCondition ( COND_HEAR_PLAYER ) )
	{
		return SCHED_ALERT_FACE_BESTSOUND;
	}
	
	// no valid route!
	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
		return SCHED_IDLE_STAND;

	// valid route. Get moving
	return SCHED_IDLE_WALK;
}


//-----------------------------------------------------------------------------
// Alert schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectAlertSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	// Scan around for new enemies
	if ( HasCondition( COND_ENEMY_DEAD ) && SelectWeightedSequence( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
		return SCHED_ALERT_SCAN;

	if( IsPlayerAlly() && HasCondition(COND_HEAR_COMBAT) )
	{
		return SCHED_ALERT_REACT_TO_COMBAT_SOUND;
	}

	if ( HasCondition ( COND_HEAR_DANGER ) ||
			  HasCondition ( COND_HEAR_PLAYER ) ||
			  HasCondition ( COND_HEAR_WORLD  ) ||
			  HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
			  HasCondition ( COND_HEAR_COMBAT ) )
	{
		return SCHED_ALERT_FACE_BESTSOUND;
	}

	if ( gpGlobals->curtime - GetEnemies()->LastTimeSeen( AI_UNKNOWN_ENEMY ) < TIME_CARE_ABOUT_DAMAGE )
		return SCHED_ALERT_FACE;

	return SCHED_ALERT_STAND;
}


//-----------------------------------------------------------------------------
// Combat schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectCombatSchedule()
{
	if ( m_hForcedInteractionPartner )
		return SelectInteractionSchedule();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( HasCondition(COND_NEW_ENEMY) && gpGlobals->curtime - GetEnemies()->FirstTimeSeen(GetEnemy()) < 2.0 )
	{
		return SCHED_WAKE_ANGRY;
	}
	
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		// clear the current (dead) enemy and try to find another.
		SetEnemy( NULL );
		 
		if ( ChooseEnemy() )
		{
			ClearCondition( COND_ENEMY_DEAD );
			return SelectSchedule();
		}

		SetState( NPC_STATE_ALERT );
		return SelectSchedule();
	}
	
	// If I'm scared of this enemy run away
	if ( IRelationType( GetEnemy() ) == D_FR )
	{
		if (HasCondition( COND_SEE_ENEMY )	|| 
			HasCondition( COND_LIGHT_DAMAGE )|| 
			HasCondition( COND_HEAVY_DAMAGE ))
		{
			FearSound();
			//ClearCommandGoal();
			return SCHED_RUN_FROM_ENEMY;
		}

		// If I've seen the enemy recently, cower. Ignore the time for unforgettable enemies.
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find( GetEnemy() );
		if ( (pMemory && pMemory->bUnforgettable) || (GetEnemyLastTimeSeen() > (gpGlobals->curtime - 5.0)) )
		{
			// If we're facing him, just look ready. Otherwise, face him.
			if ( FInAimCone( GetEnemy()->EyePosition() ) )
				return SCHED_COMBAT_STAND;

			return SCHED_FEAR_FACE;
		}
	}

	// Check if need to reload
	if ( HasCondition( COND_LOW_PRIMARY_AMMO ) || HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}

	// Can we see the enemy?
	if ( !HasCondition(COND_SEE_ENEMY) )
	{
		// enemy is unseen, but not occluded!
		// turn to face enemy
		if ( !HasCondition(COND_ENEMY_OCCLUDED) )
			return SCHED_COMBAT_FACE;

		// chase!
		if ( GetActiveWeapon() || (CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2)))
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		else if ( (CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2)))
			return SCHED_CHASE_ENEMY;
		else
			return SCHED_TAKE_COVER_FROM_ENEMY;
	}
	
	if ( HasCondition(COND_TOO_CLOSE_TO_ATTACK) ) 
		return SCHED_BACK_AWAY_FROM_ENEMY;
	
	if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ) || 
			HasCondition( COND_WEAPON_BLOCKED_BY_FRIEND ) || 
			HasCondition( COND_WEAPON_SIGHT_OCCLUDED ) )
	{
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}

	if ( GetShotRegulator()->IsInRestInterval() )
	{
		if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
			return SCHED_COMBAT_FACE;
	}

	// we can see the enemy
	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		if ( !UseAttackSquadSlots() || OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return SCHED_RANGE_ATTACK1;
		return SCHED_COMBAT_FACE;
	}

	if ( HasCondition(COND_CAN_RANGE_ATTACK2) )
		return SCHED_RANGE_ATTACK2;

	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	if ( HasCondition(COND_CAN_MELEE_ATTACK2) )
		return SCHED_MELEE_ATTACK2;

	if ( HasCondition(COND_NOT_FACING_ATTACK) )
		return SCHED_COMBAT_FACE;

	if ( !HasCondition(COND_CAN_RANGE_ATTACK1) && !HasCondition(COND_CAN_MELEE_ATTACK1) )
	{
		// if we can see enemy but can't use either attack type, we must need to get closer to enemy
		if ( GetActiveWeapon() )
			return SCHED_MOVE_TO_WEAPON_RANGE;

		// If we have an innate attack and we're too far (or occluded) then get line of sight
		if ( HasCondition( COND_TOO_FAR_TO_ATTACK ) && ( CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2)) )
			return SCHED_MOVE_TO_WEAPON_RANGE;

		// if we can see enemy but can't use either attack type, we must need to get closer to enemy
		if ( CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2) )
			return SCHED_CHASE_ENEMY;
		else
			return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	DevWarning( 2, "No suitable combat schedule!\n" );
	return SCHED_FAIL;
}


//-----------------------------------------------------------------------------
// Dead schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectDeadSchedule()
{
	if ( BecomeRagdollOnClient( vec3_origin ) )
	{
		CleanupOnDeath();
		return SCHED_DIE_RAGDOLL;
	}

	// Adrian - Alread dead (by animation event maybe?)
	// Is it safe to set it to SCHED_NONE?
	if ( m_lifeState == LIFE_DEAD )
		 return SCHED_NONE;

	CleanupOnDeath();
	return SCHED_DIE;
}


//-----------------------------------------------------------------------------
// Script schedule selection
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectScriptSchedule()
{
	Assert( m_hCine != NULL );
	if ( m_hCine )
		return SCHED_AISCRIPT;

	DevWarning( 2, "Script failed for %s\n", GetClassname() );
	CineCleanup();
	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: Select a gesture to play in response to damage we've taken
// Output : int
//-----------------------------------------------------------------------------
void CAI_BaseNPC::PlayFlinchGesture()
{
	if ( !CanFlinch() )
		return;

	Activity iFlinchActivity = ACT_INVALID;

	float flNextFlinch = random_valve->RandomFloat( 0.5f, 1.0f );

	// If I haven't flinched for a while, play the big flinch gesture
	if ( !HasMemory(bits_MEMORY_FLINCHED) )
	{
		iFlinchActivity = GetFlinchActivity( true, true );

		if ( HaveSequenceForActivity( iFlinchActivity ) )
		{
			RestartGesture( iFlinchActivity );
		}

		Remember(bits_MEMORY_FLINCHED);

	}
	else
	{
		iFlinchActivity = GetFlinchActivity( false, true );
		if ( HaveSequenceForActivity( iFlinchActivity ) )
		{
			RestartGesture( iFlinchActivity );
		}
	}

	if ( iFlinchActivity != ACT_INVALID )
	{
		//Get the duration of the flinch and delay the next one by that (plus a bit more)
		int iSequence = GetLayerSequence( FindGestureLayer( iFlinchActivity ) );

		if ( iSequence != ACT_INVALID )
		{
			flNextFlinch += SequenceDuration( iSequence );
		}

		m_flNextFlinchTime = gpGlobals->curtime + flNextFlinch;
	}
}

//-----------------------------------------------------------------------------
// Purpose: See if we should flinch in response to damage we've taken
// Output : int
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectFlinchSchedule()
{
	if ( !HasCondition(COND_HEAVY_DAMAGE) )
		return SCHED_NONE;

	// If we've flinched recently, don't do it again. A gesture flinch will be played instead.
	if ( HasMemory(bits_MEMORY_FLINCHED) )
		return SCHED_NONE;

	if ( !CanFlinch() )
		return SCHED_NONE;

	// Robin: This was in the original HL1 flinch code. Do we still want it?
	//if ( fabs( GetMotor()->DeltaIdealYaw() ) < (1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
	//	return SCHED_TAKE_COVER_FROM_ORIGIN;

	// Heavy damage. Break out of my current schedule and flinch.
	Activity iFlinchActivity = GetFlinchActivity( true, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_BIG_FLINCH;

	/*
	// Not used anymore, because gesture flinches are played instead for heavy damage
	// taken shortly after we've already flinched full.
	//
	iFlinchActivity = GetFlinchActivity( false, false );
	if ( HaveSequenceForActivity( iFlinchActivity ) )
		return SCHED_SMALL_FLINCH;
	*/

	return SCHED_NONE;
}
		
//-----------------------------------------------------------------------------
// Purpose: Decides which type of schedule best suits the NPC's current 
// state and conditions. Then calls NPC's member function to get a pointer 
// to a schedule of the proper type.
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectSchedule( void )
{
	if ( HasCondition( COND_FLOATING_OFF_GROUND ) )
	{
		SetGravity( 1.0 );
		SetGroundEntity( NULL );
		return SCHED_FALL_TO_GROUND;
	}

	switch( m_NPCState )
	{
	case NPC_STATE_NONE:
		DevWarning( 2, "NPC_STATE IS NONE!\n" );
		break;

	case NPC_STATE_PRONE:
		return SCHED_IDLE_STAND;

	case NPC_STATE_IDLE:
		AssertMsgOnce( GetEnemy() == NULL, "NPC has enemy but is not in combat state?" );
		return SelectIdleSchedule();

	case NPC_STATE_ALERT:
		AssertMsgOnce( GetEnemy() == NULL, "NPC has enemy but is not in combat state?" );
		return SelectAlertSchedule();

	case NPC_STATE_COMBAT:
		return SelectCombatSchedule();

	case NPC_STATE_DEAD:
		return SelectDeadSchedule();

	case NPC_STATE_SCRIPT:
		return SelectScriptSchedule();

	default:
		DevWarning( 2, "Invalid State for SelectSchedule!\n" );
		break;
	}

	return SCHED_FAIL;
}


//-----------------------------------------------------------------------------

int CAI_BaseNPC::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return ( m_failSchedule != SCHED_NONE ) ? m_failSchedule : SCHED_FAIL;
}
