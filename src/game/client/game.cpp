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

class CClientDLL_ConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		if(cvar->FindCommandBase(pCommand->GetName()) != NULL) {
			DevMsg("client dll tried to re-register con var/command named %s\n", pCommand->GetName());
		}
	#endif

		// Link to engine's list instead
		cvar->RegisterConCommand( pCommand );

		// Apply any command-line values.
		const char *pValue = cvar->GetCommandLineValue( pCommand->GetName() );
		if( pValue )
		{
			if ( !pCommand->IsCommand() )
			{
				( ( ConVar * )pCommand )->SetValue( pValue );
			}
		}

		return true;
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

