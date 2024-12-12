#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Engine Cvars
ConVarBase *violence_hgibs=NULL;
ConVarBase *violence_agibs=NULL;
#ifndef SWDS
ConVarBase *dsp_speaker=NULL;
ConVarBase *dsp_room = NULL;
ConVarBase *room_type=NULL;
ConVarBase *closecaption=NULL;
#endif
ConVarBase*	host_timescale=NULL;
ConVarBase *deathmatch=NULL;
ConVarBase *coop=NULL;
ConVarBase	*sv_alternateticks=NULL;
ConVarBase *sv_cheats=NULL;

#ifdef CLIENT_DLL
static void SV_CheatsChanged_f( IConVarRef pConVar, const char *pOldString, float flOldValue )
{
	if ( pConVar.GetInt() == 0 )
	{
		// cheats were disabled, revert all cheat cvars to their default values
		g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );

		DevMsg( "FCVAR_CHEAT cvars reverted to defaults.\n" );
	}
}
#endif

void InitializeSharedCvars( void )
{
#ifndef SWDS
	#ifdef GAME_DLL
	if( !engine->IsDedicatedServer() )
	#endif
	{
		dsp_speaker	= g_pCVar->FindVarBase( "dsp_speaker" );
		dsp_room = g_pCVar->FindVarBase("dsp_room");
		room_type = g_pCVar->FindVarBase("room_type");
	}
#endif

	host_timescale = g_pCVar->FindVarBase("host_timescale");
	host_timescale->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	deathmatch = g_pCVar->FindVarBase("deathmatch");
	deathmatch->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	coop = g_pCVar->FindVarBase("coop");
	coop->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	sv_alternateticks = g_pCVar->FindVarBase("sv_alternateticks");
	sv_alternateticks->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	sv_cheats = g_pCVar->FindVarBase("sv_cheats");
	sv_cheats->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );

#ifdef CLIENT_DLL
	host_timescale = new ConVar( "host_timescale","1.0", FCVAR_CLIENTDLL|FCVAR_REPLICATED, "Prescale the clock by this amount." );
	deathmatch = new ConVar( "deathmatch","0", FCVAR_CLIENTDLL | FCVAR_REPLICATED| FCVAR_NOTIFY, "Running a deathmatch server." );
	coop = new ConVar( "coop","0", FCVAR_CLIENTDLL|FCVAR_REPLICATED | FCVAR_NOTIFY, "Cooperative play." );
	sv_alternateticks = new ConVar( "sv_alternateticks", "0", FCVAR_CLIENTDLL|FCVAR_REPLICATED, "If set, server only simulates entities on even numbered ticks.\n" );
	sv_cheats = new ConVar( "sv_cheats", "0", FCVAR_CLIENTDLL|FCVAR_NOTIFY|FCVAR_REPLICATED, "Allow cheats on server", SV_CheatsChanged_f );
#endif

	violence_hgibs	= g_pCVar->FindVarBase( "violence_hgibs" );
	violence_agibs	= g_pCVar->FindVarBase( "violence_agibs" );

#ifndef SWDS
	if( !g_bTextMode ) {
		closecaption = g_pCVar->FindVarBase("closecaption");
	}
#endif
}
