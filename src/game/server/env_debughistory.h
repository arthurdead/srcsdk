//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ENV_DEBUGHISTORY_H
#define ENV_DEBUGHISTORY_H
#pragma once

#include "baseentity.h"

#define DISABLE_DEBUG_HISTORY

// Number of characters worth of debug to use per history category
#define DEBUG_HISTORY_VERSION			6
#define DEBUG_HISTORY_FIRST_VERSIONED	5
#define MAX_DEBUG_HISTORY_LINE_LENGTH	256
#define MAX_DEBUG_HISTORY_LENGTH		(1000 * MAX_DEBUG_HISTORY_LINE_LENGTH)

enum debughistorycategories_t
{
	HISTORY_ENTITY_IO,
	HISTORY_AI_DECISIONS,
	HISTORY_SCENE_PRINT,
	HISTORY_PLAYER_DAMAGE,  // record all damage done to the player

	// Add new categories here

	MAX_HISTORY_CATEGORIES,
};

//-----------------------------------------------------------------------------
// Purpose: Stores debug history in savegame files for debugging reference
//-----------------------------------------------------------------------------
class CDebugHistory : public CBaseEntity 
{
	DECLARE_CLASS( CDebugHistory, CBaseEntity );
public:
	CDebugHistory();
	~CDebugHistory();

	void	Spawn();
	void	AddDebugHistoryLine( int iCategory, const char *szLine );
	void	ClearHistories( void );
	void	DumpDebugHistory( int iCategory );

private:
	char m_DebugLines[MAX_HISTORY_CATEGORIES][MAX_DEBUG_HISTORY_LENGTH];
	char *m_DebugLineEnd[MAX_HISTORY_CATEGORIES];
};

#if defined(DISABLE_DEBUG_HISTORY)
#define ADD_DEBUG_HISTORY( category, line )		((void)0)
#else
#define ADD_DEBUG_HISTORY( category, line )		AddDebugHistoryLine( category, line )
void AddDebugHistoryLine( int iCategory, const char *pszLine );
#endif

CDebugHistory *GetDebugHistory();

void ClearDebugHistory();

#endif // ENV_DEBUGHISTORY_H
