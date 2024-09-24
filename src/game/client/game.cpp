//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "game.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Engine Cvars
const ConVar	*g_pDeveloper = NULL;

class CClientDLL_ConVarAccessor : public CDefaultAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		if(pCommand->IsFlagSet(FCVAR_GAMEDLL))
			DevMsg("client dll tried to register server con var/command named %s\n", pCommand->GetName());
	#endif

		return CDefaultAccessor::RegisterConCommandBase( pCommand );
	}
};

static CClientDLL_ConVarAccessor g_ConVarAccessor;

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeCvars( void )
{
	// Register cvars here:
	ConVar_Register( FCVAR_CLIENTDLL, &g_ConVarAccessor ); 

	g_pDeveloper	= cvar->FindVar( "developer" );
}

