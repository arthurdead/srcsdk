//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ndebugoverlay.h"

#include "ai_pathfinder.h"

#include "ai_basenpc.h"
#include "ai_waypoint.h"
#include "ai_routedist.h"
#include "ai_moveprobe.h"
#include "bitstring.h"
#include "ai_localnavigator.h"

//@todo: bad dependency!
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NUM_NPC_DEBUG_OVERLAYS	  50

const float MAX_LOCAL_NAV_DIST_GROUND[2] = { (50*12), (25*12) };
const float MAX_LOCAL_NAV_DIST_FLY[2] = { (750*12), (750*12) };

//-----------------------------------------------------------------------------
// CAI_Pathfinder
//

//-----------------------------------------------------------------------------
// Compute move type bits to nav type
//-----------------------------------------------------------------------------
Navigation_t MoveBitsToNavType( int fBits )
{
	if (fBits & bits_CAP_MOVE_FLY)
		return NAV_FLY;

	if (fBits & bits_CAP_MOVE_CLIMB)
		return NAV_CLIMB;

	if (fBits & bits_CAP_MOVE_JUMP)
		return NAV_JUMP;

	if (fBits & bits_CAP_MOVE_CRAWL)
		return NAV_CRAWL;

	if (fBits & bits_CAP_MOVE_GROUND)
		return NAV_GROUND;

	return NAV_NONE;
}

int NavTypeToMoveBits( Navigation_t nNavType )
{
	switch (nNavType)
	{
	case NAV_GROUND:
		return bits_CAP_MOVE_GROUND;

	case NAV_FLY:
		return bits_CAP_MOVE_FLY;

	case NAV_CLIMB:
		return bits_CAP_MOVE_CLIMB;

	case NAV_JUMP:
		return bits_CAP_MOVE_JUMP;

	case NAV_CRAWL:
		return bits_CAP_MOVE_CRAWL;

	default:
		return 0;
	}
}

//-----------------------------------------------------------------------------

void CAI_Pathfinder::Init()
{
}
	
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::UseStrongOptimizations()
{
	if ( !AIStrongOpt() )
	{
		return false;
	}

#ifdef HL2_DLL
	if( GetOuter()->Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		return false;
	}
#endif//HL2_DLL
	return true;
}

dtPolyRef CAI_Pathfinder::NearestPolyToNPC()
{
	CRecastMesh *pMesh = GetNavMesh();
	if(!pMesh)
		return 0;

	return pMesh->GetPolyRef( GetOuter()->GetAbsOrigin() );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
dtPolyRef CAI_Pathfinder::NearestPolyToPoint( const Vector &vecOrigin )
{
	CRecastMesh *pMesh = GetNavMesh();
	if(!pMesh)
		return 0;

	return pMesh->GetPolyRef( vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: Find a short random path of at least pathLength distance.  If
//			vDirection is given random path will expand in the given direction,
//			and then attempt to go generally straight
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::FindShortRandomPath(const Vector &vStart, float minPathLength, const Vector &directionIn) 
{
	//TODO!!!! Arthurdead
	Assert(0);
	DebuggerBreak();
	return NULL;
}

//-----------------------------------------------------------------------------

static int NPCBuildFlags( CAI_BaseNPC *pNPC, const Vector &vecOrigin )
{
	// If vecOrigin the the npc's position and npc is climbing only climb nodes allowed
	if (pNPC->GetLocalOrigin() == vecOrigin && pNPC->GetNavType() == NAV_CLIMB) 
	{
		return bits_BUILD_CLIMB;
	}
	else if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
	{
		return bits_BUILD_FLY | bits_BUILD_GIVEWAY;
	}
	else if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_GROUND)
	{
		int buildFlags = bits_BUILD_GROUND | bits_BUILD_GIVEWAY;
		if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_JUMP)
		{
			buildFlags |= bits_BUILD_JUMP;
		}
		if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_CRAWL)
		{
			buildFlags |= bits_BUILD_CRAWL;
		}

		return buildFlags;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a route to a node for the given npc with the given 
//			build flags
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::RouteTo(const Vector &vecOrigin, int buildFlags, const Vector &vEnd, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_RouteToNode );
	
	buildFlags |= NPCBuildFlags( GetOuter(), vecOrigin );
	buildFlags &= ~bits_BUILD_GET_CLOSE;

	// Check if vecOrigin is already at the smallest node

	// FIXME: an equals check is a bit sloppy, this should be a tolerance
	if (CloseEnough(vecOrigin, vEnd))
	{
		return new AI_Waypoint_t( vecOrigin, 0.0f, NAV_GROUND, bits_WP_TO_GOAL );
	}

	// Otherwise try to build a local route to the node
	AI_Waypoint_t *pResult = BuildLocalRoute(vecOrigin, vEnd, NULL, 0, buildFlags, goalTolerance);
	return pResult;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a route to a node for the given npc with the given 
//			build flags
//-----------------------------------------------------------------------------

AI_Waypoint_t* CAI_Pathfinder::RouteFrom(const Vector &vecOrigin, int buildFlags, const Vector &vEnd, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_RouteFromNode );
	
	buildFlags |= NPCBuildFlags( GetOuter(), vecOrigin );
	buildFlags |= bits_BUILD_GET_CLOSE;

	// Check if vecOrigin is already at the smallest node
	// FIXME: an equals check is a bit sloppy, this should be a tolerance
	if (CloseEnough(vecOrigin, vEnd))
	{
		return new AI_Waypoint_t( vEnd, 0.0f, NAV_GROUND, bits_WP_TO_GOAL );
	}

	// Otherwise try to build a local route from the node
	AI_Waypoint_t* pResult = BuildLocalRoute( vEnd, vecOrigin, NULL, bits_WP_TO_GOAL, buildFlags, goalTolerance);

	// Handle case of target hanging over edge near climb dismount
#if 0
	if ( !pResult &&
		 GetAreaType(area) == AREA_CLIMB && 
		 ( vecOrigin - vEnd ).Length2DSqr() < 32.0*32.0 &&
		 GetOuter()->GetMoveProbe()->CheckStandPosition(vEnd, GetOuter()->GetAITraceMask_BrushOnly() ) )
	{
		pResult = new AI_Waypoint_t( vecOrigin, 0, NAV_GROUND, bits_WP_TO_GOAL, area );
	}
#endif

	return pResult;
}

//-----------------------------------------------------------------------------
// Builds a simple route (no triangulation, no making way)
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildSimpleRoute( Navigation_t navType, const Vector &vStart, 
	const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, AreaType_e areaTargetType, float flYaw )
{
	Assert( navType == NAV_JUMP || navType == NAV_CLIMB || navType == NAV_CRAWL); // this is what this here function is for
	// Only allowed to jump to ground nodes

	AIMoveTrace_t moveTrace;
	GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, GetOuter()->GetAITraceMask(), pTarget, &moveTrace );

	// If I was able to make the move, or the vEnd is the
	// goal and I'm within tolerance, just move to vEnd
	if (!IsMoveBlocked(moveTrace))
	{
		// It worked so return a route of length one to the endpoint
		return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Builds a complex route (triangulation, making way)
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
	const Vector &vEnd, const CBaseEntity *pTarget, int endFlags,
	int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance )
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildComplexRoute );
	
	float flTotalDist = ComputePathDistance( navType, vStart, vEnd );
	if ( flTotalDist < 0.0625 )
	{
		return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );
	}
	
	unsigned int collideFlags = (buildFlags & bits_BUILD_IGNORE_NPCS) ? GetOuter()->GetAITraceMask_BrushOnly() : GetOuter()->GetAITraceMask();

	bool bCheckGround = (GetOuter()->CapabilitiesGet() & bits_CAP_SKIP_NAV_GROUND_CHECK) ? false : true;

	if ( flTotalDist <= maxLocalNavDistance || ( buildFlags & bits_BUILD_UNLIMITED_DISTANCE ) )
	{
		AIMoveTrace_t moveTrace;

		AI_PROFILE_SCOPE_BEGIN( CAI_Pathfinder_BuildComplexRoute_Direct );
	
		GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, collideFlags, pTarget, (bCheckGround) ? 100 : 0, &moveTrace);

		// If I was able to make the move...
		if (!IsMoveBlocked(moveTrace))
		{
			// It worked so return a route of length one to the endpoint
			return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );
		}
	
		// ...or the vEnd is thegoal and I'm within tolerance, just move to vEnd
		if ( (buildFlags & bits_BUILD_GET_CLOSE) && 
			 (endFlags & bits_WP_TO_GOAL) && 
			 moveTrace.flDistObstructed <= goalTolerance ) 
		{
			return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );
		}

		AI_PROFILE_SCOPE_END();

		// -------------------------------------------------------------------
		// Try to triangulate if requested
		// -------------------------------------------------------------------

		AI_PROFILE_SCOPE_BEGIN( CAI_Pathfinder_BuildComplexRoute_Triangulate );
		
		if (buildFlags & bits_BUILD_TRIANG)
		{
			if ( !UseStrongOptimizations() || ( GetOuter()->GetState() == NPC_STATE_SCRIPT || GetOuter()->IsCurSchedule( SCHED_SCENE_GENERIC, false ) ) )
			{
				float flTotalDist = ComputePathDistance( navType, vStart, vEnd );

				AI_Waypoint_t *triangRoute = BuildTriangulationRoute(vStart, vEnd, pTarget, 
					endFlags, flYaw, flTotalDist - moveTrace.flDistObstructed, navType);

				if (triangRoute)
				{
					return triangRoute;
				}
			}
		}
		
		AI_PROFILE_SCOPE_END();

		// -------------------------------------------------------------------
		// Try to giveway if requested
		// -------------------------------------------------------------------
		if (moveTrace.fStatus == AIMR_BLOCKED_NPC && (buildFlags & bits_BUILD_GIVEWAY))
		{
			// If I can't get there even ignoring NPCs, don't bother to request a giveway
			AIMoveTrace_t moveTrace2;
			GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, GetOuter()->GetAITraceMask_BrushOnly(), pTarget, (bCheckGround) ? 100 : 0, &moveTrace2 );

			if (!IsMoveBlocked(moveTrace2))
			{							
				// If I can clear the way return a route of length one to the target location
				if ( CanGiveWay(vStart, vEnd, moveTrace.pObstruction) )
				{
					return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );
				}
			}
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a jump route between vStart
//			and vEnd, ignoring entity pTarget
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Attempts to build a crawl route between vStart
//			and vEnd, ignoring entity pTarget
// Input  :
// Output : Returns a route if successful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildCrawlRoute(const Vector &vStart, const Vector &vEnd, 
											   const CBaseEntity *pTarget, int endFlags, int buildFlags, float flYaw, float goalTolerance)
{
	// Only allowed to jump to ground nodes
	//return BuildSimpleRoute( NAV_CRAWL, vStart, vEnd, pTarget, 
	//	endFlags, nodeID, NODE_GROUND, flYaw );
	return BuildComplexRoute( NAV_CRAWL, vStart, vEnd, pTarget, 
		endFlags, buildFlags, flYaw, goalTolerance, MAX_LOCAL_NAV_DIST_GROUND[UseStrongOptimizations()] );
}

AI_Waypoint_t *CAI_Pathfinder::BuildJumpRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int buildFlags, float flYaw)
{
	Vector vecDiff = vStart - vEnd;

	if( fabs(vecDiff.z) <= 24.0f && vecDiff.Length2D() <= Square(600.0f) )
		return NULL;

	// Only allowed to jump to ground nodes
	return BuildSimpleRoute( NAV_JUMP, vStart, vEnd, pTarget, 
		endFlags, AREA_GROUND, flYaw );
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a climb route between vStart
//			and vEnd, ignoring entity pTarget
// Input  :
// Output : Returns a route if sucessful or NULL if no climb route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildClimbRoute(const Vector &vStart, const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int buildFlags, float flYaw)
{
	// Only allowed to climb to climb nodes
	return BuildSimpleRoute( NAV_CLIMB, vStart, vEnd, pTarget, 
		endFlags, AREA_CLIMB, flYaw );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a ground route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if sucessful or NULL if no ground route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildGroundRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int buildFlags, float flYaw, float goalTolerance)
{
	return BuildComplexRoute( NAV_GROUND, vStart, vEnd, pTarget, 
		endFlags, buildFlags, flYaw, goalTolerance, MAX_LOCAL_NAV_DIST_GROUND[UseStrongOptimizations()] );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a fly route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if sucessful or NULL if no ground route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildFlyRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int buildFlags, float flYaw, float goalTolerance)
{
	return BuildComplexRoute( NAV_FLY, vStart, vEnd, pTarget, 
		endFlags, buildFlags, flYaw, goalTolerance, MAX_LOCAL_NAV_DIST_FLY[UseStrongOptimizations()] );
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a route between vStart and vEnd, requesting the
//			pNPCBlocker to get out of the way
// Input  :
// Output : Returns a route if sucessful or NULL if giveway failed
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CanGiveWay( const Vector& vStart, const Vector& vEnd, CBaseEntity *pBlocker)
{
	// FIXME: make this a CAI_BaseNPC member function
	CAI_BaseNPC *pNPCBlocker = pBlocker->MyNPCPointer();
	if (pNPCBlocker && pNPCBlocker->edict()) 
	{
		Disposition_t eDispBlockerToMe = pNPCBlocker->IRelationType( GetOuter() );
		if ( ( eDispBlockerToMe == D_LI ) || ( eDispBlockerToMe == D_NU ) )
		{
			return true;
		}
		
		return false;

		// FIXME: this is called in route creation, not navigation.  It shouldn't actually make
		// anyone get out of their way, just see if they'll honor the request.
		// things like locked doors, enemies and such should refuse, all others shouldn't.
		// things like breakables should know who is trying to break them, though a door hidden behind
		// some boxes shouldn't be known to the AI even though a route should connect through them but
		// be turned off.

		/*
		Vector moveDir	   = (vEnd - vStart).Normalize();
		Vector blockerDir  = (pNPCBlocker->GetLocalOrigin()  - vStart);
		float  blockerDist = DotProduct(moveDir,blockerDir);
		Vector blockPos	   = vStart + (moveDir*blockerDist);

		if (pNPCBlocker->RequestGiveWay ( m_owner->GetLocalOrigin(), blockPos, moveDir, m_owner->m_eHull))
		{
			return true;
		}
		*/
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a triangulation route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance and
//			triangulating around a blocking object at blockDist
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildTriangulationRoute(
	  const Vector &vStart, // from where
	  const Vector &vEnd,	// to where
	  const CBaseEntity *pTarget,	// an entity I can ignore
	  int endFlags,			// add these WP flags to the last waypoint
	  float flYaw,			// ideal yaw for the last waypoint
	  float flDistToBlocker,// how far away is the obstruction from the start?
	  Navigation_t navType)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildTriangulationRoute );
	
	Vector vApex;
	if (!Triangulate(navType, vStart, vEnd, flDistToBlocker, pTarget, &vApex ))
		return NULL;

	//-----------------------------------------------------------------------------
	// it worked, create a route
	//-----------------------------------------------------------------------------
	AI_Waypoint_t *pWayPoint2 = new AI_Waypoint_t( vEnd, flYaw, navType, endFlags );

	// FIXME: Compute a reasonable yaw here
	AI_Waypoint_t *waypoint1 = new AI_Waypoint_t( vApex, 0, navType, bits_WP_TO_DETOUR );

	waypoint1->SetNext(pWayPoint2);

	return waypoint1;
}

//-----------------------------------------------------------------------------
// Purpose: Get the next node (with wrapping) around a circularly wound path
// Input  : nLastNode - The starting node
//			nDirection - Direction we're moving
//			nNumNodes - Total nodes in the chain
//-----------------------------------------------------------------------------
inline int GetNextPoint( int nLastNode, int nDirection, int nNumNodes )
{
	int nNextNode = nLastNode + nDirection;
	if ( nNextNode > (nNumNodes-1) )
		nNextNode = 0;
	else if ( nNextNode < 0 )
		nNextNode = (nNumNodes-1);

	return nNextNode;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to wind a route through a series of node points in a specified direction.
// Input  : *vecCorners - Points to test between
//			nNumCorners - Number of points to test
//			&vecStart - Starting position
//			&vecEnd - Ending position
// Output : Route through the points
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildRouteThroughPoints( Vector *vecPoints, int nNumPoints, int nDirection, int nStartIndex, int nEndIndex, Navigation_t navType, CBaseEntity *pTarget )
{
	AIMoveTrace_t endTrace;
	endTrace.fStatus = AIMR_OK;

	CAI_MoveProbe *pMoveProbe = GetOuter()->GetMoveProbe();

	AI_Waypoint_t *pFirstRoute = NULL;
	AI_Waypoint_t *pHeadRoute = NULL;

	int nCurIndex = nStartIndex;
	int nNextIndex;

	// FIXME: Must be able to move to the first position (these needs some parameterization) 
	pMoveProbe->MoveLimit( navType, GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], GetOuter()->GetAITraceMask(), pTarget, &endTrace );
	if ( IsMoveBlocked( endTrace ) )
	{
		// NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], 8.0f, 255, 0, 0, 0, true, 4.0f );
		return NULL;
	}

	// NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], 8.0f, 0, 255, 0, 0, true, 4.0f );

	int nRunAwayCount = 0;
	while ( nRunAwayCount++ < nNumPoints )
	{
		// Advance our index in the specified direction
		nNextIndex = GetNextPoint( nCurIndex, nDirection, nNumPoints );

		// Try and build a local route between the current and next point
		pMoveProbe->MoveLimit( navType, vecPoints[nCurIndex], vecPoints[nNextIndex], GetOuter()->GetAITraceMask(), pTarget, &endTrace );
		if ( IsMoveBlocked( endTrace ) )
		{
			// TODO: Triangulate here if we failed?

			// We failed, so give up
			if ( pHeadRoute )
			{
				DeleteAll( pHeadRoute );
			}

			// NDebugOverlay::HorzArrow( vecPoints[nCurIndex], vecPoints[nNextIndex], 8.0f, 255, 0, 0, 0, true, 4.0f );
			return NULL;
		}

		// NDebugOverlay::HorzArrow( vecPoints[nCurIndex], vecPoints[nNextIndex], 8.0f, 0, 255, 0, 0, true, 4.0f );

		if ( pHeadRoute == NULL )
		{
			// Start a new route head
			pFirstRoute = pHeadRoute = new AI_Waypoint_t( vecPoints[nCurIndex], 0.0f, navType, bits_WP_TO_DETOUR );
		}
		else
		{
			// Link a new waypoint into the path
			AI_Waypoint_t *pNewNode = new AI_Waypoint_t( vecPoints[nCurIndex], 0.0f, navType, bits_WP_TO_DETOUR|bits_WP_DONT_SIMPLIFY );
			pHeadRoute->SetNext( pNewNode );
			pHeadRoute = pNewNode;
		}

		// See if we're done
		if ( nNextIndex == nEndIndex )
		{
			AI_Waypoint_t *pNewNode = new AI_Waypoint_t( vecPoints[nEndIndex], 0.0f, navType, bits_WP_TO_DETOUR );
			pHeadRoute->SetNext( pNewNode );
			pHeadRoute = pNewNode;
			break;
		}

		// Advance one node
		nCurIndex = nNextIndex;
	}

	return pFirstRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Find the closest point in a list of points, to a specified position
// Input  : &vecPosition - Position to test against
//			*vecPoints - List of vectors we'll check
//			nNumPoints - Number of points in the list
// Output : Index to the closest point in the list
//-----------------------------------------------------------------------------
inline int ClosestPointToPosition( const Vector &vecPosition, Vector *vecPoints, int nNumPoints )
{
	int   nBestNode = -1;
	float flBestDistSqr = FLT_MAX;
	float flDistSqr;
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistSqr = ( vecPoints[i] - vecPosition ).LengthSqr();
		if ( flDistSqr < flBestDistSqr )
		{
			flBestDistSqr = flDistSqr;
			nBestNode = i;
		}
	}

	return nBestNode;
}

//-----------------------------------------------------------------------------
// Purpose: Find which winding through a circular list is shortest in physical distance travelled
// Input  : &vecStart - Where we started from
//			nStartPoint - Starting index into the points
//			nEndPoint - Ending index into the points
//			nNumPoints - Number of points in the list
//			*vecPoints - List of vectors making up a list of points
//-----------------------------------------------------------------------------
inline int ShortestDirectionThroughPoints( const Vector &vecStart, int nStartPoint, int nEndPoint, Vector *vecPoints, int nNumPoints )
{
	const int nClockwise = 1;
	const int nCounterClockwise = -1;

	// Find the quickest direction around the object
	int nCurPoint = nStartPoint;
	int nNextPoint = GetNextPoint( nStartPoint, 1, nNumPoints );

	float flStartDistSqr = ( vecStart - vecPoints[nStartPoint] ).LengthSqr();
	float flDistanceSqr = flStartDistSqr;

	// Try going clockwise first
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistanceSqr += ( vecPoints[nCurPoint] - vecPoints[nNextPoint] ).LengthSqr();

		if ( nNextPoint == nEndPoint )
			break;

		nNextPoint = GetNextPoint( nNextPoint, 1, nNumPoints );
	}

	// Save this to test against
	float flBestDistanceSqr = flDistanceSqr;

	// Start from the beginning again
	flDistanceSqr = flStartDistSqr;

	nCurPoint = nStartPoint;
	nNextPoint = GetNextPoint( nStartPoint, -1, nNumPoints );

	// Now go the other way and see if it's shorter to do so
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistanceSqr += ( vecPoints[nCurPoint] - vecPoints[nNextPoint] ).LengthSqr();

		// We've gone over our maximum so we can't be shorter
		if ( flDistanceSqr > flBestDistanceSqr )
			break;

		// We hit the end, we're shorter
		if ( nNextPoint == nEndPoint )
			return nCounterClockwise;

		nNextPoint = GetNextPoint( nNextPoint, -1, nNumPoints );
	}

	return nClockwise;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to build an avoidance route around an object using its OBB
//			Currently this function is meant for NPCs moving around a vehicle, 
//			and is very specialized as such
//
// Output : Returns a route if successful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildOBBAvoidanceRoute(	const Vector &vStart, const Vector &vEnd,
														const CBaseEntity *pObstruction, // obstruction to avoid
														const CBaseEntity *pTarget,		 // target to ignore
														Navigation_t navType )
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildOBBAvoidanceRoute );

	// If the point we're navigating to is within our OBB, then fail
	// TODO: We could potentially also just try to get as near as possible
	if ( pObstruction->CollisionProp()->IsPointInBounds( vEnd ) )
		return NULL;

	// Find out how much we'll need to inflate the collision bounds to let us move past
	Vector vecSize = pObstruction->CollisionProp()->OBBSize();
	float flWidth = GetOuter()->GetHullWidth() * 0.5f;

	float flWidthPercX = ( flWidth / vecSize.x );
	float flWidthPercY = ( flWidth / vecSize.y );

	// Find the points around the object, bloating it by our hull width
	// The ordering of these corners wind clockwise around the object, starting at the top left
	Vector vecPoints[4];
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector(  -flWidthPercX, 1+flWidthPercY, 0.25f ), &vecPoints[0] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector( 1+flWidthPercX, 1+flWidthPercY, 0.25f ), &vecPoints[1] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector( 1+flWidthPercX,  -flWidthPercY, 0.25f ), &vecPoints[2] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector(  -flWidthPercX,  -flWidthPercY, 0.25f ), &vecPoints[3] );

	// Find the two points nearest our goals
	int nStartPoint = ClosestPointToPosition( vStart, vecPoints, ARRAYSIZE( vecPoints ) );
	int nEndPoint = ClosestPointToPosition( vEnd, vecPoints, ARRAYSIZE( vecPoints ) );

	// We won't be able to build a route if we're moving no distance between points
	if ( nStartPoint == nEndPoint )
		return NULL;

	// Find the shortest path around this wound polygon (direction is how to step through array)
	int nDirection = ShortestDirectionThroughPoints( vStart, nStartPoint, nEndPoint, vecPoints, ARRAYSIZE( vecPoints ) );

	// Attempt to build a route in our direction
	AI_Waypoint_t *pRoute = BuildRouteThroughPoints( vecPoints, ARRAYSIZE(vecPoints), nDirection, nStartPoint, nEndPoint, navType, (CBaseEntity *) pTarget );
	if ( pRoute == NULL )
	{
		// Failed that way, so try the opposite
		pRoute = BuildRouteThroughPoints( vecPoints, ARRAYSIZE(vecPoints), (-nDirection), nStartPoint, nEndPoint, navType, (CBaseEntity *) pTarget );
		if ( pRoute == NULL )
			return NULL;
	}

	return pRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a local route (not using nodes) between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if successful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildLocalRoute(const Vector &vStart, const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int buildFlags, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildLocalRoute );

	// Get waypoint yaw
	float flYaw = 0;

	// Try a ground route if requested
	if (buildFlags & bits_BUILD_GROUND)
	{
		AI_Waypoint_t *groundRoute = BuildGroundRoute(vStart,vEnd,pTarget,endFlags,buildFlags,flYaw,goalTolerance);

		if (groundRoute)
		{
			return groundRoute;
		}
	}

	// Try a fly route if requested
	if ( buildFlags & bits_BUILD_FLY )
	{
		AI_Waypoint_t *flyRoute = BuildFlyRoute(vStart,vEnd,pTarget,endFlags,buildFlags,flYaw,goalTolerance);

		if (flyRoute)
		{
			return flyRoute;
		}
	}

	// Try a crawl route if NPC can crawl and requested
	if ((buildFlags & bits_BUILD_CRAWL) && (CapabilitiesGet() & bits_CAP_MOVE_CRAWL))
	{
		AI_Waypoint_t *crawlRoute = BuildCrawlRoute(vStart,vEnd,pTarget,endFlags,buildFlags,flYaw,goalTolerance);

		if (crawlRoute)
		{
			return crawlRoute;
		}
	}

	// Try a jump route if NPC can jump and requested
	if ((buildFlags & bits_BUILD_JUMP) && (CapabilitiesGet() & bits_CAP_MOVE_JUMP))
	{
		AI_Waypoint_t *jumpRoute = BuildJumpRoute(vStart,vEnd,pTarget,endFlags,buildFlags,flYaw);

		if (jumpRoute)
		{
			return jumpRoute;
		}
	}

	// Try a climb route if NPC can climb and requested
	if ((buildFlags & bits_BUILD_CLIMB)	&& (CapabilitiesGet() & bits_CAP_MOVE_CLIMB))
	{
		AI_Waypoint_t *climbRoute = BuildClimbRoute(vStart,vEnd,pTarget,endFlags,buildFlags,flYaw);
		
		if (climbRoute)
		{
			return climbRoute;
		}
	}

	// Everything failed so return a NULL route
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Builds a route to the given vecGoal using either local movement
//			or nodes
//-----------------------------------------------------------------------------

ConVar ai_no_local_paths( "ai_no_local_paths", "0" );

AI_Waypoint_t *CAI_Pathfinder::BuildRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType, bool bLocalSucceedOnWithinTolerance, int nBuildFlags )
{
	Assert( ( nBuildFlags & ( bits_BUILD_GROUND | bits_BUILD_JUMP | bits_BUILD_FLY | bits_BUILD_CLIMB |
		bits_BUILD_CRAWL | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG | bits_BUILD_IGNORE_NPCS | bits_BUILD_COLLIDE_NPCS ) ) == 0 );
	nBuildFlags &= ~( bits_BUILD_GROUND | bits_BUILD_JUMP | bits_BUILD_FLY | bits_BUILD_CLIMB |
		bits_BUILD_CRAWL | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG | bits_BUILD_IGNORE_NPCS | bits_BUILD_COLLIDE_NPCS );

	bool bTryLocal = !ai_no_local_paths.GetBool() && ( ( nBuildFlags & bits_BUILD_NO_LOCAL_NAV ) == 0 );

	// Set up build flags
	if (curNavType == NAV_CLIMB)
	{
		 // if I'm climbing, then only allow routes that are also climb routes
		nBuildFlags = bits_BUILD_CLIMB;
		bTryLocal = false;
	}
	else if ( (CapabilitiesGet() & bits_CAP_MOVE_FLY) || (CapabilitiesGet() & bits_CAP_MOVE_SWIM) )
	{
		nBuildFlags = (bits_BUILD_FLY | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
	}
	else if (CapabilitiesGet() & bits_CAP_MOVE_GROUND)
	{
		nBuildFlags = (bits_BUILD_GROUND | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
		if ( CapabilitiesGet() & bits_CAP_MOVE_JUMP )
		{
			nBuildFlags |= bits_BUILD_JUMP;
		}
		if ( CapabilitiesGet() & bits_CAP_MOVE_CRAWL )
		{
			nBuildFlags |= bits_BUILD_CRAWL;
		}
	}

	// If our local moves can succeed if we get within the goaltolerance, set the flag
	if ( bLocalSucceedOnWithinTolerance )
	{
		nBuildFlags |= bits_BUILD_GET_CLOSE;
	}

	AI_Waypoint_t *pResult = NULL;

	//  First try a local route 
	if ( bTryLocal && CanUseLocalNavigation() )
	{
		int nLocalBuildFlags = nBuildFlags;
		if ( CapabilitiesGet() & bits_CAP_NO_LOCAL_NAV_CRAWL )
		{
			nLocalBuildFlags &= ~bits_BUILD_CRAWL;
		}
		pResult = BuildLocalRoute(vStart, vEnd, pTarget, 
								  bits_WP_TO_GOAL, 
								  nLocalBuildFlags, goalTolerance);
	}

	//  If the fails, try a node route
	if ( !pResult )
	{
		pResult = BuildNavMeshRoute( vStart, vEnd, nBuildFlags, goalTolerance );
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a radial route around the given center position
//			over a given arc size
//
// Input  : vStartPos	- where route should start from
//			vCenterPos	- the center of the arc
//			vGoalPos	- ultimate goal position
//			flRadius	- radius of the arc
//			flArc		- how long should the path be (in degrees)
//			bClockwise	- the direction we are heading
// Output : The route
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildRadialRoute( const Vector &vStartPos, const Vector &vCenterPos, const Vector &vGoalPos, float flRadius, float flArc, float flStepDist, bool bClockwise, float goalTolerance, bool bAirRoute /*= false*/ )
{
	MARK_TASK_EXPENSIVE();

	// ------------------------------------------------------------------------------
	// Make sure we have a minimum distance between nodes.  For the given 
	// radius, calculate the angular step necessary for this distance.
	// IMPORTANT: flStepDist must be large enough that given the 
	//			  NPC's movment speed that it can come to a stop
	// ------------------------------------------------------------------------------
	float flAngleStep = 2.0f * atan((0.5f*flStepDist)/flRadius);	

	// Flip direction if clockwise
	if ( bClockwise )
	{
		flArc		*= -1;
		flAngleStep *= -1;
	}

	// Calculate the start angle on the arc in world coordinates
	Vector vStartDir = ( vStartPos - vCenterPos );
	VectorNormalize( vStartDir );

	// Get our control angles
	float flStartAngle	= DEG2RAD(UTIL_VecToYaw(vStartDir));
	float flEndAngle	= flStartAngle + DEG2RAD(flArc);

	//  Offset set our first node by one arc step so NPC doesn't run perpendicular to the arc when starting a different radius
	flStartAngle += flAngleStep;

	AI_Waypoint_t*	pHeadRoute	= NULL;	// Pointer to the beginning of the route chains
	AI_Waypoint_t*	pNextRoute	= NULL; // Next leg of the route
	AI_Waypoint_t*  pLastRoute	= NULL; // The last route chain added to the head
	Vector			vLastPos	= vStartPos; // Last position along the arc in worldspace
	int				fRouteBits = ( bAirRoute ) ? bits_BUILD_FLY : bits_BUILD_GROUND; // Whether this is an air route or not
	float			flCurAngle = flStartAngle; // Starting angle
	Vector			vNextPos;
	
	// Make sure that we've got somewhere to go.  This generally means your trying to walk too small an arc.
	Assert( ( bClockwise && flCurAngle > flEndAngle ) || ( !bClockwise && flCurAngle < flEndAngle ) );

	// Start iterating through our arc
	while( 1 )
	{
		// See if we've ended our run
		if ( ( bClockwise && flCurAngle <= flEndAngle ) || ( !bClockwise && flCurAngle >= flEndAngle ) )
			break;
		
		// Get our next position along the arc
		vNextPos	= vCenterPos;
		vNextPos.x	+= flRadius * cos( flCurAngle );
		vNextPos.y	+= flRadius * sin( flCurAngle );

		// Build a route from the last position to the current one
		pNextRoute = BuildLocalRoute( vLastPos, vNextPos, NULL, 0, fRouteBits, goalTolerance);

		// If we can't find a route, we failed
		if ( pNextRoute == NULL )
			return NULL;

		// Don't simplify the route (otherwise we'll cut corners where we don't want to!
		pNextRoute->ModifyFlags( bits_WP_DONT_SIMPLIFY, true );
			
		if ( pHeadRoute )
		{
			// Tack the routes together
			AddWaypointLists( pHeadRoute, pNextRoute );
		}
		else
		{
			// Otherwise we're now the previous route
			pHeadRoute = pNextRoute;
		}
		
		// Move our position
		vLastPos  = vNextPos;
		pLastRoute = pNextRoute;

		// Move our current angle
		flCurAngle += flAngleStep;
	}

	// NOTE: We could also simply build a local route with no curve, but it's unlikely that's what was intended by the caller
	if ( pHeadRoute == NULL )
		return NULL;

	// Append a path to the final position
	pLastRoute = BuildLocalRoute( vLastPos, vGoalPos, NULL, 0, bAirRoute ? bits_BUILD_FLY : bits_BUILD_GROUND, goalTolerance );	
	if ( pLastRoute == NULL )
		return NULL;

	// Allow us to simplify the last leg of the route
	pLastRoute->ModifyFlags( bits_WP_DONT_SIMPLIFY, false );
	pLastRoute->ModifyFlags( bits_WP_TO_GOAL, true );

	// Add them together
	AddWaypointLists( pHeadRoute, pLastRoute );
	
	// Give back the complete route
	return pHeadRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Attemps to build a node route between vStart and vEnd
// Input  :
// Output : Returns a route if sucessful or NULL if no node route was possible
//-----------------------------------------------------------------------------

AI_Waypoint_t *CAI_Pathfinder::BuildNavMeshRoute(const Vector &vStart, const Vector &vEnd, int buildFlags, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildNodeRoute );

	// ----------------------------------------------------------------------
	//  Make sure network has nodes
	// ----------------------------------------------------------------------
	if (!RecastMgr().HasMeshes())
		return NULL;

	CRecastMesh *pMesh = GetNavMesh();
	if (!pMesh)
	{
		return NULL;
	}

	AI_Waypoint_t *path = pMesh->FindPath(vStart, vEnd);
	if (!path)
	{
		DbgNavMsgN( GetOuter(), "Node pathfind failed, no route between [%f, %f, %f] and [%f, %f, %f]\n", vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z );
		return NULL;
	}

	DbgNavMsg(  GetOuter(), "Node pathfind succeeded\n");
	return path;
}

//-----------------------------------------------------------------------------
// Test the triangulation route...
//-----------------------------------------------------------------------------
#ifdef _WIN32
#pragma warning (disable:4701)
#endif

bool CAI_Pathfinder::TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
	const Vector &vecApex, const Vector &vecEnd, const CBaseEntity *pTargetEnt, AIMoveTrace_t *pStartTrace )
{
	AIMoveTrace_t endTrace;
	endTrace.fStatus = AIMR_OK;	// just to make the debug overlay code easy

	// Check the triangulation
	CAI_MoveProbe *pMoveProbe = GetOuter()->GetMoveProbe();

	bool bPathClear = false;

	// See if we can get from the start point to the triangle apex
	if ( pMoveProbe->MoveLimit(navType, vecStart, vecApex, GetOuter()->GetAITraceMask(), pTargetEnt, pStartTrace ) )
	{
		// Ok, we got from the start to the triangle apex, now try
		// the triangle apex to the end
		if ( pMoveProbe->MoveLimit(navType, vecApex, vecEnd, GetOuter()->GetAITraceMask(), pTargetEnt, &endTrace ) )
		{
			bPathClear = true;
		}
	}

	// Debug mode: display the tested path...
	if (GetOuter()->m_debugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
		m_TriDebugOverlay.AddTriOverlayLines( vecStart, vecApex, vecEnd, *pStartTrace, endTrace, bPathClear);

	return bPathClear;
}

#ifdef _WIN32
#pragma warning (default:4701)
#endif


//-----------------------------------------------------------------------------
// Purpose: tries to overcome local obstacles by triangulating a path around them.
// Input  : flDist is is how far the obstruction that we are trying
//			to triangulate around is from the npc
// Output :
//-----------------------------------------------------------------------------

// FIXME: this has no concept that the blocker may not be exactly along the vecStart, vecEnd vector.
// FIXME: it should take a position (and size?) to avoid
// FIXME: this does the position checks in the same order as GiveWay() so they tend to fight each other when both are active
#define MAX_TRIAGULATION_DIST (32*12)
bool CAI_Pathfinder::Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEndIn, 
	float flDistToBlocker, const CBaseEntity *pTargetEnt, Vector *pApex )
{
	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	Assert( pApex );

	AI_PROFILE_SCOPE( CAI_Pathfinder_Triangulate );

	Vector vecForward, vecUp, vecPerpendicular;
	VectorSubtract( vecEndIn, vecStart, vecForward );
	float flTotalDist = VectorNormalize( vecForward );

	Vector vecEnd;

	// If we're walking, then don't try to triangulate over large distances
	if ( navType != NAV_FLY && flTotalDist > MAX_TRIAGULATION_DIST)
	{
		vecEnd = vecForward * MAX_TRIAGULATION_DIST;
		flTotalDist = MAX_TRIAGULATION_DIST;
		if ( !GetOuter()->GetMoveProbe()->MoveLimit(navType, vecEnd, vecEndIn, GetOuter()->GetAITraceMask(), pTargetEnt) )
		{
			return false;
		}

	}
	else
		vecEnd = vecEndIn;

	// Compute a direction vector perpendicular to the desired motion direction
	if ( 1.0f - fabs(vecForward.z) > 1e-3 )
	{
		vecUp.Init( 0, 0, 1 );
		CrossProduct( vecForward, vecUp, vecPerpendicular );	// Orthogonal to facing
	}
	else
	{
		vecUp.Init( 0, 1, 0 );
		vecPerpendicular.Init( 1, 0, 0 ); 
	}

	// Grab the size of the navigation bounding box
	float sizeX = 0.5f * GetHullLength();
	float sizeZ = 0.5f * GetHullHeight();

	// start checking right about where the object is, picking two equidistant
	// starting points, one on the left, one on the right. As we progress 
	// through the loop, we'll push these away from the obstacle, hoping to 
	// find a way around on either side. m_vecSize.x is added to the ApexDist 
	// in order to help select an apex point that insures that the NPC is 
	// sufficiently past the obstacle before trying to turn back onto its original course.

	if (GetOuter()->m_debugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
	{
		m_TriDebugOverlay.FadeTriOverlayLines();
	}

	float flApexDist = flDistToBlocker + sizeX;
	if (flApexDist > flTotalDist) 
	{
		flApexDist = flTotalDist;
	}

	// Compute triangulation apex points (NAV_FLY attempts vertical triangulation too)
	Vector vecDelta[2];
	Vector vecApex[4];
	float pApexDist[4];

	Vector vecCenter;
	int nNumToTest = 2;
	VectorMultiply( vecPerpendicular, sizeX, vecDelta[0] );

	VectorMA( vecStart, flApexDist, vecForward, vecCenter );
	VectorSubtract( vecCenter, vecDelta[0], vecApex[0] );
	VectorAdd( vecCenter, vecDelta[0], vecApex[1] );
 	vecDelta[0] *= 2.0f;
	pApexDist[0] = pApexDist[1] = flApexDist;

	if (navType == NAV_FLY)
	{
		VectorMultiply( vecUp, 3.0f * sizeZ, vecDelta[1] );
		VectorSubtract( vecCenter, vecDelta[1], vecApex[2] );
		VectorAdd( vecCenter, vecDelta[1], vecApex[3] );
		pApexDist[2] = pApexDist[3] = flApexDist;
		nNumToTest = 4;
	}

	AIMoveTrace_t moveTrace;
	for (int i = 0; i < 2; ++i )
	{
		// NOTE: Do reverse order so fliers try to move over the top first 
		for (int j = nNumToTest; --j >= 0; )
		{
			if (TestTriangulationRoute(navType, vecStart, vecApex[j], vecEnd, pTargetEnt, &moveTrace))
			{
				*pApex  = vecApex[j];
				return true;
			}

			// Here, the starting half of the triangle was blocked. Lets
			// pull back the apex toward the start...
			if (IsMoveBlocked(moveTrace))
			{
				Vector vecStartToObstruction;
				VectorSubtract( moveTrace.vEndPosition, vecStart, vecStartToObstruction );
				float flDistToObstruction = DotProduct( vecStartToObstruction, vecForward );

				float flNewApexDist = pApexDist[j];
				if (pApexDist[j] > flDistToObstruction)
					flNewApexDist = flDistToObstruction;

				VectorMA( vecApex[j], flNewApexDist - pApexDist[j], vecForward, vecApex[j] );
				pApexDist[j] = flNewApexDist;
			}

			// NOTE: This has to occur *after* the code above because
			// the above code uses vecApex for some distance computations
			if (j & 0x1)
				vecApex[j] += vecDelta[j >> 1];
			else
				vecApex[j] -= vecDelta[j >> 1];
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Triangulation debugging 
//-----------------------------------------------------------------------------

void CAI_Pathfinder::DrawDebugGeometryOverlays(int npcDebugOverlays) 
{
	m_TriDebugOverlay.Draw(npcDebugOverlays);
}

void CAI_Pathfinder::CTriDebugOverlay::Draw(int npcDebugOverlays) 
{
	if (m_debugTriOverlayLine) 
	{
		if ( npcDebugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
		{
			for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
			{
				if (m_debugTriOverlayLine[i]->draw)
				{
					NDebugOverlay::Line(m_debugTriOverlayLine[i]->origin,
										 m_debugTriOverlayLine[i]->dest,
										 m_debugTriOverlayLine[i]->r,
										 m_debugTriOverlayLine[i]->g,
										 m_debugTriOverlayLine[i]->b,
										 m_debugTriOverlayLine[i]->noDepthTest,
										 0);
				}
			}
		}
		else
		{
			ClearTriOverlayLines();
		}
	}
}

void CAI_Pathfinder::CTriDebugOverlay::AddTriOverlayLines( const Vector &vecStart, const Vector &vecApex, const Vector &vecEnd, const AIMoveTrace_t &startTrace, const AIMoveTrace_t &endTrace, bool bPathClear )
{
	static unsigned char s_TriangulationColor[2][3] = 
	{
		{ 255,   0, 0 },
		{   0, 255, 0 }
	};

	unsigned char *c = s_TriangulationColor[bPathClear];

	AddTriOverlayLine(vecStart, vecApex, c[0],c[1],c[2], false);
	AddTriOverlayLine(vecApex, vecEnd, c[0],c[1],c[2], false);

	// If we've blocked, draw an X where we were blocked...
	if (IsMoveBlocked(startTrace.fStatus))
	{
		Vector pt1, pt2;
		pt1 = pt2 = startTrace.vEndPosition;

		pt1.x -= 10; pt1.y -= 10;
		pt2.x += 10; pt2.y += 10;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);

		pt1.x += 20;
		pt2.x -= 20;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);
	}

	if (IsMoveBlocked(endTrace.fStatus))
	{
		Vector pt1, pt2;
		pt1 = pt2 = endTrace.vEndPosition;

		pt1.x -= 10; pt1.y -= 10;
		pt2.x += 10; pt2.y += 10;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);

		pt1.x += 20;
		pt2.x -= 20;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);
	}
}

void CAI_Pathfinder::CTriDebugOverlay::ClearTriOverlayLines(void) 
{
	if (m_debugTriOverlayLine)
	{
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i]->draw = false;
		}
	}
}
void CAI_Pathfinder::CTriDebugOverlay::FadeTriOverlayLines(void) 
{
	if (m_debugTriOverlayLine)
	{
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i]->r *= 0.5;
			m_debugTriOverlayLine[i]->g *= 0.5;				
			m_debugTriOverlayLine[i]->b *= 0.5;				
		}
	}
}
void CAI_Pathfinder::CTriDebugOverlay::AddTriOverlayLine(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest)
{
	if (!m_debugTriOverlayLine)
	{
		m_debugTriOverlayLine = new OverlayLine_t*[NUM_NPC_DEBUG_OVERLAYS];
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i] = new OverlayLine_t;
		}
	}
	static int overCounter = 0;

	if (overCounter >= NUM_NPC_DEBUG_OVERLAYS)
	{
		overCounter = 0;
	}
	
	m_debugTriOverlayLine[overCounter]->origin			= origin;
	m_debugTriOverlayLine[overCounter]->dest			= dest;
	m_debugTriOverlayLine[overCounter]->r				= r;
	m_debugTriOverlayLine[overCounter]->g				= g;
	m_debugTriOverlayLine[overCounter]->b				= b;
	m_debugTriOverlayLine[overCounter]->noDepthTest		= noDepthTest;
	m_debugTriOverlayLine[overCounter]->draw			= true;
	overCounter++;

}

//-----------------------------------------------------------------------------
