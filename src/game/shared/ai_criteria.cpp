//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "ai_criteria.h"

#ifdef GAME_DLL
#include "ai_speech.h"
#endif

#include "tier1/KeyValues.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace ResponseRules;

void CriteriaSet::WriteToEntity( CGameBaseEntity *pEntity )
{
	if ( GetCount() < 1 )
		return;

#ifdef GAME_DLL
	for ( int i = Head() ; IsValidIndex(i); i = Next(i) )
	{
		pEntity->AddContext( GetName(i), GetValue(i), 0 );
	}
#endif
}
