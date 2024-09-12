//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "iviewrender_beams.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_te_legacytempents.h"
#include "cl_mat_stub.h"
#include "tier0/vprof.h"
#include "iclientvehicle.h"
#include "engine/IEngineTrace.h"
#include "mathlib/vmatrix.h"
#include "rendertexture.h"
#include "c_world.h"
#include <KeyValues.h>
#include "igameevents.h"
#include "smoke_fog_overlay.h"
#include "bitmap/tgawriter.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/replay_screenshot.h"
#endif
#include "input.h"
#include "filesystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "toolframework_client.h"
#include "tier0/icommandline.h"
#include "ienginevgui.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "ScreenSpaceEffects.h"
#include "vgui_int.h"

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

#ifdef PORTAL
#include "c_prop_portal.h" //portal surface rendering functions
#endif

	
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
		  
void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height );
bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );
bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles );


extern ConVar default_fov;
extern bool g_bRenderingScreenshot;

#define SAVEGAME_SCREENSHOT_WIDTH	180
#define SAVEGAME_SCREENSHOT_HEIGHT	100

extern ConVar sensitivity;

ConVar zoom_sensitivity_ratio( "zoom_sensitivity_ratio", "1.0", 0, "Additional mouse sensitivity scale factor applied when FOV is zoomed in." );

CViewRender s_DefaultViewRender;

#if _DEBUG
bool g_bRenderingCameraView = false;
#endif


// These are the vectors for the "main" view - the one the player is looking down.
// For stereo views, they are the vectors for the middle eye.
static Vector g_vecRenderOrigin(0,0,0);
static QAngle g_vecRenderAngles(0,0,0);
static Vector g_vecPrevRenderOrigin(0,0,0);	// Last frame's render origin
static QAngle g_vecPrevRenderAngles(0,0,0); // Last frame's render angles
static Vector g_vecVForward(0,0,0), g_vecVRight(0,0,0), g_vecVUp(0,0,0);
static VMatrix g_matCamInverse;

extern ConVar cl_forwardspeed;

static ConVar v_centermove( "v_centermove", "0.15");
static ConVar v_centerspeed( "v_centerspeed","500" );

ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_CHEAT, "Sets the field-of-view for the viewmodel.", true, 0.1, true, 179.9 );
ConVar v_viewmodel_fov_script_override( "viewmodel_fov_script_override", "0", FCVAR_NONE, "If nonzero, overrides the viewmodel FOV of weapon scripts which override the viewmodel FOV." );
ConVar mat_viewportscale( "mat_viewportscale", "1.0", FCVAR_ARCHIVE, "Scale down the main viewport (to reduce GPU impact on CPU profiling)", true, (1.0f / 640.0f), true, 1.0f );
ConVar mat_viewportupscale( "mat_viewportupscale", "1", FCVAR_ARCHIVE, "Scale the viewport back up" );
ConVar cl_leveloverview( "cl_leveloverview", "0", FCVAR_CHEAT );

static ConVar cl_camera_follow_bone_index( "cl_camera_follow_bone_index"  , "-2", FCVAR_CHEAT, "Index of the bone to follow.  -2 == disabled.  -1 == root bone.  0+ is bone index." );
Vector g_cameraFollowPos;

ConVar r_mapextents( "r_mapextents", "16384", FCVAR_CHEAT, 
						   "Set the max dimension for the map.  This determines the far clipping plane" );

// UNDONE: Delete this or move to the material system?
ConVar	gl_clear( "gl_clear", "0");
ConVar	gl_clear_randomcolor( "gl_clear_randomcolor", "0", FCVAR_CHEAT, "Clear the back buffer to random colors every frame. Helps spot open seams in geometry." );

static ConVar r_farz( "r_farz", "-1", FCVAR_CHEAT, "Override the far clipping plane. -1 means to use the value in env_fog_controller." );
static ConVar cl_demoviewoverride( "cl_demoviewoverride", "0", 0, "Override view during demo playback" );
static ConVar r_nearz( "r_nearz", "-1", FCVAR_CHEAT, "Override the near clipping plane. -1 means to use the default value (usually 7)." );


void SoftwareCursorChangedCB( IConVar *pVar, const char *pOldValue, float fOldValue )
{
	ConVar *pConVar = (ConVar *)pVar;
	vgui::surface()->SetSoftwareCursor( pConVar->GetBool() );
}
static ConVar cl_software_cursor ( "cl_software_cursor", "0", FCVAR_ARCHIVE, "Switches the game to use a larger software cursor instead of the normal OS cursor", SoftwareCursorChangedCB );


static Vector s_DemoView;
static QAngle s_DemoAngle;

static void CalcDemoViewOverride( Vector &origin, QAngle &angles )
{
	engine->SetViewAngles( s_DemoAngle );

	input->ExtraMouseSample( gpGlobals->absoluteframetime, true );

	engine->GetViewAngles( s_DemoAngle );

	Vector forward, right, up;

	AngleVectors( s_DemoAngle, &forward, &right, &up );

	float speed = gpGlobals->absoluteframetime * cl_demoviewoverride.GetFloat() * 320;
	
	s_DemoView += speed * input->KeyState (&in_forward) * forward  ;
	s_DemoView -= speed * input->KeyState (&in_back) * forward ;

	s_DemoView += speed * input->KeyState (&in_moveright) * right ;
	s_DemoView -= speed * input->KeyState (&in_moveleft) * right ;

	origin = s_DemoView;
	angles = s_DemoAngle;
}



// Selects the relevant member variable to update. You could do it manually, but...
// We always set up the MONO eye, even when doing stereo, and it's set up to be mid-way between the left and right,
// so if you don't really care about L/R (e.g. culling, sound, etc), just use MONO.
CViewSetupEx &CViewRender::GetView()
{
	return m_View;
}

const CViewSetupEx &CViewRender::GetView() const
{
    return (const_cast<CViewRender*>(this))->GetView();
}


//-----------------------------------------------------------------------------
// Accessors to return the main view (where the player's looking)
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin()
{
	return g_vecRenderOrigin;
}

const QAngle &MainViewAngles()
{
	return g_vecRenderAngles;
}

const Vector &MainViewForward()
{
	return g_vecVForward;
}

const Vector &MainViewRight()
{
	return g_vecVRight;
}

const Vector &MainViewUp()
{
	return g_vecVUp;
}

const VMatrix &MainWorldToViewMatrix()
{
	return g_matCamInverse;
}

const Vector &PrevMainViewOrigin()
{
	return g_vecPrevRenderOrigin;
}

const QAngle &PrevMainViewAngles()
{
	return g_vecPrevRenderAngles;
}

//-----------------------------------------------------------------------------
// Compute the world->camera transform
//-----------------------------------------------------------------------------
void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse )
{
	// Compute view bases
	AngleVectors( vecAngles, pVecForward, pVecRight, pVecUp );

	for (int i = 0; i < 3; ++i)
	{
		(*pMatCamInverse)[0][i] = (*pVecRight)[i];	
		(*pMatCamInverse)[1][i] = (*pVecUp)[i];	
		(*pMatCamInverse)[2][i] = -(*pVecForward)[i];	
		(*pMatCamInverse)[3][i] = 0.0F;
	}
	(*pMatCamInverse)[0][3] = -DotProduct( *pVecRight, vecOrigin );
	(*pMatCamInverse)[1][3] = -DotProduct( *pVecUp, vecOrigin );
	(*pMatCamInverse)[2][3] =  DotProduct( *pVecForward, vecOrigin );
	(*pMatCamInverse)[3][3] = 1.0F;
}


bool R_CullSphere(
	VPlane const *pPlanes,
	int nPlanes,
	Vector const *pCenter,
	float radius)
{
	for(int i=0; i < nPlanes; i++)
		if(pPlanes[i].DistTo(*pCenter) < -radius)
			return true;
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void StartPitchDrift( void )
{
	GetViewRenderInstance()->StartPitchDrift();
}

static ConCommand centerview( "centerview", StartPitchDrift );

//-----------------------------------------------------------------------------
// Purpose: Initializes all view systems
//-----------------------------------------------------------------------------
void CViewRender::Init( void )
{
	memset( &m_PitchDrift, 0, sizeof( m_PitchDrift ) );

	m_bDrawOverlay = false;

	m_pDrawEntities		= cvar->FindVar( "r_drawentities" );
	m_pDrawBrushModels	= cvar->FindVar( "r_drawbrushmodels" );

	beams->InitBeams();
	tempents->Init();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
	m_ModulateSingleColor.Init( "engine/modulatesinglecolor", TEXTURE_GROUP_OTHER );
	m_WhiteMaterial.Init( "vgui/white", TEXTURE_GROUP_OTHER );
	
	extern CMaterialReference g_material_WriteZ;
	g_material_WriteZ.Init( "engine/writez", TEXTURE_GROUP_OTHER );

	g_vecRenderOrigin.Init();
	g_vecRenderAngles.Init();
	g_vecPrevRenderOrigin.Init();
	g_vecPrevRenderAngles.Init();
	g_vecVForward.Init();
	g_vecVRight.Init();
	g_vecVUp.Init();
	g_matCamInverse.Identity();

	// FIXME:  
	QAngle angles;
	engine->GetViewAngles( angles );
	AngleVectors( angles, &m_vecLastFacing );

#if defined( REPLAY_ENABLED )
	m_pReplayScreenshotTaker = NULL;
#endif

	m_flLastFOV = default_fov.GetFloat();

}

CMaterialReference &CViewRender::GetWhite()
{
	return m_WhiteMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelInit( void )
{
	beams->ClearBeams();
	tempents->Clear();

	m_BuildWorldListsNumber = 0;
	m_BuildRenderableListsNumber = 0;

	m_rbTakeFreezeFrame = false;
	m_flFreezeFrameUntil = 0;

	// Clear our overlay materials
	m_ScreenOverlayMaterial.Init( NULL );

	// Init all IScreenSpaceEffects
	g_pScreenSpaceEffects->InitScreenSpaceEffects( );

	InitFadeData();
}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelShutdown( void )
{
	g_pScreenSpaceEffects->ShutdownScreenSpaceEffects( );
}

//-----------------------------------------------------------------------------
// Purpose: Called at shutdown
//-----------------------------------------------------------------------------
void CViewRender::Shutdown( void )
{
	m_TranslucentSingleColor.Shutdown( );
	m_ModulateSingleColor.Shutdown( );
	m_ScreenOverlayMaterial.Shutdown();
	m_UnderWaterOverlayMaterial.Shutdown();
	m_WhiteMaterial.Shutdown();
	beams->ShutdownBeams();
	tempents->Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the worldlists build number
//-----------------------------------------------------------------------------

int CViewRender::BuildWorldListsNumber( void ) const
{
	return m_BuildWorldListsNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Start moving pitch toward ideal
//-----------------------------------------------------------------------------
void CViewRender::StartPitchDrift (void)
{
	if ( m_PitchDrift.laststop == gpGlobals->curtime )
	{
		// Something else is blocking the drift.
		return;		
	}

	if ( m_PitchDrift.nodrift || !m_PitchDrift.pitchvel )
	{
		m_PitchDrift.pitchvel	= v_centerspeed.GetFloat();
		m_PitchDrift.nodrift	= false;
		m_PitchDrift.driftmove	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::StopPitchDrift (void)
{
	m_PitchDrift.laststop	= gpGlobals->curtime;
	m_PitchDrift.nodrift	= true;
	m_PitchDrift.pitchvel	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the client pitch angle towards cl.idealpitch sent by the server.
// If the user is adjusting pitch manually, either with lookup/lookdown,
//   mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//-----------------------------------------------------------------------------
void CViewRender::DriftPitch (void)
{
	float		delta, move;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

#if defined( REPLAY_ENABLED )
	if ( engine->IsHLTV() || g_pEngineClientReplay->IsPlayingReplayDemo() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#else
	if ( engine->IsHLTV() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
#endif
	{
		m_PitchDrift.driftmove = 0;
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Don't count small mouse motion
	if ( m_PitchDrift.nodrift )
	{
		if ( fabs( input->GetLastForwardMove() ) < cl_forwardspeed.GetFloat() )
		{
			m_PitchDrift.driftmove = 0;
		}
		else
		{
			m_PitchDrift.driftmove += gpGlobals->frametime;
		}
	
		if ( m_PitchDrift.driftmove > v_centermove.GetFloat() )
		{
			StartPitchDrift ();
		}
		return;
	}
	
	// How far off are we
	delta = prediction->GetIdealPitch() - player->GetAbsAngles()[ PITCH ];
	if ( !delta )
	{
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Determine movement amount
	move = gpGlobals->frametime * m_PitchDrift.pitchvel;
	// Accelerate
	m_PitchDrift.pitchvel += gpGlobals->frametime * v_centerspeed.GetFloat();
	
	// Move predicted pitch appropriately
	if (delta > 0)
	{
		if ( move > delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() + QAngle( move, 0, 0 ) );
	}
	else if ( delta < 0 )
	{
		if ( move > -delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = -delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() - QAngle( move, 0, 0 ) );
	}
}

// This is called by cdll_client_int to setup view model origins. This has to be done before
// simulation so entities can access attachment points on view models during simulation.
void CViewRender::OnRenderStart()
{
	VPROF_("CViewRender::OnRenderStart", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);

    SetUpViews();

	// Adjust mouse sensitivity based upon the current FOV
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player )
	{
		default_fov.SetValue( player->m_iDefaultFOV );

		//Update our FOV, including any zooms going on
		int iDefaultFOV = default_fov.GetInt();
		int	localFOV	= player->GetFOV();
		int min_fov		= player->GetMinFOV();

		// Don't let it go too low
		localFOV = MAX( min_fov, localFOV );

		GetHud().m_flFOVSensitivityAdjust = 1.0f;
		if ( GetHud().m_flMouseSensitivityFactor )
		{
			GetHud().m_flMouseSensitivity = sensitivity.GetFloat() * GetHud().m_flMouseSensitivityFactor;
		}
		else
		{
			// No override, don't use huge sensitivity
			if ( localFOV == iDefaultFOV )
			{
				// reset to saved sensitivity
				GetHud().m_flMouseSensitivity = 0;
			}
			else
			{  
				// Set a new sensitivity that is proportional to the change from the FOV default and scaled
				//  by a separate compensating factor
				if ( iDefaultFOV == 0 )
				{
					Assert(0); // would divide by zero, something is broken with iDefatulFOV
					iDefaultFOV = 1;
				}
				GetHud().m_flFOVSensitivityAdjust = 
					((float)localFOV / (float)iDefaultFOV) * // linear fov downscale
					zoom_sensitivity_ratio.GetFloat(); // sensitivity scale factor
				GetHud().m_flMouseSensitivity = GetHud().m_flFOVSensitivityAdjust * sensitivity.GetFloat(); // regular sensitivity
			}
		}
	}

	// Setup the frustum cache for this frame.
	m_bAllowViewAccess = true;
	const CViewSetupEx &view = GetView();
	FrustumCache()->Add( &view );
	FrustumCache()->SetUpdated();
	m_bAllowViewAccess = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetupEx *CViewRender::GetViewSetup( void ) const
{
	return &m_CurrentView;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetupEx *CViewRender::GetPlayerViewSetup( void ) const
{   
    const CViewSetupEx &view = GetView();
    return &view;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DisableVis( void )
{
	m_bForceNoVis = true;
}

#ifdef DBGFLAG_ASSERT
static Vector s_DbgSetupOrigin;
static QAngle s_DbgSetupAngles;
#endif

//-----------------------------------------------------------------------------
// Gets znear + zfar
//-----------------------------------------------------------------------------
float CViewRender::GetZNear()
{
	if (r_nearz.GetFloat() > 0)
		return r_nearz.GetFloat();

	return VIEW_NEARZ;
}

float CViewRender::GetZFar()
{
	// Initialize view structure with default values
	float farZ;
	if ( r_farz.GetFloat() < 1 )
	{
		// Use the far Z from the map's parameters.
		farZ = r_mapextents.GetFloat() * 1.73205080757f;
		
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer && pPlayer->GetFogParams() )
		{
			if ( pPlayer->GetFogParams()->farz > 0 )
			{
				farZ = pPlayer->GetFogParams()->farz;
			}
		}
	}
	else
	{
		farZ = r_farz.GetFloat();
	}

	return farZ;
}


//-----------------------------------------------------------------------------
// Sets up the view parameters
//-----------------------------------------------------------------------------
void CViewRender::SetUpViews()
{
	VPROF("CViewRender::SetUpViews");

	m_bAllowViewAccess = true;

	// Initialize view structure with default values
	float farZ = GetZFar();

    // Set up the mono/middle view.
    CViewSetupEx &view = m_View;

	view.zFar				= farZ;
	view.zFarViewmodel	    = farZ;
	// UNDONE: Make this farther out? 
	//  closest point of approach seems to be view center to top of crouched box
	view.zNear			    = GetZNear();
	view.zNearViewmodel	    = 1;
	view.fov				= default_fov.GetFloat();

	view.m_bOrtho			= false;
    view.m_bViewToProjectionOverride = false;

	// Enable spatial partition access to edicts
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	bool bNoViewEnt = false;
	if( pPlayer == NULL )
	{
		bNoViewEnt = true;
	}

	// You in-view weapon aim.
	bool bCalcViewModelView = false;
	Vector ViewModelOrigin;
	QAngle ViewModelAngles;

	view.fovViewmodel = GetClientMode()->GetViewModelFOV();

	if ( engine->IsHLTV() )
	{
		HLTVCamera()->CalcView( view.origin, view.angles, view.fov );
	}
#if defined( REPLAY_ENABLED )
	else if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
	{
		ReplayCamera()->CalcView( view.origin, view.angles, view.fov );
	}
#endif
	else
	{
		// FIXME: Are there multiple views? If so, then what?
		// FIXME: What happens when there's no player?
		if (pPlayer)
		{
			pPlayer->CalcView( view.origin, view.angles, view.zNear, view.zFar, view.fov );

			// If we are looking through another entities eyes, then override the angles/origin for view
			int viewentity = render->GetViewEntity();

			if ( !g_nKillCamMode && (pPlayer->entindex() != viewentity) )
			{
				C_BaseEntity *ve = cl_entitylist->GetEnt( viewentity );
				if ( ve )
				{
					VectorCopy( ve->GetAbsOrigin(), view.origin );
					VectorCopy( ve->GetAbsAngles(), view.angles );
				}
			}

			// There is a viewmodel.
			bCalcViewModelView = true;
			ViewModelOrigin = view.origin;
			ViewModelAngles = view.angles;

			// Allow weapons to override viewmodel FOV
			C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
			if (pWeapon && pWeapon->GetViewmodelFOVOverride() != 0.0f)
			{
				if (v_viewmodel_fov_script_override.GetFloat() > 0.0f)
					view.fovViewmodel = v_viewmodel_fov_script_override.GetFloat();
				else
					view.fovViewmodel = pWeapon->GetViewmodelFOVOverride();
			}
		}
		else
		{
			view.origin.Init();
			view.angles.Init();
		}

		// Even if the engine is paused need to override the view
		// for keeping the camera control during pause.
		GetClientMode()->OverrideView( &view );
	}

	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( view.origin, view.angles, view.fov );

	if ( engine->IsPlayingDemo() )
	{
		if ( cl_demoviewoverride.GetFloat() > 0.0f )
		{
			// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
			CalcDemoViewOverride( view.origin, view.angles );
		}
		else
		{
			s_DemoView = view.origin;
			s_DemoAngle = view.angles;
		}
	}

	//Find the offset our current FOV is from the default value
	float fDefaultFov = default_fov.GetFloat();
	float flFOVOffset = fDefaultFov - view.fov;

	//Adjust the viewmodel's FOV to move with any FOV offsets on the viewer's end
	view.fovViewmodel = max(0.001f, view.fovViewmodel - flFOVOffset);

	if ( bCalcViewModelView )
	{
		Assert ( pPlayer != NULL );
		pPlayer->CalcViewModelView ( ViewModelOrigin, ViewModelAngles );
	}

	// Is this the proper place for this code?
	if ( cl_camera_follow_bone_index.GetInt() >= -1 && input->CAM_IsThirdPerson() )
	{
		VectorCopy( g_cameraFollowPos, view.origin );
	}

	// Disable spatial partition access
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

	// Compute the world->main camera transform
    // This is only done for the main "middle-eye" view, not for the various other views.
	ComputeCameraVariables( view.origin, view.angles, 
		&g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

	// set up the hearing origin...
	AudioState_t audioState;
	audioState.m_Origin = view.origin;
	audioState.m_Angles = view.angles;
	audioState.m_bIsUnderwater = pPlayer && pPlayer->AudioStateIsUnderwater( view.origin );

	ToolFramework_SetupAudioState( audioState );

    // TomF: I wonder when the audio tools modify this, if ever...
    Assert ( view.origin == audioState.m_Origin );
    Assert ( view.angles == audioState.m_Angles );
	view.origin = audioState.m_Origin;
	view.angles = audioState.m_Angles;

	GetClientMode()->OverrideAudioState( &audioState );
	engine->SetAudioState( audioState );

	g_vecPrevRenderOrigin = g_vecRenderOrigin;
	g_vecPrevRenderAngles = g_vecRenderAngles;
	g_vecRenderOrigin = view.origin;
	g_vecRenderAngles = view.angles;

#ifdef DBGFLAG_ASSERT
	s_DbgSetupOrigin = view.origin;
	s_DbgSetupAngles = view.angles;
#endif

	m_bAllowViewAccess = false;
}


//-----------------------------------------------------------------------------
// Purpose: takes a screenshot for the replay system
//-----------------------------------------------------------------------------
void CViewRender::WriteReplayScreenshot( WriteReplayScreenshotParams_t &params )
{
#if defined( REPLAY_ENABLED )
	if ( !m_pReplayScreenshotTaker )
		return;

	m_pReplayScreenshotTaker->TakeScreenshot( params );
#endif
}

void CViewRender::UpdateReplayScreenshotCache()
{
#if defined( REPLAY_ENABLED )
	// Delete the old one
	delete m_pReplayScreenshotTaker;

	// Create a new one
	m_pReplayScreenshotTaker = new CReplayScreenshotTaker( this, GetView ( STEREO_EYE_MONO ) );
#endif
}

float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets view parameters for level overview mode
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::SetUpOverView()
{
	static int oldCRC = 0;

    CViewSetupEx &view = GetView();

	view.m_bOrtho = true;

	float aspect = (float)view.width/(float)view.height;

	int size_y = 1024.0f * cl_leveloverview.GetFloat(); // scale factor, 1024 = OVERVIEW_MAP_SIZE
	int	size_x = size_y * aspect;	// standard screen aspect 

	view.origin.x -= size_x / 2;
	view.origin.y += size_y / 2;

	view.m_OrthoLeft   = 0;
	view.m_OrthoTop    = -size_y;
	view.m_OrthoRight  = size_x;
	view.m_OrthoBottom = 0;

	view.angles = QAngle( 90, 90, 0 );

	// simple movement detector, show position if moved
	int newCRC = view.origin.x + view.origin.y + view.origin.z;
	if ( newCRC != oldCRC )
	{
		Msg( "Overview: scale %.2f, pos_x %.0f, pos_y %.0f\n", cl_leveloverview.GetFloat(),
			view.origin.x, view.origin.y );
		oldCRC = newCRC;
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->ClearColor4ub( 0, 255, 0, 255 );

	// render->DrawTopView( true );
}

//-----------------------------------------------------------------------------
// Purpose: Render current view into specified rectangle
// Input  : *rect - is computed by CVideoMode_Common::GetClientViewRect()
//-----------------------------------------------------------------------------
void CViewRender::Render( vrect_t *rect )
{
	Assert(s_DbgSetupOrigin == m_View.origin);
	Assert(s_DbgSetupAngles == m_View.angles);

	VPROF_BUDGET( "CViewRender::Render", "CViewRender::Render" );
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	m_bAllowViewAccess = true;

	vrect_t vr = *rect;

	// Stub out the material system if necessary.
	CMatStubHandler matStub;

	engine->EngineStats_BeginFrame();
	
	// Assume normal vis
	m_bForceNoVis			= false;
	
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();


    // Set for console commands, etc.
    render->SetMainView ( m_View.origin, m_View.angles );

	CViewSetupEx &view = GetView();

#ifdef REPLAY_ENABLED
	const bool bPlayingBackReplay = g_pEngineClientReplay && g_pEngineClientReplay->IsPlayingReplayDemo();
	if ( pPlayer && !bPlayingBackReplay )
#else
	if ( pPlayer )
#endif
	{
		C_BasePlayer *pViewTarget = pPlayer;

		if ( pPlayer->IsObserver() && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		{
			pViewTarget = dynamic_cast<C_BasePlayer*>( pPlayer->GetObserverTarget() );
		}

		if ( pViewTarget )
		{
			float targetFOV = (float)pViewTarget->m_iFOV;

			if ( targetFOV == 0 )
			{
				// FOV of 0 means use the default FOV
				targetFOV = GameRules()->DefaultFOV();
			}

			float deltaFOV = view.fov - m_flLastFOV;
			float FOVDirection = targetFOV - pViewTarget->m_iFOVStart;

			// Clamp FOV changes to stop FOV oscillation
			if ( ( deltaFOV < 0.0f && FOVDirection > 0.0f ) ||
				( deltaFOV > 0.0f && FOVDirection < 0.0f ) )
			{
				view.fov = m_flLastFOV;
			}

			// Catch case where FOV overshoots its target FOV
			if ( ( view.fov < targetFOV && FOVDirection <= 0.0f ) ||
				( view.fov > targetFOV && FOVDirection >= 0.0f ) )
			{
				view.fov = targetFOV;
			}

			m_flLastFOV = view.fov;
		}
	}

    static ConVarRef sv_restrict_aspect_ratio_fov( "sv_restrict_aspect_ratio_fov" );
    float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
    float limitedAspectRatio = aspectRatio;
    if ( ( sv_restrict_aspect_ratio_fov.GetInt() > 0 && engine->IsWindowedMode() && gpGlobals->maxClients > 1 ) ||
	    sv_restrict_aspect_ratio_fov.GetInt() == 2 )
    {
	    limitedAspectRatio = MIN( aspectRatio, 1.85f * 0.75f ); // cap out the FOV advantage at a 1.85:1 ratio (about the widest any legit user should be)
    }

    view.fov = ScaleFOVByWidthRatio( view.fov, limitedAspectRatio );
    view.fovViewmodel = ScaleFOVByWidthRatio( view.fovViewmodel, aspectRatio );

    // Let the client mode hook stuff.
    GetClientMode()->PreRender(&view);

    GetClientMode()->AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

    ToolFramework_AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );

    float flViewportScale = mat_viewportscale.GetFloat();

	view.m_nUnscaledX = vr.x;
	view.m_nUnscaledY = vr.y;
	view.m_nUnscaledWidth = vr.width;
	view.m_nUnscaledHeight = vr.height;

#if 0
    // Good test mode for debugging viewports that are not full-size.
    view.width			= vr.width * flViewportScale * 0.75f;
    view.height			= vr.height * flViewportScale * 0.75f;
    view.x				= vr.x + view.width * 0.10f;
    view.y				= vr.y + view.height * 0.20f;
#else
    view.x				= vr.x * flViewportScale;
	view.y				= vr.y * flViewportScale;
	view.width			= vr.width * flViewportScale;
	view.height			= vr.height * flViewportScale;
#endif
    float engineAspectRatio = engine->GetScreenAspectRatio();
    view.m_flAspectRatio	= ( engineAspectRatio > 0.0f ) ? engineAspectRatio : ( (float)view.width / (float)view.height );

	// if we still don't have an aspect ratio, compute it from the view size
	if( view.m_flAspectRatio <= 0.f )
	    view.m_flAspectRatio	= (float)view.width / (float)view.height;

    int nClearFlags = VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL;

    if( gl_clear_randomcolor.GetBool() )
    {
	    CMatRenderContextPtr pRenderContext( materials );
	    pRenderContext->ClearColor3ub( rand()%256, rand()%256, rand()%256 );
	    pRenderContext->ClearBuffers( true, false, false );
	    pRenderContext->Release();
    }
    else if ( gl_clear.GetBool() )
    {
	    nClearFlags |= VIEW_CLEAR_COLOR;
    }
#ifdef _POSIX
    else
    {
	    MaterialAdapterInfo_t adapterInfo;
	    materials->GetDisplayAdapterInfo( materials->GetCurrentAdapter(), adapterInfo );

	    // On Posix, on ATI, we always clear color if we're antialiasing
	    if ( adapterInfo.m_VendorID == 0x1002 )
	    {
		    if ( g_pMaterialSystem->GetCurrentConfigForVideoCard().m_nAASamples > 0 )
		    {
			    nClearFlags |= VIEW_CLEAR_COLOR;
		    }
	    }
    }
#endif

    // Determine if we should draw view model ( client mode override )
    bool drawViewModel = GetClientMode()->ShouldDrawViewModel();

    if ( cl_leveloverview.GetFloat() > 0 )
    {
	    SetUpOverView();		
	    nClearFlags |= VIEW_CLEAR_COLOR;
	    drawViewModel = false;
    }

    // Apply any player specific overrides
    if ( pPlayer )
    {
	    // Override view model if necessary
	    if ( !pPlayer->m_Local.m_bDrawViewmodel )
	    {
		    drawViewModel = false;
	    }
    }

    int flags = (pPlayer == NULL) ? 0 : RENDERVIEW_DRAWHUD;
    if ( drawViewModel )
    {
	    flags |= RENDERVIEW_DRAWVIEWMODEL;
    }

    C_BaseEntity::PreRenderEntities();

    CViewSetupEx hudViewSetup;
	VGui_GetHudBounds( hudViewSetup.x, hudViewSetup.y, hudViewSetup.width, hudViewSetup.height );
	RenderView( view, hudViewSetup, nClearFlags, flags );

	// TODO: should these be inside or outside the stereo eye stuff?
	GetClientMode()->PostRender();

	engine->EngineStats_EndFrame();

	// Stop stubbing the material system so we can see the budget panel
	matStub.End();

	// Render the new-style embedded UI
	// TODO: when embedded UI will be used for HUD, we will need it to maintain
	// a separate screen for HUD and a separate screen stack for pause menu & main menu.
	// for now only render embedded UI in pause menu & main menu
#if defined( GAMEUI_UISYSTEM2_ENABLED ) && 0
	BaseModUI::CBaseModPanel *pBaseModPanel = BaseModUI::CBaseModPanel::GetSingletonPtr();
	// render the new-style embedded UI only if base mod panel is not visible (game-hud)
	// otherwise base mod panel will render the embedded UI on top of video/productscreen
	if ( !pBaseModPanel || !pBaseModPanel->IsVisible() )
	{
		Rect_t uiViewport;
		uiViewport.x		= rect->x;
		uiViewport.y		= rect->y;
		uiViewport.width	= rect->width;
		uiViewport.height	= rect->height;
		g_pGameUIGameSystem->Render( uiViewport, gpGlobals->curtime );
	}
#endif

	// Draw all of the UI stuff "fullscreen"
    // (this is not health, ammo, etc. Nor is it pre-game briefing interface stuff - this is the stuff that appears when you hit Esc in-game)
	// In stereo mode this is rendered inside of RenderView so it goes into the render target

	CViewSetupEx view2d;
	view2d.x				= rect->x;
	view2d.y				= rect->y;
	view2d.width			= rect->width;
	view2d.height			= rect->height;

	render->Push2DView( view2d, 0, NULL, GetFrustum() );
	render->VGui_Paint( PAINT_UIPANELS | PAINT_CURSOR );
	render->PopView( GetFrustum() );

	m_bAllowViewAccess = false;
}

static void GetPos( const CCommand &args, Vector &vecOrigin, QAngle &angles )
{
	vecOrigin = MainViewOrigin();
	angles = MainViewAngles();
	if ( ( args.ArgC() == 2 && atoi( args[1] ) == 2 ) || FStrEq( args[0], "getpos_exact" ) )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			vecOrigin = pPlayer->GetAbsOrigin();
			angles = pPlayer->GetAbsAngles();
		}
	}
}

CON_COMMAND( spec_pos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );
	Warning( "spec_goto %.1f %.1f %.1f %.1f %.1f\n", vecOrigin.x, vecOrigin.y, 
		vecOrigin.z, angles.x, angles.y );
}

CON_COMMAND( getpos, "dump position and angles to the console" )
{
	Vector vecOrigin;
	QAngle angles;
	GetPos( args, vecOrigin, angles );

	const char *pCommand1 = "setpos";
	const char *pCommand2 = "setang";
	if ( ( args.ArgC() == 2 && atoi( args[1] ) == 2 ) || FStrEq( args[0], "getpos_exact" ) )
	{
		pCommand1 = "setpos_exact";
		pCommand2 = "setang_exact";
	}

	Warning( "%s %f %f %f;", pCommand1, vecOrigin.x, vecOrigin.y, vecOrigin.z );
	Warning( "%s %f %f %f\n", pCommand2, angles.x, angles.y, angles.z );
}

ConCommand getpos_exact( "getpos_exact", getpos_callback, "dump origin and angles to the console" );

void CViewRender::PostSimulate()
{
    C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
    if ( !pLocal )
        return;

    //Tony; if the local player is in a vehicle, then we need to kill the bone cache, and re-calculate the view.
    if ( !pLocal->IsInAVehicle() && !pLocal->GetVehicle())
        return;

    IClientVehicle *pVehicle = pLocal->GetVehicle();
    Assert( pVehicle );
    CBaseAnimating *pVehicleEntity = (CBaseAnimating*)pVehicle->GetVehicleEnt();
    Assert( pVehicleEntity );

    int nRole = pVehicle->GetPassengerRole( pLocal );

    //Tony; we have to invalidate the bone cache in order for the attachment lookups to be correct!
    pVehicleEntity->InvalidateBoneCache();
    pVehicle->GetVehicleViewPosition( nRole, &m_View.origin, &m_View.angles,
										&m_View.fov );

    //Tony; everything below is from SetupView - the things that should be recalculated.. are recalculated!
    pLocal->CalcViewModelView( m_View.origin, m_View.angles );

    // Compute the world->main camera transform
    ComputeCameraVariables( m_View.origin, m_View.angles,
        &g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

    // set up the hearing origin...
    AudioState_t audioState;
    audioState.m_Origin = m_View.origin;
    audioState.m_Angles = m_View.angles;
    audioState.m_bIsUnderwater = pLocal && pLocal->AudioStateIsUnderwater(m_View.origin );

    ToolFramework_SetupAudioState( audioState );

    m_View.origin = audioState.m_Origin;
    m_View.angles = audioState.m_Angles;

    engine->SetAudioState( audioState );

    g_vecPrevRenderOrigin = g_vecRenderOrigin;
    g_vecPrevRenderAngles = g_vecRenderAngles;
    g_vecRenderOrigin = m_View.origin;
    g_vecRenderAngles = m_View.angles;

#ifdef _DEBUG
    s_DbgSetupOrigin = m_View.origin;
    s_DbgSetupAngles = m_View.angles;
#endif

}
