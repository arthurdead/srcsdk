#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Engine Cvars
ConVar *violence_hgibs=NULL;
ConVar *violence_agibs=NULL;
#ifndef SWDS
ConVar *dsp_speaker=NULL;
ConVar *dsp_room = NULL;
ConVar *room_type=NULL;
#endif
ConVar*	host_timescale=NULL;
ConVar* r_visualizetraces=NULL;
ConVar *closecaption=NULL;
ConVar *deathmatch=NULL;
ConVar *coop=NULL;
ConVar	*sv_alternateticks=NULL;
ConVar *sv_cheats=NULL;

void InitializeSharedCvars( void )
{
	violence_hgibs	= g_pCVar->FindVar( "violence_hgibs" );
	violence_agibs	= g_pCVar->FindVar( "violence_agibs" );
#ifndef SWDS
	#ifdef GAME_DLL
	if( !engine->IsDedicatedServer() )
	#endif
	{
		dsp_speaker	= g_pCVar->FindVar( "dsp_speaker" );
		dsp_room = g_pCVar->FindVar("dsp_room");
		room_type = g_pCVar->FindVar("room_type");
	}
#endif
	host_timescale = g_pCVar->FindVar("host_timescale");
	r_visualizetraces = g_pCVar->FindVar("r_visualizetraces");
	closecaption = g_pCVar->FindVar("closecaption");
	deathmatch = g_pCVar->FindVar("deathmatch");
	coop = g_pCVar->FindVar("coop");
	sv_alternateticks = g_pCVar->FindVar("sv_alternateticks");
	sv_cheats = g_pCVar->FindVar("sv_cheats");
}
