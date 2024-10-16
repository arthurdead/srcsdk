#ifndef HEIST_SHAREDDEFS_H
#define HEIST_SHAREDDEFS_H

#pragma once

#include "shareddefs.h"

//for my sanity i will be calling "heists" as "missions" because the word is too fucking overused on this project

enum
{
	//lobby/waiting for players
	//nothing can happen
	MISSION_STATE_NONE,

	//npc's don't do suspicion
	//only environment (traps) can make the mission go loud
	MISSION_STATE_CIVILLIAN,

	//npc's do suspicion normally any weird activity the mission will go loud
	MISSION_STATE_CASING,

	//boom pew pew
	MISSION_STATE_LOUD,
};

enum
{
	TEAM_CIVILIANS = TEAM_NEUTRAL,
	TEAM_HEISTERS = FIRST_GAME_TEAM,
	TEAM_POLICE = SECOND_GAME_TEAM,

	NUM_HEIST_TEAMS,
};

enum
{
	CLASS_HEISTER = FIRST_GAME_CLASS,
	CLASS_CIVILIAN,
	CLASS_POLICE,

	NUM_HEIST_ENTITY_CLASSES,
};

enum
{
	FACTION_CIVILIANS = FIRST_GAME_FACTION,
	FACTION_HEISTERS,
	FACTION_LAW_ENFORCEMENT,

	NUM_HEIST_FACTIONS,
};

#endif
