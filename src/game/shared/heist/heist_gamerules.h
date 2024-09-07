#ifndef HEIST_GAMERULES_H
#define HEIST_GAMERULES_H

#pragma once

#ifdef CLIENT_DLL
	#define CHeistGameRules C_HeistGameRules
	#define CHeistGameRulesProxy C_HeistGameRulesProxy
#endif

#include "gamerules.h"
#include "heist_shareddefs.h"

class CHeistGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHeistGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CHeistGameRules : public CGameRules
{
public:
	DECLARE_CLASS(CHeistGameRules, CGameRules);
	DECLARE_NETWORKCLASS();

	CHeistGameRules();
	~CHeistGameRules() override;

	const unsigned char *GetEncryptionKey() override
	{ return (unsigned char *)"x9Ke0BY7"; }

#ifndef CLIENT_DLL
	virtual CGameRulesProxy *AllocateProxy();

	virtual const char*		AIClassText(Class_T classType);
	virtual const char*		AIFactionText(Faction_T classType);
	virtual Team_t GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( Team_t teamIndex );

	virtual void			InitDefaultAIRelationships( void );

	virtual int				NumEntityClasses() const	{ return NUM_HEIST_ENTITY_CLASSES; }
	virtual int				NumFactions() const	{ return NUM_HEIST_FACTIONS; }
	virtual int				NumTeams() const	{ return NUM_HEIST_TEAMS; }

	virtual const char *GetGameDescription( void ) { return "Heist"; }
#endif

	void SetSpotted(bool value);

	bool AnyoneSpotted() const
	{ return m_bHeistersSpotted; }

private:
	CNetworkVar(bool, m_bHeistersSpotted);
};

inline CHeistGameRules *HeistGameRules()
{
	return static_cast<CHeistGameRules*>(GameRules());
}

#endif
