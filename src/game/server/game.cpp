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

void MapCycleFileChangedCallback( IConVarRef var, const char *pOldString, float flOldValue )
{
	if ( Q_stricmp( pOldString, var.GetString() ) != 0 )
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
			defvalue = ( ( ConVarBase * )pCommand)->GetDefault();
		}

		// Link to engine's list instead
		g_pCVar->RegisterConCommand( pCommand );

		// Apply any command-line values.
		const char *pValue = g_pCVar->GetCommandLineValue( pCommand->GetName() );
		if( pValue )
		{
			if ( !pCommand->IsCommand() )
			{
				( ( ConVarBase * )pCommand )->SetValue( pValue );
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
				ConVarBase *var = ( ConVarBase * )pCommand;
				var->SetValue( defvalue );
			}
		}

		return true;
	}
};

static CGameDLL_ConVarAccessor g_ServerConVarAccessor;

#ifndef SWDS
ConVarBase *mat_hdr_tonemapscale=NULL;
ConVarBase *mat_hdr_manual_tonemap_rate=NULL;
ConVarBase *mat_dxlevel=NULL;
#endif
ConVarBase *skill=NULL;
ConVarBase *think_trace_limit=NULL;
#ifndef SWDS
ConVarBase *vcollide_wireframe=NULL;
#endif
ConVarBase *host_thread_mode=NULL;
ConVarBase *hide_server=NULL;
ConVarBase *sv_maxreplay=NULL;
#ifndef SWDS
ConVarBase* r_visualizetraces=NULL;
#endif
ConVarBase *hostip=NULL;
ConVarBase *hostport=NULL;
ConVarBase *sv_minupdaterate=NULL;
ConVarBase *sv_maxupdaterate=NULL;
ConVarBase *sv_client_min_interp_ratio=NULL;
ConVarBase *sv_client_max_interp_ratio=NULL;
#ifndef SWDS
ConVarBase*snd_mixahead = NULL;
#endif

#ifndef SWDS
ConVarBase *cl_hud_minmode=NULL;
ConVarBase*	r_showenvcubemap=NULL;
ConVarBase*	r_eyegloss=NULL;
ConVarBase*	r_eyemove=NULL;
ConVarBase*	r_eyeshift_x=NULL;
ConVarBase*	r_eyeshift_y=NULL;
ConVarBase*	r_eyeshift_z=NULL;
ConVarBase*	r_eyesize=NULL;
ConVarBase*	mat_softwareskin=NULL;
ConVarBase*	r_nohw=NULL;
ConVarBase*	r_nosw=NULL;
ConVarBase*	r_teeth=NULL;
ConVarBase*	r_flex=NULL;
ConVarBase*	r_eyes=NULL;
ConVarBase*	r_skin=NULL;
ConVarBase*	r_maxmodeldecal=NULL;
ConVarBase*	r_modelwireframedecal=NULL;
ConVarBase*	mat_normals=NULL;
ConVarBase*	r_eyeglintlodpixels=NULL;
ConVarBase*	r_rootlod=NULL;
ConVarBase *r_drawentities = NULL;
#endif

extern void InitializeSharedCvars( void );

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeServerCvars( void )
{
	//TODO!!! Arthurdead: this is defined somewhere else??
	ConVarBase *tmp_props_break_max_pieces = g_pCVar->FindVarBase("props_break_max_pieces");
	if(tmp_props_break_max_pieces) {
		tmp_props_break_max_pieces->AddFlags( FCVAR_CLIENTDLL|FCVAR_REPLICATED );
	}

	g_pCVar->FindVarBase("tv_transmitall")->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	g_pCVar->FindVarBase("sv_restrict_aspect_ratio_fov")->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	g_pCVar->FindVarBase("sv_client_predict")->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );

	// Register cvars here:
	ConVar_Register( FCVAR_GAMEDLL, &g_ServerConVarAccessor ); 

	InitializeSharedCvars();

	skill = g_pCVar->FindVarBase("skill");
	host_thread_mode = g_pCVar->FindVarBase( "host_thread_mode" );
	hide_server = g_pCVar->FindVarBase( "hide_server" );
	sv_maxreplay = g_pCVar->FindVarBase( "sv_maxreplay" );

	hostip = g_pCVar->FindVarBase( "hostip" );
	hostport = g_pCVar->FindVarBase( "hostport" );

	sv_minupdaterate = g_pCVar->FindVarBase( "sv_minupdaterate" );
	sv_maxupdaterate = g_pCVar->FindVarBase( "sv_maxupdaterate" );
	sv_client_min_interp_ratio = g_pCVar->FindVarBase( "sv_client_min_interp_ratio" );
	sv_client_min_interp_ratio->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );
	sv_client_max_interp_ratio = g_pCVar->FindVarBase( "sv_client_max_interp_ratio" );
	sv_client_max_interp_ratio->AddFlags( FCVAR_GAMEDLL|FCVAR_REPLICATED );

#ifndef SWDS
	if(!g_bDedicatedServer) {
		snd_mixahead = g_pCVar->FindVarBase("snd_mixahead");
	}
#endif

#ifndef SWDS
	if(!g_bTextMode) {
		mat_hdr_manual_tonemap_rate = g_pCVar->FindVarBase("mat_hdr_manual_tonemap_rate");
		mat_hdr_tonemapscale = g_pCVar->FindVarBase("mat_hdr_tonemapscale");
		mat_dxlevel = g_pCVar->FindVarBase("mat_dxlevel");
		vcollide_wireframe = g_pCVar->FindVarBase("vcollide_wireframe");
		r_visualizetraces = g_pCVar->FindVarBase("r_visualizetraces");
		r_drawentities = g_pCVar->FindVarBase("r_drawentities");
		r_showenvcubemap = g_pCVar->FindVarBase("r_showenvcubemap");
		r_eyegloss = g_pCVar->FindVarBase("r_eyegloss");
		r_eyemove = g_pCVar->FindVarBase("r_eyemove");
		r_eyeshift_x = g_pCVar->FindVarBase("r_eyeshift_x");
		r_eyeshift_y = g_pCVar->FindVarBase("r_eyeshift_y");
		r_eyeshift_z = g_pCVar->FindVarBase("r_eyeshift_z");
		r_eyesize = g_pCVar->FindVarBase("r_eyesize");
		mat_softwareskin = g_pCVar->FindVarBase("mat_softwareskin");
		r_nohw = g_pCVar->FindVarBase("r_nohw");
		r_nosw = g_pCVar->FindVarBase("r_nosw");
		r_teeth = g_pCVar->FindVarBase("r_teeth");
		r_flex = g_pCVar->FindVarBase("r_flex");
		r_eyes = g_pCVar->FindVarBase("r_eyes");
		r_skin = g_pCVar->FindVarBase("r_skin");
		r_maxmodeldecal = g_pCVar->FindVarBase("r_maxmodeldecal");
		r_modelwireframedecal = g_pCVar->FindVarBase("r_modelwireframedecal");
		mat_normals = g_pCVar->FindVarBase("mat_normals");
		r_eyeglintlodpixels = g_pCVar->FindVarBase("r_eyeglintlodpixels");
		r_rootlod = g_pCVar->FindVarBase("r_rootlod");
	} else
#endif
	{
	}

#ifndef SWDS
	if(!g_bTextMode) {
		cl_hud_minmode = g_pCVar->FindVarBase("cl_hud_minmode");
	}
#endif
}
