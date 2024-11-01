//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "game_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CClientDLL_ConVarAccessor : public CDefaultAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		AssertMsg( !pCommand->IsFlagSet(FCVAR_GAMEDLL), "client dll tried to register server con var/command named %s", pCommand->GetName() );
	#endif

		return CDefaultAccessor::RegisterConCommandBase( pCommand );
	}
};

static CClientDLL_ConVarAccessor g_ClientConVarAccessor;

extern void InitializeSharedCvars( void );

ConVar *r_occlusion  =NULL;
ConVar *r_shadows = NULL;
ConVar *r_shadows_gamecontrol = NULL;
ConVar *r_flashlightscissor = NULL;
ConVar *mat_fullbright = NULL;
ConVar *sv_restrict_aspect_ratio_fov=NULL;
ConVar *mat_queue_mode=NULL;
ConVar *mat_dxlevel=NULL;
ConVar *dev_loadtime_map_elapsed=NULL;
ConVar *mat_wireframe=NULL;
ConVar *r_drawmodelstatsoverlay=NULL;
ConVar *mat_colcorrection_disableentities=NULL;
ConVar *r_flashlightdrawdepth=NULL;
ConVar *mat_tonemapping_occlusion_use_stencil=NULL;
ConVar *mat_dynamic_tonemapping=NULL;
ConVar *mat_colorcorrection=NULL;
ConVar *mat_show_ab_hdr=NULL;
ConVar *r_drawtranslucentworld=NULL;
ConVar *mat_motion_blur_enabled=NULL;
ConVar *mat_accelerate_adjust_exposure_down=NULL;
ConVar *r_DrawBeams=NULL;
ConVar *r_drawmodeldecals=NULL;
ConVar *mat_slopescaledepthbias_shadowmap=NULL;
ConVar *mat_reduceparticles=NULL;
ConVar *r_portalsopenall=NULL;
ConVar *r_shadowrendertotexture=NULL;
ConVar *r_flashlightdepthtexture=NULL;
ConVar *r_flashlightdrawfrustumbbox=NULL;
ConVar *r_studio_stats=NULL;
ConVar *r_flashlight_version2=NULL;
ConVar *r_decals=NULL;
ConVar *mat_hdr_manual_tonemap_rate=NULL;
ConVar *r_visocclusion=NULL;
ConVar *mat_depthbias_shadowmap=NULL;
ConVar *building_cubemaps=NULL;
ConVar *mat_hdr_level=NULL;
ConVar *mat_hdr_tonemapscale=NULL;
ConVar *r_lod=NULL;
ConVar *r_drawmodellightorigin=NULL;
ConVar *r_waterforceexpensive=NULL;
ConVar *r_waterforcereflectentities=NULL;
ConVar *mat_tonemap_algorithm=NULL;
ConVar *mat_force_tonemap_scale=NULL;
ConVar *cl_interpolate=NULL;
extern ConVar *cl_hud_minmode;

extern void HUDMinModeChangedCallBack( IConVar *var, const char *pOldString, float flOldValue );

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeClientCvars( void )
{
	// Register cvars here:
	ConVar_Register( FCVAR_CLIENTDLL, &g_ClientConVarAccessor ); 

	InitializeSharedCvars();

	r_occlusion = g_pCVar->FindVar("r_occlusion");
	r_shadows = g_pCVar->FindVar("r_shadows");
	r_shadows_gamecontrol = g_pCVar->FindVar("r_shadows_gamecontrol");
	r_flashlightscissor = g_pCVar->FindVar("r_flashlightscissor");
	mat_fullbright = g_pCVar->FindVar("mat_fullbright");
	sv_restrict_aspect_ratio_fov = g_pCVar->FindVar("sv_restrict_aspect_ratio_fov");
	mat_queue_mode = g_pCVar->FindVar("mat_queue_mode");
	mat_dxlevel = g_pCVar->FindVar("mat_dxlevel");
	dev_loadtime_map_elapsed = g_pCVar->FindVar("dev_loadtime_map_elapsed");
	mat_wireframe = g_pCVar->FindVar("mat_wireframe");
	r_drawmodelstatsoverlay = g_pCVar->FindVar("r_drawmodelstatsoverlay");
	mat_colcorrection_disableentities = g_pCVar->FindVar("mat_colcorrection_disableentities");
	r_flashlightdrawdepth = g_pCVar->FindVar("r_flashlightdrawdepth");
	mat_tonemapping_occlusion_use_stencil = g_pCVar->FindVar("mat_tonemapping_occlusion_use_stencil");
	mat_dynamic_tonemapping = g_pCVar->FindVar("mat_dynamic_tonemapping");
	mat_colorcorrection = g_pCVar->FindVar("mat_colorcorrection");
	mat_show_ab_hdr = g_pCVar->FindVar("mat_show_ab_hdr");
	r_drawtranslucentworld = g_pCVar->FindVar("r_drawtranslucentworld");
	mat_motion_blur_enabled = g_pCVar->FindVar("mat_motion_blur_enabled");
	mat_accelerate_adjust_exposure_down = g_pCVar->FindVar("mat_accelerate_adjust_exposure_down");
	r_DrawBeams = g_pCVar->FindVar("r_DrawBeams");
	r_drawmodeldecals = g_pCVar->FindVar("r_drawmodeldecals");
	mat_slopescaledepthbias_shadowmap = g_pCVar->FindVar("mat_slopescaledepthbias_shadowmap");
	mat_reduceparticles = g_pCVar->FindVar("mat_reduceparticles");
	r_portalsopenall = g_pCVar->FindVar("r_portalsopenall");
	r_shadowrendertotexture = g_pCVar->FindVar("r_shadowrendertotexture");
	r_flashlightdepthtexture = g_pCVar->FindVar("r_flashlightdepthtexture");
	r_flashlightdrawfrustumbbox = g_pCVar->FindVar("r_flashlightdrawfrustumbbox");
	r_studio_stats = g_pCVar->FindVar("r_studio_stats");
	r_flashlight_version2 = g_pCVar->FindVar("r_flashlight_version2");
	r_decals = g_pCVar->FindVar("r_decals");
	mat_hdr_manual_tonemap_rate = g_pCVar->FindVar("mat_hdr_manual_tonemap_rate");
	r_visocclusion = g_pCVar->FindVar("r_visocclusion");
	mat_depthbias_shadowmap = g_pCVar->FindVar("mat_depthbias_shadowmap");
	building_cubemaps = g_pCVar->FindVar("building_cubemaps");
	mat_hdr_level = g_pCVar->FindVar("mat_hdr_level");
	mat_hdr_tonemapscale = g_pCVar->FindVar("mat_hdr_tonemapscale");
	r_lod = g_pCVar->FindVar("r_lod");
	r_drawmodellightorigin = g_pCVar->FindVar("r_drawmodellightorigin");
	r_waterforceexpensive = g_pCVar->FindVar("r_waterforceexpensive");
	r_waterforcereflectentities = g_pCVar->FindVar("r_waterforcereflectentities");
	mat_tonemap_algorithm = g_pCVar->FindVar("mat_tonemap_algorithm");
	mat_force_tonemap_scale = g_pCVar->FindVar("mat_force_tonemap_scale");
	cl_interpolate = g_pCVar->FindVar("cl_interpolate");

	cl_hud_minmode = new ConVar( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );
}

