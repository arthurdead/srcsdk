//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "game.h"
#include "physics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void MapCycleFileChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( Q_stricmp( pOldString, mapcyclefile.GetString() ) != 0 )
	{
		if ( GameRules() )
		{
			// For multiplayer games, forces the mapcyclefile to be reloaded
			GameRules()->ResetMapCycleTimeStamp();
		}
	}
}

ConVar	displaysoundlist( "displaysoundlist","0" );
ConVar  mapcyclefile( "mapcyclefile", "mapcycle.txt", FCVAR_NONE, "Name of the .txt file used to cycle the maps on multiplayer servers ", MapCycleFileChangedCallback );
ConVar  servercfgfile( "servercfgfile","server.cfg" );
ConVar  lservercfgfile( "lservercfgfile","listenserver.cfg" );

// multiplayer server rules
ConVar	falldamage( "mp_falldamage","0", FCVAR_NOTIFY );
ConVar	weaponstay( "mp_weaponstay","0", FCVAR_NOTIFY );
ConVar	forcerespawn( "mp_forcerespawn","1", FCVAR_NOTIFY );
ConVar	footsteps( "mp_footsteps","1", FCVAR_NOTIFY );
ConVar	flashlight( "mp_flashlight","1", FCVAR_NOTIFY );
ConVar	aimcrosshair( "mp_autocrosshair","1", FCVAR_NOTIFY );
ConVar	decalfrequency( "decalfrequency","10", FCVAR_NOTIFY );
ConVar	teamlist( "mp_teamlist","hgrunt;scientist", FCVAR_NOTIFY );
ConVar	teamoverride( "mp_teamoverride","1" );
ConVar	defaultteam( "mp_defaultteam","0" );
ConVar	allowNPCs( "mp_allowNPCs","1", FCVAR_NOTIFY );

ConVar suitvolume( "suitvolume", "0.25", FCVAR_ARCHIVE );

class CGameDLL_ConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		AssertMsg( g_pCVar->FindCommandBase(pCommand->GetName()) == NULL, "server dll tried to re-register con var/command named %s", pCommand->GetName());
		AssertMsg( !pCommand->IsFlagSet(FCVAR_CLIENTDLL), "server dll tried to register client con var/command named %s", pCommand->GetName() );
	#endif

		// Remember "unlinked" default value for replicated cvars
		bool replicated = pCommand->IsFlagSet( FCVAR_REPLICATED );
		const char *defvalue = NULL;
		if ( replicated && !pCommand->IsCommand() )
		{
			defvalue = ( ( ConVar * )pCommand)->GetDefault();
		}

		// Link to engine's list instead
		g_pCVar->RegisterConCommand( pCommand );

		// Apply any command-line values.
		const char *pValue = g_pCVar->GetCommandLineValue( pCommand->GetName() );
		if( pValue )
		{
			if ( !pCommand->IsCommand() )
			{
				( ( ConVar * )pCommand )->SetValue( pValue );
			}
		}
		else
		{
			// NOTE:  If not overridden at the command line, then if it's a replicated cvar, make sure that it's
			//  value is the server's value.  This solves a problem where think_limit is defined in shared
			//  code but the value is inside and #if defined( _DEBUG ) block and if you have a debug game .dll
			//  and a release client, then the limiit was coming from the client even though the server value 
			//  was the one that was important during debugging.  Now the server trumps the client value for
			//  replicated ConVars by setting the value here after the ConVar has been linked.
			if ( replicated && defvalue && !pCommand->IsCommand() )
			{
				ConVar *var = ( ConVar * )pCommand;
				var->SetValue( defvalue );
			}
		}

		return true;
	}
};

static CGameDLL_ConVarAccessor g_ServerConVarAccessor;

ConVar *mat_hdr_tonemapscale=NULL;
ConVar *mat_hdr_manual_tonemap_rate=NULL;
#ifndef SWDS
ConVar *mat_dxlevel=NULL;
#endif
ConVar *skill=NULL;
ConVar *think_trace_limit=NULL;
ConVar *vcollide_wireframe=NULL;
ConVar *host_thread_mode=NULL;
ConVar *hide_server=NULL;
ConVar *sv_maxreplay=NULL;

extern void InitializeSharedCvars( void );

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeServerCvars( void )
{
	//TODO!!! Arthurdead: this is defined somewhere else??
	ConVar *tmp_props_break_max_pieces = g_pCVar->FindVar("props_break_max_pieces");
	if(tmp_props_break_max_pieces) {
		tmp_props_break_max_pieces->AddFlags( FCVAR_REPLICATED );
	}

	// Register cvars here:
	ConVar_Register( FCVAR_GAMEDLL, &g_ServerConVarAccessor ); 

	InitializeSharedCvars();

	skill = g_pCVar->FindVar("skill");
	host_thread_mode = g_pCVar->FindVar( "host_thread_mode" );
	hide_server = g_pCVar->FindVar( "hide_server" );
	sv_maxreplay = g_pCVar->FindVar( "sv_maxreplay" );

#ifndef SWDS
	if(!engine->IsDedicatedServer()) {
		mat_hdr_tonemapscale = g_pCVar->FindVar("mat_hdr_tonemapscale");
		mat_hdr_manual_tonemap_rate = g_pCVar->FindVar("mat_hdr_manual_tonemap_rate");
		mat_dxlevel = g_pCVar->FindVar("mat_dxlevel");
		vcollide_wireframe = g_pCVar->FindVar("vcollide_wireframe");
	} else
#endif
	{
		mat_hdr_tonemapscale = new ConVar( "mat_hdr_tonemapscale", "1.0", FCVAR_CHEAT|FCVAR_REPLICATED, "The HDR tonemap scale. 1 = Use autoexposure, 0 = eyes fully closed, 16 = eyes wide open." );
		mat_hdr_manual_tonemap_rate = new ConVar( "mat_hdr_manual_tonemap_rate", "1.0" );
		vcollide_wireframe = new ConVar( "vcollide_wireframe", "0", FCVAR_CHEAT, "Render physics collision models in wireframe", NULL );
	}
}
