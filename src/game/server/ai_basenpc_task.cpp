#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_tacticalservices.h"
#include "scripted.h"
#include "ai_memory.h"
#include "ai_moveprobe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ai_task_pre_script;

TaskFunc_t CAI_BaseNPC::s_DefaultTasks[LAST_SHARED_TASK][2]{
	#define AI_TASK_ENUM(name, params, ...) \
		{ &CAI_BaseNPC::START_##name, &CAI_BaseNPC::RUN_##name },

	#include "ai_default_task_enum.inc"
};

#define BASENPC_UNIMPLEMENTED_TASK(name) \
	void CAI_BaseNPC::START_##name( const Task_t *pTask ) \
	{ \
		Assert(0); \
		DebuggerBreak(); \
		TaskFail(FAIL_UNIMPLEMENTED); \
	} \
	void CAI_BaseNPC::RUN_##name( const Task_t *pTask ) \
	{ \
		Assert(0); \
		DebuggerBreak(); \
		TaskFail(FAIL_UNIMPLEMENTED); \
	}

#define BASENPC_START_TASK_ALIAS(name, alias) \
	void CAI_BaseNPC::START_##name( const Task_t *pTask ) \
	{ START_##alias(pTask); }
#define BASENPC_RUN_TASK_ALIAS(name, alias) \
	void CAI_BaseNPC::RUN_##name( const Task_t *pTask ) \
	{ RUN_##alias(pTask); }

#define BASENPC_START_TASK_NULL(name) \
	void CAI_BaseNPC::START_##name( const Task_t *pTask ) \
	{ \
	}
#define BASENPC_RUN_TASK_NULL(name) \
	void CAI_BaseNPC::RUN_##name( const Task_t *pTask ) \
	{ \
	}

void CAI_BaseNPC::START_TASK_RESET_ACTIVITY( const Task_t *pTask )
{
	m_Activity = ACT_RESET;

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_RESET_ACTIVITY)

void CAI_BaseNPC::START_TASK_CREATE_PENDING_WEAPON( const Task_t *pTask )
{
	Assert( m_iszPendingWeapon != NULL_STRING );

	GiveWeapon( m_iszPendingWeapon );
	m_iszPendingWeapon = NULL_STRING;

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_CREATE_PENDING_WEAPON)

void CAI_BaseNPC::START_TASK_RANDOMIZE_FRAMERATE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	float newRate = GetPlaybackRate();
	float percent = pTask->data[0].AsFloat() / 100.0f;

	newRate += ( newRate * random_valve->RandomFloat(-percent, percent) );

	SetPlaybackRate(newRate);

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_RANDOMIZE_FRAMERATE)

void CAI_BaseNPC::START_TASK_DEFER_DODGE( const Task_t *pTask )
{
	m_flNextDodgeTime = gpGlobals->curtime + pTask->data[0].AsFloat();
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_DEFER_DODGE)

void CAI_BaseNPC::START_TASK_ANNOUNCE_ATTACK( const Task_t *pTask )
{
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_ANNOUNCE_ATTACK)

void CAI_BaseNPC::START_TASK_TURN_RIGHT( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	StartTurn( -pTask->data[0].AsFloat() );
}
BASENPC_RUN_TASK_ALIAS(TASK_TURN_RIGHT, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_TURN_LEFT( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	StartTurn( pTask->data[0].AsFloat() );
}
BASENPC_RUN_TASK_ALIAS(TASK_TURN_LEFT, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_REMEMBER( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_MEMORY_ID );

	Remember( pTask->data[0].nMemoryId );

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_REMEMBER)

void CAI_BaseNPC::START_TASK_FORGET( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_MEMORY_ID );

	Forget( pTask->data[0].nMemoryId );

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_FORGET)

void CAI_BaseNPC::START_TASK_STORE_LASTPOSITION( const Task_t *pTask )
{
	m_vecLastPosition = GetLocalOrigin();

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_STORE_LASTPOSITION)

void CAI_BaseNPC::START_TASK_CLEAR_LASTPOSITION( const Task_t *pTask )
{
	m_vecLastPosition = vec3_origin;

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_CLEAR_LASTPOSITION)

void CAI_BaseNPC::START_TASK_STORE_POSITION_IN_SAVEPOSITION( const Task_t *pTask )
{
	m_vSavePosition = GetLocalOrigin();

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_STORE_POSITION_IN_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_STORE_BESTSOUND_IN_SAVEPOSITION( const Task_t *pTask )
{
	CSound *pBestSound = GetBestSound();
	if ( pBestSound )
	{
		m_vSavePosition = pBestSound->GetSoundOrigin();

		CBaseEntity *pSoundEnt = pBestSound->m_hOwner.Get();
		if ( pSoundEnt )
		{
			Vector vel;
			pSoundEnt->GetVelocity( &vel, NULL );

			// HACKHACK: run away from cars in the right direction
			m_vSavePosition += vel * 2;	// add in 2 seconds of velocity
		}

		TaskComplete();
	}
	else
	{
		TaskFail("No Sound!");
	}
}
BASENPC_RUN_TASK_NULL(TASK_STORE_BESTSOUND_IN_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION( const Task_t *pTask )
{
	CSound *pBestSound = GetBestSound();
	if ( pBestSound )
	{
		m_vSavePosition = pBestSound->GetSoundReactOrigin();

		TaskComplete();
	}
	else
	{
		TaskFail("No Sound!");
	}
}
BASENPC_RUN_TASK_NULL(TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION( const Task_t *pTask )
{
	if ( GetEnemy() != NULL )
	{
		m_vSavePosition = GetEnemy()->GetAbsOrigin();

		TaskComplete();
	}
	else
	{
		TaskFail(FAIL_NO_ENEMY);
	}
}
BASENPC_RUN_TASK_NULL(TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_STOP_MOVING( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeBool() );

	if ( GetNavType() == NAV_CLIMB )
	{
		// Don't clear the goal so that the climb can finish
		DbgNavMsg( this, "Start TASK_STOP_MOVING with climb workaround\n" );
	}
	else if ( ( GetNavigator()->IsGoalSet() && GetNavigator()->IsGoalActive() ) || GetNavType() == NAV_JUMP )
	{
		DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );

		if ( pTask->data[0].AsBool() )
		{
			DbgNavMsg( this, "Initiating stopping path\n" );
			GetNavigator()->StopMoving( false );
		}
		else
		{
			GetNavigator()->ClearGoal();
		}

		// E3 Hack
		if  ( HasPoseMoveYaw() ) 
		{
			SetPoseParameter( m_poseMove_Yaw, 0 );
		}
	}
	else
	{
		if ( pTask->data[0].AsBool() && GetNavigator()->SetGoalFromStoppingPath() )
		{
			DbgNavMsg( this, "Start TASK_STOP_MOVING\n" );
			DbgNavMsg( this, "Initiating stopping path\n" );
		}
		else
		{
			GetNavigator()->ClearGoal();
			SetIdealActivity( GetStoppedActivity() );
			TaskComplete();
		}
	}
}
void CAI_BaseNPC::RUN_TASK_STOP_MOVING( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeBool() );

	if ( pTask->data[0].AsBool() )
	{
		ChainRunTask( TASK_WAIT_FOR_MOVEMENT );
		if ( GetTaskStatus() == TASKSTATUS_COMPLETE )
		{
			DbgNavMsg( this, "TASK_STOP_MOVING Complete\n" );
		}
	}
	else
	{
		// if they're jumping, wait until they land
		if (GetNavType() == NAV_JUMP)
		{
			if (GetFlags() & FL_ONGROUND)
			{
				DbgNavMsg( this, "Jump landed\n" );
				SetNavType( NAV_GROUND ); // this assumes that NAV_JUMP only happens with npcs that use NAV_GROUND as base movement
			}
			else if (GetSmoothedVelocity().Length() > 0.01) // use an EPSILON damnit!!
			{
				// wait until you land
				return;
			}
			else
			{
				DbgNavMsg( this, "Jump stuck\n" );
				// stopped and stuck!
				SetNavType( NAV_GROUND );
				TaskFail( FAIL_STUCK_ONTOP );						
			}
		}

		// @TODO (toml 10-30-02): this is unacceptable, but needed until navigation can handle commencing
		// 						  a navigation while in the middle of a climb
		if (GetNavType() == NAV_CLIMB)
		{
			if (GetActivity() != ACT_CLIMB_DISMOUNT)
			{
				// Try to just pause the climb, but dismount if we're in SCHED_FAIL
				if (IsCurSchedule( SCHED_FAIL, false ))
				{
					GetMotor()->MoveClimbStop();
				}
				else
				{
					GetMotor()->MoveClimbPause();
				}

				TaskComplete();
			}
			else if (IsActivityFinished())
			{
				// Dismount complete.
				GetMotor()->MoveClimbStop();

				// Fix up our position if we have to
				Vector vecTeleportOrigin;
				if (GetMotor()->MoveClimbShouldTeleportToSequenceEnd( vecTeleportOrigin ))
				{
					SetLocalOrigin( vecTeleportOrigin );
				}

				TaskComplete();
			}
			// wait until you reach the end
			return;
		}

		DbgNavMsg( this, "TASK_STOP_MOVING Complete\n" );
		SetIdealActivity( GetStoppedActivity() );

		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_PLAY_SEQUENCE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeActivity() );

	SetIdealActivity( pTask->data[0].AsActivity() );
}
void CAI_BaseNPC::RUN_TASK_PLAY_SEQUENCE( const Task_t *pTask )
{
	AutoMovement( );

	if ( IsActivityFinished() )
	{
		TaskComplete();
	}
}

BASENPC_START_TASK_ALIAS(TASK_PLAY_PRIVATE_SEQUENCE, TASK_PLAY_SEQUENCE)
BASENPC_RUN_TASK_ALIAS(TASK_PLAY_PRIVATE_SEQUENCE, TASK_PLAY_SEQUENCE)

BASENPC_START_TASK_ALIAS(TASK_PLAY_SEQUENCE_FACE_ENEMY, TASK_PLAY_SEQUENCE)
void CAI_BaseNPC::RUN_TASK_PLAY_SEQUENCE_FACE_ENEMY( const Task_t *pTask )
{
	CBaseEntity *pTarget = GetEnemy();
	if ( pTarget )
	{
		GetMotor()->SetIdealYawAndUpdate( pTarget->GetAbsOrigin() - GetLocalOrigin() , AI_KEEP_YAW_SPEED );
	}

	if ( IsActivityFinished() )
	{
		TaskComplete();
	}
}

BASENPC_START_TASK_ALIAS(TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY, TASK_PLAY_PRIVATE_SEQUENCE)
BASENPC_RUN_TASK_ALIAS(TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY, TASK_PLAY_SEQUENCE_FACE_ENEMY)

BASENPC_START_TASK_ALIAS(TASK_PLAY_SEQUENCE_FACE_TARGET, TASK_PLAY_SEQUENCE)
void CAI_BaseNPC::RUN_TASK_PLAY_SEQUENCE_FACE_TARGET( const Task_t *pTask )
{
	CBaseEntity *pTarget = m_hTargetEnt.Get();
	if ( pTarget )
	{
		GetMotor()->SetIdealYawAndUpdate( pTarget->GetAbsOrigin() - GetLocalOrigin() , AI_KEEP_YAW_SPEED );
	}

	if ( IsActivityFinished() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_ADD_GESTURE_WAIT( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeActivity() );

	int iLayer = AddGesture( pTask->data[0].AsActivity() );
	if (iLayer > 0)
	{
		float flDuration = GetLayerDuration( iLayer );

		SetWait( flDuration );
	}
	else
	{
		TaskFail( "Unable to allocate gesture" );
	}
}
void CAI_BaseNPC::RUN_TASK_ADD_GESTURE_WAIT( const Task_t *pTask )
{
	if ( IsWaitFinished() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_ADD_GESTURE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeActivity() );

	AddGesture( pTask->data[0].AsActivity() );

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_ADD_GESTURE)

void CAI_BaseNPC::START_TASK_SET_SCHEDULE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_SCHEDULE_ID );

	if ( !SetSchedule( pTask->data[0].schedId ) )
		TaskFail(FAIL_SCHEDULE_NOT_FOUND);
}
BASENPC_RUN_TASK_NULL(TASK_SET_SCHEDULE)

void CAI_BaseNPC::START_TASK_FIND_BACKAWAY_FROM_SAVEPOSITION( const Task_t *pTask )
{
	if ( GetEnemy() != NULL )
	{
		Vector backPos;
		if ( !GetTacticalServices()->FindBackAwayPos( m_vSavePosition, &backPos ) )
		{
			// no place to backaway
			TaskFail(FAIL_NO_BACKAWAY_POSITION);
		}
		else 
		{
			if (GetNavigator()->SetGoal( AI_NavGoal_t( backPos, ACT_RUN ) ) )
			{
				TaskComplete();
			}
			else
			{
				// no place to backaway
				TaskFail(FAIL_NO_ROUTE);
			}
		}
	}
	else
	{
		TaskFail(FAIL_NO_ENEMY);
	}
}
BASENPC_RUN_TASK_NULL(TASK_FIND_BACKAWAY_FROM_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_FIND_COVER_FROM_ENEMY( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	if ( FindCoverFromEnemy( false, 0.0f, FLT_MAX ) )
	{
		m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[0].AsFloat();
		TaskComplete();
	}
	else
		TaskFail(FAIL_NO_COVER);
}
BASENPC_RUN_TASK_NULL(TASK_FIND_COVER_FROM_ENEMY)

void CAI_BaseNPC::START_TASK_FIND_COVER_FROM_ORIGIN( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	Vector coverPos;

	if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) ) 
	{
		AI_NavGoal_t goal(coverPos, ACT_RUN, AIN_HULL_TOLERANCE);
		GetNavigator()->SetGoal( goal );

		m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[0].AsFloat();
	}
	else
	{
		// no coverwhatsoever.
		TaskFail(FAIL_NO_COVER);
	}
}
BASENPC_RUN_TASK_NULL(TASK_FIND_COVER_FROM_ORIGIN)

void CAI_BaseNPC::START_TASK_FACE_LASTPOSITION( const Task_t *pTask )
{
	GetMotor()->SetIdealYawToTarget( m_vecLastPosition );
	GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
	SetTurnActivity(); 
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_LASTPOSITION, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_FACE_SAVEPOSITION( const Task_t *pTask )
{
	GetMotor()->SetIdealYawToTarget( m_vSavePosition );
	GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
	SetTurnActivity(); 
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_SAVEPOSITION, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_FACE_AWAY_FROM_SAVEPOSITION( const Task_t *pTask )
{
	GetMotor()->SetIdealYawToTarget( m_vSavePosition, 0, 180.0 );
	GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
	SetTurnActivity(); 
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_AWAY_FROM_SAVEPOSITION, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_SET_IDEAL_YAW_TO_CURRENT( const Task_t *pTask )
{
	GetMotor()->SetIdealYaw( UTIL_AngleMod( GetLocalAngles().y ) );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SET_IDEAL_YAW_TO_CURRENT)

void CAI_BaseNPC::START_TASK_FACE_TARGET( const Task_t *pTask )
{
	if ( m_hTargetEnt != NULL )
	{
		GetMotor()->SetIdealYawToTarget( m_hTargetEnt->GetAbsOrigin() );
		SetTurnActivity(); 
	}
	else
	{
		TaskFail(FAIL_NO_TARGET);
	}
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_TARGET, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_FACE_PLAYER( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	// track head to the client for a while.
	SetWait( pTask->data[0].AsFloat() );
}
void CAI_BaseNPC::RUN_TASK_FACE_PLAYER( const Task_t *pTask )
{
	// Get edict for one player
	CBasePlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this); 
	if ( pPlayer )
	{
		GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
		SetTurnActivity();
		if ( IsWaitFinished() && GetMotor()->DeltaIdealYaw() < 10 )
		{
			TaskComplete();
		}
	}
	else
	{
		TaskFail(FAIL_NO_PLAYER);
	}
}

void CAI_BaseNPC::START_TASK_FACE_INTERACTION_ANGLES( const Task_t *pTask )
{
	if ( !m_hForcedInteractionPartner )
	{
		TaskFail( FAIL_NO_TARGET );
		return;
	}

	// Get our running interaction from our partner,
	// as this should only run with the NPC "receiving" the interaction
	ScriptedNPCInteraction_t *pInteraction = m_hForcedInteractionPartner->GetRunningDynamicInteraction();

	if ( !(pInteraction->iFlags & SCNPC_FLAG_TEST_OTHER_ANGLES) )
	{
		TaskComplete();
		return;
	}

	// Get our target's origin
	Vector vecTarget = m_hForcedInteractionPartner->GetAbsOrigin();

	// Face the angles the interaction actually wants us at, opposite to the partner
	float angInteractionAngle = pInteraction->angRelativeAngles.y;
	angInteractionAngle += 180.0f;

	GetMotor()->SetIdealYaw( AngleNormalize( CalcIdealYaw( vecTarget ) + angInteractionAngle ) );

	if (FacingIdeal())
		TaskComplete();
	else
	{
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) );
		SetTurnActivity();
	}
}
void CAI_BaseNPC::RUN_TASK_FACE_INTERACTION_ANGLES( const Task_t *pTask )
{
	if ( !m_hForcedInteractionPartner )
	{
		TaskFail( FAIL_NO_TARGET );
		return;
	}

	// Get our running interaction from our partner,
	// as this should only run with the NPC "receiving" the interaction
	ScriptedNPCInteraction_t *pInteraction = m_hForcedInteractionPartner->GetRunningDynamicInteraction();

	// Get our target's origin
	Vector vecTarget = m_hForcedInteractionPartner->GetAbsOrigin();

	// Face the angles the interaction actually wants us at, opposite to the partner
	float angInteractionAngle = pInteraction->angRelativeAngles.y;
	angInteractionAngle += 180.0f;

	GetMotor()->SetIdealYawAndUpdate( CalcIdealYaw( vecTarget ) + angInteractionAngle, AI_KEEP_YAW_SPEED );

	if (IsWaitFinished())
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_FACE_ENEMY( const Task_t *pTask )
{
	Vector vecEnemyLKP = GetEnemyLKP();
	if (!FInAimCone( vecEnemyLKP ))
	{
		GetMotor()->SetIdealYawToTarget( vecEnemyLKP );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
		SetTurnActivity(); 
	}
	else
	{
		float flReasonableFacing = CalcReasonableFacing( true );
		if ( fabsf( flReasonableFacing - GetMotor()->GetIdealYaw() ) < 1 )
			TaskComplete();
		else
		{
			GetMotor()->SetIdealYaw( flReasonableFacing );
			SetTurnActivity();
		}
	}
}
void CAI_BaseNPC::RUN_TASK_FACE_ENEMY( const Task_t *pTask )
{
	// If the yaw is locked, this function will not act correctly
	Assert( GetMotor()->IsYawLocked() == false );

	Vector vecEnemyLKP = GetEnemyLKP();
	if (!FInAimCone( vecEnemyLKP ))
	{
		GetMotor()->SetIdealYawToTarget( vecEnemyLKP );
		GetMotor()->SetIdealYaw( CalcReasonableFacing( true ) ); // CalcReasonableFacing() is based on previously set ideal yaw
	}
	else
	{
		float flReasonableFacing = CalcReasonableFacing( true );
		if ( fabsf( flReasonableFacing - GetMotor()->GetIdealYaw() ) > 1 )
			GetMotor()->SetIdealYaw( flReasonableFacing );
	}

	GetMotor()->UpdateYaw();
	
	if ( FacingIdeal( m_flFaceEnemyTolerance ) )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_FACE_IDEAL( const Task_t *pTask )
{
	SetTurnActivity();
}
void CAI_BaseNPC::RUN_TASK_FACE_IDEAL( const Task_t *pTask )
{
	// If the yaw is locked, this function will not act correctly
	Assert( GetMotor()->IsYawLocked() == false );

	GetMotor()->UpdateYaw();

	if ( FacingIdeal() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_FACE_REASONABLE( const Task_t *pTask )
{
	GetMotor()->SetIdealYaw( CalcReasonableFacing() );
	SetTurnActivity();
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_REASONABLE, TASK_FACE_IDEAL)

void CAI_BaseNPC::START_TASK_FACE_PATH( const Task_t *pTask )
{
	if (!GetNavigator()->IsGoalActive())
	{
		DevWarning( 2, "No route to face!\n");
		TaskFail(FAIL_NO_ROUTE);
	}
	else
	{
		const float NPC_TRIVIAL_TURN = 15;	// (Degrees). Turns this small or smaller, don't bother with a transition.

		GetMotor()->SetIdealYawToTarget( GetNavigator()->GetCurWaypointPos());

		if( fabs( GetMotor()->DeltaIdealYaw() ) <= NPC_TRIVIAL_TURN )
		{
			// This character is already facing the path well enough that 
			// moving will look fairly natural. Don't bother with a transitional
			// turn animation.
			TaskComplete();
			return;
		}
		SetTurnActivity();
	}
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_PATH, TASK_FACE_IDEAL)

BASENPC_START_TASK_NULL(TASK_WAIT_PVS)
void CAI_BaseNPC::RUN_TASK_WAIT_PVS( const Task_t *pTask )
{
	if ( ShouldAlwaysThink() || 
		 UTIL_FindClientInPVS(edict()) || 
		 ( GetState() == NPC_STATE_COMBAT && GetEnemy() && gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() ) < 15 ) )
	{
		TaskComplete();
	}
}

BASENPC_START_TASK_NULL(TASK_WAIT_INDEFINITE)
BASENPC_RUN_TASK_NULL(TASK_WAIT_INDEFINITE)

void CAI_BaseNPC::START_TASK_WAIT( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	SetWait( pTask->data[0].AsFloat() );
}
void CAI_BaseNPC::RUN_TASK_WAIT( const Task_t *pTask )
{
	if ( IsWaitFinished() )
	{
		TaskComplete();
	}
}

BASENPC_START_TASK_ALIAS(TASK_WAIT_FACE_ENEMY, TASK_WAIT)
void CAI_BaseNPC::RUN_TASK_WAIT_FACE_ENEMY( const Task_t *pTask )
{
	Vector vecEnemyLKP = GetEnemyLKP();
	if (!FInAimCone( vecEnemyLKP ))
	{
		GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP , AI_KEEP_YAW_SPEED );
	}

	if ( IsWaitFinished() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_WAIT_RANDOM( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	SetWait( 0, pTask->data[0].AsFloat() );
}
BASENPC_RUN_TASK_ALIAS(TASK_WAIT_RANDOM, TASK_WAIT)

BASENPC_START_TASK_ALIAS(TASK_WAIT_FACE_ENEMY_RANDOM, TASK_WAIT_RANDOM)
BASENPC_RUN_TASK_ALIAS(TASK_WAIT_FACE_ENEMY_RANDOM, TASK_WAIT_FACE_ENEMY)

void CAI_BaseNPC::START_TASK_MOVE_TO_TARGET_RANGE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	// Identical tasks, except that target_range uses m_hTargetEnt, 
	// and Goal range uses the nav goal
	CBaseEntity *pTarget = m_hTargetEnt.Get();
	if ( pTarget == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if ( (pTarget->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
	{
		TaskComplete();
	}

	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
	}
	else
	{
		// set that we're probably going to stop before the goal
		GetNavigator()->SetArrivalDistance( pTask->data[0].AsFloat() );
	}
}
void CAI_BaseNPC::RUN_TASK_MOVE_TO_TARGET_RANGE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	// Identical tasks, except that target_range uses m_hTargetEnt, 
	// and Goal range uses the nav goal
	CBaseEntity *pTarget = m_hTargetEnt.Get();

	float distance;

	if ( pTarget == NULL )
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
	}
	else
	{
		bool bForceRun = false; 

		// Check Z first, and only check 2d if we're within that
		Vector vecGoalPos = GetNavigator()->GetGoalPos();
		distance = fabs(vecGoalPos.z - GetLocalOrigin().z);
		if ( distance < pTask->data[0].AsFloat() )
		{
			distance = ( vecGoalPos - GetLocalOrigin() ).Length2D();
		}
		else
		{
			// If the target is significantly higher or lower than me, I must run. 
			bForceRun = true;
		}

		// If we're jumping, wait until we're finished to update our goal position.
		if ( GetNavigator()->GetNavType() != NAV_JUMP )
		{
			// Re-evaluate when you think your finished, or the target has moved too far
			if ( (distance < pTask->data[0].AsFloat()) || (vecGoalPos - pTarget->GetAbsOrigin()).Length() > pTask->data[0].AsFloat() * 0.5 )
			{
				distance = ( pTarget->GetAbsOrigin() - GetLocalOrigin() ).Length2D();
				if ( !GetNavigator()->UpdateGoalPos( pTarget->GetAbsOrigin() ) )
				{
					TaskFail( FAIL_NO_ROUTE );
					return;
				}
			}
		}
		
		// Set the appropriate activity based on an overlapping range
		// overlap the range to prevent oscillation
		// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility
		if ( distance < pTask->data[0].AsFloat() )
		{
			TaskComplete();

// HL2 uses TASK_STOP_MOVING
			GetNavigator()->StopMoving();		// Stop moving
		}
		else
		{
			// Pick the right movement activity.
			Activity followActivity; 
				
			if( bForceRun )
			{
				followActivity = ACT_RUN;
			}
			else
			{
				followActivity = ( distance < 190 && m_NPCState != NPC_STATE_COMBAT ) ? ACT_WALK : ACT_RUN;
			}

			// Don't confuse move and shoot by resetting the activity every think
			Activity curActivity = GetNavigator()->GetMovementActivity();
			switch( curActivity )
			{
			case ACT_WALK_AIM:	curActivity = ACT_WALK;	break;
			case ACT_RUN_AIM:	curActivity = ACT_RUN;	break;
			}

			if ( curActivity != followActivity )
			{
				GetNavigator()->SetMovementActivity(followActivity);
			}
			GetNavigator()->SetArrivalDirection( pTarget );
		}
	}
}

void CAI_BaseNPC::START_TASK_MOVE_TO_GOAL_RANGE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	// Identical tasks, except that target_range uses m_hTargetEnt, 
	// and Goal range uses the nav goal
	CBaseEntity *pTarget = GetNavigator()->GetGoalTarget();
	if ( pTarget == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if ( (pTarget->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
	{
		TaskComplete();
	}

	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
	}
	else
	{
		// set that we're probably going to stop before the goal
		GetNavigator()->SetArrivalDistance( pTask->data[0].AsFloat() );
	}
}
void CAI_BaseNPC::RUN_TASK_MOVE_TO_GOAL_RANGE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	// Identical tasks, except that target_range uses m_hTargetEnt, 
	// and Goal range uses the nav goal
	CBaseEntity *pTarget = GetNavigator()->GetGoalTarget();

	float distance;

	if ( pTarget == NULL )
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
	}
	else
	{
		bool bForceRun = false; 

		// Check Z first, and only check 2d if we're within that
		Vector vecGoalPos = GetNavigator()->GetGoalPos();
		distance = fabs(vecGoalPos.z - GetLocalOrigin().z);
		if ( distance < pTask->data[0].AsFloat() )
		{
			distance = ( vecGoalPos - GetLocalOrigin() ).Length2D();
		}
		else
		{
			// If the target is significantly higher or lower than me, I must run. 
			bForceRun = true;
		}

		// If we're jumping, wait until we're finished to update our goal position.
		if ( GetNavigator()->GetNavType() != NAV_JUMP )
		{
			// Re-evaluate when you think your finished, or the target has moved too far
			if ( (distance < pTask->data[0].AsFloat()) || (vecGoalPos - pTarget->GetAbsOrigin()).Length() > pTask->data[0].AsFloat() * 0.5 )
			{
				distance = ( pTarget->GetAbsOrigin() - GetLocalOrigin() ).Length2D();
				if ( !GetNavigator()->UpdateGoalPos( pTarget->GetAbsOrigin() ) )
				{
					TaskFail( FAIL_NO_ROUTE );
					return;
				}
			}
		}
		
		// Set the appropriate activity based on an overlapping range
		// overlap the range to prevent oscillation
		// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility
		if ( distance < pTask->data[0].AsFloat() )
		{
			TaskComplete();

// HL2 uses TASK_STOP_MOVING
			GetNavigator()->StopMoving();		// Stop moving
		}
		else
		{
			// Pick the right movement activity.
			Activity followActivity; 
				
			if( bForceRun )
			{
				followActivity = ACT_RUN;
			}
			else
			{
				followActivity = ( distance < 190 && m_NPCState != NPC_STATE_COMBAT ) ? ACT_WALK : ACT_RUN;
			}

			// Don't confuse move and shoot by resetting the activity every think
			Activity curActivity = GetNavigator()->GetMovementActivity();
			switch( curActivity )
			{
			case ACT_WALK_AIM:	curActivity = ACT_WALK;	break;
			case ACT_RUN_AIM:	curActivity = ACT_RUN;	break;
			}

			if ( curActivity != followActivity )
			{
				GetNavigator()->SetMovementActivity(followActivity);
			}
			GetNavigator()->SetArrivalDirection( pTarget );
		}
	}
}

void CAI_BaseNPC::START_TASK_WAIT_UNTIL_NO_DANGER_SOUND( const Task_t *pTask )
{
	if( !HasCondition( COND_HEAR_DANGER ) )
	{
		TaskComplete();
	}
}
void CAI_BaseNPC::RUN_TASK_WAIT_UNTIL_NO_DANGER_SOUND( const Task_t *pTask )
{
	if( !HasCondition( COND_HEAR_DANGER ) )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_TARGET_PLAYER( const Task_t *pTask )
{
	CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );
	if ( pPlayer )
	{
		SetTarget( pPlayer );
		TaskComplete();
	}
	else
		TaskFail( FAIL_NO_PLAYER );
}
BASENPC_RUN_TASK_NULL(TASK_TARGET_PLAYER)

void CAI_BaseNPC::START_TASK_SCRIPT_RUN_TO_TARGET( const Task_t *pTask )
{
	StartScriptMoveToTargetTask( pTask->iTask );
}
void CAI_BaseNPC::RUN_TASK_SCRIPT_RUN_TO_TARGET( const Task_t *pTask )
{
	StartScriptMoveToTargetTask( pTask->iTask );
}

BASENPC_START_TASK_ALIAS(TASK_SCRIPT_WALK_TO_TARGET, TASK_SCRIPT_RUN_TO_TARGET)
BASENPC_RUN_TASK_ALIAS(TASK_SCRIPT_WALK_TO_TARGET, TASK_SCRIPT_RUN_TO_TARGET)

BASENPC_START_TASK_ALIAS(TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET, TASK_SCRIPT_RUN_TO_TARGET)
BASENPC_RUN_TASK_ALIAS(TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET, TASK_SCRIPT_RUN_TO_TARGET)

void CAI_BaseNPC::START_TASK_CLEAR_MOVE_WAIT( const Task_t *pTask )
{
	m_flMoveWaitFinished = gpGlobals->curtime;

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_CLEAR_MOVE_WAIT)

void CAI_BaseNPC::START_TASK_MELEE_ATTACK1( const Task_t *pTask )
{
	SetLastAttackTime( gpGlobals->curtime );
	ResetIdealActivity( ACT_MELEE_ATTACK1 );
}
void CAI_BaseNPC::RUN_TASK_MELEE_ATTACK1( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_MELEE_ATTACK2( const Task_t *pTask )
{
	SetLastAttackTime( gpGlobals->curtime );
	ResetIdealActivity( ACT_MELEE_ATTACK2 );
}
void CAI_BaseNPC::RUN_TASK_MELEE_ATTACK2( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_RANGE_ATTACK1( const Task_t *pTask )
{
	SetLastAttackTime( gpGlobals->curtime );
	ResetIdealActivity( ACT_RANGE_ATTACK1 );
}
void CAI_BaseNPC::RUN_TASK_RANGE_ATTACK1( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_RANGE_ATTACK2( const Task_t *pTask )
{
	SetLastAttackTime( gpGlobals->curtime );
	ResetIdealActivity( ACT_RANGE_ATTACK2 );
}
void CAI_BaseNPC::RUN_TASK_RANGE_ATTACK2( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_RELOAD( const Task_t *pTask )
{
	ResetIdealActivity( ACT_RELOAD );
}
void CAI_BaseNPC::RUN_TASK_RELOAD( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_SPECIAL_ATTACK1( const Task_t *pTask )
{
	ResetIdealActivity( ACT_SPECIAL_ATTACK1 );
}
void CAI_BaseNPC::RUN_TASK_SPECIAL_ATTACK1( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_SPECIAL_ATTACK2( const Task_t *pTask )
{
	ResetIdealActivity( ACT_SPECIAL_ATTACK2 );
}
void CAI_BaseNPC::RUN_TASK_SPECIAL_ATTACK2( const Task_t *pTask )
{
	RunAttackTask( pTask->iTask );
}

void CAI_BaseNPC::START_TASK_SET_ACTIVITY( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeActivity() );

	Activity goalActivity = pTask->data[0].AsActivity();
	if (goalActivity != ACT_RESET)
	{
		SetIdealActivity( goalActivity );
	}
	else
	{
		m_Activity = ACT_RESET;
	}
}
void CAI_BaseNPC::RUN_TASK_SET_ACTIVITY( const Task_t *pTask )
{
	if ( IsActivityStarted() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_GET_CHASE_PATH_TO_ENEMY( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	if ( ( pEnemy->GetAbsOrigin() - GetEnemyLKP() ).LengthSqr() < Square(pTask->data[0].AsFloat()) )
	{
		ChainStartTask( TASK_GET_PATH_TO_ENEMY );
	}
	else
	{
		ChainStartTask( TASK_GET_PATH_TO_ENEMY_LKP );
	}

	if ( !TaskIsComplete() && !HasCondition(COND_TASK_FAILED) )
		TaskFail(FAIL_NO_ROUTE);
}
BASENPC_RUN_TASK_NULL(TASK_GET_CHASE_PATH_TO_ENEMY)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_ENEMY_LKP( const Task_t *pTask )
{
	CBaseEntity *pEnemy = GetEnemy();
	if (!pEnemy || IsUnreachable(pEnemy))
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}
	AI_NavGoal_t goal( GetEnemyLKP() );

	TranslateNavGoal( pEnemy, goal.dest );

	if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
	{
		TaskComplete();
	}
	else
	{
		// no way to get there =(
		DevWarning( 2, "GetPathToEnemyLKP failed!!\n" );
		RememberUnreachable(GetEnemy());
		TaskFail(FAIL_NO_ROUTE);
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_ENEMY_LKP)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_INTERACTION_PARTNER( const Task_t *pTask )
{
	if ( !m_hForcedInteractionPartner || IsUnreachable(m_hForcedInteractionPartner.Get()) )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	// Calculate the position we need to be at to start the interaction.
	CalculateForcedInteractionPosition();

	AI_NavGoal_t goal( m_vecForcedWorldPosition );
	TranslateNavGoal( m_hForcedInteractionPartner.Get(), goal.dest );

	if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
	{
		TaskComplete();
	}
	else
	{
		DevWarning( 2, "GetPathToInteractionPartner failed!!\n" );
		RememberUnreachable(m_hForcedInteractionPartner.Get());
		TaskFail(FAIL_NO_ROUTE);
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_INTERACTION_PARTNER)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	if ( GetEnemy() )
	{
		// Find out which range to use (either innately or a held weapon)
		float flRange = -1.0f;
		if ( CapabilitiesGet() & (bits_CAP_INNATE_RANGE_ATTACK1|bits_CAP_INNATE_RANGE_ATTACK2) )
		{
			flRange = InnateRange1MaxRange();
		}
		else if ( GetActiveWeapon() )
		{
			flRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
		}
		else
		{
			// You can't call this task without either innate range attacks or a weapon!
			Assert( 0 );
			TaskFail( FAIL_NO_ROUTE );
		}

		// Clamp to the specified range, if supplied
		if ( pTask->data[0].AsFloat() != 0 && pTask->data[0].AsFloat() < flRange )
			flRange = pTask->data[0].AsFloat();

		// For now, just try running straight at enemy
		float dist = EnemyDistance( GetEnemy() );
		if ( dist <= flRange || GetNavigator()->SetVectorGoalFromTarget( GetEnemy()->GetAbsOrigin(), dist - flRange ) )
		{
			TaskComplete();
			return;
		}
	}

	TaskFail( FAIL_NO_ROUTE );
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_ENEMY_LKP_LOS( const Task_t *pTask )
{
	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}

	AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
	float flMaxRange = 2000;
	float flMinRange = 0;
	
	if ( GetActiveWeapon() )
	{
		flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
		flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
	}
	else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		flMaxRange = InnateRange1MaxRange();
		flMinRange = InnateRange1MinRange();
	}

	//Check against NPC's max range
	if ( flMaxRange > m_flDistTooFar )
	{
		flMaxRange = m_flDistTooFar;
	}

	Vector vecEnemy 	= GetEnemyLKP();
	Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

	Vector posLos;
	bool found = false;

	if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
	{
		float dist = ( posLos - vecEnemyEye ).Length();
		if ( dist < flMaxRange && dist > flMinRange )
			found = true;
	}
	
	if ( !found )
	{
		FlankType_t eFlankType = FLANKTYPE_NONE;
		Vector vecFlankRefPos = vec3_origin;
		float flFlankParam = 0;
	
		if ( GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, eFlankType, vecFlankRefPos, flFlankParam, &posLos ) )
		{
			found = true;
		}
	}

	if ( !found )
	{
		TaskFail( FAIL_NO_SHOOT );
	}
	else
	{
		// else drop into run task to offer an interrupt
		m_vInterruptSavePosition = posLos;
	}
}
void CAI_BaseNPC::RUN_TASK_GET_PATH_TO_ENEMY_LKP_LOS( const Task_t *pTask )
{
	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}
	if ( GetTaskInterrupt() > 0 )
	{
		ClearTaskInterrupt();

		Vector vecEnemy = GetEnemyLKP();
		AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

		GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
		GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
	}
	else
		TaskInterrupt();
}

void CAI_BaseNPC::START_TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}

	AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
	float flMaxRange = 2000;
	float flMinRange = 0;
	
	if ( GetActiveWeapon() )
	{
		flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
		flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
	}
	else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		flMaxRange = InnateRange1MaxRange();
		flMinRange = InnateRange1MinRange();
	}

	//Check against NPC's max range
	if ( flMaxRange > m_flDistTooFar )
	{
		flMaxRange = m_flDistTooFar;
	}

	Vector vecEnemy 	= GetEnemy()->GetAbsOrigin();
	Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

	Vector posLos;
	bool found = false;

	if ( !found )
	{
		FlankType_t eFlankType = FLANKTYPE_NONE;
		Vector vecFlankRefPos = vec3_origin;
		float flFlankParam = 0;

		eFlankType = FLANKTYPE_RADIUS;
		vecFlankRefPos = m_vSavePosition;
		flFlankParam = pTask->data[0].AsFloat();

		if ( GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, eFlankType, vecFlankRefPos, flFlankParam, &posLos ) )
		{
			found = true;
		}
	}

	if ( !found )
	{
		TaskFail( FAIL_NO_SHOOT );
	}
	else
	{
		// else drop into run task to offer an interrupt
		m_vInterruptSavePosition = posLos;
	}
}
BASENPC_RUN_TASK_ALIAS(TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS, TASK_GET_PATH_TO_ENEMY_LOS)

void CAI_BaseNPC::START_TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}

	AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
	float flMaxRange = 2000;
	float flMinRange = 0;
	
	if ( GetActiveWeapon() )
	{
		flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
		flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
	}
	else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		flMaxRange = InnateRange1MaxRange();
		flMinRange = InnateRange1MinRange();
	}

	//Check against NPC's max range
	if ( flMaxRange > m_flDistTooFar )
	{
		flMaxRange = m_flDistTooFar;
	}

	Vector vecEnemy 	= GetEnemy()->GetAbsOrigin();
	Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

	Vector posLos;
	bool found = false;

	if ( !found )
	{
		FlankType_t eFlankType = FLANKTYPE_NONE;
		Vector vecFlankRefPos = vec3_origin;
		float flFlankParam = 0;

		eFlankType = FLANKTYPE_ARC;
		vecFlankRefPos = m_vSavePosition;
		flFlankParam = pTask->data[0].AsFloat();

		if ( GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, eFlankType, vecFlankRefPos, flFlankParam, &posLos ) )
		{
			found = true;
		}
	}

	if ( !found )
	{
		TaskFail( FAIL_NO_SHOOT );
	}
	else
	{
		// else drop into run task to offer an interrupt
		m_vInterruptSavePosition = posLos;
	}
}
BASENPC_RUN_TASK_ALIAS(TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS, TASK_GET_PATH_TO_ENEMY_LOS)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_ENEMY_LOS( const Task_t *pTask )
{
	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}

	AI_PROFILE_SCOPE(CAI_BaseNPC_FindLosToEnemy);
	float flMaxRange = 2000;
	float flMinRange = 0;
	
	if ( GetActiveWeapon() )
	{
		flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
		flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
	}
	else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		flMaxRange = InnateRange1MaxRange();
		flMinRange = InnateRange1MinRange();
	}

	//Check against NPC's max range
	if ( flMaxRange > m_flDistTooFar )
	{
		flMaxRange = m_flDistTooFar;
	}

	Vector vecEnemy 	= GetEnemy()->GetAbsOrigin();
	Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

	Vector posLos;
	bool found = false;

	if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
	{
		float dist = ( posLos - vecEnemyEye ).Length();
		if ( dist < flMaxRange && dist > flMinRange )
			found = true;
	}
	
	if ( !found )
	{
		FlankType_t eFlankType = FLANKTYPE_NONE;
		Vector vecFlankRefPos = vec3_origin;
		float flFlankParam = 0;

		if ( GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, eFlankType, vecFlankRefPos, flFlankParam, &posLos ) )
		{
			found = true;
		}
	}

	if ( !found )
	{
		TaskFail( FAIL_NO_SHOOT );
	}
	else
	{
		// else drop into run task to offer an interrupt
		m_vInterruptSavePosition = posLos;
	}
}
void CAI_BaseNPC::RUN_TASK_GET_PATH_TO_ENEMY_LOS( const Task_t *pTask )
{
	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}
	if ( GetTaskInterrupt() > 0 )
	{
		ClearTaskInterrupt();

		Vector vecEnemy = GetEnemy()->GetAbsOrigin();
		AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

		GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
		GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
	}
	else
		TaskInterrupt();
}

void CAI_BaseNPC::START_TASK_SET_GOAL( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_GOAL_TYPE );

	switch ( pTask->data[0].goalType )
	{
	case GOAL_ENEMY:	//Enemy
		
		if ( GetEnemy() == NULL )
		{
			TaskFail( FAIL_NO_ENEMY );
			return;
		}
		
		//Setup our stored info
		m_vecStoredPathGoal = GetEnemy()->GetAbsOrigin();
		m_nStoredPathType	= GOALTYPE_ENEMY;
		m_fStoredPathFlags	= 0;
		m_hStoredPathTarget	= GetEnemy();
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;
	
	case GOAL_ENEMY_LKP:		//Enemy's last known position

		if ( GetEnemy() == NULL )
		{
			TaskFail( FAIL_NO_ENEMY );
			return;
		}
		
		//Setup our stored info
		m_vecStoredPathGoal = GetEnemyLKP();
		m_nStoredPathType	= GOALTYPE_LOCATION;
		m_fStoredPathFlags	= 0;
		m_hStoredPathTarget	= NULL;
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;
	
	case GOAL_TARGET:			//Target entity
		
		if ( m_hTargetEnt == NULL )
		{
			TaskFail( FAIL_NO_TARGET );
			return;
		}
		
		//Setup our stored info
		m_vecStoredPathGoal = m_hTargetEnt->GetAbsOrigin();
		m_nStoredPathType	= GOALTYPE_TARGETENT;
		m_fStoredPathFlags	= 0;
		m_hStoredPathTarget	= m_hTargetEnt;
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;

	case GOAL_SAVED_POSITION:	//Saved position
		
		//Setup our stored info
		m_vecStoredPathGoal = m_vSavePosition;
		m_nStoredPathType	= GOALTYPE_LOCATION;
		m_fStoredPathFlags	= 0;
		m_hStoredPathTarget	= NULL;
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;
	}

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SET_GOAL)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_GOAL( const Task_t *pTask )
{
	Assert( pTask->numData == 2 );
	Assert( pTask->data[0].nType == TASK_DATA_PATH_TYPE );
	Assert( pTask->data[1].CanBeFloat() );

	AI_NavGoal_t goal( m_nStoredPathType, 
					   AIN_DEF_ACTIVITY, 
					   AIN_HULL_TOLERANCE,
					   AIN_DEF_FLAGS,
					   m_hStoredPathTarget.Get() );
	
	bool	foundPath = false;

	//Find our path
	switch ( pTask->data[0].pathType )
	{
	case PATH_TRAVEL:	//A land path to our goal
		goal.dest = m_vecStoredPathGoal;
		foundPath = GetNavigator()->SetGoal( goal );
		break;

	case PATH_LOS:		//A path to get LOS to our goal
		{
			float flMaxRange = 2000.0f;
			float flMinRange = 0.0f;

			if ( GetActiveWeapon() )
			{
				flMaxRange = MAX( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
				flMinRange = MIN( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
			}
			else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
			{
				flMaxRange = InnateRange1MaxRange();
				flMinRange = InnateRange1MinRange();
			}

			// Check against NPC's max range
			if ( flMaxRange > m_flDistTooFar )
			{
				flMaxRange = m_flDistTooFar;
			}

			Vector	eyePosition = ( m_hStoredPathTarget != NULL ) ? m_hStoredPathTarget->EyePosition() : m_vecStoredPathGoal;

			Vector posLos;

			// See if we've found it
			if ( GetTacticalServices()->FindLos( m_vecStoredPathGoal, eyePosition, flMinRange, flMaxRange, 1.0f, &posLos ) )
			{
				goal.dest = posLos;
				foundPath = GetNavigator()->SetGoal( goal );
			}
			else
			{
				// No LOS to goal
				TaskFail( FAIL_NO_SHOOT );
				return;
			}
		}
		
		break;

	case PATH_COVER:	//Get a path to cover FROM our goal
		{
			CBaseEntity *pEntity = ( m_hStoredPathTarget == NULL ) ? this : m_hStoredPathTarget.Get();

			//Find later cover first
			Vector coverPos;

			if ( GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), 0, &coverPos ) )
			{
				AI_NavGoal_t goal( coverPos, ACT_RUN );
				GetNavigator()->SetGoal( goal, AIN_CLEAR_PREVIOUS_STATE );
				
					//FIXME: What exactly is this doing internally?
				m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[1].AsFloat();
				TaskComplete();
				return;
			}
			else
			{
				//Try any cover
				if ( GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), &coverPos ) ) 
				{
					//If we've found it, find a safe route there
					AI_NavGoal_t coverGoal( GOALTYPE_COVER, 
											coverPos,
											ACT_RUN,
											AIN_HULL_TOLERANCE,
											AIN_DEF_FLAGS,
											m_hStoredPathTarget.Get() );
					
					foundPath = GetNavigator()->SetGoal( goal );

					m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[1].AsFloat();
				}
				else
				{
					TaskFail( FAIL_NO_COVER );
				}
			}
		}

		break;
	}

	//Now validate our try
	if ( foundPath )
	{
		TaskComplete();
	}
	else
	{
		TaskFail( FAIL_NO_ROUTE );
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_GOAL)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_ENEMY( const Task_t *pTask )
{
	if (IsUnreachable(GetEnemy()))
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}
				
	if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
	{
		TaskComplete();
	}
	else
	{
		// no way to get there =( 
		DevWarning( 2, "GetPathToEnemy failed!!\n" );
		RememberUnreachable(GetEnemy());
		TaskFail(FAIL_NO_ROUTE);
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_ENEMY)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_ENEMY_CORPSE( const Task_t *pTask )
{
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	Vector vecEnemyLKP = GetEnemyLKP();

	GetNavigator()->SetGoal( vecEnemyLKP - forward * 64, AIN_CLEAR_TARGET);
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_ENEMY_CORPSE)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_PLAYER( const Task_t *pTask )
{
	CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player" );

	AI_NavGoal_t goal;

	goal.type = GOALTYPE_LOCATION;
	goal.dest = pPlayer->WorldSpaceCenter();
	goal.pTarget = pPlayer;

	GetNavigator()->SetGoal( goal );
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_PLAYER)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_SAVEPOSITION_LOS( const Task_t *pTask )
{
	if ( GetEnemy() == NULL )
	{
		TaskFail(FAIL_NO_ENEMY);
		return;
	}

	float flMaxRange = 2000;
	float flMinRange = 0;
	if ( GetActiveWeapon() )
	{
		flMaxRange = MAX(GetActiveWeapon()->m_fMaxRange1,GetActiveWeapon()->m_fMaxRange2);
		flMinRange = MIN(GetActiveWeapon()->m_fMinRange1,GetActiveWeapon()->m_fMinRange2);
	}
	else if ( CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		flMaxRange = InnateRange1MaxRange();
		flMinRange = InnateRange1MinRange();
	}

	// Check against NPC's max range
	if (flMaxRange > m_flDistTooFar)
	{
		flMaxRange = m_flDistTooFar;
	}

	Vector posLos;

	if (GetTacticalServices()->FindLos(m_vSavePosition,m_vSavePosition, flMinRange, flMaxRange, 1.0, &posLos))
	{
		GetNavigator()->SetGoal( AI_NavGoal_t( posLos, ACT_RUN, AIN_HULL_TOLERANCE ) );
	}
	else
	{
		// no coverwhatsoever.
		TaskFail(FAIL_NO_SHOOT);
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_SAVEPOSITION_LOS)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_TARGET_WEAPON( const Task_t *pTask )
{
	// Finds the nearest node within the leniency distances,
	// whether the node can see the target or not.
	const float XY_LENIENCY = 64.0;
	const float Z_LENIENCY	= 72.0;

	if (m_hTargetEnt == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else 
	{
		CRecastMesh *pMesh = GetNavMesh();
		if( !pMesh )
		{
			TaskFail( FAIL_NO_ROUTE );
			return;
		}

		// Since this weapon MAY be on a table, we find the nearest node without verifying
		// line-of-sight, since weapons on the table will not be able to see nodes very nearby.

		bool bHasPath = true;
		Vector vecNodePos;

		vecNodePos = pMesh->ClosestPointOnMesh( m_hTargetEnt->GetAbsOrigin() );
		if( vecNodePos == vec3_origin )
		{
			TaskFail( FAIL_NO_ROUTE );
			return;
		}

		float flDistZ;
		flDistZ = fabs( vecNodePos.z - m_hTargetEnt->GetAbsOrigin().z );
		if( flDistZ > Z_LENIENCY )
		{
			// The gun is too far away from its nearest node on the Z axis.
			TaskFail( "Target not within Z_LENIENCY!\n");
			CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
			if( pWeapon )
			{
				// Lock this weapon for a long time so no one else tries to get it.
				pWeapon->Lock( 30.0f, pWeapon );
				return;
			}
		}

		if( flDistZ >= 16.0 )
		{
			// The gun is higher or lower, but it's within reach. (probably on a table).
			float flDistXY = ( vecNodePos - m_hTargetEnt->GetAbsOrigin() ).Length2D();

			// This means we have to stand on the nearest node and still be able to
			// reach the gun.
			if( flDistXY > XY_LENIENCY )
			{
				TaskFail( "Target not within XY_LENIENCY!\n" );
				CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
				if( pWeapon )
				{
					// Lock this weapon for a long time so no one else tries to get it.
					pWeapon->Lock( 30.0f, pWeapon );
					return;
				}
			}

			AI_NavGoal_t goal( vecNodePos );
			goal.pTarget = m_hTargetEnt.Get();
			bHasPath = GetNavigator()->SetGoal( goal );
		}
		else
		{
			// The gun is likely just lying on the floor. Just pick it up.
			AI_NavGoal_t goal( m_hTargetEnt->GetAbsOrigin() );
			goal.pTarget = m_hTargetEnt.Get();
			bHasPath = GetNavigator()->SetGoal( goal );
		}

		if( !bHasPath )
		{
			CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( m_hTargetEnt.Get() );
			if( pWeapon )
			{
				// Lock this weapon for a long time so no one else tries to get it.
				pWeapon->Lock( 15.0f, pWeapon );
			}
		}
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_TARGET_WEAPON)

void CAI_BaseNPC::START_TASK_GET_PATH_OFF_OF_NPC( const Task_t *pTask )
{
	Assert( ( GetGroundEntity() && ( GetGroundEntity()->IsPlayer() || ( GetGroundEntity()->IsNPC() && IRelationType( GetGroundEntity() ) == D_LI ) ) ) );
	GetNavigator()->SetAllowBigStep( GetGroundEntity() );
	ChainStartTask( TASK_MOVE_AWAY_PATH, 48 );
}
void CAI_BaseNPC::RUN_TASK_GET_PATH_OFF_OF_NPC( const Task_t *pTask )
{
	//TODO!!! Arthurdead
	//GetNavigator()->SetAllowBigStep( UTIL_GetLocalPlayer() );
	ChainRunTask( TASK_MOVE_AWAY_PATH, 48 );
}

void CAI_BaseNPC::START_TASK_GET_PATH_TO_TARGET( const Task_t *pTask )
{
	if (m_hTargetEnt == NULL)
	{
		TaskFail(FAIL_NO_TARGET);
	}
	else 
	{
		AI_NavGoal_t goal( static_cast<const Vector&>(m_hTargetEnt->EyePosition()) );
		goal.pTarget = m_hTargetEnt.Get();
		GetNavigator()->SetGoal( goal );
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_TARGET)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_COMMAND_GOAL( const Task_t *pTask )
{
	if (!GetNavigator()->SetGoal( m_vecCommandGoal ))
	{
		OnMoveToCommandGoalFailed();
		TaskFail(FAIL_NO_ROUTE);
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_COMMAND_GOAL)

void CAI_BaseNPC::START_TASK_MARK_COMMAND_GOAL_POS( const Task_t *pTask )
{
	// Start watching my position to detect whether another AI process has moved me from my mark.
	m_CommandMoveMonitor.SetMark( this, COMMAND_GOAL_TOLERANCE );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_MARK_COMMAND_GOAL_POS)

void CAI_BaseNPC::START_TASK_CLEAR_COMMAND_GOAL( const Task_t *pTask )
{
	m_vecCommandGoal = vec3_invalid;
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_CLEAR_COMMAND_GOAL)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_LASTPOSITION( const Task_t *pTask )
{
	if (!GetNavigator()->SetGoal( m_vecLastPosition ))
	{
		TaskFail(FAIL_NO_ROUTE);
	}
	else
	{
		GetNavigator()->SetGoalTolerance( 48 );
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_LASTPOSITION)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_SAVEPOSITION( const Task_t *pTask )
{
	GetNavigator()->SetGoal( m_vSavePosition );
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_SAVEPOSITION)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_BESTSOUND( const Task_t *pTask )
{
	CSound *pSound = GetBestSound();
	if (!pSound)
	{
		TaskFail(FAIL_NO_SOUND);
	}
	else
	{
		GetNavigator()->SetGoal( pSound->GetSoundReactOrigin() );
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_BESTSOUND)

void CAI_BaseNPC::START_TASK_GET_PATH_TO_BESTSCENT( const Task_t *pTask )
{
	CSound *pScent = GetBestScent();
	if (!pScent) 
	{
		TaskFail(FAIL_NO_SCENT);
	}
	else
	{
		GetNavigator()->SetGoal( pScent->GetSoundOrigin() );
	}
}
BASENPC_RUN_TASK_NULL(TASK_GET_PATH_TO_BESTSCENT)

void CAI_BaseNPC::START_TASK_GET_PATH_AWAY_FROM_BEST_SOUND( const Task_t *pTask )
{
	CSound *pBestSound = GetBestSound();
	if ( !pBestSound )
	{
		TaskFail("No Sound!");
		return;
	}

	GetMotor()->SetIdealYawToTarget( pBestSound->GetSoundOrigin() );
	ChainStartTask( TASK_MOVE_AWAY_PATH, pTask->data, pTask->numData );
	LockBestSound();
}
void CAI_BaseNPC::RUN_TASK_GET_PATH_AWAY_FROM_BEST_SOUND( const Task_t *pTask )
{
	ChainRunTask( TASK_MOVE_AWAY_PATH, pTask->data, pTask->numData );
	if ( GetNavigator()->IsGoalActive() )
	{
		Vector vecDest = GetNavigator()->GetGoalPos();
		float flDist = ( GetAbsOrigin() - vecDest ).Length();

		if( flDist < 10.0 * 12.0 )
		{
			TaskFail("Path away from best sound too short!\n");
		}
	}
}

void CAI_BaseNPC::START_TASK_MOVE_AWAY_PATH( const Task_t *pTask )
{
	// Drop into run task to support interrupt
	DesireStand();
}
void CAI_BaseNPC::RUN_TASK_MOVE_AWAY_PATH( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	QAngle ang = GetLocalAngles();
	ang.y = GetMotor()->GetIdealYaw() + 180;
	Vector move;

	switch ( GetTaskInterrupt() )
	{
	case 0:
		{
			// See if we're moving away from a vehicle
			CSound *pBestSound = GetBestSound( SOUND_MOVE_AWAY );
			if ( pBestSound && pBestSound->m_hOwner && pBestSound->m_hOwner->GetServerVehicle() )
			{
				// Move away from the vehicle's center, regardless of our facing
				move = ( GetAbsOrigin() - pBestSound->m_hOwner->WorldSpaceCenter() );
				VectorNormalize( move );
			}
			else
			{
				// Use the first angles
				AngleVectors( ang, &move );
			}

			if ( GetNavigator()->SetVectorGoal( move, pTask->data[0].AsFloat(), MIN(36,pTask->data[0].AsFloat()), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ))
			{
				TaskComplete();
			}
			else
			{
				ang.y = GetMotor()->GetIdealYaw() + 91;
				AngleVectors( ang, &move );

				if ( GetNavigator()->SetVectorGoal( move, pTask->data[0].AsFloat(), MIN(24,pTask->data[0].AsFloat()), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
				{
					TaskComplete();
				}
				else
				{
					TaskInterrupt();
				}
			}
		}
		break;

	case 1:
		{
			ang.y = GetMotor()->GetIdealYaw() + 271;
			AngleVectors( ang, &move );

			if ( GetNavigator()->SetVectorGoal( move, pTask->data[0].AsFloat(), MIN(24,pTask->data[0].AsFloat()), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
			{
				TaskComplete();
			}
			else
			{
				ang.y = GetMotor()->GetIdealYaw() + 180;
				while (ang.y < 0)
					ang.y += 360;
				while (ang.y >= 360)
					ang.y -= 360;
				if ( ang.y < 45 || ang.y >= 315 )
					ang.y = 0;
				else if ( ang.y < 135 )
					ang.y = 90;
				else if ( ang.y < 225 )
					ang.y = 180;
				else
					ang.y = 270;

				AngleVectors( ang, &move );

				if ( GetNavigator()->SetVectorGoal( move, pTask->data[0].AsFloat(), MIN(6,pTask->data[0].AsFloat()), false ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
				{
					TaskComplete();
				}
				else
				{
					TaskInterrupt();
				}
			}
		}
		break;

	case 2:
		{
			ClearTaskInterrupt();
			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) ) 
			{
				GetNavigator()->SetGoal( AI_NavGoal_t( coverPos, ACT_RUN ) );
				m_flMoveWaitFinished = gpGlobals->curtime + 2;
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;
	}
}

void CAI_BaseNPC::START_TASK_WEAPON_RUN_PATH( const Task_t *pTask )
{
	GetNavigator()->SetMovementActivity(ACT_RUN);
}
void CAI_BaseNPC::RUN_TASK_WEAPON_RUN_PATH( const Task_t *pTask )
{
	CBaseEntity *pTarget = m_hTargetEnt.Get();
	if ( pTarget )
	{
		if ( pTarget->GetOwnerEntity() )
		{
			TaskFail(FAIL_WEAPON_OWNED);
		}
		else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
		{
			TaskComplete();
		}
	}
	else
	{
		TaskFail(FAIL_ITEM_NO_FIND);
	}
}

BASENPC_START_TASK_ALIAS(TASK_ITEM_RUN_PATH, TASK_WEAPON_RUN_PATH)
BASENPC_RUN_TASK_ALIAS(TASK_ITEM_RUN_PATH, TASK_WEAPON_RUN_PATH)

void CAI_BaseNPC::START_TASK_RUN_PATH( const Task_t *pTask )
{
	// UNDONE: This is in some default AI and some NPCs can't run? -- walk instead?
	if ( TranslateActivity( ACT_RUN ) != ACT_INVALID )
	{
		GetNavigator()->SetMovementActivity( ACT_RUN );
	}
	else
	{
		GetNavigator()->SetMovementActivity(ACT_WALK);
	}

	// Cover is void once I move
	Forget( bits_MEMORY_INCOVER );

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_RUN_PATH)

void CAI_BaseNPC::START_TASK_WALK_PATH_FOR_UNITS( const Task_t *pTask )
{
	GetNavigator()->SetMovementActivity(ACT_WALK);
}
BASENPC_RUN_TASK_ALIAS(TASK_WALK_PATH_FOR_UNITS, TASK_RUN_PATH_FOR_UNITS)

void CAI_BaseNPC::START_TASK_RUN_PATH_FOR_UNITS( const Task_t *pTask )
{
	GetNavigator()->SetMovementActivity(ACT_RUN);
}
void CAI_BaseNPC::RUN_TASK_RUN_PATH_FOR_UNITS( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	float distance;
	distance = (m_vecLastPosition - GetLocalOrigin()).Length2D();

	// Walk path until far enough away
	if ( distance > pTask->data[0].AsFloat() || 
		 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_WALK_PATH( const Task_t *pTask )
{
	bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
	if ( bIsFlying && ( TranslateActivity( ACT_FLY ) != ACT_INVALID) )
	{
		GetNavigator()->SetMovementActivity(ACT_FLY);
	}
	else if ( TranslateActivity( ACT_WALK ) != ACT_INVALID )
	{
		GetNavigator()->SetMovementActivity(ACT_WALK);
	}
	else
	{
		GetNavigator()->SetMovementActivity(ACT_RUN);
	}

	// Cover is void once I move
	Forget( bits_MEMORY_INCOVER );

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_WALK_PATH)

void CAI_BaseNPC::START_TASK_WALK_PATH_WITHIN_DIST( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetMovementActivity(ACT_WALK);
	// set that we're probably going to stop before the goal
	GetNavigator()->SetArrivalDistance( pTask->data[0].AsFloat() );
}
void CAI_BaseNPC::RUN_TASK_WALK_PATH_WITHIN_DIST( const Task_t *pTask )
{
	Vector vecDiff;
	vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

	if( vecDiff.Length() <= pTask->data[0].AsFloat() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_RUN_PATH_WITHIN_DIST( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetMovementActivity(ACT_RUN);
	// set that we're probably going to stop before the goal
	GetNavigator()->SetArrivalDistance( pTask->data[0].AsFloat() );
}
BASENPC_RUN_TASK_ALIAS(TASK_RUN_PATH_WITHIN_DIST, TASK_WALK_PATH_WITHIN_DIST)

void CAI_BaseNPC::START_TASK_RUN_PATH_FLEE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	Vector vecDiff;
	vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

	if( vecDiff.Length() <= pTask->data[0].AsFloat() )
	{
		GetNavigator()->StopMoving();
		TaskFail("Flee path shorter than task parameter");
	}
	else
	{
		GetNavigator()->SetMovementActivity(ACT_RUN);
	}
}
void CAI_BaseNPC::RUN_TASK_RUN_PATH_FLEE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	Vector vecDiff;
	vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

	if( vecDiff.Length() <= pTask->data[0].AsFloat() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_WALK_PATH_TIMED( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetMovementActivity(ACT_WALK);
	SetWait( pTask->data[0].AsFloat() );
}
void CAI_BaseNPC::RUN_TASK_WALK_PATH_TIMED( const Task_t *pTask )
{
	if ( IsWaitFinished() || 
		 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_RUN_PATH_TIMED( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetMovementActivity(ACT_RUN);
	SetWait( pTask->data[0].AsFloat() );
}
BASENPC_RUN_TASK_ALIAS(TASK_RUN_PATH_TIMED, TASK_WALK_PATH_TIMED)

void CAI_BaseNPC::START_TASK_STRAFE_PATH( const Task_t *pTask )
{
	Vector2D vec2DirToPoint; 
	Vector2D vec2RightSide;

	// to start strafing, we have to first figure out if the target is on the left side or right side
	Vector right;
	AngleVectors( GetLocalAngles(), NULL, &right, NULL );

	vec2DirToPoint = ( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() ).AsVector2D();
	Vector2DNormalize(vec2DirToPoint);
	vec2RightSide = right.AsVector2D();
	Vector2DNormalize(vec2RightSide);

	if ( DotProduct2D ( vec2DirToPoint, vec2RightSide ) > 0 )
	{
		// strafe right
		GetNavigator()->SetMovementActivity(ACT_STRAFE_RIGHT);
	}
	else
	{
		// strafe left
		GetNavigator()->SetMovementActivity(ACT_STRAFE_LEFT);
	}

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_STRAFE_PATH)

void CAI_BaseNPC::START_TASK_WAIT_FOR_MOVEMENT_STEP( const Task_t *pTask )
{
	if ( IsMovementFrozen() )
	{
		TaskFail(FAIL_FROZEN);
		return;
	}

	if(!GetNavigator()->IsGoalActive())
	{
		TaskComplete();
		return;
	}

	if ( IsActivityFinished() )
	{
		TaskComplete();
		return;
	}
	ValidateNavGoal();
}
BASENPC_RUN_TASK_ALIAS(TASK_WAIT_FOR_MOVEMENT_STEP, TASK_WAIT_FOR_MOVEMENT)

void CAI_BaseNPC::START_TASK_WAIT_FOR_MOVEMENT( const Task_t *pTask )
{
	if ( IsMovementFrozen() )
	{
		TaskFail(FAIL_FROZEN);
		return;
	}

	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
	}
	else if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}
}
void CAI_BaseNPC::RUN_TASK_WAIT_FOR_MOVEMENT( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	if ( IsMovementFrozen() )
	{
		TaskFail(FAIL_FROZEN);
		return;
	}

	bool fTimeExpired = ( pTask->data[0].AsFloat() != 0 && pTask->data[0].AsFloat() < gpGlobals->curtime - GetTimeTaskStarted() );
	
	if (fTimeExpired || GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->StopMoving();		// Stop moving
	}
	else if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}
}

void CAI_BaseNPC::START_TASK_SMALL_FLINCH( const Task_t *pTask )
{
	Remember(bits_MEMORY_FLINCHED);
	SetIdealActivity( GetFlinchActivity( false, false ) );
	m_flNextFlinchTime = gpGlobals->curtime + random_valve->RandomFloat( 3, 5 );
}
void CAI_BaseNPC::RUN_TASK_SMALL_FLINCH( const Task_t *pTask )
{
	if ( IsActivityFinished() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_BIG_FLINCH( const Task_t *pTask )
{
	Remember(bits_MEMORY_FLINCHED);
	SetIdealActivity( GetFlinchActivity( true, false ) );
	m_flNextFlinchTime = gpGlobals->curtime + random_valve->RandomFloat( 3, 5 );
}
BASENPC_RUN_TASK_ALIAS(TASK_BIG_FLINCH, TASK_SMALL_FLINCH)

void CAI_BaseNPC::START_TASK_DIE( const Task_t *pTask )
{
	GetNavigator()->StopMoving();
	SetIdealActivity( GetDeathActivity() );
	m_lifeState = LIFE_DYING;
}
void CAI_BaseNPC::RUN_TASK_DIE( const Task_t *pTask )
{
	RunDieTask();
}

void CAI_BaseNPC::START_TASK_SOUND_WAKE( const Task_t *pTask )
{
	AlertSound();
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SOUND_WAKE)

void CAI_BaseNPC::START_TASK_SOUND_DIE( const Task_t *pTask )
{
	CTakeDamageInfo info;
	DeathSound( info );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SOUND_DIE)

void CAI_BaseNPC::START_TASK_SOUND_IDLE( const Task_t *pTask )
{
	IdleSound();
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SOUND_IDLE)

void CAI_BaseNPC::START_TASK_SOUND_PAIN( const Task_t *pTask )
{
	CTakeDamageInfo info;
	PainSound( info );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SOUND_PAIN)

void CAI_BaseNPC::START_TASK_SOUND_ANGRY( const Task_t *pTask )
{
	// sounds are complete as soon as we get here, cause we've already played them.
	DevMsg( 2, "SOUND\n" );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SOUND_ANGRY)

void CAI_BaseNPC::START_TASK_SPEAK_SENTENCE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeInt() );

	SpeakSentence(pTask->data[0].AsInt());
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SPEAK_SENTENCE)

void CAI_BaseNPC::START_TASK_WAIT_FOR_SPEAK_FINISH( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeBool() );

	if ( !GetExpresser() )
		TaskComplete();
	else
	{
		// Are we waiting for our speech to end? Or for the mutex to be free?
		if ( pTask->data[0].AsBool() )
		{
			// Waiting for our speech to end
			if ( GetExpresser()->CanSpeakAfterMyself() )
			{
				TaskComplete();
			}
		}
		else
		{
			// Waiting for the speech & the delay afterwards
			if ( !GetExpresser()->IsSpeaking() )
			{
				TaskComplete();
			}
		}
	}
}
void CAI_BaseNPC::RUN_TASK_WAIT_FOR_SPEAK_FINISH( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeBool() );

	Assert( GetExpresser() );
	if ( GetExpresser() )
	{
		// Are we waiting for our speech to end? Or for the mutex to be free?
		if ( pTask->data[0].AsBool() )
		{
			// Waiting for our speech to end
			if ( GetExpresser()->CanSpeakAfterMyself() )
			{
				TaskComplete();
			}
		}
		else
		{
			// Waiting for the speech & the delay afterwards
			if ( !GetExpresser()->IsSpeaking() )
			{
				TaskComplete();
			}
		}
	}
}

void CAI_BaseNPC::START_TASK_WAIT_FOR_SCRIPT( const Task_t *pTask )
{
	if ( !m_hCine )
	{
		DevMsg( "Scripted sequence destroyed while in use\n" );
		TaskFail( FAIL_SCHEDULE_NOT_FOUND );
		return;
	}
}
void CAI_BaseNPC::RUN_TASK_WAIT_FOR_SCRIPT( const Task_t *pTask )
{
	//
	// Waiting to play a script. If the script is ready, start playing the sequence.
	//
	if ( m_hCine && m_hCine->IsTimeToStart() )
	{
		TaskComplete();
		m_hCine->OnBeginSequence(this);

		// If we have an entry, we have to play it first
		if ( m_hCine->m_iszEntry != NULL_STRING )
		{
			m_hCine->OnEntrySequence( this );
			m_hCine->StartSequence( (CAI_BaseNPC *)this, m_hCine->m_iszEntry, true );
		}
		else
		{
			m_hCine->OnActionSequence( this );
			m_hCine->StartSequence( (CAI_BaseNPC *)this, m_hCine->m_iszPlay, true );
		}

		// StartSequence() can call CineCleanup().  If that happened, just exit schedule
		if ( !m_hCine )
		{
			ClearSchedule( "Waiting for script, but lost script!" );
		}

		SetPlaybackRate( 1.0 );
		//DevMsg( 2, "Script %s has begun for %s\n", STRING( m_hCine->m_iszPlay ), GetClassname() );
	}
	else if (!m_hCine)
	{
		DevMsg( "Cine died!\n");
		TaskComplete();
	}
	else if ( IsRunningDynamicInteraction() )
	{
		// If we've lost our partner, abort
		if ( !m_hInteractionPartner )
		{
			CineCleanup();
		}
	}
}

void CAI_BaseNPC::START_TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY( const Task_t *pTask )
{
	if ( !m_hCine )
	{
		DevMsg( "Scripted sequence destroyed while in use\n" );
		TaskFail( FAIL_SCHEDULE_NOT_FOUND );
		return;
	}

	string_t iszArrivalText;

	if ( m_hCine->m_iszPreIdle != NULL_STRING )
	{
		iszArrivalText = m_hCine->m_iszPreIdle;
	}
	else if ( m_hCine->m_iszEntry != NULL_STRING )
	{
		iszArrivalText = m_hCine->m_iszEntry;
	}
	else if ( m_hCine->m_iszPlay != NULL_STRING )
	{
		iszArrivalText = m_hCine->m_iszPlay;
	}
	else if ( m_hCine->m_iszPostIdle != NULL_STRING )
	{
		iszArrivalText = m_hCine->m_iszPostIdle;
	}
	else
		iszArrivalText = NULL_STRING;

	m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
	m_strScriptArrivalSequence = NULL_STRING;

	if ( iszArrivalText != NULL_STRING )
	{
		m_ScriptArrivalActivity = (Activity)GetActivityID( STRING( iszArrivalText ) );
		if ( m_ScriptArrivalActivity == ACT_INVALID )
			m_strScriptArrivalSequence = iszArrivalText;
	}

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY)

void CAI_BaseNPC::START_TASK_PLAY_SCRIPT( const Task_t *pTask )
{
	// Throw away any stopping paths we have saved, because we 
	// won't be able to resume them after the sequence.
	GetNavigator()->IgnoreStoppingPath();

	if ( HasMovement( GetSequence() ) || m_hCine->m_bIgnoreGravity )
	{
		AddFlag( FL_FLY );
		SetGroundEntity( NULL );
	}

	if (m_hCine)
	{
		m_hCine->SynchronizeSequence( this );
	}
	//
	// Start playing a scripted sequence.
	//
	m_scriptState = SCRIPT_PLAYING;
}
void CAI_BaseNPC::RUN_TASK_PLAY_SCRIPT( const Task_t *pTask )
{
	//
	// Playing a scripted sequence.
	//
	AutoMovement( );

	if ( IsSequenceFinished() )
	{
		// Check to see if we are done with the action sequence.
		if ( m_hCine->FinishedActionSequence( this ) )
		{
			// dvs: This is done in FixScriptNPCSchedule -- doing it here is too early because we still
			//      need to play our post-action idle sequence, which might also require FL_FLY.
			//
			// drop to ground if this guy is only marked "fly" because of the auto movement
			/*if ( !(m_hCine->m_savedFlags & FL_FLY) )
			{
				if ( ( GetFlags() & FL_FLY ) && !m_hCine->m_bIgnoreGravity )
				{
					RemoveFlag( FL_FLY );
				}
			}*/

			if (m_hCine)
			{
				m_hCine->SequenceDone( this );
			}

			TaskComplete();
		}
		else if ( m_hCine && m_hCine->m_bForceSynch )
		{
			m_hCine->SynchronizeSequence( this );
		}
	}

	if ( IsRunningDynamicInteraction() && m_poseInteractionRelativeYaw > -1 )
	{
		// Animations in progress require pose parameters to be set every frame, so keep setting the interaction relative yaw pose.
		// The random value is added to help it pass server transmit checks.
		SetPoseParameter( m_poseInteractionRelativeYaw, GetPoseParameter( m_poseInteractionRelativeYaw ) + RandomFloat( -0.1f, 0.1f ) );
	}
}

void CAI_BaseNPC::START_TASK_PLAY_SCRIPT_POST_IDLE( const Task_t *pTask )
{
	//
	// Start playing a scripted post idle.
	//
	m_scriptState = SCRIPT_POST_IDLE;
}
void CAI_BaseNPC::RUN_TASK_PLAY_SCRIPT_POST_IDLE( const Task_t *pTask )
{
	if ( !m_hCine )
	{
		DevMsg( "Scripted sequence destroyed while in use\n" );
		TaskFail( FAIL_SCHEDULE_NOT_FOUND );
		return;
	}

	//
	// Playing a scripted post idle sequence. Quit early if another sequence has grabbed the NPC.
	//
	if ( IsSequenceFinished() || ( m_hCine->m_hNextCine != NULL ) )
	{
		m_hCine->PostIdleDone( this );
	}
}

void CAI_BaseNPC::START_TASK_PRE_SCRIPT( const Task_t *pTask )
{
	if ( !ai_task_pre_script.GetBool() )
	{
		TaskComplete();
	}
	else if ( !m_hCine )
	{
		TaskComplete();
		//DevMsg( "Scripted sequence destroyed while in use\n" );
		//TaskFail( FAIL_SCHEDULE_NOT_FOUND );
	}
	else
	{
		m_hCine->DelayStart( true );
		TaskComplete();
	}
}
BASENPC_RUN_TASK_NULL(TASK_PRE_SCRIPT)

void CAI_BaseNPC::START_TASK_ENABLE_SCRIPT( const Task_t *pTask )
{
	//
	// Start waiting to play a script. Play the script's pre idle animation if one
	// is specified, otherwise just go to our default idle activity.
	//
	if ( m_hCine->m_iszPreIdle != NULL_STRING )
	{
		m_hCine->OnPreIdleSequence( this );
		m_hCine->StartSequence( ( CAI_BaseNPC * )this, m_hCine->m_iszPreIdle, false );
		if ( FStrEq( STRING( m_hCine->m_iszPreIdle ), STRING( m_hCine->m_iszPlay ) ) )
		{
			SetPlaybackRate( 0 );
		}
	}
	else if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
	{
		// FIXME: too many ss assume its safe to leave the npc is whatever sequence they were in before, so only slam their activity
		//		  if they're playing a recognizable movement animation
		//
		// dvs: Check current activity rather than ideal activity. Since scripted NPCs early out in MaintainActivity,
		//      they'll never reach their ideal activity if it's different from their current activity.
		if ( GetActivity() == ACT_WALK || 
			 GetActivity() == ACT_RUN || 
			 GetActivity() == ACT_WALK_AIM || 
			 GetActivity() == ACT_RUN_AIM )
		{
			SetActivity( ACT_IDLE );
		}
	}
}
void CAI_BaseNPC::RUN_TASK_ENABLE_SCRIPT( const Task_t *pTask )
{
	if ( !m_hCine )
	{
		DevMsg( "Scripted sequence destroyed while in use\n" );
		TaskFail( FAIL_SCHEDULE_NOT_FOUND );
		return;
	}

	if (!m_hCine->IsWaitingForBegin())
	{
		m_hCine->DelayStart( false );
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_PLANT_ON_SCRIPT( const Task_t *pTask )
{
	if ( m_hTargetEnt != NULL )
	{
		SetLocalOrigin( m_hTargetEnt->GetAbsOrigin() );	// Plant on target
	}

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_PLANT_ON_SCRIPT)

void CAI_BaseNPC::START_TASK_FACE_SCRIPT( const Task_t *pTask )
{
	if ( m_hTargetEnt != NULL )
	{
		GetMotor()->SetIdealYaw( UTIL_AngleMod( m_hTargetEnt->GetLocalAngles().y ) );
	}

	if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
	{
		SetTurnActivity();
		
		// dvs: HACK: MaintainActivity won't do anything while scripted, so go straight there.
		SetActivity( GetIdealActivity() );
	}

	GetNavigator()->StopMoving();
}
BASENPC_RUN_TASK_ALIAS(TASK_FACE_SCRIPT, TASK_FACE_IDEAL)

BASENPC_START_TASK_NULL(TASK_PLAY_SCENE)
void CAI_BaseNPC::RUN_TASK_PLAY_SCENE( const Task_t *pTask )
{
	if (!IsInLockedScene())
	{
		ClearSchedule( "Playing a scene, but not in a scene!" );
	}
	if (GetNavigator()->GetGoalType() != GOALTYPE_NONE)
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_SUGGEST_STATE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_NPCSTATE );

	SetIdealState( pTask->data[0].nNpcState );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SUGGEST_STATE)

void CAI_BaseNPC::START_TASK_SET_FAIL_SCHEDULE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].nType == TASK_DATA_SCHEDULE_ID );

	m_failSchedule = pTask->data[0].schedId;

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SET_FAIL_SCHEDULE)

void CAI_BaseNPC::START_TASK_SET_TOLERANCE_DISTANCE( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetGoalTolerance( pTask->data[0].AsFloat() );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SET_TOLERANCE_DISTANCE)

void CAI_BaseNPC::START_TASK_SET_ROUTE_SEARCH_TIME( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	GetNavigator()->SetMaxRouteRebuildTime( pTask->data[0].AsFloat() );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_SET_ROUTE_SEARCH_TIME)

void CAI_BaseNPC::START_TASK_CLEAR_FAIL_SCHEDULE( const Task_t *pTask )
{
	m_failSchedule = SCHED_NONE;
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_CLEAR_FAIL_SCHEDULE)

void CAI_BaseNPC::START_TASK_WEAPON_FIND( const Task_t *pTask )
{
	m_hTargetEnt = Weapon_FindUsable( Vector(1000,1000,1000) );
	if (m_hTargetEnt)
	{
		TaskComplete();
	}
	else
	{
		TaskFail(FAIL_ITEM_NO_FIND);
	}
}
BASENPC_RUN_TASK_NULL(TASK_WEAPON_FIND)

void CAI_BaseNPC::START_TASK_ITEM_PICKUP( const Task_t *pTask )
{
	if (GetTarget() && fabs( GetTarget()->WorldSpaceCenter().z - GetAbsOrigin().z ) >= 12.0f)
	{
		SetIdealActivity( ACT_PICKUP_RACK );
	}
	else
	{
		SetIdealActivity( ACT_PICKUP_GROUND );
	}
}
void CAI_BaseNPC::RUN_TASK_ITEM_PICKUP( const Task_t *pTask )
{
	if ( IsActivityFinished() )
	{
		TaskComplete();
	}
}

void CAI_BaseNPC::START_TASK_WEAPON_PICKUP( const Task_t *pTask )
{
	if( GetActiveWeapon() )
	{
		Weapon_Drop( GetActiveWeapon() );
	}

	if( GetTarget() )
	{
		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>(GetTarget());
		if( pWeapon )
		{
			if( Weapon_IsOnGround( pWeapon ) )
			{
				// Squat down
				SetIdealActivity( ACT_PICKUP_GROUND );
			}
			else
			{
				// Reach and take this weapon from rack or shelf.
				SetIdealActivity( ACT_PICKUP_RACK );
			}

			return;
		}
	}

	TaskFail("Weapon went away!\n");
}
void CAI_BaseNPC::RUN_TASK_WEAPON_PICKUP( const Task_t *pTask )
{
	if ( IsActivityFinished() )
	{
		CBaseCombatWeapon	 *pWeapon = dynamic_cast<CBaseCombatWeapon *>(	(CBaseEntity *)m_hTargetEnt);
		CBaseCombatCharacter *pOwner  = pWeapon->GetOwner();
		if ( !pOwner )
		{
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_WEAPON_OWNED);
		}
	}
}

void CAI_BaseNPC::START_TASK_WEAPON_CREATE( const Task_t *pTask )
{
	if( !GetActiveWeapon() && GetTarget() )
	{
		// Create a copy of the weapon this NPC is trying to pick up.
		CBaseCombatWeapon *pTargetWeapon = dynamic_cast<CBaseCombatWeapon*>(GetTarget());

		if( pTargetWeapon )
		{
			CBaseCombatWeapon *pWeapon = Weapon_Create( pTargetWeapon->GetClassname() );
			if ( pWeapon )
			{
				Weapon_Equip( pWeapon );
			}
		}
	}

	SetTarget( NULL );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_WEAPON_CREATE)

void CAI_BaseNPC::START_TASK_FALL_TO_GROUND( const Task_t *pTask )
{
	// Set a wait time to try to force a ground ent.
	SetWait(4);
}
void CAI_BaseNPC::RUN_TASK_FALL_TO_GROUND( const Task_t *pTask )
{
	if ( GetFlags() & FL_ONGROUND )
	{
		TaskComplete();
	}
	else if( GetFlags() & FL_FLY )
	{
		// We're never going to fall if we're FL_FLY.
		RemoveFlag( FL_FLY );
	}
	else
	{
		if( IsWaitFinished() )
		{
			// After 4 seconds of trying to fall to ground, Assume that we're in a bad case where the NPC
			// isn't actually falling, and make an attempt to slam the ground entity to whatever's under the NPC.
			Vector maxs = WorldAlignMaxs() - Vector( .1, .1, .2 );
			Vector mins = WorldAlignMins() + Vector( .1, .1, 0 );
			Vector vecStart	= GetAbsOrigin() + Vector( 0, 0, .1 );
			Vector vecDown	= GetAbsOrigin();
			vecDown.z -= 0.2;

			trace_t trace;
			m_pMoveProbe->TraceHull( vecStart, vecDown, mins, maxs, GetAITraceMask(), &trace );

			if( trace.m_pEnt )
			{
				// Found something!
				SetGroundEntity( trace.m_pEnt );
				TaskComplete();
			}
			else
			{
				// Try again in a few seconds.
				SetWait(4);
			}
		}
	}
}

void CAI_BaseNPC::START_TASK_WANDER( const Task_t *pTask )
{
	Assert( pTask->numData == 2 );
	Assert( pTask->data[0].CanBeFloat() );
	Assert( pTask->data[1].CanBeFloat() );

	if ( GetNavigator()->SetWanderGoal( pTask->data[0].AsFloat(), pTask->data[1].AsFloat() ) )
		TaskComplete();
	else
		TaskFail(FAIL_NOT_REACHABLE);
}
BASENPC_RUN_TASK_NULL(TASK_WANDER)

void CAI_BaseNPC::START_TASK_FREEZE( const Task_t *pTask )
{
	SetPlaybackRate( 0 );
}
void CAI_BaseNPC::RUN_TASK_FREEZE( const Task_t *pTask )
{
	if ( m_flFrozen < 1.0f )
	{
		Unfreeze();
	}
}

void CAI_BaseNPC::START_TASK_GATHER_CONDITIONS( const Task_t *pTask )
{
	GatherConditions();

	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_GATHER_CONDITIONS)

void CAI_BaseNPC::START_TASK_IGNORE_OLD_ENEMIES( const Task_t *pTask )
{
	m_flAcceptableTimeSeenEnemy = gpGlobals->curtime;
	if ( GetEnemy() && GetEnemyLastTimeSeen() < m_flAcceptableTimeSeenEnemy )
	{
		CBaseEntity *pNewEnemy = BestEnemy();

		Assert( pNewEnemy != GetEnemy() );

		if( pNewEnemy != NULL )
		{
			// New enemy! Clear the timers and set conditions.
			SetEnemy( pNewEnemy );
			SetState( NPC_STATE_COMBAT );
		}
		else
		{
			SetEnemy( NULL );
			ClearAttackConditions();
		}
	}
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_IGNORE_OLD_ENEMIES)

void CAI_BaseNPC::START_TASK_ADD_HEALTH( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	TakeHealth( pTask->data[0].AsFloat(), DMG_GENERIC );
	TaskComplete();
}
BASENPC_RUN_TASK_NULL(TASK_ADD_HEALTH)

BASENPC_START_TASK_NULL(TASK_FIND_COVER_FROM_BEST_SOUND)
void CAI_BaseNPC::RUN_TASK_FIND_COVER_FROM_BEST_SOUND( const Task_t *pTask )
{
	Assert( pTask->numData == 1 );
	Assert( pTask->data[0].CanBeFloat() );

	switch( GetTaskInterrupt() )
	{
	case 0:
		{
			if ( !FindCoverFromBestSound( &m_vInterruptSavePosition ) )
				TaskFail(FAIL_NO_COVER);
			else
			{
				GetNavigator()->IgnoreStoppingPath();
				LockBestSound();
				TaskInterrupt();
			}
		}
		break;

	case 1:
		{
			AI_NavGoal_t goal(m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE);

			CSound *pBestSound = GetBestSound();
			if ( pBestSound )
				goal.maxInitialSimplificationDist = pBestSound->Volume() * 0.5;

			if ( GetNavigator()->SetGoal( goal ) )
			{
				m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[0].AsFloat();
			}
		}
		break;
	}
}

void CAI_BaseNPC::START_TASK_INVALID( const Task_t *pTask )
{
	DebuggerBreak();
}
void CAI_BaseNPC::RUN_TASK_INVALID( const Task_t *pTask )
{
	DebuggerBreak();
}

void CAI_BaseNPC::START_TASK_DEBUG_BREAK( const Task_t *pTask )
{
	DebuggerBreak();
}
void CAI_BaseNPC::RUN_TASK_DEBUG_BREAK( const Task_t *pTask )
{
	DebuggerBreak();
}

BASENPC_UNIMPLEMENTED_TASK(TASK_REACT_TO_COMBAT_SOUND)
BASENPC_UNIMPLEMENTED_TASK(TASK_SOUND_DEATH)
BASENPC_UNIMPLEMENTED_TASK(TASK_FIND_LATERAL_COVER_FROM_ENEMY)

//TODO Arthurdead!!!!
#if 0
#ifndef AI_USES_NAV_MESH
	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
#else
	case TASK_FIND_NEAR_AREA_COVER_FROM_ENEMY:
	case TASK_FIND_FAR_AREA_COVER_FROM_ENEMY:
	case TASK_FIND_AREA_COVER_FROM_ENEMY:
#endif
	case TASK_FIND_COVER_FROM_ENEMY:
		{	
			bool 	bNodeCover 		= ( task != TASK_FIND_COVER_FROM_ENEMY );
		#ifndef AI_USES_NAV_MESH
			float 	flMinDistance 	= ( task == TASK_FIND_FAR_NODE_COVER_FROM_ENEMY ) ? pTask->flTaskData : 0.0;
			float 	flMaxDistance 	= ( task == TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY ) ? pTask->flTaskData : FLT_MAX;
		#else
			float 	flMinDistance 	= ( task == TASK_FIND_FAR_AREA_COVER_FROM_ENEMY ) ? pTask->data[0].AsFloat() : 0.0;
			float 	flMaxDistance 	= ( task == TASK_FIND_NEAR_AREA_COVER_FROM_ENEMY ) ? pTask->data[0].AsFloat() : FLT_MAX;
		#endif

			if ( FindCoverFromEnemy( bNodeCover, flMinDistance, flMaxDistance ) )
			{
				if ( task == TASK_FIND_COVER_FROM_ENEMY )
					m_flMoveWaitFinished = gpGlobals->curtime + pTask->data[0].AsFloat();
				TaskComplete();
			}
			else
				TaskFail(FAIL_NO_COVER);
		}
		break;

#ifndef AI_USES_NAV_MESH
	case TASK_GET_PATH_TO_RANDOM_NODE:  // Task argument is lenth of path to build
		{
			if ( GetNavigator()->SetRandomGoal( pTask->flTaskData ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
		
			break;
		}
#else
	case TASK_GET_PATH_TO_RANDOM_AREA:  // Task argument is lenth of path to build
		{
			if ( GetNavigator()->SetRandomGoal( pTask->data[0].AsFloat() ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_AREA);
		
			break;
		}
#endif
#endif

BASENPC_UNIMPLEMENTED_TASK(TASK_FIND_NEAR_NAV_MESH_COVER_FROM_ENEMY)
BASENPC_UNIMPLEMENTED_TASK(TASK_FIND_FAR_NAV_MESH_COVER_FROM_ENEMY)
BASENPC_UNIMPLEMENTED_TASK(TASK_FIND_NAV_MESH_COVER_FROM_ENEMY)
BASENPC_UNIMPLEMENTED_TASK(TASK_GET_PATH_TO_RANDOM_LOCATION)