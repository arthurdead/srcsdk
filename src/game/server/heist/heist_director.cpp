#include "heist_director.h"
#include "heist_gamerules.h"
#include "heist_player.h"
#include "ai_basenpc.h"
#include "ai_memory.h"
#include "con_nprint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_MISSION, "Missions" );

ConVar sv_heist_missionfile("sv_heist_missionfile", "default");

static CMissionDirector s_Director;
CMissionDirector *MissionDirector()
{ return &s_Director; }

CMissionDirector::CMissionDirector()
{
}

void CMissionDirector::MakeMissionLoud()
{
	if(IsMissionLoud())
		return;

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

	m_AssaultTimer.Start();
}

int CMissionDirector::GetMissionState() const
{
	return HeistGameRules()->GetMissionState();
}

void CMissionDirector::mission_casing(const CCommand &args)
{
	if(!UTIL_IsCommandIssuedByServerAdmin()) {
		return;
	}

	if(GetMissionState() == MISSION_STATE_CASING)
		return;

	m_AssaultTimer.Invalidate();
	m_nAssaultFSM = ASSAULT_FSM_INVALID;
	m_iCurrentAssault = -1;
	m_iCurrentSpawner = -1;

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

void CMissionDirector::LevelInitPostEntity()
{
	//TODO!!!! remove this when lobby/waiting for players is supported
	HeistGameRules()->m_nMissionState = MISSION_STATE_CASING;

	m_AssaultTimer.Invalidate();
	m_nAssaultFSM = ASSAULT_FSM_INVALID;
	m_iCurrentAssault = -1;
	m_iCurrentSpawner = -1;

	LoadMissionFile();

	for(int i = 0; i < m_Locations.GetNumStrings(); ++i) {
		MissionLocation &loc = m_Locations[i];

		CBaseEntity *pEntity = g_pEntityList->FindEntityByNameFast( NULL, loc.targetname );
		if( pEntity ) {
			loc.origin = pEntity->GetAbsOrigin();
			loc.angles = pEntity->GetAbsAngles();
		} else {
			Log_Warning(LOG_MISSION, "entity '%s' not found\n", STRING(loc.targetname));
		}
	}
}

void CMissionDirector::FrameUpdatePostEntityThink()
{
	if(!IsMissionLoud())
		return;

	if(m_Assaults.IsEmpty())
		return;

	float time = m_AssaultTimer.GetElapsedTime();

	if(m_nAssaultFSM != ASSAULT_FSM_SWITCH_TO_NEXT) {
		if(m_iCurrentAssault < (m_Assaults.Count()-1)) {
			int next = m_iCurrentAssault+1;
			if(time >= m_Assaults[next].m_StartTime) {
				m_nAssaultFSM = ASSAULT_FSM_SWITCH_TO_NEXT;
			}
		}
	}

	if(m_nAssaultFSM != ASSAULT_FSM_INVALID) {
		if(m_iCurrentAssault == -1 && m_nAssaultFSM == ASSAULT_FSM_SWITCH_TO_NEXT) {
			for(int i = 0; i < m_Assaults.Count(); ++i) {
				if(m_Assaults[i].m_SpawnerSet && !m_Assaults[i].m_SpawnerSet->IsEmpty()) {
					m_iCurrentAssault = i;
					m_iCurrentSpawner = -1;
					m_nAssaultFSM = ASSAULT_FSM_ACTIVE;
				}
			}
		}

		if(m_iCurrentAssault != -1) {
			const MissionAssault &assault = m_Assaults[m_iCurrentAssault];

			IMissionSpawner *spawner = NULL;

			if(m_iCurrentSpawner == -1) {
				m_iCurrentSpawner = 0;
				spawner = (*assault.m_SpawnerSet)[m_iCurrentSpawner];
				spawner->Start();
			} else {
				spawner = (*assault.m_SpawnerSet)[m_iCurrentSpawner];
			}

			spawner->Update();

			if(spawner->IsDone()) {
				if(m_nAssaultFSM == ASSAULT_FSM_SWITCH_TO_NEXT) {
					for(int i = m_iCurrentAssault; i < m_Assaults.Count(); ++i) {
						if(m_Assaults[i].m_SpawnerSet && !m_Assaults[i].m_SpawnerSet->IsEmpty()) {
							m_iCurrentAssault = i;
							m_iCurrentSpawner = -1;
							m_nAssaultFSM = ASSAULT_FSM_ACTIVE;
						}
					}
				} else {
					if(++m_iCurrentSpawner == assault.m_SpawnerSet->Count()) {
						m_iCurrentSpawner = 0;
					}

					spawner = (*assault.m_SpawnerSet)[m_iCurrentSpawner];
					spawner->Start();
				}
			}
		}
	}
}

bool CMissionDirector::Init()
{
	RegisterSpawnerFactory( "BasicSpawner", DefaultSpawnerAllocator<CBasicMissionSpawner> );

	return true;
}

void CMissionDirector::RegisterSpawnerFactory( const char *name, AllocSpawner_t func )
{
	m_SpawnerAllocators[name] = func;
}

bool CMissionDirector::LoadMissionFile()
{
	char missionfile[MAX_PATH];

	KeyValues::AutoDelete misison_kv = new KeyValues("Mission");

	V_snprintf(missionfile, sizeof(missionfile), "maps/%s_mission.txt", STRING(gpGlobals->mapname));
	if(!misison_kv->LoadFromFile(g_pFullFileSystem, missionfile, "BSP")) {
		V_snprintf(missionfile, sizeof(missionfile), "scripts/missions/%s.txt", sv_heist_missionfile.GetString());
		if(!misison_kv->LoadFromFile(g_pFullFileSystem, missionfile, "GAME_NOBSP")) {
			Log_Error(LOG_MISSION,"unable to load mission file\n");
			return false;
		}
	}

	struct ParseSpawnerLater_t
	{
		CUtlString name;
		IMissionSpawner *spawner;
		KeyValues *params;
	};

	CUtlVector< ParseSpawnerLater_t > spawners;

	struct ParseSetLater_t
	{
		CUtlString name;
		CUtlStringList spawners;
	};

	CUtlVector< ParseSetLater_t > spawner_sets;

	struct ParseAssaultLater_t
	{
		float time;
		CUtlString set;
	};

	CUtlVector< ParseAssaultLater_t > assaults;

	FOR_EACH_SUBKEY(misison_kv, mission_section) {
		const char *section_name = mission_section->GetName();
		if(V_stricmp(section_name, "Objectives") == 0) {

		} else if(V_stricmp(section_name, "Locations") == 0) {
			FOR_EACH_SUBKEY(mission_section, loc_kv) {
				MissionLocation loc;

				const char *loc_name = loc_kv->GetName();
				FOR_EACH_VALUE(loc_kv, loc_section) {
					const char *key_name = loc_section->GetName();
					if(V_stricmp(key_name, "Entity") == 0) {
						loc.targetname = AllocPooledString( loc_section->GetString() );
					} else {
						Log_Warning(LOG_MISSION, "'%s': unrecognized key '%s' in location '%s'\n", missionfile, key_name, loc_name);
					}
				}

				m_Locations.Insert( loc_name, Move(loc) );
			}
		} else if(V_stricmp(section_name, "Spawners") == 0) {
			FOR_EACH_SUBKEY(mission_section, spawner_kv) {
				const char *spawner_name = spawner_kv->GetName();
				KeyValues *spawner_section = spawner_kv->GetFirstSubKey();
				if(spawner_section) {
					const char *key_name = spawner_section->GetName();
					UtlSymId_t idx = m_SpawnerAllocators.Find( key_name );
					if(idx != m_SpawnerAllocators.InvalidIndex()) {
						ParseSpawnerLater_t late;
						late.name = spawner_name;
						late.spawner = m_SpawnerAllocators[idx]();
						late.params = spawner_section;
						spawners.AddToTail(Move(late));
					} else {
						Log_Warning(LOG_MISSION, "'%s': unrecognized spawner class '%s'\n", missionfile, key_name);
					}

					if(spawner_section->GetNextKey()) {
						Log_Warning(LOG_MISSION, "'%s': '%s' multiple spawner class\n", missionfile, spawner_name);
					}
				} else {
					Log_Warning(LOG_MISSION, "'%s': '%s' missing spawner class\n", missionfile, spawner_name);
				}
			}
		} else if(V_stricmp(section_name, "SpawnerSets") == 0) {
			FOR_EACH_SUBKEY(mission_section, set_kv) {
				const char *set_name = set_kv->GetName();
				ParseSetLater_t later;
				later.name = set_name;
				FOR_EACH_VALUE(set_kv, spawner_kv) {
					const char *spawner = spawner_kv->GetString();
					later.spawners.CopyAndAddToTail( spawner );
				}
				spawner_sets.AddToTail( Move(later) );
			}
		} else if(V_stricmp(section_name, "Assaults") == 0) {
			FOR_EACH_VALUE(mission_section, assault_kv) {
				ParseAssaultLater_t later;
				later.time = V_atof( assault_kv->GetName() );
				later.set = assault_kv->GetString();
				assaults.AddToTail( Move(later) );
			}
		} else {
			Log_Warning(LOG_MISSION, "'%s': unrecognized section '%s'\n", missionfile, section_name);
		}
	}

	for(int i = 0; i < spawners.Count(); ++i) {
		if(!spawners[i].spawner->Parse( spawners[i].params )) {
			return false;
		}

		m_Spawners.Insert( spawners[i].name, spawners[i].spawner );
	}

	for(int i = 0; i < spawner_sets.Count(); ++i) {
		UtlSymId_t set_idx = m_SpawnerSets.Insert( spawner_sets[i].name );

		for(int j = 0; j < spawner_sets[i].spawners.Count(); ++j) {
			UtlSymId_t spawner_idx = m_Spawners.Find( spawner_sets[i].spawners[j] );
			if(spawner_idx != m_Spawners.InvalidIndex()) {
				m_SpawnerSets[set_idx].AddToTail( m_Spawners[spawner_idx] );
			} else {
				Log_Warning(LOG_MISSION, "'%s': unknown spawner '%s'\n", missionfile, spawner_sets[i].spawners[j] );
			}
		}
	}

	assaults.Sort(
		[](const ParseAssaultLater_t *rhs, const ParseAssaultLater_t *lhs) -> int {
			return (rhs->time > lhs->time);
		}
	);

	for(int i = 0; i < assaults.Count(); ++i) {
		UtlSymId_t set_idx = m_SpawnerSets.Find( assaults[i].set );
		if(set_idx != m_SpawnerSets.InvalidIndex()) {
			MissionAssault assault;
			assault.m_StartTime = assaults[i].time;
			assault.m_SpawnerSet = &m_SpawnerSets[set_idx];
			m_Assaults.AddToTail( Move(assault) );
		} else {
			Log_Warning(LOG_MISSION, "'%s': unknown set '%s'\n", missionfile, assaults[i].set.Get() );
		}
	}

	return true;
}

const MissionLocation *CMissionDirector::FindLocation( const char *name ) const
{
	UtlSymId_t idx = m_Locations.Find(name);
	if(idx == m_Locations.InvalidIndex()) {
		return NULL;
	}

	return &m_Locations[idx];
}

bool CBasicMissionSpawner::Parse(KeyValues *params)
{
	FOR_EACH_TRUE_SUBKEY(params, param_kv) {
		const char *param_name = param_kv->GetName();
		if(V_stricmp(param_name, "Locations") == 0) {
			FOR_EACH_VALUE(param_kv, locs_kv) {
				const char *loc_name = locs_kv->GetString();
				const MissionLocation *loc = MissionDirector()->FindLocation( loc_name );
				if(loc) {
					m_Locations.AddToTail( loc );
				} else {
					Log_Warning(LOG_MISSION, "BasicSpawner: unknown location '%s'\n", loc_name);
				}
			}
		} else if(V_stricmp(param_name, "Entities") == 0) {
			FOR_EACH_TRUE_SUBKEY(param_kv, entity_kv) {
				const char *classname = STRING( AllocPooledString( entity_kv->GetName() ) );

				EntityInfo info;
				info.keyvalues = entity_kv->MakeCopy();

				m_Entities.Insert( classname, Move(info) );
			}
		} else {
			Log_Warning(LOG_MISSION, "BasicSpawner: unknown param '%s'\n", param_name );
		}
	}

	FOR_EACH_VALUE(params, param) {
		Log_Msg(LOG_MISSION,"value %s %s\n", param->GetName(), param->GetString());
	}

	return true;
}

bool CBasicMissionSpawner::IsDone() const
{
	return false;
}

void CBasicMissionSpawner::Start()
{

}

void CBasicMissionSpawner::Update()
{

}
