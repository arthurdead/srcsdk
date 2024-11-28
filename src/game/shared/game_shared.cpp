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
	deathmatch = g_pCVar->FindVarBase("deathmatch");
	coop = g_pCVar->FindVarBase("coop");
	sv_alternateticks = g_pCVar->FindVarBase("sv_alternateticks");
	sv_cheats = g_pCVar->FindVarBase("sv_cheats");

	violence_hgibs	= g_pCVar->FindVarBase( "violence_hgibs" );
	violence_agibs	= g_pCVar->FindVarBase( "violence_agibs" );

#ifndef SWDS
	if( !g_bTextMode ) {
		closecaption = g_pCVar->FindVarBase("closecaption");
	}
#endif
}
