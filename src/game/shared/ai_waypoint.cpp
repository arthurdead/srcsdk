//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "mempool.h"
#include "ai_navtype.h"
#ifndef AI_USES_NAV_MESH
#include "ai_node.h"
#endif
#include "ai_waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	WAYPOINT_POOL_SIZE 512

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------

DEFINE_FIXEDSIZE_ALLOCATOR( AI_Waypoint_t, WAYPOINT_POOL_SIZE, CUtlMemoryPool::GROW_FAST );

//-------------------------------------

AI_Waypoint_t::AI_Waypoint_t()
{
	memset( this, 0, sizeof(*this) );
	vecLocation	= vec3_invalid;
#ifndef AI_USES_NAV_MESH
	iNodeID		= NO_NODE;
#else
	pArea = NULL;
#endif
	flPathDistGoal = -1;
}

//-------------------------------------

#ifndef AI_USES_NAV_MESH
AI_Waypoint_t::AI_Waypoint_t( const Vector &initPosition, float initYaw, Navigation_t initNavType, int initWaypointFlags, int initNodeID )
#else
AI_Waypoint_t::AI_Waypoint_t( const Vector &initPosition, float initYaw, Navigation_t initNavType, int initWaypointFlags, CNavArea *initArea )
#endif
{
	memset( this, 0, sizeof(*this) );

	// A Route of length one to the endpoint
	vecLocation	= initPosition;
	flYaw		= initYaw;
	m_iWPType	= initNavType;
	m_fWaypointFlags = initWaypointFlags;
#ifndef AI_USES_NAV_MESH
	iNodeID		= initNodeID;
#else
	pArea		= initArea;
#endif

	flPathDistGoal = -1;
}

//-------------------------------------

AI_Waypoint_t *	AI_Waypoint_t::GetLast()
{
	Assert( !pNext || pNext->pPrev == this ); 
	AI_Waypoint_t *pCurr = this;
	while (pCurr->GetNext())
	{
		pCurr = pCurr->GetNext();
	}

	return pCurr;
}


//-----------------------------------------------------------------------------

void CAI_WaypointList::RemoveAll()
{
	DeleteAll( &m_pFirstWaypoint );
	Assert( m_pFirstWaypoint == NULL );
}

//-------------------------------------

void CAI_WaypointList::PrependWaypoints( AI_Waypoint_t *pWaypoints )
{
	AddWaypointLists( pWaypoints, GetFirst() );
	Set( pWaypoints );
}

//-------------------------------------

void CAI_WaypointList::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags, float flYaw )
{
#ifndef AI_USES_NAV_MESH
	PrependWaypoints( new AI_Waypoint_t( newPoint, flYaw, navType, waypointFlags, NO_NODE ) );
#else
	PrependWaypoints( new AI_Waypoint_t( newPoint, flYaw, navType, waypointFlags, NULL ) );
#endif
}

//-------------------------------------

void CAI_WaypointList::Set(AI_Waypoint_t* route)
{
	m_pFirstWaypoint = route;
}

//-------------------------------------

AI_Waypoint_t *CAI_WaypointList::GetLast()
{
	AI_Waypoint_t *p = GetFirst();
	if (!p)
		return NULL;
	while ( p->GetNext() )
		p = p->GetNext();

	return p;
}

//-------------------------------------

const AI_Waypoint_t *CAI_WaypointList::GetLast() const
{
	return const_cast<CAI_WaypointList *>(this)->GetLast();
}

//-------------------------------------

#ifdef DEBUG
void AssertRouteValid( AI_Waypoint_t* route )
{
	// Check that the goal wasn't just clobbered
	if (route) 
	{
		AI_Waypoint_t* waypoint = route;

		while (waypoint)
		{
#ifdef _GOALDEBUG
			if (!waypoint->GetNext() && !(waypoint->Flags() & (bits_WP_TO_GOAL|bits_WP_TO_PATHCORNER)))
			{
				DevMsg( "!!ERROR!! Final waypoint is not a goal!\n");
			}
#endif
			waypoint->AssertValid();
			waypoint = waypoint->GetNext();
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Deletes a waypoint linked list
//-----------------------------------------------------------------------------
void DeleteAll( AI_Waypoint_t *pWaypointList )
{
	while ( pWaypointList )
	{
		AI_Waypoint_t *pPrevWaypoint = pWaypointList;
		pWaypointList = pWaypointList->GetNext();
		delete pPrevWaypoint;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds addRoute to the end of oldRoute
//-----------------------------------------------------------------------------
void AddWaypointLists(AI_Waypoint_t *oldRoute, AI_Waypoint_t *addRoute)
{
	// Add to the end of the route
	AI_Waypoint_t *waypoint = oldRoute;

	while (waypoint->GetNext()) 
	{
		waypoint = waypoint->GetNext();
	}

	waypoint->ModifyFlags( bits_WP_TO_GOAL, false );

	// Check for duplication, but copy the type
#ifndef AI_USES_NAV_MESH
	if (waypoint->iNodeID != NO_NODE			&&
		waypoint->iNodeID == addRoute->iNodeID	)
#else
	if (waypoint->pArea != NULL			&&
		waypoint->pArea == addRoute->pArea	)
#endif
	{
//		waypoint->iWPType = addRoute->iWPType; <<TODO>> found case where this was bad
		AI_Waypoint_t *pNext = addRoute->GetNext();
		delete addRoute;
		waypoint->SetNext(pNext);
	}
	else
	{
		waypoint->SetNext(addRoute);
	}

	while (waypoint->GetNext()) 
	{
		waypoint = waypoint->GetNext();
	}

	waypoint->ModifyFlags( bits_WP_TO_GOAL, true );

}

//-----------------------------------------------------------------------------