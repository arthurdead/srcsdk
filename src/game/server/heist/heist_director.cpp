#include "heist_director.h"
#include "heist_gamerules.h"
#include "heist_player.h"
#include "ai_basenpc.h"
#include "ai_memory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMissionDirector s_Director;
CMissionDirector *MissionDirector()
{ return &s_Director; }

void CMissionDirector::MakeMissionLoud()
{
	HeistGameRules()->m_nMissionState = MISSION_STATE_LOUD;

	//make all players heisters
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(i);
		if(!pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != TEAM_CIVILIANS) {
			continue;
		}

		pPlayer->SetPreventWeaponPickup(false);
		pPlayer->ChangeTeam(TEAM_HEISTERS, false, true);
		pPlayer->ChangeFaction(FACTION_HEISTERS);
	}

	CAI_BaseNPC **ppNpcs = g_AI_Manager.AccessAIs();

	for(int i = 0; i < g_AI_Manager.NumAIs(); ++i) {
		//loop all non-corrupt npc's

		if(ppNpcs[i]->GetState() == NPC_STATE_DEAD || ppNpcs[i]->GetTeamNumber() == TEAM_HEISTERS) {
			continue;
		}

		//add all corrupt cops to memory
		for(int j = 0; j < g_AI_Manager.NumAIs(); ++j) {
			if(ppNpcs[j]->GetState() == NPC_STATE_DEAD || ppNpcs[j]->GetTeamNumber() != TEAM_HEISTERS) {
				continue;
			}

			ppNpcs[i]->GetEnemies()->UpdateMemory(ppNpcs[j], ppNpcs[j]->GetAbsOrigin(), 0.0f, true);
		}

		//add heisters to memory
		for(int j = 1; j <= gpGlobals->maxClients; ++j) {
			CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(j);
			if(!pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != TEAM_HEISTERS) {
				continue;
			}

			ppNpcs[i]->GetEnemies()->UpdateMemory(pPlayer, pPlayer->GetAbsOrigin(), 0.0f, true);
		}
	}
}

int CMissionDirector::GetMissionState() const
{
	return HeistGameRules()->m_nMissionState;
}

void CMissionDirector::mission_casing(const CCommand &args)
{
	if(!UTIL_IsCommandIssuedByServerAdmin()) {
		return;
	}

	CAI_BaseNPC **ppNpcs = g_AI_Manager.AccessAIs();

	//remove all corrupt cops
	for(int i = 0; i < g_AI_Manager.NumAIs(); ++i) {
		if(ppNpcs[i]->GetTeamNumber() == TEAM_HEISTERS) {
			UTIL_Remove(ppNpcs[i]);
		}
	}

	//disguise heisters as civilians
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)UTIL_PlayerByIndex(i);
		if(!pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != TEAM_HEISTERS) {
			continue;
		}

		pPlayer->ChangeTeam(TEAM_CIVILIANS, false, true);
		pPlayer->ChangeFaction(FACTION_CIVILIANS);
	}

	//reset everyones memories
	for(int i = 0; i < g_AI_Manager.NumAIs(); ++i) {
		if(ppNpcs[i]->GetState() == NPC_STATE_DEAD || ppNpcs[i]->IsMarkedForDeletion()) {
			continue;
		}

		ppNpcs[i]->GetEnemies()->RefreshMemories();
	}

	HeistGameRules()->m_nMissionState = MISSION_STATE_CASING;
}

void CMissionDirector::mission_loud(const CCommand &args)
{
	MakeMissionLoud();
}

void CMissionDirector::PlayerSpawned(CHeistPlayer *pPlayer)
{
	
}