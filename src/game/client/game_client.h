//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#pragma once

class ConVar;

#ifndef __MINGW32__
typedef ConVar ConVarBase;
#else
class ConVarBase;
#endif

extern void GameDLLInit( void );

// Engine Cvars
extern ConVarBase *developer;
#endif		// GAME_H
