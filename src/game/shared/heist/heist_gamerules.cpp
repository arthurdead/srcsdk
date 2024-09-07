#include "cbase.h"
#include "heist_gamerules.h"
#include "suspicioner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "dt_shared.h"
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS(CHeistGameRules);

#ifdef GAME_DLL
void *NetProxy_HeistGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CHeistGameRules *pRules = HeistGameRules();
	Assert(pRules);
	return pRules;
}
#else
void NetProxy_HeistGameRules(const RecvProp *pProp, void **pOut, void *pData, int objectID)
{
	CHeistGameRules *pRules = HeistGameRules();
	Assert(pRules);
	*pOut = pRules;
}
#endif

BEGIN_NETWORK_TABLE( CHeistGameRules, DT_HeistGameRules )
	PropBool(PROPINFO(m_bHeistersSpotted)),
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
#undef CHeistGameRules
IMPLEMENT_CLIENTCLASS_NULL(C_HeistGameRules, DT_HeistGameRules, CHeistGameRules)
#define CHeistGameRules C_HeistGameRules
#else
IMPLEMENT_SERVERCLASS( CHeistGameRules, DT_HeistGameRules );
#endif

BEGIN_NETWORK_TABLE( CHeistGameRulesProxy, DT_HeistGameRulesProxy )
	PropDataTable("heist_gamerules_data", 0, 0, &REFERENCE_DATATABLE(DT_HeistGameRules), NetProxy_HeistGameRules)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(HeistGameRulesProxy, DT_HeistGameRulesProxy)

CHeistGameRules::CHeistGameRules()
{
	m_bHeistersSpotted = false;
}

CHeistGameRules::~CHeistGameRules()
{
}

void CHeistGameRules::SetSpotted(bool value)
{
	m_bHeistersSpotted = value;

	if(value) {
		CSuspicioner::ClearAll();
	}
}

#ifdef GAME_DLL
CGameRulesProxy *CHeistGameRules::AllocateProxy()
{ return new CHeistGameRulesProxy(); }

const char* CHeistGameRules::AIClassText(Class_T classType)
{
	switch (classType)
	{
		case CLASS_HEISTER:			return "CLASS_HEISTER";
		case CLASS_CIVILIAN:			return "CLASS_CIVILIAN";
		case CLASS_POLICE:			return "CLASS_POLICE";
		default:					return BaseClass::AIClassText(classType);
	}
}

const char* CHeistGameRules::AIFactionText(Faction_T classType)
{
	switch (classType)
	{
		case FACTION_CIVILIANS:			return "FACTION_CIVILIANS";
		case FACTION_HEISTERS:			return "FACTION_HEISTERS";
		case FACTION_LAW_ENFORCEMENT:			return "FACTION_LAW_ENFORCEMENT";
		default:					return BaseClass::AIFactionText(classType);
	}
}

Team_t CHeistGameRules::GetTeamIndex( const char *pTeamName )
{
	if(FStrEq(pTeamName, "Heisters") ||
		FStrEq(pTeamName, "Robbers")) {
		return TEAM_HEISTERS;
	} else if(FStrEq(pTeamName, "Civilians") ||
				FStrEq(pTeamName, "Pedestrians")) {
		return TEAM_CIVILIANS;
	} else if(FStrEq(pTeamName, "Cops") ||
				FStrEq(pTeamName, "Police") ||
				FStrEq(pTeamName, "LAW") ||
				FStrEq(pTeamName, "Law Enforcement")) {
		return TEAM_POLICE;
	}

	return BaseClass::GetTeamIndex( pTeamName );
}

const char *CHeistGameRules::GetIndexedTeamName( Team_t teamIndex )
{
	switch(teamIndex) {
	case TEAM_HEISTERS:
		return "Heisters";
	case TEAM_CIVILIANS:
		return "Civilians";
	case TEAM_POLICE:
		return "Cops";
	default:
		return BaseClass::GetIndexedTeamName( teamIndex );
	}
}

void CHeistGameRules::InitDefaultAIRelationships()
{
	BaseClass::InitDefaultAIRelationships();

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_HEISTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_POLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_CIVILIAN, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_HEISTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_POLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_CIVILIAN, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_POLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_HEISTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_CIVILIAN, D_LI, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_LAW_ENFORCEMENT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_HEISTERS, D_FR, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_LAW_ENFORCEMENT, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_LAW_ENFORCEMENT, FACTION_LAW_ENFORCEMENT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_LAW_ENFORCEMENT, FACTION_HEISTERS, D_HT, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_CIVILIANS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_LAW_ENFORCEMENT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_HEISTERS, D_LI, 0);
}
#endif
