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

ConVarBase *r_occlusion  =NULL;
ConVarBase *r_shadows = NULL;
ConVarBase *r_shadows_gamecontrol = NULL;
ConVarBase *r_flashlightscissor = NULL;
ConVarBase *mat_fullbright = NULL;
ConVarBase *mat_queue_mode=NULL;
ConVarBase *mat_dxlevel=NULL;
ConVarBase *dev_loadtime_map_elapsed=NULL;
ConVarBase *mat_wireframe=NULL;
ConVarBase *r_drawmodelstatsoverlay=NULL;
ConVarBase *mat_colcorrection_disableentities=NULL;
ConVarBase *r_flashlightdrawdepth=NULL;
ConVarBase *mat_tonemapping_occlusion_use_stencil=NULL;
ConVarBase *mat_dynamic_tonemapping=NULL;
ConVarBase *mat_colorcorrection=NULL;
ConVarBase *mat_show_ab_hdr=NULL;
ConVarBase *r_drawtranslucentworld=NULL;
ConVarBase *mat_motion_blur_enabled=NULL;
ConVarBase *mat_accelerate_adjust_exposure_down=NULL;
ConVarBase *r_DrawBeams=NULL;
ConVarBase *r_drawmodeldecals=NULL;
ConVarBase *mat_slopescaledepthbias_shadowmap=NULL;
ConVarBase *mat_reduceparticles=NULL;
ConVarBase *r_portalsopenall=NULL;
ConVarBase *r_shadowrendertotexture=NULL;
ConVarBase *r_flashlightdepthtexture=NULL;
ConVarBase *r_flashlightdrawfrustumbbox=NULL;
ConVarBase *r_studio_stats=NULL;
ConVarBase *r_flashlight_version2=NULL;
ConVarBase *r_decals=NULL;
ConVarBase *mat_hdr_manual_tonemap_rate=NULL;
ConVarBase *r_visocclusion=NULL;
ConVarBase *mat_depthbias_shadowmap=NULL;
ConVarBase *building_cubemaps=NULL;
ConVarBase *mat_hdr_level=NULL;
ConVarBase *mat_hdr_tonemapscale=NULL;
ConVarBase *r_lod=NULL;
ConVarBase *r_drawmodellightorigin=NULL;
ConVarBase *r_waterforceexpensive=NULL;
ConVarBase *r_waterforcereflectentities=NULL;
ConVarBase *mat_tonemap_algorithm=NULL;
ConVarBase *mat_force_tonemap_scale=NULL;
ConVarBase *cl_interpolate=NULL;
ConVarBase* r_visualizetraces=NULL;
ConVarBase* snd_soundmixer = NULL;
ConVarBase* dsp_volume = NULL;
ConVarBase *cl_updaterate = NULL;
ConVarBase *cl_cmdrate = NULL;
ConVarBase *r_drawentities = NULL;
ConVarBase*r_drawbrushmodels = NULL;
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
ConVarBase *cl_hud_minmode=NULL;

extern void HUDMinModeChangedCallBack( IConVarRef var, const char *pOldString, float flOldValue );

// Register your console variables here
// This gets called one time when the game is initialied
void InitializeClientCvars( void )
{
	// Register cvars here:
	ConVar_Register( FCVAR_CLIENTDLL, &g_ClientConVarAccessor );

	InitializeSharedCvars();

	dev_loadtime_map_elapsed = g_pCVar->FindVarBase("dev_loadtime_map_elapsed");

	cl_interpolate = g_pCVar->FindVarBase("cl_interpolate");
	cl_updaterate = g_pCVar->FindVarBase("cl_updaterate");
	cl_cmdrate = g_pCVar->FindVarBase("cl_cmdrate");

	snd_soundmixer = g_pCVar->FindVarBase("snd_soundmixer");
	dsp_volume = g_pCVar->FindVarBase("dsp_volume");

	if(!g_bTextMode) {
		r_occlusion = g_pCVar->FindVarBase("r_occlusion");
		r_shadows = g_pCVar->FindVarBase("r_shadows");
		r_shadows_gamecontrol = g_pCVar->FindVarBase("r_shadows_gamecontrol");
		r_flashlightscissor = g_pCVar->FindVarBase("r_flashlightscissor");
		mat_fullbright = g_pCVar->FindVarBase("mat_fullbright");
		mat_queue_mode = g_pCVar->FindVarBase("mat_queue_mode");
		mat_dxlevel = g_pCVar->FindVarBase("mat_dxlevel");
		mat_wireframe = g_pCVar->FindVarBase("mat_wireframe");
		r_drawmodelstatsoverlay = g_pCVar->FindVarBase("r_drawmodelstatsoverlay");
		mat_colcorrection_disableentities = g_pCVar->FindVarBase("mat_colcorrection_disableentities");
		r_flashlightdrawdepth = g_pCVar->FindVarBase("r_flashlightdrawdepth");
		mat_tonemapping_occlusion_use_stencil = g_pCVar->FindVarBase("mat_tonemapping_occlusion_use_stencil");
		mat_dynamic_tonemapping = g_pCVar->FindVarBase("mat_dynamic_tonemapping");
		mat_colorcorrection = g_pCVar->FindVarBase("mat_colorcorrection");
		mat_show_ab_hdr = g_pCVar->FindVarBase("mat_show_ab_hdr");
		r_drawtranslucentworld = g_pCVar->FindVarBase("r_drawtranslucentworld");
		mat_motion_blur_enabled = g_pCVar->FindVarBase("mat_motion_blur_enabled");
		mat_accelerate_adjust_exposure_down = g_pCVar->FindVarBase("mat_accelerate_adjust_exposure_down");
		r_DrawBeams = g_pCVar->FindVarBase("r_DrawBeams");
		r_drawmodeldecals = g_pCVar->FindVarBase("r_drawmodeldecals");
		mat_slopescaledepthbias_shadowmap = g_pCVar->FindVarBase("mat_slopescaledepthbias_shadowmap");
		mat_reduceparticles = g_pCVar->FindVarBase("mat_reduceparticles");
		r_portalsopenall = g_pCVar->FindVarBase("r_portalsopenall");
		r_shadowrendertotexture = g_pCVar->FindVarBase("r_shadowrendertotexture");
		r_flashlightdepthtexture = g_pCVar->FindVarBase("r_flashlightdepthtexture");
		r_flashlightdrawfrustumbbox = g_pCVar->FindVarBase("r_flashlightdrawfrustumbbox");
		r_studio_stats = g_pCVar->FindVarBase("r_studio_stats");
		r_flashlight_version2 = g_pCVar->FindVarBase("r_flashlight_version2");
		r_decals = g_pCVar->FindVarBase("r_decals");
		mat_hdr_manual_tonemap_rate = g_pCVar->FindVarBase("mat_hdr_manual_tonemap_rate");
		r_visocclusion = g_pCVar->FindVarBase("r_visocclusion");
		mat_depthbias_shadowmap = g_pCVar->FindVarBase("mat_depthbias_shadowmap");
		building_cubemaps = g_pCVar->FindVarBase("building_cubemaps");
		mat_hdr_level = g_pCVar->FindVarBase("mat_hdr_level");
		mat_hdr_tonemapscale = g_pCVar->FindVarBase("mat_hdr_tonemapscale");
		r_lod = g_pCVar->FindVarBase("r_lod");
		r_drawmodellightorigin = g_pCVar->FindVarBase("r_drawmodellightorigin");
		r_waterforceexpensive = g_pCVar->FindVarBase("r_waterforceexpensive");
		r_waterforcereflectentities = g_pCVar->FindVarBase("r_waterforcereflectentities");
		mat_tonemap_algorithm = g_pCVar->FindVarBase("mat_tonemap_algorithm");
		mat_force_tonemap_scale = g_pCVar->FindVarBase("mat_force_tonemap_scale");
		r_visualizetraces = g_pCVar->FindVarBase("r_visualizetraces");
		r_drawentities = g_pCVar->FindVarBase("r_drawentities");
		r_drawbrushmodels = g_pCVar->FindVarBase("r_drawbrushmodels");
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

		cl_hud_minmode = new ConVar( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );
	}
}

