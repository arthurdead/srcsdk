//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_motor.h"
#include "ai_behavior_fear.h"
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BEHAVIOR_FEAR_SAFETY_TIME		5
#define FEAR_SAFE_PLACE_TOLERANCE		36.0f
#define FEAR_ENEMY_TOLERANCE_CLOSE_DIST_SQR		Square(300.0f) // (25 feet)
#define FEAR_ENEMY_TOLERANCE_TOO_CLOSE_DIST_SQR	Square( 60.0f ) // (5 Feet)

ConVar ai_enable_fear_behavior( "ai_enable_fear_behavior", "1" );

ConVar ai_fear_player_dist("ai_fear_player_dist", "720" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_FearBehavior::CAI_FearBehavior()
{
	m_SafePlaceMoveMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::Precache( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FEAR_IN_SAFE_PLACE:
		// We've arrived! Lock the hint and set the marker. we're safe for now.
		m_SafePlaceMoveMonitor.SetMark( GetOuter(), FEAR_SAFE_PLACE_TOLERANCE );
		TaskComplete();
		break;

	case TASK_FEAR_GET_PATH_TO_SAFETY_HINT:
		// Using TaskInterrupt() optimizations. See RunTask().
		break;

	case TASK_FEAR_WAIT_FOR_SAFETY:
		m_flTimeToSafety = gpGlobals->curtime + BEHAVIOR_FEAR_SAFETY_TIME;
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FEAR_WAIT_FOR_SAFETY:
		if( HasCondition(COND_SEE_ENEMY) )
		{
			m_flTimeToSafety = gpGlobals->curtime + BEHAVIOR_FEAR_SAFETY_TIME;
		}
		else
		{
			if( gpGlobals->curtime > m_flTimeToSafety )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_FEAR_GET_PATH_TO_SAFETY_HINT:
		{
			switch( GetOuter()->GetTaskInterrupt() )
			{
			case 0:// Find the hint node
				{
					TaskFail("Fear: Couldn't find hint node\n");
					m_flDeferUntil = gpGlobals->curtime + 3.0f;// Don't bang the hell out of this behavior. If we don't find a node, take a short break and run regular AI.
				}
				break;

			case 1:// Do the pathfinding.
				{
					TaskFail("Fear: Couldn't find hint node\n");
				}
				break;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TRUE if I have an enemy and that enemy would attack me if it could
// Notes  : Returns FALSE if the enemy is neutral or likes me.
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::EnemyDislikesMe()
{
	CBaseEntity *pEnemy = GetEnemy();

	if( pEnemy == NULL )
		return false;

	if( pEnemy->MyNPCPointer() == NULL )
		return false;

	Disposition_t disposition = pEnemy->MyNPCPointer()->IRelationType(GetOuter());

	Assert(disposition != D_ER);

	if( disposition >= D_LI )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// This place is definitely no longer safe. Stop picking it for a while.
//-----------------------------------------------------------------------------
void CAI_FearBehavior::MarkAsUnsafe()
{
}

//-----------------------------------------------------------------------------
// Am I in safe place from my enemy?
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::IsInASafePlace()
{
	// No safe place in mind.
	if( !m_SafePlaceMoveMonitor.IsMarkSet() )
		return false;

	// I have a safe place, but I'm not there.
	if( m_SafePlaceMoveMonitor.TargetMoved(GetOuter()) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::SpoilSafePlace()
{
	m_SafePlaceMoveMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
// Notes  : This behavior runs when I have an enemy that I fear, but who
//			does NOT hate or fear me (meaning they aren't going to fight me)
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::CanSelectSchedule()
{
	if( !GetOuter()->IsInterruptable() )
		return false;

	if( m_flDeferUntil > gpGlobals->curtime )
		return false;

	CBaseEntity *pEnemy = GetEnemy();

	if( pEnemy == NULL )
		return false;

	//if( !HasCondition(COND_SEE_PLAYER) )
	//	return false;

	if( !ai_enable_fear_behavior.GetBool() )
		return false;
	
	if( GetOuter()->IRelationType(pEnemy) != D_FR )
		return false;

	// Don't run fear behavior if we've been ordered somewhere
	if (GetOuter()->GetCommandGoal() != vec3_invalid)
		return false;

	// Don't run fear behavior if we're running any non-follow behaviors
	if (GetOuter()->GetPrimaryBehavior() && GetOuter()->GetPrimaryBehavior() != this && !FStrEq(GetOuter()->GetPrimaryBehavior()->GetName(), "Follow"))
		return false;

	if (m_hFearGoal && m_iszFearTarget != NULL_STRING)
	{
		if (pEnemy->NameMatches(m_iszFearTarget) || pEnemy->ClassMatches(m_iszFearTarget))
			return true;
	}

	if( !pEnemy->ClassMatches("npc_hunter") )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::GatherConditions()
{
	BaseClass::GatherConditions();

	ClearCondition( COND_FEAR_ENEMY_CLOSE );
	ClearCondition( COND_FEAR_ENEMY_TOO_CLOSE );
	if( GetEnemy() )
	{
		float flEnemyDistSqr = GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin());

		if( flEnemyDistSqr < FEAR_ENEMY_TOLERANCE_TOO_CLOSE_DIST_SQR )
		{
			SetCondition( COND_FEAR_ENEMY_TOO_CLOSE );
			if( IsInASafePlace() )
			{
				SpoilSafePlace();
			}
		}
		else if( flEnemyDistSqr < FEAR_ENEMY_TOLERANCE_CLOSE_DIST_SQR && GetEnemy()->GetEnemy() == GetOuter() )	
		{
			// Only become scared of an enemy at this range if they're my enemy, too
			SetCondition( COND_FEAR_ENEMY_CLOSE );
			if( IsInASafePlace() )
			{
				SpoilSafePlace();
			}
		}
	}

	ClearCondition(COND_FEAR_SEPARATED_FROM_PLAYER);

	// Check for separation from the player
	//	-The player is farther away than 60 feet
	//  -I haven't seen the player in 2 seconds
	//
	// Here's the distance check:
	CBasePlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin()); 

	if( pPlayer != NULL && GetAbsOrigin().DistToSqr(pPlayer->GetAbsOrigin()) >= Square( ai_fear_player_dist.GetFloat() * 1.5f )  )
	{
		SetCondition(COND_FEAR_SEPARATED_FROM_PLAYER);
	}

	// Here's the visibility check. We can't skip this because it's time-sensitive
	if( GetOuter()->FVisible(pPlayer) )
	{
		m_flTimePlayerLastVisible = gpGlobals->curtime;
	}
	else
	{
		if( gpGlobals->curtime - m_flTimePlayerLastVisible >= 2.0f )
		{
			SetCondition(COND_FEAR_SEPARATED_FROM_PLAYER);
		}
	}

	if( HasCondition(COND_FEAR_SEPARATED_FROM_PLAYER) )
	{
		//Msg("I am separated from player\n");

		if( IsInASafePlace() )
		{
			SpoilSafePlace();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::BeginScheduleSelection()
{
	m_flTimePlayerLastVisible = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::EndScheduleSelection()
{
	// We don't have to release our hints or markers or anything here. 
	// Just because we ran other AI for a while doesn't mean we aren't still in a safe place.
	//ReleaseAllHints();
}




//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
// Notes  : If fear behavior is running at all, we know we're afraid of our enemy
//-----------------------------------------------------------------------------
int CAI_FearBehavior::SelectSchedule()
{
	bool bInSafePlace = IsInASafePlace();

	if( !HasCondition(COND_HEAR_DANGER) )
	{
		if( !bInSafePlace )
		{
			// Always move to a safe place if we're not running from a danger sound
			return SCHED_FEAR_MOVE_TO_SAFE_PLACE;
		}
		else
		{
			// We ARE in a safe place
			if( HasCondition(COND_CAN_RANGE_ATTACK1) )
				return SCHED_RANGE_ATTACK1;

			return SCHED_FEAR_STAY_IN_SAFE_PLACE;
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if( GetOuter()->GetState() != NPC_STATE_SCRIPT )
	{
		// Stop doing ANYTHING if we get scared.
		//GetOuter()->SetCustomInterruptCondition( COND_HEAR_DANGER );

		if( !IsCurSchedule(SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY, false) && !IsCurSchedule(SCHED_FEAR_MOVE_TO_SAFE_PLACE, false) )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal(COND_FEAR_SEPARATED_FROM_PLAYER) );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_FearBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_FEAR_MOVE_TO_SAFE_PLACE:
		if( HasCondition(COND_FEAR_ENEMY_TOO_CLOSE) )
		{
			// If I'm moving to a safe place AND have an enemy too close to me,
			// make the move to safety while ignoring the condition.
			// this stops an oscillation
			// IS THIS CODE EVER EVEN BEING CALLED? (sjb)
			return SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
Activity CAI_FearBehavior::NPC_TranslateActivity( Activity activity )
{
	return BaseClass::NPC_TranslateActivity( activity );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::SetParameters( CAI_FearGoal *pGoal, string_t target )
{
	m_hFearGoal = pGoal;
	m_iszFearTarget = target;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//=============================================================================
//=============================================================================
// >AI_GOAL_FEAR
//=============================================================================
//=============================================================================
LINK_ENTITY_TO_CLASS( ai_goal_fear, CAI_FearGoal );

BEGIN_MAPENTITY( CAI_FearGoal, MAPENT_POINTCLASS )
	//DEFINE_KEYFIELD_AUTO( m_iSomething, "something" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

	// Outputs
	//DEFINE_OUTPUT( m_OnSeeFearEntity, "OnSeeFearEntity" ),
	DEFINE_OUTPUT( m_OnArriveAtFearNode, "OnArrivedAtNode" ),
END_MAPENTITY()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_FearBehavior *pBehavior;

	if ( !pAI->GetBehavior( &pBehavior ) )
	{
		return;
	}

	pBehavior->SetParameters(this, m_target);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearGoal::DisableGoal( CAI_BaseNPC *pAI )
{
	CAI_FearBehavior *pBehavior;

	if ( !pAI->GetBehavior( &pBehavior ) )
	{
		return;
	}

	pBehavior->SetParameters(NULL, NULL_STRING);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_FearGoal::InputActivate( inputdata_t &&inputdata )
{
	BaseClass::InputActivate( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_FearGoal::InputDeactivate( inputdata_t &&inputdata )
{
	BaseClass::InputDeactivate( inputdata );
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_FearBehavior )

	DECLARE_TASK( TASK_FEAR_GET_PATH_TO_SAFETY_HINT, TaskParamCheck_t() )
	DECLARE_TASK( TASK_FEAR_WAIT_FOR_SAFETY, TaskParamCheck_t() )
	DECLARE_TASK( TASK_FEAR_IN_SAFE_PLACE, TaskParamCheck_t() )

	DECLARE_CONDITION( COND_FEAR_ENEMY_CLOSE )
	DECLARE_CONDITION( COND_FEAR_ENEMY_TOO_CLOSE )
	DECLARE_CONDITION( COND_FEAR_SEPARATED_FROM_PLAYER )

	DEFINE_SCHEDULE_FILE( SCHED_FEAR_MOVE_TO_SAFE_PLACE );
	DEFINE_SCHEDULE_FILE( SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY);
	DEFINE_SCHEDULE_FILE( SCHED_FEAR_STAY_IN_SAFE_PLACE);


AI_END_CUSTOM_SCHEDULE_PROVIDER()
