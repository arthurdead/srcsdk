//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "ai_behavior_assault.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "gamestringpool.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_debug_assault("ai_debug_assault", "0");

CGameString g_AssaultPointString( "assault_assaultpoint" );
CGameString g_RallyPointString( "assault_rallypoint" );

BEGIN_MAPENTITY( CRallyPoint, MAPENT_POINTCLASS )
	DEFINE_MAP_FIELD( m_AssaultPointName,
		"assaultpoint",
		"Assault Point",
		"Location to move to as assault begins"
	),

	DEFINE_MAP_FIELD( m_RallySequenceName,
		"rallysequence",
		"Rally Sequence",
		"Override the NPC's wait activity by entering a sequence name."
	),

	DEFINE_MAP_FIELD( m_flAssaultDelay,
		"assaultdelay",
		"Assault Delay",
		"How long to wait after cue is given before assault begins.",
		"0"
	),

	DEFINE_MAP_FIELD( m_iPriority,
		"priority",
		"Priority",
		"Higher priority rally points get selected first.",
		"1"
	),

	DEFINE_MAP_FIELD( m_iStrictness,
		"strict"
	),

	DEFINE_MAP_FIELD( m_bForceCrouch,
		"forcecrouch",
		"Force Crouch",
		"NPCs using this assault point are forced into crouching while holding it.",
		"0"
	),

	DEFINE_MAP_FIELD( m_bIsUrgent,
		"urgent",
		"Urgent",
		"If true, NPCs will consider movement to this rally point as Urgent Navigation. NPCs will ignore prop_physics obstructions, and eventually teleport to reach the point.",
		"0"
	),

	DEFINE_MAP_FIELD( m_bShouldLock,
		"lockpoint"
	),

	DEFINE_MAP_OUTPUT( m_OnArrival,
		"OnArrival",
		"Fires when the NPC reaches this rally point"
	),
END_MAPENTITY();

//---------------------------------------------------------
// Purpose: Communicate exclusivity
//---------------------------------------------------------
int CRallyPoint::DrawDebugTextOverlays()
{
	int		offset;

	offset = BaseClass::DrawDebugTextOverlays();
	if ( (m_debugOverlays & OVERLAY_TEXT_BIT) )
	{	
		switch( m_sExclusivity )
		{
		case RALLY_EXCLUSIVE_NOT_EVALUATED:
			EntityText( offset, "Exclusive: Not Evaluated", 0 );
			break;
		case RALLY_EXCLUSIVE_YES:
			EntityText( offset, "Exclusive: YES", 0 );
			break;
		case RALLY_EXCLUSIVE_NO:
			EntityText( offset, "Exclusive: NO", 0 );
			break;
		default:
			EntityText( offset, "Exclusive: !?INVALID?!", 0 );
			break;
		}
		offset++;

		if( IsLocked() )
			EntityText( offset, "LOCKED.", 0 );
		else
			EntityText( offset, "Available", 0 );

		offset++;
	}

	return offset;
}

//---------------------------------------------------------
// Purpose: If a rally point is 'exclusive' that means that
// anytime an NPC is anywhere on the assault chain that
// begins with this rally point, the assault is considered
// 'exclusive' and no other NPCs will be allowed to use it
// until the current NPC clears the entire assault chain
// or dies.
// 
// If exclusivity has not been determined the first time
// this function is called, it will be computed and cached
//---------------------------------------------------------
bool CRallyPoint::IsExclusive()
{
	if( m_sExclusivity == RALLY_EXCLUSIVE_NOT_EVALUATED )
	{
		// We need to evaluate! Walk the chain of assault points
		// and if *ANY* assault points  on this assault chain
		// are set to Never Time Out then set this rally point to 
		// be exclusive to stop other NPC's walking down the chain
		// and ending up clumped up at the infinite rally point.
		CAssaultPoint *pAssaultEnt = (CAssaultPoint *)gEntList.FindEntityByName( NULL, m_AssaultPointName );

		if( !pAssaultEnt )
		{
			// Well, this is awkward. Leave it up to other assault code to tattle on the missing assault point.
			// We will just assume this assault is not exclusive.
			m_sExclusivity = RALLY_EXCLUSIVE_NO;
			return false;
		}

		// Otherwise, we start by assuming this assault chain is not exclusive.
		m_sExclusivity = RALLY_EXCLUSIVE_NO;

		if( pAssaultEnt )
		{
			CAssaultPoint *pFirstAssaultEnt = pAssaultEnt; //some assault chains are circularly linked

			do
			{
				if( pAssaultEnt->m_bNeverTimeout )
				{
					// We found a never timeout assault point! That makes this whole chain exclusive.
					m_sExclusivity = RALLY_EXCLUSIVE_YES;
					break;
				}

				pAssaultEnt = (CAssaultPoint *)gEntList.FindEntityByName( NULL, pAssaultEnt->m_NextAssaultPointName );
				if ( pAssaultEnt && !pAssaultEnt->ClassMatchesExact( g_AssaultPointString ) )
				{
					pAssaultEnt = NULL;
				}
			} while( (pAssaultEnt != NULL) && (pAssaultEnt != pFirstAssaultEnt) );

		}
	}

	return (m_sExclusivity == RALLY_EXCLUSIVE_YES);
}


BEGIN_MAPENTITY( CAssaultPoint, MAPENT_POINTCLASS )

	DEFINE_KEYFIELD_AUTO( m_NextAssaultPointName, "nextassaultpoint" ),
	DEFINE_KEYFIELD_AUTO( m_flAssaultTimeout, "assaulttimeout" ),
	DEFINE_KEYFIELD_AUTO( m_bClearOnContact, "clearoncontact" ),
	DEFINE_KEYFIELD_AUTO( m_bAllowDiversion, "allowdiversion" ),
	DEFINE_KEYFIELD_AUTO( m_flAllowDiversionRadius, "allowdiversionradius" ),
	DEFINE_KEYFIELD_AUTO( m_bNeverTimeout, "nevertimeout" ),
	DEFINE_KEYFIELD_AUTO( m_iStrictness, "strict" ),
	DEFINE_KEYFIELD_AUTO( m_bForceCrouch, "forcecrouch" ),
	DEFINE_KEYFIELD_AUTO( m_bIsUrgent, "urgent" ),

	DEFINE_KEYFIELD_AUTO( m_flAssaultPointTolerance, "assaulttolerance" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetClearOnContact", InputSetClearOnContact ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowDiversion", InputSetAllowDiversion ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetForceClear", InputSetForceClear ),

	// Outputs
	DEFINE_OUTPUT( m_OnArrival, "OnArrival" ),
	DEFINE_OUTPUT( m_OnAssaultClear, "OnAssaultClear" ),
END_MAPENTITY();

LINK_ENTITY_TO_CLASS( assault_rallypoint, CRallyPoint );	// just a copy of info_target for now
LINK_ENTITY_TO_CLASS( assault_assaultpoint, CAssaultPoint ); // has its own class because it needs the entity I/O

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::CanRunAScriptedNPCInteraction( bool bForced )
{
	if ( m_AssaultCue == CUE_NO_ASSAULT )
	{
		// It's OK with the assault behavior, so leave it up to the base class.
		return BaseClass::CanRunAScriptedNPCInteraction( bForced );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_AssaultBehavior::CAI_AssaultBehavior()
{
	m_AssaultCue = CUE_NO_ASSAULT;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays
// Input  : Previous text offset from the top
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_AssaultBehavior::DrawDebugTextOverlays( int text_offset )
{
	char	tempstr[ 512 ];
	int		offset;

	offset = BaseClass::DrawDebugTextOverlays( text_offset );
	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{	
		Q_snprintf( tempstr, sizeof(tempstr), "Assault Point: %s %s", STRING( m_AssaultPointName ), VecToString( m_hAssaultPoint->GetAbsOrigin() ) );
		GetOuter()->EntityText( offset, tempstr, 0 );
		offset++;
	}

	return offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::ReceiveAssaultCue( AssaultCue_t cue )
{
	if ( GetOuter() )
		GetOuter()->ForceDecisionThink();

	m_ReceivedAssaultCue = cue;

	SetCondition( COND_PROVOKED );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::AssaultHasBegun()
{
	if( m_AssaultCue == CUE_DONT_WAIT && IsRunning() && m_bHitRallyPoint )
	{
		return true;
	}

	return m_ReceivedAssaultCue == m_AssaultCue;
}

//-----------------------------------------------------------------------------
// Purpose: Find an assaultpoint matching the iszAssaultPointName. 
//			If more than one assault point of this type is found, randomly
//			use any of them EXCEPT the one most recently used.
//-----------------------------------------------------------------------------
CAssaultPoint *CAI_AssaultBehavior::FindAssaultPoint( string_t iszAssaultPointName )
{
	CUtlVector<CAssaultPoint*>pAssaultPoints;
	CUtlVector<CAssaultPoint*>pClearAssaultPoints;

	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, iszAssaultPointName );
	while( pEnt != NULL )
	{
		CAssaultPoint *pAssaultEnt;
		
		if ( !pEnt->Downcast( g_AssaultPointString, &pAssaultEnt) )
		{
			if ( !pEnt->ClassMatchesExact( g_RallyPointString ) )
			{
				DevMsg( "**ERROR: Entity %s being used as an assault_assaultpoint, but is actually a %s!\n", pEnt->GetDebugName(), pEnt->GetClassname() );
			}
		}
		else
		{
			pAssaultPoints.AddToTail( pAssaultEnt );
		}
		
		pEnt = gEntList.FindEntityByName( pEnt, iszAssaultPointName );
	}

	// Didn't find any?!
	if( pAssaultPoints.Count() < 1 )
		return NULL;

	// Only found one, just return it.
	if( pAssaultPoints.Count() == 1 )
		return pAssaultPoints[0];

	// Throw out any nodes that I cannot fit my bounding box on.
	for( int i = 0 ; i < pAssaultPoints.Count() ; i++ )
	{
		trace_t tr;
		CAI_BaseNPC *pNPC = GetOuter();
		CAssaultPoint *pAssaultPoint = pAssaultPoints[i];

		AI_TraceHull ( pAssaultPoint->GetAbsOrigin(), pAssaultPoint->GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), MASK_SOLID, pNPC, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0 )
		{
			// Copy this into the list of clear points.
			pClearAssaultPoints.AddToTail(pAssaultPoint);
		}
	}

	// Only one clear assault point left!
	if( pClearAssaultPoints.Count() == 1 )
		return pClearAssaultPoints[0];

	// NONE left. Just return a random assault point, knowing that it's blocked. This is the old behavior, anyway.
	if( pClearAssaultPoints.Count() < 1 )
		return pAssaultPoints[ random_valve->RandomInt(0, (pAssaultPoints.Count() - 1)) ];

	// We found several! First throw out the one most recently used.
	// This prevents picking the same point at this branch twice in a row.
	float flMostRecentTime = -1.0f; // Impossibly old
	int iMostRecentIndex = -1;
	for( int i = 0 ; i < pClearAssaultPoints.Count() ; i++ )
	{
		if( pClearAssaultPoints[i]->m_flTimeLastUsed > flMostRecentTime )
		{
			flMostRecentTime = pClearAssaultPoints[i]->m_flTimeLastUsed;
			iMostRecentIndex = i;
		}
	}

	Assert( iMostRecentIndex > -1 );

	// Remove the most recently used 
	pClearAssaultPoints.Remove( iMostRecentIndex );

	if ( !m_hGoal || m_hGoal->m_BranchMethod == BRANCH_RANDOM )
		return pClearAssaultPoints[ random_valve->RandomInt(0, (pClearAssaultPoints.Count() - 1)) ];

	CAssaultPoint *pBest = NULL;
	Vector vStart = GetOuter()->GetAbsOrigin();
	float distBest, distCur;

	if ( m_hGoal->m_BranchMethod == BRANCH_CLOSEST )
	{
		distBest = FLT_MAX;
		for( int i = 0 ; i < pClearAssaultPoints.Count() ; i++ )
		{
			distCur = ( pClearAssaultPoints[i]->GetAbsOrigin() - vStart ).LengthSqr();
			if ( distCur < distBest )
			{
				pBest = pClearAssaultPoints[i];
				distBest = distCur;
			}
		}
	}
	else
	{
		distBest = 0;
		for( int i = 0 ; i < pClearAssaultPoints.Count() ; i++ )
		{
			distCur = ( pClearAssaultPoints[i]->GetAbsOrigin() - vStart ).LengthSqr();
			if ( distCur > distBest )
			{
				pBest = pClearAssaultPoints[i];
				distBest = distCur;
			}
		}
	}

	return pBest;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::SetAssaultPoint( CAssaultPoint *pAssaultPoint )
{
	Assert( pAssaultPoint != NULL );
	m_hAssaultPoint = pAssaultPoint;
	pAssaultPoint->m_flTimeLastUsed = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::ClearAssaultPoint( void )
{
	// To announce clear means that this NPC hasn't seen any live targets in
	// the assault area for the length of time the level designer has 
	// specified that they should be vigilant. This effectively deactivates
	// the assault behavior.
	// This can also be happen if an assault point has ClearOnContact set, and
	// an NPC assaulting to this point has seen an enemy.
	Assert( m_hAssaultPoint != NULL );
	if ( m_hAssaultPoint == NULL )
	{
		DevMsg("**ERROR: ClearAssaultPoint called with no assault point\n" );

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		ClearSchedule( "No assault point" );
		return;
	}

	// keep track of the name of the assault point
	m_AssaultPointName = m_hAssaultPoint->m_NextAssaultPointName;

	// Do we need to move to another assault point?
	if( m_hAssaultPoint->m_NextAssaultPointName != NULL_STRING )
	{
		CAssaultPoint *pNextPoint = FindAssaultPoint( m_hAssaultPoint->m_NextAssaultPointName );
		
		if( pNextPoint )
		{
			SetAssaultPoint( pNextPoint );
			
			// Send our NPC to the next assault point!
			m_bHitAssaultPoint = false;

			return;
		}
		else
		{
			CBaseEntity *pNextRally = gEntList.FindEntityByName( NULL, m_hAssaultPoint->m_NextAssaultPointName );
			if ( pNextRally->ClassMatchesExact( g_RallyPointString ) && m_hGoal && m_hGoal->IsActive() )
			{
				SetParameters( m_hAssaultPoint->m_NextAssaultPointName, (AssaultCue_t)m_hGoal->m_AssaultCue, m_hGoal->m_RallySelectMethod );
				return;
			}
			else
			{
				DevMsg("**ERROR: Can't find next assault point: %s\n", STRING(m_hAssaultPoint->m_NextAssaultPointName) );

				// Bomb out of assault behavior.
				m_AssaultCue = CUE_NO_ASSAULT;
				ClearSchedule( "Can't find next assault point" );

				return;
			}
		}
	}

	// Just set the cue back to NO_ASSAULT. This disables the behavior.
	m_AssaultCue = CUE_NO_ASSAULT;

	// Exclusive or not, we unlock here. The assault is done.
	UnlockRallyPoint();

	m_hAssaultPoint->m_OnAssaultClear.FireOutput( GetOuter(), GetOuter(), 0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::OnHitAssaultPoint( void )
{
	GetOuter()->SpeakSentence( ASSAULT_SENTENCE_HIT_ASSAULT_POINT );
	m_bHitAssaultPoint = true;

	Assert( m_hAssaultPoint != NULL );
	if ( m_hAssaultPoint )
	{
		m_hAssaultPoint->m_OnArrival.FireOutput( GetOuter(), m_hAssaultPoint.Get(), 0 );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// If this NPC is moving towards an assault point which
	//		a) Has a Next Assault Point, and 
	//		b) Is flagged to Clear On Arrival,
	// then hit and clear the assault point (fire all entity I/O) and move on to the next one without
	// interrupting the NPC's schedule. This provides a more fluid movement from point to point.
	if( IsCurSchedule( SCHED_MOVE_TO_ASSAULT_POINT ) )
	{
		if( m_hAssaultPoint && m_hAssaultPoint->HasSpawnFlags(SF_ASSAULTPOINT_CLEARONARRIVAL) && m_hAssaultPoint->m_NextAssaultPointName != NULL_STRING )
		{
			float flDist = GetAbsOrigin().DistTo( m_hAssaultPoint->GetAbsOrigin() );

			if( flDist <= GetOuter()->GetMotor()->MinStoppingDist() )
			{
				OnHitAssaultPoint();
				ClearAssaultPoint();

				if ( m_hAssaultPoint )
				{
					AI_NavGoal_t goal( m_hAssaultPoint->GetAbsOrigin() );
					goal.pTarget = m_hAssaultPoint.Get();
					
					if ( GetNavigator()->SetGoal( goal ) == false )
					{
						TaskFail( "Can't refresh assault path" );
					}
				}
				else
				{
					// Bomb out of assault behavior.
					m_AssaultCue = CUE_NO_ASSAULT;
					ClearSchedule( "Can't find next assault point" );
				}
			}
		}
	}

	if ( IsForcingCrouch() && GetOuter()->IsCrouching() )
	{
		ClearCondition( COND_HEAR_BULLET_IMPACT );
	}

	if( OnStrictAssault() )
	{
		// Don't get distracted. Die trying if you have to.
		ClearCondition( COND_HEAR_DANGER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		BaseClass::StartTask( pTask );
		break;

	case TASK_ASSAULT_DEFER_SCHEDULE_SELECTION:
		Assert( pTask->numData == 1 );
		Assert( pTask->data[0].CanBeFloat() );

		m_flTimeDeferScheduleSelection = gpGlobals->curtime + pTask->data[0].AsFloat();
		TaskComplete();
		break;

	case TASK_ASSAULT_MOVE_AWAY_PATH:
		break;

	case TASK_ANNOUNCE_CLEAR:
		{
			// If we're at an assault point that can never be cleared, keep waiting forever (if it's the last point in the assault)
			if ( m_hAssaultPoint && 
				 !m_hAssaultPoint->HasSpawnFlags( SF_ASSAULTPOINT_CLEARONARRIVAL ) &&
				 m_hAssaultPoint->m_bNeverTimeout && 
				 m_hAssaultPoint->m_NextAssaultPointName == NULL_STRING )
			{
				TaskComplete();
				return;
			}

			ClearAssaultPoint();
			TaskComplete();
		}
		break;

	case TASK_WAIT_ASSAULT_DELAY:
		{
			if( m_hRallyPoint )
			{
				GetOuter()->SetWait( m_hRallyPoint->m_flAssaultDelay );
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_AWAIT_ASSAULT_TIMEOUT:
		// Maintain vigil for as long as the level designer has asked. Wait
		// and look for targets.
		GetOuter()->SetWait( m_hAssaultPoint->m_flAssaultTimeout );
		break;

	case TASK_GET_PATH_TO_RALLY_POINT:
		{
			AI_NavGoal_t goal( m_hRallyPoint->GetAbsOrigin() );
			goal.pTarget = m_hRallyPoint.Get();
			if ( GetNavigator()->SetGoal( goal ) == false )
			{
				// Try and get as close as possible otherwise
				AI_NavGoal_t nearGoal( GOALTYPE_LOCATION, m_hRallyPoint->GetAbsOrigin(), AIN_DEF_ACTIVITY, 256 );
				if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
				{
					//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
					ClearCondition( COND_TASK_FAILED );
					GetNavigator()->SetArrivalDirection( m_hRallyPoint->GetAbsAngles() );
					TaskComplete();
					return;
				}
			}
			GetNavigator()->SetArrivalDirection( m_hRallyPoint->GetAbsAngles() );
		}
		break;

	case TASK_FACE_RALLY_POINT:
		{
			UpdateForceCrouch();
			
			GetMotor()->SetIdealYaw( m_hRallyPoint->GetAbsAngles().y );
			GetOuter()->SetTurnActivity(); 
		}
		break;

	case TASK_GET_PATH_TO_ASSAULT_POINT:
		{
			AI_NavGoal_t goal( m_hAssaultPoint->GetAbsOrigin() );
			goal.pTarget = m_hAssaultPoint.Get();
			if ( GetNavigator()->SetGoal( goal ) == false )
			{
				// Try and get as close as possible otherwise
				AI_NavGoal_t nearGoal( GOALTYPE_LOCATION, m_hAssaultPoint->GetAbsOrigin(), AIN_DEF_ACTIVITY, 256 );
				if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
				{
					//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
					ClearCondition( COND_TASK_FAILED );
					GetNavigator()->SetArrivalDirection( m_hAssaultPoint->GetAbsAngles() );
					TaskComplete();
					return;
				}
			}
			GetNavigator()->SetArrivalDirection( m_hAssaultPoint->GetAbsAngles() );
		}
		break;

	case TASK_FACE_ASSAULT_POINT:
		{
			UpdateForceCrouch();

			if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				// If I can already fight when I arrive, don't bother running any facing code. Let
				// The combat AI do that. Turning here will only make the NPC look dumb in a combat
				// situation because it will take time to turn before attacking.
				TaskComplete();
			}
			else
			{
				GetMotor()->SetIdealYaw( m_hAssaultPoint->GetAbsAngles().y );
				GetOuter()->SetTurnActivity(); 
			}
		}
		break;

	case TASK_HIT_ASSAULT_POINT:
		OnHitAssaultPoint();
		TaskComplete();
		break;

	case TASK_HIT_RALLY_POINT:
		// Once we're stading on it and facing the correct direction,
		// we have arrived at rally point.
		GetOuter()->SpeakSentence( ASSAULT_SENTENCE_HIT_RALLY_POINT );

		m_bHitRallyPoint = true;
		m_hRallyPoint->m_OnArrival.FireOutput( GetOuter(), m_hRallyPoint.Get(), 0 );

		TaskComplete();
		break;

	case TASK_AWAIT_CUE:
		if( PollAssaultCue() )
		{
			TaskComplete();
		}
		else
		{
			// Don't do anything if we've been told to crouch
			if ( IsForcingCrouch() )
				break;

			else if( m_hRallyPoint->m_RallySequenceName != NULL_STRING )
			{
				// The cue hasn't been given yet, so set to the rally sequence.
				sequence_t sequence = GetOuter()->LookupSequence( STRING( m_hRallyPoint->m_RallySequenceName ) );
				if( sequence != INVALID_SEQUENCE )
				{
					GetOuter()->ResetSequence( sequence );
					GetOuter()->SetIdealActivity( ACT_DO_NOT_DISTURB );
				}
			}
			else
			{
				// Only chain this task if I'm not playing a custom animation
				if( GetOuter()->GetEnemy() )
				{
					ChainStartTask( TASK_FACE_ENEMY, 0 );
				}
			}
		}
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
void CAI_AssaultBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_ASSAULT_DELAY:
	case TASK_AWAIT_ASSAULT_TIMEOUT:
		if ( m_hAssaultPoint )
		{
			if ( m_hAssaultPoint->m_bInputForcedClear || (m_hAssaultPoint->m_bClearOnContact && HasCondition( COND_SEE_ENEMY )) )
			{
				// If we're on an assault that should clear on contact, clear when we see an enemy
				TaskComplete();
			}
		}

		if( GetOuter()->IsWaitFinished() && ( pTask->iTask == TASK_WAIT_ASSAULT_DELAY || !m_hAssaultPoint->m_bNeverTimeout ) )
		{
			TaskComplete();
		}
		break;

	case TASK_FACE_RALLY_POINT:
	case TASK_FACE_ASSAULT_POINT:
		GetMotor()->UpdateYaw();

		if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Out early if the NPC can attack.
			TaskComplete();
		}

		if ( GetOuter()->FacingIdeal() )
		{
			TaskComplete();
		}
		break;

	case TASK_AWAIT_CUE:
		// If we've lost our rally point, abort
		if ( !m_hRallyPoint )
		{
			TaskFail("No rally point.");
			break;
		}

		if( PollAssaultCue() )
		{
			TaskComplete();
		}

		if ( IsForcingCrouch() )
			break;

		if( GetOuter()->GetEnemy() && m_hRallyPoint->m_RallySequenceName == NULL_STRING && !HasCondition(COND_ENEMY_OCCLUDED) )
		{
			// I have an enemy and I'm NOT playing a custom animation.
			ChainRunTask( TASK_FACE_ENEMY, 0 );
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		if ( ai_debug_assault.GetBool() )
		{
			if ( IsCurSchedule( SCHED_MOVE_TO_ASSAULT_POINT ) )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), GetNavigator()->GetGoalPos(), 255,0,0, true,0.1);
				NDebugOverlay::Box( GetNavigator()->GetGoalPos(), -Vector(10,10,10), Vector(10,10,10), 255,0,0, 8, 0.1 );
			}
			else if ( IsCurSchedule( SCHED_MOVE_TO_RALLY_POINT ) )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), GetNavigator()->GetGoalPos(), 0,255,0, true,0.1);
				NDebugOverlay::Box( GetNavigator()->GetGoalPos(), -Vector(10,10,10), Vector(10,10,10), 0,255,0, 8, 0.1 );
			}
		}

		if ( m_hAssaultPoint && (m_hAssaultPoint->m_bInputForcedClear || (m_hAssaultPoint->m_bClearOnContact && HasCondition( COND_SEE_ENEMY ))) )
		{
			DevMsg( "Assault Cleared due to Contact or Input!\n" );
			ClearAssaultPoint();
			TaskComplete();
			return;
		}

		if ( ( ( !GetOuter()->DidChooseEnemy() && gpGlobals->curtime - GetOuter()->GetTimeEnemyAcquired() > 1 ) || !GetOuter()->GetEnemy() ) )
		{
			CBaseEntity *pNewEnemy = GetOuter()->BestEnemy();

			if( pNewEnemy != NULL && pNewEnemy != GetOuter()->GetEnemy() )
			{
				GetOuter()->SetEnemy( pNewEnemy );
				GetOuter()->SetState( NPC_STATE_COMBAT );
			}
		}

		BaseClass::RunTask( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CRallyPoint *CAI_AssaultBehavior::FindBestRallyPointInRadius( const Vector &vecCenter, float flRadius )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::FindBestRallyPointInRadius", VPROF_BUDGETGROUP_NPCS );

	const int RALLY_SEARCH_ENTS	= 30;
	CBaseEntity *pEntities[RALLY_SEARCH_ENTS];
	int iNumEntities = UTIL_EntitiesInSphere( pEntities, RALLY_SEARCH_ENTS, vecCenter, flRadius, FL_NO_ENTITY_FLAGS );

	CRallyPoint *pBest = NULL;
	int iBestPriority = -1;

	for ( int i = 0; i < iNumEntities; i++ )
	{
		CRallyPoint *pRallyEnt = dynamic_cast<CRallyPoint *>(pEntities[i]);

		if( pRallyEnt )
		{
			if( !pRallyEnt->IsLocked() )
			{
				// Consider this point.
				if( pRallyEnt->m_iPriority > iBestPriority )
				{
					pBest = pRallyEnt;
					iBestPriority = pRallyEnt->m_iPriority;
				}
			}
		}
	}

	return pBest;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::IsValidShootPosition( const Vector &vLocation )
{
	CBaseEntity *pCuePoint = NULL;
	float flTolerance = 0.0f;

	if( m_bHitRallyPoint && !m_bHitAssaultPoint && !AssaultHasBegun() )
	{
		if( m_hRallyPoint != NULL )
		{
			pCuePoint = m_hRallyPoint.Get();
			flTolerance = CUE_POINT_TOLERANCE;
		}
	}
	else if( m_bHitAssaultPoint )
	{
		if( m_hAssaultPoint != NULL )
		{
			pCuePoint = m_hAssaultPoint.Get();
			flTolerance = m_hAssaultPoint->m_flAssaultPointTolerance;
		}
	}

	if ( pCuePoint && (vLocation - pCuePoint->GetAbsOrigin()).Length2DSqr() > Square( flTolerance - 0.1 ) )
		return false;

	return BaseClass::IsValidShootPosition( vLocation );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CAI_AssaultBehavior::GetMaxTacticalLateralMovement( void )
{
	return CUE_POINT_TOLERANCE - 0.1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::UpdateOnRemove()
{
	// Ignore exclusivity. Our NPC just died.
	UnlockRallyPoint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::OnStrictAssault( void )
{ 
	return (m_hAssaultPoint && m_hAssaultPoint->m_iStrictness); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::UpdateForceCrouch( void )
{
	if ( IsForcingCrouch() )
	{
		// Only force crouch when we're near the point we're supposed to crouch at
		float flDistanceToTargetSqr = GetOuter()->GetAbsOrigin().DistToSqr( AssaultHasBegun() ? m_hAssaultPoint->GetAbsOrigin() : m_hRallyPoint->GetAbsOrigin() );
		if ( flDistanceToTargetSqr < (64*64) )
		{
			GetOuter()->ForceCrouch();
		}
		else
		{
			GetOuter()->ClearForceCrouch();
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::IsForcingCrouch( void )
{
	if ( AssaultHasBegun() )
		return (m_hAssaultPoint && m_hAssaultPoint->m_bForceCrouch);

	return (m_hRallyPoint && m_hRallyPoint->m_bForceCrouch);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::IsUrgent( void )
{
	if ( AssaultHasBegun() )
		return (m_hAssaultPoint && m_hAssaultPoint->m_bIsUrgent);

	return (m_hRallyPoint && m_hRallyPoint->m_bIsUrgent);
}

//-----------------------------------------------------------------------------
// Purpose: Unlock any rally points the behavior is currently locking
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::UnlockRallyPoint( void )
{
	CAI_AssaultBehavior *pBehavior;
	if ( GetOuter()->GetBehavior( &pBehavior ) )
	{
		if( pBehavior->m_hRallyPoint )
		{
			pBehavior->m_hRallyPoint->Unlock( GetOuter() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRallyPoint - 
//			assaultcue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::SetParameters( CBaseEntity *pRallyEnt, AssaultCue_t assaultcue )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::SetParameters", VPROF_BUDGETGROUP_NPCS );

	// Clean up any soon to be dangling rally points
	UnlockRallyPoint();

	// Firstly, find a rally point. 
	CRallyPoint *pRallyPoint = dynamic_cast<CRallyPoint *>(pRallyEnt);

	if( pRallyPoint )
	{
		if( !pRallyPoint->IsLocked() )
		{
			// Claim it.
			m_hRallyPoint = pRallyPoint;
			m_hRallyPoint->Lock( GetOuter() );
			
			m_AssaultCue = assaultcue;
			InitializeBehavior();
			return;
		}
		else
		{
			DevMsg("**ERROR: Specified a rally point that is LOCKED!\n" );
		}
	}
	else
	{
		DevMsg("**ERROR: Bad RallyPoint in SetParameters\n" );

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		ClearSchedule( "Bad rally point" );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rallypointname - 
//			assaultcue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::SetParameters( string_t rallypointname, AssaultCue_t assaultcue, int rallySelectMethod )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::SetParameters", VPROF_BUDGETGROUP_NPCS );

	// Clean up any soon to be dangling rally points
	UnlockRallyPoint();

	// Firstly, find a rally point. 
	CRallyPoint *pRallyEnt = dynamic_cast<CRallyPoint *>(gEntList.FindEntityByName( NULL, rallypointname ) );

	CRallyPoint *pBest = NULL;
	int iBestPriority = -1;

	switch( rallySelectMethod )
	{
	case RALLY_POINT_SELECT_CLOSEST:
	case RALLY_POINT_SELECT_FURTHEST:
		{
			while( pRallyEnt )
			{
				if( !pRallyEnt->IsLocked() )
				{
					// Consider this point.
					if( pRallyEnt->m_iPriority > iBestPriority )
					{
						// This point is higher priority. I must take it.
						pBest = pRallyEnt;
						iBestPriority = pRallyEnt->m_iPriority;
					}
					else if ( pRallyEnt->m_iPriority == iBestPriority )
					{
						// This point is the same priority as my current best. 
						// I must take it if it is closer.
						Vector vecStart = GetOuter()->GetAbsOrigin();

						float flNewDist, flBestDist;

						flNewDist = ( pRallyEnt->GetAbsOrigin() - vecStart ).LengthSqr();
						flBestDist = ( pBest->GetAbsOrigin() - vecStart ).LengthSqr();

						if( ( rallySelectMethod == RALLY_POINT_SELECT_CLOSEST && flNewDist < flBestDist ) ||
							( rallySelectMethod == RALLY_POINT_SELECT_FURTHEST && flNewDist > flBestDist ) )
						{
							// Priority is already identical. Just take this point.
							pBest = pRallyEnt;
						}
					}
				}

				pRallyEnt = dynamic_cast<CRallyPoint *>(gEntList.FindEntityByName( pRallyEnt, rallypointname, NULL ) );
			}
		}
		break;

	case RALLY_POINT_SELECT_RANDOM:
		{
			// Gather all available points into a utilvector, then pick one at random.
			
			CUtlVector<CRallyPoint *> rallyPoints; // List of rally points that are available to choose from.

			while( pRallyEnt )
			{
				if( !pRallyEnt->IsLocked() )
				{
					rallyPoints.AddToTail( pRallyEnt );
				}

				pRallyEnt = dynamic_cast<CRallyPoint *>(gEntList.FindEntityByName( pRallyEnt, rallypointname ) );
			}

			if( rallyPoints.Count() > 0 )
			{
				pBest = rallyPoints[ random_valve->RandomInt(0, rallyPoints.Count()- 1) ];
			}
		}
		break;

	default:
		DevMsg( "ERROR: INVALID RALLY POINT SELECTION METHOD. Assault will not function.\n");
		break;
	}

	if( !pBest )
	{
		DevMsg("%s Didn't find a best rally point!\n", GetOuter()->GetEntityName().ToCStr() );
		return;
	}

	if ( pBest->ShouldBeLocked() )
		pBest->Lock( GetOuter() );
	m_hRallyPoint = pBest;

	if( !m_hRallyPoint )
	{
		DevMsg("**ERROR: Can't find a rally point named '%s'\n", STRING( rallypointname ));

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		ClearSchedule( "Can't find rally point" );
		return;
	}

	m_AssaultCue = assaultcue;
	InitializeBehavior();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::InitializeBehavior()
{
	// initialize the variables that track whether the NPC has reached (hit)
	// his rally and assault points already. Be advised, having hit the point
	// only means you have been to it at some point. Doesn't mean you're standing
	// there still. Mainly used to understand which 'phase' of the assault an NPC
	// is in.
	m_bHitRallyPoint = false;
	m_bHitAssaultPoint = false;

	m_hAssaultPoint = NULL;

	m_bDiverting = false;
	m_flLastSawAnEnemyAt = 0;

	// Also reset the status of externally received assault cues
	m_ReceivedAssaultCue = CUE_NO_ASSAULT;

	CAssaultPoint *pAssaultEnt = FindAssaultPoint( m_hRallyPoint->m_AssaultPointName );

	if( pAssaultEnt )
	{
		SetAssaultPoint(pAssaultEnt);
	}
	else
	{
		DevMsg("**ERROR: Can't find any assault points named: %s\n", STRING( m_hRallyPoint->m_AssaultPointName ));

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		ClearSchedule( "Can't find assault point" );
		return;
	}

	// Slam the NPC's schedule so that he starts picking Assault schedules right now.
	ClearSchedule( "Initializing assault behavior" );
}

//-----------------------------------------------------------------------------
// Purpose: Check conditions and see if the cue to being an assault has come.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::PollAssaultCue( void )
{
	// right now, always go when the commander says.
	if( m_ReceivedAssaultCue == CUE_COMMANDER )
	{
		return true;
	}

	switch( m_AssaultCue )
	{
	case CUE_NO_ASSAULT:
		// NO_ASSAULT never ever triggers.
		return false;
		break;

	case CUE_ENTITY_INPUT:
		return m_ReceivedAssaultCue == CUE_ENTITY_INPUT;
		break;

	case CUE_PLAYER_GUNFIRE:
		// Any gunfire will trigger this right now (sjb)
		if( HasCondition( COND_HEAR_COMBAT ) )
		{
			return true;
		}
		break;

	case CUE_DONT_WAIT:
		// Just keep going!
		m_ReceivedAssaultCue = CUE_DONT_WAIT;
		return true;
		break;

	case CUE_COMMANDER:
		// Player told me to go, so go!
		return m_ReceivedAssaultCue == CUE_COMMANDER;
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
		return false;

	// We're letting other AI run for a little while because the assault AI failed recently.
	if ( m_flTimeDeferScheduleSelection > gpGlobals->curtime )
		return false;

	// No schedule selection if no assault is being conducted.
	if( m_AssaultCue == CUE_NO_ASSAULT )
		return false;

	if ( !m_hAssaultPoint || !m_hRallyPoint )
	{
		Disable();
		return false;
	}

	// Remember when we last saw an enemy
	if ( GetEnemy() )
	{
		m_flLastSawAnEnemyAt = gpGlobals->curtime;
	}

	// If we've seen an enemy in the last few seconds, and we're allowed to divert,
	// let the base AI decide what I should do.
	if ( IsAllowedToDivert() )
	{
		// Return true, but remember that we're actually allowing them to divert
		// This is done because we don't want the assault behaviour to think it's finished with the assault.
		m_bDiverting = true;
	}
	else if ( m_bDiverting )
	{
		// If we were diverting, provoke us to make a new schedule selection
		SetCondition( COND_PROVOKED );

		m_bDiverting = false;
	}

	// If we're diverting, let the base AI decide everything
	if ( m_bDiverting )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::BeginScheduleSelection()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::EndScheduleSelection()
{
	m_bHitAssaultPoint = false;

	if( m_hRallyPoint != NULL )
	{
		if( !m_hRallyPoint->IsExclusive() )
			m_bHitRallyPoint = false;

		if( !m_hRallyPoint->IsExclusive() || !GetOuter()->IsAlive() )
		{
			// Here we unlock the rally point if it is NOT EXCLUSIVE
			// -OR- the Outer is DEAD. (This gives us a head-start on 
			// preparing the point to take new NPCs right away. Otherwise
			// we have to wait two seconds until the behavior is destroyed.)
			// NOTICE that the legacy (non-episodic) support calls UnlockRallyPoint
			// unconditionally on EndScheduleSelection()
			UnlockRallyPoint();
		}
	}

	GetOuter()->ClearForceCrouch();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_AssaultBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		// This nasty schedule can allow the NPC to violate their position near
		// the assault point. Translate it away to something stationary. (sjb)
		return SCHED_COMBAT_FACE;
		break;

	case SCHED_RANGE_ATTACK1:
		if ( GetOuter()->GetShotRegulator()->IsInRestInterval() )				
		{
			if ( GetOuter()->HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				GetOuter()->VacateStrategySlot();
			return SCHED_COMBAT_FACE; // @TODO (toml 07-02-03): Should do something more tactically sensible
		}
		break;

	case SCHED_MOVE_TO_WEAPON_RANGE:
	case SCHED_CHASE_ENEMY:
		if( m_bHitAssaultPoint )
		{
			return SCHED_WAIT_AND_CLEAR;
		}
		else
		{
			return SCHED_MOVE_TO_ASSAULT_POINT;
		}
		break;

	case SCHED_HOLD_RALLY_POINT:
		if( HasCondition(COND_NO_PRIMARY_AMMO) || HasCondition(COND_LOW_PRIMARY_AMMO) )
		{
			return SCHED_RELOAD;
		}
		break;

	case SCHED_MOVE_TO_ASSAULT_POINT:
		{
		float flDist = ( m_hAssaultPoint->GetAbsOrigin() - GetAbsOrigin() ).Length();
		if ( flDist <= 12.0f )
			return SCHED_AT_ASSAULT_POINT;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::OnStartSchedule( int scheduleType )
{
	if ( scheduleType == SCHED_HIDE_AND_RELOAD ) //!!!HACKHACK
	{
		// Dirty the assault point flag so that we return to assault point
		m_bHitAssaultPoint = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::ClearSchedule( const char *szReason )
{
	// HACKHACK: In reality, we shouldn't be clearing the schedule ever if the assault
	// behavior isn't actually in charge of the NPC. Fix after ship. For now, hacking
	// a fix to Grigori failing to make it over the fence of the graveyard in d1_town_02a
	if ( GetOuter()->ClassMatches( "npc_monk" ) && GetOuter()->GetState() == NPC_STATE_SCRIPT )
		return;

	// Don't allow it if we're in a vehicle
	if ( GetOuter()->IsInAVehicle() )
		return;

	GetOuter()->ClearSchedule( szReason );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::IsAllowedToDivert( void )
{
	if ( m_hAssaultPoint && m_hAssaultPoint->m_bAllowDiversion )
	{
		if ( m_hAssaultPoint->m_flAllowDiversionRadius == 0.0f || (m_bHitAssaultPoint && GetEnemy() != NULL && GetEnemy()->GetAbsOrigin().DistToSqr(m_hAssaultPoint->GetAbsOrigin()) <= Square(m_hAssaultPoint->m_flAllowDiversionRadius)) ) 
		{
			if ( m_flLastSawAnEnemyAt && ((gpGlobals->curtime - m_flLastSawAnEnemyAt) < ASSAULT_DIVERSION_TIME) )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	// If we're allowed to divert, add the appropriate interrupts to our movement schedules
	if ( IsAllowedToDivert() )
	{
		if ( IsCurSchedule( SCHED_MOVE_TO_ASSAULT_POINT ) ||
			IsCurSchedule( SCHED_MOVE_TO_RALLY_POINT ) || 
			IsCurSchedule( SCHED_HOLD_RALLY_POINT ) )
		{
			GetOuter()->SetCustomInterruptCondition( COND_NEW_ENEMY );
			GetOuter()->SetCustomInterruptCondition( COND_SEE_ENEMY );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::OnScheduleChange()
{
	if( IsCurSchedule(SCHED_WAIT_AND_CLEAR, false) )
	{
		if( m_hAssaultPoint && m_hAssaultPoint->m_bClearOnContact )
		{
			if( HasCondition(COND_SEE_ENEMY) )
			{
				ClearAssaultPoint();
			}
		}
	}

	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_AssaultBehavior::SelectSchedule()
{
	if ( !OnStrictAssault() )
	{
		if( HasCondition( COND_PLAYER_PUSHING ) )
			return SCHED_ASSAULT_MOVE_AWAY; 

		if( HasCondition( COND_HEAR_DANGER ) )
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	}

	if( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		return SCHED_MELEE_ATTACK1;

	// If you're empty, reload before trying to carry out any assault functions.
	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
		return SCHED_RELOAD;

	if( m_bHitRallyPoint && !m_bHitAssaultPoint && !AssaultHasBegun() )
	{
		// If I have hit my rally point, but I haven't hit my assault point yet, 
		// Make sure I'm still on my rally point, cause another behavior may have moved me.
		// 2D check to be within 32 units of my rallypoint.
		Vector vecDiff = GetAbsOrigin() - m_hRallyPoint->GetAbsOrigin();
		vecDiff.z = 0.0;

		if( vecDiff.LengthSqr() > Square(CUE_POINT_TOLERANCE) )
		{
			// Someone moved me away. Get back to rally point.
			m_bHitRallyPoint = false;
			return SCHED_MOVE_TO_RALLY_POINT;
		}
	}
	else if( m_bHitAssaultPoint )
	{
		// Likewise. If I have hit my assault point, make sure I'm still there. Another
		// behavior (hide and reload) may have moved me away. 
		Vector vecDiff = GetAbsOrigin() - m_hAssaultPoint->GetAbsOrigin();
		vecDiff.z = 0.0;

		if( vecDiff.LengthSqr() > Square(CUE_POINT_TOLERANCE) )
		{
			// Someone moved me away.
			m_bHitAssaultPoint = false;
		}
	}

	// Go to my rally point, unless the assault's begun.
	if( !m_bHitRallyPoint && !AssaultHasBegun() )
	{
		GetOuter()->SpeakSentence( ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_RALLY );
		return SCHED_MOVE_TO_RALLY_POINT;
	}

	if( !m_bHitAssaultPoint )
	{
		if( m_ReceivedAssaultCue == m_AssaultCue || m_ReceivedAssaultCue == CUE_COMMANDER || m_AssaultCue == CUE_DONT_WAIT )
		{
			GetOuter()->SpeakSentence( ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT );

			if ( m_hRallyPoint && !m_hRallyPoint->IsExclusive() )
			{
				// If this assault chain is not exclusive, then free up the rallypoint so that others can follow me
				// Otherwise, we do not unlock this rally point until we are FINISHED or DEAD. It's exclusively our chain of assault
				UnlockRallyPoint();// Here we go! Free up the rally point since I'm moving to assault.
			}

			if ( !UpdateForceCrouch() )
			{
				GetOuter()->ClearForceCrouch();
			}

			return SCHED_MOVE_TO_ASSAULT_POINT;
		}
		else if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			return SCHED_RANGE_ATTACK1;
		}
		else if( HasCondition( COND_NO_PRIMARY_AMMO ) )
		{
			// Don't run off to reload.
			return SCHED_RELOAD;
		}
		else if( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
		{
			GetOuter()->SpeakSentence( ASSAULT_SENTENCE_UNDER_ATTACK );
			return SCHED_ALERT_FACE;
		}
		else if( GetOuter()->GetEnemy() && !HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_CAN_RANGE_ATTACK2) && !HasCondition(COND_ENEMY_OCCLUDED) )
		{
			return SCHED_COMBAT_FACE;
		}
		else
		{
			UpdateForceCrouch();
			return SCHED_HOLD_RALLY_POINT;
		}
	}

	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		GetOuter()->SpeakSentence( ASSAULT_SENTENCE_COVER_NO_AMMO );
		return SCHED_HIDE_AND_RELOAD;
	}

	if( m_hAssaultPoint->HasSpawnFlags( SF_ASSAULTPOINT_CLEARONARRIVAL ) )
	{
		return SCHED_CLEAR_ASSAULT_POINT;
	}

	if ( (!GetEnemy() || HasCondition(COND_ENEMY_OCCLUDED)) && !GetOuter()->HasConditionsToInterruptSchedule( SCHED_WAIT_AND_CLEAR ) )
	{
		// Don't have an enemy. Just keep an eye on things.
		return SCHED_WAIT_AND_CLEAR;
	}

	if ( OnStrictAssault() )
	{
		// Don't allow the base class to select a schedule cause it will probably move the NPC.
		if( !HasCondition(COND_CAN_RANGE_ATTACK1)	&&
			!HasCondition(COND_CAN_RANGE_ATTACK2)	&&
			!HasCondition(COND_CAN_MELEE_ATTACK1)	&&
			!HasCondition(COND_CAN_MELEE_ATTACK2)	&&
			!HasCondition(COND_TOO_CLOSE_TO_ATTACK)	&&
			!HasCondition(COND_NOT_FACING_ATTACK) )
		{
			return SCHED_WAIT_AND_CLEAR;
		}
	}

#ifdef HL2_EPISODIC
	// This ugly patch fixes a bug where Combine Soldiers on an assault would not shoot through glass, because of the way
	// that shooting through glass is implemented in their AI. (sjb)
	if( HasCondition(COND_SEE_ENEMY) && HasCondition(COND_WEAPON_SIGHT_OCCLUDED) && !HasCondition(COND_LOW_PRIMARY_AMMO) )
	{	
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetOuter()->GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 )
		{
			if( GetOuter()->Classify() == CLASS_COMBINE && FClassnameIs(GetOuter(), "npc_combine_s") )
			{
				return SCHED_SHOOT_ENEMY_COVER;
			}
		}
	}
#endif//HL2_EPISODIC
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//
// CAI_AssaultGoal
//
// Purpose: 
//			
//
//-----------------------------------------------------------------------------
BEGIN_MAPENTITY( CAI_AssaultGoal, MAPENT_POINTCLASS )
	DEFINE_KEYFIELD_AUTO( m_RallyPoint, "rallypoint" ),
	DEFINE_KEYFIELD_AUTO( m_AssaultCue, "AssaultCue" ),
	DEFINE_KEYFIELD_AUTO( m_RallySelectMethod, "RallySelectMethod" ),
	DEFINE_KEYFIELD_AUTO( m_BranchMethod, "BranchMethod" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "BeginAssault", InputBeginAssault ),
END_MAPENTITY();


//-------------------------------------
LINK_ENTITY_TO_CLASS( ai_goal_assault, CAI_AssaultGoal );
//-------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_AssaultBehavior *pBehavior;

	if ( !pAI->GetBehavior( &pBehavior ) )
		return;

	pBehavior->SetGoal( this );
	pBehavior->SetParameters( m_RallyPoint, (AssaultCue_t)m_AssaultCue, m_RallySelectMethod );

	// Duplicate the output
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::DisableGoal( CAI_BaseNPC *pAI )
{ 
	CAI_AssaultBehavior *pBehavior;

	if ( pAI->GetBehavior( &pBehavior ) )
	{
		pBehavior->Disable();
	
		// Don't leave any hanging rally points locked.
		pBehavior->UnlockRallyPoint();

		pBehavior->ClearSchedule( "Assault goal disabled" );
		pBehavior->SetGoal( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: ENTITY I/O method for telling the assault behavior to cue assault
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::InputBeginAssault( inputdata_t &&inputdata )
{
	int i;

	for( i = 0 ; i < NumActors() ; i++ )
	{
		CAI_BaseNPC *pActor = GetActor( i );

		if( pActor )
		{
			// Now use this actor to lookup the Behavior
			CAI_AssaultBehavior *pBehavior;

			if( pActor->GetBehavior( &pBehavior ) )
			{
				// GOT IT! Now tell the behavior that entity i/o wants to cue the assault.
				pBehavior->ReceiveAssaultCue( CUE_ENTITY_INPUT );
			}
		}
	}
}


AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_AssaultBehavior)

	DECLARE_TASK(TASK_GET_PATH_TO_RALLY_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_FACE_RALLY_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_GET_PATH_TO_ASSAULT_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_FACE_ASSAULT_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_AWAIT_CUE, TaskParamCheck_t())
	DECLARE_TASK(TASK_AWAIT_ASSAULT_TIMEOUT, TaskParamCheck_t())
	DECLARE_TASK(TASK_ANNOUNCE_CLEAR, TaskParamCheck_t())
	DECLARE_TASK(TASK_WAIT_ASSAULT_DELAY, TaskParamCheck_t())
	DECLARE_TASK(TASK_HIT_ASSAULT_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_HIT_RALLY_POINT, TaskParamCheck_t())
	DECLARE_TASK(TASK_ASSAULT_DEFER_SCHEDULE_SELECTION, TaskParamCheck_t())
	
	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_MOVE_TO_RALLY_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_ASSAULT_FAILED_TO_MOVE)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_FAIL_MOVE_TO_RALLY_POINT)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_HOLD_RALLY_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_HOLD_ASSAULT_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_MOVE_TO_ASSAULT_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_AT_ASSAULT_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_WAIT_AND_CLEAR)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_CLEAR_ASSAULT_POINT)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE_FILE(SCHED_ASSAULT_MOVE_AWAY)

AI_END_CUSTOM_SCHEDULE_PROVIDER()
