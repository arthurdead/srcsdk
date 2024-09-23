//========= Copyright Valve Corporation, All rights reserved. ============//
// tactical_mission.cpp
// Interface for managing player "missions"
// Michael Booth, June 2009

#include "cbase.h"
#include "tactical_mission.h"

/**
 * Global singleton accessor.
 */
static CTacticalMissionManager *manager = NULL;
CTacticalMissionManager &TheTacticalMissions( void )
{
	if(!manager)
		manager = GameRules()->TacticalMissionManagerFactory();

	return *manager;
}


//---------------------------------------------------------------------------------------------
class CListMissions : public CTacticalMissionManager::IForEachMission 
{
public:
	virtual bool Inspect( const CTacticalMission &mission )
	{
		Msg( "%s\n", mission.GetName() );
		return true;
	}
};

CON_COMMAND_F( mission_list, "List all available tactical missions", FCVAR_GAMEDLL )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CListMissions list;
	TheTacticalMissions().ForEachMission( list );
}


CON_COMMAND_F( mission_show, "Show the given mission", FCVAR_GAMEDLL )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "%s <name of mission>\n", args.Arg(0) );
		return;
	}

	const CTacticalMission *mission = TheTacticalMissions().GetMission( args.Arg(1) );
	if ( mission )
	{
		const CTacticalMissionZone *zone = mission->GetDeployZone( NULL );
		if ( zone )
		{
			
		}
		else
		{
			Msg( "No deploy zone\n" );
		}
	}
	else
	{
		Msg( "Unknown mission '%s'\n", args.Arg(1) );
	}
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTacticalMissionManager::CTacticalMissionManager( void )
{
	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "teamplay_round_start" );
}


//---------------------------------------------------------------------------------------------
void CTacticalMissionManager::FireGameEvent( IGameEvent *gameEvent )
{
	if ( FStrEq( gameEvent->GetName(), "round_start" ) || FStrEq( gameEvent->GetName(), "teamplay_round_start" ) )
	{
		OnRoundRestart();
	}
}


//---------------------------------------------------------------------------------------------
void CTacticalMissionManager::Register( CTacticalMission *mission )
{
	if ( m_missionVector.Find( mission ) == m_missionVector.InvalidIndex() )
	{
		m_missionVector.AddToTail( mission );
	}
}


//---------------------------------------------------------------------------------------------
void CTacticalMissionManager::Unregister( CTacticalMission *mission )
{
	m_missionVector.FindAndRemove( mission );
}


//---------------------------------------------------------------------------------------------
/**
 * Given a mission name, return the mission (or NULL)
 */
const CTacticalMission *CTacticalMissionManager::GetMission( const char *name )
{
	FOR_EACH_VEC( m_missionVector, it )
	{
		if ( FStrEq( m_missionVector[it]->GetName(), name ) )
			return m_missionVector[it];
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
/**
 * Iterate each mission.
 * If functor returns false, stop iterating and return false.
 */
bool CTacticalMissionManager::ForEachMission( CTacticalMissionManager::IForEachMission &func )
{
	FOR_EACH_VEC( m_missionVector, it )
	{
		if ( !func.Inspect( *m_missionVector[it] ) )
			return false;
	}

	return true;
}
