//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_PATHFINDER_H
#define AI_PATHFINDER_H

#include "ai_component.h"
#include "ai_navtype.h"
#include "ai_hull.h"

#if defined( _WIN32 )
#pragma once
#endif

struct AIMoveTrace_t;
struct OverlayLine_t;
struct AI_Waypoint_t;
#ifndef AI_USES_NAV_MESH
class CAI_Link;
class CAI_Network;
class CAI_Node;
#else
class CNavArea;
class CNavLadder;
class CFuncElevator;
#endif
class CAI_Pathfinder;

#ifdef AI_USES_NAV_MESH
enum AreaType_e
{
	AREA_ANY,			// Used to specify any type of node (for search)
	AREA_GROUND,     
	AREA_AIR,       
	AREA_CLIMB,  
	AREA_WATER     
};

AreaType_e GetAreaType(CNavArea *area);

int GetAreaAcceptedMoveTypes(CNavArea *area, Hull_t hull);
#endif

#ifdef AI_USES_NAV_MESH
class NPCPathCost
{
public:
	NPCPathCost(CAI_Pathfinder *pPathFinder);

	float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length);

protected:
	CAI_Pathfinder *m_pPathFinder;
	CAI_BaseNPC *m_pNPC;
};
#endif

//-----------------------------------------------------------------------------
// The type of route to build
enum RouteBuildFlags_e 
{
	bits_BUILD_GROUND		=			0x00000001, // 
	bits_BUILD_JUMP			=			0x00000002, //
	bits_BUILD_FLY			=			0x00000004, // 
	bits_BUILD_CLIMB		=			0x00000008, //
	bits_BUILD_GIVEWAY		=			0x00000010, //
	bits_BUILD_TRIANG		=			0x00000020, //
	bits_BUILD_IGNORE_NPCS	=			0x00000040, // Ignore collisions with NPCs
	bits_BUILD_COLLIDE_NPCS	=			0x00000080, // Use    collisions with NPCs (redundant for argument clarity)
	bits_BUILD_GET_CLOSE	=			0x00000100, // the route will be built even if it can't reach the destination
};

//-----------------------------------------------------------------------------
// CAI_Pathfinder
//
// Purpose: Executes pathfinds through an associated network.
//
//-----------------------------------------------------------------------------

class CAI_Pathfinder : public CAI_Component
{
public:
	CAI_Pathfinder( CAI_BaseNPC *pOuter )
	 :	CAI_Component(pOuter)
	#ifndef AI_USES_NAV_MESH
		,m_flLastStaleLinkCheckTime( 0 )
		,m_pNetwork( NULL )
	#endif
	{
	}

#ifndef AI_USES_NAV_MESH
	void Init( CAI_Network *pNetwork );
#else
	void Init();
#endif
	
	//---------------------------------

#ifndef AI_USES_NAV_MESH
	int				NearestNodeToNPC();
	int				NearestNodeToPoint( const Vector &vecOrigin );
#else
	CNavArea *		NearestAreaToNPC();
	CNavArea *		NearestAreaToPoint( const Vector &vecOrigin );
#endif

#ifndef AI_USES_NAV_MESH
	AI_Waypoint_t*	FindBestPath		(int startID, int endID);
	AI_Waypoint_t*	FindShortRandomPath	(int startID, float minPathLength, const Vector &vDirection = vec3_origin);
#else
	AI_Waypoint_t*	FindBestPath		(CNavArea * startArea, CNavArea * endArea);
	AI_Waypoint_t*	FindShortRandomPath	(CNavArea * startArea, float minPathLength, const Vector &vDirection = vec3_origin);
#endif

	// --------------------------------

#ifndef AI_USES_NAV_MESH
	bool			IsLinkUsable(CAI_Link *pLink, int startID);
#else
	bool			IsAreaUsable(CNavArea *pArea);
#endif

	// --------------------------------
	
	AI_Waypoint_t *BuildRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType = NAV_NONE, bool bLocalSucceedOnWithinTolerance = false );
#ifndef AI_USES_NAV_MESH
	void UnlockRouteNodes( AI_Waypoint_t * );
#endif

	// --------------------------------

#ifndef AI_USES_NAV_MESH
	void SetIgnoreBadLinks()		{ m_bIgnoreStaleLinks = true; } // lasts only for the next pathfind
#endif

	// --------------------------------
	
#ifndef AI_USES_NAV_MESH
	virtual AI_Waypoint_t *BuildNodeRoute( const Vector &vStart, const Vector &vEnd, int buildFlags, float goalTolerance );
#else
	virtual AI_Waypoint_t *BuildAreaRoute( const Vector &vStart, const Vector &vEnd, int buildFlags, float goalTolerance );
#endif

#ifndef AI_USES_NAV_MESH
	virtual AI_Waypoint_t *BuildLocalRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float goalTolerance);
#else
	virtual AI_Waypoint_t *BuildLocalRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area, int buildFlags, float goalTolerance);
#endif

	virtual AI_Waypoint_t *BuildRadialRoute( const Vector &vStartPos, const Vector &vCenterPos, const Vector &vGoalPos, float flRadius, float flArc, float flStepDist, bool bClockwise, float goalTolerance, bool bAirRoute );	

#ifndef AI_USES_NAV_MESH
	virtual AI_Waypoint_t *BuildTriangulationRoute( const Vector &vStart, 
													const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID,
													float flYaw, float flDistToBlocker, Navigation_t navType);
#else
	virtual AI_Waypoint_t *BuildTriangulationRoute( const Vector &vStart, 
													const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area,
													float flYaw, float flDistToBlocker, Navigation_t navType);
#endif

	virtual AI_Waypoint_t *BuildOBBAvoidanceRoute(  const Vector &vStart, const Vector &vEnd, 
													const CBaseEntity *pObstruction, const CBaseEntity *pTarget, 
													Navigation_t navType );

	// --------------------------------
	
	bool Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, 
						float flDistToBlocker, CBaseEntity const *pTargetEnt, Vector *pApex );
						
	// --------------------------------
	
	void DrawDebugGeometryOverlays( int m_debugOverlays );

protected:
	virtual bool	CanUseLocalNavigation() { return true; }

private:
	friend class CPathfindNearestNodeFilter;

	//---------------------------------
#ifndef AI_USES_NAV_MESH
	AI_Waypoint_t*	RouteToNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance);
	AI_Waypoint_t*	RouteFromNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance);

	AI_Waypoint_t *	BuildNearestNodeRoute( const Vector &vGoal, bool bToNode, int buildFlags, float goalTolerance, int *pNearestNode );

	//---------------------------------
	
	AI_Waypoint_t*	MakeRouteFromParents(int *parentArray, int endID);
	AI_Waypoint_t*	CreateNodeWaypoint( Hull_t hullType, int nodeID, int nodeFlags = 0 );
#else
	AI_Waypoint_t*	RouteToArea(const Vector &vecOrigin, int buildFlags, CNavArea * area, float goalTolerance);
	AI_Waypoint_t*	RouteFromArea(const Vector &vecOrigin, int buildFlags, CNavArea * area, float goalTolerance);

	AI_Waypoint_t *	BuildNearestAreaRoute( const Vector &vGoal, bool bToArea, int buildFlags, float goalTolerance, CNavArea **pNearestArea );

	//---------------------------------
	
	AI_Waypoint_t*	MakeRouteFromParents(int *parentArray, int endID);
	AI_Waypoint_t*	CreateAreaWaypoint( Hull_t hullType, CNavArea *area, int areaFlags = 0 );
#endif

	AI_Waypoint_t*	BuildRouteThroughPoints( Vector *vecPoints, int nNumPoints, int nDirection, int nStartIndex, int nEndIndex, Navigation_t navType, CBaseEntity *pTarget );

#ifndef AI_USES_NAV_MESH
	bool			IsLinkStillStale(int moveType, CAI_Link *nodeLink);
#endif

	// --------------------------------
#ifndef AI_USES_NAV_MESH
	// Builds a simple route (no triangulation, no making way)
	AI_Waypoint_t	*BuildSimpleRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd, 
		const CBaseEntity *pTarget, int endFlags, int nodeID, int nodeTargetType, float flYaw);

	// Builds a complex route (triangulation, making way)
	AI_Waypoint_t	*BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
		const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, 
		int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance );

	AI_Waypoint_t	*BuildGroundRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildFlyRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildJumpRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );
	AI_Waypoint_t	*BuildClimbRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );
#else
	// Builds a simple route (no triangulation, no making way)
	AI_Waypoint_t	*BuildSimpleRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd, 
		const CBaseEntity *pTarget, int endFlags, CNavArea * area, int areaTargetType, float flYaw);

	// Builds a complex route (triangulation, making way)
	AI_Waypoint_t	*BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
		const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, CNavArea * area, 
		int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance );

	AI_Waypoint_t	*BuildGroundRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildFlyRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildJumpRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area, int buildFlags, float flYaw );
	AI_Waypoint_t	*BuildClimbRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, CNavArea * area, int buildFlags, float flYaw );
#endif

#ifndef AI_USES_NAV_MESH
	// Computes the link type
	Navigation_t ComputeWaypointType( CAI_Node **ppNodes, int parentID, int destID );
#else
	Navigation_t ComputeWaypointType( CNavArea *parentArea, CNavArea *destArea );
#endif

	// --------------------------------
	
	bool TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
		const Vector &vecApex, const Vector &vecEnd, const CBaseEntity *pTargetEnt, AIMoveTrace_t *pStartTrace );

	// --------------------------------
#ifndef AI_USES_NAV_MESH
	bool			CheckStaleRoute( const Vector &vStart, const Vector &vEnd, int moveTypes);
	bool			CheckStaleNavTypeRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd );
#endif

	// --------------------------------
	
	bool			CanGiveWay( const Vector& vStart, const Vector& vEnd, CBaseEntity *pNPCBlocker );

	// --------------------------------

	bool			UseStrongOptimizations();

	// --------------------------------
	// Debugging fields and functions

	class CTriDebugOverlay
	{
	public:
		CTriDebugOverlay()
		 :	m_debugTriOverlayLine( NULL )
		{
		}
		void AddTriOverlayLines( const Vector &vecStart, const Vector &vecApex, const Vector &vecEnd, const AIMoveTrace_t &startTrace, const AIMoveTrace_t &endTrace, bool bPathClear );
		void ClearTriOverlayLines(void);
		void FadeTriOverlayLines(void);

		void Draw(int npcDebugOverlays);
	private:
		void AddTriOverlayLine(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest);

		OverlayLine_t	**m_debugTriOverlayLine;
	};

	CTriDebugOverlay m_TriDebugOverlay;

	//---------------------------------
#ifndef AI_USES_NAV_MESH
	float m_flLastStaleLinkCheckTime;	// Last time I check for a stale link
	bool m_bIgnoreStaleLinks;
#endif

	//---------------------------------

#ifndef AI_USES_NAV_MESH
	CAI_Network *GetNetwork()				{ return m_pNetwork; }
	const CAI_Network *GetNetwork() const	{ return m_pNetwork; }
	
	CAI_Network *m_pNetwork;
#endif

public:
	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_PATHFINDER_H
