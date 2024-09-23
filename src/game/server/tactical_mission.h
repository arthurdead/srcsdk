//========= Copyright Valve Corporation, All rights reserved. ============//
// tactical_mission.h
// Interface for managing player "missions"
// Michael Booth, June 2009

#ifndef TACTICAL_MISSION_H
#define TACTICAL_MISSION_H

#include "GameEventListener.h"
#include "tier1/utlvector.h"

class CBasePlayer;

//---------------------------------------------------------------------------------------------
/**
 * A mission zone defines a region of space where something tactically interesting occurs.
 */
class CTacticalMissionZone
{
public:

};


//---------------------------------------------------------------------------------------------
/**
 * A mission encapsulates an important task or set of tasks, such as capturing an enemy point
 */
class CTacticalMission
{
public:
	virtual ~CTacticalMission() { }

	virtual const CTacticalMissionZone *GetDeployZone( CBasePlayer *who ) const;	// where give player should be during this mission
	virtual const CTacticalMissionZone *GetObjectiveZone( void ) const;				// control points, setup gates, sections of cart path, etc.
	virtual const CTacticalMissionZone *GetEnemyZone( void ) const;					// where we expect enemies to be during this mission

	virtual const char *GetName( void ) const = 0;									// return name of this mission
};

inline const CTacticalMissionZone *CTacticalMission::GetDeployZone( CBasePlayer *who ) const
{
	return NULL;
}

inline const CTacticalMissionZone *CTacticalMission::GetObjectiveZone( void ) const
{
	return NULL;
}

inline const CTacticalMissionZone *CTacticalMission::GetEnemyZone( void ) const
{
	return NULL;
}


//---------------------------------------------------------------------------------------------
/**
 * The mission manager provides access to all available missions
 */
class CTacticalMissionManager : public CGameEventListener
{
public:
	CTacticalMissionManager( void );
	virtual ~CTacticalMissionManager() { }

	virtual void FireGameEvent( IGameEvent *event );						// incoming event processing

	virtual void OnServerActivate( void ) { }								// invoked when server loads a new map, after everything has been created/spawned
	virtual void OnRoundRestart( void ) { } 								// invoked when a game round restarts

	virtual void Register( CTacticalMission *mission );
	virtual void Unregister( CTacticalMission *mission );

	virtual const CTacticalMission *GetMission( const char *name );			// given a mission name, return the mission (or NULL)
	
	/**
	 * Iterate each mission.
	 * If functor returns false, stop iterating and return false.
	 */
	class IForEachMission
	{
	public:
		virtual bool Inspect( const CTacticalMission &mission ) = 0;
	};
	virtual bool ForEachMission( IForEachMission &func );

protected:
	CUtlVector< CTacticalMission * > m_missionVector;
};


// global singleton
extern CTacticalMissionManager &TheTacticalMissions( void );


#endif // TACTICAL_MISSION_H
