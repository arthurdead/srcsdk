//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef GAME_H
#define GAME_H

#pragma once

#include "globals.h"
#include "tier1/convar.h"

extern void GameDLLInit( void );

extern ConVar	displaysoundlist;
extern ConVar	mapcyclefile;
extern ConVar	servercfgfile;
extern ConVar	lservercfgfile;

// multiplayer server rules
extern ConVar	teamplay;
extern ConVar	fraglimit;
extern ConVar	falldamage;
extern ConVar	weaponstay;
extern ConVar	forcerespawn;
extern ConVar	footsteps;
extern ConVar	flashlight;
extern ConVar	aimcrosshair;
extern ConVar	decalfrequency;
extern ConVar	teamlist;
extern ConVar	teamoverride;
extern ConVar	defaultteam;
extern ConVar	allowNPCs;

extern ConVar	suitvolume;

// Engine Cvars
extern ConVarBase *developer;
#endif		// GAME_H
