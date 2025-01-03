//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_ROUTE_H
#define AI_ROUTE_H
#pragma once

#include "ai_basenpc.h"
#include "mathlib/vector.h"
#include "ai_waypoint.h"

struct AI_Waypoint_t;
#ifndef SWDS
struct OverlayLine_t;
#endif

class CAI_BaseNPC;

//=============================================================================
//	>> CAI_Path
//=============================================================================

#define DEF_WAYPOINT_TOLERANCE (0.1)

class CAI_Path
{
//-----------------------------------------------------------------

public:

	void			SetWaypoints(AI_Waypoint_t* route, bool fSetGoalFromLast = false) ;

	void 			PrependWaypoints( AI_Waypoint_t *pWaypoints );
	void 			PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags );

	bool 			IsEmpty() const				{ return m_Waypoints.IsEmpty(); }

	AI_Waypoint_t *	GetCurWaypoint() 				{ return m_Waypoints.GetFirst(); }
	const AI_Waypoint_t *GetCurWaypoint() const 	{ return m_Waypoints.GetFirst(); }

	AI_Waypoint_t *	GetGoalWaypoint() 				{ return m_Waypoints.GetLast(); }
	const AI_Waypoint_t *GetGoalWaypoint() const 	{ return m_Waypoints.GetLast(); }

	const Vector &	CurWaypointPos() const;
	const Vector &	NextWaypointPos() const;
	float			CurWaypointYaw() const;
	int				CurWaypointFlags() const;
	Navigation_t	CurWaypointNavType() const;

	AI_Waypoint_t * GetTransitionWaypoint();

	//---------------------------------

	float			GetPathLength();
	float			GetPathDistanceToGoal( const Vector &);

	float			GetStartTime() const	{ return m_routeStartTime; }

	//---------------------------------
	// How close do we need to get to the goal
	//---------------------------------
	void			SetGoalTolerance(float tolerance)	{ m_goalTolerance = tolerance;		}
	float			GetGoalTolerance() const			{ return m_goalTolerance;			}

	void			SetWaypointTolerance(float tolerance)	{ m_waypointTolerance = tolerance;  }
	float			GetWaypointTolerance() const			{ return m_waypointTolerance;  }

	//---------------------------------
	// The activity to use during motion
	//---------------------------------
	Activity		GetMovementActivity() const					{ return m_activity;				}
	Activity		SetMovementActivity(Activity activity);
	int				GetMovementSequence() const 				{ return m_sequence;				}
	int				SetMovementSequence(int sequence)			{ return (m_sequence = sequence);	}

	Activity		GetArrivalActivity( ) const;
	void			SetArrivalActivity(Activity activity);
	int				GetArrivalSequence( ) const;
	void			SetArrivalSequence(int sequence);

	void			SetGoalDirection( const Vector &goalDirection );
	void			SetGoalDirection( CBaseEntity *pTarget );
	Vector			GetGoalDirection( const Vector &startPos );

	void			SetGoalSpeed( float flSpeed );
	void			SetGoalSpeed( CBaseEntity *pTarget );
	float			GetGoalSpeed( const Vector &startPos );

	void			SetGoalStoppingDistance( float flDistance );
	float			GetGoalStoppingDistance( ) const;

	//---------------------------------
	// Target of this path
	//---------------------------------
	void			SetTarget(CBaseEntity * pTarget )	{ m_target = pTarget;				}
	void			ClearTarget()						{ m_target.Term(); m_vecTargetOffset = vec3_origin;	}
	void			SetTargetOffset( const Vector &vecOffset)	{ m_vecTargetOffset = vecOffset;	}
	CBaseEntity *	GetTarget()							{ return m_target.Get();					}

	void			SetGoalType(GoalType_t goalType);				// Set the goal type
	void			SetGoalPosition(const Vector &goalPos);			// Set the goal position
	void			SetLastWaypointAsGoal(bool bReset = false);
	void			ResetGoalPosition(const Vector &goalPos);		// Reset the goal position

	// Returns the *base* goal position (without the offset applied) 
	const Vector&	BaseGoalPosition() const;

	// Returns the *actual* goal position (with the offset applied)
	const Vector &	ActualGoalPosition(void) const;			// Get the goal position

	GoalType_t		GoalType(void) const;						// Get the goal type
	
	void			SetGoalFlags( unsigned flags )		{ m_goalFlags = flags;				}
	unsigned		GoalFlags( void ) const;			// Get the goal flags

	void			Advance( void );					// Advance to next waypoint if possible

	bool			CurWaypointIsGoal(void) const;

	void			Clear(void);

	CAI_Path();
	~CAI_Path();

	//---------------------------------

	void ClearWaypoints() { m_Waypoints.RemoveAll(); }

private:

	// Computes the goal distance for each waypoint along the route
	static void ComputeRouteGoalDistances(AI_Waypoint_t *pGoalWaypoint);

	//---------------------------------
	CAI_WaypointList m_Waypoints;

	//---------------------------------
	float		m_goalTolerance;			// How close do we need to get to the goal
	Activity	m_activity;					// The activity to use during motion
	int			m_sequence;					// The sequence to use during motion
	EHANDLE		m_target;					// Target of this path
	Vector		m_vecTargetOffset;			// offset from the target in world space
	float		m_waypointTolerance;

	//---------------------------------
	Activity	m_arrivalActivity;
	int			m_arrivalSequence;

	bool		m_bGoalPosSet;				// Was goal position set (used to check for errors)
	Vector		m_goalPos;					// Our ultimate goal position

	bool		m_bGoalTypeSet;				// Was goal position set (used to check for errors)
	GoalType_t	m_goalType;					// Type of goal

	unsigned	m_goalFlags;				// Goal flags

	//---------------------------------
	float		m_routeStartTime;

	//---------------------------------
	Vector		m_goalDirection;
	EHANDLE		m_goalDirectionTarget;

	float		m_goalSpeed;
	EHANDLE		m_goalSpeedTarget;

	float		m_goalStoppingDistance;		// Distance we want to stop before the goal

	//---------------------------------
	static AI_Waypoint_t gm_InvalidWaypoint;

};

#endif // AI_ROUTE_H
