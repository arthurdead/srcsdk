#ifndef HEIST_SHAREDDEFS_H
#define HEIST_SHAREDDEFS_H

#pragma once

#include "shareddefs.h"

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