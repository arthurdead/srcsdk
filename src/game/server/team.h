//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAM_H
#define TEAM_H
#pragma once

#include "shareddefs.h"
#include "utlvector.h"
#include "baseentity.h"

class CBasePlayer;
class CTeamSpawnPoint;

class CTeam : public CLogicalEntity
{
	DECLARE_CLASS( CTeam, CLogicalEntity );
public:
	CTeam( void );
	virtual ~CTeam( void );

	DECLARE_SERVERCLASS();

	virtual void Precache( void ) { return; };

	virtual void Think( void );

	virtual void			UpdateOnRemove( void );

	//-----------------------------------------------------------------------------
	// Initialization
	//-----------------------------------------------------------------------------
	virtual void		Init( const char *pName, Team_t iNumber );

	//-----------------------------------------------------------------------------
	// Data Handling
	//-----------------------------------------------------------------------------
	virtual int			GetTeamNumber( void ) const;
	virtual const char *GetName( void );
	virtual void		UpdateClientData( CBasePlayer *pPlayer );
	virtual bool		ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity );

	//-----------------------------------------------------------------------------
	// Spawnpoints
	//-----------------------------------------------------------------------------
	virtual void InitializeSpawnpoints( void );
	virtual void AddSpawnpoint( CTeamSpawnPoint *pSpawnpoint );
	virtual void RemoveSpawnpoint( CTeamSpawnPoint *pSpawnpoint );
	virtual CBaseEntity *SpawnPlayer( CBasePlayer *pPlayer );

	//-----------------------------------------------------------------------------
	// Players
	//-----------------------------------------------------------------------------
	virtual void InitializePlayers( void );
	virtual void AddPlayer( CBasePlayer *pPlayer );
	virtual void RemovePlayer( CBasePlayer *pPlayer );
	virtual int  GetNumPlayers( void );
	virtual CBasePlayer *GetPlayer( int iIndex );

	//-----------------------------------------------------------------------------
	// Scoring
	//-----------------------------------------------------------------------------
	virtual void AddScore( int iScore );
	virtual void SetScore( int iScore );
	virtual int  GetScore( void );
	virtual void ResetScores( void );

	// Round scoring
	virtual int GetRoundsWon( void ) { return m_iRoundsWon; }
	virtual void SetRoundsWon( int iRounds ) { m_iRoundsWon = iRounds; }
	virtual void IncrementRoundsWon( void ) { m_iRoundsWon++; }

	void AwardAchievement( int iAchievement );

	virtual int GetAliveMembers( void );

private:
	virtual void	ChangeTeam( Team_t iTeamNum );

public:
	CUtlVector< CTeamSpawnPoint * > m_aSpawnPoints;
	CNetworkUtlVector( CBasePlayer *,		m_aPlayers );

	// Data
	CNetworkString( m_szTeamname, MAX_TEAM_NAME_LENGTH );
	CNetworkVar( int, m_iScore );
	CNetworkVar( int, m_iRoundsWon );
	int		m_iDeaths;

	// Spawnpoints
	int		m_iLastSpawn;		// Index of the last spawnpoint used
};

extern CUtlVector< CHandle< CTeam > > g_Teams;
extern CTeam *GetGlobalTeamByIndex( int iIndex );
extern CTeam *GetGlobalTeamByTeam( Team_t iIndex );
extern int GetNumberOfTeams( void );
extern const char* GetTeamNameByIndex( int iIndex );
extern const char* GetNameOfTeam( Team_t iIndex );

#endif // TEAM_H
