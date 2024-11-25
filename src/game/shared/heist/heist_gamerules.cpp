#include "cbase.h"
#include "heist_gamerules.h"
#include "dt_shared.h"

#ifdef GAME_DLL
#include "heist_player.h"
#include "heist_director.h"
#else
#include "c_heist_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const unsigned char *CSharedHeistGameRules::GetEncryptionKey()
{ return (unsigned char *)"x9Ke0BY7"; }

REGISTER_GAMERULES_CLASS_ALIASED(HeistGameRules);

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
	C_HeistGameRules *pRules = HeistGameRules();
	Assert(pRules);
	*pOut = pRules;
}
#endif

BEGIN_NETWORK_TABLE( CSharedHeistGameRules, DT_HeistGameRules )
	PropInt(PROPINFO(m_nMissionState)),
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
IMPLEMENT_CLIENTCLASS_NULL( C_HeistGameRules, DT_HeistGameRules, CHeistGameRules )
#else
IMPLEMENT_SERVERCLASS( CHeistGameRules, DT_HeistGameRules );
#endif

BEGIN_NETWORK_TABLE( CSharedHeistGameRulesProxy, DT_HeistGameRulesProxy )
	PropDataTable("heist_gamerules_data", 0, 0, &REFERENCE_DATATABLE(DT_HeistGameRules), NetProxy_HeistGameRules)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(HeistGameRulesProxy, DT_HeistGameRulesProxy)

#if defined( CLIENT_DLL )
	#define CHeistGameRules C_HeistGameRules
#endif

CSharedHeistGameRules::CHeistGameRules()
{
	m_nMissionState = MISSION_STATE_NONE;
}

CSharedHeistGameRules::~CHeistGameRules()
{
}

#if defined( CLIENT_DLL )
	#undef CHeistGameRules
#endif

#ifdef GAME_DLL
DEFINE_ENTITY_FACTORY( CSharedHeistGameRulesProxy );

IEntityFactory *CSharedHeistGameRules::ProxyFactory()
{ return &g_CSharedHeistGameRulesProxyFactory; }

const char* CSharedHeistGameRules::AIClassText(Class_T classType)
{
	switch (classType)
	{
		case CLASS_HEISTER:			return "CLASS_HEISTER";
		case CLASS_CIVILIAN:			return "CLASS_CIVILIAN";
		case CLASS_POLICE:			return "CLASS_POLICE";
		case CLASS_RIVALS:			return "CLASS_RIVALS";
		default:					return BaseClass::AIClassText(classType);
	}
}

const char* CSharedHeistGameRules::AIFactionText(Faction_T classType)
{
	switch (classType)
	{
		case FACTION_CIVILIANS:			return "FACTION_CIVILIANS";
		case FACTION_HEISTERS:			return "FACTION_HEISTERS";
		case FACTION_APEX_SECURITY:			return "FACTION_APEX_SECURITY";
		case FACTION_ABBADON_CIRCLE:			return "FACTION_ABBADON_CIRCLE";
		case FACTION_GOLDEN_HOTEL:			return "FACTION_GOLDEN_HOTEL";
		case FACTION_STEEL_N_SPEAR_PMC:			return "FACTION_STEEL_N_SPEAR_PMC";
		case FACTION_NY_SWAT:			return "FACTION_NY_SWAT";
		default:					return BaseClass::AIFactionText(classType);
	}
}

Team_t CSharedHeistGameRules::GetTeamIndex( const char *pTeamName )
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
	} else if(FStrEq(pTeamName, "Rivals")) {
		return TEAM_RIVALS;
	}

	return BaseClass::GetTeamIndex( pTeamName );
}

const char *CSharedHeistGameRules::GetIndexedTeamName( Team_t teamIndex )
{
	switch(teamIndex) {
	case TEAM_HEISTERS:
		return "Heisters";
	case TEAM_CIVILIANS:
		return "Civilians";
	case TEAM_POLICE:
		return "Cops";
	case TEAM_RIVALS:
		return "Rivals";
	default:
		return BaseClass::GetIndexedTeamName( teamIndex );
	}
}

void CSharedHeistGameRules::InitDefaultAIRelationships()
{
	BaseClass::InitDefaultAIRelationships();

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_HEISTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_POLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_CIVILIAN, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEISTER, CLASS_RIVALS, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_HEISTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_POLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_CIVILIAN, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CIVILIAN, CLASS_RIVALS, D_FR, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_POLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_HEISTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_CIVILIAN, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_POLICE, CLASS_RIVALS, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_RIVALS, CLASS_HEISTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_RIVALS, CLASS_POLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_RIVALS, CLASS_CIVILIAN, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_RIVALS, CLASS_RIVALS, D_LI, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_HEISTERS, D_FR, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_APEX_SECURITY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_ABBADON_CIRCLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_GOLDEN_HOTEL, D_FR, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_STEEL_N_SPEAR_PMC, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_CIVILIANS, FACTION_NY_SWAT, D_LI, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_CIVILIANS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_HEISTERS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_APEX_SECURITY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_ABBADON_CIRCLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_GOLDEN_HOTEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_STEEL_N_SPEAR_PMC, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_HEISTERS, FACTION_NY_SWAT, D_HT, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_HEISTERS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_APEX_SECURITY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_ABBADON_CIRCLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_GOLDEN_HOTEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_STEEL_N_SPEAR_PMC, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_APEX_SECURITY, FACTION_NY_SWAT, D_LI, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_CIVILIANS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_HEISTERS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_APEX_SECURITY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_ABBADON_CIRCLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_GOLDEN_HOTEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_STEEL_N_SPEAR_PMC, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_ABBADON_CIRCLE, FACTION_NY_SWAT, D_HT, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_CIVILIANS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_HEISTERS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_APEX_SECURITY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_ABBADON_CIRCLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_GOLDEN_HOTEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_STEEL_N_SPEAR_PMC, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_GOLDEN_HOTEL, FACTION_NY_SWAT, D_HT, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_HEISTERS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_APEX_SECURITY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_ABBADON_CIRCLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_GOLDEN_HOTEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_STEEL_N_SPEAR_PMC, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_STEEL_N_SPEAR_PMC, FACTION_NY_SWAT, D_LI, 0);

	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_CIVILIANS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_HEISTERS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_APEX_SECURITY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_ABBADON_CIRCLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_GOLDEN_HOTEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_STEEL_N_SPEAR_PMC, D_LI, 0);
	CBaseCombatCharacter::SetDefaultFactionRelationship(FACTION_NY_SWAT, FACTION_NY_SWAT, D_LI, 0);
}
#endif
