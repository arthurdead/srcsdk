//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_behavior.h"
#include "scripted.h"
#include "env_debughistory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_bBehaviorHost_PreventBaseClassGatherConditions;

CGenericClassmap< CAI_BehaviorBase >	CAI_BehaviorBase::m_BehaviorClasses;

//-----------------------------------------------------------------------------
// CAI_BehaviorBase
//-----------------------------------------------------------------------------

//-------------------------------------

CAI_ClassScheduleIdSpace *CAI_BehaviorBase::GetClassScheduleIdSpace()
{
	return GetOuter()->GetClassScheduleIdSpace();
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays (override in subclass to add additional text)
// Input  : Previous text offset from the top
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_BehaviorBase::DrawDebugTextOverlays( int text_offset )
{
	char	tempstr[ 512 ];

	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		Q_snprintf( tempstr, sizeof( tempstr ), "Behv: %s, ", GetName() );
		GetOuter()->EntityText( text_offset, tempstr, 0 );
		text_offset++;

		for ( int i = 0; i < m_ScheduleChannels.Count(); i++ )
		{
			AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[i];

			if ( pScheduleState->bActive && pScheduleState->pSchedule )
			{
				const char *pName = NULL;
				pName = pScheduleState->pSchedule->GetName();
				if ( !pName )
				{
					pName = "Unknown";
				}
				Q_snprintf(tempstr,sizeof(tempstr),"    Schd [%d]: %s, ", i, pName );
				GetOuter()->EntityText(text_offset,tempstr,0);
				text_offset++;

				const Task_t *pTask = GetCurTask( i );
				if ( pTask )
				{
					Q_snprintf(tempstr,sizeof(tempstr),"    Task [%d]: %s (#%d), ", i, GetSchedulingSymbols()->TaskIdToSymbol( GetClassScheduleIdSpace()->TaskLocalToGlobal( pTask->iTask ) ), pScheduleState->iCurTask );
				}
				else
				{
					Q_snprintf(tempstr,sizeof(tempstr),"    Task [%d]: None",i);
				}
				GetOuter()->EntityText(text_offset,tempstr,0);
				text_offset++;

			}

		}
	}

	return text_offset;
}

//-------------------------------------

void CAI_BehaviorBase::GatherConditions()
{
	Assert( m_pBackBridge != NULL );
	
	m_pBackBridge->BehaviorBridge_GatherConditions();
}

//-------------------------------------

void CAI_BehaviorBase::OnStartSchedule( int scheduleType )
{
}

//-------------------------------------

int CAI_BehaviorBase::SelectSchedule()
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BehaviorBridge_SelectSchedule();
}

//-------------------------------------

int CAI_BehaviorBase::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	m_fOverrode = false; 
	return SCHED_NONE;
}

//-------------------------------------

void CAI_BehaviorBase::StartTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

void CAI_BehaviorBase::RunTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

int CAI_BehaviorBase::TranslateSchedule( int scheduleType )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BehaviorBridge_TranslateSchedule( scheduleType );
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetNewSchedule( int channel )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return NULL;
	}

	int scheduleType;

	//
	// Schedule selection code here overrides all leaf schedule selection.
	//
	scheduleType = SelectSchedule( channel );
	CAI_Schedule *pSchedule = GetSchedule( scheduleType );

	if( pSchedule != NULL )
		m_ScheduleChannels[channel].idealSchedule = pSchedule->GetId();

	return pSchedule;
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetFailSchedule( AIChannelScheduleState_t *pScheduleState )
{
	int prevSchedule;
	int failedTask;

	if ( pScheduleState->pSchedule )
		prevSchedule = GetClassScheduleIdSpace()->ScheduleGlobalToLocal( pScheduleState->pSchedule->GetId() );
	else
		prevSchedule = SCHED_NONE;

	const Task_t *pTask = GetTask( pScheduleState );
	if ( pTask )
		failedTask = pTask->iTask;
	else
		failedTask = TASK_INVALID;

	Assert( AI_IdIsLocal( prevSchedule ) );
	Assert( AI_IdIsLocal( failedTask ) );

	int scheduleType = SelectFailSchedule( prevSchedule, failedTask, pScheduleState->taskFailureCode );
	return GetSchedule( scheduleType );
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetSchedule(int schedule)
{
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		Warning("ERROR: %s missing schedule!\n", GetSchedulingErrorName());
		return g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
	}

	if( schedule == SCHED_NONE )
		return NULL;

	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}

	if ( schedule == -1 )
		return NULL;

	return g_AI_SchedulesManager.GetScheduleFromID( schedule );
}

//-------------------------------------

const Task_t *CAI_BehaviorBase::GetTask( AIChannelScheduleState_t *pScheduleState ) 
{
	int iScheduleIndex = pScheduleState->iCurTask;
	if ( !pScheduleState->pSchedule || iScheduleIndex < 0 || iScheduleIndex >= pScheduleState->pSchedule->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &pScheduleState->pSchedule->GetTaskList()[ iScheduleIndex ];
}

//-------------------------------------

bool CAI_BehaviorBase::IsCurSchedule( int schedule, bool fIdeal )
{
	if ( AI_IdIsLocal( schedule ) )
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);

	return GetOuter()->IsCurSchedule( schedule, fIdeal );
}

//-------------------------------------

const char *CAI_BehaviorBase::GetSchedulingErrorName()
{
	return "CAI_Behavior";
}

//-------------------------------------

void CAI_BehaviorBase::StartChannel( int channel )
{
	if ( channel < 0 )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	m_ScheduleChannels.EnsureCount( channel + 1 );
	m_ScheduleChannels[channel].bActive = true;
}


//-------------------------------------
void CAI_BehaviorBase::StopChannel( int channel )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	m_ScheduleChannels[channel].bActive = false;
}


//-------------------------------------

void CAI_BehaviorBase::MaintainChannelSchedules()
{
	for ( int i = 0; i < m_ScheduleChannels.Count(); i++ )
	{
		MaintainSchedule( i );
	}
}

//-------------------------------------

#define MAX_TASKS_RUN 10

void CAI_BehaviorBase::MaintainSchedule( int channel )
{
	CAI_Schedule *pNewSchedule;
	bool		runTask = true;

	AssertMsg( m_ScheduleChannels.IsValidIndex( channel ), "Bad schedule channel" );

	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	if ( !pScheduleState->bActive )
	{
		return;
	}

	int i;

	bool bStopProcessing = false;
	for ( i = 0; i < MAX_TASKS_RUN && !bStopProcessing; i++ )
	{
		if ( pScheduleState->pSchedule != NULL && pScheduleState->fTaskStatus == TASKSTATUS_COMPLETE )
		{
			// Schedule is valid, so advance to the next task if the current is complete.
			pScheduleState->fTaskStatus = TASKSTATUS_NEW;
			pScheduleState->iCurTask++;
		}

		// validate existing schedule 
		if ( !IsScheduleValid( pScheduleState ) )
		{
			// Notify the NPC that his schedule is changing
			pScheduleState->bScheduleWasInterrupted = true;
			OnScheduleChange( channel );

// 			if ( !m_bConditionsGathered )
// 			{
// 				// occurs if a schedule is exhausted within one think
// 				GatherConditions();
// 			}

			if ( pScheduleState->taskFailureCode != NO_TASK_FAILURE )
			{
				// Get a fail schedule if the previous schedule failed during execution and 
				// the NPC is still in its ideal state. Otherwise, the NPC would immediately
				// select the same schedule again and fail again.

				if ( GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT )
				{
					DevMsg( GetOuter(), AIMF_IGNORE_SELECTED, "      Behavior %s channel %d (failed)\n", GetName(), channel );
				}

				ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      Behavior %s channel %d (failed)\n", GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel ) );

				pNewSchedule = GetFailSchedule( pScheduleState );
				pScheduleState->idealSchedule = pNewSchedule->GetId();
				DevWarning( 2, "(%s) Schedule (%s) Failed at %d!\n", STRING( GetOuter()->GetEntityName() ), pScheduleState->pSchedule ? pScheduleState->pSchedule->GetName() : "GetCurSchedule() == NULL", pScheduleState->iCurTask );
				SetSchedule( channel, pNewSchedule );
			}
			else
			{
				pNewSchedule = GetNewSchedule( channel );

				SetSchedule( channel, pNewSchedule );
			}
		}

		if ( !pScheduleState->pSchedule )
		{
			pNewSchedule = GetNewSchedule( channel );

			if (pNewSchedule)
			{
				SetSchedule( channel, pNewSchedule );
			}
		}

		if ( !pScheduleState->pSchedule || pScheduleState->pSchedule->NumTasks() == 0 )
		{
			return;
		}

		if ( pScheduleState->fTaskStatus == TASKSTATUS_NEW )
		{	
			if ( pScheduleState->iCurTask == 0 )
			{
				int globalId = pScheduleState->pSchedule->GetId();
				int localId = GetClassScheduleIdSpace()->ScheduleGlobalToLocal( globalId );
				OnStartSchedule( channel, (localId != -1)? localId : globalId );
			}

			const Task_t *pTask = GetCurTask( channel );
			Assert( pTask != NULL );

			if ( GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT )
			{
				DevMsg( GetOuter(), AIMF_IGNORE_SELECTED, "  Behavior %s channel %d Task: %s\n", GetName(), channel, GetSchedulingSymbols()->TaskIdToSymbol( GetClassScheduleIdSpace()->TaskLocalToGlobal( pTask->iTask ) ) );
			}

			ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):  Behavior %s channel %d Task: %s\n", GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel, GetSchedulingSymbols()->TaskIdToSymbol( GetClassScheduleIdSpace()->TaskLocalToGlobal( pTask->iTask ) ) ) );

			pScheduleState->fTaskStatus = TASKSTATUS_RUN_MOVE_AND_TASK;
			pScheduleState->taskFailureCode    = NO_TASK_FAILURE;
			pScheduleState->timeCurTaskStarted = gpGlobals->curtime;

			StartTask( channel, pTask );
		}

		if ( !TaskIsComplete( channel ) && pScheduleState->fTaskStatus != TASKSTATUS_NEW )
		{
			if ( pScheduleState->fTaskStatus != TASKSTATUS_COMPLETE && pScheduleState->fTaskStatus != TASKSTATUS_RUN_MOVE && pScheduleState->taskFailureCode == NO_TASK_FAILURE && runTask )
			{
				const Task_t *pTask = GetCurTask( channel );
				Assert( pTask != NULL );

				RunTask( channel, pTask );

				if ( !TaskIsComplete( channel ) )
				{
					bStopProcessing = true;
				}
			}
			else
			{
				bStopProcessing = true;
			}
		}
	}
}

//-------------------------------------

bool CAI_BehaviorBase::IsScheduleValid( AIChannelScheduleState_t *pScheduleState )
{
	CAI_Schedule *pSchedule = pScheduleState->pSchedule;
	if ( !pSchedule || pSchedule->NumTasks() == 0 )
	{
		return false;
	}

	if ( pScheduleState->iCurTask == pSchedule->NumTasks() )
	{
		return false;
	}

	if ( pScheduleState->taskFailureCode != NO_TASK_FAILURE )
	{
		return false;
	}

	//Start out with the base schedule's set interrupt conditions
	CAI_ScheduleBits testBits;
	pSchedule->GetInterruptMask( &testBits );
	testBits.And( GetOuter()->AccessConditionBits(), &testBits  );

	if (!testBits.IsAllClear()) 
	{
		// If in developer mode save the interrupt text for debug output
		if (developer.GetInt()) 
		{
			// Find the first non-zero bit
			for (int i=0;i<MAX_CONDITIONS;i++)
			{
				if (testBits.IsBitSet(i))
				{
					int channel = pScheduleState - m_ScheduleChannels.Base();
					const char *pszInterruptText = GetOuter()->ConditionName( AI_RemapToGlobal( i ) );
					if (!pszInterruptText)
					{
						pszInterruptText = "(UNKNOWN CONDITION)";
					}

					if (GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
					{
						DevMsg(GetOuter(), AIMF_IGNORE_SELECTED, "      Behavior %s channel %d Break condition -> %s\n", GetName(), channel, pszInterruptText );
					}

					ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):      Behavior %s channel %d Break condition -> %s\n",  GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel, pszInterruptText ) );

					break;
				}
			}
		}

		return false;
	}

	return true;
}

//-------------------------------------

void CAI_BehaviorBase::SetSchedule( int channel, CAI_Schedule *pNewSchedule )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	//Assert( pNewSchedule != NULL );
	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	pScheduleState->timeCurTaskStarted = pScheduleState->timeStarted = gpGlobals->curtime;
	pScheduleState->bScheduleWasInterrupted = false;

	pScheduleState->pSchedule = pNewSchedule ;
	pScheduleState->iCurTask = 0;
	pScheduleState->fTaskStatus = TASKSTATUS_NEW;
	pScheduleState->failSchedule = SCHED_NONE;

	// this is very useful code if you can isolate a test case in a level with a single NPC. It will notify
	// you of every schedule selection the NPC makes.

	if( pNewSchedule )
	{
		if (GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
		{
			DevMsg(GetOuter(), AIMF_IGNORE_SELECTED, "Behavior %s channel %d Schedule: %s (time: %.2f)\n", GetName(), channel, pNewSchedule->GetName(), gpGlobals->curtime );
		}

		ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d): Behavior %s channel %d Schedule: %s (time: %.2f)\n", GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel, pNewSchedule->GetName(), gpGlobals->curtime ) );
	}
}

//-------------------------------------

bool CAI_BehaviorBase::SetSchedule( int channel, int localScheduleID )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return false;
	}

	int translatedSchedule = TranslateSchedule( channel, localScheduleID );
	CAI_Schedule *pNewSchedule = GetSchedule( translatedSchedule );
	if ( pNewSchedule )
	{
		AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

		if ( AI_IdIsLocal( localScheduleID ) )
		{
			pScheduleState->idealSchedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal( localScheduleID );
		}
		else
		{
			pScheduleState->idealSchedule = localScheduleID;
		}
		SetSchedule( channel, pNewSchedule ); 
		return true;
	}
	return false;
}

//-------------------------------------

void CAI_BehaviorBase::ClearSchedule( int channel, const char *szReason )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	if (szReason && GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg( GetOuter(), AIMF_IGNORE_SELECTED, "  Behavior %s channel %d Schedule cleared: %s\n", GetName(), channel, szReason );
	}

	if ( szReason )
	{
		ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs( "%s(%d):  Behavior %s channel %d Schedule cleared: %s\n", GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel, szReason ) );
	}

	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	pScheduleState->timeCurTaskStarted = pScheduleState->timeStarted = 0;
	pScheduleState->bScheduleWasInterrupted = true;
	pScheduleState->fTaskStatus = TASKSTATUS_NEW;
	pScheduleState->idealSchedule = AI_RemapToGlobal( SCHED_NONE );
	pScheduleState->pSchedule = NULL;
	pScheduleState->iCurTask = 0;
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetCurSchedule( int channel )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return NULL;
	}

	return m_ScheduleChannels[channel].pSchedule;
}

//-------------------------------------

bool CAI_BehaviorBase::IsCurSchedule( int channel, int schedId, bool fIdeal )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return false;
	}

	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	if ( !pScheduleState->pSchedule )
		return ( schedId == SCHED_NONE || schedId == AI_RemapToGlobal(SCHED_NONE) );

	schedId = ( AI_IdIsLocal( schedId ) ) ? GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedId) : schedId;

	if ( fIdeal )
		return ( schedId == pScheduleState->idealSchedule );

	return ( pScheduleState->pSchedule->GetId() == schedId ); 
}

//-------------------------------------

void CAI_BehaviorBase::OnScheduleChange( int channel )
{
}

//-------------------------------------

void CAI_BehaviorBase::OnStartSchedule( int channel, int scheduleType )
{
}

//-------------------------------------

int CAI_BehaviorBase::SelectSchedule( int channel )
{
	return SCHED_NONE;
}

//-------------------------------------

int CAI_BehaviorBase::SelectFailSchedule(  int channel, int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return SCHED_NONE;
}

//-------------------------------------

void CAI_BehaviorBase::TaskComplete( int channel, bool fIgnoreSetFailedCondition )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	if ( fIgnoreSetFailedCondition || m_ScheduleChannels[channel].taskFailureCode == NO_TASK_FAILURE )
	{
		m_ScheduleChannels[channel].taskFailureCode = NO_TASK_FAILURE;
		m_ScheduleChannels[channel].fTaskStatus = TASKSTATUS_COMPLETE;
	}
}

//-------------------------------------

void CAI_BehaviorBase::TaskFail( int channel, AI_TaskFailureCode_t code )
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return;
	}

	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	if ( GetOuter()->m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		DevMsg( GetOuter(), AIMF_IGNORE_SELECTED, "      Behavior %s channel %d: TaskFail -> %s\n", GetName(), channel, TaskFailureToString( code ) );
	}

	ADD_DEBUG_HISTORY( HISTORY_AI_DECISIONS, UTIL_VarArgs("%s(%d):       Behavior %s channel %d: TaskFail -> %s\n", GetOuter()->GetDebugName(), GetOuter()->entindex(), GetName(), channel, TaskFailureToString( code ) ) );

	pScheduleState->taskFailureCode = code;
}

//-------------------------------------

const Task_t *CAI_BehaviorBase::GetCurTask( int channel ) 
{
	if ( !m_ScheduleChannels.IsValidIndex( channel ) )
	{
		AssertMsg( 0, "Bad schedule channel" );
		return NULL;
	}

	AIChannelScheduleState_t *pScheduleState = &m_ScheduleChannels[channel];

	int iScheduleIndex = pScheduleState->iCurTask;
	if ( !pScheduleState->pSchedule || iScheduleIndex < 0 || iScheduleIndex >= pScheduleState->pSchedule->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &pScheduleState->pSchedule->GetTaskList()[ iScheduleIndex ];
}

//-------------------------------------

void CAI_BehaviorBase::StartTask( int channel, const Task_t *pTask )
{
	Assert( 0 );
}

//-------------------------------------

void CAI_BehaviorBase::RunTask( int channel, const Task_t *pTask )
{
	Assert( 0 );
}

//-------------------------------------

float CAI_BehaviorBase::GetJumpGravity() const
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BehaviorBridge_GetJumpGravity();
}

//-------------------------------------

bool CAI_BehaviorBase::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, float maxUp, float maxDown, float maxDist ) const
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BehaviorBridge_IsJumpLegal( startPos, apex, endPos, maxUp, maxDown, maxDist );
}

//-------------------------------------

bool CAI_BehaviorBase::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BehaviorBridge_MovementCost( moveType, vecStart, vecEnd, pCost );
}

//-------------------------------------

bool CAI_BehaviorBase::NotifyChangeBehaviorStatus( bool fCanFinishSchedule )
{
	bool fInterrupt = GetOuter()->OnBehaviorChangeStatus( this, fCanFinishSchedule );
	
	if ( !GetOuter()->IsInterruptable())
		return false;
		
	if ( fInterrupt )
	{
		if ( GetOuter()->m_hCine )
		{
			if( GetOuter()->m_hCine->PlayedSequence() )
			{
				DevWarning( "NPC: %s canceled running script %s due to behavior change\n", GetOuter()->GetDebugName(), GetOuter()->m_hCine->GetDebugName() );
			}
			else
			{
				DevWarning( "NPC: %s canceled script %s without playing, due to behavior change\n", GetOuter()->GetDebugName(), GetOuter()->m_hCine->GetDebugName() );
			}

			GetOuter()->m_hCine->CancelScript();
		}

		//!!!HACKHACK
		// this is dirty, but it forces NPC to pick a new schedule next time through.
		GetOuter()->ClearSchedule( "Changed behavior status" );
	}

	return fInterrupt;
}
