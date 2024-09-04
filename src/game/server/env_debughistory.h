//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ENV_DEBUGHISTORY_H
#define ENV_DEBUGHISTORY_H
#pragma once

enum debughistorycategories_t
{
	HISTORY_ENTITY_IO,
	HISTORY_AI_DECISIONS,
	HISTORY_SCENE_PRINT,
	HISTORY_PLAYER_DAMAGE,  // record all damage done to the player

	// Add new categories here

	MAX_HISTORY_CATEGORIES,
};

#define DISABLE_DEBUG_HISTORY

#if defined(DISABLE_DEBUG_HISTORY)
#define ADD_DEBUG_HISTORY( category, line )		((void)0)
#else
#define ADD_DEBUG_HISTORY( category, line )		AddDebugHistoryLine( category, line )
void AddDebugHistoryLine( int iCategory, const char *pszLine );
#endif

void ClearDebugHistory();

#endif // ENV_DEBUGHISTORY_H
