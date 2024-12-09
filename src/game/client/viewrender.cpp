//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Responsible for drawing the scene
//
//===========================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "model_types.h"
#include "clientsideeffects.h"
#include "particlemgr.h"
#include "viewrender.h"
#include "iclientmode.h"
#include "voice_status.h"
#include "glow_overlay.h"
#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialsystem.h"
#include "detailobjectsystem.h"
#include "tier0/vprof.h"
#include "tier1/mempool.h"
#include "vstdlib/jobthread.h"
#include "datacache/imdlcache.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "tier0/icommandline.h"
#include "view_scene.h"
#include "particles_ez.h"
#include "engine/IStaticPropMgr.h"
#include "engine/ivdebugoverlay.h"
#include "c_pixel_visibility.h"
#include "clienteffectprecachesystem.h"
#include "c_rope.h"
#include "c_effects.h"
#include "smoke_fog_overlay.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "ScreenSpaceEffects.h"
#include "toolframework_client.h"
#include "c_func_reflective_glass.h"
#include "KeyValues.h"
#include "renderparm.h"
#include "studio_stats.h"
#include "con_nprint.h"
#include "clientmode_shared.h"
#include "modelrendersystem.h"
#include "mapbase/c_func_fake_worldportal.h"

#ifdef PORTAL
//#include "C_Portal_Player.h"
#include "portal_render_targets.h"
#include "PortalRender.h"
#endif

#include "rendertexture.h"
#include "viewpostprocess.h"
#include "viewdebug.h"

#include "c_point_camera.h"

// Projective textures
#include "c_env_projectedtexture.h"

#include "c_lights.h"

#include "colorcorrectionmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar csm_color_light( "csm_color_light", "0" );
static ConVar csm_color_ambient( "csm_color_ambient", "0" );

static Vector ConvertLightmapGammaToLinear( int *iColor4 )
{
	Vector vecColor;
	for ( int i = 0; i < 3; ++i )
	{
		vecColor[i] = powf( iColor4[i] / 255.0f, 2.2f );
	}
	vecColor *= iColor4[3] / 255.0f;
	return vecColor;
}

static void testfreezeframe_f( void )
{
	GetViewRenderInstance()->FreezeFrame( 3.0 );
}
static ConCommand test_freezeframe( "test_freezeframe", testfreezeframe_f, "Test the freeze frame code.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------

extern ConVarBase *r_visocclusion;
extern ConVarBase *r_flashlightdepthtexture;
extern ConVar vcollide_wireframe;
extern ConVarBase *mat_motion_blur_enabled;
extern ConVar r_depthoverlay;
extern ConVar mat_viewportscale;
extern ConVar mat_viewportupscale;
extern bool g_bDumpRenderTargets;

//-----------------------------------------------------------------------------
// Convars related to controlling rendering
//-----------------------------------------------------------------------------
ConVar cl_maxrenderable_dist("cl_maxrenderable_dist", "3000", FCVAR_CHEAT, "Max distance from the camera at which things will be rendered" );

ConVar r_entityclips( "r_entityclips", "1" ); //FIXME: Nvidia drivers before 81.94 on cards that support user clip planes will have problems with this, require driver update? Detect and disable?

// Matches the version in the engine
ConVar r_drawopaqueworld( "r_drawopaqueworld", "1", FCVAR_CHEAT );
extern ConVarBase *r_drawtranslucentworld;
ConVar r_3dsky( "r_3dsky","1", 0, "Enable the rendering of 3d sky boxes" );
ConVar r_skybox( "r_skybox","1", FCVAR_CHEAT, "Enable the rendering of sky boxes" );
ConVar r_drawviewmodel( "r_drawviewmodel","1", FCVAR_CHEAT );
ConVar r_drawtranslucentrenderables( "r_drawtranslucentrenderables", "1", FCVAR_CHEAT );
ConVar r_drawopaquerenderables( "r_drawopaquerenderables", "1", FCVAR_CHEAT );
static ConVar r_threaded_renderables( "r_threaded_renderables", "1" );
static ConVar r_skybox_use_complex_views( "r_skybox_use_complex_views", "0", FCVAR_CHEAT, "Enable complex views in skyboxes, like reflective glass" );
static ConVar r_nearz_skybox( "r_nearz_skybox", "2.0", FCVAR_CHEAT );


// FIXME: This is not static because we needed to turn it off for TF2 playtests
ConVar r_DrawDetailProps( "r_DrawDetailProps", "0", FCVAR_NONE, "0=Off, 1=Normal, 2=Wireframe" );

ConVar r_worldlistcache( "r_worldlistcache", "1" );

static ConVar r_flashlightdepth_drawtranslucents( "r_flashlightdepth_drawtranslucents", "0", FCVAR_NONE );

ConVar r_flashlightvolumetrics( "r_flashlightvolumetrics", "1" );

ConVar mat_ambient_light_r_forced( "mat_ambient_light_r_forced", "-1.0" );
ConVar mat_ambient_light_g_forced( "mat_ambient_light_g_forced", "-1.0" );
ConVar mat_ambient_light_b_forced( "mat_ambient_light_b_forced", "-1.0" );

//-----------------------------------------------------------------------------
// Convars related to fog color
//-----------------------------------------------------------------------------
static void GetFogColor( fogparams_t *pFogParams, float *pColor, bool ignoreOverride = false, bool ignoreHDRColorScale = false );
static float GetFogMaxDensity( fogparams_t *pFogParams, bool ignoreOverride = false );
static bool GetFogEnable( fogparams_t *pFogParams, bool ignoreOverride = false );
static float GetFogStart( fogparams_t *pFogParams, bool ignoreOverride = false );
static float GetFogEnd( fogparams_t *pFogParams, bool ignoreOverride = false );
float GetSkyboxFogStart( bool ignoreOverride = false );
static float GetSkyboxFogEnd( bool ignoreOverride = false );
float GetSkyboxFogMaxDensity( bool ignoreOverride = false );
void GetSkyboxFogColor( float *pColor, bool ignoreOverride = false, bool ignoreHDRColorScale = false );
static void FogOverrideCallback( IConVarRef pConVar, char const *, float );
static ConVar fog_override( "fog_override", "0", FCVAR_CHEAT, "", FogOverrideCallback );
// set any of these to use the maps fog
static ConVar fog_start( "fog_start", "-1", FCVAR_CHEAT );
static ConVar fog_end( "fog_end", "-1", FCVAR_CHEAT );
static ConVar fog_color( "fog_color", "-1 -1 -1", FCVAR_CHEAT );
static ConVar fog_enable( "fog_enable", "1", FCVAR_CHEAT );
static ConVar fog_startskybox( "fog_startskybox", "-1", FCVAR_CHEAT );
static ConVar fog_endskybox( "fog_endskybox", "-1", FCVAR_CHEAT );
static ConVar fog_maxdensityskybox( "fog_maxdensityskybox", "-1", FCVAR_CHEAT );
static ConVar fog_colorskybox( "fog_colorskybox", "-1 -1 -1", FCVAR_CHEAT );
ConVar fog_enableskybox( "fog_enableskybox", "1", FCVAR_CHEAT );
static ConVar fog_maxdensity( "fog_maxdensity", "-1", FCVAR_CHEAT );
static ConVar fog_hdrcolorscale( "fog_hdrcolorscale", "-1", FCVAR_CHEAT );
static ConVar fog_hdrcolorscaleskybox( "fog_hdrcolorscaleskybox", "-1", FCVAR_CHEAT );

static void FogOverrideCallback( IConVarRef pConVar, char const *, float )
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !localPlayer )
		return;

	if ( pConVar.GetInt() == -1 )
	{
		fogparams_t *pFogParams = localPlayer->GetFogParams();

		float fogColor[3];
		fog_start.SetValue( GetFogStart( pFogParams, true ) );
		fog_end.SetValue( GetFogEnd( pFogParams, true ) );
		GetFogColor( pFogParams, fogColor, true, true );
		fog_color.SetValue( VarArgs( "%.1f %.1f %.1f", fogColor[0]*255, fogColor[1]*255, fogColor[2]*255 ) );
		fog_enable.SetValue( GetFogEnable( pFogParams, true ) );
		fog_startskybox.SetValue( GetSkyboxFogStart( true ) );
		fog_endskybox.SetValue( GetSkyboxFogEnd( true ) );
		fog_maxdensityskybox.SetValue( GetSkyboxFogMaxDensity( true ) );
		GetSkyboxFogColor( fogColor, true, true );
		fog_colorskybox.SetValue( VarArgs( "%.1f %.1f %.1f", fogColor[0]*255, fogColor[1]*255, fogColor[2]*255 ) );
		fog_enableskybox.SetValue( localPlayer->m_Local.m_skybox3d.fog.enable.Get() );
		fog_maxdensity.SetValue( GetFogMaxDensity( pFogParams, true ) );
		fog_hdrcolorscale.SetValue( pFogParams->HDRColorScale );
		fog_hdrcolorscaleskybox.SetValue( localPlayer->m_Local.m_skybox3d.fog.HDRColorScale.Get() );
	}
}

//-----------------------------------------------------------------------------
// Water-related convars
//-----------------------------------------------------------------------------
static ConVar r_debugcheapwater( "r_debugcheapwater", "0", FCVAR_CHEAT );
extern ConVarBase *r_waterforceexpensive;
extern ConVarBase *r_waterforcereflectentities;
static ConVar r_WaterDrawRefraction( "r_WaterDrawRefraction", "1", 0, "Enable water refraction" );
static ConVar r_WaterDrawReflection( "r_WaterDrawReflection", "1", 0, "Enable water reflection" );
ConVar r_ForceWaterLeaf( "r_ForceWaterLeaf", "1", 0, "Enable for optimization to water - considers view in leaf under water for purposes of culling" );
static ConVar mat_drawwater( "mat_drawwater", "1", FCVAR_CHEAT );
ConVar mat_clipz( "mat_clipz", "1" );


//-----------------------------------------------------------------------------
// Other convars
//-----------------------------------------------------------------------------
static ConVar r_screenfademinsize( "r_screenfademinsize", "0" );
static ConVar r_screenfademaxsize( "r_screenfademaxsize", "0" );
static ConVar cl_drawmonitors( "cl_drawmonitors", "1" );
ConVar r_eyewaterepsilon( "r_eyewaterepsilon", "10.0", FCVAR_CHEAT );

#ifdef TF_CLIENT_DLL
static ConVar pyro_dof( "pyro_dof", "1", FCVAR_ARCHIVE );
#endif

extern ConVarBase *r_fastzreject;

extern ConVar cl_leveloverview;

extern ConVar localplayer_visionflags;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
static Vector g_vecCurrentRenderOrigin(0,0,0);
static QAngle g_vecCurrentRenderAngles(0,0,0);
static Vector g_vecCurrentVForward(0,0,0), g_vecCurrentVRight(0,0,0), g_vecCurrentVUp(0,0,0);
static VMatrix g_matCurrentCamInverse;
bool s_bCanAccessCurrentView = false;
IntroData_t *g_pIntroData = NULL;
static bool	g_bRenderingView = false;			// For debugging...
int g_CurrentViewID = VIEW_NONE;
bool g_bRenderingScreenshot = false;

static FrustumCache_t s_FrustumCache;
FrustumCache_t *FrustumCache( void )
{
	return &s_FrustumCache;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void FrustumCache_t::Add( const CViewSetupEx *pView )
{
	// Check for a valid view setup.
	if ( !pView )
		return;

	// Create the perspective frustum.
	GeneratePerspectiveFrustum( pView->origin, pView->angles, pView->zNear, pView->zFar, pView->fov, pView->m_flAspectRatio, m_Frustums );
}

#define FREEZECAM_SNAPSHOT_FADE_SPEED 340
float g_flFreezeFlash = 0.0f;

//-----------------------------------------------------------------------------

CON_COMMAND( r_cheapwaterstart,  "" )
{
	if( args.ArgC() == 2 )
	{
		float dist = atof( args[ 1 ] );
		GetViewRenderInstance()->SetCheapWaterStartDistance( dist );
	}
	else
	{
		float start, end;
		GetViewRenderInstance()->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterstart: %f\n", start );
	}
}

CON_COMMAND( r_cheapwaterend,  "" )
{
	if( args.ArgC() == 2 )
	{
		float dist = atof( args[ 1 ] );
		GetViewRenderInstance()->SetCheapWaterEndDistance( dist );
	}
	else
	{
		float start, end;
		GetViewRenderInstance()->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterend: %f\n", end );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CWorldListCache
{
public:
	CWorldListCache()
	{

	}
	void Flush()
	{
		for ( int i = m_Entries.FirstInorder(); i != m_Entries.InvalidIndex(); i = m_Entries.NextInorder( i ) )
		{
			delete m_Entries[i];
		}
		m_Entries.RemoveAll();
	}

	bool Find( const CViewSetupEx &viewSetup, IWorldRenderList **ppList, ClientWorldListInfo_t **ppListInfo )
	{
		Entry_t lookup( viewSetup );

		int i = m_Entries.Find( &lookup );

		if ( i != m_Entries.InvalidIndex() )
		{
			Entry_t *pEntry = m_Entries[i];
			*ppList = InlineAddRef( pEntry->pList );
			*ppListInfo = InlineAddRef( pEntry->pListInfo );
			return true;
		}

		return false;
	}

	void Add( const CViewSetupEx &viewSetup, IWorldRenderList *pList, ClientWorldListInfo_t *pListInfo )
	{
		m_Entries.Insert( new Entry_t( viewSetup, pList, pListInfo ) );
	}

private:
	struct Entry_t
	{
		Entry_t( const CViewSetupEx &viewSetup, IWorldRenderList *pList = NULL, ClientWorldListInfo_t *pListInfo = NULL ) :
			pList( ( pList ) ? InlineAddRef( pList ) : NULL ),
			pListInfo( ( pListInfo ) ? InlineAddRef( pListInfo ) : NULL )
		{
            // @NOTE (toml 8/18/2006): because doing memcmp, need to fill all of the fields and the padding!
			memset( &m_bOrtho, 0, offsetof(Entry_t, pList ) - offsetof(Entry_t, m_bOrtho ) );
			m_bOrtho = viewSetup.m_bOrtho;			
			m_OrthoLeft = viewSetup.m_OrthoLeft;		
			m_OrthoTop = viewSetup.m_OrthoTop;
			m_OrthoRight = viewSetup.m_OrthoRight;
			m_OrthoBottom = viewSetup.m_OrthoBottom;
			fov = viewSetup.fov;				
			origin = viewSetup.origin;					
			angles = viewSetup.angles;				
			zNear = viewSetup.zNear;			
			zFar = viewSetup.zFar;			
			m_flAspectRatio = viewSetup.m_flAspectRatio;
			m_bOffCenter = viewSetup.m_bOffCenter;
			m_flOffCenterTop = viewSetup.m_flOffCenterTop;
			m_flOffCenterBottom = viewSetup.m_flOffCenterBottom;
			m_flOffCenterLeft = viewSetup.m_flOffCenterLeft;
			m_flOffCenterRight = viewSetup.m_flOffCenterRight;
		}

		~Entry_t()
		{
			if ( pList )
				pList->Release();
			if ( pListInfo )
				pListInfo->Release();
		}

		// The fields from CViewSetupEx that would actually affect the list
		float	m_OrthoLeft;		
		float	m_OrthoTop;
		float	m_OrthoRight;
		float	m_OrthoBottom;
		float	fov;				
		Vector	origin;					
		QAngle	angles;				
		float	zNear;			
		float	zFar;			
		float	m_flAspectRatio;
		float	m_flOffCenterTop;
		float	m_flOffCenterBottom;
		float	m_flOffCenterLeft;
		float	m_flOffCenterRight;
		bool	m_bOrtho;			
		bool	m_bOffCenter;

		IWorldRenderList *pList;
		ClientWorldListInfo_t *pListInfo;
	};

	class CEntryComparator
	{
	public:
		CEntryComparator( int ) {}
		bool operator!() const { return false; }
		bool operator()( const Entry_t *lhs, const Entry_t *rhs ) const 
		{ 
			return ( memcmp( lhs, rhs, sizeof(Entry_t) - ( sizeof(Entry_t) - offsetof(Entry_t, pList ) ) ) < 0 );
		}
	};

	CUtlRBTree<Entry_t *, unsigned short, CEntryComparator> m_Entries;
};

CWorldListCache g_WorldListCache;

void FlushWorldLists()
{
	g_WorldListCache.Flush();
}

//-----------------------------------------------------------------------------
// Standard 3d skybox view
//-----------------------------------------------------------------------------
class CSkyboxView : public CRendering3dView
{
public:
	DECLARE_CLASS( CSkyboxView, CRendering3dView );
	CSkyboxView(CViewRender *pMainView) : 
		CRendering3dView( pMainView ),
		m_pSky3dParams( NULL )
	  {
	  }

	bool			Setup( const CViewSetupEx &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible );
	void			Draw();

protected:

#ifdef PORTAL
	virtual bool ShouldDrawPortals() { return false; }
#endif

	virtual SkyboxVisibility_t	ComputeSkyboxVisibility();

	bool			GetSkyboxFogEnable();

	void			Enable3dSkyboxFog( void );
	void			DrawInternal( view_id_t iSkyBoxViewID = VIEW_3DSKY, bool bInvokePreAndPostRender = true, ITexture *pRenderTarget = NULL, ITexture *pDepthTarget = NULL );

	sky3dparams_t *	PreRender3dSkyboxWorld( SkyboxVisibility_t nSkyboxVisible );

	sky3dparams_t *m_pSky3dParams;
};

//-----------------------------------------------------------------------------
// 3d skybox view when drawing portals
//-----------------------------------------------------------------------------
#ifdef PORTAL
class CPortalSkyboxView : public CSkyboxView
{
	DECLARE_CLASS( CPortalSkyboxView, CSkyboxView );
public:
	CPortalSkyboxView(CViewRender *pMainView) : 
	  CSkyboxView( pMainView ),
		  m_pRenderTarget( NULL )
	  {}

	  bool			Setup( const CViewSetupEx &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible, ITexture *pRenderTarget = NULL );

	  //Skybox drawing through portals with workarounds to fix area bits, position/scaling, view id's..........
	  void			Draw();

private:
	virtual SkyboxVisibility_t	ComputeSkyboxVisibility();

	ITexture *m_pRenderTarget;
};
#endif


//-----------------------------------------------------------------------------
// Shadow depth texture
//-----------------------------------------------------------------------------
class CShadowDepthView : public CRendering3dView
{
public:
	DECLARE_CLASS( CShadowDepthView, CRendering3dView );
	CShadowDepthView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	void Setup( const CViewSetupEx &shadowViewIn, ITexture *pRenderTarget, ITexture *pDepthTexture );
	void Draw();

private:
	ITexture *m_pRenderTarget;
	ITexture *m_pDepthTexture;
};

//-----------------------------------------------------------------------------
// Freeze frame. Redraws the frame at which it was enabled.
//-----------------------------------------------------------------------------
class CFreezeFrameView : public CRendering3dView
{
public:
	DECLARE_CLASS( CFreezeFrameView, CRendering3dView );
	CFreezeFrameView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	void Setup( const CViewSetupEx &view );
	void Draw();

private:
	CMaterialReference m_pFreezeFrame;
	CMaterialReference m_TranslucentSingleColor;

	int					m_nSubRect[ 4 ];
	int					m_nScreenSize[ 2 ];
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CBaseWorldView : public CRendering3dView
{
public:
	DECLARE_CLASS( CBaseWorldView, CRendering3dView );
protected:
	CBaseWorldView(CViewRender *pMainView) : CRendering3dView( pMainView ) {}

	virtual bool	AdjustView( float waterHeight );

	void			DrawSetup( float waterHeight, int flags, float waterZAdjust, int iForceViewLeaf = -1 );
	void			DrawExecute( float waterHeight, view_id_t viewID, float waterZAdjust );

	virtual void	PushView( float waterHeight );
	virtual void	PopView();

	void			SSAO_DepthPass();
	void			DrawDepthOfField();

	virtual ITexture	*GetRefractionTexture() { return GetWaterRefractionTexture(); }
	virtual ITexture	*GetReflectionTexture() { return GetWaterReflectionTexture(); }
};


//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
class CSimpleWorldView : public CBaseWorldView
{
public:
	DECLARE_CLASS( CSimpleWorldView, CBaseWorldView );
	CSimpleWorldView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

	void			Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info, ViewCustomVisibility_t *pCustomVisibility = NULL );
	void			Draw();

private: 
	VisibleFogVolumeInfo_t m_fogInfo;

};


//-----------------------------------------------------------------------------
// Base class for scenes with water
//-----------------------------------------------------------------------------
class CBaseWaterView : public CBaseWorldView
{
public:
	DECLARE_CLASS( CBaseWaterView, CBaseWorldView );
	CBaseWaterView(CViewRender *pMainView) : 
		CBaseWorldView( pMainView ),
		m_SoftwareIntersectionView( pMainView )
	{}

	//	void Setup( const CViewSetupEx &, const WaterRenderInfo_t& info );

protected:
	void			CalcWaterEyeAdjustments( const VisibleFogVolumeInfo_t &fogInfo, float &newWaterHeight, float &waterZAdjust, bool bSoftwareUserClipPlane );

	class CSoftwareIntersectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CSoftwareIntersectionView, CBaseWorldView );
	public:
		CSoftwareIntersectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup( bool bAboveWater );
		void Draw();

	private: 
		CBaseWaterView *GetOuter() { return GET_OUTER( CBaseWaterView, m_SoftwareIntersectionView ); }
	};

	friend class CSoftwareIntersectionView;

	CSoftwareIntersectionView m_SoftwareIntersectionView;

	WaterRenderInfo_t m_waterInfo;
	float m_waterHeight;
	float m_waterZAdjust;
	bool m_bSoftwareUserClipPlane;
	VisibleFogVolumeInfo_t m_fogInfo;
};


//-----------------------------------------------------------------------------
// Scenes above water
//-----------------------------------------------------------------------------
class CAboveWaterView : public CBaseWaterView
{
public:
	DECLARE_CLASS( CAboveWaterView, CBaseWaterView );
	CAboveWaterView(CViewRender *pMainView) : 
		CBaseWaterView( pMainView ),
		m_ReflectionView( pMainView ),
		m_RefractionView( pMainView ),
		m_IntersectionView( pMainView )
	{}

	void Setup(  const CViewSetupEx &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo );
	void			Draw();

	class CReflectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CReflectionView, CBaseWorldView );
	public:
		CReflectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup( bool bReflectEntities );
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_ReflectionView ); }
	};

	class CRefractionView : public CBaseWorldView
	{
		DECLARE_CLASS( CRefractionView, CBaseWorldView );
	public:
		CRefractionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_RefractionView ); }
	};

	class CIntersectionView : public CBaseWorldView
	{
		DECLARE_CLASS( CIntersectionView, CBaseWorldView );
	public:
		CIntersectionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CAboveWaterView *GetOuter() { return GET_OUTER( CAboveWaterView, m_IntersectionView ); }
	};


	friend class CRefractionView;
	friend class CReflectionView;
	friend class CIntersectionView;

	bool m_bViewIntersectsWater;

	CReflectionView m_ReflectionView;
	CRefractionView m_RefractionView;
	CIntersectionView m_IntersectionView;
};


//-----------------------------------------------------------------------------
// Scenes below water
//-----------------------------------------------------------------------------
class CUnderWaterView : public CBaseWaterView
{
public:
	DECLARE_CLASS( CUnderWaterView, CBaseWaterView );
	CUnderWaterView(CViewRender *pMainView) : 
		CBaseWaterView( pMainView ),
		m_RefractionView( pMainView )
	{}

	void			Setup( const CViewSetupEx &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info );
	void			Draw();

	class CRefractionView : public CBaseWorldView
	{
		DECLARE_CLASS( CRefractionView, CBaseWorldView );
	public:
		CRefractionView(CViewRender *pMainView) : CBaseWorldView( pMainView ) {}

		void Setup();
		void Draw();

	private:
		CUnderWaterView *GetOuter() { return GET_OUTER( CUnderWaterView, m_RefractionView ); }
	};

	friend class CRefractionView;

	bool m_bDrawSkybox; // @MULTICORE (toml 8/17/2006): remove after setup hoisted

	CRefractionView m_RefractionView;
};


//-----------------------------------------------------------------------------
// Scenes containing reflective glass
//-----------------------------------------------------------------------------
class CReflectiveGlassView : public CSimpleWorldView
{
public:
	DECLARE_CLASS( CReflectiveGlassView, CSimpleWorldView );
	CReflectiveGlassView( CViewRender *pMainView ) : BaseClass( pMainView )
	{
	}

	virtual bool AdjustView( float flWaterHeight );
	virtual void PushView( float waterHeight );
	virtual void PopView( );
	void Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane );
	void Draw();

	ITexture	*GetReflectionTexture() { return m_pRenderTarget; }
	ITexture *m_pRenderTarget;

	cplane_t m_ReflectionPlane;
};

class CRefractiveGlassView : public CSimpleWorldView
{
public:
	DECLARE_CLASS( CRefractiveGlassView, CSimpleWorldView );
	CRefractiveGlassView( CViewRender *pMainView ) : BaseClass( pMainView )
	{
	}

	virtual bool AdjustView( float flWaterHeight );
	virtual void PushView( float waterHeight );
	virtual void PopView( );
	void Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane );
	void Draw();

	ITexture	*GetRefractionTexture() { return m_pRenderTarget; }
	ITexture *m_pRenderTarget;

	cplane_t m_ReflectionPlane;
};


//-----------------------------------------------------------------------------
// Computes draw flags for the engine to build its world surface lists
//-----------------------------------------------------------------------------
static inline unsigned long BuildEngineDrawWorldListFlags( unsigned nDrawFlags )
{
	unsigned long nEngineFlags = 0;

	if ( ( nDrawFlags & DF_SKIP_WORLD ) == 0 )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_WORLD_GEOMETRY;
	}

	if ( ( nDrawFlags & DF_SKIP_WORLD_DECALS_AND_OVERLAYS ) == 0 )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS;
	}

	if ( nDrawFlags & DF_DRAWSKYBOX )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SKYBOX;
	}

	if ( nDrawFlags & DF_RENDER_ABOVEWATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( nDrawFlags & DF_RENDER_UNDERWATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( nDrawFlags & DF_RENDER_WATER )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_WATERSURFACE;
	}

	if( nDrawFlags & DF_CLIP_SKYBOX )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_CLIPSKYBOX;
	}

	if( nDrawFlags & DF_SHADOW_DEPTH_MAP )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SHADOWDEPTH;
		nEngineFlags &= ~DRAWWORLDLISTS_DRAW_DECALS_AND_OVERLAYS;
	}

	if( nDrawFlags & DF_RENDER_REFRACTION )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_REFRACTION;
	}

	if( nDrawFlags & DF_RENDER_REFLECTION )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_REFLECTION;
	}

	if( nDrawFlags & DF_SSAO_DEPTH_PASS )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SSAO | DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER | DRAWWORLDLISTS_DRAW_INTERSECTSWATER | DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER ;
		nEngineFlags &= ~( DRAWWORLDLISTS_DRAW_WATERSURFACE | DRAWWORLDLISTS_DRAW_REFRACTION | DRAWWORLDLISTS_DRAW_REFLECTION );
	}

	return nEngineFlags;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void SetClearColorToFogColor()
{
	unsigned char ucFogColor[3];
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->GetFogColor( ucFogColor );
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
	{
		// @MULTICORE (toml 8/16/2006): Find a way to not do this twice in eye above water case
		float scale = LinearToGammaFullRange( pRenderContext->GetToneMappingScaleLinear().x );
		ucFogColor[0] *= scale;
		ucFogColor[1] *= scale;
		ucFogColor[2] *= scale;
	}
	pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
}

//-----------------------------------------------------------------------------
// Precache of necessary materials
//-----------------------------------------------------------------------------

CLIENTEFFECT_REGISTER_BEGIN( PrecacheViewRender )
	CLIENTEFFECT_MATERIAL( "scripted/intro_screenspaceeffect" )
CLIENTEFFECT_REGISTER_END()

CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffects )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_and_add_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_nohdr_clear" )
	CLIENTEFFECT_MATERIAL( "dev/bloomadd" )
	CLIENTEFFECT_MATERIAL( "dev/downsample" )

	CLIENTEFFECT_MATERIAL( "dev/downsample_non_hdr" )

	CLIENTEFFECT_MATERIAL( "dev/no_pixel_write" )
	CLIENTEFFECT_MATERIAL( "dev/lumcompare" )
	CLIENTEFFECT_MATERIAL( "dev/floattoscreen_combine" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb_vanilla" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb" )
	CLIENTEFFECT_MATERIAL( "dev/engine_post" )
	CLIENTEFFECT_MATERIAL( "dev/motion_blur" )
	CLIENTEFFECT_MATERIAL( "dev/upscale" )
	CLIENTEFFECT_MATERIAL( "dev/depth_of_field" )
	CLIENTEFFECT_MATERIAL( "dev/blurgaussian_3x3" )
	CLIENTEFFECT_MATERIAL( "dev/fade_blur" )

	CLIENTEFFECT_MATERIAL( "dev/glow_color" )
	CLIENTEFFECT_MATERIAL( "dev/glow_downsample" )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_x" )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_y" )
	CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
	CLIENTEFFECT_MATERIAL( "engine/writestencil" )

#ifdef TF_CLIENT_DLL
	CLIENTEFFECT_MATERIAL( "dev/pyro_blur_filter_y" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_blur_filter_x" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_dof" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_vignette_border" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_vignette" )
	CLIENTEFFECT_MATERIAL( "dev/pyro_post" )
#endif

CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )

//-----------------------------------------------------------------------------
// Accessors to return the current view being rendered
//-----------------------------------------------------------------------------
const Vector &CurrentViewOrigin()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderOrigin;
}

const QAngle &CurrentViewAngles()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderAngles;
}

const Vector &CurrentViewForward()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVForward;
}

const Vector &CurrentViewRight()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVRight;
}

const Vector &CurrentViewUp()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVUp;
}

const VMatrix &CurrentWorldToViewMatrix()
{
	Assert( s_bCanAccessCurrentView );
	return g_matCurrentCamInverse;
}


//-----------------------------------------------------------------------------
// Methods to set the current view/guard access to view parameters
//-----------------------------------------------------------------------------
void AllowCurrentViewAccess( bool allow )
{
	s_bCanAccessCurrentView = allow;
}

bool IsCurrentViewAccessAllowed()
{
	return s_bCanAccessCurrentView;
}

static ConVar mat_lpreview_mode( "mat_lpreview_mode", "-1", FCVAR_CHEAT );
void SetupCurrentView( const Vector &vecOrigin, const QAngle &angles, view_id_t viewID, bool bDrawWorldNormal = false, bool bCullFrontFaces = false )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Store off view origin and angles
	g_vecCurrentRenderOrigin = vecOrigin;
	g_vecCurrentRenderAngles = angles;

	// Compute the world->main camera transform
	ComputeCameraVariables( vecOrigin, angles, 
		&g_vecCurrentVForward, &g_vecCurrentVRight, &g_vecCurrentVUp, &g_matCurrentCamInverse );

	g_CurrentViewID = viewID;
	AllowCurrentViewAccess( true );

	// Cache off fade distances
	float flScreenFadeMinSize, flScreenFadeMaxSize;
	GetViewRenderInstance()->GetScreenFadeDistances( &flScreenFadeMinSize, &flScreenFadeMaxSize );
	modelinfo->SetViewScreenFadeRange( flScreenFadeMinSize, flScreenFadeMaxSize );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
#ifdef PORTAL
	if ( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, ((viewID == VIEW_MAIN) || (viewID == VIEW_3DSKY)) ? 1 : 0 );
	}
#else
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, ((viewID == VIEW_MAIN) || (viewID == VIEW_3DSKY)) ? 1 : 0 );
#endif

	if ( bDrawWorldNormal )
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING, ENABLE_FIXED_LIGHTING_OUTPUTNORMAL_AND_DEPTH );

	if ( mat_lpreview_mode.GetInt() != -1 )
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING, mat_lpreview_mode.GetInt() );
}

view_id_t CurrentViewID()
{
	Assert( g_CurrentViewID != VIEW_ILLEGAL );
	return ( view_id_t )g_CurrentViewID;
}

//-----------------------------------------------------------------------------
// Purpose: Portal views are considered 'Main' views. This function tests a view id 
//			against all view ids used by portal renderables, as well as the main view.
//-----------------------------------------------------------------------------
bool IsMainView ( view_id_t id )
{
#if defined(PORTAL)
	return ( (id == VIEW_MAIN) || g_pPortalRender->IsPortalViewID( id ) );
#else
	return (id == VIEW_MAIN);
#endif
}

void FinishCurrentView()
{
	AllowCurrentViewAccess( false );
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
void CSimpleRenderExecutor::AddView( CRendering3dView *pView )
{
 	CBase3dView *pPrevRenderer = m_pMainView->SetActiveRenderer( pView );
	pView->Draw();
	m_pMainView->SetActiveRenderer( pPrevRenderer );
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CViewRender::CViewRender()
	: m_SimpleExecutor( this )
{
	m_flCheapWaterStartDistance = 0.0f;
	m_flCheapWaterEndDistance = 0.1f;
	m_BaseDrawFlags = 0;
	m_pActiveRenderer = NULL;
	m_pCurrentlyDrawingEntity = NULL;
	m_bAllowViewAccess = false;
}

extern ConVarBase *r_drawentities;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CViewRender::ShouldDrawEntities( void )
{
	return (r_drawentities->GetInt() != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Check all conditions which would prevent drawing the view model
// Input  : drawViewmodel - 
//			*viewmodel - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawViewModel( bool bDrawViewmodel )
{
	if ( !bDrawViewmodel )
		return false;

	if ( !r_drawviewmodel.GetBool() )
		return false;

	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
		return false;

	if ( !ShouldDrawEntities() )
		return false;

	if ( render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CViewRender::UpdateRefractIfNeededByList( CViewModelRenderablesList::RenderGroups_t &list )
{
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientRenderableMod *pRenderableMod = list[i].m_pRenderableMod;
		Assert( pRenderableMod );

		if ( pRenderableMod->GetRenderFlags() & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
		{
			UpdateRefractTexture();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DrawRenderablesInList( CViewModelRenderablesList::RenderGroups_t &list, int flags )
{
	Assert( m_pCurrentlyDrawingEntity == NULL );
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientRenderable *pRenderable = list[i].m_pRenderable;
		Assert( pRenderable );

		IClientRenderableMod *pRenderableMod = list[i].m_pRenderableMod;
		Assert( pRenderableMod );

		// Non-view models wanting to render in view model list...
		if ( pRenderable->ShouldDraw() )
		{
			m_pCurrentlyDrawingEntity = pRenderable->GetIClientUnknown()->GetBaseEntity();
			pRenderableMod->DrawModel( STUDIO_RENDER | flags, list[i].m_InstanceData );
		}
	}
	m_pCurrentlyDrawingEntity = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Actually draw the view model
// Input  : drawViewModel - 
//-----------------------------------------------------------------------------
void CViewRender::DrawViewModels( const CViewSetupEx &view, bool drawViewmodel )
{
	VPROF( "CViewRender::DrawViewModel" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	bool bShouldDrawPlayerViewModel = ShouldDrawViewModel( drawViewmodel );
	bool bShouldDrawToolViewModels = ToolsEnabled();

	if ( !bShouldDrawPlayerViewModel && !bShouldDrawToolViewModels )
		return;

#ifdef PORTAL //in portal, we'd like a copy of the front buffer without the gun in it for use with the depth doubler
	g_pPortalRender->UpdateDepthDoublerTexture( view );
#endif

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	PIXEVENT( pRenderContext, "DrawViewModels" );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();

	CViewSetupEx viewModelSetup( view );
	viewModelSetup.zNear = view.zNearViewmodel;
	viewModelSetup.zFar = view.zFarViewmodel;
	viewModelSetup.fov = view.fovViewmodel;
	viewModelSetup.m_flAspectRatio = engine->GetScreenAspectRatio();

	ITexture *pRTColor = NULL;
	ITexture *pRTDepth = NULL;

	render->Push3DView( viewModelSetup, 0, pRTColor, GetFrustum(), pRTDepth );

#ifdef PORTAL //the depth range hack doesn't work well enough for the portal mod (and messing with the depth hack values makes some models draw incorrectly)
				//step up to a full depth clear if we're extremely close to a portal (in a portal environment)
	extern bool LocalPlayerIsCloseToPortal( void ); //defined in C_Portal_Player.cpp, abstracting to a single bool function to remove explicit dependence on c_portal_player.h/cpp, you can define the function as a "return true" in other build configurations at the cost of some perf
	bool bUseDepthHack = !LocalPlayerIsCloseToPortal();
	if( !bUseDepthHack )
		pRenderContext->ClearBuffers( false, true, false );
#else
	const bool bUseDepthHack = true;
#endif

	// FIXME: Add code to read the current depth range
	float depthmin = 0.0f;
	float depthmax = 1.0f;

	// HACK HACK:  Munge the depth range to prevent view model from poking into walls, etc.
	// Force clipped down range
	if( bUseDepthHack )
		pRenderContext->DepthRange( 0.0f, 0.1f );

	CViewModelRenderablesList list;
	ClientLeafSystem()->CollateViewModelRenderables( &list );
	CViewModelRenderablesList::RenderGroups_t &opaqueList = list.m_RenderGroups[ CViewModelRenderablesList::GROUP_OPAQUE ];
	CViewModelRenderablesList::RenderGroups_t &translucentList = list.m_RenderGroups[ CViewModelRenderablesList::GROUP_TRANSLUCENT ];

	if ( ToolsEnabled() && ( !bShouldDrawPlayerViewModel || !bShouldDrawToolViewModels ) )
	{
		int nOpaque = opaqueList.Count();
		for ( int i = nOpaque-1; i >= 0; --i )
		{
			IClientRenderable *pRenderable = opaqueList[ i ].m_pRenderable;
			bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity() ? true : false;
			if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
			{
				opaqueList.FastRemove( i );
			}
		}

		int nTranslucent = translucentList.Count();
		for ( int i = nTranslucent-1; i >= 0; --i )
		{
			IClientRenderable *pRenderable = translucentList[ i ].m_pRenderable;
			bool bEntity = pRenderable->GetIClientUnknown()->GetBaseEntity() ? true : false;
			if ( ( bEntity && !bShouldDrawPlayerViewModel ) || ( !bEntity && !bShouldDrawToolViewModels ) )
			{
				translucentList.FastRemove( i );
			}
		}
	}

	// Update refract for opaque models & draw
	bool bUpdatedRefractForOpaque = UpdateRefractIfNeededByList( opaqueList );
	DrawRenderablesInList( opaqueList );

	// Update refract for translucent models (if we didn't already update it above) & draw
	if ( !bUpdatedRefractForOpaque ) // Only do this once for better perf
	{
		UpdateRefractIfNeededByList( translucentList );
	}
	DrawRenderablesInList( translucentList, STUDIO_TRANSPARENCY );

	// Reset the depth range to the original values
	if( bUseDepthHack )
		pRenderContext->DepthRange( depthmin, depthmax );

	render->PopView( GetFrustum() );

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

extern ConVarBase *r_drawbrushmodels;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawBrushModels( void )
{
	if ( !r_drawbrushmodels->GetInt() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenSpaceEffects( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenSpaceEffects()");
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// FIXME: Screen-space effects are busted in the editor
	if ( engine->IsHammerRunning() )
		return;

	g_pScreenSpaceEffects->RenderEffects( x, y, w, h );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the screen space effect material (can't be done during rendering)
//-----------------------------------------------------------------------------
void CViewRender::SetScreenOverlayMaterial( IMaterial *pMaterial )
{
	m_ScreenOverlayMaterial.Init( pMaterial );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IMaterial *CViewRender::GetScreenOverlayMaterial( )
{
	return m_ScreenOverlayMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the screen space effect material (can't be done during rendering)
//-----------------------------------------------------------------------------
void CViewRender::SetIndexedScreenOverlayMaterial( int i, IMaterial *pMaterial )
{
	if (i < 0 || i >= MAX_SCREEN_OVERLAYS)
		return;

	m_IndexedScreenOverlayMaterials[i].Init( pMaterial );

	if (pMaterial == NULL)
	{
		// Check if we should set to false
		int i;
		for (i = 0; i < MAX_SCREEN_OVERLAYS; i++)
		{
			if (m_IndexedScreenOverlayMaterials[i] != NULL)
				break;
		}

		if (i == MAX_SCREEN_OVERLAYS)
			m_bUsingIndexedScreenOverlays = false;
	}
	else
	{
		m_bUsingIndexedScreenOverlays = true;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IMaterial *CViewRender::GetIndexedScreenOverlayMaterial( int i )
{
	if (i < 0 || i >= MAX_SCREEN_OVERLAYS)
		return NULL;

	return m_IndexedScreenOverlayMaterials[i];
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CViewRender::ResetIndexedScreenOverlays()
{
	for (int i = 0; i < MAX_SCREEN_OVERLAYS; i++)
	{
		m_IndexedScreenOverlayMaterials[i].Init( NULL );
	}

	m_bUsingIndexedScreenOverlays = false;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CViewRender::GetMaxIndexedScreenOverlays( ) const
{
	return MAX_SCREEN_OVERLAYS;
}

//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenOverlay( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenOverlay()");

	if (m_ScreenOverlayMaterial)
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

		if ( m_ScreenOverlayMaterial->NeedsFullFrameBufferTexture() )
		{
            // FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
			DrawScreenEffectMaterial( m_ScreenOverlayMaterial, x, y, w, h );
		}
		else if ( m_ScreenOverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
		{
			// First copy the FB off to the offscreen texture
			UpdateRefractTexture( x, y, w, h, true );

			// Now draw the entire screen using the material...
			CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
			ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
			int sw = pTexture->GetActualWidth();
			int sh = pTexture->GetActualHeight();
            // Note - don't offset by x,y - already done by the viewport.
			pRenderContext->DrawScreenSpaceRectangle( m_ScreenOverlayMaterial, 0, 0, w, h,
												 0, 0, sw-1, sh-1, sw, sh );
		}
		else
		{
			byte color[4] = { 255, 255, 255, 255 };
			render->ViewDrawFade( color, m_ScreenOverlayMaterial );
		}
	}

	if (m_bUsingIndexedScreenOverlays)
	{
		for (int i = 0; i < MAX_SCREEN_OVERLAYS; i++)
		{
			if (!m_IndexedScreenOverlayMaterials[i])
				continue;

			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

			if ( m_IndexedScreenOverlayMaterials[i]->NeedsFullFrameBufferTexture() )
			{
				// FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
				DrawScreenEffectMaterial( m_IndexedScreenOverlayMaterials[i], x, y, w, h );
			}
			else if ( m_IndexedScreenOverlayMaterials[i]->NeedsPowerOfTwoFrameBufferTexture() )
			{
				// First copy the FB off to the offscreen texture
				UpdateRefractTexture( x, y, w, h, true );

				// Now draw the entire screen using the material...
				CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
				ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
				int sw = pTexture->GetActualWidth();
				int sh = pTexture->GetActualHeight();
				// Note - don't offset by x,y - already done by the viewport.
				pRenderContext->DrawScreenSpaceRectangle( m_IndexedScreenOverlayMaterials[i], 0, 0, w, h,
													 0, 0, sw-1, sh-1, sw, sh );
			}
			else
			{
				byte color[4] = { 255, 255, 255, 255 };
				render->ViewDrawFade( color, m_IndexedScreenOverlayMaterials[i] );
			}
		}
	}
}

void CViewRender::DrawUnderwaterOverlay( void )
{
	IMaterial *pOverlayMat = m_UnderWaterOverlayMaterial;

	if ( pOverlayMat )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

		int x, y, w, h;

		pRenderContext->GetViewport( x, y, w, h );
		if ( pOverlayMat->NeedsFullFrameBufferTexture() )
		{
            // FIXME: check with multi/sub-rect renders. Should this be 0,0,w,h instead?
			DrawScreenEffectMaterial( pOverlayMat, x, y, w, h );
		}
		else if ( pOverlayMat->NeedsPowerOfTwoFrameBufferTexture() )
		{
			// First copy the FB off to the offscreen texture
			UpdateRefractTexture( x, y, w, h, true );

			// Now draw the entire screen using the material...
			CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
			ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
			int sw = pTexture->GetActualWidth();
			int sh = pTexture->GetActualHeight();
            // Note - don't offset by x,y - already done by the viewport.
			pRenderContext->DrawScreenSpaceRectangle( pOverlayMat, 0, 0, w, h,
													  0, 0, sw-1, sh-1, sw, sh );
		}
		else
		{
            // Note - don't offset by x,y - already done by the viewport.
            // FIXME: actually test this code path.
			pRenderContext->DrawScreenSpaceRectangle( pOverlayMat, 0, 0, w, h,
													  0, 0, 1, 1, 1, 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the min/max fade distances
//-----------------------------------------------------------------------------
void CViewRender::GetScreenFadeDistances( float *min, float *max )
{
	if ( min )
	{
		*min = m_FadeData.m_flPixelMin;
	}

	if ( max )
	{
		*max = m_FadeData.m_flPixelMax;
	}
}

void CViewRender::OnScreenFadeMinSize( const CCommand &args )
{
	if ( args.ArgC() < 2 )
		return;

	m_FadeData.m_flPixelMin = atof( args[1] ) * 1000.0f;
}

void CViewRender::OnScreenFadeMaxSize( const CCommand &args )
{
	if ( args.ArgC() < 2 )
		return;

	m_FadeData.m_flPixelMax = atof( args[1] ) * 1000.0f;
}

enum FadeMode_t
{
	FADE_MODE_NONE = 0,
	FADE_MODE_LOW,
	FADE_MODE_MED,
	FADE_MODE_HIGH,
	FADE_MODE_LEVEL,

	FADE_MODE_COUNT,
};

// Fade data.
FadeData_t g_aFadeData[FADE_MODE_COUNT] = 
{
	//	PixelMin	PixelMax	Width		DistScale		FadeMode_t
	{	  0.0f,		  0.0f,		1280.0f,	1.0f	}, //	FADE_MODE_NONE 
	{	 10.0f,		 15.0f,		 800.0f,	1.0f	}, //	FADE_MODE_LOW
	{	  5.0f,		 10.0f,		1024.0f,	1.0f	}, //	FADE_MODE_MED
	{	  0.0f,		  0.0f,		1280.0f,	1.0f	}, //	FADE_MODE_HIGH
	{	  0.0f,		  0.0f,		1280.0f,	1.0f	}, //	FADE_MODE_LEVEL
};

//-----------------------------------------------------------------------------
// Purpose: Initialize the fade data.
//-----------------------------------------------------------------------------
void CViewRender::InitFadeData( void )
{
	// What system are we running.
	// GetActualCPULevel() returns the cpu_level convar value, even on the 360.
	// L4D knocks down this convar in splitscreen mode to control the fade distances.
	int nSystemLevel = GetActualCPULevel();

	switch(nSystemLevel) {
	case CPU_LEVEL_LOW:
		m_FadeData = g_aFadeData[FADE_MODE_LOW];
		break;
	case CPU_LEVEL_MEDIUM:
		m_FadeData = g_aFadeData[FADE_MODE_MED];
		break;
	case CPU_LEVEL_HIGH:
		m_FadeData = g_aFadeData[FADE_MODE_HIGH];
		break;
	default:
		m_FadeData = g_aFadeData[FADE_MODE_LOW];
		break;
	}
}

C_BaseEntity *CViewRender::GetCurrentlyDrawingEntity()
{
	return m_pCurrentlyDrawingEntity;
}

void CViewRender::SetCurrentlyDrawingEntity( C_BaseEntity *pEnt )
{
	m_pCurrentlyDrawingEntity = pEnt;
}

bool CViewRender::UpdateShadowDepthTexture( ITexture *pRenderTarget, ITexture *pDepthTexture, const CViewSetupEx &shadowViewIn )
{
	VPROF_INCREMENT_COUNTER( "shadow depth textures rendered", 1 );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

#ifdef PIX_ENABLE
	char szPIXEventName[128];
	sprintf( szPIXEventName, "UpdateShadowDepthTexture (%s)", pDepthTexture->GetName() );
	PIXEVENT( pRenderContext, szPIXEventName );
#endif

	CRefPtr<CShadowDepthView> pShadowDepthView = new CShadowDepthView( this );
	pShadowDepthView->Setup( shadowViewIn, pRenderTarget, pDepthTexture );
	AddViewToScene( pShadowDepthView );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Renders world and all entities, etc.
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene( bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, const CViewSetupEx &view, 
								int nClearFlags, view_id_t viewID, bool bDrawViewModel, int baseDrawFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::ViewDrawScene" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// this allows the refract texture to be updated once per *scene* on 360
	// (e.g. once for a monitor scene and once for the main scene)
	g_viewscene_refractUpdateFrame = gpGlobals->framecount - 1;

	g_pClientShadowMgr->PreRender();

	// Shadowed flashlights supported on ps_2_b and up...
	if ( r_flashlightdepthtexture->GetBool() && (viewID == VIEW_MAIN) && ( !view.m_bDrawWorldNormal ) )
	{
		g_pClientShadowMgr->ComputeShadowDepthTextures( view );

		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

		// GSTRINGMIGRATION
		if ( g_pCSMEnvLight != NULL && g_pCSMEnvLight->IsCascadedShadowMappingEnabled() )
		{
			UpdateCascadedShadow( view );
		}
		else
		{
			pRenderContext->SetIntRenderingParameter( INT_CASCADED_DEPTHTEXTURE, 0 );
		}
	}

	m_BaseDrawFlags = baseDrawFlags;

	SetupCurrentView( view.origin, view.angles, viewID, view.m_bDrawWorldNormal, view.m_bCullFrontFaces );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view
	unsigned int visFlags;
	SetupVis( view, visFlags, pCustomVisibility );

	if ( !bDrew3dSkybox && 
		( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) && ( visFlags & IVRenderView::VIEW_SETUP_VIS_EX_RETURN_FLAGS_USES_RADIAL_VIS ) )
	{
		// This covers the case where we don't see a 3dskybox, yet radial vis is clipping
		// the far plane.  Need to clear to fog color in this case.
		nClearFlags |= VIEW_CLEAR_COLOR;
		SetClearColorToFogColor( );
	}

	bool drawSkybox = r_skybox.GetBool();
	if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
	{
		drawSkybox = false;
	}

	ParticleMgr()->IncrementFrameCode();

	DrawWorldAndEntities( drawSkybox, view, nClearFlags, pCustomVisibility );

	// Disable fog for the rest of the stuff
	DisableFog();

	// UNDONE: Don't do this with masked brush models, they should probably be in a separate list
	// render->DrawMaskEntities()

	// Here are the overlays...

	if ( !view.m_bDrawWorldNormal )
	{
		CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );
	}

	// issue the pixel visibility tests
	if ( IsMainView( CurrentViewID() ) && !view.m_bDrawWorldNormal )
	{
		PixelVisibility_EndCurrentView();
	}

	// Draw rain..
	DrawPrecipitation();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( view );

	// Draw client side effects
	// NOTE: These are not sorted against the rest of the frame
	clienteffects->DrawEffects( gpGlobals->frametime );	

	// Mark the frame as locked down for client fx additions
	SetFXCreationAllowed( false );

	// Invoke post-render methods
	IGameSystem::PostRenderAllSystems();

	FinishCurrentView();

	// Free shadow depth textures for use in future view
	if ( r_flashlightdepthtexture->GetBool() && ( !view.m_bDrawWorldNormal ) )
	{
		g_pClientShadowMgr->UnlockAllShadowDepthTextures();
	}

	// Set int rendering parameters back to defaults
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING, 0 );
}


void CheckAndTransitionColor( float flPercent, float *pColor, float *pLerpToColor )
{
	if ( pLerpToColor[0] != pColor[0] || pLerpToColor[1] != pColor[1] || pLerpToColor[2] != pColor[2] )
	{
		float flDestColor[3];

		flDestColor[0] = pLerpToColor[0];
		flDestColor[1] = pLerpToColor[1];
		flDestColor[2] = pLerpToColor[2];

		pColor[0] = FLerp( pColor[0], flDestColor[0], flPercent );
		pColor[1] = FLerp( pColor[1], flDestColor[1], flPercent );
		pColor[2] = FLerp( pColor[2], flDestColor[2], flPercent );
	}
	else
	{
		pColor[0] = pLerpToColor[0];
		pColor[1] = pLerpToColor[1];
		pColor[2] = pLerpToColor[2];
	}
}

static void GetFogColorTransition( fogparams_t *pFogParams, float *pColorPrimary, float *pColorSecondary )
{
	if ( !pFogParams )
		return;

	if ( pFogParams->lerptime >= gpGlobals->curtime )
	{
		float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

		float flPrimaryColorLerp[3] = { (float)pFogParams->colorPrimaryLerpTo.GetR(), (float)pFogParams->colorPrimaryLerpTo.GetG(), (float)pFogParams->colorPrimaryLerpTo.GetB() };
		float flSecondaryColorLerp[3] = { (float)pFogParams->colorSecondaryLerpTo.GetR(), (float)pFogParams->colorSecondaryLerpTo.GetG(), (float)pFogParams->colorSecondaryLerpTo.GetB() };

		CheckAndTransitionColor( flPercent, pColorPrimary, flPrimaryColorLerp );
		CheckAndTransitionColor( flPercent, pColorSecondary, flSecondaryColorLerp );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
static void GetFogColor( fogparams_t *pFogParams, float *pColor, bool ignoreOverride, bool ignoreHDRColorScale )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( !pbp || !pFogParams )
		return;

	bool bFogOverride = fog_override.GetBool() && !ignoreOverride;

	float HDRColorScale;
	if( bFogOverride && (fog_hdrcolorscale.GetFloat() != -1.0f) )
	{
		HDRColorScale = fog_hdrcolorscale.GetFloat();
	}
	else
	{
		HDRColorScale = pFogParams->HDRColorScale;
	}

	pColor[0] = pColor[1] = pColor[2] = -1.0f;

	const char *fogColorString = fog_color.GetString();
	if( bFogOverride && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor+1, pColor+2 );
	}

	if( (pColor[0] == -1.0f) && (pColor[1] == -1.0f) && (pColor[2] == -1.0f) ) //if not overriding fog, or if we get non-overridden fog color values
	{
		float flPrimaryColor[3] = { (float)pFogParams->colorPrimary.GetR(), (float)pFogParams->colorPrimary.GetG(), (float)pFogParams->colorPrimary.GetB() };
		float flSecondaryColor[3] = { (float)pFogParams->colorSecondary.GetR(), (float)pFogParams->colorSecondary.GetG(), (float)pFogParams->colorSecondary.GetB() };

		GetFogColorTransition( pFogParams, flPrimaryColor, flSecondaryColor );

		if( pFogParams->blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			pbp->EyeVectors( &forward, NULL, NULL );
			
			Vector vNormalized = pFogParams->dirPrimary;
			VectorNormalize( vNormalized );
			pFogParams->dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( pFogParams->dirPrimary ) + 0.5;

			// FIXME: convert to linear colorspace
			pColor[0] = flPrimaryColor[0] * flBlendFactor + flSecondaryColor[0] * ( 1 - flBlendFactor );
			pColor[1] = flPrimaryColor[1] * flBlendFactor + flSecondaryColor[1] * ( 1 - flBlendFactor );
			pColor[2] = flPrimaryColor[2] * flBlendFactor + flSecondaryColor[2] * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = flPrimaryColor[0];
			pColor[1] = flPrimaryColor[1];
			pColor[2] = flPrimaryColor[2];
		}
	}

	if ( !ignoreHDRColorScale && g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
	{
		VectorScale( pColor, HDRColorScale, pColor );
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


static float GetFogStart( fogparams_t *pFogParams, bool ignoreOverride )
{
	if( !pFogParams )
		return 0.0f;

	if( fog_override.GetInt() && !ignoreOverride )
	{
		if( fog_start.GetFloat() == -1.0f )
		{
			return pFogParams->start;
		}
		else
		{
			return fog_start.GetFloat();
		}
	}
	else
	{
		if ( pFogParams->lerptime > gpGlobals->curtime )
		{
			if ( pFogParams->start != pFogParams->startLerpTo )
			{
				if ( pFogParams->lerptime > gpGlobals->curtime )
				{
					float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

					return FLerp( pFogParams->start, pFogParams->startLerpTo, flPercent );
				}
				else
				{
					if ( pFogParams->start != pFogParams->startLerpTo )
					{
						pFogParams->start = pFogParams->startLerpTo;
					}
				}
			}
		}

		return pFogParams->start;
	}
}

static float GetFogEnd( fogparams_t *pFogParams, bool ignoreOverride )
{
	if( !pFogParams )
		return 0.0f;

	if( fog_override.GetInt() && !ignoreOverride )
	{
		if( fog_end.GetFloat() == -1.0f )
		{
			return pFogParams->end;
		}
		else
		{
			return fog_end.GetFloat();
		}
	}
	else
	{
		if ( pFogParams->lerptime > gpGlobals->curtime )
		{
			if ( pFogParams->end != pFogParams->endLerpTo )
			{
				if ( pFogParams->lerptime > gpGlobals->curtime )
				{
					float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

					return FLerp( pFogParams->end, pFogParams->endLerpTo, flPercent );
				}
				else
				{
					if ( pFogParams->end != pFogParams->endLerpTo )
					{
						pFogParams->end = pFogParams->endLerpTo;
					}
				}
			}
		}

		return pFogParams->end;
	}
}

static bool GetFogEnable( fogparams_t *pFogParams, bool ignoreOverride )
{
	if ( cl_leveloverview.GetFloat() > 0 )
		return false;

	// Ask the clientmode
	if ( GetClientMode()->ShouldDrawFog() == false )
		return false;

	if( fog_override.GetInt() && !ignoreOverride )
	{
		if( fog_enable.GetInt() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if( pFogParams )
			return pFogParams->enable != false;

		return false;
	}
}


static float GetFogMaxDensity( fogparams_t *pFogParams, bool ignoreOverride )
{
	if( !pFogParams )
		return 1.0f;

	if ( cl_leveloverview.GetFloat() > 0 )
		return 1.0f;

	// Ask the clientmode
	if ( !GetClientMode()->ShouldDrawFog() )
		return 1.0f;

	if ( fog_override.GetInt() && !ignoreOverride )
	{
		if ( fog_maxdensity.GetFloat() == -1.0f )
			return pFogParams->maxdensity;
		else
			return fog_maxdensity.GetFloat();
	}
	else
	{
		if ( pFogParams->lerptime > gpGlobals->curtime )
		{
			if ( pFogParams->maxdensity != pFogParams->maxdensityLerpTo )
			{
				if ( pFogParams->lerptime > gpGlobals->curtime )
				{
					float flPercent = 1.0f - (( pFogParams->lerptime - gpGlobals->curtime ) / pFogParams->duration );

					return FLerp( pFogParams->maxdensity, pFogParams->maxdensityLerpTo, flPercent );
				}
				else
				{
					if ( pFogParams->maxdensity != pFogParams->maxdensityLerpTo )
					{
						pFogParams->maxdensity = pFogParams->maxdensityLerpTo;
					}
				}
			}
		}

		return pFogParams->maxdensity;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the skybox fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
void GetSkyboxFogColor( float *pColor, bool ignoreOverride, bool ignoreHDRColorScale )
{			   
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	bool bFogOverride = fog_override.GetBool() && !ignoreOverride;
	float HDRColorScale;
	if( bFogOverride && (fog_hdrcolorscaleskybox.GetFloat() != -1.0f) )
	{
		HDRColorScale = fog_hdrcolorscaleskybox.GetFloat();
	}
	else
	{
		HDRColorScale = local->m_skybox3d.fog.HDRColorScale;
	}

	const char *fogColorString = fog_colorskybox.GetString();
	if( bFogOverride && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor+1, pColor+2 );
	}

	if( (pColor[0] == -1.0f) && (pColor[1] == -1.0f) && (pColor[2] == -1.0f) ) //if not overriding fog, or if we get non-overridden fog color values
	{
		if( local->m_skybox3d.fog.blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			pbp->EyeVectors( &forward, NULL, NULL );

			Vector vNormalized = local->m_skybox3d.fog.dirPrimary;
			VectorNormalize( vNormalized );
			local->m_skybox3d.fog.dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( local->m_skybox3d.fog.dirPrimary ) + 0.5;
						 
			// FIXME: convert to linear colorspace
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetR() * ( 1 - flBlendFactor );
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetG() * ( 1 - flBlendFactor );
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetB() * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR();
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG();
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB();
		}
	}

	if ( !ignoreHDRColorScale && g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
	{
		VectorScale( pColor, HDRColorScale, pColor );
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


float GetSkyboxFogStart( bool ignoreOverride )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() && !ignoreOverride )
	{
		if( fog_startskybox.GetFloat() == -1.0f )
		{
			return local->m_skybox3d.fog.start;
		}
		else
		{
			return fog_startskybox.GetFloat();
		}
	}
	else
	{
		return local->m_skybox3d.fog.start;
	}
}

float GetSkyboxFogEnd( bool ignoreOverride )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() && !ignoreOverride )
	{
		if( fog_endskybox.GetFloat() == -1.0f )
		{
			return local->m_skybox3d.fog.end;
		}
		else
		{
			return fog_endskybox.GetFloat();
		}
	}
	else
	{
		return local->m_skybox3d.fog.end;
	}
}


float GetSkyboxFogMaxDensity( bool ignoreOverride )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( !pbp )
		return 1.0f;

	CPlayerLocalData *local = &pbp->m_Local;

	if ( cl_leveloverview.GetFloat() > 0 )
		return 1.0f;

	// Ask the clientmode
	if ( !GetClientMode()->ShouldDrawFog() )
		return 1.0f;

	if ( fog_override.GetInt() && !ignoreOverride )
	{
		if ( fog_maxdensityskybox.GetFloat() == -1.0f )
			return local->m_skybox3d.fog.maxdensity;
		else
			return fog_maxdensityskybox.GetFloat();
	}
	else
		return local->m_skybox3d.fog.maxdensity;
}


void CViewRender::DisableFog( void )
{
	VPROF("CViewRander::DisableFog()");

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->FogMode( MATERIAL_FOG_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::SetupVis( const CViewSetupEx& view, unsigned int &visFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::SetupVis" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( pCustomVisibility && pCustomVisibility->m_nNumVisOrigins )
	{
		// Pass array or vis origins to merge
		render->ViewSetupVisEx( ShouldForceNoVis(), pCustomVisibility->m_nNumVisOrigins, pCustomVisibility->m_rgVisOrigins, visFlags );
	}
	else
	{
		// Use render origin as vis origin by default
		render->ViewSetupVisEx( ShouldForceNoVis(), 1, &view.origin, visFlags );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Renders voice feedback and other sprites attached to players
// Input  : none
//-----------------------------------------------------------------------------
void CViewRender::RenderPlayerSprites()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	GetClientVoiceMgr()->DrawHeadLabels();
}

//-----------------------------------------------------------------------------
// Sets up, cleans up the main 3D view
//-----------------------------------------------------------------------------
void CViewRender::SetupMain3DView( const CViewSetupEx &view, const CViewSetupEx &hudViewSetup, int &nClearFlags, ITexture *pRenderTarget )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// FIXME: I really want these fields removed from CViewSetupEx 
	// and passed in as independent flags
	// Clear the color here if requested.

	int nDepthStencilFlags = nClearFlags & ( VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL );
	nClearFlags &= ~( nDepthStencilFlags ); // Clear these flags
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		nClearFlags |= nDepthStencilFlags; // Add them back in if we're clearing color
	}

	// If we are using HDR, we render to the HDR full frame buffer texture
	// instead of whatever was previously the render target
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT )
	{
		if ( view.m_bHDRTarget )
		{
			render->Push3DView( view, nClearFlags, pRenderTarget, GetFrustum() );
		}
		else
		{
			CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
			pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_BACK_BUFFER_INDEX, BACK_BUFFER_INDEX_HDR );
			pRenderContext.SafeRelease(); // don't want to hold for long periods in case in a locking active share thread mode
			render->Push3DView( view, nClearFlags, NULL, GetFrustum() );
		}
	}
	else
	{
		ITexture *pRTColor = NULL;
		ITexture *pRTDepth = NULL;

		render->Push3DView( view, nClearFlags, pRTColor, GetFrustum(), pRTDepth );
	}

	// If we didn't clear the depth here, we'll need to clear it later
	nClearFlags ^= nDepthStencilFlags; // Toggle these bits
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		// If we cleared the color here, we don't need to clear it later
		nClearFlags &= ~( VIEW_CLEAR_COLOR | VIEW_CLEAR_FULL_TARGET );
	}
}

void CViewRender::CleanupMain3DView( const CViewSetupEx &view )
{
	// Make sure we reset from the HDR rendertarget back to the main backbuffer
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT )
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_BACK_BUFFER_INDEX, BACK_BUFFER_INDEX_DEFAULT );
		pRenderContext.SafeRelease(); // don't want to hold for long periods in case in a locking active share thread mode
	}

	render->PopView( GetFrustum() );
}

void CViewRender::UpdateCascadedShadow( const CViewSetupEx &view )
{
	static CTextureReference s_CascadedShadowDepthTexture;
	static CTextureReference s_CascadedShadowColorTexture;
	if ( !s_CascadedShadowDepthTexture.IsValid() )
	{
		s_CascadedShadowDepthTexture.Init( "_rt_CascadedShadowDepth", TEXTURE_GROUP_OTHER );
	}

	if ( !s_CascadedShadowColorTexture.IsValid() )
	{
		s_CascadedShadowColorTexture.Init( "_rt_CascadedShadowColor", TEXTURE_GROUP_OTHER );
	}

	ITexture *pDepthTexture = s_CascadedShadowDepthTexture;
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->SetIntRenderingParameter( INT_CASCADED_DEPTHTEXTURE, int( pDepthTexture ) );

	QAngle angCascadedAngles;
	Vector vecLight, vecAmbient;
	g_pCSMEnvLight->GetShadowMappingConstants( angCascadedAngles, vecLight, vecAmbient );

	if ( csm_color_light.GetBool() )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, csm_color_light.GetString() );
		vecLight = ConvertLightmapGammaToLinear( iParsed );
	}

	if ( csm_color_ambient.GetBool() )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, csm_color_ambient.GetString() );
		vecAmbient = ConvertLightmapGammaToLinear( iParsed );
	}

	for ( int i = 0; i < 3; i++ )
	{
		vecAmbient[i] = MIN( vecAmbient[i], vecLight[i] );
	}

	Vector vecAmbientDelta = vecLight - vecAmbient;
	vecAmbientDelta.NormalizeInPlace();
	pRenderContext->SetVectorRenderingParameter( VECTOR_RENDERPARM_CASCADED_AMBIENT_MIN, vecAmbient );
	pRenderContext->SetVectorRenderingParameter( VECTOR_RENDERPARM_CASCADED_AMBIENT_DELTA, vecAmbientDelta );

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( angCascadedAngles, &vecFwd, &vecRight, &vecUp );

	pRenderContext->SetVectorRenderingParameter( VECTOR_RENDERPARM_CASCADED_FORWARD, vecFwd );

	Vector vecMainViewFwd;
	AngleVectors( view.angles, &vecMainViewFwd );

	CViewSetupEx cascadedShadowView;
	cascadedShadowView.angles = angCascadedAngles;
	cascadedShadowView.m_bOrtho = true;

	cascadedShadowView.width = s_CascadedShadowDepthTexture->GetMappingWidth() / 2;
	cascadedShadowView.height = s_CascadedShadowDepthTexture->GetMappingHeight();

	cascadedShadowView.m_flAspectRatio = 1.0f;
	cascadedShadowView.m_bDoBloomAndToneMapping = false;
	cascadedShadowView.zFar = cascadedShadowView.zFarViewmodel = 2000.0f;
	cascadedShadowView.zNear = cascadedShadowView.zNearViewmodel = 7.0f;
	cascadedShadowView.fov = cascadedShadowView.fovViewmodel = 90.0f;

	struct ShadowConfig_t
	{
		float flOrthoSize;
		float flForwardOffset;
		float flUVOffsetX;
		float flViewDepthBiasHack;
	} shadowConfigs[] = {
		{ 64.0f, 0.0f, 0.25f, 0.0f },
		{ 384.0f, 256.0f, 0.75f, 0.0f }
	};
	const int iCascadedShadowCount = ARRAYSIZE( shadowConfigs );

	/*switch ( mode )
	{
	case CASCADEDCONFIG_NORMAL:
	{
		ShadowConfig_t &closeShadow = shadowConfigs[0];
		ShadowConfig_t &farShadow = shadowConfigs[1];
		closeShadow.flOrthoSize = 256.0f;
		closeShadow.flForwardOffset = 102.0f;
		farShadow.flOrthoSize = 786.0f;
		farShadow.flForwardOffset = 384.0f;

		vecMainViewFwd.z = 0.0f;
		vecMainViewFwd.NormalizeInPlace();
	}
	break;

	case CASCADEDCONFIG_SPACE:
	{
		ShadowConfig_t &closeShadow = shadowConfigs[0];
		ShadowConfig_t &farShadow = shadowConfigs[1];
		closeShadow.flOrthoSize = 4.0f;
		closeShadow.flForwardOffset = 2.0f;
		closeShadow.flUVOffsetX = 0.25f;
		farShadow.flViewDepthBiasHack = 1.5f;
	}
	break;
	}*/

	g_pCSMEnvLight->CopyShadowConfigData( shadowConfigs );

	vecMainViewFwd -= vecFwd * DotProduct( vecMainViewFwd, vecFwd );

	static VMatrix s_CSMSwapMatrix[2];
	static int s_iCSMSwapIndex = 0;

	//C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	//Vector vecCascadeOrigin( ( mode == CASCADEDCONFIG_SPACE ) ? view.origin :
	//						 pPlayer ? pPlayer->GetRenderOrigin() : vec3_origin );
	Vector vecCascadeOrigin = view.origin - ( vecFwd * 1536 );

	// This can only be set once per frame, not per shadow view. Fuckers.
	/*if ( mode == CASCADEDCONFIG_SPACE )
	{
		pRenderContext->SetShadowDepthBiasFactors( 1.0f, 0.000005f );
	}
	else*/
	{
		pRenderContext->SetShadowDepthBiasFactors( 8.0f, 0.0005f );
	}

	for ( int i = 0; i < iCascadedShadowCount; i++ )
	{
		const ShadowConfig_t &shadowConfig = shadowConfigs[i];

		cascadedShadowView.m_OrthoTop = -shadowConfig.flOrthoSize;
		cascadedShadowView.m_OrthoRight = shadowConfig.flOrthoSize;
		cascadedShadowView.m_OrthoBottom = shadowConfig.flOrthoSize;
		cascadedShadowView.m_OrthoLeft = -shadowConfig.flOrthoSize;

		cascadedShadowView.x = i * cascadedShadowView.width;
		cascadedShadowView.y = 0;

		Vector vecOrigin = vecCascadeOrigin + vecMainViewFwd * shadowConfig.flForwardOffset;
		const float flViewFrustumWidthScale = shadowConfig.flOrthoSize * 2.0f / cascadedShadowView.width;
		const float flViewFrustumHeightScale = shadowConfig.flOrthoSize * 2.0f / cascadedShadowView.height;
		const float flFractionX = fmod( DotProduct( vecOrigin, vecRight ), flViewFrustumWidthScale );
		const float flFractionY = fmod( DotProduct( vecOrigin, vecUp ), flViewFrustumHeightScale );
		vecOrigin -= flFractionX * vecRight;
		vecOrigin -= flFractionY * vecUp;

		if ( i > 0 )
		{
			const ShadowConfig_t &prevShadowConfig = shadowConfigs[i - 1];
			const float flCascadedScale = prevShadowConfig.flOrthoSize / shadowConfig.flOrthoSize;
			Vector vecOffsetShadowMapSpace = vecOrigin - cascadedShadowView.origin;
			Vector2D vecCascadedOffset( DotProduct( -vecRight, vecOffsetShadowMapSpace ) * 0.5f,
										DotProduct( vecUp, vecOffsetShadowMapSpace ) );
			vecCascadedOffset *= 0.5f / shadowConfig.flOrthoSize;

			vecCascadedOffset.x = vecCascadedOffset.x + 0.5f + 0.25f * ( 1.0f - flCascadedScale );
			vecCascadedOffset.y = vecCascadedOffset.y + 0.5f - flCascadedScale * 0.5f;

			vecOffsetShadowMapSpace.Init( flCascadedScale, vecCascadedOffset.x, vecCascadedOffset.y );
			pRenderContext->SetVectorRenderingParameter( VECTOR_RENDERPARM_CASCADED_STEP, vecOffsetShadowMapSpace );
		}

		cascadedShadowView.origin = vecOrigin;

		VMatrix worldToView, viewToProjection, worldToProjection, worldToTexture;
		render->GetMatricesForView( cascadedShadowView, &worldToView, &viewToProjection, &worldToProjection, &worldToTexture );

		if ( i == 0 )
		{
			VMatrix tmp;
			MatrixBuildScale( tmp, 0.25f, -0.5f, 1.0f );
			tmp[0][3] = shadowConfig.flUVOffsetX;
			tmp[1][3] = 0.5f;

			VMatrix &currentSwapMatrix = s_CSMSwapMatrix[s_iCSMSwapIndex];
			MatrixMultiply( tmp, worldToProjection, currentSwapMatrix );
			pRenderContext->SetIntRenderingParameter( INT_CASCADED_MATRIX_ADDRESS_0, reinterpret_cast<int>( &currentSwapMatrix ) );
		}

		cascadedShadowView.origin -= vecFwd * shadowConfig.flViewDepthBiasHack;
		UpdateShadowDepthTexture( s_CascadedShadowColorTexture, s_CascadedShadowDepthTexture, cascadedShadowView );
	}

	s_iCSMSwapIndex = ( s_iCSMSwapIndex + 1 ) % 2;
}

//-----------------------------------------------------------------------------
// Queues up an overlay rendering
//-----------------------------------------------------------------------------
void CViewRender::QueueOverlayRenderView( const CViewSetupEx &view, int nClearFlags, int whatToDraw )
{
	// Can't have 2 in a single scene
	Assert( !m_bDrawOverlay );

    m_bDrawOverlay = true;
	m_OverlayViewSetup = view;
	m_OverlayClearFlags = nClearFlags;
	m_OverlayDrawFlags = whatToDraw;
}

//-----------------------------------------------------------------------------
// Purpose: Force the view to freeze on the next frame for the specified time
//-----------------------------------------------------------------------------
void CViewRender::FreezeFrame( float flFreezeTime )
{
	if ( flFreezeTime == 0 )
	{
		m_flFreezeFrameUntil = 0;
		m_rbTakeFreezeFrame = false;
	}
	else
	{
		if ( m_flFreezeFrameUntil > gpGlobals->curtime )
		{
			m_flFreezeFrameUntil += flFreezeTime;
		}
		else
		{
			m_flFreezeFrameUntil = gpGlobals->curtime + flFreezeTime;
			m_rbTakeFreezeFrame = true;
		}
	}
}

const char *COM_GetModDirectory();

void PositionHudPanels( CUtlVector< vgui::VPANEL > &list, const CViewSetupEx &view )
{
	for ( int i = 0; i < list.Count(); ++i )
	{
		vgui::VPANEL root = list[ i ];
		if ( root != 0 )
		{
			vgui::ipanel()->SetPos( root, view.x, view.y );
			vgui::ipanel()->SetSize( root, view.width, view.height );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This renders the entire 3D view and the in-game hud/viewmodel
// Input  : &view - 
//			whatToDraw - 
//-----------------------------------------------------------------------------
// This renders the entire 3D view.
void CViewRender::RenderView( const CViewSetupEx &view, const CViewSetupEx &hudViewSetup, int nClearFlags, int whatToDraw )
{
	m_UnderWaterOverlayMaterial.Shutdown();					// underwater view will set

	m_CurrentView = view;

	C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, true );
	VPROF( "CViewRender::RenderView" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Don't want Left4Dead running less than DX 9
	if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 )
	{
		// We know they were running at least 9.0 when the game started...we check the 
		// value in ClientDLL_Init()...so they must be messing with their DirectX settings.
		Warning( "This game has a minimum requirement of Shader Model 2.0 to run properly.\n" );
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	ITexture *saveRenderTarget = pRenderContext->GetRenderTarget();
	pRenderContext.SafeRelease(); // don't want to hold for long periods in case in a locking active share thread mode

	if ( !m_rbTakeFreezeFrame && m_flFreezeFrameUntil > gpGlobals->curtime )
	{
		CRefPtr<CFreezeFrameView> pFreezeFrameView = new CFreezeFrameView( this );
		pFreezeFrameView->Setup( view );
		AddViewToScene( pFreezeFrameView );

		g_bRenderingView = true;
		AllowCurrentViewAccess( true );
	}
	else
	{
		g_flFreezeFlash = 0.0f;

		g_pClientShadowMgr->AdvanceFrame();

		if ( cl_drawmonitors.GetBool() && 
			( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 70 ) &&
			( ( whatToDraw & RENDERVIEW_SUPPRESSMONITORRENDERING ) == 0 ) )
		{
			const CViewSetupEx &viewMiddle = GetView();
			DrawMonitors( viewMiddle );	

			// Any fake world portals?
			Frustum_t frustum;
			GeneratePerspectiveFrustum( view.origin, view.angles, view.zNear, view.zFar, view.fov, view.m_flAspectRatio, frustum );

			Vector vecAbsPlaneNormal;
			float flLocalPlaneDist;
			C_FuncFakeWorldPortal *pPortalEnt = NextFakeWorldPortal( NULL, view, vecAbsPlaneNormal, flLocalPlaneDist, frustum );
			while ( pPortalEnt != NULL )
			{
				ITexture *pCameraTarget = pPortalEnt->RenderTarget();
				int width = pCameraTarget->GetActualWidth();
				int height = pCameraTarget->GetActualHeight();

				DrawFakeWorldPortal( pCameraTarget, pPortalEnt, viewMiddle, C_BasePlayer::GetLocalPlayer(), 0, 0, width, height, view, vecAbsPlaneNormal, flLocalPlaneDist );

				pPortalEnt = NextFakeWorldPortal( pPortalEnt, view, vecAbsPlaneNormal, flLocalPlaneDist, frustum );
			}
		}

		g_bRenderingView = true;

		RenderPreScene( view );

		// Must be first 
		render->SceneBegin();

		g_pColorCorrectionMgr->UpdateColorCorrection();

		// Send the current tonemap scalar to the material system
		UpdateMaterialSystemTonemapScalar();

		pRenderContext.GetFrom( g_pMaterialSystem );
		pRenderContext->TurnOnToneMapping();
		pRenderContext.SafeRelease();

		// clear happens here probably
		SetupMain3DView( view, hudViewSetup, nClearFlags, saveRenderTarget );

		bool bDrew3dSkybox = false;
		SkyboxVisibility_t nSkyboxVisible = SKYBOX_NOT_VISIBLE;

		// Force it to clear the framebuffer if they're in solid space.
		if ( ( nClearFlags & VIEW_CLEAR_COLOR ) == 0 )
		{
			MDLCACHE_CRITICAL_SECTION();
			if ( enginetrace->GetPointContents( view.origin ) == CONTENTS_SOLID )
			{
				nClearFlags |= VIEW_CLEAR_COLOR;
			}
		}

		PreViewDrawScene( view );

		// For script_intro viewmodel fix
		bool bDrawnViewmodel = false;

		// Render world and all entities, particles, etc.
		if( !g_pIntroData )
		{
			// Don't bother with the skybox if we're drawing an ND buffer for the SFM
			if ( !view.m_bDrawWorldNormal )
			{
				// if the 3d skybox world is drawn, then don't draw the normal skybox
				CSkyboxView *pSkyView = new CSkyboxView( this );
				if ( ( bDrew3dSkybox = pSkyView->Setup( view, &nClearFlags, &nSkyboxVisible ) ) != false )
				{
					AddViewToScene( pSkyView );
				}
				SafeRelease( pSkyView );
			}

			ViewDrawScene( bDrew3dSkybox, nSkyboxVisible, view, nClearFlags, VIEW_MAIN, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );
		}
		else
		{
			ViewDrawScene_Intro( view, nClearFlags, *g_pIntroData, bDrew3dSkybox, nSkyboxVisible, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );
			bDrawnViewmodel = true;
		}

		// We can still use the 'current view' stuff set up in ViewDrawScene
		AllowCurrentViewAccess( true );

		PostViewDrawScene( view );

		engine->DrawPortals();

		DisableFog();

		// Finish scene
		render->SceneEnd();

		// Draw lightsources if enabled
		render->DrawLights();

		RenderPlayerSprites();

		// Image-space motion blur
		if ( !building_cubemaps->GetBool() /*&& view.m_bDoBloomAndToneMapping*/ ) // We probably should use a different view. variable here
		{
			if ( IsDepthOfFieldEnabled() )
			{
				pRenderContext.GetFrom( g_pMaterialSystem );
				{
					PIXEVENT( pRenderContext, "DoDepthOfField()" );
					DoDepthOfField( view );
				}
				pRenderContext.SafeRelease();
			}

			if ( ( view.m_nMotionBlurMode != MOTION_BLUR_DISABLE ) && ( mat_motion_blur_enabled->GetInt() ) && ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 90 ) )
			{
				pRenderContext.GetFrom( g_pMaterialSystem );
				{
					PIXEVENT( pRenderContext, "DoImageSpaceMotionBlur" );
					DoImageSpaceMotionBlur( view, view.x, view.y, view.width, view.height );
				}
				pRenderContext.SafeRelease();
			}
		}

		GetClientModeNormal()->DoPostScreenSpaceEffects( &view );

		// Now actually draw the viewmodel
		if (!bDrawnViewmodel)
			DrawViewModels( view, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );

		DrawUnderwaterOverlay();

		PixelVisibility_EndScene();

		// Draw fade over entire screen if needed
		byte color[4];
		bool blend;
		GetViewEffects()->GetFadeParams( &color[0], &color[1], &color[2], &color[3], &blend );

		// Store off color fade params to be applied in fullscreen postprocess pass
		SetViewFadeParams( color[0], color[1], color[2], color[3], blend );

		// Draw an overlay to make it even harder to see inside smoke particle systems.
		DrawSmokeFogOverlay();

		// Overlay screen fade on entire screen
		IMaterial* pMaterial = blend ? m_ModulateSingleColor : m_TranslucentSingleColor;
		render->ViewDrawFade( color, pMaterial );
		PerformScreenOverlay( view.x, view.y, view.width, view.height );

		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			pRenderContext.GetFrom( g_pMaterialSystem );
			pRenderContext->SetToneMappingScaleLinear( Vector( 1, 1, 1 ) );
			pRenderContext.SafeRelease();
		}

		// Prevent sound stutter if going slow
		engine->Sound_ExtraUpdate();	

		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			pRenderContext.GetFrom( g_pMaterialSystem );
			pRenderContext->SetToneMappingScaleLinear(Vector(1,1,1));
			pRenderContext.SafeRelease();
		}
	
		if ( !building_cubemaps->GetBool() && view.m_bDoBloomAndToneMapping )
		{
			pRenderContext.GetFrom( g_pMaterialSystem );
			{
				PIXEVENT( pRenderContext, "DoEnginePostProcessing" );

				bool bFlashlightIsOn = false;
				C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
				if ( pLocal )
				{
					bFlashlightIsOn = pLocal->IsEffectActive( EF_DIMLIGHT );
				}
				DoEnginePostProcessing( view.x, view.y, view.width, view.height, bFlashlightIsOn );
			}
			pRenderContext.SafeRelease();
		}

		// And here are the screen-space effects

		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "GrabPreColorCorrectedFrame" );

		// Grab the pre-color corrected frame for editing purposes
		engine->GrabPreColorCorrectedFrame( view.x, view.y, view.width, view.height );

		PerformScreenSpaceEffects( 0, 0, view.width, view.height );

		GetClientMode()->DoPostScreenSpaceEffects( &view );

		// Draw client side effects post screen effects
		// NOTE: These are not sorted against the rest of the frame
		clienteffects->DrawEffects( gpGlobals->frametime, BITS_CLIENTEFFECT_POSTSCREEN );	

		CleanupMain3DView( view );

		if ( m_rbTakeFreezeFrame )
		{
			Rect_t rect;
			rect.x = view.x;
			rect.y = view.y;
			rect.width = view.width;
			rect.height = view.height;

			pRenderContext = g_pMaterialSystem->GetRenderContext();

			pRenderContext->CopyRenderTargetToTextureEx( GetFullscreenTexture(), 0, &rect, &rect );

			pRenderContext.SafeRelease();
			m_rbTakeFreezeFrame = false;
		}

		pRenderContext = g_pMaterialSystem->GetRenderContext();
		pRenderContext->SetRenderTarget( saveRenderTarget );
		pRenderContext.SafeRelease();

		// Draw the overlay
		if ( m_bDrawOverlay )
		{	   
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "DrawOverlay" );

			// This allows us to be ok if there are nested overlay views
			CViewSetupEx currentView = m_CurrentView;
			CViewSetupEx tempView = m_OverlayViewSetup;
			tempView.fov = ScaleFOVByWidthRatio( tempView.fov, tempView.m_flAspectRatio / ( 4.0f / 3.0f ) );
			tempView.m_bDoBloomAndToneMapping = false;	// FIXME: Hack to get Mark up and running
			tempView.m_nMotionBlurMode = MOTION_BLUR_DISABLE;		// FIXME: Hack to get Mark up and running
			m_bDrawOverlay = false;
			RenderView( tempView, hudViewSetup, m_OverlayClearFlags, m_OverlayDrawFlags );
			m_CurrentView = currentView;
		}

	}

	if ( mat_viewportupscale.GetBool() && mat_viewportscale.GetFloat() < 1.0f ) 
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

		ITexture	*pFullFrameFB1 = g_pMaterialSystem->FindTexture( "_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET );
		IMaterial	*pCopyMaterial = g_pMaterialSystem->FindMaterial( "dev/upscale", TEXTURE_GROUP_OTHER );
		pCopyMaterial->IncrementReferenceCount();

		Rect_t	DownscaleRect, UpscaleRect;

		DownscaleRect.x = view.x;
		DownscaleRect.y = view.y;
		DownscaleRect.width = view.width;
		DownscaleRect.height = view.height;

		UpscaleRect.x = view.m_nUnscaledX;
		UpscaleRect.y = view.m_nUnscaledY;
		UpscaleRect.width = view.m_nUnscaledWidth;
		UpscaleRect.height = view.m_nUnscaledHeight;

		pRenderContext->CopyRenderTargetToTextureEx( pFullFrameFB1, 0, &DownscaleRect, &DownscaleRect );
		pRenderContext->DrawScreenSpaceRectangle( pCopyMaterial, UpscaleRect.x, UpscaleRect.y, UpscaleRect.width, UpscaleRect.height,
			DownscaleRect.x, DownscaleRect.y, DownscaleRect.x+DownscaleRect.width-1, DownscaleRect.y+DownscaleRect.height-1, 
			pFullFrameFB1->GetActualWidth(), pFullFrameFB1->GetActualHeight() );

		pCopyMaterial->DecrementReferenceCount();
	}

	// Draw the 2D graphics
	m_CurrentView = hudViewSetup;

	render->Push2DView( hudViewSetup, 0, saveRenderTarget, GetFrustum() );

	Render2DEffectsPreHUD( hudViewSetup );

	if ( whatToDraw & RENDERVIEW_DRAWHUD )
	{
		VPROF_BUDGET( "VGui_DrawHud", VPROF_BUDGETGROUP_OTHER_VGUI );
		int viewWidth = hudViewSetup.m_nUnscaledWidth;
		int viewHeight = hudViewSetup.m_nUnscaledHeight;
		int viewActualWidth = hudViewSetup.m_nUnscaledWidth;
		int viewActualHeight = hudViewSetup.m_nUnscaledHeight;
		int viewX = hudViewSetup.m_nUnscaledX;
		int viewY = hudViewSetup.m_nUnscaledY;
		int viewFramebufferX = 0;
		int viewFramebufferY = 0;
		int viewFramebufferWidth = viewWidth;
		int viewFramebufferHeight = viewHeight;
		bool bClear = false;
		bool bPaintMainMenu = false;
		ITexture *pTexture = NULL;

		// Get the render context out of materials to avoid some debug stuff.
		// WARNING THIS REQUIRES THE .SafeRelease below or it'll never release the ref
		pRenderContext = g_pMaterialSystem->GetRenderContext();

		// clear depth in the backbuffer before we push the render target
		if( bClear )
		{
			pRenderContext->ClearBuffers( false, true, true );
		}

		// constrain where VGUI can render to the view
		pRenderContext->PushRenderTargetAndViewport( pTexture, NULL, viewX, viewY, viewActualWidth, viewActualHeight );
		// If drawing off-screen, force alpha for that pass
		if (pTexture)
		{
			pRenderContext->OverrideAlphaWriteEnable( true, true );
		}

		// clear the render target if we need to
		if( bClear )
		{
			pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
			pRenderContext->ClearBuffers( true, false );
		}
		pRenderContext.SafeRelease();

		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "VGui_DrawHud", __FUNCTION__ );

		// paint the vgui screen
		VGui_PreRender();

		CUtlVector< vgui::VPANEL > vecHudPanels;
		vecHudPanels.AddToTail( VGui_GetClientDLLRootPanel() );
		if(VGui_GetFullscreenRootVPANEL() != VGui_GetClientDLLRootPanel())
			vecHudPanels.AddToTail( VGui_GetFullscreenRootVPANEL() );
		vecHudPanels.AddToTail( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
		vecHudPanels.AddToTail( enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS ) );

		// Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
		PositionHudPanels( vecHudPanels, hudViewSetup );

		// The crosshair, etc. needs to get at the current setup stuff
		AllowCurrentViewAccess( true );

		// Draw the in-game stuff based on the actual viewport being used
		render->VGui_Paint( PAINT_INGAMEPANELS );

		// maybe paint the main menu and cursor too if we're in stereo hud mode
		if( bPaintMainMenu )
			render->VGui_Paint( PAINT_UIPANELS | PAINT_CURSOR );

		AllowCurrentViewAccess( false );

		VGui_PostRender();

		GetClientMode()->PostRenderVGui();
		pRenderContext = g_pMaterialSystem->GetRenderContext();
		if (pTexture)
		{
			pRenderContext->OverrideAlphaWriteEnable( false, true );
		}
		pRenderContext->PopRenderTargetAndViewport();

		pRenderContext->Flush();
		pRenderContext.SafeRelease();
	}

	CDebugViewRender::Draw2DDebuggingInfo( hudViewSetup );

	Render2DEffectsPostHUD( hudViewSetup );

	g_bRenderingView = false;

	// We can no longer use the 'current view' stuff set up in ViewDrawScene
	s_bCanAccessCurrentView = false;

	CDebugViewRender::GenerateOverdrawForTesting();

	render->PopView( GetFrustum() );
	g_WorldListCache.Flush();

	m_CurrentView = view;

	g_pModelRenderSystem->CleanupRenderData();
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPreHUD( const CViewSetupEx &view )
{
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPostHUD( const CViewSetupEx &view )
{
}



//-----------------------------------------------------------------------------
//
// NOTE: Below here is all of the stuff that needs to be done for water rendering
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Determines what kind of water we're going to use
//-----------------------------------------------------------------------------
void CViewRender::DetermineWaterRenderInfo( const VisibleFogVolumeInfo_t &fogVolumeInfo, WaterRenderInfo_t &info )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// By default, assume cheap water (even if there's no water in the scene!)
	info.m_bCheapWater = true;
	info.m_bRefract = false;
	info.m_bReflect = false;
	info.m_bReflectEntities = false;
	info.m_bDrawWaterSurface = false;
	info.m_bOpaqueWater = true;



	IMaterial *pWaterMaterial = fogVolumeInfo.m_pFogVolumeMaterial;
	if (( fogVolumeInfo.m_nVisibleFogVolume == -1 ) || !pWaterMaterial )
		return;


	// Use cheap water if mat_drawwater is set
	info.m_bDrawWaterSurface = mat_drawwater.GetBool();
	if ( !info.m_bDrawWaterSurface )
	{
		info.m_bOpaqueWater = false;
		return;
	}

	bool bForceExpensive = r_waterforceexpensive->GetBool();
	bool bForceReflectEntities = r_waterforcereflectentities->GetBool();

#ifdef PORTAL
	switch( g_pPortalRender->ShouldForceCheaperWaterLevel() )
	{
	case 0: //force cheap water
		info.m_bCheapWater = true;
		return;

	case 1: //downgrade level to "simple reflection"
		bForceExpensive = false;

	case 2: //downgrade level to "reflect world"
		bForceReflectEntities = false;
	
	default:
		break;
	};
#endif

	// Determine if the water surface is opaque or not
	info.m_bOpaqueWater = !pWaterMaterial->IsTranslucent();

	// DX level 70 can't handle anything but cheap water
	if (engine->GetDXSupportLevel() < 80)
		return;

	bool bForceCheap = false;

	// The material can override the default settings though
	IMaterialVar *pForceCheapVar = pWaterMaterial->FindVar( "$forcecheap", NULL, false );
	IMaterialVar *pForceExpensiveVar = pWaterMaterial->FindVar( "$forceexpensive", NULL, false );
	if ( pForceCheapVar && pForceCheapVar->IsDefined() )
	{
		bForceCheap = ( pForceCheapVar->GetIntValueFast() != 0 );
		if ( bForceCheap )
		{
			bForceExpensive = false;
		}
	}
	if ( !bForceCheap && pForceExpensiveVar && pForceExpensiveVar->IsDefined() )
	{
		 bForceExpensive = bForceExpensive || ( pForceExpensiveVar->GetIntValueFast() != 0 );
	}

	bool bDebugCheapWater = r_debugcheapwater.GetBool();
	if( bDebugCheapWater )
	{
		Msg( "Water material: %s dist to water: %f\nforcecheap: %s forceexpensive: %s\n", 
			pWaterMaterial->GetName(), fogVolumeInfo.m_flDistanceToWater, 
			bForceCheap ? "true" : "false", bForceExpensive ? "true" : "false" );
	}

	// Unless expensive water is active, reflections are off.
	bool bLocalReflection;
	if( !bForceExpensive || !r_WaterDrawReflection.GetBool() )
	{
		bLocalReflection = false;
	}
	else
	{
		IMaterialVar *pReflectTextureVar = pWaterMaterial->FindVar( "$reflecttexture", NULL, false );
		bLocalReflection = pReflectTextureVar && (pReflectTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);
	}

	// Brian says FIXME: I disabled cheap water LOD when local specular is specified.
	// There are very few places that appear to actually
	// take advantage of it (places where water is in the PVS, but outside of LOD range).
	// It was 2 hours before code lock, and I had the choice of either doubling fill-rate everywhere
	// by making cheap water lod actually work (the water LOD wasn't actually rendering!!!)
	// or to just always render the reflection + refraction if there's a local specular specified.
	// Note that water LOD *does* work with refract-only water

	// Gary says: I'm reverting this change so that water LOD works on dx9 for ep2.

	// Check if the water is out of the cheap water LOD range; if so, use cheap water
	if ( ( (fogVolumeInfo.m_flDistanceToWater >= m_flCheapWaterEndDistance) && !bLocalReflection ) || bForceCheap )
 		return;

	// Get the material that is for the water surface that is visible and check to see
	// what render targets need to be rendered, if any.
	if ( !r_WaterDrawRefraction.GetBool() )
	{
		info.m_bRefract = false;
	}
	else
	{
		IMaterialVar *pRefractTextureVar = pWaterMaterial->FindVar( "$refracttexture", NULL, false );
		info.m_bRefract = pRefractTextureVar && (pRefractTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);

		// Refractive water can be seen through
		if ( info.m_bRefract )
		{
			info.m_bOpaqueWater = false;
		}
	}

	info.m_bReflect = bLocalReflection;
	if ( info.m_bReflect )
	{
		if( bForceReflectEntities )
		{
			info.m_bReflectEntities = true;
		}
		else
		{
			IMaterialVar *pReflectEntitiesVar = pWaterMaterial->FindVar( "$reflectentities", NULL, false );
			info.m_bReflectEntities = pReflectEntitiesVar && (pReflectEntitiesVar->GetIntValueFast() != 0);
		}
	}

	info.m_bCheapWater = !info.m_bReflect && !info.m_bRefract;

	if( bDebugCheapWater )
	{
		Warning( "refract: %s reflect: %s\n", info.m_bRefract ? "true" : "false", info.m_bReflect ? "true" : "false" );
	}
}

//-----------------------------------------------------------------------------
// Draws the world and all entities
//-----------------------------------------------------------------------------
void CViewRender::DrawWorldAndEntities( bool bDrawSkybox, const CViewSetupEx &viewIn, int nClearFlags, ViewCustomVisibility_t *pCustomVisibility )
{
	MDLCACHE_CRITICAL_SECTION();
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	VisibleFogVolumeInfo_t fogVolumeInfo;
#ifdef PORTAL //in portal, we can't use the fog volume for the camera since it's almost never in the same fog volume as what's in front of the portal
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		render->GetVisibleFogVolume( viewIn.origin, &fogVolumeInfo );
	}
	else
	{
		render->GetVisibleFogVolume( g_pPortalRender->GetExitPortalFogOrigin(), &fogVolumeInfo );
	}
#else
	render->GetVisibleFogVolume( viewIn.origin, &fogVolumeInfo );
#endif

	WaterRenderInfo_t info;
	DetermineWaterRenderInfo( fogVolumeInfo, info );

	if ( info.m_bCheapWater )
	{		     
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "bCheapWater" );
		cplane_t glassReflectionPlane;
		// New expansions allow for custom render targets and multiple mirror renders
		Frustum_t frustum;
		GeneratePerspectiveFrustum( viewIn.origin, viewIn.angles, viewIn.zNear, viewIn.zFar, viewIn.fov, viewIn.m_flAspectRatio, frustum );

		ITexture *pTextureTargets[2];
		C_BaseEntity *pReflectiveGlass = NextReflectiveGlass( NULL, viewIn, glassReflectionPlane, frustum, pTextureTargets );
		while ( pReflectiveGlass != NULL )
		{		
			if (pTextureTargets[0])
			{
				CRefPtr<CReflectiveGlassView> pGlassReflectionView = new CReflectiveGlassView( this );
				pGlassReflectionView->m_pRenderTarget = pTextureTargets[0];
				pGlassReflectionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, bDrawSkybox, fogVolumeInfo, info, glassReflectionPlane );
				AddViewToScene( pGlassReflectionView );
			}

			if (pTextureTargets[1])
			{
				CRefPtr<CRefractiveGlassView> pGlassRefractionView = new CRefractiveGlassView( this );
				pGlassRefractionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, bDrawSkybox, fogVolumeInfo, info, glassReflectionPlane );
				pGlassRefractionView->m_pRenderTarget = pTextureTargets[1];
				AddViewToScene( pGlassRefractionView );
			}

			pReflectiveGlass = NextReflectiveGlass( pReflectiveGlass, viewIn, glassReflectionPlane, frustum, pTextureTargets );
		}

		CRefPtr<CSimpleWorldView> pNoWaterView = new CSimpleWorldView( this );
		pNoWaterView->Setup( viewIn, nClearFlags, bDrawSkybox, fogVolumeInfo, info, pCustomVisibility );
		AddViewToScene( pNoWaterView );
		return;
	}

	Assert( !pCustomVisibility );

	// Blat out the visible fog leaf if we're not going to use it
	if ( !r_ForceWaterLeaf.GetBool() )
	{
		fogVolumeInfo.m_nVisibleFogVolumeLeaf = -1;
	}

	// We can see water of some sort
	if ( !fogVolumeInfo.m_bEyeInFogVolume )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "CAboveWaterView" );
		CRefPtr<CAboveWaterView> pAboveWaterView = new CAboveWaterView( this );
		pAboveWaterView->Setup( viewIn, bDrawSkybox, fogVolumeInfo, info );
		AddViewToScene( pAboveWaterView );
	}
	else
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "CUnderWaterView" );
		CRefPtr<CUnderWaterView> pUnderWaterView = new CUnderWaterView( this );
		pUnderWaterView->Setup( viewIn, bDrawSkybox, fogVolumeInfo, info );
		AddViewToScene( pUnderWaterView );
	}
}


//-----------------------------------------------------------------------------
// Pushes a water render target
//-----------------------------------------------------------------------------
static Vector SavedLinearLightMapScale(-1,-1,-1);			// x<0 = no saved scale

static void SetLightmapScaleForWater(void)
{
	if (g_pMaterialSystemHardwareConfig->GetHDRType()==HDR_TYPE_INTEGER)
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		SavedLinearLightMapScale=pRenderContext->GetToneMappingScaleLinear();
		Vector t25=SavedLinearLightMapScale;
		t25*=0.25;
		pRenderContext->SetToneMappingScaleLinear(t25);
	}
}

//-----------------------------------------------------------------------------
// Returns true if the view plane intersects the water
//-----------------------------------------------------------------------------
bool DoesViewPlaneIntersectWater( float waterZ, int leafWaterDataID )
{
	if ( leafWaterDataID == -1 )
		return false;

#ifdef PORTAL //when rendering portal views point/plane intersections just don't cut it.
	if( g_pPortalRender->GetViewRecursionLevel() != 0 )
		return g_pPortalRender->DoesExitPortalViewIntersectWaterPlane( waterZ, leafWaterDataID );
#endif

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	
	VMatrix viewMatrix, projectionMatrix, viewProjectionMatrix, inverseViewProjectionMatrix;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	pRenderContext->GetMatrix( MATERIAL_PROJECTION, &projectionMatrix );
	MatrixMultiply( projectionMatrix, viewMatrix, viewProjectionMatrix );
	MatrixInverseGeneral( viewProjectionMatrix, inverseViewProjectionMatrix );

	Vector mins, maxs;
	ClearBounds( mins, maxs );
	Vector testPoint[4];
	testPoint[0].Init( -1.0f, -1.0f, 0.0f );
	testPoint[1].Init( -1.0f, 1.0f, 0.0f );
	testPoint[2].Init( 1.0f, -1.0f, 0.0f );
	testPoint[3].Init( 1.0f, 1.0f, 0.0f );
	int i;
	bool bAbove = false;
	bool bBelow = false;
	float fudge = 7.0f;
	for( i = 0; i < 4; i++ )
	{
		Vector worldPos;
		Vector3DMultiplyPositionProjective( inverseViewProjectionMatrix, testPoint[i], worldPos );
		AddPointToBounds( worldPos, mins, maxs );
//		Warning( "viewplanez: %f waterZ: %f\n", worldPos.z, waterZ );
		if( worldPos.z + fudge > waterZ )
		{
			bAbove = true;
		}
		if( worldPos.z - fudge < waterZ )
		{
			bBelow = true;
		}
	}

	// early out if the near plane doesn't cross the z plane of the water.
	if( !( bAbove && bBelow ) )
		return false;

	Vector vecFudge( fudge, fudge, fudge );
	mins -= vecFudge;
	maxs += vecFudge;
	
	// the near plane does cross the z value for the visible water volume.  Call into
	// the engine to find out if the near plane intersects the water volume.
	return render->DoesBoxIntersectWaterVolume( mins, maxs, leafWaterDataID );
} 

#ifdef PORTAL 

//-----------------------------------------------------------------------------
// Purpose: Draw the scene during another draw scene call. We must draw our portals
//			after opaques but before translucents, so this ViewDrawScene resets the view
//			and doesn't flag the rendering as ended when it ends.
// Input  : bDrawSkybox - do we draw the skybox
//			&view - the camera view to render from
//			nClearFlags -  how to clear the buffer
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_PortalStencil( const CViewSetupEx &viewIn, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::ViewDrawScene_PortalStencil" );

	CViewSetupEx view( viewIn );

	// Record old view stats
	Vector vecOldOrigin = CurrentViewOrigin();
	QAngle vecOldAngles = CurrentViewAngles();

	int iCurrentViewID = g_CurrentViewID;
	int iRecursionLevel = g_pPortalRender->GetViewRecursionLevel();
	Assert( iRecursionLevel > 0 );

	//get references to reflection textures
	CTextureReference pPrimaryWaterReflectionTexture;
	pPrimaryWaterReflectionTexture.Init( GetWaterReflectionTexture() );
	CTextureReference pReplacementWaterReflectionTexture;
	pReplacementWaterReflectionTexture.Init( portalrendertargets->GetWaterReflectionTextureForStencilDepth( iRecursionLevel ) );

	//get references to refraction textures
	CTextureReference pPrimaryWaterRefractionTexture;
	pPrimaryWaterRefractionTexture.Init( GetWaterRefractionTexture() );
	CTextureReference pReplacementWaterRefractionTexture;
	pReplacementWaterRefractionTexture.Init( portalrendertargets->GetWaterRefractionTextureForStencilDepth( iRecursionLevel ) );


	//swap texture contents for the primary render targets with those we set aside for this recursion level
	if( pReplacementWaterReflectionTexture != NULL )
		pPrimaryWaterReflectionTexture->SwapContents( pReplacementWaterReflectionTexture );

	if( pReplacementWaterRefractionTexture != NULL )
		pPrimaryWaterRefractionTexture->SwapContents( pReplacementWaterRefractionTexture );

	bool bDrew3dSkybox = false;
	SkyboxVisibility_t nSkyboxVisible = SKYBOX_NOT_VISIBLE;
	int iClearFlags = 0;

	Draw3dSkyboxworld_Portal( view, iClearFlags, bDrew3dSkybox, nSkyboxVisible );

	bool drawSkybox = r_skybox.GetBool();
	if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
	{
		drawSkybox = false;
	}

	//generate unique view ID's for each stencil view
	view_id_t iNewViewID = (view_id_t)g_pPortalRender->GetCurrentViewId();
	SetupCurrentView( view.origin, view.angles, (view_id_t)iNewViewID );
	
	// update vis data
	unsigned int visFlags;
	SetupVis( view, visFlags, pCustomVisibility );

	VisibleFogVolumeInfo_t fogInfo;
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
	{
		render->GetVisibleFogVolume( view.origin, &fogInfo );
	}
	else
	{
		render->GetVisibleFogVolume( g_pPortalRender->GetExitPortalFogOrigin(), &fogInfo );
	}

	WaterRenderInfo_t waterInfo;
	DetermineWaterRenderInfo( fogInfo, waterInfo );

	if ( waterInfo.m_bCheapWater )
	{		     
		cplane_t glassReflectionPlane;
		if ( IsReflectiveGlassInView( viewIn, glassReflectionPlane ) )
		{								    
			CRefPtr<CReflectiveGlassView> pGlassReflectionView = new CReflectiveGlassView( this );
			pGlassReflectionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, glassReflectionPlane );
			AddViewToScene( pGlassReflectionView );

			CRefPtr<CRefractiveGlassView> pGlassRefractionView = new CRefractiveGlassView( this );
			pGlassRefractionView->Setup( viewIn, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, glassReflectionPlane );
			AddViewToScene( pGlassRefractionView );
		}

		CSimpleWorldView *pClientView = new CSimpleWorldView( this );
		pClientView->Setup( view, VIEW_CLEAR_OBEY_STENCIL, drawSkybox, fogInfo, waterInfo, pCustomVisibility );
		AddViewToScene( pClientView );
		SafeRelease( pClientView );
	}
	else
	{
		// We can see water of some sort
		if ( !fogInfo.m_bEyeInFogVolume )
		{
			CRefPtr<CAboveWaterView> pAboveWaterView = new CAboveWaterView( this );
			pAboveWaterView->Setup( viewIn, drawSkybox, fogInfo, waterInfo );
			AddViewToScene( pAboveWaterView );
		}
		else
		{
			CRefPtr<CUnderWaterView> pUnderWaterView = new CUnderWaterView( this );
			pUnderWaterView->Setup( viewIn, drawSkybox, fogInfo, waterInfo );
			AddViewToScene( pUnderWaterView );
		}
	}

	// Disable fog for the rest of the stuff
	DisableFog();

	CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );

	// Draw rain..
	DrawPrecipitation();

	//prerender version only
	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( view );

	// Return to the previous view
	SetupCurrentView( vecOldOrigin, vecOldAngles, (view_id_t)iCurrentViewID );
	g_CurrentViewID = iCurrentViewID; //just in case the cast to view_id_t screwed up the id #


	//swap back the water render targets
	if( pReplacementWaterReflectionTexture != NULL )
		pPrimaryWaterReflectionTexture->SwapContents( pReplacementWaterReflectionTexture );

	if( pReplacementWaterRefractionTexture != NULL )
		pPrimaryWaterRefractionTexture->SwapContents( pReplacementWaterRefractionTexture );
}

void CViewRender::Draw3dSkyboxworld_Portal( const CViewSetupEx &view, int &nClearFlags, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible, ITexture *pRenderTarget ) 
{ 
	CRefPtr<CPortalSkyboxView> pSkyView = new CPortalSkyboxView( this ); 
	if ( ( bDrew3dSkybox = pSkyView->Setup( view, &nClearFlags, &nSkyboxVisible, pRenderTarget ) ) == true )
	{
		AddViewToScene( pSkyView );
	}
}

#endif //PORTAL

//-----------------------------------------------------------------------------
// Methods related to controlling the cheap water distance
//-----------------------------------------------------------------------------
void CViewRender::SetCheapWaterStartDistance( float flCheapWaterStartDistance )
{
	m_flCheapWaterStartDistance = flCheapWaterStartDistance;
}

void CViewRender::SetCheapWaterEndDistance( float flCheapWaterEndDistance )
{
	m_flCheapWaterEndDistance = flCheapWaterEndDistance;
}

void CViewRender::GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance )
{
	flCheapWaterStartDistance = m_flCheapWaterStartDistance;
	flCheapWaterEndDistance = m_flCheapWaterEndDistance;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &view - 
//			&introData - 
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_Intro( const CViewSetupEx &view, int nClearFlags, const IntroData_t &introData,
	bool bDrew3dSkybox, SkyboxVisibility_t nSkyboxVisible, bool bDrawViewModel, ViewCustomVisibility_t *pCustomVisibility )
{
	VPROF( "CViewRender::ViewDrawScene" );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// this allows the refract texture to be updated once per *scene* on 360
	// (e.g. once for a monitor scene and once for the main scene)
	g_viewscene_refractUpdateFrame = gpGlobals->framecount - 1;

	// -----------------------------------------------------------------------
	// Set the clear color to black since we are going to be adding up things
	// in the frame buffer.
	// -----------------------------------------------------------------------
	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	
	// -----------------------------------------------------------------------
	// Draw the primary scene and copy it to the first framebuffer texture
	// -----------------------------------------------------------------------	
	unsigned int visFlags;

	// NOTE: We only increment this once since time doesn't move forward.
	ParticleMgr()->IncrementFrameCode();

	if( introData.m_bDrawPrimary  )
	{
		CViewSetupEx playerView( view );
		playerView.origin = introData.m_vecCameraView;
		playerView.angles = introData.m_vecCameraViewAngles;

		// Ortho handling (change this code if we ever use m_hCameraEntity for other things)
		if (introData.m_hCameraEntity /*&& introData.m_hCameraEntity->IsOrtho()*/)
		{
			playerView.m_bOrtho = true;
			introData.m_hCameraEntity->GetOrthoDimensions( playerView.m_OrthoTop, playerView.m_OrthoBottom,
				playerView.m_OrthoLeft, playerView.m_OrthoRight );
		}

		if ( introData.m_playerViewFOV )
		{
			playerView.fov = ScaleFOVByWidthRatio( introData.m_playerViewFOV, engine->GetScreenAspectRatio() / ( 4.0f / 3.0f ) );
		}

		bool drawSkybox;
		int nViewFlags;
		if (introData.m_bDrawSky2)
		{
			drawSkybox = r_skybox.GetBool();
			nViewFlags = VIEW_CLEAR_DEPTH;

			// if the 3d skybox world is drawn, then don't draw the normal skybox
			CSkyboxView *pSkyView = new CSkyboxView( this );
			if ( ( bDrew3dSkybox = pSkyView->Setup( playerView, &nClearFlags, &nSkyboxVisible ) ) != false )
			{
				AddViewToScene( pSkyView );
			}
			SafeRelease( pSkyView );
		}
		else
		{
			drawSkybox = false;
			nViewFlags = (VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH);
		}

		g_pClientShadowMgr->PreRender();

		// Shadowed flashlights supported on ps_2_b and up...
		if ( r_flashlightdepthtexture->GetBool() )
		{
			g_pClientShadowMgr->ComputeShadowDepthTextures( playerView );
		}

		SetupCurrentView( playerView.origin, playerView.angles, VIEW_INTRO_PLAYER );

		// Invoke pre-render methods
		IGameSystem::PreRenderAllSystems();

		// Start view, clear frame/z buffer if necessary
		SetupVis( playerView, visFlags, pCustomVisibility );

		if (introData.m_bDrawSky2)
		{
			if ( !bDrew3dSkybox && 
				( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) /*&& ( visFlags & IVRenderView::VIEW_SETUP_VIS_EX_RETURN_FLAGS_USES_RADIAL_VIS )*/ )
			{
				// This covers the case where we don't see a 3dskybox, yet radial vis is clipping
				// the far plane.  Need to clear to fog color in this case.
				nClearFlags |= VIEW_CLEAR_COLOR;
				//SetClearColorToFogColor( );
			}
		
			if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
			{
				drawSkybox = false;
			}
		}
		
		render->Push3DView( playerView, nViewFlags, NULL, GetFrustum() );
		DrawWorldAndEntities( drawSkybox, playerView, nViewFlags );
		render->PopView( GetFrustum() );

		// Free shadow depth textures for use in future view
		if ( r_flashlightdepthtexture->GetBool() )
		{
			g_pClientShadowMgr->UnlockAllShadowDepthTextures();
		}
	}
	else
	{
		pRenderContext->ClearBuffers( true, true );
	}
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, view.x, view.y, view.width, view.height, false, &actualRect );

	if (introData.m_bDrawSky)
	{
		// if the 3d skybox world is drawn, then don't draw the normal skybox
		CSkyboxView *pSkyView = new CSkyboxView( this );
		if ( ( bDrew3dSkybox = pSkyView->Setup( view, &nClearFlags, &nSkyboxVisible ) ) != false )
		{
			AddViewToScene( pSkyView );
		}
		SafeRelease( pSkyView );
	}

	g_pClientShadowMgr->PreRender();

	// Shadowed flashlights supported on ps_2_b and up...
	if ( r_flashlightdepthtexture->GetBool() )
	{
		g_pClientShadowMgr->ComputeShadowDepthTextures( view );
	}

	// -----------------------------------------------------------------------
	// Draw the secondary scene and copy it to the second framebuffer texture
	// -----------------------------------------------------------------------
	SetupCurrentView( view.origin, view.angles, VIEW_INTRO_CAMERA );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view, clear frame/z buffer if necessary
	SetupVis( view, visFlags );

	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );

	bool drawSkybox;
	int nViewFlags;
	if (introData.m_bDrawSky)
	{
		drawSkybox = r_skybox.GetBool();
		nViewFlags = VIEW_CLEAR_DEPTH;

		if ( !bDrew3dSkybox && 
			( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) /*&& ( visFlags & IVRenderView::VIEW_SETUP_VIS_EX_RETURN_FLAGS_USES_RADIAL_VIS )*/ )
		{
			// This covers the case where we don't see a 3dskybox, yet radial vis is clipping
			// the far plane.  Need to clear to fog color in this case.
			nViewFlags |= VIEW_CLEAR_COLOR;
			//SetClearColorToFogColor( );
		}

		if ( bDrew3dSkybox || ( nSkyboxVisible == SKYBOX_NOT_VISIBLE ) )
		{
			drawSkybox = false;
		}
	}
	else
	{
		drawSkybox = false;
		nViewFlags = (VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH);
	}

	DrawWorldAndEntities( drawSkybox, view, nViewFlags );

	// Solution for viewmodels not drawing in script_intro
	DrawViewModels( view, bDrawViewModel );

	UpdateScreenEffectTexture( 1, view.x, view.y, view.width, view.height );

	// -----------------------------------------------------------------------
	// Draw quads on the screen for each screenspace pass.
	// -----------------------------------------------------------------------
	// Find the material that we use to render the overlays
	IMaterial *pOverlayMaterial = g_pMaterialSystem->FindMaterial( "scripted/intro_screenspaceeffect", TEXTURE_GROUP_OTHER );
	IMaterialVar *pModeVar = pOverlayMaterial->FindVar( "$mode", NULL );
	IMaterialVar *pAlphaVar = pOverlayMaterial->FindVar( "$alpha", NULL );

	pRenderContext->ClearBuffers( true, true );
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	
	int passID;
	for( passID = 0; passID < introData.m_Passes.Count(); passID++ )
	{
		const IntroDataBlendPass_t& pass = introData.m_Passes[passID];
		if ( pass.m_Alpha == 0 )
			continue;

		// Pick one of the blend modes for the material.
		if ( pass.m_BlendMode >= 0 && pass.m_BlendMode <= 9 )
		{
			pModeVar->SetIntValue( pass.m_BlendMode );
		}
		else
		{
			Assert(0);
		}
		// Set the alpha value for the material.
		pAlphaVar->SetFloatValue( pass.m_Alpha );
		
		// Draw a quad for this pass.
		ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );
		pRenderContext->DrawScreenSpaceRectangle( pOverlayMaterial, 0, 0, view.width, view.height,
											actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
											pTexture->GetActualWidth(), pTexture->GetActualHeight() );
	}
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
	
	// Draw the starfield
	// FIXME
	// blur?
	
	// Disable fog for the rest of the stuff
	DisableFog();
	
	// Here are the overlays...
	CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );

	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// And here are the screen-space effects
	PerformScreenSpaceEffects( 0, 0, view.width, view.height );

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	CDebugViewRender::Draw3DDebuggingInfo( view );

	// Let the particle manager simulate things that haven't been simulated.
	ParticleMgr()->PostRender();

	// Invoke post-render methods
	IGameSystem::PostRenderAllSystems();

	FinishCurrentView();

	// Free shadow depth textures for use in future view
	if ( r_flashlightdepthtexture->GetBool() )
	{
		g_pClientShadowMgr->UnlockAllShadowDepthTextures();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets up scene and renders camera view
// Input  : cameraNum - 
//			&cameraView
//			*localPlayer - 
//			x - 
//			y - 
//			width - 
//			height - 
//			highend - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::DrawOneMonitor( ITexture *pRenderTarget, int cameraNum, C_PointCamera *pCameraEnt, 
	const CViewSetupEx &cameraView, C_BasePlayer *localPlayer, int x, int y, int width, int height )
{
	VPROF_INCREMENT_COUNTER( "cameras rendered", 1 );
	// Setup fog state for the camera.
	fogparams_t oldFogParams;
	float flOldZFar = 0.0f;

	bool bSky = pCameraEnt->IsSkyEnabled();

	bool fogEnabled = pCameraEnt->IsFogEnabled();

	CViewSetupEx monitorView = cameraView;

	fogparams_t *pFogParams = NULL;

	if ( fogEnabled )
	{	
		if ( !localPlayer )
			return false;

		pFogParams = localPlayer->GetFogParams();

		// Save old fog data.
		oldFogParams = *pFogParams;
		flOldZFar = cameraView.zFar;

		pFogParams->enable = true;
		pFogParams->start = pCameraEnt->GetFogStart();
		pFogParams->end = pCameraEnt->GetFogEnd();
		pFogParams->farz = pCameraEnt->GetFogEnd();
		pFogParams->maxdensity = pCameraEnt->GetFogMaxDensity();

		unsigned char r, g, b;
		pCameraEnt->GetFogColor( r, g, b );
		pFogParams->colorPrimary.SetR( r );
		pFogParams->colorPrimary.SetG( g );
		pFogParams->colorPrimary.SetB( b );

		monitorView.zFar = pCameraEnt->GetFogEnd();
	}

	monitorView.width = width;
	monitorView.height = height;
	monitorView.x = x;
	monitorView.y = y;
	monitorView.origin = pCameraEnt->GetAbsOrigin();
	monitorView.angles = pCameraEnt->GetAbsAngles();
	monitorView.fov = pCameraEnt->GetFOV();
	if (pCameraEnt->IsOrtho())
	{
		monitorView.m_bOrtho = true;
		pCameraEnt->GetOrthoDimensions( monitorView.m_OrthoTop, monitorView.m_OrthoBottom,
			monitorView.m_OrthoLeft, monitorView.m_OrthoRight );
	}
	else
	{
		monitorView.m_bOrtho = false;
	}
	monitorView.m_flAspectRatio = pCameraEnt->UseScreenAspectRatio() ? 0.0f : 1.0f;
	monitorView.m_bViewToProjectionOverride = false;

	// 
	// Monitor sky handling
	// 
	SkyboxVisibility_t nSkyMode = pCameraEnt->SkyMode();
	if ( nSkyMode == SKYBOX_3DSKYBOX_VISIBLE )
	{
		int nClearFlags = (VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR);
		bool bDrew3dSkybox = false;

		Frustum frustum;
		render->Push3DView( monitorView, nClearFlags, pRenderTarget, (VPlane *)frustum );

		// if the 3d skybox world is drawn, then don't draw the normal skybox
		CSkyboxView *pSkyView = new CSkyboxView( this );
		if ( ( bDrew3dSkybox = pSkyView->Setup( monitorView, &nClearFlags, &nSkyMode ) ) != false )
		{
			AddViewToScene( pSkyView );
		}
		SafeRelease( pSkyView );

		ViewDrawScene( bDrew3dSkybox, nSkyMode, monitorView, nClearFlags, VIEW_MONITOR );
 		render->PopView( frustum );
	}
	else if (nSkyMode == SKYBOX_NOT_VISIBLE)
	{
		// @MULTICORE (toml 8/11/2006): this should be a renderer....
		Frustum frustum;
		render->Push3DView( monitorView, VIEW_CLEAR_DEPTH, pRenderTarget, (VPlane *)frustum );

		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		pRenderContext->PushRenderTargetAndViewport( pRenderTarget );
		pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, 1 );
		if ( pRenderTarget )
		{
			pRenderContext->OverrideAlphaWriteEnable( true, true );
		}

		ViewDrawScene( false, nSkyMode, monitorView, 0, VIEW_MONITOR );

		pRenderContext->PopRenderTargetAndViewport();
		render->PopView( frustum );
	}
	else
	{
		// @MULTICORE (toml 8/11/2006): this should be a renderer....
		Frustum frustum;
		render->Push3DView( monitorView, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pRenderTarget, (VPlane *)frustum );
		ViewDrawScene( false, nSkyMode, monitorView, 0, VIEW_MONITOR );
		render->PopView( frustum );
	}

	// Reset the world fog parameters.
	if ( fogEnabled )
	{
		if ( pFogParams )
		{
			*pFogParams = oldFogParams;
		}
		monitorView.zFar = flOldZFar;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets up scene and renders WIP fake world portal view.
//			Based on code from monitors, mirrors, and logic_measure_movement.
//			
// Input  : cameraNum - 
//			&cameraView
//			*localPlayer - 
//			x - 
//			y - 
//			width - 
//			height - 
//			highend - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::DrawFakeWorldPortal( ITexture *pRenderTarget, C_FuncFakeWorldPortal *pCameraEnt, const CViewSetupEx &cameraView, C_BasePlayer *localPlayer, 
						int x, int y, int width, int height,
						const CViewSetupEx &mainView, const Vector &vecAbsPlaneNormal, float flLocalPlaneDist )
{
	VPROF_INCREMENT_COUNTER( "cameras rendered", 1 );
	// Setup fog state for the camera.
	fogparams_t oldFogParams;
	float flOldZFar = 0.0f;

	// If fog should be disabled instead of using the player's controller, a blank fog controller can just be used
	bool fogEnabled = true; //pCameraEnt->IsFogEnabled();

	CViewSetup monitorView = cameraView;

	fogparams_t *pFogParams = NULL;

	if ( fogEnabled )
	{	
		if ( !localPlayer )
			return false;

		pFogParams = localPlayer->GetFogParams();

		// Save old fog data.
		oldFogParams = *pFogParams;

		if ( pCameraEnt->GetFog() )
		{
			*pFogParams = *pCameraEnt->GetFog();
		}
	}

	monitorView.x = x;
	monitorView.y = y;
	monitorView.width = width;
	monitorView.height = height;
	monitorView.m_bOrtho = mainView.m_bOrtho;
	monitorView.fov = mainView.fov;
	monitorView.m_flAspectRatio = mainView.m_flAspectRatio;
	monitorView.m_bViewToProjectionOverride = false;

	matrix3x4_t worldToView;
	AngleIMatrix( mainView.angles, mainView.origin, worldToView );

	matrix3x4_t targetToWorld;
	{
		// NOTE: m_PlaneAngles is angle offset
		QAngle targetAngles = pCameraEnt->m_hTargetPlane->GetAbsAngles() - pCameraEnt->m_PlaneAngles;
		AngleMatrix( targetAngles, pCameraEnt->m_hTargetPlane->GetAbsOrigin(), targetToWorld );
	}

	matrix3x4_t portalToWorld;
	{
		Vector left, up;
		VectorVectors( vecAbsPlaneNormal, left, up );
		VectorNegate( left );
		portalToWorld.Init( vecAbsPlaneNormal, left, up, pCameraEnt->GetAbsOrigin() );
	}

	matrix3x4_t portalToView;
	ConcatTransforms( worldToView, portalToWorld, portalToView );

	if ( pCameraEnt->m_flScale > 0.0f )
	{
		portalToView[0][3] /= pCameraEnt->m_flScale;
		portalToView[1][3] /= pCameraEnt->m_flScale;
		portalToView[2][3] /= pCameraEnt->m_flScale;
	}

	matrix3x4_t viewToPortal;
	MatrixInvert( portalToView, viewToPortal );

	matrix3x4_t newViewToWorld;
	ConcatTransforms( targetToWorld, viewToPortal, newViewToWorld );

	MatrixAngles( newViewToWorld, monitorView.angles, monitorView.origin );


	// @MULTICORE (toml 8/11/2006): this should be a renderer....
	int nClearFlags = (VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_OBEY_STENCIL);
	bool bDrew3dSkybox = false;

	Frustum frustum;
	render->Push3DView( monitorView, nClearFlags, pRenderTarget, (VPlane *)frustum );

	// 
	// Sky handling
	// 
	SkyboxVisibility_t nSkyMode = pCameraEnt->SkyMode();
	if ( nSkyMode == SKYBOX_3DSKYBOX_VISIBLE )
	{
		// if the 3d skybox world is drawn, then don't draw the normal skybox
		CSkyboxView *pSkyView = new CSkyboxView( this );
		if ( ( bDrew3dSkybox = pSkyView->Setup( monitorView, &nClearFlags, &nSkyMode ) ) != false )
		{
			AddViewToScene( pSkyView );
		}
		SafeRelease( pSkyView );
	}

	Vector4D plane;

	// target direction
	MatrixGetColumn( targetToWorld, 0, plane.AsVector3D() );
	VectorNormalize( plane.AsVector3D() );
	VectorNegate( plane.AsVector3D() );

	plane.w =
		MatrixColumnDotProduct( targetToWorld, 3, plane.AsVector3D() ) // target clip plane distance
		- flLocalPlaneDist // portal plane distance on the brush. This distance needs to be accounted for while placing the exit target
		- 0.1;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PushCustomClipPlane( plane.Base() );

	ViewDrawScene( bDrew3dSkybox, nSkyMode, monitorView, nClearFlags, VIEW_MONITOR );

	pRenderContext->PopCustomClipPlane();
 	render->PopView( frustum );

	// Reset the world fog parameters.
	if ( fogEnabled )
	{
		if ( pFogParams )
		{
			*pFogParams = oldFogParams;
		}
		monitorView.zFar = flOldZFar;
	}
	return true;
}

void CViewRender::DrawMonitors( const CViewSetupEx &cameraView )
{
#ifdef PORTAL
	g_pPortalRender->DrawPortalsToTextures( this, cameraView );
#endif

	// Early out if no cameras
	C_PointCamera *pCameraEnt = GetPointCameraList();
	if ( !pCameraEnt )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

#ifdef _DEBUG
	g_bRenderingCameraView = true;
#endif

	// FIXME: this should check for the ability to do a render target maybe instead.
	// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
	ITexture *pCameraTarget = GetCameraTexture();
	int width = pCameraTarget->GetActualWidth();
	int height = pCameraTarget->GetActualHeight();

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	
	int cameraNum;
	for ( cameraNum = 0; pCameraEnt != NULL; pCameraEnt = pCameraEnt->m_pNext )
	{
		if ( !pCameraEnt->IsActive() || pCameraEnt->IsDormant() )
			continue;

		// Check if the camera has its own render target
		// (Multiple render target support)
		if ( pCameraTarget != pCameraEnt->RenderTarget() )
		{
			pCameraTarget = pCameraEnt->RenderTarget();
			width = pCameraTarget->GetActualWidth();
			height = pCameraTarget->GetActualHeight();
		}

		if ( !DrawOneMonitor( pCameraTarget, cameraNum, pCameraEnt, cameraView, player, 0, 0, width, height ) )
			continue;

		++cameraNum;
	}

#ifdef _DEBUG
	g_bRenderingCameraView = false;
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

ClientWorldListInfo_t *ClientWorldListInfo_t::AllocPooled( const ClientWorldListInfo_t &exemplar )
{
	size_t nBytes = AlignValue( ( exemplar.m_LeafCount * ((sizeof(LeafIndex_t) * 2) + sizeof(LeafFogVolume_t)) ), 4096 );

	ClientWorldListInfo_t *pResult = gm_Pool.GetObject();

	byte *pMemory = (byte *)pResult->m_pLeafList;

	if ( pMemory )
	{
		// Previously allocated, add a reference. Otherwise comes out of GetObject as a new object with a refcount of 1
		pResult->AddRef();
	}

	if ( !pMemory || _msize( pMemory ) < nBytes )
	{
		pMemory = (byte *)realloc( pMemory, nBytes );
	}

	pResult->m_pLeafList = (LeafIndex_t*)pMemory;
	pResult->m_pLeafFogVolume = (LeafFogVolume_t*)( pMemory + exemplar.m_LeafCount * sizeof(LeafIndex_t) );
	pResult->m_pActualLeafIndex = (LeafIndex_t*)( (byte *)( pResult->m_pLeafFogVolume ) + exemplar.m_LeafCount * sizeof(LeafFogVolume_t) );

	pResult->m_bPooledAlloc = true;

	return pResult;
}

bool ClientWorldListInfo_t::OnFinalRelease()
{
	if ( m_bPooledAlloc )
	{
		Assert( m_pLeafList );
		gm_Pool.PutObject( this );
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBase3dView::CBase3dView( CViewRender *pMainView ) :
m_pMainView( pMainView ),
m_Frustum( pMainView->m_Frustum )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnt - 
// Output : int
//-----------------------------------------------------------------------------
VPlane* CBase3dView::GetFrustum()
{
	// The frustum is only valid while in a RenderView call.
	// @MULTICORE (toml 8/11/2006): reimplement this when ready -- Assert(g_bRenderingView || g_bRenderingCameraView || g_bRenderingScreenshot);	
	return m_Frustum;
}


CObjectPool<ClientWorldListInfo_t> ClientWorldListInfo_t::gm_Pool;


//-----------------------------------------------------------------------------
// Base class for 3d views
//-----------------------------------------------------------------------------
CRendering3dView::CRendering3dView(CViewRender *pMainView) :
	CBase3dView( pMainView ),
	m_pWorldRenderList( NULL ), 
	m_pRenderablesList( NULL ), 
	m_pWorldListInfo( NULL ), 
	m_pCustomVisibility( NULL ),
	m_DrawFlags( 0 ),
	m_ClearFlags( 0 )
{
}


//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void CRendering3dView::Setup( const CViewSetupEx &setup )
{
	// @MULTICORE (toml 8/15/2006): don't reset if parameters don't require it. For now, just reset
	memcpy( (void *)static_cast<CViewSetupEx *>(this), (void *)&setup, sizeof( setup ) );
	ReleaseLists();

	m_pRenderablesList = new CClientRenderablesList; 
	m_pCustomVisibility = NULL;
}


//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void CRendering3dView::ReleaseLists()
{
	SafeRelease( m_pWorldRenderList );
	SafeRelease( m_pRenderablesList );
	SafeRelease( m_pWorldListInfo );
	m_pCustomVisibility = NULL;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::SetupRenderablesList( int viewID )
{
	VPROF( "CViewRender::SetupRenderablesList" );

	// Clear the list.
	int i;
	for( i=0; i < CClientRenderablesList::GROUP_COUNT; i++ )
	{
		m_pRenderablesList->m_RenderGroupCounts[i] = 0;
	}

	// Now collate the entities in the leaves.
	if( !m_pMainView->ShouldDrawEntities() )
	{
		return;
	}

	m_pMainView->IncRenderablesListsNumber();

	// Precache information used commonly in CollateRenderables
	SetupRenderInfo_t setupInfo;
	setupInfo.m_pWorldListInfo = m_pWorldListInfo;
	setupInfo.m_nRenderFrame = m_pMainView->BuildRenderablesListsNumber();	// only one incremented?
	setupInfo.m_nDetailBuildFrame = m_pMainView->BuildWorldListsNumber();	//
	setupInfo.m_pRenderList = m_pRenderablesList;
	setupInfo.m_bDrawDetailObjects = GetClientMode()->ShouldDrawDetailObjects() && r_DrawDetailProps.GetInt();
	setupInfo.m_bDrawTranslucentObjects = ( r_flashlightdepth_drawtranslucents.GetBool() || (viewID != VIEW_SHADOW_DEPTH_TEXTURE) || m_bRenderFlashlightDepthTranslucents );
	setupInfo.m_nViewID = viewID;
	setupInfo.m_vecRenderOrigin = origin;
	setupInfo.m_vecRenderForward = CurrentViewForward();

	float fMaxDist = cl_maxrenderable_dist.GetFloat();

	// Shadowing light typically has a smaller farz than cl_maxrenderable_dist
	setupInfo.m_flRenderDistSq = (viewID == VIEW_SHADOW_DEPTH_TEXTURE) ? MIN(zFar, fMaxDist) : fMaxDist;
	setupInfo.m_flRenderDistSq *= setupInfo.m_flRenderDistSq;

	ClientLeafSystem()->BuildRenderablesList( setupInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Builds lists of things to render in the world, called once per view
//-----------------------------------------------------------------------------
void CRendering3dView::UpdateRenderablesOpacity()
{
	// Compute the prop opacity based on the view position and fov zoom scale
	float flFactor = 1.0f;
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( pLocal )
	{
		flFactor = pLocal->GetFOVDistanceAdjustFactor();
	}

	if ( cl_leveloverview.GetFloat() > 0 )
	{
		// disable prop fading
		flFactor = -1;
	}

	// When zoomed in, tweak the opacity to stay visible from further away
	staticpropmgr->ComputePropOpacity( origin, flFactor );

	// Build a list of detail props to render
	DetailObjectSystem()->BuildDetailObjectRenderLists( origin );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// Kinda awkward...three optional parameters at the end...
void CRendering3dView::BuildWorldRenderLists( bool bDrawEntities, int iForceViewLeaf /* = -1 */, 
	bool bUseCacheIfEnabled /* = true */, bool bShadowDepth /* = false */, float *pReflectionWaterHeight /*= NULL*/ )
{
	VPROF_BUDGET( "BuildWorldRenderLists", VPROF_BUDGETGROUP_WORLD_RENDERING );

    // @MULTICORE (toml 8/18/2006): to address....
	extern void UpdateClientRenderableInPVSStatus();
	UpdateClientRenderableInPVSStatus();

	Assert( !m_pWorldRenderList && !m_pWorldListInfo);

	m_pMainView->IncWorldListsNumber();
	// Override vis data if specified this render, otherwise use default behavior with NULL
	VisOverrideData_t* pVisData = ( m_pCustomVisibility && m_pCustomVisibility->m_VisData.m_fDistToAreaPortalTolerance != FLT_MAX ) ?  &m_pCustomVisibility->m_VisData : NULL;
	bool bUseCache = ( bUseCacheIfEnabled && r_worldlistcache.GetBool() );
	if ( !bUseCache || pVisData || !g_WorldListCache.Find( *this, &m_pWorldRenderList, &m_pWorldListInfo ) )
	{
        // @MULTICORE (toml 8/18/2006): when make parallel, will have to change caching to be atomic, where follow ons receive a pointer to a list that is not yet built
		m_pWorldRenderList =  render->CreateWorldList();
		m_pWorldListInfo = new ClientWorldListInfo_t;

		render->BuildWorldLists( m_pWorldRenderList, m_pWorldListInfo, 
			( m_pCustomVisibility ) ? m_pCustomVisibility->m_iForceViewLeaf : iForceViewLeaf, 
			pVisData, bShadowDepth, pReflectionWaterHeight );

		if ( bUseCache && !pVisData )
		{
			g_WorldListCache.Add( *this, m_pWorldRenderList, m_pWorldListInfo );
		}
	}

	if ( bDrawEntities )
	{
		UpdateRenderablesOpacity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Computes the actual world list info based on the render flags
//-----------------------------------------------------------------------------
void CRendering3dView::PruneWorldListInfo()
{
	// Drawing everything? Just return the world list info as-is 
	int nWaterDrawFlags = m_DrawFlags & (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER);

	if ( nWaterDrawFlags == DF_RENDER_ABOVEWATER && !m_pWorldListInfo->m_bHasWater )
		return;

	if ( nWaterDrawFlags == (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER) )
	{
		return;
	}

	ClientWorldListInfo_t *pNewInfo;
	// Only allocate memory if actually will draw something
	if ( m_pWorldListInfo->m_LeafCount > 0 && nWaterDrawFlags )
	{
		pNewInfo = ClientWorldListInfo_t::AllocPooled( *m_pWorldListInfo );
	}
	else
	{
		pNewInfo = new ClientWorldListInfo_t;
	}

	pNewInfo->m_ViewFogVolume = m_pWorldListInfo->m_ViewFogVolume;
	pNewInfo->m_bHasWater = m_pWorldListInfo->m_bHasWater;
	pNewInfo->m_LeafCount = 0;

	if ( nWaterDrawFlags != DF_RENDER_UNDERWATER || m_pWorldListInfo->m_bHasWater )
	{
		// Not drawing anything? Then don't bother with renderable lists
		if ( nWaterDrawFlags != 0 )
		{
			// Create a sub-list based on the actual leaves being rendered
			bool bRenderingUnderwater = (nWaterDrawFlags & DF_RENDER_UNDERWATER) != 0;
			for ( int i = 0; i < m_pWorldListInfo->m_LeafCount; ++i )
			{
				bool bLeafIsUnderwater = ( m_pWorldListInfo->m_pLeafFogVolume[i] != -1 );
				if ( bRenderingUnderwater == bLeafIsUnderwater )
				{
					pNewInfo->m_pLeafList[ pNewInfo->m_LeafCount ] = m_pWorldListInfo->m_pLeafList[ i ];
					pNewInfo->m_pLeafFogVolume[ pNewInfo->m_LeafCount ] = m_pWorldListInfo->m_pLeafFogVolume[ i ];
					pNewInfo->m_pActualLeafIndex[ pNewInfo->m_LeafCount ] = i;
					++pNewInfo->m_LeafCount;
				}
			}
		}
	}

	m_pWorldListInfo->Release();
	m_pWorldListInfo = pNewInfo;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static inline void UpdateBrushModelLightmap( IClientRenderable *pEnt )
{
	model_t *pModel = ( model_t * )pEnt->GetModel();
	render->UpdateBrushModelLightmap( pModel, pEnt );
}


void CRendering3dView::BuildRenderableRenderLists( int viewID )
{
	MDLCACHE_CRITICAL_SECTION();

	if ( viewID != VIEW_SHADOW_DEPTH_TEXTURE )
	{
		render->BeginUpdateLightmaps();
	}

	m_pMainView->IncRenderablesListsNumber();

	ClientWorldListInfo_t& info = *m_pWorldListInfo;

#if 0
	// For better sorting, find out the leaf *nearest* to the camera
	// and render translucent objects as if they are in that leaf.
	if( m_pMainView->ShouldDrawEntities() && ( viewID != VIEW_SHADOW_DEPTH_TEXTURE ) )
	{
		ClientLeafSystem()->ComputeTranslucentRenderLeaf( 
			info.m_LeafCount, info.m_pLeafList, info.m_pLeafFogVolume, m_pMainView->BuildRenderablesListsNumber(), viewID );
	}
#endif

	SetupRenderablesList( viewID );

	if ( viewID == VIEW_MAIN )
	{
		StudioStats_FindClosestEntity( m_pRenderablesList );
	}

	if ( viewID != VIEW_SHADOW_DEPTH_TEXTURE )
	{
		// update lightmap on brush models if necessary
		CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_OPAQUE_BRUSH];
		int nOpaque = m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_OPAQUE_BRUSH];
		int i;
		for( i=0; i < nOpaque; ++i )
		{
			Assert(pEntities[i].m_TwoPass==0);
			UpdateBrushModelLightmap( pEntities[i].m_pRenderable );
		}

		// update lightmap on brush models if necessary
		pEntities = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
		int nTranslucent = m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
		for( i=0; i < nTranslucent; ++i )
		{
			if ( pEntities[i].m_nModelType != RENDERABLE_MODEL_BRUSH )
				continue;
			Assert(pEntities[i].m_TwoPass==0);
			UpdateBrushModelLightmap( pEntities[i].m_pRenderable );
		}

		render->EndUpdateLightmaps();
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::DrawWorld( float waterZAdjust )
{
	VPROF_INCREMENT_COUNTER( "RenderWorld", 1 );
	VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_WORLD_RENDERING );
	if( !r_drawopaqueworld.GetBool() )
	{
		return;
	}

	unsigned long engineFlags = BuildEngineDrawWorldListFlags( m_DrawFlags );

	render->DrawWorldLists( m_pWorldRenderList, engineFlags, waterZAdjust );
}


CMaterialReference g_material_WriteZ; //init'ed on by CViewRender::Init()

//-----------------------------------------------------------------------------
// Fakes per-entity clip planes on cards that don't support user clip planes.
//  Achieves the effect by drawing an invisible box that writes to the depth buffer
//  around the clipped area. It's not perfect, but better than nothing.
//-----------------------------------------------------------------------------
static void DrawClippedDepthBox( IClientRenderable *pEnt, float *pClipPlane )
{
//#define DEBUG_DRAWCLIPPEDDEPTHBOX //uncomment to draw the depth box as a colorful box

	static const int iQuads[6][5] = {	{ 0, 4, 6, 2, 0 }, //always an extra copy of first index at end to make some algorithms simpler
										{ 3, 7, 5, 1, 3 },
										{ 1, 5, 4, 0, 1 },
										{ 2, 6, 7, 3, 2 },
										{ 0, 2, 3, 1, 0 },
										{ 5, 7, 6, 4, 5 } };

	static const int iLines[12][2] = {	{ 0, 1 },
										{ 0, 2 },
										{ 0, 4 },
										{ 1, 3 },
										{ 1, 5 },
										{ 2, 3 },
										{ 2, 6 },
										{ 3, 7 },
										{ 4, 6 },
										{ 4, 5 },
										{ 5, 7 },
										{ 6, 7 } };


#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
	static const float fColors[6][3] = {	{ 1.0f, 0.0f, 0.0f },
											{ 0.0f, 1.0f, 1.0f },
											{ 0.0f, 1.0f, 0.0f },
											{ 1.0f, 0.0f, 1.0f },
											{ 0.0f, 0.0f, 1.0f },
											{ 1.0f, 1.0f, 0.0f } };
#endif

	
	

	Vector vNormal = *(Vector *)pClipPlane;
	float fPlaneDist = pClipPlane[3];

	Vector vMins, vMaxs;
	pEnt->GetRenderBounds( vMins, vMaxs );

	Vector vOrigin = pEnt->GetRenderOrigin();
	QAngle qAngles = pEnt->GetRenderAngles();
	
	Vector vForward, vUp, vRight;
	AngleVectors( qAngles, &vForward, &vRight, &vUp );

	Vector vPoints[8];
	vPoints[0] = vOrigin + (vForward * vMins.x) + (vRight * vMins.y) + (vUp * vMins.z);
	vPoints[1] = vOrigin + (vForward * vMaxs.x) + (vRight * vMins.y) + (vUp * vMins.z);
	vPoints[2] = vOrigin + (vForward * vMins.x) + (vRight * vMaxs.y) + (vUp * vMins.z);
	vPoints[3] = vOrigin + (vForward * vMaxs.x) + (vRight * vMaxs.y) + (vUp * vMins.z);
	vPoints[4] = vOrigin + (vForward * vMins.x) + (vRight * vMins.y) + (vUp * vMaxs.z);
	vPoints[5] = vOrigin + (vForward * vMaxs.x) + (vRight * vMins.y) + (vUp * vMaxs.z);
	vPoints[6] = vOrigin + (vForward * vMins.x) + (vRight * vMaxs.y) + (vUp * vMaxs.z);
	vPoints[7] = vOrigin + (vForward * vMaxs.x) + (vRight * vMaxs.y) + (vUp * vMaxs.z);

	int iClipped[8];
	float fDists[8];
	for( int i = 0; i != 8; ++i )
	{
		fDists[i] = vPoints[i].Dot( vNormal ) - fPlaneDist;
		iClipped[i] = (fDists[i] > 0.0f) ? 1 : 0;
	}

	Vector vSplitPoints[8][8]; //obviously there are only 12 lines, not 64 lines or 64 split points, but the indexing is way easier like this
	int iLineStates[8][8]; //0 = unclipped, 2 = wholly clipped, 3 = first point clipped, 4 = second point clipped

	//categorize lines and generate split points where needed
	for( int i = 0; i != 12; ++i )
	{
		const int *pPoints = iLines[i];
		int iLineState = (iClipped[pPoints[0]] + iClipped[pPoints[1]]);
		if( iLineState != 1 ) //either both points are clipped, or neither are clipped
		{
			iLineStates[pPoints[0]][pPoints[1]] = 
				iLineStates[pPoints[1]][pPoints[0]] = 
					iLineState;
		}
		else
		{
			//one point is clipped, the other is not
			if( iClipped[pPoints[0]] == 1 )
			{
				//first point was clipped, index 1 has the negative distance
				float fInvTotalDist = 1.0f / (fDists[pPoints[0]] - fDists[pPoints[1]]);
				vSplitPoints[pPoints[0]][pPoints[1]] = 
					vSplitPoints[pPoints[1]][pPoints[0]] =
						(vPoints[pPoints[1]] * (fDists[pPoints[0]] * fInvTotalDist)) - (vPoints[pPoints[0]] * (fDists[pPoints[1]] * fInvTotalDist));
				
				Assert( fabs( vNormal.Dot( vSplitPoints[pPoints[0]][pPoints[1]] ) - fPlaneDist ) < 0.01f );

				iLineStates[pPoints[0]][pPoints[1]] = 3;
				iLineStates[pPoints[1]][pPoints[0]] = 4;
			}
			else
			{
				//second point was clipped, index 0 has the negative distance
				float fInvTotalDist = 1.0f / (fDists[pPoints[1]] - fDists[pPoints[0]]);
				vSplitPoints[pPoints[0]][pPoints[1]] = 
					vSplitPoints[pPoints[1]][pPoints[0]] =
						(vPoints[pPoints[0]] * (fDists[pPoints[1]] * fInvTotalDist)) - (vPoints[pPoints[1]] * (fDists[pPoints[0]] * fInvTotalDist));

				Assert( fabs( vNormal.Dot( vSplitPoints[pPoints[0]][pPoints[1]] ) - fPlaneDist ) < 0.01f );

				iLineStates[pPoints[0]][pPoints[1]] = 4;
				iLineStates[pPoints[1]][pPoints[0]] = 3;
			}
		}
	}


	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
	pRenderContext->Bind( g_pMaterialSystem->FindMaterial( "debug/debugvertexcolor", TEXTURE_GROUP_OTHER ), NULL );
#else
	pRenderContext->Bind( g_material_WriteZ, NULL );
#endif

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( false );
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 18 ); //6 sides, possible one cut per side. Any side is capable of having 3 tri's. Lots of padding for things that aren't possible

	//going to draw as a collection of triangles, arranged as a triangle fan on each side
	for( int i = 0; i != 6; ++i )
	{
		const int *pPoints = iQuads[i];

		//can't start the fan on a wholly clipped line, so seek to one that isn't
		int j = 0;
		do
		{
			if( iLineStates[pPoints[j]][pPoints[j+1]] != 2 ) //at least part of this line will be drawn
				break;

			++j;
		} while( j != 3 );

		if( j == 3 ) //not enough lines to even form a triangle
			continue;

		float *pStartPoint = 0;
		float *pTriangleFanPoints[4]; //at most, one of our fans will have 5 points total, with the first point being stored separately as pStartPoint
		int iTriangleFanPointCount = 1; //the switch below creates the first for sure
		
		//figure out how to start the fan
		switch( iLineStates[pPoints[j]][pPoints[j+1]] )
		{
		case 0: //uncut
			pStartPoint = &vPoints[pPoints[j]].x;
			pTriangleFanPoints[0] = &vPoints[pPoints[j+1]].x;
			break;

		case 4: //second index was clipped
			pStartPoint = &vPoints[pPoints[j]].x;
			pTriangleFanPoints[0] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			break;

		case 3: //first index was clipped
			pStartPoint = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			pTriangleFanPoints[0] = &vPoints[pPoints[j + 1]].x;
			break;

		default:
			Assert( false );
			break;
		};

		for( ++j; j != 3; ++j ) //add end points for the rest of the indices, we're assembling a triangle fan
		{
			switch( iLineStates[pPoints[j]][pPoints[j+1]] )
			{
			case 0: //uncut line, normal endpoint
				pTriangleFanPoints[iTriangleFanPointCount] = &vPoints[pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			case 2: //wholly cut line, no endpoint
				break;

			case 3: //first point is clipped, normal endpoint
				//special case, adds start and end point
				pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
				++iTriangleFanPointCount;

				pTriangleFanPoints[iTriangleFanPointCount] = &vPoints[pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			case 4: //second point is clipped
				pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
				++iTriangleFanPointCount;
				break;

			default:
				Assert( false );
				break;
			};
		}
		
		//special case endpoints, half-clipped lines have a connecting line between them and the next line (first line in this case)
		switch( iLineStates[pPoints[j]][pPoints[j+1]] )
		{
		case 3:
		case 4:
			pTriangleFanPoints[iTriangleFanPointCount] = &vSplitPoints[pPoints[j]][pPoints[j+1]].x;
			++iTriangleFanPointCount;
			break;
		};

		Assert( iTriangleFanPointCount <= 4 );

		//add the fan to the mesh
		int iLoopStop = iTriangleFanPointCount - 1;
		for( int k = 0; k != iLoopStop; ++k )
		{
			meshBuilder.Position3fv( pStartPoint );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			float fHalfColors[3] = { fColors[i][0] * 0.5f, fColors[i][1] * 0.5f, fColors[i][2] * 0.5f };
			meshBuilder.Color3fv( fHalfColors );
#endif
			meshBuilder.AdvanceVertex();
			
			meshBuilder.Position3fv( pTriangleFanPoints[k] );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			meshBuilder.Color3fv( fColors[i] );
#endif
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pTriangleFanPoints[k+1] );
#ifdef DEBUG_DRAWCLIPPEDDEPTHBOX
			meshBuilder.Color3fv( fColors[i] );
#endif
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
	pRenderContext->Flush( false );
}

static inline bool BlurTest( IClientRenderable *pRenderable, IClientRenderableMod *pRenderableMod, int drawFlags, bool bPreDraw, const RenderableInstance_t &instance )
{
	if( CurrentViewID() == VIEW_MONITOR )
		return false;

	if( !bPreDraw )
		return false;

	IClientUnknownMod *pUnknownMod = pRenderableMod ? pRenderableMod->GetIClientUnknownMod() : NULL;
	if( !pUnknownMod )
		return false;

	IClientEntityMod *pClientEntityMod = pUnknownMod->GetIClientEntityMod();
	if( !pClientEntityMod )
		return false;

	if( pClientEntityMod->IsBlurred() )
	{
		const CViewSetupEx *pViewSetup = GetViewRenderInstance()->GetViewSetup();
		if( pViewSetup )
		{
			BlurEntity( pRenderable, pRenderableMod, bPreDraw, drawFlags, instance, *pViewSetup, pViewSetup->x, pViewSetup->y, pViewSetup->width, pViewSetup->height );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Unified bit of draw code for opaque and translucent renderables
//-----------------------------------------------------------------------------
static inline void DrawRenderable( IClientRenderable *pEnt, IClientRenderableMod *pEntMod, int flags, const RenderableInstance_t &instance )
{
	float *pRenderClipPlane = NULL;
	if( r_entityclips.GetBool() )
		pRenderClipPlane = pEnt->GetRenderClipPlane();

	if( pRenderClipPlane )	
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		if( !g_pMaterialSystem->UsingFastClipping() ) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
			pRenderContext->PushCustomClipPlane( pRenderClipPlane );
		else
			DrawClippedDepthBox( pEnt, pRenderClipPlane );
		Assert( GetViewRenderInstance()->GetCurrentlyDrawingEntity() == NULL );
		GetViewRenderInstance()->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		bool bBlockNormalDraw = BlurTest( pEnt, pEntMod, flags, true, instance );
		if( !bBlockNormalDraw ) {
			if(pEntMod)
				pEntMod->DrawModel( flags, instance );
			else
				pEnt->DO_NOT_USE_DrawModel( flags );
		}
		BlurTest( pEnt, pEntMod, flags, false, instance );
		GetViewRenderInstance()->SetCurrentlyDrawingEntity( NULL );

		if( !g_pMaterialSystem->UsingFastClipping() )	
			pRenderContext->PopCustomClipPlane();
	}
	else
	{
		Assert( GetViewRenderInstance()->GetCurrentlyDrawingEntity() == NULL );
		GetViewRenderInstance()->SetCurrentlyDrawingEntity( pEnt->GetIClientUnknown()->GetBaseEntity() );
		bool bBlockNormalDraw = BlurTest( pEnt, pEntMod, flags, true, instance );
		if( !bBlockNormalDraw ) {
			if(pEntMod)
				pEntMod->DrawModel( flags, instance );
			else
				pEnt->DO_NOT_USE_DrawModel( flags );
		}
		BlurTest( pEnt, pEntMod, flags, false, instance );
		GetViewRenderInstance()->SetCurrentlyDrawingEntity( NULL );
	}
}

//-----------------------------------------------------------------------------
// Draws all opaque renderables in leaves that were rendered
//-----------------------------------------------------------------------------
static inline void DrawOpaqueRenderable( IClientRenderable *pEnt, IClientRenderableMod *pEntMod, bool bTwoPass, ERenderDepthMode DepthMode, int nDefaultFlags = 0 )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	float color[3];

	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );

	int flags = nDefaultFlags | STUDIO_RENDER;
	if ( bTwoPass )
	{
		flags |= STUDIO_TWOPASS;
	}

	if ( DepthMode == DEPTH_MODE_SHADOW )
	{
		flags |= STUDIO_SHADOWDEPTHTEXTURE;
	}
	else if ( DepthMode == DEPTH_MODE_SSA0 )
	{
		flags |= STUDIO_SSAODEPTHTEXTURE;
	}

	RenderableInstance_t instance;
	instance.m_nAlpha = 255;
	DrawRenderable( pEnt, pEntMod, flags, instance );
}

//-------------------------------------


ConVar r_drawopaquestaticpropslast( "r_drawopaquestaticpropslast", "0", 0, "Whether opaque static props are rendered after non-npcs" );

#ifdef STAGING_ONLY
#define DEBUG_BUCKETS 0
#else
#define DEBUG_BUCKETS 0
#endif

#if DEBUG_BUCKETS
ConVar r_drawopaquesbucket( "r_drawopaquesbucket", "0", FCVAR_CHEAT, "Draw only specific bucket: positive - props, negative - ents" );
ConVar r_drawopaquesbucket_stats( "r_drawopaquesbucket_stats", "1", FCVAR_CHEAT, "Draw distribution of props/ents in the buckets" );
#endif

static void SetupBonesOnBaseAnimating( C_BaseAnimating *&pBaseAnimating )
{
	pBaseAnimating->SetupBones( NULL, -1, -1, gpGlobals->curtime );
}


static void DrawOpaqueRenderables_DrawBrushModels( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		Assert( !itEntity->m_TwoPass );
		DrawOpaqueRenderable( itEntity->m_pRenderable, itEntity->m_pRenderableMod, false, DepthMode );
	}
}

static void DrawOpaqueRenderables_DrawStaticProps( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	if ( pEntitiesEnd == pEntitiesBegin )
		return;

	float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	render->SetColorModulation(	one );
	render->SetBlend( 1.0f );
	
	const int MAX_STATICS_PER_BATCH = 512;
	IClientRenderable *pStatics[ MAX_STATICS_PER_BATCH ];
	
	int numScheduled = 0, numAvailable = MAX_STATICS_PER_BATCH;

	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		if ( itEntity->m_pRenderable )
			/**/;
		else
			continue;

		if ( g_pStudioStatsEntity != NULL && g_CurrentViewID == VIEW_MAIN && itEntity->m_pRenderable == g_pStudioStatsEntity )
		{
			DrawOpaqueRenderable( itEntity->m_pRenderable, itEntity->m_pRenderableMod, false, DepthMode, STUDIO_GENERATE_STATS );
			continue;
		}

		pStatics[ numScheduled ++ ] = itEntity->m_pRenderable;
		if ( -- numAvailable > 0 )
			continue; // place a hint for compiler to predict more common case in the loop
		
		staticpropmgr->DrawStaticProps( pStatics, numScheduled, DepthMode != DEPTH_MODE_NORMAL, vcollide_wireframe.GetBool() );
		numScheduled = 0;
		numAvailable = MAX_STATICS_PER_BATCH;
	}
	
	if ( numScheduled )
		staticpropmgr->DrawStaticProps( pStatics, numScheduled, DepthMode != DEPTH_MODE_NORMAL, vcollide_wireframe.GetBool() );
}

static void DrawOpaqueRenderables_Range( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	for( CClientRenderablesList::CEntry *itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
	{
		if ( itEntity->m_pRenderable )
			DrawOpaqueRenderable( itEntity->m_pRenderable, itEntity->m_pRenderableMod, ( itEntity->m_TwoPass != 0 ), DepthMode );
	}
}

ConVar cl_modelfastpath( "cl_modelfastpath", "1" );
ConVar cl_skipslowpath( "cl_skipslowpath", "0", FCVAR_CHEAT, "Set to 1 to skip any models that don't go through the model fast path" );
extern ConVar r_drawothermodels;
static void	DrawOpaqueRenderables_ModelRenderables( int nCount, ModelRenderSystemData_t* pModelRenderables, ERenderDepthMode DepthMode )
{
	ModelRenderMode_t renderMode = MODEL_RENDER_MODE_NORMAL;

	switch (DepthMode) {
	case DEPTH_MODE_SHADOW:
		renderMode = MODEL_RENDER_MODE_SHADOW_DEPTH;
	case DEPTH_MODE_NORMAL:
		renderMode = MODEL_RENDER_MODE_NORMAL;
	}

	g_pModelRenderSystem->DrawModels( pModelRenderables, nCount, renderMode );
}

static void	DrawOpaqueRenderables_NPCs( CClientRenderablesList::CEntry *pEntitiesBegin, CClientRenderablesList::CEntry *pEntitiesEnd, ERenderDepthMode DepthMode )
{
	DrawOpaqueRenderables_Range( pEntitiesBegin, pEntitiesEnd, DepthMode );
}

void CRendering3dView::DrawOpaqueRenderables( ERenderDepthMode DepthMode )
{
	VPROF_BUDGET("CViewRender::DrawOpaqueRenderables", "DrawOpaqueRenderables" );

	if( !r_drawopaquerenderables.GetBool() )
		return;

	if( !m_pMainView->ShouldDrawEntities() )
		return;

	render->SetBlend( 1 );

	//
	// Prepare to iterate over all leaves that were visible, and draw opaque things in them.	
	//
	RopeManager()->ResetRenderCache();
	g_pParticleSystemMgr->ResetRenderCache();

	bool const bDrawopaquestaticpropslast = r_drawopaquestaticpropslast.GetBool();

	//
	// First do the brush models
	//
	{
		CClientRenderablesList::CEntry *pEntitiesBegin, *pEntitiesEnd;
		pEntitiesBegin = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_OPAQUE_BRUSH];
		pEntitiesEnd = pEntitiesBegin + m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_OPAQUE_BRUSH];
		DrawOpaqueRenderables_DrawBrushModels( pEntitiesBegin, pEntitiesEnd, DepthMode );
	}

	bool bUseFastPath = ( 
	#if 0
		cl_modelfastpath.GetInt() != 0
	#else
		false
	#endif
	);

	//
	// Sort everything that's not a static prop
	//
	int numOpaqueEnts = 0;
	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
		numOpaqueEnts += m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];

	int nStaticPropCount = 0;
	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
		nStaticPropCount += m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];

	CUtlVector< C_BaseAnimating * > arrBoneSetupNpcsLast( (C_BaseAnimating **)_alloca( numOpaqueEnts * sizeof( C_BaseAnimating * ) ), numOpaqueEnts, numOpaqueEnts );
	CUtlVector< CClientRenderablesList::CEntry > arrRenderEntsNpcsFirst( (CClientRenderablesList::CEntry *)_alloca( numOpaqueEnts * sizeof( CClientRenderablesList::CEntry ) ), numOpaqueEnts, numOpaqueEnts );
	CUtlVector< ModelRenderSystemData_t > arrModelRenderables( (ModelRenderSystemData_t *)_alloca( ( numOpaqueEnts + nStaticPropCount ) * sizeof( ModelRenderSystemData_t ) ), (numOpaqueEnts + nStaticPropCount), (numOpaqueEnts + nStaticPropCount) );
	int numFastModel = 0, numNpcs = 0, numNpcsRender = 0, numNonNpcsAnimating = 0;

#if DEBUG_BUCKETS
	int debugNumSlowOpaqueEnts[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ];
	int debugSlowStaticPropCount[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ];
#endif

	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
	{
		int nBucketOpaqueEnts = m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];

	#if DEBUG_BUCKETS
		debugNumSlowOpaqueEnts[ bucket ] = nBucketOpaqueEnts;
	#endif

		for( CClientRenderablesList::CEntry
			* const pEntitiesBegin = m_pRenderablesList->m_RenderGroups[ CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ],
			* const pEntitiesEnd = pEntitiesBegin + nBucketOpaqueEnts,
			*itEntity = pEntitiesBegin; itEntity < pEntitiesEnd; ++ itEntity )
		{
			IClientUnknown *pUnknown = itEntity->m_pRenderable ? itEntity->m_pRenderable->GetIClientUnknown() : NULL;
			IClientUnknownMod *pUnknownMod = itEntity->m_pRenderableMod ? itEntity->m_pRenderableMod->GetIClientUnknownMod() : NULL;
			IClientModelRenderable *pModelRenderable = pUnknownMod ? pUnknownMod->GetClientModelRenderable() : NULL;

			C_BaseEntity *pEntity = pUnknown ? pUnknown->GetBaseEntity() : NULL;
			if ( pEntity )
			{
				if ( pEntity->IsNPC() )
				{
					C_BaseAnimating *pba = assert_cast<C_BaseAnimating *>( pEntity );

					if( (!bUseFastPath || !pModelRenderable) && !cl_skipslowpath.GetBool() ) 
						arrRenderEntsNpcsFirst[ numNpcsRender ++ ] = *itEntity;

					numNpcs ++;
					arrBoneSetupNpcsLast[ numOpaqueEnts - numNpcs ] = pba;

					if( (!bUseFastPath || !pModelRenderable) || cl_skipslowpath.GetBool() )
					{
						itEntity->m_pRenderable = NULL;		// We will render NPCs separately
						itEntity->m_pRenderableMod = NULL;
						itEntity->m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;

					#if DEBUG_BUCKETS
						--debugNumSlowOpaqueEnts[ bucket ];
					#endif

						continue;
					}
				}
				else if ( pEntity->GetBaseAnimating() )
				{
					C_BaseAnimating *pba = assert_cast<C_BaseAnimating *>( pEntity );
					arrBoneSetupNpcsLast[ numNonNpcsAnimating ++ ] = pba;
					// fall through
				}
			}

			if ( bUseFastPath && pModelRenderable )
			{
				ModelRenderSystemData_t data;
				data.m_pRenderable = itEntity->m_pRenderable;
				data.m_pRenderableMod = itEntity->m_pRenderableMod;
				data.m_pModelRenderable = pModelRenderable;
				data.m_InstanceData = itEntity->m_InstanceData;

				arrModelRenderables[ numFastModel ++ ] = data;

				itEntity->m_pRenderable = NULL;
				itEntity->m_pRenderableMod = NULL;
				itEntity->m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;

			#if DEBUG_BUCKETS
				--debugNumSlowOpaqueEnts[ bucket ];
			#endif

				continue;
			}
			else if( cl_skipslowpath.GetBool() )
			{
				itEntity->m_pRenderable = NULL;
				itEntity->m_pRenderableMod = NULL;
				itEntity->m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;

			#if DEBUG_BUCKETS
				--debugNumSlowOpaqueEnts[ bucket ];
			#endif

				continue;
			}
		}

		int nBucketStaticPropCount = m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];

	#if DEBUG_BUCKETS
		debugSlowStaticPropCount[ bucket ] = nBucketStaticPropCount;
	#endif

		if( bUseFastPath || cl_skipslowpath.GetBool() )
		{
			for( CClientRenderablesList::CEntry
				* const pPropsBegin = m_pRenderablesList->m_RenderGroups[ CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ],
				* const pPropsEnd = pPropsBegin + nBucketStaticPropCount,
				*itProp = pPropsBegin; itProp < pPropsEnd; ++ itProp )
			{
				IClientUnknown *pUnknown = itProp->m_pRenderable ? itProp->m_pRenderable->GetIClientUnknown() : NULL;
				IClientUnknownMod *pUnknownMod = itProp->m_pRenderableMod ? itProp->m_pRenderableMod->GetIClientUnknownMod() : NULL;
				IClientModelRenderable *pModelRenderable = pUnknownMod ? pUnknownMod->GetClientModelRenderable() : NULL;

				if ( bUseFastPath && pModelRenderable )
				{
					ModelRenderSystemData_t data;
					data.m_pRenderable = itProp->m_pRenderable;
					data.m_pRenderableMod = itProp->m_pRenderableMod;
					data.m_pModelRenderable = pModelRenderable;
					data.m_InstanceData = itProp->m_InstanceData;

					arrModelRenderables[ numFastModel ++ ] = data;

					itProp->m_pRenderable = NULL;
					itProp->m_pRenderableMod = NULL;
					itProp->m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;

				#if DEBUG_BUCKETS
					--debugSlowStaticPropCount[ bucket ];
				#endif

					continue;
				}
				else if( cl_skipslowpath.GetBool() )
				{
					itProp->m_pRenderable = NULL;
					itProp->m_pRenderableMod = NULL;
					itProp->m_RenderHandle = INVALID_CLIENT_RENDER_HANDLE;

				#if DEBUG_BUCKETS
					--debugSlowStaticPropCount[ bucket ];
				#endif

					continue;
				}
			}
		}
	}

#if 0
	if ( r_threaded_renderables.GetBool() )
	{
		ParallelProcess( "BoneSetupNpcsLast", arrBoneSetupNpcsLast.Base() + numOpaqueEnts - numNpcs, numNpcs, &SetupBonesOnBaseAnimating );
		ParallelProcess( "BoneSetupNpcsLast NonNPCs", arrBoneSetupNpcsLast.Base(), numNonNpcsAnimating, &SetupBonesOnBaseAnimating );
	}
#endif

	//
	// Draw model renderables now (ie. models that use the fast path)
	//					 
	DrawOpaqueRenderables_ModelRenderables( numFastModel, arrModelRenderables.Base(), DepthMode );

	//
	// Draw static props + opaque entities from the biggest bucket to the smallest
	//
	CClientRenderablesList::CEntry * pEnts[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ][2];
	CClientRenderablesList::CEntry * pProps[ RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS ][2];

#if DEBUG_BUCKETS
	con_nprint_s nxPrn = { 0 };
	if ( r_drawopaquesbucket_stats.GetBool() )
	{
		nxPrn.time_to_live = -1;
		nxPrn.color[0] = 0.9f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 0.9f;
		nxPrn.fixed_width_font = true;
	}
#endif

	for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
	{
		pEnts[bucket][0] = m_pRenderablesList->m_RenderGroups[ CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];
		pEnts[bucket][1] = pEnts[bucket][0] + m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE + 2 * bucket ];
		
		pProps[bucket][0] = m_pRenderablesList->m_RenderGroups[ CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];
		pProps[bucket][1] = pProps[bucket][0] + m_pRenderablesList->m_RenderGroupCounts[ CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE + 2 * bucket ];

		// Render sequence debugging
	#if DEBUG_BUCKETS
		if ( r_drawopaquesbucket_stats.GetBool() )
		{
			nxPrn.index = 20 + bucket * 3;

			int nTotalEnts = (pEnts[bucket][1] - pEnts[bucket][0]);
			int nTotalProps = (pProps[bucket][1] - pProps[bucket][0]);

			auto printTotals = [&](bool ents_first) {
				if ( ents_first ) {
					if(debugNumSlowOpaqueEnts[ bucket ] != nTotalEnts) {
						engine->Con_NXPrintf( &nxPrn, "[%i] Ents: %3d, %3d", bucket + 1, debugNumSlowOpaqueEnts[ bucket ], nTotalEnts );
					} else {
						engine->Con_NXPrintf( &nxPrn, "[%i] Ents: %3d", bucket + 1, debugNumSlowOpaqueEnts[ bucket ] );
					}
				} else {
					if(debugSlowStaticPropCount[ bucket ] != nTotalProps) {
						engine->Con_NXPrintf( &nxPrn, "[%i] Props: %3d, %3d", bucket + 1, debugSlowStaticPropCount[ bucket ], nTotalProps );
					} else {
						engine->Con_NXPrintf( &nxPrn, "[%i] Props: %3d", bucket + 1, debugSlowStaticPropCount[ bucket ] );
					}
				}
			};
			
			printTotals( bDrawopaquestaticpropslast );
			++ nxPrn.index;
			printTotals( !bDrawopaquestaticpropslast );
			++ nxPrn.index;
		}
	#endif
	}

#if DEBUG_BUCKETS
	if ( r_drawopaquesbucket_stats.GetBool() )
	{
		++ nxPrn.index;
		engine->Con_NXPrintf( &nxPrn, "Npcs: %3d", numNpcsRender );
		++ nxPrn.index;
		engine->Con_NXPrintf( &nxPrn, "Fast Model: %3d", numFastModel );
	}
#endif

	if(!cl_skipslowpath.GetBool()) {
#if DEBUG_BUCKETS
		if ( r_drawopaquesbucket.GetInt() != 0 )
		{
			int iBucket = r_drawopaquesbucket.GetInt();

			if ( iBucket > 0 && iBucket <= RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS )
			{
				DrawOpaqueRenderables_Range( pEnts[iBucket - 1][0], pEnts[iBucket - 1][1], DepthMode );
			}
			if ( iBucket < 0 && iBucket >= -RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS )
			{
				DrawOpaqueRenderables_DrawStaticProps( pProps[- 1 - iBucket][0], pProps[- 1 - iBucket][1], DepthMode );
			}
		} else {
#endif
			for ( int bucket = 0; bucket < RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS; ++ bucket )
			{
				// PVS-Studio pointed out that the two sides of the if/else were identical. Fixing
				// this long-broken behavior would change rendering, so I fixed the code but
				// commented out the new behavior. Uncomment the if statement and else block
				// when needed.
				if ( bDrawopaquestaticpropslast )
				{
					DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], DepthMode );
					DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], DepthMode );
				}
				else
				{
					DrawOpaqueRenderables_DrawStaticProps( pProps[bucket][0], pProps[bucket][1], DepthMode );
					DrawOpaqueRenderables_Range( pEnts[bucket][0], pEnts[bucket][1], DepthMode );
				}
			}
	#if DEBUG_BUCKETS
		}
	#endif

		//
		// Draw NPCs now
		//
		DrawOpaqueRenderables_NPCs( arrRenderEntsNpcsFirst.Base(), arrRenderEntsNpcsFirst.Base() + numNpcsRender, DepthMode );
	}

	//
	// Ropes and particles
	//
	RopeManager()->DrawRenderCache( DepthMode != DEPTH_MODE_NORMAL );
	g_pParticleSystemMgr->DrawRenderCache( DepthMode != DEPTH_MODE_NORMAL );
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentWorldInLeaves( bool bShadowDepth )
{
	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;
	for( int iCurLeafIndex = info.m_LeafCount - 1; iCurLeafIndex >= 0; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, m_DrawFlags ) )
		{
			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, m_DrawFlags, bShadowDepth );
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentWorldAndDetailPropsInLeaves( int iCurLeafIndex, int iFinalLeafIndex, int nEngineDrawFlags, int &nDetailLeafCount, LeafIndex_t* pDetailLeafList, bool bShadowDepth )
{
	//TODO!!!! Arthurdead
	DistanceFadeInfo_t tmp;

	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldAndDetailPropsInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;
	for( ; iCurLeafIndex >= iFinalLeafIndex; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, nEngineDrawFlags ) )
		{
			if(r_DrawDetailProps.GetInt() != 0) {
				// First draw any queued-up detail props from previously visited leaves
				DetailObjectSystem()->RenderTranslucentDetailObjects( tmp, CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );
				nDetailLeafCount = 0;
			}

			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( m_pWorldRenderList, nActualLeafIndex, nEngineDrawFlags, bShadowDepth );
		}

		// Queue up detail props that existed in this leaf
		if ( ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( info.m_pLeafList[iCurLeafIndex], m_pMainView->BuildWorldListsNumber() ) )
		{
			pDetailLeafList[nDetailLeafCount] = info.m_pLeafList[iCurLeafIndex];
			++nDetailLeafCount;
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list
//-----------------------------------------------------------------------------
static inline void DrawTranslucentRenderable( IClientRenderable *pEnt, IClientRenderableMod *pEntMod, const RenderableInstance_t &instance, bool twoPass, bool bShadowDepth, bool bIgnoreDepth )
{
	IClientAlphaPropertyMod *pAlphaPropMod = pEntMod ? pEntMod->GetClientAlphaPropertyMod() : NULL;

	// Determine blending amount and tell engine
	float blend = (float)( instance.m_nAlpha / 255.0f );

	// Totally gone
	if ( blend <= 0.0f )
		return;

	if(pAlphaPropMod) {
		if ( pAlphaPropMod->IgnoresZBuffer() != bIgnoreDepth )
			return;
	} else {
		if(pEnt->DO_NOT_USE_IgnoresZBuffer() != bIgnoreDepth)
			return;
	}

	// Tell engine
	render->SetBlend( blend );

	float color[3];
	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );

	int flags = STUDIO_RENDER | STUDIO_TRANSPARENCY;
	if ( twoPass )
		flags |= STUDIO_TWOPASS;

	if ( bShadowDepth )
		flags |= STUDIO_SHADOWDEPTHTEXTURE;

	DrawRenderable( pEnt, pEntMod, flags, instance );
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentRenderablesNoWorld( bool bInSkybox )
{
	VPROF( "CViewRender::DrawTranslucentRenderablesNoWorld" );

	if ( !m_pMainView->ShouldDrawEntities() || !r_drawtranslucentrenderables.GetBool() )
		return;

	// Draw the particle singletons.
	DrawParticleSingletons( bInSkybox );

	bool bShadowDepth = (m_DrawFlags & ( DF_SHADOW_DEPTH_MAP | DF_SSAO_DEPTH_PASS ) ) != 0;

	CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
	int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY] - 1;

	while( iCurTranslucentEntity >= 0 )
	{
		IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
		IClientRenderableMod *pRenderableMod = pEntities[iCurTranslucentEntity].m_pRenderableMod;

		int nRenderFlags = pRenderableMod->GetRenderFlags();
		
		if ( nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
		{
			UpdateRefractTexture();
		}

		if ( nRenderFlags & ERENDERFLAGS_NEEDS_FULL_FB )
		{
			UpdateScreenEffectTexture();
		}

		DrawTranslucentRenderable( pRenderable, pRenderableMod, pEntities[iCurTranslucentEntity].m_InstanceData, pEntities[iCurTranslucentEntity].m_TwoPass != 0, bShadowDepth, false );
		--iCurTranslucentEntity;
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list that ignore the Z buffer
//-----------------------------------------------------------------------------
void CRendering3dView::DrawNoZBufferTranslucentRenderables( void )
{
	VPROF( "CViewRender::DrawNoZBufferTranslucentRenderables" );

	if ( !m_pMainView->ShouldDrawEntities() || !r_drawtranslucentrenderables.GetBool() )
		return;

	bool bShadowDepth = (m_DrawFlags & ( DF_SHADOW_DEPTH_MAP | DF_SSAO_DEPTH_PASS ) ) != 0;

	CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
	int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY] - 1;

	while( iCurTranslucentEntity >= 0 )
	{
		IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
		IClientRenderableMod *pRenderableMod = pEntities[iCurTranslucentEntity].m_pRenderableMod;

		int nRenderFlags = pRenderableMod ? pRenderableMod->GetRenderFlags() : 0;
		if(!pRenderableMod) {
			if(pRenderable->DO_NOT_USE_UsesFullFrameBufferTexture()) {
				nRenderFlags |= ERENDERFLAGS_NEEDS_FULL_FB;
			}
			if(pRenderable->DO_NOT_USE_UsesPowerOfTwoFrameBufferTexture()) {
				nRenderFlags |= ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;
			}
		}
		
		if ( nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
		{
			UpdateRefractTexture();
		}

		if ( nRenderFlags & ERENDERFLAGS_NEEDS_FULL_FB )
		{
			UpdateScreenEffectTexture();
		}

		DrawTranslucentRenderable( pRenderable, pRenderableMod, pEntities[iCurTranslucentEntity].m_InstanceData, pEntities[iCurTranslucentEntity].m_TwoPass != 0, bShadowDepth, true );
		--iCurTranslucentEntity;
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}

static ConVar r_unlimitedrefract( "r_unlimitedrefract", "0" );

static void UpdateNecessaryRenderTargets( int nRenderFlags )
{
	if ( nRenderFlags )
	{
		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		ITexture *rt = pRenderContext->GetRenderTarget();

		// supress refract update
		if (
			( nRenderFlags & ERENDERFLAGS_REFRACT_ONLY_ONCE_PER_FRAME ) &&
			( nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB ) &&
			( gpGlobals->framecount == g_viewscene_refractUpdateFrame ) &&
			( r_unlimitedrefract.GetInt() == 0 ) )
		{
			nRenderFlags &= ~( ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB );
		}
		if ( rt )
		{
			if ( nRenderFlags & ERENDERFLAGS_NEEDS_FULL_FB )
			{
				UpdateScreenEffectTexture( 0, 0, 0, rt->GetActualWidth(), rt->GetActualHeight(), true );
			}
			else if ( nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
			{
				UpdateRefractTexture(0, 0, rt->GetActualWidth(), rt->GetActualHeight());
			}
		}
		else
		{
			if ( nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB )
			{
				UpdateRefractTexture();
			}
		}

		pRenderContext.SafeRelease();
	}
}

ConVar cl_tlucfastpath( "cl_tlucfastpath", "1" );
extern ConVar cl_colorfastpath;

//-----------------------------------------------------------------------------
// Renders all translucent world, entities, and detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CRendering3dView::DrawTranslucentRenderables( bool bInSkybox, bool bShadowDepth )
{
	const ClientWorldListInfo_t& info = *m_pWorldListInfo;

#ifdef PORTAL //if we're in the portal mod, we need to make a detour so we can render portal views using stencil areas
	if( ShouldDrawPortals() ) //no recursive stencil views during skybox rendering (although we might be drawing a skybox while already in a recursive stencil view)
	{
		int iDrawFlagsBackup = m_DrawFlags;

		if( g_pPortalRender->DrawPortalsUsingStencils( (CViewRender *)m_pMainView ) )// @MULTICORE (toml 8/10/2006): remove this hack cast
		{
			m_DrawFlags = iDrawFlagsBackup;

			//reset visibility
			unsigned int iVisFlags = 0;
			m_pMainView->SetupVis( *this, iVisFlags, m_pCustomVisibility );		

			//recreate drawlists (since I can't find an easy way to backup the originals)
			{
				SafeRelease( m_pWorldRenderList );
				SafeRelease( m_pWorldListInfo );
				BuildWorldRenderLists( ((m_DrawFlags & DF_DRAW_ENTITITES) != 0), m_pCustomVisibility ? m_pCustomVisibility->m_iForceViewLeaf : -1, false );

				AssertMsg( m_DrawFlags & DF_DRAW_ENTITITES, "It shouldn't be possible to get here if this wasn't set, needs special case investigation" );
				for( int i = m_pRenderablesList->m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY]; --i >= 0; )
				{
					m_pRenderablesList->m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY][i].m_pRenderable->ComputeFxBlend();
				}
			}

			if( r_depthoverlay.GetBool() )
			{
				CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
				ITexture *pDepthTex = GetFullFrameDepthTexture();

				IMaterial *pMaterial = g_pMaterialSystem->FindMaterial( "debug/showz", TEXTURE_GROUP_OTHER, true );
				pMaterial->IncrementReferenceCount();
				IMaterialVar *BaseTextureVar = pMaterial->FindVar( "$basetexture", NULL, false );
				IMaterialVar *pDepthInAlpha = NULL;
				if( IsPC() )
				{
					pDepthInAlpha = pMaterial->FindVar( "$ALPHADEPTH", NULL, false );
					pDepthInAlpha->SetIntValue( 1 );
				}

				BaseTextureVar->SetTextureValue( pDepthTex );

				pRenderContext->OverrideDepthEnable( true, false ); //don't write to depth, or else we'll never see translucents
				pRenderContext->DrawScreenSpaceQuad( pMaterial );
				pRenderContext->OverrideDepthEnable( false, true );
				pMaterial->DecrementReferenceCount();
			}
		}
		else
		{
			//done recursing in, time to go back out and do translucents
			CMatRenderContextPtr pRenderContext( g_pMaterialSystem );		

			UpdateFullScreenDepthTexture();
		}
	}
#else
	{
		//opaques generally write depth, and translucents generally don't.
		//So immediately after opaques are done is the best time to snap off the depth buffer to a texture.
		switch ( g_CurrentViewID )
		{				 
		case VIEW_MAIN:
			UpdateFullScreenDepthTexture();
			break;

		default:
			g_pMaterialSystem->GetRenderContext()->SetFullScreenDepthTextureValidityFlag( false );
			break;
		}
	}
#endif

	if ( !r_drawtranslucentworld->GetBool() )
	{
		DrawTranslucentRenderablesNoWorld( bInSkybox );
		return;
	}

	VPROF_BUDGET( "CViewRender::DrawTranslucentRenderables", "DrawTranslucentRenderables" );
	int iPrevLeaf = info.m_LeafCount - 1;
	int nDetailLeafCount = 0;
	LeafIndex_t *pDetailLeafList = (LeafIndex_t*)stackalloc( info.m_LeafCount * sizeof(LeafIndex_t) );

// 	bool bDrawUnderWater = (nFlags & DF_RENDER_UNDERWATER) != 0;
// 	bool bDrawAboveWater = (nFlags & DF_RENDER_ABOVEWATER) != 0;
// 	bool bDrawWater = (nFlags & DF_RENDER_WATER) != 0;
// 	bool bClipSkybox = (nFlags & DF_CLIP_SKYBOX ) != 0;
	unsigned long nEngineDrawFlags = BuildEngineDrawWorldListFlags( m_DrawFlags & ~DF_DRAWSKYBOX );

	DetailObjectSystem()->BeginTranslucentDetailRendering();

	if ( m_pMainView->ShouldDrawEntities() && r_drawtranslucentrenderables.GetBool() )
	{
		MDLCACHE_CRITICAL_SECTION();
		// Draw the particle singletons.
		DrawParticleSingletons( bInSkybox );

		CClientRenderablesList::CEntry *pEntities = m_pRenderablesList->m_RenderGroups[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
		int iCurTranslucentEntity = m_pRenderablesList->m_RenderGroupCounts[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY] - 1;

		bool bRenderingWaterRenderTargets = m_DrawFlags & ( DF_RENDER_REFRACTION | DF_RENDER_REFLECTION );

		while( iCurTranslucentEntity >= 0 )
		{
			// Seek the current leaf up to our current translucent-entity leaf.
			int iThisLeaf = pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf;

			// First draw the translucent parts of the world up to and including those in this leaf
			DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, iThisLeaf, nEngineDrawFlags, nDetailLeafCount, pDetailLeafList, bShadowDepth );

			// We're traversing the leaf list backwards to get the appropriate sort ordering (back to front)
			iPrevLeaf = iThisLeaf - 1;

			// Draw all the translucent entities with this leaf.
			int nLeaf = info.m_pLeafList[iThisLeaf];

			bool bDrawDetailProps = ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( nLeaf, m_pMainView->BuildWorldListsNumber() );
			if ( bDrawDetailProps && r_DrawDetailProps.GetInt() != 0 )
			{
				// Draw detail props up to but not including this leaf
				Assert( nDetailLeafCount > 0 ); 
				--nDetailLeafCount;
				Assert( pDetailLeafList[nDetailLeafCount] == nLeaf );
				DetailObjectSystem()->RenderTranslucentDetailObjects( DistanceFadeInfo_t(), CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );

				// Draw translucent renderables in the leaf interspersed with detail props
				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
					IClientRenderableMod *pRenderableMod = pEntities[iCurTranslucentEntity].m_pRenderableMod;

					// Draw any detail props in this leaf that's farther than the entity
					const Vector &vecRenderOrigin = pRenderable->GetRenderOrigin();
					DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( DistanceFadeInfo_t(), CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nLeaf, &vecRenderOrigin );

					int nRenderFlags = pRenderableMod ? pRenderableMod->GetRenderFlags() : 0;
					if(!pRenderableMod) {
						if(pRenderable->DO_NOT_USE_UsesFullFrameBufferTexture()) {
							nRenderFlags |= ERENDERFLAGS_NEEDS_FULL_FB;
						}
						if(pRenderable->DO_NOT_USE_UsesPowerOfTwoFrameBufferTexture()) {
							nRenderFlags |= ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;
						}
					}

					bool bUsesPowerOfTwoFB = nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;
					bool bUsesFullFB       = nRenderFlags & ERENDERFLAGS_NEEDS_FULL_FB;

					if ( ( bUsesPowerOfTwoFB || bUsesFullFB )&& !bShadowDepth )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}

						CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
						ITexture *rt = pRenderContext->GetRenderTarget();

						if ( rt && bUsesFullFB )
						{
							UpdateScreenEffectTexture( 0, 0, 0, rt->GetActualWidth(), rt->GetActualHeight(), true );
						}
						else if ( bUsesPowerOfTwoFB )
						{
							UpdateRefractTexture();
						}

						pRenderContext.SafeRelease();
					}

					// Then draw the translucent renderable
					DrawTranslucentRenderable( pRenderable, pRenderableMod, pEntities[iCurTranslucentEntity].m_InstanceData, (pEntities[iCurTranslucentEntity].m_TwoPass != 0), bShadowDepth, false );
				}

				// Draw all remaining props in this leaf
				DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( m_pRenderablesList->m_DetailFade, CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nLeaf, NULL );
			}
			else
			{
				if(r_DrawDetailProps.GetInt() != 0) {
					// Draw queued up detail props (we know that the list of detail leaves won't include this leaf, since ShouldDrawDetailObjectsInLeaf is false)
					// Therefore no fixup on nDetailLeafCount is required as in the above section
					DetailObjectSystem()->RenderTranslucentDetailObjects( m_pRenderablesList->m_DetailFade, CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );
				}

				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
					IClientRenderableMod *pRenderableMod = pEntities[iCurTranslucentEntity].m_pRenderableMod;

					int nRenderFlags = pRenderableMod ? pRenderableMod->GetRenderFlags() : 0;
					if(!pRenderableMod) {
						if(pRenderable->DO_NOT_USE_UsesFullFrameBufferTexture()) {
							nRenderFlags |= ERENDERFLAGS_NEEDS_FULL_FB;
						}
						if(pRenderable->DO_NOT_USE_UsesPowerOfTwoFrameBufferTexture()) {
							nRenderFlags |= ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;
						}
					}

					bool bUsesPowerOfTwoFB = nRenderFlags & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;
					bool bUsesFullFB       = nRenderFlags & ERENDERFLAGS_NEEDS_FULL_FB;

					if ( ( bUsesPowerOfTwoFB || bUsesFullFB )&& !bShadowDepth )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}

						UpdateNecessaryRenderTargets( nRenderFlags );
					}

					DrawTranslucentRenderable( pRenderable, pRenderableMod, pEntities[iCurTranslucentEntity].m_InstanceData, (pEntities[iCurTranslucentEntity].m_TwoPass != 0), bShadowDepth, false );
				}
			}

			nDetailLeafCount = 0;
		}
	}

	// Draw the rest of the surfaces in world leaves
	DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, 0, nEngineDrawFlags, nDetailLeafCount, pDetailLeafList, bShadowDepth );

	if(r_DrawDetailProps.GetInt() != 0) {
		// Draw any queued-up detail props from previously visited leaves
		DetailObjectSystem()->RenderTranslucentDetailObjects( m_pRenderablesList->m_DetailFade, CurrentViewOrigin(), CurrentViewForward(), CurrentViewRight(), CurrentViewUp(), nDetailLeafCount, pDetailLeafList );
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::EnableWorldFog( void )
{
	VPROF("CViewRender::EnableWorldFog");
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	fogparams_t *pFogParams = NULL;
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if ( pbp )
	{
		pFogParams = pbp->GetFogParams();
	}

	if( GetFogEnable( pFogParams ) )
	{
		float fogColor[3];
		GetFogColor( pFogParams, fogColor );
		pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
		pRenderContext->FogColor3fv( fogColor );
		pRenderContext->FogStart( GetFogStart( pFogParams ) );
		pRenderContext->FogEnd( GetFogEnd( pFogParams ) );
		pRenderContext->FogMaxDensity( GetFogMaxDensity( pFogParams ) );
	}
	else
	{
		pRenderContext->FogMode( MATERIAL_FOG_NONE );
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int CRendering3dView::GetDrawFlags()
{
	return m_DrawFlags;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CRendering3dView::SetFogVolumeState( const VisibleFogVolumeInfo_t &fogInfo, bool bUseHeightFog )
{
	render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, bUseHeightFog );

#ifdef PORTAL

	//the idea behind fog shifting is this...
	//Normal fog simulates the effect of countless tiny particles between your viewpoint and whatever geometry is rendering.
	//But, when rendering to a portal view, there's a large space between the virtual camera and the portal exit surface.
	//This space isn't supposed to exist, and therefore has none of the tiny particles that make up fog.
	//So, we have to shift fog start/end out to align the distances with the portal exit surface instead of the virtual camera to eliminate fog simulation in the non-space
	if( g_pPortalRender->GetViewRecursionLevel() == 0 )
		return; //rendering one of the primary views, do nothing

	g_pPortalRender->ShiftFogForExitPortalView();

#endif //#ifdef PORTAL
}


//-----------------------------------------------------------------------------
// Standard 3d skybox view
//-----------------------------------------------------------------------------
SkyboxVisibility_t CSkyboxView::ComputeSkyboxVisibility()
{
	return engine->IsSkyboxVisibleFromPoint( origin );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CSkyboxView::GetSkyboxFogEnable()
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return false;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_enableskybox.GetInt() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return !!local->m_skybox3d.fog.enable;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::Enable3dSkyboxFog( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if( GetSkyboxFogEnable() )
	{
		float fogColor[3];
		GetSkyboxFogColor( fogColor );
		float scale = 1.0f;
		if ( local->m_skybox3d.scale > 0.0f )
		{
			scale = 1.0f / local->m_skybox3d.scale;
		}
		pRenderContext->FogMode( MATERIAL_FOG_LINEAR );
		pRenderContext->FogColor3fv( fogColor );
		pRenderContext->FogStart( GetSkyboxFogStart() * scale );
		pRenderContext->FogEnd( GetSkyboxFogEnd() * scale );
		pRenderContext->FogMaxDensity( GetSkyboxFogMaxDensity() );
	}
	else
	{
		pRenderContext->FogMode( MATERIAL_FOG_NONE );
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
sky3dparams_t *CSkyboxView::PreRender3dSkyboxWorld( SkyboxVisibility_t nSkyboxVisible )
{
	if ( ( nSkyboxVisible != SKYBOX_3DSKYBOX_VISIBLE ) && r_3dsky.GetInt() != 2 )
		return NULL;

	// render the 3D skybox
	if ( !r_3dsky.GetInt() )
		return NULL;

	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();

	// No local player object yet...
	if ( !pbp )
		return NULL;

	CPlayerLocalData* local = &pbp->m_Local;
	if ( local->m_skybox3d.area == 255 )
		return NULL;

	return &local->m_skybox3d;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::DrawInternal( view_id_t iSkyBoxViewID, bool bInvokePreAndPostRender, ITexture *pRenderTarget, ITexture *pDepthTarget )
{
	unsigned char **areabits = render->GetAreaBits();
	unsigned char *savebits;
	unsigned char tmpbits[ 32 ];
	savebits = *areabits;
	memset( tmpbits, 0, sizeof(tmpbits) );

	// set the sky area bit
	tmpbits[m_pSky3dParams->area>>3] |= 1 << (m_pSky3dParams->area&7);

	*areabits = tmpbits;

	// if you can get really close to the skybox geometry it's possible that you'll be able to clip into it
	// with this near plane.  If so, move it in a bit.  It's at 2.0 to give us more precision.  That means you 
	// need to keep the eye position at least 2 * scale away from the geometry in the skybox
	zNear = r_nearz_skybox.GetFloat();

	// Use the fog's farz if specified
	if (m_pSky3dParams->fog.farz > 0)
	{
		zFar = ( m_pSky3dParams->scale > 0.0f ?
			m_pSky3dParams->fog.farz / m_pSky3dParams->scale :
			m_pSky3dParams->fog.farz );
	}
	else
		zNear = 2.0;
	zFar = MAX_TRACE_LENGTH;

	// scale origin by sky scale
	float scale = (m_pSky3dParams->scale > 0) ? (1.0f / m_pSky3dParams->scale) : 1.0f;
	VectorScale( origin, scale, origin );

	// Skybox angle support.
	// 
	// If any of the angles aren't 0, do the rotation code.
	if (m_pSky3dParams->skycamera)
	{
		// Re-use the x coordinate to determine if we shuld do this with angles
		if (m_pSky3dParams->angles.GetX() != 0)
		{
			const matrix3x4_t &matSky = m_pSky3dParams->skycamera->EntityToWorldTransform();
			matrix3x4_t matView;
			AngleMatrix( angles, origin, matView );
			ConcatTransforms( matSky, matView, matView );
			MatrixAngles( matView, angles, origin );
		}
		else
		{
			Vector vSkyOrigin = m_pSky3dParams->skycamera->GetAbsOrigin();
			VectorAdd( origin, vSkyOrigin, origin );

			if( m_bCustomViewMatrix )
			{
				Vector vTransformedSkyOrigin;
				VectorRotate( vSkyOrigin, m_matCustomViewMatrix, vTransformedSkyOrigin ); //Rotate instead of transform because we haven't scale the existing offset yet

				//scale existing translation, and tack on the skybox offset (subtract because it's a view matrix)
				m_matCustomViewMatrix.m_flMatVal[0][3] = (m_matCustomViewMatrix.m_flMatVal[0][3] * scale) - vTransformedSkyOrigin.x;
				m_matCustomViewMatrix.m_flMatVal[1][3] = (m_matCustomViewMatrix.m_flMatVal[1][3] * scale) - vTransformedSkyOrigin.y;
				m_matCustomViewMatrix.m_flMatVal[2][3] = (m_matCustomViewMatrix.m_flMatVal[2][3] * scale) - vTransformedSkyOrigin.z;
			}
		}
	}
	else
	{
		if (m_pSky3dParams->angles.GetX() != 0 ||
			m_pSky3dParams->angles.GetY() != 0 ||
			m_pSky3dParams->angles.GetZ() != 0)
		{
			matrix3x4_t matSky, matView;
			AngleMatrix( m_pSky3dParams->angles, m_pSky3dParams->origin, matSky );
			AngleMatrix( angles, origin, matView );
			ConcatTransforms( matSky, matView, matView );
			MatrixAngles( matView, angles, origin );
		}
		else
		{
			Vector vSkyOrigin = m_pSky3dParams->origin;
			VectorAdd( origin, vSkyOrigin, origin );

			if( m_bCustomViewMatrix )
			{
				Vector vTransformedSkyOrigin;
				VectorRotate( vSkyOrigin, m_matCustomViewMatrix, vTransformedSkyOrigin ); //Rotate instead of transform because we haven't scale the existing offset yet

				//scale existing translation, and tack on the skybox offset (subtract because it's a view matrix)
				m_matCustomViewMatrix.m_flMatVal[0][3] = (m_matCustomViewMatrix.m_flMatVal[0][3] * scale) - vTransformedSkyOrigin.x;
				m_matCustomViewMatrix.m_flMatVal[1][3] = (m_matCustomViewMatrix.m_flMatVal[1][3] * scale) - vTransformedSkyOrigin.y;
				m_matCustomViewMatrix.m_flMatVal[2][3] = (m_matCustomViewMatrix.m_flMatVal[2][3] * scale) - vTransformedSkyOrigin.z;
			}
		}
	}

	Enable3dSkyboxFog();

	// BUGBUG: Fix this!!!  We shouldn't need to call setup vis for the sky if we're connecting
	// the areas.  We'd have to mark all the clusters in the skybox area in the PVS of any 
	// cluster with sky.  Then we could just connect the areas to do our vis.
	//m_bOverrideVisOrigin could hose us here, so call direct
	render->ViewSetupVis( false, 1, m_pSky3dParams->skycamera ? &m_pSky3dParams->skycamera->GetAbsOrigin() : &m_pSky3dParams->origin.Get() );
	render->Push3DView( (*this), m_ClearFlags, pRenderTarget, GetFrustum(), pDepthTarget );

	// Store off view origin and angles
	SetupCurrentView( origin, angles, iSkyBoxViewID );

	// Invoke pre-render methods
	if ( bInvokePreAndPostRender )
	{
		IGameSystem::PreRenderAllSystems();
	}

	render->BeginUpdateLightmaps();
	BuildWorldRenderLists( true, -1, true );
	BuildRenderableRenderLists( iSkyBoxViewID );
	render->EndUpdateLightmaps();

	g_pClientShadowMgr->ComputeShadowTextures( (*this), m_pWorldListInfo->m_LeafCount, m_pWorldListInfo->m_pLeafList );

	DrawWorld( 0.0f );

	// Iterate over all leaves and render objects in those leaves
	DrawOpaqueRenderables( DEPTH_MODE_NORMAL );

	// Iterate over all leaves and render objects in those leaves
	DrawTranslucentRenderables( true, false );
	DrawNoZBufferTranslucentRenderables();

	// Allows reflective glass to be drawn in the skybox.
	// New expansions also allow for custom render targets and multiple mirror renders
	if (r_skybox_use_complex_views.GetBool())
	{
		VisibleFogVolumeInfo_t fogVolumeInfo;
		render->GetVisibleFogVolume( origin, &fogVolumeInfo );

		WaterRenderInfo_t info;
		info.m_bCheapWater = true;
		info.m_bRefract = false;
		info.m_bReflect = false;
		info.m_bReflectEntities = false;
		info.m_bDrawWaterSurface = false;
		info.m_bOpaqueWater = true;

		cplane_t glassReflectionPlane;
		Frustum_t frustum;
		GeneratePerspectiveFrustum( origin, angles, zNear, zFar, fov, m_flAspectRatio, frustum );

		ITexture *pTextureTargets[2];
		C_BaseEntity *pReflectiveGlass = NextReflectiveGlass( NULL, (*this), glassReflectionPlane, frustum, pTextureTargets );
		while ( pReflectiveGlass != NULL )
		{
			if (pTextureTargets[0])
			{
				CRefPtr<CReflectiveGlassView> pGlassReflectionView = new CReflectiveGlassView( m_pMainView );
				pGlassReflectionView->m_pRenderTarget = pTextureTargets[0];
				pGlassReflectionView->Setup( (*this), VIEW_CLEAR_DEPTH, true, fogVolumeInfo, info, glassReflectionPlane );
				m_pMainView->AddViewToScene( pGlassReflectionView );
			}

			if (pTextureTargets[1])
			{
				CRefPtr<CRefractiveGlassView> pGlassRefractionView = new CRefractiveGlassView( m_pMainView );
				pGlassRefractionView->m_pRenderTarget = pTextureTargets[1];
				pGlassRefractionView->Setup( (*this), VIEW_CLEAR_DEPTH, true, fogVolumeInfo, info, glassReflectionPlane );
				m_pMainView->AddViewToScene( pGlassRefractionView );
			}

			pReflectiveGlass = NextReflectiveGlass( pReflectiveGlass, (*this), glassReflectionPlane, frustum, pTextureTargets );
		}
	}

	m_pMainView->DisableFog();

	CGlowOverlay::UpdateSkyOverlays( zFar, m_bCacheFullSceneState );

	PixelVisibility_EndCurrentView();

	// restore old area bits
	*areabits = savebits;

	// Invoke post-render methods
	if( bInvokePreAndPostRender )
	{
		IGameSystem::PostRenderAllSystems();
		FinishCurrentView();
	}

	// FIXME: Workaround to 3d skybox not depth-of-fielding properly. The real fix is for the 3d skybox dest alpha depth values
	// to be biased. Currently all I do is clear alpha to 1 after the 3D skybox path. This avoids the skybox being unblurred.
	if( IsDepthOfFieldEnabled() )
	{
		// draw a fullscreen quad setting destalpha to 1

		IMaterial *pMat = g_pMaterialSystem->FindMaterial( "dev/clearalpha", TEXTURE_GROUP_OTHER, true );
		if ( pMat != NULL )
		{
			int nWidth = 0;
			int nHeight = 0;
			int nDummy = 0;
			CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
			pRenderContext->GetViewport( nDummy, nDummy, nWidth, nHeight );

			pRenderContext->DrawScreenSpaceRectangle(
				pMat,
				0, 0, nWidth, nHeight,
				0, 0, nWidth-1, nHeight-1,
				nWidth, nHeight, NULL /*GetClientWorldEntity()->GetClientRenderable()*/ );
		}
	}

	render->PopView( GetFrustum() );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CSkyboxView::Setup( const CViewSetupEx &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible )
{
	BaseClass::Setup( view );

	// The skybox might not be visible from here
	*pSkyboxVisible = ComputeSkyboxVisibility();
	m_pSky3dParams = PreRender3dSkyboxWorld( *pSkyboxVisible );

	if ( !m_pSky3dParams )
	{
		return false;
	}

	// At this point, we've cleared everything we need to clear
	// The next path will need to clear depth, though.
	m_ClearFlags = *pClearFlags;
	*pClearFlags &= ~( VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL | VIEW_CLEAR_FULL_TARGET );
	*pClearFlags |= VIEW_CLEAR_DEPTH; // Need to clear depth after rednering the skybox

	m_DrawFlags = DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_RENDER_WATER;
	if (m_pSky3dParams->skycolor.GetA() != 0 && *pSkyboxVisible != SKYBOX_NOT_VISIBLE)
	{
		m_ClearFlags |= (VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH);
		m_DrawFlags |= DF_CLIP_SKYBOX;

		color32 color = m_pSky3dParams->skycolor.Get();

		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		pRenderContext->ClearColor4ub( color.r(), color.g(), color.b(), color.a() );
		pRenderContext.SafeRelease();
	}
	else if( r_skybox.GetBool() )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	return true;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CSkyboxView::Draw()
{
	VPROF_BUDGET( "CViewRender::Draw3dSkyboxworld", "3D Skybox" );

	ITexture *pRTColor = NULL;
	ITexture *pRTDepth = NULL;

	DrawInternal(VIEW_3DSKY, true, pRTColor, pRTDepth );
}


#ifdef PORTAL
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CPortalSkyboxView::Setup( const CViewSetupEx &view, int *pClearFlags, SkyboxVisibility_t *pSkyboxVisible, ITexture *pRenderTarget )
{
	if ( !BaseClass::Setup( view, pClearFlags, pSkyboxVisible ) )
		return false;

	m_pRenderTarget = pRenderTarget;
	return true;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
SkyboxVisibility_t CPortalSkyboxView::ComputeSkyboxVisibility()
{
	return g_pPortalRender->IsSkyboxVisibleFromExitPortal();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CPortalSkyboxView::Draw()
{
	AssertMsg( (g_pPortalRender->GetViewRecursionLevel() != 0) && g_pPortalRender->IsRenderingPortal(), "This is designed for through-portal views. Use the regular skybox drawing code for primary views" );

	VPROF_BUDGET( "CViewRender::Draw3dSkyboxworld_Portal", "3D Skybox (portal view)" );

	int iCurrentViewID = g_CurrentViewID;

	Frustum FrustumBackup;
	memcpy( FrustumBackup, GetFrustum(), sizeof( Frustum ) );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	bool bClippingEnabled = pRenderContext->EnableClipping( false );

	//NOTE: doesn't magically map to VIEW_3DSKY at (0,0) like PORTAL_VIEWID maps to VIEW_MAIN
	view_id_t iSkyBoxViewID = (view_id_t)g_pPortalRender->GetCurrentSkyboxViewId();

	bool bInvokePreAndPostRender = ( g_pPortalRender->ShouldUseStencilsToRenderPortals() == false );

	DrawInternal( iSkyBoxViewID, bInvokePreAndPostRender, m_pRenderTarget, NULL );

	pRenderContext->EnableClipping( bClippingEnabled );

	memcpy( GetFrustum(), FrustumBackup, sizeof( Frustum ) );
	render->OverrideViewFrustum( FrustumBackup );

	g_CurrentViewID = iCurrentViewID;
}
#endif // PORTAL


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CShadowDepthView::Setup( const CViewSetupEx &shadowViewIn, ITexture *pRenderTarget, ITexture *pDepthTexture )
{
	BaseClass::Setup( shadowViewIn );
	m_pRenderTarget = pRenderTarget;
	m_pDepthTexture = pDepthTexture;
}


bool DrawingShadowDepthView( void ) //for easy externing
{
	return (CurrentViewID() == VIEW_SHADOW_DEPTH_TEXTURE);
}

bool DrawingMainView() //for easy externing
{
	return (CurrentViewID() == VIEW_MAIN);
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CShadowDepthView::Draw()
{
	VPROF_BUDGET( "CShadowDepthView::Draw", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// Start view
	unsigned int visFlags;
	m_pMainView->SetupVis( (*this), visFlags );  // @MULTICORE (toml 8/9/2006): Portal problem, not sending custom vis down

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->ClearColor3ub(0xFF, 0xFF, 0xFF);

	pRenderContext.SafeRelease();

	render->Push3DView( (*this), VIEW_CLEAR_DEPTH, m_pRenderTarget, GetFrustum(), m_pDepthTexture );

	pRenderContext.GetFrom(g_pMaterialSystem);
	pRenderContext->PushRenderTargetAndViewport( m_pRenderTarget, m_pDepthTexture, x, y, width, height );
	pRenderContext.SafeRelease();

	SetupCurrentView( origin, angles, VIEW_SHADOW_DEPTH_TEXTURE );

	MDLCACHE_CRITICAL_SECTION();

	{
		VPROF_BUDGET( "BuildWorldRenderLists", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		BuildWorldRenderLists( true, -1, true, true ); // @MULTICORE (toml 8/9/2006): Portal problem, not sending custom vis down
	}

	{
		VPROF_BUDGET( "BuildRenderableRenderLists", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		BuildRenderableRenderLists( CurrentViewID() );
	}

	engine->Sound_ExtraUpdate();	// Make sure sound doesn't stutter

	m_DrawFlags = m_pMainView->GetBaseDrawFlags() | DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_SHADOW_DEPTH_MAP;	// Don't draw water surface...

	{
		VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawWorld( 0.0f );
	}

	// Draw opaque and translucent renderables with appropriate override materials
	// OVERRIDE_DEPTH_WRITE is OK with a NULL material pointer
	modelrender->ForcedMaterialOverride( NULL, OVERRIDE_DEPTH_WRITE );	

	{
		VPROF_BUDGET( "DrawOpaqueRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawOpaqueRenderables( DEPTH_MODE_SHADOW );
	}

	if ( m_bRenderFlashlightDepthTranslucents || r_flashlightdepth_drawtranslucents.GetBool() )
	{
		VPROF_BUDGET( "DrawTranslucentRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawTranslucentRenderables( false, true );
	}

	modelrender->ForcedMaterialOverride( 0 );

	m_DrawFlags = 0;

	pRenderContext.GetFrom( g_pMaterialSystem );

	pRenderContext->PopRenderTargetAndViewport();

	render->PopView( GetFrustum() );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CFreezeFrameView::Setup( const CViewSetupEx &shadowViewIn )
{
	BaseClass::Setup( shadowViewIn );

	VGui_GetTrueScreenSize( m_nScreenSize[ 0 ], m_nScreenSize[ 1 ] );
	VGui_GetPanelBounds( m_nSubRect[ 0 ], m_nSubRect[ 1 ], m_nSubRect[ 2 ], m_nSubRect[ 3 ] );

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", "_rt_FullScreen" );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	pVMTKeyValues->SetInt( "$nofog", 1 );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	m_pFreezeFrame.Init( "FreezeFrame_FullScreen", TEXTURE_GROUP_OTHER, pVMTKeyValues );
	m_pFreezeFrame->Refresh();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFreezeFrameView::Draw( void )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->DrawScreenSpaceRectangle( m_pFreezeFrame, x, y, width, height,
		m_nSubRect[ 0 ], m_nSubRect[ 1 ], m_nSubRect[ 0 ] + m_nSubRect[ 2 ] - 1, m_nSubRect[ 1 ] + m_nSubRect[ 3 ] - 1, m_nScreenSize[ 0 ], m_nScreenSize[ 1 ] );

	//Fake a fade during freezeframe view.
	if ( g_flFreezeFlash >= gpGlobals->curtime && engine->IsTakingScreenshot() == false )
	{
		// Overlay screen fade on entire screen
		IMaterial* pMaterial = m_TranslucentSingleColor;

		int iFadeAlpha = FREEZECAM_SNAPSHOT_FADE_SPEED * ( g_flFreezeFlash - gpGlobals->curtime );
		
		iFadeAlpha = MIN( iFadeAlpha, 255 );
		iFadeAlpha = MAX( 0, iFadeAlpha );
		
		pMaterial->AlphaModulate( iFadeAlpha * ( 1.0f / 255.0f ) );
		pMaterial->ColorModulate( 1.0f,	1.0f, 1.0f );
		pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true );

		pRenderContext->DrawScreenSpaceRectangle( pMaterial, x, y, width, height, m_nSubRect[ 0 ], m_nSubRect[ 1 ], m_nSubRect[ 0 ] + m_nSubRect[ 2 ] - 1, m_nSubRect[ 1 ] + m_nSubRect[ 3 ] - 1, m_nScreenSize[ 0 ], m_nScreenSize[ 1 ] );
	}
}

//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
bool CBaseWorldView::AdjustView( float waterHeight )
{
	if( m_DrawFlags & DF_RENDER_REFRACTION )
	{
		ITexture *pTexture = GetRefractionTexture();

		// Use the aspect ratio of the main view! So, don't recompute it here
		x = y = 0;
		width = pTexture->GetActualWidth();
		height = pTexture->GetActualHeight();

		return true;
	}

	if( m_DrawFlags & DF_RENDER_REFLECTION )
	{
		ITexture *pTexture = GetReflectionTexture();

		// If the main view is overriding the projection matrix (for Stereo or
		// some other nefarious purpose) make sure to include any Y offset in 
		// the custom projection matrix in our reflected overridden projection
		// matrix.
		if( m_bViewToProjectionOverride )
		{
			m_ViewToProjection[1][2] = -m_ViewToProjection[1][2];
		}

		// Use the aspect ratio of the main view! So, don't recompute it here
		x = y = 0;
		width = pTexture->GetActualWidth();
		height = pTexture->GetActualHeight();
		angles[0] = -angles[0];
		angles[2] = -angles[2];
		origin[2] -= 2.0f * ( origin[2] - (waterHeight));
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
void CBaseWorldView::PushView( float waterHeight )
{
	float spread = 2.0f;
	if( m_DrawFlags & DF_FUDGE_UP )
	{
		waterHeight += spread;
	}
	else
	{
		waterHeight -= spread;
	}

	MaterialHeightClipMode_t clipMode = MATERIAL_HEIGHTCLIPMODE_DISABLE;
	if ( ( m_DrawFlags & DF_CLIP_Z ) && mat_clipz.GetBool() )
	{
		if( m_DrawFlags & DF_CLIP_BELOW )
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT;
		}
		else
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT;
		}
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if( m_DrawFlags & DF_RENDER_REFRACTION )
	{
		pRenderContext->SetFogZ( waterHeight );
		pRenderContext->SetHeightClipZ( waterHeight );
		pRenderContext->SetHeightClipMode( clipMode );

		// Have to re-set up the view since we reset the size
		render->Push3DView( *this, m_ClearFlags, GetRefractionTexture(), GetFrustum() );

		return;
	}

	if( m_DrawFlags & DF_RENDER_REFLECTION )
	{
		ITexture *pTexture = GetReflectionTexture();

		pRenderContext->SetFogZ( waterHeight );

		bool bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();
		if( bSoftwareUserClipPlane && ( origin[2] > waterHeight - r_eyewaterepsilon.GetFloat() ) )
		{
			waterHeight = origin[2] + r_eyewaterepsilon.GetFloat();
		}

		pRenderContext->SetHeightClipZ( waterHeight );
		pRenderContext->SetHeightClipMode( clipMode );

		render->Push3DView( *this, m_ClearFlags, pTexture, GetFrustum() );

		SetLightmapScaleForWater();
		return;
	}

	if ( m_ClearFlags & ( VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR | VIEW_CLEAR_STENCIL ) )
	{
		if ( m_ClearFlags & VIEW_CLEAR_OBEY_STENCIL )
		{
			pRenderContext->ClearBuffersObeyStencil( ( m_ClearFlags & VIEW_CLEAR_COLOR ) ? true : false, ( m_ClearFlags & VIEW_CLEAR_DEPTH ) ? true : false );
		}
		else
		{
			pRenderContext->ClearBuffers( ( m_ClearFlags & VIEW_CLEAR_COLOR ) ? true : false, ( m_ClearFlags & VIEW_CLEAR_DEPTH ) ? true : false, ( m_ClearFlags & VIEW_CLEAR_STENCIL ) ? true : false );
		}
	}

	pRenderContext->SetHeightClipMode( clipMode );
	if ( clipMode != MATERIAL_HEIGHTCLIPMODE_DISABLE )
	{   
		pRenderContext->SetHeightClipZ( waterHeight );
	}
}


//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
void CBaseWorldView::PopView()
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );
	if( m_DrawFlags & (DF_RENDER_REFRACTION | DF_RENDER_REFLECTION) )
	{
		render->PopView( GetFrustum() );
		if (SavedLinearLightMapScale.x>=0)
		{
			pRenderContext->SetToneMappingScaleLinear(SavedLinearLightMapScale);
			SavedLinearLightMapScale.x=-1;
		}
	}
}


//-----------------------------------------------------------------------------
// Draws the world + entities
//-----------------------------------------------------------------------------
void CBaseWorldView::DrawSetup( float waterHeight, int nSetupFlags, float waterZAdjust, int iForceViewLeaf )
{
	int savedViewID = g_CurrentViewID;
	g_CurrentViewID = VIEW_ILLEGAL;

	bool bViewChanged = AdjustView( waterHeight );

	if ( bViewChanged )
	{
		render->Push3DView( *this, 0, NULL, GetFrustum() );
	}

	render->BeginUpdateLightmaps();

	bool bDrawEntities = ( nSetupFlags & DF_DRAW_ENTITITES ) != 0;
	bool bDrawReflection = ( nSetupFlags & DF_RENDER_REFLECTION ) != 0;
	BuildWorldRenderLists( bDrawEntities, iForceViewLeaf, true, false, bDrawReflection ? &waterHeight : NULL );

	PruneWorldListInfo();

	if ( bDrawEntities )
	{
		BuildRenderableRenderLists( savedViewID );
	}

	render->EndUpdateLightmaps();

	if ( bViewChanged )
	{
		render->PopView( GetFrustum() );
	}

#ifdef TF_CLIENT_DLL
	bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

	if ( savedViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() )
	{
		SSAO_DepthPass();
	}
#endif

	g_CurrentViewID = savedViewID;
}


void MaybeInvalidateLocalPlayerAnimation()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( ( pPlayer != NULL ) && pPlayer->InFirstPersonView() )
	{
		// We sometimes need different animation for the main view versus the shadow rendering,
		// so we need to reset the cache to ensure this actually happens.
		pPlayer->InvalidateBoneCache();

		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		if ( pWeapon != NULL )
		{
			pWeapon->InvalidateBoneCache();
		}

#if defined USES_ECON_ITEMS
		// ...and all the things you're wearing/holding/etc
		int NumWearables = pPlayer->GetNumWearables();
		for ( int i = 0; i < NumWearables; ++i )
		{
			CEconWearable* pItem = pPlayer->GetWearable ( i );
			if ( pItem != NULL )
			{
				pItem->InvalidateBoneCache();
			}
		}
#endif // USES_ECON_ITEMS

	}
}

void CBaseWorldView::DrawExecute( float waterHeight, view_id_t viewID, float waterZAdjust )
{
	int savedViewID = g_CurrentViewID;

	// @MULTICORE (toml 8/16/2006): rethink how, where, and when this is done...
	g_CurrentViewID = VIEW_SHADOW_DEPTH_TEXTURE;
	MaybeInvalidateLocalPlayerAnimation();
	g_pClientShadowMgr->ComputeShadowTextures( *this, m_pWorldListInfo->m_LeafCount, m_pWorldListInfo->m_pLeafList );
	MaybeInvalidateLocalPlayerAnimation();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	g_CurrentViewID = viewID;

	// Update our render view flags.
	int iDrawFlagsBackup = m_DrawFlags;
	m_DrawFlags |= m_pMainView->GetBaseDrawFlags();

	PushView( waterHeight );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	ITexture *pSaveFrameBufferCopyTexture = pRenderContext->GetFrameBufferCopyTexture( 0 );
	if ( engine->GetDXSupportLevel() >= 80 )
	{
		pRenderContext->SetFrameBufferCopyTexture( GetPowerOfTwoFrameBufferTexture() );
	}

	pRenderContext.SafeRelease();

	ERenderDepthMode DepthMode = DEPTH_MODE_NORMAL;

	m_DrawFlags |= DF_SKIP_WORLD_DECALS_AND_OVERLAYS;
	DrawWorld( waterZAdjust );
	m_DrawFlags &= ~DF_SKIP_WORLD_DECALS_AND_OVERLAYS;

	if ( m_DrawFlags & DF_DRAW_ENTITITES )
	{
		DrawOpaqueRenderables( DepthMode );

#ifdef TF_CLIENT_DLL
		bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

		if ( g_CurrentViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() ) // Pyro-vision Goggles
		{
			DrawDepthOfField();
		}
#endif
		DrawTranslucentRenderables( false, false );
		DrawNoZBufferTranslucentRenderables();
	}
	else
	{
#ifdef TF_CLIENT_DLL
		bool bVisionOverride = ( localplayer_visionflags.GetInt() & ( 0x01 ) ); // Pyro-vision Goggles

		if ( g_CurrentViewID == VIEW_MAIN && bVisionOverride && pyro_dof.GetBool() ) // Pyro-vision Goggles
		{
			DrawDepthOfField();
		}
#endif
		// Draw translucent world brushes only, no entities
		DrawTranslucentWorldInLeaves( false );
	}

	// issue the pixel visibility tests for sub-views
	if ( !IsMainView( CurrentViewID() ) && CurrentViewID() != VIEW_INTRO_CAMERA )
	{
		PixelVisibility_EndCurrentView();
	}

	pRenderContext.GetFrom( g_pMaterialSystem );
	pRenderContext->SetFrameBufferCopyTexture( pSaveFrameBufferCopyTexture );
	PopView();

	m_DrawFlags = iDrawFlagsBackup;

	g_CurrentViewID = savedViewID;
}


void CBaseWorldView::SSAO_DepthPass()
{
	if ( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		return;
	}

#if 1
	VPROF_BUDGET( "CSimpleWorldView::SSAO_DepthPass", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	int savedViewID = g_CurrentViewID;
	g_CurrentViewID = VIEW_SSAO;

	ITexture *pSSAO = g_pMaterialSystem->FindTexture( "_rt_ResolvedFullFrameDepth", TEXTURE_GROUP_RENDER_TARGET );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->ClearColor4ub( 255, 255, 255, 255 );

	pRenderContext.SafeRelease();

	render->Push3DView( (*this), VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, pSSAO, GetFrustum() );

	MDLCACHE_CRITICAL_SECTION();

	engine->Sound_ExtraUpdate();	// Make sure sound doesn't stutter

	m_DrawFlags |= DF_SSAO_DEPTH_PASS;

	{
		VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawWorld( 0.0f );
	}

	// Draw opaque and translucent renderables with appropriate override materials
	// OVERRIDE_SSAO_DEPTH_WRITE is OK with a NULL material pointer
	modelrender->ForcedMaterialOverride( NULL, OVERRIDE_SSAO_DEPTH_WRITE );	

	{
		VPROF_BUDGET( "DrawOpaqueRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawOpaqueRenderables( DEPTH_MODE_SSA0 );
	}

#if 0
	if ( m_bRenderFlashlightDepthTranslucents || r_flashlightdepth_drawtranslucents.GetBool() )
	{
		VPROF_BUDGET( "DrawTranslucentRenderables", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );
		DrawTranslucentRenderables( false, true );
	}
#endif

	modelrender->ForcedMaterialOverride( 0 );

	m_DrawFlags &= ~DF_SSAO_DEPTH_PASS;

	pRenderContext.GetFrom( g_pMaterialSystem );

	render->PopView( GetFrustum() );

	pRenderContext.SafeRelease();

	g_CurrentViewID = savedViewID;
#endif
}


void CBaseWorldView::DrawDepthOfField( )
{
	if ( !g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	ITexture *pSmallFB0 = g_pMaterialSystem->FindTexture( "_rt_smallfb0", TEXTURE_GROUP_RENDER_TARGET );
	ITexture *pSmallFB1 = g_pMaterialSystem->FindTexture( "_rt_smallfb1", TEXTURE_GROUP_RENDER_TARGET );

	Rect_t	DestRect;
	int w = pSmallFB0->GetActualWidth();
	int h = pSmallFB0->GetActualHeight();
	DestRect.x = 0;
	DestRect.y = 0;
	DestRect.width = w;
	DestRect.height = h;

	pRenderContext->CopyRenderTargetToTextureEx( pSmallFB0, 0, NULL, &DestRect );

	IMaterial *pPyroBlurXMaterial = g_pMaterialSystem->FindMaterial( "dev/pyro_blur_filter_x", TEXTURE_GROUP_OTHER );
	IMaterial *pPyroBlurYMaterial = g_pMaterialSystem->FindMaterial( "dev/pyro_blur_filter_y", TEXTURE_GROUP_OTHER );

	pRenderContext->PushRenderTargetAndViewport( pSmallFB1, 0, 0, w, h );
	pRenderContext->DrawScreenSpaceRectangle( pPyroBlurYMaterial, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
	pRenderContext->PopRenderTargetAndViewport();

	pRenderContext->PushRenderTargetAndViewport( pSmallFB0, 0, 0, w, h );
	pRenderContext->DrawScreenSpaceRectangle( pPyroBlurXMaterial, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
	pRenderContext->PopRenderTargetAndViewport();

	IMaterial *pPyroDepthOfFieldMaterial = g_pMaterialSystem->FindMaterial( "dev/pyro_dof",  TEXTURE_GROUP_OTHER );

	pRenderContext->DrawScreenSpaceRectangle( pPyroDepthOfFieldMaterial, x, y, width, height, 0, 0, width-1, height-1, width, height );
}

//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
void CSimpleWorldView::Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, ViewCustomVisibility_t *pCustomVisibility )
{
	BaseClass::Setup( view );

	m_ClearFlags = nClearFlags;
	m_DrawFlags = DF_DRAW_ENTITITES;

	if ( !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
	}
	else
	{
		bool bViewIntersectsWater = DoesViewPlaneIntersectWater( fogInfo.m_flWaterHeight, fogInfo.m_nVisibleFogVolume );
		if( bViewIntersectsWater )
		{
			// have to draw both sides if we can see both.
			m_DrawFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
		}
		else if ( fogInfo.m_bEyeInFogVolume )
		{
			m_DrawFlags |= DF_RENDER_UNDERWATER;
		}
		else
		{
			m_DrawFlags |= DF_RENDER_ABOVEWATER;
		}
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}

	if ( !fogInfo.m_bEyeInFogVolume && bDrawSkybox )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	m_pCustomVisibility = pCustomVisibility;
	m_fogInfo = fogInfo;
}


//-----------------------------------------------------------------------------
// Draws the scene when there's no water or only cheap water
//-----------------------------------------------------------------------------
void CSimpleWorldView::Draw()
{
	VPROF( "CViewRender::ViewDrawScene_NoWater" );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	PIXEVENT( pRenderContext, "CSimpleWorldView::Draw" );

	pRenderContext.SafeRelease();

	DrawSetup( 0, m_DrawFlags, 0 );

	if ( !m_fogInfo.m_bEyeInFogVolume )
	{
		EnableWorldFog();
	}
	else
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;

		SetFogVolumeState( m_fogInfo, false );

		pRenderContext.GetFrom( g_pMaterialSystem );

		unsigned char ucFogColor[3];
		pRenderContext->GetFogColor( ucFogColor );
		pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
	}

	pRenderContext.SafeRelease();

	DrawExecute( 0, CurrentViewID(), 0 );

	pRenderContext.GetFrom( g_pMaterialSystem );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CalcWaterEyeAdjustments( const VisibleFogVolumeInfo_t &fogInfo,
											 float &newWaterHeight, float &waterZAdjust, bool bSoftwareUserClipPlane )
{
	if( !bSoftwareUserClipPlane )
	{
		newWaterHeight = fogInfo.m_flWaterHeight;
		waterZAdjust = 0.0f;
		return;
	}

	newWaterHeight = fogInfo.m_flWaterHeight;
	float eyeToWaterZDelta = origin[2] - fogInfo.m_flWaterHeight;
	float epsilon = r_eyewaterepsilon.GetFloat();
	waterZAdjust = 0.0f;
	if( fabs( eyeToWaterZDelta ) < epsilon )
	{
		if( eyeToWaterZDelta > 0 )
		{
			newWaterHeight = origin[2] - epsilon;
		}
		else
		{
			newWaterHeight = origin[2] + epsilon;
		}
		waterZAdjust = newWaterHeight - fogInfo.m_flWaterHeight;
	}

	//	Warning( "view.origin[2]: %f newWaterHeight: %f fogInfo.m_flWaterHeight: %f waterZAdjust: %f\n", 
	//		( float )view.origin[2], newWaterHeight, fogInfo.m_flWaterHeight, waterZAdjust );
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CSoftwareIntersectionView::Setup( bool bAboveWater )
{
	BaseClass::Setup( *GetOuter() );

	m_DrawFlags = ( bAboveWater ) ? DF_RENDER_UNDERWATER : DF_RENDER_ABOVEWATER;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseWaterView::CSoftwareIntersectionView::Draw()
{
	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );
	DrawExecute( GetOuter()->m_waterHeight, CurrentViewID(), GetOuter()->m_waterZAdjust );
}

//-----------------------------------------------------------------------------
// Draws the scene when the view point is above the level of the water
//-----------------------------------------------------------------------------
void CAboveWaterView::Setup( const CViewSetupEx &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	BaseClass::Setup( view );

	m_bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();

	CalcWaterEyeAdjustments( fogInfo, m_waterHeight, m_waterZAdjust, m_bSoftwareUserClipPlane );

	// BROKEN STUFF!
	if ( m_waterZAdjust == 0.0f )
	{
		m_bSoftwareUserClipPlane = false;
	}

	m_DrawFlags = DF_RENDER_ABOVEWATER | DF_DRAW_ENTITITES;
	m_ClearFlags = VIEW_CLEAR_DEPTH;

#ifdef PORTAL
	if( g_pPortalRender->ShouldObeyStencilForClears() )
		m_ClearFlags |= VIEW_CLEAR_OBEY_STENCIL;
#endif

	if ( bDrawSkybox )
	{
		m_DrawFlags |= DF_DRAWSKYBOX;
	}

	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_UNDERWATER;
	}

	m_fogInfo = fogInfo;
	m_waterInfo = waterInfo;
}

		 
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::Draw()
{
	VPROF( "CViewRender::ViewDrawScene_EyeAboveWater" );

	// eye is outside of water
	
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	
	// render the reflection
	if( m_waterInfo.m_bReflect )
	{
		m_ReflectionView.Setup( m_waterInfo.m_bReflectEntities );
		m_pMainView->AddViewToScene( &m_ReflectionView );
	}
	
	bool bViewIntersectsWater = false;

	// render refraction
	if ( m_waterInfo.m_bRefract )
	{
		m_RefractionView.Setup();
		m_pMainView->AddViewToScene( &m_RefractionView );

		if( !m_bSoftwareUserClipPlane )
		{
			bViewIntersectsWater = DoesViewPlaneIntersectWater( m_fogInfo.m_flWaterHeight, m_fogInfo.m_nVisibleFogVolume );
		}
	}
	else if ( !( m_DrawFlags & DF_DRAWSKYBOX ) )
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;
	}

#ifdef PORTAL
	if( g_pPortalRender->ShouldObeyStencilForClears() )
		m_ClearFlags |= VIEW_CLEAR_OBEY_STENCIL;
#endif

	// NOTE!!!!!  YOU CAN ONLY DO THIS IF YOU HAVE HARDWARE USER CLIP PLANES!!!!!!
	bool bHardwareUserClipPlanes = !g_pMaterialSystemHardwareConfig->UseFastClipping();
	if( bViewIntersectsWater && bHardwareUserClipPlanes )
	{
		// This is necessary to keep the non-water fogged world from drawing underwater in 
		// the case where we want to partially see into the water.
		m_DrawFlags |= DF_CLIP_Z | DF_CLIP_BELOW;
	}

	// render the world
	DrawSetup( m_waterHeight, m_DrawFlags, m_waterZAdjust );
	EnableWorldFog();
	DrawExecute( m_waterHeight, CurrentViewID(), m_waterZAdjust );

	if ( m_waterInfo.m_bRefract )
	{
		if ( m_bSoftwareUserClipPlane )
		{
			m_SoftwareIntersectionView.Setup( true );
			m_SoftwareIntersectionView.Draw( );
		}
		else if ( bViewIntersectsWater )
		{
			m_IntersectionView.Setup();
			m_pMainView->AddViewToScene( &m_IntersectionView );
		}
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CReflectionView::Setup( bool bReflectEntities )
{
	BaseClass::Setup( *GetOuter() );

	m_ClearFlags = VIEW_CLEAR_DEPTH;

	// NOTE: Clearing the color is unnecessary since we're drawing the skybox
	// and dest-alpha is never used in the reflection
	m_DrawFlags = DF_RENDER_REFLECTION | DF_CLIP_Z | DF_CLIP_BELOW | 
		DF_RENDER_ABOVEWATER;

	// NOTE: This will cause us to draw the 2d skybox in the reflection 
	// (which we want to do instead of drawing the 3d skybox)
	m_DrawFlags |= DF_DRAWSKYBOX;

	if( bReflectEntities )
	{
		m_DrawFlags |= DF_DRAW_ENTITITES;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CReflectionView::Draw()
{
#ifdef PORTAL
	g_pPortalRender->WaterRenderingHandler_PreReflection();
#endif

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFLECTION );

	// Disable occlusion visualization in reflection
	bool bVisOcclusion = r_visocclusion->GetBool();
	r_visocclusion->SetValue( 0 );

	DrawSetup( GetOuter()->m_fogInfo.m_flWaterHeight, m_DrawFlags, 0.0f, GetOuter()->m_fogInfo.m_nVisibleFogVolumeLeaf );

	EnableWorldFog();
	DrawExecute( GetOuter()->m_fogInfo.m_flWaterHeight, VIEW_REFLECTION, 0.0f );

	r_visocclusion->SetValue( bVisOcclusion );
	
#ifdef PORTAL
	// deal with stencil
	g_pPortalRender->WaterRenderingHandler_PostReflection();
#endif

	// finish off the view and restore the previous view.
	SetupCurrentView( origin, angles, ( view_id_t )nSaveViewID );

	// This is here for multithreading
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->Flush();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CRefractionView::Setup()
{
	BaseClass::Setup( *GetOuter() );

	m_ClearFlags = VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH;

	m_DrawFlags = DF_RENDER_REFRACTION | DF_CLIP_Z | 
		DF_RENDER_UNDERWATER | DF_FUDGE_UP | 
		DF_DRAW_ENTITITES ;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CRefractionView::Draw()
{
#ifdef PORTAL
	g_pPortalRender->WaterRenderingHandler_PreRefraction();
#endif

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFRACTION );

	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );

	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	SetClearColorToFogColor();
	DrawExecute( GetOuter()->m_waterHeight, VIEW_REFRACTION, GetOuter()->m_waterZAdjust );

#ifdef PORTAL
	// deal with stencil
	g_pPortalRender->WaterRenderingHandler_PostRefraction();
#endif

	// finish off the view.  restore the previous view.
	SetupCurrentView( origin, angles, ( view_id_t )nSaveViewID );

	// This is here for multithreading
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CIntersectionView::Setup()
{
	BaseClass::Setup( *GetOuter() );
	m_DrawFlags = DF_RENDER_UNDERWATER | DF_CLIP_Z | DF_DRAW_ENTITITES;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CAboveWaterView::CIntersectionView::Draw()
{
	DrawSetup( GetOuter()->m_fogInfo.m_flWaterHeight, m_DrawFlags, 0 );

	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	SetClearColorToFogColor( );
	DrawExecute( GetOuter()->m_fogInfo.m_flWaterHeight, VIEW_NONE, 0 );
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
}


//-----------------------------------------------------------------------------
// Draws the scene when the view point is under the level of the water
//-----------------------------------------------------------------------------
void CUnderWaterView::Setup( const CViewSetupEx &view, bool bDrawSkybox, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	BaseClass::Setup( view );

	m_bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();

	CalcWaterEyeAdjustments( fogInfo, m_waterHeight, m_waterZAdjust, m_bSoftwareUserClipPlane );

	IMaterial *pWaterMaterial = fogInfo.m_pFogVolumeMaterial;
	if (engine->GetDXSupportLevel() >= 90 )					// screen voerlays underwater are a dx9 feature
	{
		IMaterialVar *pScreenOverlayVar = pWaterMaterial->FindVar( "$underwateroverlay", NULL, false );
		if ( pScreenOverlayVar && ( pScreenOverlayVar->IsDefined() ) )
		{
			char const *pOverlayName = pScreenOverlayVar->GetStringValue();
			if ( pOverlayName[0] != '0' )						// fixme!!!
			{
				IMaterial *pOverlayMaterial = g_pMaterialSystem->FindMaterial( pOverlayName,  TEXTURE_GROUP_OTHER );
				m_pMainView->SetWaterOverlayMaterial( pOverlayMaterial );
			}
		}
	}
	// NOTE: We're not drawing the 2d skybox under water since it's assumed to not be visible.

	// render the world underwater
	// Clear the color to get the appropriate underwater fog color
	m_DrawFlags = DF_FUDGE_UP | DF_RENDER_UNDERWATER | DF_DRAW_ENTITITES;
	m_ClearFlags = VIEW_CLEAR_DEPTH;

	if( !m_bSoftwareUserClipPlane )
	{
		m_DrawFlags |= DF_CLIP_Z;
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		m_DrawFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		m_DrawFlags |= DF_RENDER_ABOVEWATER;
	}

	m_fogInfo = fogInfo;
	m_waterInfo = waterInfo;
	m_bDrawSkybox = bDrawSkybox;
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::Draw()
{
	// FIXME: The 3d skybox shouldn't be drawn when the eye is under water

	VPROF( "CViewRender::ViewDrawScene_EyeUnderWater" );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	// render refraction (out of water)
	if ( m_waterInfo.m_bRefract )
	{
		m_RefractionView.Setup( );
		m_pMainView->AddViewToScene( &m_RefractionView );
	}

	if ( !m_waterInfo.m_bRefract )
	{
		SetFogVolumeState( m_fogInfo, true );
		unsigned char ucFogColor[3];
		pRenderContext->GetFogColor( ucFogColor );
		pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
	}

	DrawSetup( m_waterHeight, m_DrawFlags, m_waterZAdjust );
	SetFogVolumeState( m_fogInfo, false );
	DrawExecute( m_waterHeight, CurrentViewID(), m_waterZAdjust );
	m_ClearFlags = 0;

	if( m_waterZAdjust != 0.0f && m_bSoftwareUserClipPlane && m_waterInfo.m_bRefract )
	{
		m_SoftwareIntersectionView.Setup( false );
		m_SoftwareIntersectionView.Draw( );
	}
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );

}



//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::CRefractionView::Setup()
{
	BaseClass::Setup( *GetOuter() );
	// NOTE: Refraction renders into the back buffer, over the top of the 3D skybox
	// It is then blitted out into the refraction target. This is so that
	// we only have to set up 3d sky vis once, and only render it once also!
	m_DrawFlags = DF_CLIP_Z | 
		DF_CLIP_BELOW | DF_RENDER_ABOVEWATER | 
		DF_DRAW_ENTITITES;

	m_ClearFlags = VIEW_CLEAR_DEPTH;
	if ( GetOuter()->m_bDrawSkybox )
	{
		m_ClearFlags |= VIEW_CLEAR_COLOR;
		m_DrawFlags |= DF_DRAWSKYBOX | DF_CLIP_SKYBOX;
	}
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CUnderWaterView::CRefractionView::Draw()
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	SetFogVolumeState( GetOuter()->m_fogInfo, true );
	unsigned char ucFogColor[3];
	pRenderContext->GetFogColor( ucFogColor );
	pRenderContext->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );

	DrawSetup( GetOuter()->m_waterHeight, m_DrawFlags, GetOuter()->m_waterZAdjust );

	EnableWorldFog();
	DrawExecute( GetOuter()->m_waterHeight, VIEW_REFRACTION, GetOuter()->m_waterZAdjust );

	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = width;
	srcRect.height = height;

	// Optionally write the rendered image to a debug texture
	if ( g_bDumpRenderTargets )
	{
		DumpTGAofRenderTarget( width, height, "WaterRefract" );
	}

	ITexture *pTexture = GetWaterRefractionTexture();
	pRenderContext->CopyRenderTargetToTextureEx( pTexture, 0, &srcRect, NULL );
}


//-----------------------------------------------------------------------------
//
// Reflective glass view starts here
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Draws the scene when the view contains reflective glass
//-----------------------------------------------------------------------------
void CReflectiveGlassView::Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, 
	const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane )
{
	BaseClass::Setup( view, nClearFlags, bDrawSkybox, fogInfo, waterInfo, NULL );
	m_ReflectionPlane = reflectionPlane;
}


bool CReflectiveGlassView::AdjustView( float flWaterHeight )
{
	ITexture *pTexture = GetReflectionTexture();
		   
	// Use the aspect ratio of the main view! So, don't recompute it here
	x = y = 0;
	width = pTexture->GetActualWidth();
	height = pTexture->GetActualHeight();

	// Reflect the camera origin + vectors around the reflection plane 
	float flDist = DotProduct( origin, m_ReflectionPlane.normal ) - m_ReflectionPlane.dist;
	VectorMA( origin, - 2.0f * flDist, m_ReflectionPlane.normal, origin );

	Vector vecForward, vecUp;
	AngleVectors( angles, &vecForward, NULL, &vecUp );

	float flDot = DotProduct( vecForward, m_ReflectionPlane.normal );
	VectorMA( vecForward, - 2.0f * flDot, m_ReflectionPlane.normal, vecForward );

	flDot = DotProduct( vecUp, m_ReflectionPlane.normal );
	VectorMA( vecUp, - 2.0f * flDot, m_ReflectionPlane.normal, vecUp );

	VectorAngles( vecForward, vecUp, angles );
	return true;
}

void CReflectiveGlassView::PushView( float waterHeight )
{
	render->Push3DView( *this, m_ClearFlags, GetReflectionTexture(), GetFrustum() );
	 
	Vector4D plane;
	VectorCopy( m_ReflectionPlane.normal, plane.AsVector3D() );
	plane.w = m_ReflectionPlane.dist + 0.1f;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PushCustomClipPlane( plane.Base() );
}

void CReflectiveGlassView::PopView( )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PopCustomClipPlane( );
	render->PopView( GetFrustum() );
}


//-----------------------------------------------------------------------------
// Renders reflective or refractive parts of glass
//-----------------------------------------------------------------------------
void CReflectiveGlassView::Draw()
{
	VPROF( "CReflectiveGlassView::Draw" );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	PIXEVENT( pRenderContext, "CReflectiveGlassView::Draw" );

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFLECTION );

	// Disable occlusion visualization in reflection
	bool bVisOcclusion = r_visocclusion->GetBool();
	r_visocclusion->SetValue( 0 );
				   

	int lastView = g_CurrentViewID;
	g_CurrentViewID = VIEW_REFLECTION;
	BaseClass::Draw();
	g_CurrentViewID = lastView;

	r_visocclusion->SetValue( bVisOcclusion );

	// finish off the view.  restore the previous view.
	SetupCurrentView( origin, angles, (view_id_t)nSaveViewID );

	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}



//-----------------------------------------------------------------------------
// Draws the scene when the view contains reflective glass
//-----------------------------------------------------------------------------
void CRefractiveGlassView::Setup( const CViewSetupEx &view, int nClearFlags, bool bDrawSkybox, 
	const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t &waterInfo, const cplane_t &reflectionPlane )
{
	BaseClass::Setup( view, nClearFlags, bDrawSkybox, fogInfo, waterInfo, NULL );
	m_ReflectionPlane = reflectionPlane;
}


bool CRefractiveGlassView::AdjustView( float flWaterHeight )
{
	ITexture *pTexture = GetRefractionTexture();

	// Use the aspect ratio of the main view! So, don't recompute it here
	x = y = 0;
	width = pTexture->GetActualWidth();
	height = pTexture->GetActualHeight();
	return true;
}


void CRefractiveGlassView::PushView( float waterHeight )
{
	render->Push3DView( *this, m_ClearFlags, GetRefractionTexture(), GetFrustum() );

	Vector4D plane;
	VectorMultiply( m_ReflectionPlane.normal, -1, plane.AsVector3D() );
	plane.w = -m_ReflectionPlane.dist + 0.1f;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PushCustomClipPlane( plane.Base() );
}


void CRefractiveGlassView::PopView( )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->PopCustomClipPlane( );
	render->PopView( GetFrustum() );
}



//-----------------------------------------------------------------------------
// Renders reflective or refractive parts of glass
//-----------------------------------------------------------------------------
void CRefractiveGlassView::Draw()
{
	VPROF( "CRefractiveGlassView::Draw" );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	PIXEVENT( pRenderContext, "CRefractiveGlassView::Draw" );

	// Store off view origin and angles and set the new view
	int nSaveViewID = CurrentViewID();
	SetupCurrentView( origin, angles, VIEW_REFRACTION );

	BaseClass::Draw();

	// finish off the view.  restore the previous view.
	SetupCurrentView( origin, angles, (view_id_t)nSaveViewID );

	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->Flush();
}
