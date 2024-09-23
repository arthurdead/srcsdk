//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module implements functions to support ehandles.
//
//=============================================================================//

#include "cbase.h"
#include "ehandle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CBaseEntityList *g_pEntityList;

IHandleEntity* CBaseHandle::Get() const
{
	return g_pEntityList->LookupEntity( *this );
}

#if defined( GAME_DLL )
	
	#include "entitylist.h"


	void DebugCheckEHandleAccess( void *pEnt )
	{
		extern bool g_bDisableEhandleAccess;

		if ( g_bDisableEhandleAccess )
		{
			Msg( "Access of EHANDLE/CHandle for class %s:%p in destructor!\n",
				((CBaseEntity*)pEnt)->GetClassname(), pEnt );
		}
	}

#else
	
#endif


