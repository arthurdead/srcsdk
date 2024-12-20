#ifndef HEIST_SHAREDDEFS_H
#define HEIST_SHAREDDEFS_H

#pragma once

#include "shareddefs.h"

//for my sanity i will be calling "heists" as "missions" because the word is too fucking overused on this project

enum MissionState_t : unsigned char
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

#endif
