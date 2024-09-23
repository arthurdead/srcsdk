//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#include "cbase.h"

#include "bitstring.h"

#include "ai_tacticalservices.h"
#include "ai_basenpc.h"
#include "ai_moveprobe.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_find_lateral_cover( "ai_find_lateral_cover", "1" );
ConVar ai_find_lateral_los( "ai_find_lateral_los", "1" );

#ifdef _DEBUG
ConVar ai_debug_cover( "ai_debug_cover", "0" );

#define DebugFindCover( from, to, r, g, b ) \
	if ( !ai_debug_cover.GetBool() || \
		 !GetOuter()->m_bSelected ) \
		; \
	else \
		NDebugOverlay::Line( from, to, r, g, b, false, 1 )

#define DebugFindCover2( from, to, r, g, b ) \
	if ( ai_debug_cover.GetInt() < 2 || \
		 !GetOuter()->m_bSelected ) \
		; \
	else \
		NDebugOverlay::Line( from, to, r, g, b, false, 1 )

ConVar ai_debug_tactical_los( "ai_debug_tactical_los", "0" );

#define ShouldDebugLos() ( ai_debug_tactical_los.GetBool() && GetOuter()->m_bSelected )

#else
#define DebugFindCover( from, to, r, g, b ) ((void)0)
#define DebugFindCover2( from, to, r, g, b ) ((void)0)
#define ShouldDebugLos() false
#endif

//-------------------------------------

void CAI_TacticalServices::Init()
{
	m_pPathfinder = GetOuter()->GetPathfinder();
	Assert( m_pPathfinder );
}
	
//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam, Vector *pResult)
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLos );

	MARK_TASK_EXPENSIVE();

	Vector pos = FindLosPos( threatPos, threatEyePos, 
											 minThreatDist, maxThreatDist, 
											 blockTime, eFlankType, vecFlankRefPos, flFlankParam );
	
	if (pos == vec3_origin)
		return false;

	*pResult = pos;

	return true;
}

//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, Vector *pResult)
{
	return FindLos( threatPos, threatEyePos, minThreatDist, maxThreatDist, blockTime, FLANKTYPE_NONE, vec3_origin, 0, pResult );
}

//-------------------------------------

bool CAI_TacticalServices::FindBackAwayPos( const Vector &vecThreat, Vector *pResult )
{
	MARK_TASK_EXPENSIVE();

	Vector vMoveAway = GetAbsOrigin() - vecThreat;
	vMoveAway.NormalizeInPlace();

	if ( GetOuter()->GetNavigator()->FindVectorGoal( pResult, vMoveAway, 10*12, 10*12, true ) )
		return true;

	Vector pos = FindBackAwayPos( vecThreat );
	
	if (pos != vec3_origin)
	{
		*pResult = pos;
		return true;
	}

	if ( GetOuter()->GetNavigator()->FindVectorGoal( pResult, vMoveAway, GetHullWidth() * 4, GetHullWidth() * 2, true ) )
		return true;

	return false;
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	return FindCoverPos( GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist, pResult );
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos( const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindCoverPos );

	MARK_TASK_EXPENSIVE();

	Vector pos = FindCoverPos( vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist );
	
	if (pos == vec3_origin)
		return false;

	*pResult = pos;

	return true;
}

//-------------------------------------
// Checks lateral cover
//-------------------------------------
bool CAI_TacticalServices::TestLateralCover( const Vector &vecCheckStart, const Vector &vecCheckEnd, float flMinDist )
{
	trace_t	tr;

	if ( (vecCheckStart - vecCheckEnd).LengthSqr() > Square(flMinDist) )
	{
		if (GetOuter()->IsCoverPosition(vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset()))
		{
			if ( GetOuter()->IsValidCover ( vecCheckEnd ) )
			{
				AIMoveTrace_t moveTrace;
				GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, GetOuter()->GetAITraceMask(), NULL, &moveTrace );
				if (moveTrace.fStatus == AIMR_OK)
				{
					DebugFindCover( vecCheckEnd + GetOuter()->GetViewOffset(), vecCheckStart, 0, 255, 0 );
					return true;
				}
			}
		}
	}

	DebugFindCover( vecCheckEnd + GetOuter()->GetViewOffset(), vecCheckStart, 255, 0, 0 );

	return false;
}


//-------------------------------------
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//-------------------------------------

#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks
bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, Vector *pResult )
{
	return FindLateralCover( vecThreat, flMinDist, COVER_CHECKS * COVER_DELTA, COVER_CHECKS, pResult );
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	return FindLateralCover( GetAbsOrigin(), vecThreat, flMinDist, distToCheck, numChecksPerDir, pResult );
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vNearPos, const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLateralCover );

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	int		i;

	if ( TestLateralCover( vecThreat, vNearPos, flMinDist ) )
	{
		*pResult = GetLocalOrigin();
		return true;
	}

	if( !ai_find_lateral_cover.GetBool() )
	{
		// Force the NPC to use the nodegraph to find cover. NOTE: We let the above code run
		// to detect the case where the NPC may already be standing in cover, but we don't 
		// make any additional lateral checks.
		return false;
	}

	Vector right =  vecThreat - vNearPos;
	float temp;

	right.z = 0;
	VectorNormalize( right );
	temp = right.x;
	right.x = -right.y;
	right.y = temp;

	vecStepRight = right * (distToCheck / (float)numChecksPerDir);
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = vNearPos;
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < numChecksPerDir ; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralCover( vecCheckStart, vecLeftTest, flMinDist ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralCover( vecCheckStart, vecRightTest, flMinDist ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}

//-------------------------------------
// Purpose: Find a nearby node that further away from the enemy than the
//			min range of my current weapon if there is one or just futher
//			away than my current location if I don't have a weapon.  
//			Used to back away for attacks
//-------------------------------------
Vector CAI_TacticalServices::FindBackAwayPos(const Vector &vecThreat )
{
	if ( !RecastMgr().HasMeshes() )
	{
		DevWarning( 2, "Graph not ready for FindBackAwayNode!\n" );
		return vec3_origin;
	}

	//TODO!!!! Arthurdead
	//DebuggerBreak();
	Assert(0);
	return vec3_origin;
}

//-------------------------------------
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy. 
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//-------------------------------------

Vector CAI_TacticalServices::FindCoverPos(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	return FindCoverPos(GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist );
}

//-------------------------------------
Vector CAI_TacticalServices::FindCoverPos(const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	if ( !RecastMgr().HasMeshes() )
		return vec3_origin;

	AI_PROFILE_SCOPE( CAI_TacticalServices_FindCoverNode );

	MARK_TASK_EXPENSIVE();

	DebugFindCover( GetOuter()->EyePosition(), vThreatEyePos, 0, 255, 255 );

	if ( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5 * flMaxDist)
	{
		flMinDist = 0.5 * flMaxDist;
	}

	//TODO!!!! Arthurdead
	//DebuggerBreak();
	Assert(0);
	return vec3_origin;
}

//-------------------------------------
// Purpose:  Return node ID that has line of sight to target I want to shoot
//
// Input  :	pNPC			- npc that's looking for a place to shoot from
//			vThreatPos		- position of entity/location I'm trying to shoot
//			vThreatEyePos	- eye position of entity I'm trying to shoot. If 
//							  entity has no eye position, just give vThreatPos again
//			flMinThreatDist	- minimum distance that node must be from vThreatPos
//			flMaxThreadDist	- maximum distance that node can be from vThreadPos
//			vThreatFacing	- optional argument.  If given the returned node
//							  will also be behind the given facing direction (flanking)
//			flBlockTime		- how long to block this node from use
// Output :	int				- ID number of node that meets qualifications
//-------------------------------------
Vector CAI_TacticalServices::FindLosPos(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam )
{
	if ( !RecastMgr().HasMeshes() )
		return vec3_origin;

	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLosNode );

	MARK_TASK_EXPENSIVE();

	//TODO!!!! Arthurdead
	//DebuggerBreak();
	Assert(0);
	return vec3_origin;

	// We failed.  No range attack node node was found
	return vec3_origin;
}

//-------------------------------------
// Checks lateral LOS
//-------------------------------------
bool CAI_TacticalServices::TestLateralLos( const Vector &vecCheckStart, const Vector &vecCheckEnd )
{
	trace_t	tr;

	// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
	AI_TraceLOS(  vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset(), NULL, &tr );

	if (tr.fraction == 1.0)
	{
		if ( GetOuter()->IsValidShootPosition( vecCheckEnd ) )
		{
			if (GetOuter()->TestShootPosition(vecCheckEnd,vecCheckStart))
			{
				AIMoveTrace_t moveTrace;
				GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, GetOuter()->GetAITraceMask(), NULL, &moveTrace );
				if (moveTrace.fStatus == AIMR_OK)
				{
					return true;
				}
			}
		}
	}

	return false;
}


//-------------------------------------

bool CAI_TacticalServices::FindLateralLos( const Vector &vecThreat, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLateralLos );

	if( !m_bAllowFindLateralLos )
	{
		return false;
	}

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	bool	bLookingForEnemy = GetEnemy() && VectorsAreEqual(vecThreat, GetEnemy()->EyePosition(), 0.1f);
	int		i;

	if(  !bLookingForEnemy || GetOuter()->HasCondition(COND_SEE_ENEMY) || GetOuter()->HasCondition(COND_HAVE_ENEMY_LOS) || 
		 GetOuter()->GetTimeScheduleStarted() == gpGlobals->curtime ) // Conditions get nuked before tasks run, assume should try
	{
		// My current position might already be valid.
		if ( TestLateralLos(vecThreat, GetLocalOrigin()) )
		{
			*pResult = GetLocalOrigin();
			return true;
		}
	}

	if( !ai_find_lateral_los.GetBool() )
	{
		// Allows us to turn off lateral LOS at the console. Allow the above code to run 
		// just in case the NPC has line of sight to begin with.
		return false;
	}

	int iChecks = COVER_CHECKS;
	int iDelta = COVER_DELTA;

	// If we're limited in how far we're allowed to move laterally, don't bother checking past it
	int iMaxLateralDelta = GetOuter()->GetMaxTacticalLateralMovement();
	if ( iMaxLateralDelta != MAXTACLAT_IGNORE && iMaxLateralDelta < iDelta )
	{
		iChecks = 1;
		iDelta = iMaxLateralDelta;
	}

	Vector right;
	AngleVectors( GetLocalAngles(), NULL, &right, NULL );
	vecStepRight = right * iDelta;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = GetLocalOrigin();
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < iChecks; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralLos( vecCheckStart, vecLeftTest ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralLos( vecCheckStart, vecRightTest ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}
