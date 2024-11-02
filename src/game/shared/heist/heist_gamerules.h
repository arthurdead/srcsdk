#ifndef HEIST_GAMERULES_H
#define HEIST_GAMERULES_H

#pragma once

#include "gamerules.h"
#include "heist_shareddefs.h"

#ifdef CLIENT_DLL
class C_HeistGameRulesProxy;
typedef C_HeistGameRulesProxy CSharedHeistGameRulesProxy;
class C_HeistGameRules;
typedef C_HeistGameRules CSharedHeistGameRules;
#else
class CHeistGameRulesProxy;
typedef CHeistGameRulesProxy CSharedHeistGameRulesProxy;
class CHeistGameRules;
typedef CHeistGameRules CSharedHeistGameRules;
#endif

#ifdef CLIENT_DLL
	#define CHeistGameRulesProxy C_HeistGameRulesProxy
#endif

class CHeistGameRulesProxy : public CSharedGameRulesProxy
{
public:
	DECLARE_CLASS( CHeistGameRulesProxy, CSharedGameRulesProxy );

#ifdef CLIENT_DLL
	#undef CHeistGameRulesProxy
#endif

	DECLARE_NETWORKCLASS();
};

#ifdef CLIENT_DLL
	#define CHeistGameRules C_HeistGameRules
#endif

class CHeistGameRules : public CSharedGameRules
{
public:
	DECLARE_CLASS(CHeistGameRules, CSharedGameRules);
	CHeistGameRules();
	~CHeistGameRules() override;
	const char *Name() override { return V_STRINGIFY(CHeistGameRules); }

#ifdef CLIENT_DLL
	#undef CHeistGameRules
#endif

	DECLARE_NETWORKCLASS();

	const unsigned char *GetEncryptionKey() override;

#ifndef CLIENT_DLL
	virtual IEntityFactory *ProxyFactory();

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

	int GetMissionState() const
	{ return m_nMissionState; }

private:
	friend class CMissionDirector;

	CNetworkVar(int, m_nMissionState);
};

inline CSharedHeistGameRules *HeistGameRules()
{
	return static_cast<CSharedHeistGameRules*>(GameRules());
}

#endif
