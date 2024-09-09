//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side CBasePlayer.
//
//			- Manages the player's flashlight effect.
//
//===========================================================================//
#include "cbase.h"
#include "c_baseplayer.h"
#include "flashlighteffect.h"
#include "weapon_selection.h"
#include "history_resource.h"
#include "iinput.h"
#include "input.h"
#include "view.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "c_soundscape.h"
#include "usercmd.h"
#include "c_playerresource.h"
#include "iclientvehicle.h"
#include "view_shared.h"
#include "movevars_shared.h"
#include "prediction.h"
#include "tier0/vprof.h"
#include "filesystem.h"
#include "bitbuf.h"
#include "KeyValues.h"
#include "particles_simple.h"
#include "fx_water.h"
#include "hltvcamera.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "view_scene.h"
#include "c_vguiscreen.h"
#include "datacache/imdlcache.h"
#include "vgui/ISurface.h"
#include "voice_status.h"
#include "fx.h"
#include "dt_utlvector_recv.h"
#include "cam_thirdperson.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif
#include "steam/steam_api.h"
#include "cellcoord.h"
#include "debugoverlay_shared.h"
#include "ammodef.h"
#include "c_basetempentity.h"
#include "beamdraw.h"
#include "iviewrender_beams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Don't alias here
#if defined( CBasePlayer )
#undef CBasePlayer	
#endif

int g_nKillCamMode = OBS_MODE_NONE;
int g_nKillCamTarget1 = 0;
int g_nKillCamTarget2 = 0;

extern ConVar mp_forcecamera; // in gamevars_shared.h
extern ConVar r_mapextents;

#define CYCLELATCH_TOLERANCE 0.15f

#define FLASHLIGHT_DISTANCE		1000
#define MAX_VGUI_INPUT_MODE_SPEED 30
#define MAX_VGUI_INPUT_MODE_SPEED_SQ (MAX_VGUI_INPUT_MODE_SPEED*MAX_VGUI_INPUT_MODE_SPEED)

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

extern ConVar default_fov;
extern ConVar sensitivity;

static C_BasePlayer *s_pLocalPlayer = NULL;

static ConVar	cl_customsounds ( "cl_customsounds", "0", 0, "Enable customized player sound playback" );
static ConVar	spec_track		( "spec_track", "0", 0, "Tracks an entity in spec mode" );
static ConVar	cl_smooth		( "cl_smooth", "1", 0, "Smooth view/eye origin after prediction errors" );
static ConVar	cl_smoothtime	( 
	"cl_smoothtime", 
	"0.1", 
	0, 
	"Smooth client's view after prediction error over this many seconds",
	true, 0.01,	// min/max is 0.01/2.0
	true, 2.0
	 );

ConVar	spec_freeze_time( "spec_freeze_time", "5.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Time spend frozen in observer freeze cam." );
ConVar	spec_freeze_traveltime( "spec_freeze_traveltime", "0.7", FCVAR_CHEAT | FCVAR_REPLICATED, "Time taken to zoom in to frame a target in observer freeze cam.", true, 0.01, false, 0 );
ConVar	spec_freeze_distance_min( "spec_freeze_distance_min", "80", FCVAR_CHEAT, "Minimum random distance from the target to stop when framing them in observer freeze cam." );
ConVar	spec_freeze_distance_max( "spec_freeze_distance_max", "200", FCVAR_CHEAT, "Maximum random distance from the target to stop when framing them in observer freeze cam." );

static ConVar	cl_first_person_uses_world_model ( "cl_first_person_uses_world_model", "0", FCVAR_ARCHIVE, "Causes the third person model to be drawn instead of the view model" );

ConVar demo_fov_override( "demo_fov_override", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "If nonzero, this value will be used to override FOV during demo playback." );

// This only needs to be approximate - it just controls the distance to the pivot-point of the head ("the neck") of the in-game character, not the player's real-world neck length.
// Ideally we would find this vector by subtracting the neutral-pose difference between the head bone (the pivot point) and the "eyes" attachment point.
// However, some characters don't have this attachment point, and finding the neutral pose is a pain.
// This value is found by hand, and a good value depends more on the in-game models than on actual human shapes.
ConVar cl_meathook_neck_pivot_ingame_up( "cl_meathook_neck_pivot_ingame_up", "7.0" );
ConVar cl_meathook_neck_pivot_ingame_fwd( "cl_meathook_neck_pivot_ingame_fwd", "3.0" );

static ConVar	cl_clean_textures_on_death( "cl_clean_textures_on_death", "0", FCVAR_DEVELOPMENTONLY,  "If enabled, attempts to purge unused textures every time a freeze cam is shown" );


void RecvProxy_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut );

void RecvProxy_ObserverTarget( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_ObserverMode  ( const CRecvProxyData *pData, void *pStruct, void *pOut );

// -------------------------------------------------------------------------------- //
// RecvTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		RecvPropInt		(RECVINFO(deadflag)),
	END_RECV_TABLE()


BEGIN_RECV_TABLE_NOBASE( CPlayerLocalData, DT_Local )
	RecvPropArray3( RECVINFO_ARRAY(m_chAreaBits), RecvPropInt(RECVINFO(m_chAreaBits[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_chAreaPortalBits), RecvPropInt(RECVINFO(m_chAreaPortalBits[0]))),
	RecvPropInt(RECVINFO(m_iHideHUD)),

	// View
	
	RecvPropFloat(RECVINFO(m_flFOVRate)),
	
	RecvPropInt		(RECVINFO(m_bDucked)),
	RecvPropInt		(RECVINFO(m_bDucking)),
	RecvPropInt		(RECVINFO(m_bInDuckJump)),
	RecvPropFloat	(RECVINFO(m_flDucktime)),
	RecvPropFloat	(RECVINFO(m_flDuckJumpTime)),
	RecvPropFloat	(RECVINFO(m_flJumpTime)),
	RecvPropFloat	(RECVINFO(m_flFallVelocity)),

#if PREDICTION_ERROR_CHECK_LEVEL > 1 
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngle.m_Value[0], m_vecPunchAngle[0])),
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngle.m_Value[1], m_vecPunchAngle[1])),
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngle.m_Value[2], m_vecPunchAngle[2] )),
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngleVel.m_Value[0], m_vecPunchAngleVel[0] )),
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngleVel.m_Value[1], m_vecPunchAngleVel[1] )),
	RecvPropFloat	(RECVINFO_NAME( m_vecPunchAngleVel.m_Value[2], m_vecPunchAngleVel[2] )),
#else
	RecvPropVector	(RECVINFO(m_vecPunchAngle)),
	RecvPropVector	(RECVINFO(m_vecPunchAngleVel)),
#endif

	RecvPropInt		(RECVINFO(m_bDrawViewmodel)),
	RecvPropInt		(RECVINFO(m_bWearingSuit)),
	RecvPropBool	(RECVINFO(m_bPoisoned)),
	RecvPropFloat	(RECVINFO(m_flStepSize)),
	RecvPropInt		(RECVINFO(m_bAllowAutoMovement)),

	// 3d skybox data
	RecvPropInt(RECVINFO(m_skybox3d.scale)),
	RecvPropVector(RECVINFO(m_skybox3d.origin)),
	RecvPropInt(RECVINFO(m_skybox3d.area)),

	// 3d skybox fog data
	RecvPropInt( RECVINFO( m_skybox3d.fog.enable ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.blend ) ),
	RecvPropVector( RECVINFO( m_skybox3d.fog.dirPrimary ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorPrimary ), 0, RecvProxy_Int32ToColor32 ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorSecondary ), 0, RecvProxy_Int32ToColor32 ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.start ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.end ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.maxdensity ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.HDRColorScale ) ),

	// fog data
	RecvPropEHandle( RECVINFO( m_PlayerFog.m_hCtrl ) ),

	// audio data
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 0 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 1 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 2 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 3 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 4 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 5 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 6 ) ),
	RecvPropVector( RECVINFO_ARRAYELEM( m_audio.localSound, 7 ) ),
	RecvPropInt( RECVINFO( m_audio.soundscapeIndex ) ),
	RecvPropInt( RECVINFO( m_audio.localBits ) ),
	RecvPropEHandle( RECVINFO( m_audio.ent ) ),

	//Tony; tonemap stuff! -- TODO! Optimize this with bit sizes from env_tonemap_controller.
	RecvPropFloat( RECVINFO( m_TonemapParams.m_flTonemapScale ) ),
	RecvPropFloat( RECVINFO( m_TonemapParams.m_flTonemapRate ) ),
	RecvPropFloat( RECVINFO( m_TonemapParams.m_flBloomScale ) ),

	RecvPropFloat( RECVINFO( m_TonemapParams.m_flAutoExposureMin ) ),
	RecvPropFloat( RECVINFO( m_TonemapParams.m_flAutoExposureMax ) ),
END_RECV_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE( C_BasePlayer, DT_LocalPlayerExclusive )

		//TODO!!!! why does this break things?
		//RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ), // RECVINFO_NAME redirects the received var to m_vecNetworkOrigin for interpolation purposes
		RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin), 0, C_BaseEntity::RecvProxy_CellOrigin ), // RECVINFO_NAME again

		RecvPropFloat( RECVINFO_ARRAYELEM( m_angEyeAngles, 0 ) ),
		RecvPropFloat( RECVINFO_ARRAYELEM( m_angEyeAngles, 1 ) ),

		RecvPropDataTable	( RECVINFO_DT(m_Local),0, &REFERENCE_RECV_TABLE(DT_Local) ),

		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecViewOffset, 0) ),
		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecViewOffset, 1) ),
		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecViewOffset, 2) ),
		RecvPropFloat		( RECVINFO(m_flFriction) ),

		RecvPropArray3		( RECVINFO_ARRAY(m_iAmmo), RecvPropInt( RECVINFO_ARRAYELEM(m_iAmmo, 0)) ),
		
		RecvPropInt			( RECVINFO(m_fOnTarget) ),

		RecvPropInt			( RECVINFO( m_nTickBase ) ),
		RecvPropInt			( RECVINFO( m_nNextThinkTick ) ),

		RecvPropEHandle		( RECVINFO( m_hLastWeapon ) ),
		RecvPropEHandle		( RECVINFO( m_hGroundEntity ) ),

 		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecVelocity, 0), 0, RecvProxy_LocalVelocityX ),
 		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecVelocity, 1), 0, RecvProxy_LocalVelocityY ),
 		RecvPropFloat		( RECVINFO_ARRAYELEM(m_vecVelocity, 2), 0, RecvProxy_LocalVelocityZ ),

		RecvPropVector		( RECVINFO( m_vecBaseVelocity ) ),

		RecvPropEHandle		( RECVINFO( m_hConstraintEntity)),
		RecvPropVector		( RECVINFO( m_vecConstraintCenter) ),
		RecvPropFloat		( RECVINFO( m_flConstraintRadius )),
		RecvPropFloat		( RECVINFO( m_flConstraintWidth )),
		RecvPropFloat		( RECVINFO( m_flConstraintSpeedFactor )),
		RecvPropBool		( RECVINFO( m_bConstraintPastRadius )),

		RecvPropFloat		( RECVINFO( m_flDeathTime )),

		RecvPropInt			( RECVINFO( m_nWaterLevel ) ),
		RecvPropFloat		( RECVINFO( m_flLaggedMovementValue )),

		RecvPropEHandle		( RECVINFO(m_hTonemapController) ),
	END_RECV_TABLE()

void RecvProxy_CycleLatch(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	C_BaseAnimating* pEntity = static_cast<C_BaseAnimating *>(pStruct);

	float flServerCycle = (float)pData->m_Value.m_Int / 16.0f;
	float flCurCycle = pEntity->GetCycle();

	if(fabs(flCurCycle - flServerCycle) > CYCLELATCH_TOLERANCE) {
		pEntity->SetServerIntendedCycle(flServerCycle);
	}
}

	BEGIN_RECV_TABLE_NOBASE( C_BasePlayer, DT_NonLocalPlayerExclusive )
		RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin), 0, C_BaseEntity::RecvProxy_CellOrigin ), // RECVINFO_NAME again
		RecvPropFloat( RECVINFO_ARRAYELEM( m_angEyeAngles, 0 ) ),
		RecvPropFloat( RECVINFO_ARRAYELEM( m_angEyeAngles, 1 ) ),
		RecvPropInt(RECVINFO(m_cycleLatch), 0, &RecvProxy_CycleLatch),
	END_RECV_TABLE()
	
// -------------------------------------------------------------------------------- //
// DT_BasePlayer datatable.
// -------------------------------------------------------------------------------- //

	IMPLEMENT_CLIENTCLASS_DT(C_BasePlayer, DT_BasePlayer, CBasePlayer)
		// We have both the local and nonlocal data in here, but the server proxies
		// only send one.
		RecvPropDataTable( "localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalPlayerExclusive) ),
		RecvPropDataTable( "nonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_NonLocalPlayerExclusive) ),

		RecvPropInt(RECVINFO(m_iSpawnInterpCounter)),

		RecvPropDataTable(RECVINFO_DT(pl), 0, &REFERENCE_RECV_TABLE(DT_PlayerState), DataTableRecvProxy_StaticDataTable),

		RecvPropInt		(RECVINFO(m_iFOV)),
		RecvPropInt		(RECVINFO(m_iFOVStart)),
		RecvPropFloat	(RECVINFO(m_flFOVTime)),
		RecvPropInt		(RECVINFO(m_iDefaultFOV)),
		RecvPropEHandle (RECVINFO(m_hZoomOwner)),

		RecvPropEHandle( RECVINFO(m_hVehicle) ),
		RecvPropEHandle( RECVINFO(m_hUseEntity) ),

		RecvPropEHandle		( RECVINFO( m_hViewEntity ) ),		// L4D: send view entity to everyone for first-person spectating
		RecvPropEHandle		( RECVINFO( m_hGroundEntity ) ),

		RecvPropInt		(RECVINFO(m_iHealth)),
		RecvPropInt		(RECVINFO(m_lifeState)),

		RecvPropInt		(RECVINFO(m_iBonusProgress)),
		RecvPropInt		(RECVINFO(m_iBonusChallenge)),

		RecvPropFloat	(RECVINFO(m_flMaxspeed)),
		RecvPropInt		(RECVINFO(m_fFlags)),


		RecvPropInt		(RECVINFO(m_iObserverMode), 0, RecvProxy_ObserverMode ),
		RecvPropEHandle	(RECVINFO(m_hObserverTarget), RecvProxy_ObserverTarget ),
		RecvPropArray	( RecvPropEHandle( RECVINFO( m_hViewModel[0] ) ), m_hViewModel ),
		

		RecvPropString( RECVINFO(m_szLastPlaceName) ),
		RecvPropVector( RECVINFO(m_vecLadderNormal) ),
		RecvPropInt		(RECVINFO(m_ladderSurfaceProps) ),

		RecvPropEHandle( RECVINFO( m_hPostProcessCtrl ) ),		// Send to everybody - for spectating
		RecvPropEHandle( RECVINFO( m_hColorCorrectionCtrl ) ),	// Send to everybody - for spectating

	END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerState )

	DEFINE_PRED_FIELD(  deadflag, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( netname, string_t ),
	// DEFINE_FIELD( fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( anglechange, FIELD_FLOAT ),
	// DEFINE_FIELD( v_angle, FIELD_VECTOR ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerLocalData )

	// DEFINE_PRED_TYPEDESCRIPTION( m_skybox3d, sky3dparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( m_fog, fogparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( m_audio, audioparams_t ),
	DEFINE_FIELD( m_nStepside, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( m_iHideHUD, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
#if PREDICTION_ERROR_CHECK_LEVEL > 1
	DEFINE_PRED_FIELD( m_vecPunchAngle, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vecPunchAngleVel, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
#else
	DEFINE_PRED_FIELD_TOL( m_vecPunchAngle, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD_TOL( m_vecPunchAngleVel, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
#endif
	DEFINE_PRED_FIELD( m_bDrawViewmodel, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bWearingSuit, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPoisoned, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAllowAutoMovement, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_bDucked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDucking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInDuckJump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDucktime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDuckJumpTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flJumpTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flFallVelocity, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
//	DEFINE_PRED_FIELD( m_nOldButtons, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_nOldButtons, FIELD_INTEGER ),
	DEFINE_PRED_FIELD( m_flStepSize, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_flFOVRate, FIELD_FLOAT ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA( C_BasePlayer )

	DEFINE_PRED_TYPEDESCRIPTION( m_Local, CPlayerLocalData ),
	DEFINE_PRED_TYPEDESCRIPTION( pl, CPlayerState ),

	DEFINE_PRED_FIELD( m_iFOV, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hZoomOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flFOVTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_iFOVStart, FIELD_INTEGER, 0 ),

	DEFINE_PRED_FIELD( m_hVehicle, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flMaxspeed, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_PRED_FIELD( m_iHealth, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iBonusProgress, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iBonusChallenge, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fOnTarget, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_lifeState, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nWaterLevel, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),
	
	DEFINE_PRED_FIELD_TOL( m_vecBaseVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.05 ),

	DEFINE_FIELD( m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( m_flWaterJumpTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSwimSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecLadderNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_ladderSurfaceProps, FIELD_INTEGER ),
	DEFINE_FIELD( m_flPhysics, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_szAnimExtension, FIELD_CHARACTER ),
	DEFINE_FIELD( m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonReleased, FIELD_INTEGER ),
	// DEFINE_FIELD( m_vecOldViewAngles, FIELD_VECTOR ),

	// DEFINE_ARRAY( m_iOldAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES ),

	//DEFINE_FIELD( m_hOldVehicle, FIELD_EHANDLE ),
	// DEFINE_FIELD( m_pModelLight, dlight_t* ),
	// DEFINE_FIELD( m_pEnvironmentLight, dlight_t* ),
	// DEFINE_FIELD( m_pBrightLight, dlight_t* ),
	DEFINE_PRED_FIELD( m_hLastWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_nTickBase, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_hGroundEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_ARRAY( m_hViewModel, FIELD_EHANDLE, MAX_VIEWMODELS, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( m_surfaceFriction, FIELD_FLOAT ),

	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( player, C_BasePlayer );

// -------------------------------------------------------------------------------- //
// Functions.
// -------------------------------------------------------------------------------- //
C_BasePlayer::C_BasePlayer() :
	m_iv_vecViewOffset( "C_BasePlayer::m_iv_vecViewOffset" ),
	m_iv_angEyeAngles( "C_BasePlayer::m_iv_angEyeAngles" )
{
	AddVar( &m_vecViewOffset, &m_iv_vecViewOffset, LATCH_SIMULATION_VAR );

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_iSpawnInterpCounterCache = 0;
	m_iSpawnInterpCounter = 0;

	m_iIDEntIndex = 0;
	
#ifdef _DEBUG																
	m_vecLadderNormal.Init();
	m_ladderSurfaceProps = 0;
	m_vecOldViewAngles.Init();
#endif
	m_hViewEntity = NULL;

	m_bFlashlightEnabled = false;

	m_pCurrentVguiScreen = NULL;
	m_pCurrentCommand = NULL;

	m_flPredictionErrorTime = -100;
	m_StuckLast = 0;
	m_bWasFrozen = false;

	m_bResampleWaterSurface = true;
	
	ResetObserverMode();

	m_vecPredictionError.Init();
	m_flPredictionErrorTime = 0;

	m_surfaceProps = 0;
	m_pSurfaceData = NULL;
	m_surfaceFriction = 1.0f;
	m_chTextureType = 0;

	m_flNextAchievementAnnounceTime = 0;

	m_bFiredWeapon = false;

	m_nForceVisionFilterFlags = 0;
	m_nLocalPlayerVisionFlags = 0;

	m_bIsLocalPlayer = false;
	m_afButtonForced = 0;

	m_flServerCycle = -1.0f;

	m_pFlashlightBeam = NULL;

	SetPredictionEligible(true);

	ListenForGameEvent( "base_player_teleported" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer::~C_BasePlayer()
{
	DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
	if ( this == s_pLocalPlayer )
	{
		s_pLocalPlayer = NULL;
	}

	m_PlayerAnimState->Release();

	TurnOffFlashlight();
}

bool C_BasePlayer::PostConstructor( const char *szClassname )
{
	if(!BaseClass::PostConstructor( szClassname ))
		return false;

	m_PlayerAnimState = CreateAnimState();

	return true;
}

bool C_BasePlayer::ShouldReceiveProjectedTextures( int flags )
{
	Assert(flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK);

	if(IsEffectActive(EF_NODRAW)) {
		return false;
	}

	if(flags & SHADOW_FLAGS_FLASHLIGHT) {
		return true;
	}

	return BaseClass::ShouldReceiveProjectedTextures(flags);
}

ShadowType_t C_BasePlayer::ShadowCastType()
{
	if(!IsVisible()) {
		return SHADOWS_NONE;
	}

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

CStudioHdr *C_BasePlayer::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if(m_PlayerAnimState)
		m_PlayerAnimState->OnNewModel();

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::Spawn( void )
{
	// Clear all flags except for FL_FULLEDICT
	ClearFlags();
	AddFlag( FL_CLIENT );

	int effects = GetEffects() & EF_NOSHADOW;
	SetEffects( effects );

	m_iFOV	= 0;	// init field of view.

    SetModel( "models/player.mdl" );

	Precache();

	SetThink(NULL);

	SharedSpawn();

	m_bWasFreezeFraming = false;

	m_bFiredWeapon = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BasePlayer::AudioStateIsUnderwater( const Vector &vecMainViewOrigin ) const
{
	if ( IsObserver() )
	{
		// Just check the view position
		int cont = enginetrace->GetPointContents_WorldOnly ( vecMainViewOrigin, MASK_WATER );
		return (cont & MASK_WATER) ? true : false;
	}

	return ( GetWaterLevel() >= WL_Eyes );
}

bool C_BasePlayer::IsHLTV() const
{
	return ( IsLocalPlayer() && engine->IsHLTV() );	
}

bool C_BasePlayer::IsReplay() const
{
#if defined( REPLAY_ENABLED )
	return ( IsLocalPlayer() && g_pEngineClientReplay->IsPlayingReplayDemo() );
#else
	return false;
#endif
}

CBaseEntity	*C_BasePlayer::GetObserverTarget() const	// returns players target or NULL
{
	if ( IsHLTV() )
	{
		return HLTVCamera()->GetPrimaryTarget();
	}
#if defined( REPLAY_ENABLED )
	if ( IsReplay() )
	{
		return ReplayCamera()->GetPrimaryTarget();
	}
#endif
	
	if ( GetObserverMode() == OBS_MODE_ROAMING )
	{
		return NULL;	// no target in roaming mode
	}
	else
	{
		return m_hObserverTarget;
	}
}

// Helper method to fix up visiblity across split screen for view models when observer target or mode changes
static void UpdateViewmodelVisibility( C_BasePlayer *player )
{
	// Update view model visibility
	for ( int i = 0; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = player->GetViewModel( i );
		if ( !vm )
			continue;
		vm->UpdateVisibility();
	}
}

// Called from Recv Proxy, mainly to reset tone map scale
void C_BasePlayer::SetObserverTarget( EHANDLE hObserverTarget )
{
	// If the observer target is changing to an entity that the client doesn't know about yet,
	// it can resolve to NULL.  If the client didn't have an observer target before, then
	// comparing EHANDLEs directly will see them as equal, since it uses Get(), and compares
	// NULL to NULL.  To combat this, we need to check against GetEntryIndex() and
	// GetSerialNumber().
	if ( hObserverTarget.GetEntryIndex() != m_hObserverTarget.GetEntryIndex() ||
		hObserverTarget.GetSerialNumber() != m_hObserverTarget.GetSerialNumber())
	{
		// Init based on the new handle's entry index and serial number, so that it's Get()
		// has a chance to become non-NULL even if it currently resolves to NULL.
		m_hObserverTarget.Init( hObserverTarget.GetEntryIndex(), hObserverTarget.GetSerialNumber() );

		IGameEvent *event = gameeventmanager->CreateEvent( "spec_target_updated" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}

		if ( IsLocalPlayer() )
		{
			ResetToneMapping( 1.0 );
		}

		UpdateViewmodelVisibility( this );
	}
}


void C_BasePlayer::SetObserverMode ( int iNewMode )
{
	if ( m_iObserverMode != iNewMode )
	{
		m_iObserverMode = iNewMode;
	}
}


int C_BasePlayer::GetObserverMode() const 
{ 
	if ( IsHLTV() )
	{
		return HLTVCamera()->GetMode();
	}
#if defined( REPLAY_ENABLED )
	if ( IsReplay() )
	{
		return ReplayCamera()->GetMode();
	}
#endif

	return m_iObserverMode; 
}

uint8 C_BasePlayer::ViewModel_OverrideRenderAlpha( uint8 nAlpha )
{
	return AlphaProp()->ComputeRenderAlpha();
}

RenderableTranslucencyType_t C_BasePlayer::ViewModel_ComputeTranslucencyType( void )
{
	return ComputeTranslucencyType();
}

int C_BasePlayer::ViewModel_GetRenderFlags( void )
{
	return GetRenderFlags();
}

//-----------------------------------------------------------------------------
// Used by prediction, sets the view angles for the player
//-----------------------------------------------------------------------------
void C_BasePlayer::SetLocalViewAngles( const QAngle &viewAngles )
{
	pl.v_angle = viewAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ang - 
//-----------------------------------------------------------------------------
void C_BasePlayer::SetViewAngles( const QAngle& ang )
{
	SetLocalAngles( ang );
	SetNetworkAngles( ang );
}


surfacedata_t* C_BasePlayer::GetGroundSurface()
{
	//
	// Find the name of the material that lies beneath the player.
	//
	Vector start, end;
	VectorCopy( GetAbsOrigin(), start );
	VectorCopy( start, end );

	// Straight down
	end.z -= 64;

	// Fill in default values, just in case.
	
	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );

	trace_t	trace;
	UTIL_TraceRay( ray, MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

	if ( trace.fraction == 1.0f )
		return NULL;	// no ground
	
	return physprops->GetSurfaceData( trace.surface.surfaceProps );
}

void C_BasePlayer::FireGameEvent( IGameEvent *event )
{
	
}

//-----------------------------------------------------------------------------
// returns the player name
//-----------------------------------------------------------------------------
const char * C_BasePlayer::GetPlayerName()
{
	return g_PR ? g_PR->GetPlayerName( entindex() ) : "";
}

//-----------------------------------------------------------------------------
// Is the player dead?
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsPlayerDead()
{
	return pl.deadflag == true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BasePlayer::SetVehicleRole( int nRole )
{
	if ( !IsInAVehicle() )
		return;

	// HL2 has only a player in a vehicle.
	if ( nRole > VEHICLE_ROLE_DRIVER )
		return;

	char szCmd[64];
	Q_snprintf( szCmd, sizeof( szCmd ), "vehicleRole %i\n", nRole );
	engine->ServerCmd( szCmd );
}

//-----------------------------------------------------------------------------
// Purpose: Store original ammo data to see what has changed
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BasePlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	for (int i = 0; i < MAX_AMMO_TYPES; ++i)
	{
		m_iOldAmmo[i] = GetAmmoCount(i);
	}

	m_bWasFreezeFraming = (GetObserverMode() == OBS_MODE_FREEZECAM);
	m_hOldFogController.Set( m_Local.m_PlayerFog.m_hCtrl.Get() );

	BaseClass::OnPreDataChanged( updateType );
}

void C_BasePlayer::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}

void C_BasePlayer::CheckForLocalPlayer()
{
	int iLocalPlayerIndex = engine->GetLocalPlayer();

	if ( g_nKillCamMode )
		iLocalPlayerIndex = g_nKillCamTarget1;

	if ( iLocalPlayerIndex == entindex() )
	{
		Assert( s_pLocalPlayer == NULL );
		s_pLocalPlayer = this;
		m_bIsLocalPlayer = true;

		// Reset our sound mixed in case we were in a freeze cam when we
		// changed level, which would cause the snd_soundmixer to be left modified.
		ConVar *pVar = (ConVar *)cvar->FindVar( "snd_soundmixer" );
		pVar->Revert();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BasePlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// This has to occur here as opposed to OnDataChanged so that EHandles to the player created
	//  on this same frame are not stomped because prediction thinks there
	//  isn't a local player yet!!!

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Make sure s_pLocalPlayer is correct

		CheckForLocalPlayer();
	}

	bool bForceEFNoInterp = IsNoInterpolationFrame();

	if ( IsLocalPlayer( this ) )
	{
		SetSimulatedEveryTick( true );
	}
	else
	{
		SetSimulatedEveryTick( false );

		// estimate velocity for non local players
		float flTimeDelta = GetSimulationTime() - GetOldSimulationTime();

		if (IsParentChanging())
		{
			bForceEFNoInterp = true;
		}

		if ( flTimeDelta > 0  &&  !( IsNoInterpolationFrame() || bForceEFNoInterp ) )
		{
			Vector newVelo = (GetNetworkOrigin() - GetOldOrigin()  ) / flTimeDelta;
			SetAbsVelocity( newVelo);
		}
	}

	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::PostDataUpdate( updateType );
			 
	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		QAngle angles;
		engine->GetViewAngles( angles );
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetLocalViewAngles( angles );
			m_flOldPlayerZ = GetLocalOrigin().z;
		}
		SetLocalAngles( angles );

		if ( !m_bWasFreezeFraming && GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			m_vecFreezeFrameStart = MainViewOrigin();
			m_flFreezeFrameStartTime = gpGlobals->curtime;
			m_flFreezeFrameDistance = RandomFloat( spec_freeze_distance_min.GetFloat(), spec_freeze_distance_max.GetFloat() );
			m_flFreezeZOffset = RandomFloat( -30, 20 );
			m_bSentFreezeFrame = false;
			m_nForceVisionFilterFlags = 0;

			C_BaseEntity *target = GetObserverTarget();
			if ( target && target->IsPlayer() )
			{
				C_BasePlayer *player = ToBasePlayer( target );
				if ( player )
				{
					m_nForceVisionFilterFlags = player->GetVisionFilterFlags();
					localplayer_visionflags.SetValue( m_nForceVisionFilterFlags );
					CalculateVisionUsingCurrentFlags();
				}
			}

			IGameEvent *pEvent = gameeventmanager->CreateEvent( "show_freezepanel" );
			if ( pEvent )
			{
				pEvent->SetInt( "killer", target ? target->entindex() : 0 );
				gameeventmanager->FireEventClientSide( pEvent );
			}

			// Force the sound mixer to the freezecam mixer
			ConVar *pVar = (ConVar *)cvar->FindVar( "snd_soundmixer" );
			pVar->SetValue( "FreezeCam_Only" );

			// When we start, give unused textures an opportunity to unload
			if ( cl_clean_textures_on_death.GetBool() )
				g_pMaterialSystem->UncacheUnusedMaterials( false );
		}
		else if ( m_bWasFreezeFraming && GetObserverMode() != OBS_MODE_FREEZECAM )
		{
			IGameEvent *pEvent = gameeventmanager->CreateEvent( "hide_freezepanel" );
			if ( pEvent )
			{
				gameeventmanager->FireEventClientSide( pEvent );
			}

			GetViewRenderInstance()->FreezeFrame(0);

			ConVar *pVar = (ConVar *)cvar->FindVar( "snd_soundmixer" );
			pVar->Revert();

			m_nForceVisionFilterFlags = 0;
			CalculateVisionUsingCurrentFlags();
		}
		
		// force calculate vision when the local vision flags changed
		int nCurrentLocalPlayerVisionFlags = GetLocalPlayerVisionFilterFlags();
		if ( m_nLocalPlayerVisionFlags != nCurrentLocalPlayerVisionFlags )
		{
			localplayer_visionflags.SetValue( nCurrentLocalPlayerVisionFlags );
			CalculateVisionUsingCurrentFlags();
			m_nLocalPlayerVisionFlags = nCurrentLocalPlayerVisionFlags;
		}
	}

	// If we are updated while paused, allow the player origin to be snapped by the
	//  server if we receive a packet from the server
	if ( engine->IsPaused() || bForceEFNoInterp )
	{
		ResetLatched();
	}

	// Did we just respawn?
	if(m_iSpawnInterpCounter != m_iSpawnInterpCounterCache)
	{
		// fix up interp
		MoveToLastReceivedPosition( true );
		ResetLatched();
		
		Respawn();
	
		m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
	}
}

const QAngle& C_BasePlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
		return vec3_angle;
	else
		return m_PlayerAnimState->GetRenderAngles();
}

void C_BasePlayer::Respawn()
{
	RemoveAllDecals();

	m_PlayerAnimState->ResetGestureSlots();
	
	// reset HDR
	if ( IsLocalPlayer() )
		ResetToneMapping(1.0);

	if(m_pClientsideRagdoll) {
		UTIL_Remove(m_pClientsideRagdoll);
		m_pClientsideRagdoll = NULL;
	}
}

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType );

public:
	CNetworkHandle( C_BasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) ),
END_RECV_TABLE()

void C_TEPlayerAnimEvent::PostDataUpdate( DataUpdateType_t updateType )
{
	C_BasePlayer *pPlayer = (C_BasePlayer*)m_hPlayer.Get();

	if ( pPlayer && !pPlayer->IsDormant() )
		pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
}

void C_BasePlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() && prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
			return;

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}

void C_BasePlayer::CalculateIKLocks(float currentTime)
{
	BaseClass::CalculateIKLocks( currentTime );
}

void C_BasePlayer::UpdateClientSideAnimation()
{
	// Keep the model upright; pose params will handle pitch aiming.
	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

int C_BasePlayer::GetVisionFilterFlags()
{
	// Force our vision filter to a specific setting
	if ( m_nForceVisionFilterFlags != 0 )
	{
		return m_nForceVisionFilterFlags;
	}

	int nVisionOptInFlags = UTIL_GetActiveHolidaysVisionFilter();

	return nVisionOptInFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BasePlayer::CalculateVisionUsingCurrentFlags( void )
{
	if ( IsLocalPlayer() )
	{
		for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
		{
			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iClient );
			if ( !pPlayer )
				continue;

			if ( !pPlayer->IsAlive() )
				continue;

			if ( pPlayer->GetActiveWeapon() )
			{
				pPlayer->GetActiveWeapon()->RestartParticleEffect();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BasePlayer::CanSetSoundMixer( void )
{
	// Can't set sound mixers when we're in freezecam mode, since it has a code-enforced mixer
	return (GetObserverMode() != OBS_MODE_FREEZECAM);
}

void C_BasePlayer::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();

	switch( messageType )
	{
		case PLAY_PLAYER_JINGLE:
			PlayPlayerJingle();
			break;
	}
}

void C_BasePlayer::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	if(state == SHOULDTRANSMIT_END) {
		TurnOffFlashlight();
	}

	BaseClass::NotifyShouldTransmit( state );
}

//-----------------------------------------------------------------------------
// Purpose: Process incoming data
//-----------------------------------------------------------------------------
void C_BasePlayer::OnDataChanged( DataUpdateType_t updateType )
{
	if ( IsLocalPlayer() )
	{
		SetPredictionEligible( true );
	}

	BaseClass::OnDataChanged( updateType );

	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		// Reset engine areabits pointer
		render->SetAreaState( m_Local.m_chAreaBits, m_Local.m_chAreaPortalBits );

		// Check for Ammo pickups.
		int ammoTypes = GetAmmoDef()->NumAmmoTypes();
		for ( int i = 0; i <= ammoTypes; i++ )
		{
			if ( GetAmmoCount(i) > m_iOldAmmo[i] )
			{
				// Don't add to ammo pickup if the ammo doesn't do it
				const FileWeaponInfo_t *pWeaponData = gWR.GetWeaponFromAmmo(i);

				if ( !pWeaponData || !( pWeaponData->iFlags & ITEM_FLAG_NOAMMOPICKUPS ) )
				{
					// We got more ammo for this ammo index. Add it to the ammo history
					CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
					if( pHudHR )
					{
						pHudHR->AddToHistory( HISTSLOT_AMMO, i, abs(GetAmmoCount(i) - m_iOldAmmo[i]) );
					}
				}
			}
		}

		Soundscape_Update( m_Local.m_audio );

		if ( m_hOldFogController.Get() != m_Local.m_PlayerFog.m_hCtrl.Get() )
		{
			FogControllerChanged( updateType == DATA_UPDATE_CREATED );
		}
	}

	if(updateType == DATA_UPDATE_CREATED)
	{
		SetContextThink( &C_BasePlayer::ThinkIDTarget, TICK_ALWAYS_THINK, "ThinkIDTarget" );
	}
}

void C_BasePlayer::ThinkIDTarget()
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors(GetLocalAngles(), &vForward);

	for(int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient) {
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if(!pEnt || !pEnt->IsPlayer()) {
			continue;
		}

		if(pEnt->entindex() == entindex()) {
			continue;
		}

		Vector vTargetOrigin = pEnt->GetAbsOrigin();
		Vector vMyOrigin =  GetAbsOrigin();

		Vector vDir = vTargetOrigin - vMyOrigin;
		
		if(vDir.Length() > 128) {
			continue;
		}

		VectorNormalize(vDir);

		if(DotProduct(vForward, vDir) < 0.0f) {
			continue;
		}

		m_vLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if(bFoundViewTarget == false) {
		m_vLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	m_viewtarget = m_vLookAtTarget;

	UpdateIDTarget();
}

void C_BasePlayer::UpdateIDTarget()
{
	if(!IsLocalPlayer()) {
		return;
	}

	m_iIDEntIndex = 0;

	if(GetObserverMode() == OBS_MODE_CHASE ||
		GetObserverMode() == OBS_MODE_DEATHCAM) {
		return;
	}

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA(MainViewOrigin(), 1500, MainViewForward(), vecEnd);
	VectorMA(MainViewOrigin(), 10, MainViewForward(), vecStart);
	UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	if(!tr.startsolid && tr.DidHitNonWorldEntity()) {
		C_BaseEntity *pEntity = tr.m_pEnt;

		if(pEntity && (pEntity != this)) {
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Did we just enter a vehicle this frame?
//-----------------------------------------------------------------------------
bool C_BasePlayer::JustEnteredVehicle()
{
	if ( !IsInAVehicle() )
		return false;

	return ( m_hOldVehicle == m_hVehicle );
}

//-----------------------------------------------------------------------------
// Are we in VGUI input mode?.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsInVGuiInputMode() const
{
	return (m_pCurrentVguiScreen.Get() != NULL);
}

//-----------------------------------------------------------------------------
// Are we inputing to a view model vgui screen
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsInViewModelVGuiInputMode() const
{
	C_BaseEntity *pScreenEnt = m_pCurrentVguiScreen.Get();

	if ( !pScreenEnt )
		return false;

	Assert( dynamic_cast<C_VGuiScreen*>(pScreenEnt) );
	C_VGuiScreen *pVguiScreen = static_cast<C_VGuiScreen*>(pScreenEnt);

	return ( pVguiScreen->IsAttachedToViewModel() && pVguiScreen->AcceptsInput() );
}

//-----------------------------------------------------------------------------
// Check to see if we're in vgui input mode...
//-----------------------------------------------------------------------------
void C_BasePlayer::DetermineVguiInputMode( CUserCmd *pCmd )
{
	// If we're dead, close down and abort!
	if ( !IsAlive() )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// If we're in vgui mode *and* we're holding down mouse buttons,
	// stay in vgui mode even if we're outside the screen bounds
	if (m_pCurrentVguiScreen.Get() && (pCmd->buttons & (IN_ATTACK | IN_ATTACK2)) )
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
		return;
	}

	// We're not in vgui input mode if we're moving, or have hit a key
	// that will make us move...

	// Not in vgui mode if we're moving too quickly
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if (GetVelocity().LengthSqr() > MAX_VGUI_INPUT_MODE_SPEED_SQ)
	if ( 0 )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Don't enter vgui mode if we've got combat buttons held down
	bool bAttacking = false;
	if ( ((pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2)) && !m_pCurrentVguiScreen.Get() )
	{
		bAttacking = true;
	}

	// Not in vgui mode if we're pushing any movement key at all
	// Not in vgui mode if we're in a vehicle...
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if ((pCmd->forwardmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->sidemove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->upmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->buttons & IN_JUMP) ||
	//	(bAttacking) )
	if ( bAttacking || IsInAVehicle() )
	{ 
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Don't interact with world screens when we're in a menu
	if ( vgui::surface()->IsCursorVisible() )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Not in vgui mode if there are no nearby screens
	C_BaseEntity *pOldScreen = m_pCurrentVguiScreen.Get();

	m_pCurrentVguiScreen = FindNearbyVguiScreen( EyePosition(), pCmd->viewangles, GetTeamNumber() );

	if (pOldScreen != m_pCurrentVguiScreen)
	{
		DeactivateVguiScreen( pOldScreen );
		ActivateVguiScreen( m_pCurrentVguiScreen.Get() );
	}

	if (m_pCurrentVguiScreen.Get())
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handling
//-----------------------------------------------------------------------------
bool C_BasePlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{
	// Allow the vehicle to clamp the view angles
	if ( IsInAVehicle() )
	{
		IClientVehicle *pVehicle = m_hVehicle.Get()->GetClientVehicle();
		if ( pVehicle )
		{
			pVehicle->UpdateViewAngles( this, pCmd );
			engine->SetViewAngles( pCmd->viewangles );
		}
	}
	else 
	{
		if ( joy_autosprint.GetBool() )
		{
			if ( input->KeyState( &in_joyspeed ) != 0.0f )
			{
				pCmd->buttons |= IN_SPEED;
			}
		}

		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->CreateMove( flInputSampleTime, pCmd, m_vecOldViewAngles );
		}
	}

	// If the frozen flag is set, prevent view movement (server prevents the rest of the movement)
	if ( GetFlags() & FL_FROZEN )
	{
		// Don't stomp the first time we get frozen
		if ( m_bWasFrozen )
		{
			// Stomp the new viewangles with old ones
			pCmd->viewangles = m_vecOldViewAngles;
			engine->SetViewAngles( pCmd->viewangles );
		}
		else
		{
			m_bWasFrozen = true;
		}
	}
	else
	{
		m_bWasFrozen = false;
	}

	m_vecOldViewAngles = pCmd->viewangles;
	
	// Check to see if we're in vgui input mode...
	DetermineVguiInputMode( pCmd );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Player has changed to a new team
//-----------------------------------------------------------------------------
void C_BasePlayer::TeamChange( int iNewTeam )
{
	// Base class does nothing
}


//-----------------------------------------------------------------------------
// Purpose: Creates, destroys, and updates the flashlight effect as needed.
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFlashlight()
{
	// TERROR: if we're in-eye spectating, use that player's flashlight
	C_BasePlayer *pFlashlightPlayer = this;
	if ( !IsAlive() )
	{
		if ( GetObserverMode() == OBS_MODE_IN_EYE )
		{
			pFlashlightPlayer = ToBasePlayer( GetObserverTarget() );
		}
	}

	if ( pFlashlightPlayer )
	{
		FlashlightEffectManager().SetEntityIndex( pFlashlightPlayer->entindex() );
	}

	// The dim light is the flashlight.
	if ( pFlashlightPlayer && pFlashlightPlayer->IsAlive() && pFlashlightPlayer->IsEffectActive( EF_DIMLIGHT ) && !pFlashlightPlayer->GetViewEntity() )
	{
		// Make sure we're using the proper flashlight texture
		const char *pszTextureName = pFlashlightPlayer->GetFlashlightTextureName();
		if ( !m_bFlashlightEnabled )
		{
			// Turned on the headlight; create it.
			if ( pszTextureName )
			{
				FlashlightEffectManager().TurnOnFlashlight( pFlashlightPlayer->entindex(), pszTextureName, pFlashlightPlayer->GetFlashlightFOV(),
					pFlashlightPlayer->GetFlashlightFarZ(), pFlashlightPlayer->GetFlashlightLinearAtten() );
			}
			else
			{
				FlashlightEffectManager().TurnOnFlashlight( pFlashlightPlayer->entindex() );
			}

			if(!m_pFlashlightBeam) {
				int iAttachment = pFlashlightPlayer->LookupAttachment( "anim_attachment_RH" );
				if(iAttachment > 0) {
					Vector vecOrigin;
					QAngle eyeAngles = pFlashlightPlayer->m_angEyeAngles;
					pFlashlightPlayer->GetAttachment(iAttachment, vecOrigin, eyeAngles);

					Vector vForward;
					AngleVectors(eyeAngles, &vForward);

					trace_t tr;
					UTIL_TraceLine(vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, pFlashlightPlayer, COLLISION_GROUP_NONE, &tr);

					BeamInfo_t beamInfo;
					beamInfo.m_nType = TE_BEAMPOINTS;
					beamInfo.m_vecStart = tr.startpos;
					beamInfo.m_vecEnd = tr.endpos;
					beamInfo.m_pszModelName = "sprites/glow01.vmt";
					beamInfo.m_pszHaloName = "sprites/glow01.vmt";
					beamInfo.m_flHaloScale = 3.0;
					beamInfo.m_flWidth = 8.0f;
					beamInfo.m_flEndWidth = 35.0f;
					beamInfo.m_flFadeLength = 300.0f;
					beamInfo.m_flAmplitude = 0;
					beamInfo.m_flBrightness = 60.0;
					beamInfo.m_flSpeed = 0.0f;
					beamInfo.m_nStartFrame = 0.0;
					beamInfo.m_flFrameRate = 0.0;
					beamInfo.m_flRed = 255.0;
					beamInfo.m_flGreen = 255.0;
					beamInfo.m_flBlue = 255.0;
					beamInfo.m_nSegments = 8;
					beamInfo.m_bRenderable = true;
					beamInfo.m_flLife = 0.5;
					beamInfo.m_nFlags = FBEAM_FOREVER|FBEAM_ONLYNOISEONCE|FBEAM_NOTILE|FBEAM_HALOBEAM;
					m_pFlashlightBeam = beams->CreateBeamPoints(beamInfo);
				}
			}

			m_bFlashlightEnabled = true;
		}
	}
	else if ( m_bFlashlightEnabled )
	{
		// Turned off the flashlight; delete it.
		TurnOffFlashlight();
	}

	if ( pFlashlightPlayer && m_bFlashlightEnabled )
	{
		Vector vecForward, vecRight, vecUp;
		Vector vecPos;
		//Check to see if we have an externally specified flashlight origin, if not, use eye vectors/render origin
		if ( pFlashlightPlayer->m_vecFlashlightOrigin != vec3_origin && pFlashlightPlayer->m_vecFlashlightOrigin.IsValid() )
		{
			vecPos = pFlashlightPlayer->m_vecFlashlightOrigin;
			vecForward = pFlashlightPlayer->m_vecFlashlightForward;
			vecRight = pFlashlightPlayer->m_vecFlashlightRight;
			vecUp = pFlashlightPlayer->m_vecFlashlightUp;
		}
		else
		{
			EyeVectors( &vecForward, &vecRight, &vecUp );
			vecPos = GetRenderOrigin() + m_vecViewOffset;
		}

		// Update the light with the new position and direction.		
		FlashlightEffectManager().UpdateFlashlight( vecPos, vecForward, vecRight, vecUp, pFlashlightPlayer->GetFlashlightFOV(), 
			pFlashlightPlayer->CastsFlashlightShadows(), pFlashlightPlayer->GetFlashlightFarZ(), pFlashlightPlayer->GetFlashlightLinearAtten(),
			pFlashlightPlayer->GetFlashlightTextureName() );

		if(m_pFlashlightBeam) {
			int iAttachment = pFlashlightPlayer->LookupAttachment( "anim_attachment_RH" );
			if(iAttachment > 0) {
				Vector vecOrigin;
				QAngle eyeAngles = pFlashlightPlayer->m_angEyeAngles;
				pFlashlightPlayer->GetAttachment(iAttachment, vecOrigin, eyeAngles);

				Vector vForward;
				AngleVectors(eyeAngles, &vForward);

				trace_t tr;
				UTIL_TraceLine(vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, pFlashlightPlayer, COLLISION_GROUP_NONE, &tr);

				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beams->UpdateBeamInfo(m_pFlashlightBeam, beamInfo);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turns off flashlight if it's active (TERROR)
//-----------------------------------------------------------------------------
void C_BasePlayer::TurnOffFlashlight( void )
{
	if ( m_bFlashlightEnabled )
	{
		if(m_pFlashlightBeam) {
			m_pFlashlightBeam->flags = 0;
			m_pFlashlightBeam->die = gpGlobals->curtime - 1;

			m_pFlashlightBeam = NULL;
		}

		FlashlightEffectManager().TurnOffFlashlight();
		m_bFlashlightEnabled = false;
	}
}

extern float UTIL_WaterLevel( const Vector &position, float minz, float maxz );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::CreateWaterEffects( void )
{
	// Must be completely submerged to bother
	if ( GetWaterLevel() < 3 )
	{
		m_bResampleWaterSurface = true;
		return;
	}

	// Do special setup if this is our first time back underwater
	if ( m_bResampleWaterSurface )
	{
		// Reset our particle timer
		m_tWaterParticleTimer.Init( 32 );
		
		// Find the surface of the water to clip against
		m_flWaterSurfaceZ = UTIL_WaterLevel( WorldSpaceCenter(), WorldSpaceCenter().z, WorldSpaceCenter().z + 256 );
		m_bResampleWaterSurface = false;
	}

	// Make sure the emitter is setup
	if ( m_pWaterEmitter == NULL )
	{
		m_pWaterEmitter = WaterDebrisEffect::Create( "splish" );
		if ( !m_pWaterEmitter.IsValid() )
			return;
	}

	Vector vecVelocity;
	GetVectors( &vecVelocity, NULL, NULL );

	Vector offset = WorldSpaceCenter();

	m_pWaterEmitter->SetSortOrigin( offset );

	SimpleParticle	*pParticle;

	float curTime = gpGlobals->frametime;

	// Add as many particles as we need
	while ( m_tWaterParticleTimer.NextEvent( curTime ) )
	{
		offset = WorldSpaceCenter() + ( vecVelocity * 128.0f ) + RandomVector( -128, 128 );

		// Make sure we don't start out of the water!
		if ( offset.z > m_flWaterSurfaceZ )
		{
			offset.z = ( m_flWaterSurfaceZ - 8.0f );
		}

		pParticle = (SimpleParticle *) m_pWaterEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_Fleck_Cement[random->RandomInt(0,1)], offset );

		if (pParticle == NULL)
			continue;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= random->RandomFloat( 2.0f, 4.0f );

		pParticle->m_vecVelocity = RandomVector( -2.0f, 2.0f );

		//FIXME: We should tint these based on the water's fog value!
		float color = random->RandomInt( 32, 128 );
		pParticle->m_uchColor[0] = color;
		pParticle->m_uchColor[1] = color;
		pParticle->m_uchColor[2] = color;

		pParticle->m_uchStartSize	= 1;
		pParticle->m_uchEndSize		= 1;
		
		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -0.5f, 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
//-----------------------------------------------------------------------------
void C_BasePlayer::OverrideView( CViewSetup *pSetup )
{
}

bool C_BasePlayer::ShouldInterpolate()
{
	// always interpolate myself
	if ( IsLocalPlayer() )
		return true;
	// always interpolate entity if followed by HLTV/Replay
#if defined( REPLAY_ENABLED )
	if ( HLTVCamera()->GetCameraMan() == this ||
		 ReplayCamera()->GetCameraMan() == this )
		return true;
#else
	// always interpolate entity if followed by HLTV
	if ( HLTVCamera()->GetCameraMan() == this )
		return true;
#endif
	return BaseClass::ShouldInterpolate();
}


bool C_BasePlayer::ShouldDraw()
{
	return ShouldDrawThisPlayer() && BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BasePlayer::ShouldPlayerDrawParticles( void )
{
	return true;
}

int C_BasePlayer::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if(!ReadyToDraw()) {
		return 0;
	}
#ifndef PORTAL
	// In Portal this check is already performed as part of
	// C_Portal_Player::DrawModel()
	if ( !ShouldDrawThisPlayer() )
	{
		return 0;
	}
#endif
	return BaseClass::DrawModel( flags, instance );
}

//-----------------------------------------------------------------------------
// Computes the render mode for this player
//-----------------------------------------------------------------------------
PlayerRenderMode_t C_BasePlayer::GetPlayerRenderMode()
{
	// check if local player chases owner of this weapon in first person
	C_BasePlayer *pLocalPlayer = GetLocalPlayer();
	if ( !pLocalPlayer )
		return PLAYER_RENDER_THIRDPERSON;

	if ( pLocalPlayer->IsObserver() )
	{
		if ( pLocalPlayer->GetObserverTarget() != this )
			return PLAYER_RENDER_THIRDPERSON;
		if ( pLocalPlayer->GetObserverMode() != OBS_MODE_IN_EYE )
			return PLAYER_RENDER_THIRDPERSON;
	}
	else
	{
		if ( pLocalPlayer != this )
			return PLAYER_RENDER_THIRDPERSON;
	}

	if ( input->CAM_IsThirdPerson() )
		return PLAYER_RENDER_THIRDPERSON;

//	if ( IsInThirdPersonView() )
//		return PLAYER_RENDER_THIRDPERSON;

	return PLAYER_RENDER_FIRSTPERSON;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_BasePlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	C_BasePlayer *player = ToBasePlayer( target );
	
	if ( player )
	{
		if ( player->IsAlive() )
		{
			if ( player->GetFlags() & FL_DUCKING )
			{
				return VEC_DUCK_VIEW_SCALED( player );
			}

			return VEC_VIEW_SCALED( player );
		}
		else
		{
			// assume it's the players ragdoll
			return VEC_DEAD_VIEWHEIGHT_SCALED( player );
		}
	}

	// assume it's the players ragdoll
	return VEC_DEAD_VIEWHEIGHT;
}

void C_BasePlayer::CalcChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BaseEntity *target = GetObserverTarget();

	if ( !target ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	// If our target isn't visible, we're at a camera point of some kind.
	// Instead of letting the player rotate around an invisible point, treat
	// the point as a fixed camera.
	if ( !target->GetBaseAnimating() && !target->GetModel() )
	{
		CalcRoamingView( eyeOrigin, eyeAngles, fov );
		return;
	}

	// QAngle tmpangles;

	Vector forward, viewpoint;

	// GetObserverCamOrigin() returns ragdoll pos if player is ragdolled
	Vector origin = target->GetObserverCamOrigin();

	VectorAdd( origin, GetChaseCamViewOffset( target ), origin );

	QAngle viewangles;

	if ( GetObserverMode() == OBS_MODE_IN_EYE )
	{
		viewangles = eyeAngles;
	}
	else if ( IsLocalPlayer() )
	{
		engine->GetViewAngles( viewangles );
	}
	else
	{
		viewangles = EyeAngles();
	}

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;

	float flMinDistance = CHASE_CAM_DISTANCE_MIN;
	float flMaxDistance = CHASE_CAM_DISTANCE_MAX;
	
	if ( target && target->IsBaseTrain() )
	{
		// if this is a train, we want to be back a little further so we can see more of it
		flMaxDistance *= 2.5f;
	}

	if ( target )
	{
		C_BaseAnimating *pTargetAnimating = target->GetBaseAnimating();
		if ( pTargetAnimating )
		{
			float flScaleSquared = pTargetAnimating->GetModelScale() * pTargetAnimating->GetModelScale();
			flMinDistance *= flScaleSquared;
			flMaxDistance *= flScaleSquared;
			m_flObserverChaseDistance = flMaxDistance;
		}
	}

	if ( target && !target->IsPlayer() && target->IsNPC() )
	{
		// if this is a boss, we want to be back a little further so we can see more of it
		flMaxDistance *= 2.5f;
		m_flObserverChaseDistance = flMaxDistance;
	}

	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, flMinDistance, flMaxDistance );
	
	AngleVectors( viewangles, &forward );

	VectorNormalize( forward );

	VectorMA(origin, -m_flObserverChaseDistance, forward, viewpoint );

	trace_t trace;
	CTraceFilterNoNPCsOrPlayer filter( target, COLLISION_GROUP_NONE );
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, viewpoint, WALL_MIN, WALL_MAX, MASK_SOLID, &filter, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if (trace.fraction < 1.0)
	{
		viewpoint = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}
	
	VectorCopy( viewangles, eyeAngles );
	VectorCopy( viewpoint, eyeOrigin );

	fov = GetFOV();
}

void C_BasePlayer::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BaseEntity *target = GetObserverTarget();
	
	if ( !target ) 
	{
		target = this;
	}

	m_flObserverChaseDistance = 0.0;

	eyeOrigin = target->EyePosition();
	eyeAngles = target->EyeAngles();
	
	if ( spec_track.GetInt() > 0 )
	{
		C_BaseEntity *target =  ClientEntityList().GetBaseEntity( spec_track.GetInt() );

		if ( target )
		{
			Vector v = target->GetAbsOrigin(); v.z += 54;
			QAngle a; VectorAngles( v - eyeOrigin, a );

			NormalizeAngles( a );
			eyeAngles = a;
			engine->SetViewAngles( a );
		}
	}

	// Apply a smoothing offset to smooth out prediction errors.
	Vector vSmoothOffset;
	GetPredictionErrorSmoothingVector( vSmoothOffset );
	eyeOrigin += vSmoothOffset;

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the view for the player while he's in freeze frame observer mode
//-----------------------------------------------------------------------------
void C_BasePlayer::CalcFreezeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	C_BaseEntity *pTarget = GetObserverTarget();
	if ( !pTarget )
	{
		CalcDeathCamView( eyeOrigin, eyeAngles, fov );
		return;
	}

	// Zoom towards our target
	float flCurTime = (gpGlobals->curtime - m_flFreezeFrameStartTime);
	float flBlendPerc = clamp( flCurTime / spec_freeze_traveltime.GetFloat(), 0.f, 1.f );
	flBlendPerc = SimpleSpline( flBlendPerc );

	Vector vecCamDesired = pTarget->GetObserverCamOrigin();	// Returns ragdoll origin if they're ragdolled
	VectorAdd( vecCamDesired, GetChaseCamViewOffset( pTarget ), vecCamDesired );
	Vector vecCamTarget = vecCamDesired;
	if ( pTarget->IsAlive() )
	{
		// Look at their chest, not their head
		Vector maxs = pTarget->GetBaseAnimating() ? VEC_HULL_MAX_SCALED( pTarget->GetBaseAnimating() ) : VEC_HULL_MAX;
		vecCamTarget.z -= (maxs.z * 0.5);
	}
	else
	{
		vecCamTarget.z += pTarget->GetBaseAnimating() ? VEC_DEAD_VIEWHEIGHT_SCALED( pTarget->GetBaseAnimating() ).z : VEC_DEAD_VIEWHEIGHT.z;	// look over ragdoll, not through
	}

	// Figure out a view position in front of the target
	Vector vecEyeOnPlane = eyeOrigin;
	vecEyeOnPlane.z = vecCamTarget.z;
	Vector vecTargetPos = vecCamTarget;
	Vector vecToTarget = vecTargetPos - vecEyeOnPlane;
	VectorNormalize( vecToTarget );

	// Stop a few units away from the target, and shift up to be at the same height
	vecTargetPos = vecCamTarget - (vecToTarget * m_flFreezeFrameDistance);
	float flEyePosZ = pTarget->EyePosition().z;
	vecTargetPos.z = flEyePosZ + m_flFreezeZOffset;

	// Now trace out from the target, so that we're put in front of any walls
	trace_t trace;
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( vecCamTarget, vecTargetPos, WALL_MIN, WALL_MAX, MASK_SOLID, pTarget, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();
	if (trace.fraction < 1.0)
	{
		// The camera's going to be really close to the target. So we don't end up
		// looking at someone's chest, aim close freezecams at the target's eyes.
		vecTargetPos = trace.endpos;
		vecCamTarget = vecCamDesired;

		// To stop all close in views looking up at character's chins, move the view up.
		vecTargetPos.z += fabs(vecCamTarget.z - vecTargetPos.z) * 0.85;
		C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
		UTIL_TraceHull( vecCamTarget, vecTargetPos, WALL_MIN, WALL_MAX, MASK_SOLID, pTarget, COLLISION_GROUP_NONE, &trace );
		C_BaseEntity::PopEnableAbsRecomputations();
		vecTargetPos = trace.endpos;
	}

	// Look directly at the target
	vecToTarget = vecCamTarget - vecTargetPos;
	VectorNormalize( vecToTarget );
	VectorAngles( vecToTarget, eyeAngles );
	
	VectorLerp( m_vecFreezeFrameStart, vecTargetPos, flBlendPerc, eyeOrigin );

	if ( flCurTime >= spec_freeze_traveltime.GetFloat() && !m_bSentFreezeFrame )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "freezecam_started" );
		if ( pEvent )
		{
			gameeventmanager->FireEventClientSide( pEvent );
		}

		m_bSentFreezeFrame = true;
		GetViewRenderInstance()->FreezeFrame( spec_freeze_time.GetFloat() );
	}
}

void C_BasePlayer::CalcInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	C_BaseEntity *target = GetObserverTarget();

	if ( !target ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	if ( !target->IsAlive() )
	{
		// if dead, show from 3rd person
		CalcChaseCamView( eyeOrigin, eyeAngles, fov );
		return;
	}

	fov = GetFOV();	// TODO use tragets FOV

	m_flObserverChaseDistance = 0.0;

	eyeAngles = target->EyeAngles();
	eyeOrigin = target->GetAbsOrigin();

	// Apply punch angle
	VectorAdd( eyeAngles, GetPunchAngle(), eyeAngles );

#if defined( REPLAY_ENABLED )
	if( engine->IsHLTV() || g_pEngineClientReplay->IsPlayingReplayDemo() )
#else
	if( engine->IsHLTV() )
#endif
	{
		C_BaseAnimating *pTargetAnimating = target->GetBaseAnimating();
		if ( target->GetFlags() & FL_DUCKING )
		{
			eyeOrigin += pTargetAnimating ? VEC_DUCK_VIEW_SCALED( pTargetAnimating ) : VEC_DUCK_VIEW;
		}
		else
		{
			eyeOrigin += pTargetAnimating ? VEC_VIEW_SCALED( pTargetAnimating ) : VEC_VIEW;
		}
	}
	else
	{
		Vector offset = GetViewOffset();
#ifdef HL2MP
		offset = target->GetViewOffset();
#endif
		eyeOrigin += offset; // hack hack
	}

	engine->SetViewAngles( eyeAngles );
}

float C_BasePlayer::GetDeathCamInterpolationTime()
{
	return DEATH_ANIMATION_TIME;
}


void C_BasePlayer::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	CBaseEntity	* pKiller = NULL; 

	if ( mp_forcecamera.GetInt() == OBS_ALLOW_ALL )
	{
		// if mp_forcecamera is off let user see killer or look around
		pKiller = GetObserverTarget();
		eyeAngles = EyeAngles();
	}

	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / GetDeathCamInterpolationTime();
	interpolation = clamp( interpolation, 0.0f, 1.0f );

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, ( CHASE_CAM_DISTANCE_MIN * 2 ), CHASE_CAM_DISTANCE_MAX );

	QAngle aForward = eyeAngles;
	Vector origin = EyePosition();			

	// NOTE:  This will create the ragdoll in CSS if m_hRagdoll is set, but m_pRagdoll is not yet presetn
	IRagdoll *pRagdoll = GetRepresentativeRagdoll();
	if ( pRagdoll )
	{
		origin = pRagdoll->GetRagdollOrigin();
		origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z;
	}
	
	if ( pKiller && pKiller->IsPlayer() && (pKiller != this) ) 
	{														
		Vector vKiller = pKiller->EyePosition() - origin;
		QAngle aKiller; VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	};

	Vector vForward; AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if (trace.fraction < 1.0)
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}

	fov = GetFOV();
}



//-----------------------------------------------------------------------------
// Purpose: Return the weapon to have open the weapon selection on, based upon our currently active weapon
//			Base class just uses the weapon that's currently active.
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *C_BasePlayer::GetActiveWeaponForSelection( void )
{
	return GetActiveWeapon();
}

C_BaseAnimating* C_BasePlayer::GetRenderedWeaponModel()
{
	// Attach to either their weapon model or their view model.
	if ( ShouldDrawLocalPlayer() || !IsLocalPlayer() )
	{
		return GetActiveWeapon();
	}
	else
	{
		return GetViewModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets a pointer to the local player, if it exists yet.
// Output : C_BasePlayer
//-----------------------------------------------------------------------------
C_BasePlayer *C_BasePlayer::GetLocalPlayer( void )
{
	return s_pLocalPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bThirdperson - 
//-----------------------------------------------------------------------------
void C_BasePlayer::ThirdPersonSwitch( bool bThirdperson )
{
	// We've switch from first to third, or vice versa.
	UpdateVisibility();

	// Update the visibility of anything bone attached to us.
	if ( IsLocalPlayer() )
	{
		bool bShouldDrawLocalPlayer = ShouldDrawLocalPlayer();
		for ( int i=0; i<GetNumBoneAttachments(); ++i )
		{
			C_BaseAnimating* pBoneAttachment = GetBoneAttachment( i );
			if ( pBoneAttachment )
			{
				if ( bShouldDrawLocalPlayer )
				{
					pBoneAttachment->RemoveEffects( EF_NODRAW );
				}
				else
				{
					pBoneAttachment->AddEffects( EF_NODRAW );
				}
			}
		}
		
		//Notify weapon.
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->ThirdPersonSwitch( bThirdperson );
		}
	}
}

//----------------------------------------------------------------------------
// Hooks into the fast path render system
//----------------------------------------------------------------------------
IClientModelRenderable *C_BasePlayer::GetClientModelRenderable()
{ 
	// Honor base class eligibility
	if ( !BaseClass::GetClientModelRenderable() )
		return NULL;

	// No fast path for firstperson local players
	if ( IsLocalPlayer() )
	{
		bool bThirdPerson = input->CAM_IsThirdPerson() || ( ToolsEnabled() && ToolFramework_IsThirdPersonCamera() );
		if ( !bThirdPerson )
		{
			return NULL;
		}
	}

	// if local player is spectating this player in first person mode, don't use fast path, so we can skip drawing it
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( localPlayer && localPlayer->IsObserver() )
	{
		if ( localPlayer->GetObserverMode() == OBS_MODE_IN_EYE &&
			localPlayer->GetObserverTarget() == this &&
			!input->CAM_IsThirdPerson() )
			return NULL;
	}

	// don't use fastpath for teammates (causes extra work for glows)
	if ( localPlayer && localPlayer->GetTeamNumber() == GetTeamNumber() )
	{
		return NULL;
	}

	return this; 
}

//-----------------------------------------------------------------------------
// Purpose: single place to decide whether the camera is in the first-person position
//          NOTE - ShouldDrawLocalPlayer() can be true even if the camera is in the first-person position, e.g. in VR.
//-----------------------------------------------------------------------------
/*static*/ bool C_BasePlayer::LocalPlayerInFirstPersonView()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer == NULL )
	{
		return false;
	}
	int ObserverMode = pLocalPlayer->GetObserverMode();
	if ( ( ObserverMode == OBS_MODE_NONE ) || ( ObserverMode == OBS_MODE_IN_EYE ) )
	{
		return !input->CAM_IsThirdPerson() && ( !ToolsEnabled() || !ToolFramework_IsThirdPersonCamera() );
	}

	// Not looking at the local player, e.g. in a replay in third person mode or freelook.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: single place to decide whether the local player should draw
//-----------------------------------------------------------------------------
/*static*/ bool C_BasePlayer::ShouldDrawLocalPlayer()
{
	return !LocalPlayerInFirstPersonView() || cl_first_person_uses_world_model.GetBool();
}



//-----------------------------------------------------------------------------
// Purpose: single place to decide whether the camera is in the first-person position
//          NOTE - ShouldDrawLocalPlayer() can be true even if the camera is in the first-person position, e.g. in VR.
//-----------------------------------------------------------------------------
bool C_BasePlayer::InFirstPersonView()
{
	if ( IsLocalPlayer() )
	{
		return LocalPlayerInFirstPersonView();
	}
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer == NULL )
	{
		return false;
	}
	// If this is who we're observing in first person, it's counted as the "local" player.
	if ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalPlayer->GetObserverTarget() == ToBasePlayer(this) )
	{
		return LocalPlayerInFirstPersonView();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: single place to decide whether the player is being drawn with the standard model (i.e. not the viewmodel)
//          NOTE - ShouldDrawLocalPlayer() can be true even if the camera is in the first-person position, e.g. in VR.
//-----------------------------------------------------------------------------
bool C_BasePlayer::ShouldDrawThisPlayer()
{
	if ( IsRagdoll() )
	{
		return false;
	}

	if ( !IsAlive() )
	{
		return false;
	}

	if ( !InFirstPersonView() )
	{
		return true;
	}

	if ( cl_first_person_uses_world_model.GetBool() )
	{
		return true;
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsLocalPlayer( const C_BaseEntity *pl )
{
	return ( GetLocalPlayer() == pl );
}

bool C_BasePlayer::IsLocalPlayer( void ) const
{
	return ( GetLocalPlayer() == this );
}

int	C_BasePlayer::GetUserID( void ) const
{
	player_info_t pi;

	if ( !engine->GetPlayerInfo( entindex(), &pi ) )
		return -1;

	return pi.userID;
}

void C_BasePlayer::UpdateClientData( void )
{
	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}
}

// Prediction stuff
void C_BasePlayer::PreThink( void )
{
	ItemPreFrame();

	UpdateClientData();

	UpdateUnderwaterState();

	// Update the player's fog data if necessary.
	UpdateFogController();

	if (m_lifeState >= LIFE_DYING)
		return;

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}
}

void C_BasePlayer::PostThink( void )
{
	MDLCACHE_CRITICAL_SECTION();

	if ( IsAlive())
	{
		// Need to do this on the client to avoid prediction errors
		UpdateCollisionBounds();
		
		// do weapon stuff
		ItemPostFrame();

		if ( GetFlags() & FL_ONGROUND )
		{		
			m_Local.m_flFallVelocity = 0;
		}

		// Don't allow bogus sequence on player
		if ( GetSequence() == -1 )
		{
			SetSequence( 0 );
		}

		StudioFrameAdvance();
	}

	// Even if dead simulate entities
	SimulatePlayerSimulatedEntities();
}

//-----------------------------------------------------------------------------
// Purpose: send various tool messages - viewoffset, and base class messages (flex and bones)
//-----------------------------------------------------------------------------
void C_BasePlayer::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_BasePlayer::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	BaseClass::GetToolRecordingState( msg );

	msg->SetBool( "baseplayer", true );
	msg->SetBool( "localplayer", IsLocalPlayer() );
	msg->SetString( "playername", GetPlayerName() );

	static CameraRecordingState_t state;
	state.m_flFOV = GetFOV();

	float flZNear = GetViewRenderInstance()->GetZNear();
	float flZFar = GetViewRenderInstance()->GetZFar();
	CalcView( state.m_vecEyePosition, state.m_vecEyeAngles, flZNear, flZFar, state.m_flFOV );
	state.m_bThirdPerson = !engine->IsPaused() && ::input->CAM_IsThirdPerson();

	// this is a straight copy from ClientModeShared::OverrideView,
	// When that method is removed in favor of rolling it into CalcView,
	// then this code can (should!) be removed
	if ( state.m_bThirdPerson )
	{
		const Vector& cam_ofs = g_ThirdPersonManager.GetCameraOffsetAngles();
		
		QAngle camAngles;
		camAngles[ PITCH ] = cam_ofs[ PITCH ];
		camAngles[ YAW ] = cam_ofs[ YAW ];
		camAngles[ ROLL ] = 0;

		Vector camForward, camRight, camUp;
		AngleVectors( camAngles, &camForward, &camRight, &camUp );

		VectorMA( state.m_vecEyePosition, -cam_ofs[ ROLL ], camForward, state.m_vecEyePosition );

		// Override angles from third person camera
		VectorCopy( camAngles, state.m_vecEyeAngles );
	}

	msg->SetPtr( "camera", &state );
}


//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
bool C_BasePlayer::Simulate()
{
	//Frame updates
	if ( this == C_BasePlayer::GetLocalPlayer() )
	{
		//Update the flashlight
		UpdateFlashlight();

		// Update the player's fog data if necessary.
		UpdateFogController();
	}
	else
	{
		// update step sounds for all other players
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}

	bool ret = BaseClass::Simulate();

	if ( IsNoInterpolationFrame() || Teleported() )
	{
		ResetLatched();
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseViewModel
//		Consider using GetRenderedWeaponModel() instead - it will get the
//		viewmodel or the active weapon as appropriate.
//-----------------------------------------------------------------------------
C_BaseViewModel *C_BasePlayer::GetViewModel( int index /*= 0*/, bool bObserverOK )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	C_BaseViewModel *vm = m_hViewModel[ index ];
	
	if ( bObserverOK && GetObserverMode() == OBS_MODE_IN_EYE )
	{
		C_BasePlayer *target =  ToBasePlayer( GetObserverTarget() );

		// get the targets viewmodel unless the target is an observer itself
		if ( target && target != this && !target->IsObserver() )
		{
			vm = target->GetViewModel( index );
		}
	}

	return vm;
}

C_BaseCombatWeapon	*C_BasePlayer::GetActiveWeapon( void ) const
{
	const C_BasePlayer *fromPlayer = this;

	// if localplayer is in InEye spectator mode, return weapon on chased player
	if ( (fromPlayer == GetLocalPlayer()) && ( GetObserverMode() == OBS_MODE_IN_EYE) )
	{
		C_BaseEntity *target =  GetObserverTarget();

		if ( target && target->IsPlayer() )
		{
			fromPlayer = ToBasePlayer( target );
		}
	}

	return fromPlayer->C_BaseCombatCharacter::GetActiveWeapon();
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector C_BasePlayer::GetAutoaimVector( float flScale )
{
	// Never autoaim a predicted weapon (for now)
	Vector	forward;
	AngleVectors( GetAbsAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

void C_BasePlayer::PlayPlayerJingle()
{
	// Find player sound for shooter
	player_info_t info;
	engine->GetPlayerInfo( entindex(), &info );

	if ( !cl_customsounds.GetBool() )
		return;

	// Doesn't have a jingle sound
	if ( !info.customFiles[1] )	
		return;

	char soundhex[ 16 ];
	Q_binarytohex( (byte *)&info.customFiles[1], sizeof( info.customFiles[1] ), soundhex, sizeof( soundhex ) );

	// See if logo has been downloaded.
	char fullsoundname[ 512 ];
	Q_snprintf( fullsoundname, sizeof( fullsoundname ), "sound/temp/%s.wav", soundhex );

	if ( !filesystem->FileExists( fullsoundname ) )
	{
		char custname[ 512 ];
		Q_snprintf( custname, sizeof( custname ), "download/user_custom/%c%c/%s.dat", soundhex[0], soundhex[1], soundhex );
		// it may have been downloaded but not copied under materials folder
		if ( !filesystem->FileExists( custname ) )
			return; // not downloaded yet

		// copy from download folder to materials/temp folder
		// this is done since material system can access only materials/*.vtf files

		if ( !engine->CopyLocalFile( custname, fullsoundname) )
			return;
	}

	Q_snprintf( fullsoundname, sizeof( fullsoundname ), "temp/%s.wav", soundhex );

	CLocalPlayerFilter filter;

	EmitSound_t ep;
	ep.m_nChannel = CHAN_VOICE;
	ep.m_pSoundName =  fullsoundname;
	ep.m_flVolume = VOL_NORM;
	ep.m_SoundLevel = SNDLVL_NORM;

	C_BaseEntity::EmitSound( filter, GetSoundSourceIndex(), ep );
}

// Stuff for prediction
void C_BasePlayer::SetSuitUpdate(const char *name, int fgroup, int iNoRepeat)
{
	// FIXME:  Do something here?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::ResetAutoaim( void )
{
#if 0
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
#endif
	m_fOnTarget = false;
}

//-----------------------------------------------------------------------------
// Purpose: Return the player who will predict this entity
//-----------------------------------------------------------------------------
C_BasePlayer *C_BasePlayer::GetPredictionOwner( void )
{
	return this;
}

bool C_BasePlayer::ShouldPredict( void )
{
	// Do this before calling into baseclass so prediction data block gets allocated
	if ( IsLocalPlayer() )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special processing for player simulation
// NOTE: Don't chain to BaseClass!!!!
//-----------------------------------------------------------------------------
void C_BasePlayer::PhysicsSimulate( void )
{
	VPROF( "C_BasePlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount)
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	if ( !IsLocalPlayer() )
		return;

	C_CommandContext *ctx = GetCommandContext();
	Assert( ctx );
	Assert( ctx->needsprocessing );
	if ( !ctx->needsprocessing )
		return;

	ctx->needsprocessing = false;

	// Handle FL_FROZEN.
	if(GetFlags() & FL_FROZEN)
	{
		ctx->cmd.forwardmove = 0;
		ctx->cmd.sidemove = 0;
		ctx->cmd.upmove = 0;
		ctx->cmd.buttons = 0;
		ctx->cmd.impulse = 0;
		//VectorCopy ( pl.v_angle, ctx->cmd.viewangles );
	}

	// Run the next command
	MoveHelper()->SetHost( this );
	prediction->RunCommand( 
		this, 
		&ctx->cmd, 
		MoveHelper() );
	MoveHelper()->SetHost( NULL );
}

const QAngle& C_BasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void C_BasePlayer::SetPunchAngle( const QAngle &angle )
{
	m_Local.m_vecPunchAngle = angle;
}


float C_BasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}

void C_BasePlayer::SetWaterJumpTime( float flWaterJumpTime )
{
	m_flWaterJumpTime = flWaterJumpTime;
}

float C_BasePlayer::GetSwimSoundTime() const
{
	return m_flSwimSoundTime;
}

void C_BasePlayer::SetSwimSoundTime( float flSwimSoundTime )
{
	m_flSwimSoundTime = flSwimSoundTime;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float C_BasePlayer::GetFOV( void )
{
	// Allow users to override the FOV during demo playback
	bool bUseDemoOverrideFov = engine->IsPlayingDemo() && demo_fov_override.GetFloat() > 0.0f;
#if defined( REPLAY_ENABLED )
	bUseDemoOverrideFov = bUseDemoOverrideFov && !g_pEngineClientReplay->IsPlayingReplayDemo();
#endif
	if ( bUseDemoOverrideFov )
	{
		return clamp( demo_fov_override.GetFloat(), 10.0f, 90.0f );
	}

	if ( GetObserverMode() == OBS_MODE_IN_EYE )
	{
		C_BasePlayer *pTargetPlayer = dynamic_cast<C_BasePlayer*>( GetObserverTarget() );

		// get fov from observer target. Not if target is observer itself
		if ( pTargetPlayer && !pTargetPlayer->IsObserver() )
		{
			return pTargetPlayer->GetFOV();
		}
	}

	// Allow our vehicle to override our FOV if it's currently at the default FOV.
	float flDefaultFOV;
	IClientVehicle *pVehicle = GetVehicle();
	if ( pVehicle )
	{
		CacheVehicleView();
		flDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : m_flVehicleViewFOV;
	}
	else
	{
		flDefaultFOV = GetDefaultFOV();
	}
	
	float fFOV = ( m_iFOV == 0 ) ? flDefaultFOV : m_iFOV;

	// Don't do lerping during prediction. It's only necessary when actually rendering,
	// and it'll cause problems due to prediction timing messiness.
	if ( !prediction->InPrediction() )
	{
		// See if we need to lerp the values for local player
		if ( IsLocalPlayer() && ( fFOV != m_iFOVStart ) && (m_Local.m_flFOVRate > 0.0f ) )
		{
			float deltaTime = (float)( gpGlobals->curtime - m_flFOVTime ) / m_Local.m_flFOVRate;

			if ( GetPredictable() )
			{
				// m_flFOVTime was set to a predicted time in the future, because the FOV change was predicted.
				deltaTime = (float)( GetFinalPredictedTime() - m_flFOVTime );
				deltaTime += ( gpGlobals->interpolation_amount * TICK_INTERVAL );
				deltaTime /= m_Local.m_flFOVRate;
			}

			if ( deltaTime >= 1.0f )
			{
				//If we're past the zoom time, just take the new value and stop lerping
				m_iFOVStart = fFOV;
			}
			else
			{
				fFOV = SimpleSplineRemapValClamped( deltaTime, 0.0f, 1.0f, (float) m_iFOVStart, fFOV );
			}
		}
	}

	float fMinFOV = GetMinFOV();

	fFOV = MAX(fMinFOV, fFOV);

	return fFOV;
}

void RecvProxy_LocalVelocityX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	float flNewVel_x = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.x != flNewVel_x )	// Should this use an epsilon check?
	{
		vecVelocity.x = flNewVel_x;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_LocalVelocityY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	float flNewVel_y = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.y != flNewVel_y )
	{
		vecVelocity.y = flNewVel_y;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_LocalVelocityZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;
	
	Assert( pPlayer );

	float flNewVel_z = pData->m_Value.m_Float;

	Vector vecVelocity = pPlayer->GetLocalVelocity();

	if( vecVelocity.z != flNewVel_z )
	{
		vecVelocity.z = flNewVel_z;
		pPlayer->SetLocalVelocity( vecVelocity );
	}
}

void RecvProxy_ObserverTarget( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	EHANDLE hTarget;

	RecvProxy_IntToEHandle( pData, pStruct, &hTarget );

	pPlayer->SetObserverTarget( hTarget );
}

void RecvProxy_ObserverMode( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *pPlayer = (C_BasePlayer *) pStruct;

	Assert( pPlayer );

	pPlayer->SetObserverMode ( pData->m_Value.m_Int );
}

void C_BasePlayer::RecvProxy_LocalOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((float*)pOut)[0] = pData->m_Value.m_Vector[0];
	((float*)pOut)[1] = pData->m_Value.m_Vector[1];
}

void C_BasePlayer::RecvProxy_LocalOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((float*)pOut) = pData->m_Value.m_Float;
}

void C_BasePlayer::RecvProxy_NonLocalOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((float*)pOut)[0] = pData->m_Value.m_Vector[0];
	((float*)pOut)[1] = pData->m_Value.m_Vector[1];
}

void C_BasePlayer::RecvProxy_NonLocalOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((float*)pOut) = pData->m_Value.m_Float;
}

void C_BasePlayer::RecvProxy_NonLocalCellOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *player = (C_BasePlayer *) pStruct;

	player->m_vecCellOrigin.x = pData->m_Value.m_Vector[0];
	player->m_vecCellOrigin.y = pData->m_Value.m_Vector[1];

	register int const cellwidth = player->m_cellwidth; // Load it into a register
	((float*)pOut)[0] = CoordFromCell( cellwidth, player->m_cellX, pData->m_Value.m_Vector[0] );
	((float*)pOut)[1] = CoordFromCell( cellwidth, player->m_cellY, pData->m_Value.m_Vector[1] );
}

void C_BasePlayer::RecvProxy_NonLocalCellOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BasePlayer *player = (C_BasePlayer *) pStruct;

	player->m_vecCellOrigin.z = pData->m_Value.m_Float;

	register int const cellwidth = player->m_cellwidth; // Load it into a register
	*((float*)pOut) = CoordFromCell( cellwidth, player->m_cellZ, pData->m_Value.m_Float );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from a vehicle
//-----------------------------------------------------------------------------
void C_BasePlayer::LeaveVehicle( void )
{
	if ( NULL == m_hVehicle.Get() )
		return;

// Let server do this for now
#if 0
	IClientVehicle *pVehicle = GetVehicle();
	Assert( pVehicle );

	int nRole = pVehicle->GetPassengerRole( this );
	Assert( nRole != VEHICLE_ROLE_NONE );

	SetParent( NULL );

	// Find the first non-blocked exit point:
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	pVehicle->GetPassengerExitPoint( nRole, &vNewPos, &qAngles );
	OnVehicleEnd( vNewPos );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );

	m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	RemoveEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_WALK );
	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	qAngles[ROLL] = 0;
	SnapEyeAngles( qAngles );

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	Weapon_Switch( m_hLastWeapon );
#endif
}


float C_BasePlayer::GetMinFOV()	const
{
	return 75;
}

float C_BasePlayer::GetFinalPredictedTime() const
{
	return ( m_nFinalPredictedTick * TICK_INTERVAL );
}

void C_BasePlayer::NotePredictionError( const Vector &vDelta )
{
	// don't worry about prediction errors when dead
	if ( !IsAlive() )
		return;

	Vector vOldDelta;

	GetPredictionErrorSmoothingVector( vOldDelta );

	// sum all errors within smoothing time
	m_vecPredictionError = vDelta + vOldDelta;

	// remember when last error happened
	m_flPredictionErrorTime = gpGlobals->curtime;
 
	ResetLatched(); 
}


// offset curtime and setup bones at that time using fake interpolation
// fake interpolation means we don't have reliable interpolation history (the local player doesn't animate locally)
// so we just modify cycle and origin directly and use that as a fake guess
bool C_BasePlayer::ForceSetupBonesAtTimeFakeInterpolation( matrix3x4_t *pBonesOut, float curtimeOffset )
{
	// we don't have any interpolation data, so fake it
	float cycle = m_flCycle;
	Vector origin = GetLocalOrigin();

	// blow the cached prev bones
	InvalidateBoneCache();
	// reset root position to flTime
	Interpolate( gpGlobals->curtime + curtimeOffset );

	// force cycle back by boneDt
	m_flCycle = fmod( 10 + cycle + GetPlaybackRate() * curtimeOffset, 1.0f );
	SetLocalOrigin( origin + curtimeOffset * GetLocalVelocity() );
	// Setup bone state to extrapolate physics velocity
	bool bSuccess = SetupBones( pBonesOut, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime + curtimeOffset );

	m_flCycle = cycle;
	SetLocalOrigin( origin );
	return bSuccess;
}

bool C_BasePlayer::GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt )
{
	if ( !IsLocalPlayer() )
		return BaseClass::GetRagdollInitBoneArrays(pDeltaBones0, pDeltaBones1, pCurrentBones, boneDt);

	bool bSuccess = true;

	if ( !ForceSetupBonesAtTimeFakeInterpolation( pDeltaBones0, -boneDt ) )
		bSuccess = false;
	if ( !ForceSetupBonesAtTimeFakeInterpolation( pDeltaBones1, 0 ) )
		bSuccess = false;

	float ragdollCreateTime = PhysGetSyncCreateTime();
	if ( ragdollCreateTime != gpGlobals->curtime )
	{
		if ( !ForceSetupBonesAtTimeFakeInterpolation( pCurrentBones, ragdollCreateTime - gpGlobals->curtime ) )
			bSuccess = false;
	}
	else
	{
		if ( !SetupBones( pCurrentBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime ) )
			bSuccess = false;
	}
	return bSuccess;
}


void C_BasePlayer::GetPredictionErrorSmoothingVector( Vector &vOffset )
{
	if ( engine->IsPlayingDemo() || !cl_smooth.GetInt() || !cl_predict->GetInt() || engine->IsPaused() )
	{
		vOffset.Init();
		return;
	}

	float errorAmount = ( gpGlobals->curtime - m_flPredictionErrorTime ) / cl_smoothtime.GetFloat();

	if ( errorAmount >= 1.0f )
	{
		vOffset.Init();
		return;
	}
	
	errorAmount = 1.0f - errorAmount;

	vOffset = m_vecPredictionError * errorAmount;
}


IRagdoll* C_BasePlayer::GetRepresentativeRagdoll() const
{
	return m_pRagdoll;
}

IMaterial *C_BasePlayer::GetHeadLabelMaterial( void )
{
	if ( GetClientVoiceMgr() == NULL )
		return NULL;

	return GetClientVoiceMgr()->GetHeadLabelMaterial();
}

bool IsInFreezeCam( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Set the fog controller data per player.
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void C_BasePlayer::FogControllerChanged( bool bSnap )
{
	if ( m_Local.m_PlayerFog.m_hCtrl )
	{
		fogparams_t	*pFogParams = &(m_Local.m_PlayerFog.m_hCtrl->m_fog);

		/*
		Msg("Updating Fog Target: (%d,%d,%d) %.0f,%.0f -> (%d,%d,%d) %.0f,%.0f (%.2f seconds)\n", 
					m_CurrentFog.colorPrimary.GetR(), m_CurrentFog.colorPrimary.GetB(), m_CurrentFog.colorPrimary.GetG(), 
					m_CurrentFog.start.Get(), m_CurrentFog.end.Get(), 
					pFogParams->colorPrimary.GetR(), pFogParams->colorPrimary.GetB(), pFogParams->colorPrimary.GetG(), 
					pFogParams->start.Get(), pFogParams->end.Get(), pFogParams->duration.Get() );*/
		

		// Setup the fog color transition.
		m_Local.m_PlayerFog.m_OldColor = m_CurrentFog.colorPrimary;
		m_Local.m_PlayerFog.m_flOldStart = m_CurrentFog.start;
		m_Local.m_PlayerFog.m_flOldEnd = m_CurrentFog.end;
		m_Local.m_PlayerFog.m_flOldMaxDensity = m_CurrentFog.maxdensity;
		m_Local.m_PlayerFog.m_flOldHDRColorScale = m_CurrentFog.HDRColorScale;
		m_Local.m_PlayerFog.m_flOldFarZ = m_CurrentFog.farz;

		m_Local.m_PlayerFog.m_NewColor = pFogParams->colorPrimary;
		m_Local.m_PlayerFog.m_flNewStart = pFogParams->start;
		m_Local.m_PlayerFog.m_flNewEnd = pFogParams->end;
		m_Local.m_PlayerFog.m_flNewMaxDensity = pFogParams->maxdensity;
		m_Local.m_PlayerFog.m_flNewHDRColorScale = pFogParams->HDRColorScale;
		m_Local.m_PlayerFog.m_flNewFarZ = pFogParams->farz;

		m_Local.m_PlayerFog.m_flTransitionTime = bSnap ? -1 : gpGlobals->curtime;

		m_CurrentFog = *pFogParams;

		// Update the fog player's local fog data with the fog controller's data if need be.
		UpdateFogController();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see that the controllers data is up to date.
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFogController( void )
{
	if ( m_Local.m_PlayerFog.m_hCtrl )
	{
		// Don't bother copying while we're transitioning, since it'll be stomped in UpdateFogBlend();
		if ( m_Local.m_PlayerFog.m_flTransitionTime == -1 && (m_hOldFogController.Get() == m_Local.m_PlayerFog.m_hCtrl.Get()) )
		{
			fogparams_t	*pFogParams = &(m_Local.m_PlayerFog.m_hCtrl->m_fog);
			if ( m_CurrentFog != *pFogParams )
			{
				/*
					Msg("FORCING UPDATE: (%d,%d,%d) %.0f,%.0f -> (%d,%d,%d) %.0f,%.0f (%.2f seconds)\n", 
										m_CurrentFog.colorPrimary.GetR(), m_CurrentFog.colorPrimary.GetB(), m_CurrentFog.colorPrimary.GetG(), 
										m_CurrentFog.start.Get(), m_CurrentFog.end.Get(), 
										pFogParams->colorPrimary.GetR(), pFogParams->colorPrimary.GetB(), pFogParams->colorPrimary.GetG(), 
										pFogParams->start.Get(), pFogParams->end.Get(), pFogParams->duration.Get() );*/
					

				m_CurrentFog = *pFogParams;
			}
		}
	}
	else
	{
		if ( m_CurrentFog.farz != -1 || m_CurrentFog.enable != false )
		{
			// No fog controller in this level. Use default fog parameters.
			m_CurrentFog.farz = -1;
			m_CurrentFog.enable = false;
		}
	}

	// Update the fog blending state - of necessary.
	UpdateFogBlend();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFogBlend( void )
{
	// Transition.
	if ( m_Local.m_PlayerFog.m_flTransitionTime != -1 )
	{
		float flTimeDelta = gpGlobals->curtime - m_Local.m_PlayerFog.m_flTransitionTime;
		if ( flTimeDelta < m_CurrentFog.duration )
		{
			float flScale = flTimeDelta / m_CurrentFog.duration;
			m_CurrentFog.colorPrimary.SetR( ( m_Local.m_PlayerFog.m_NewColor.r * flScale ) + ( m_Local.m_PlayerFog.m_OldColor.r * ( 1.0f - flScale ) ) );
			m_CurrentFog.colorPrimary.SetG( ( m_Local.m_PlayerFog.m_NewColor.g * flScale ) + ( m_Local.m_PlayerFog.m_OldColor.g * ( 1.0f - flScale ) ) );
			m_CurrentFog.colorPrimary.SetB( ( m_Local.m_PlayerFog.m_NewColor.b * flScale ) + ( m_Local.m_PlayerFog.m_OldColor.b * ( 1.0f - flScale ) ) );
			m_CurrentFog.start.Set( ( m_Local.m_PlayerFog.m_flNewStart * flScale ) + ( ( m_Local.m_PlayerFog.m_flOldStart * ( 1.0f - flScale ) ) ) );
			m_CurrentFog.end.Set( ( m_Local.m_PlayerFog.m_flNewEnd * flScale ) + ( ( m_Local.m_PlayerFog.m_flOldEnd * ( 1.0f - flScale ) ) ) );
			m_CurrentFog.maxdensity.Set( ( m_Local.m_PlayerFog.m_flNewMaxDensity * flScale ) + ( ( m_Local.m_PlayerFog.m_flOldMaxDensity * ( 1.0f - flScale ) ) ) );
			m_CurrentFog.HDRColorScale.Set( ( m_Local.m_PlayerFog.m_flNewHDRColorScale * flScale ) + ( ( m_Local.m_PlayerFog.m_flOldHDRColorScale * ( 1.0f - flScale ) ) ) );

			// Lerp to a sane FarZ (default value comes from CViewRender::GetZFar())
			float newFarZ = m_Local.m_PlayerFog.m_flNewFarZ;
			if ( newFarZ <= 0 )
				newFarZ = r_mapextents.GetFloat() * 1.73205080757f;

			float oldFarZ = m_Local.m_PlayerFog.m_flOldFarZ;
			if ( oldFarZ <= 0 )
				oldFarZ = r_mapextents.GetFloat() * 1.73205080757f;

			m_CurrentFog.farz.Set( ( newFarZ * flScale ) + ( ( oldFarZ * ( 1.0f - flScale ) ) ) );
		}
		else
		{
			// Slam the final fog values.
			m_CurrentFog.colorPrimary.SetR( m_Local.m_PlayerFog.m_NewColor.r );
			m_CurrentFog.colorPrimary.SetG( m_Local.m_PlayerFog.m_NewColor.g );
			m_CurrentFog.colorPrimary.SetB( m_Local.m_PlayerFog.m_NewColor.b );
			m_CurrentFog.start.Set( m_Local.m_PlayerFog.m_flNewStart );
			m_CurrentFog.end.Set( m_Local.m_PlayerFog.m_flNewEnd );
			m_CurrentFog.maxdensity.Set( m_Local.m_PlayerFog.m_flNewMaxDensity );
			m_CurrentFog.HDRColorScale.Set( m_Local.m_PlayerFog.m_flNewHDRColorScale );
			m_CurrentFog.farz.Set( m_Local.m_PlayerFog.m_flNewFarZ );
			m_Local.m_PlayerFog.m_flTransitionTime = -1;

			/*
				Msg("Finished transition to (%d,%d,%d) %.0f,%.0f\n", 
								m_CurrentFog.colorPrimary.GetR(), m_CurrentFog.colorPrimary.GetB(), m_CurrentFog.colorPrimary.GetG(), 
								m_CurrentFog.start.Get(), m_CurrentFog.end.Get() );*/
				
		}
	}
}

bool C_BasePlayer::PreRender()
{
	if ( !IsVisible() || 
		!GetClientMode()->ShouldDrawLocalPlayer( this ) )
	{
		return true;
	}

	// Add in lighting effects
	return CreateLightEffects();
}

bool C_BasePlayer::ShouldRegenerateOriginFromCellBits() const
{
	return BaseClass::ShouldRegenerateOriginFromCellBits();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BasePlayer::GetSteamID( CSteamID *pID )
{
	// try to make this a little more efficient

	player_info_t pi;
	if ( engine->GetPlayerInfo( entindex(), &pi ) )
	{
		if ( pi.friendsID && steamapicontext && steamapicontext->SteamUtils() )
		{
#if 1	// new
			static EUniverse universe = k_EUniverseInvalid;

			if ( universe == k_EUniverseInvalid )
				universe = steamapicontext->SteamUtils()->GetConnectedUniverse();

			pID->InstancedSet( pi.friendsID, 1, universe, k_EAccountTypeIndividual );
#else	// old
			pID->InstancedSet( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
#endif

			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: In meathook mode, fix the bone transforms to hang the user's own
//			avatar under the camera.
//-----------------------------------------------------------------------------
void C_BasePlayer::BuildFirstPersonMeathookTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, const char *pchHeadBoneName )
{
	// Handle meathook mode. If we aren't rendering, just use last frame's transforms
	if ( !InFirstPersonView() )
		return;

	// If we're in third-person view, don't do anything special.
	// If we're in first-person view rendering the main view and using the viewmodel, we shouldn't have even got here!
	// If we're in first-person view rendering the main view(s), meathook and headless.
	// If we're in first-person view rendering shadowbuffers/reflections, don't do anything special either (we could do meathook but with a head?)
	if ( IsRagdoll() )
	{
		// We're re-animating specifically to set up the ragdoll.
		// Meathook can push the player through the floor, which makes the ragdoll fall through the world, which is no good.
		// So do nothing.
		return;
	}

	if ( !DrawingMainView() )
	{
		return;
	}

	// If we aren't drawing the player anyway, don't mess with the bones. This can happen in Portal.
	if( !ShouldDrawThisPlayer() )
	{
		return;
	}

	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );

	int iHead = LookupBone( pchHeadBoneName );
	if ( iHead == -1 )
	{
		return;
	}

	matrix3x4_t &mHeadTransform = GetBoneForWrite( iHead );

	// "up" on the head bone is along the negative Y axis - not sure why.
	//Vector vHeadTransformUp ( -mHeadTransform[0][1], -mHeadTransform[1][1], -mHeadTransform[2][1] );
	//Vector vHeadTransformFwd ( mHeadTransform[0][1], mHeadTransform[1][1], mHeadTransform[2][1] );
	Vector vHeadTransformTranslation ( mHeadTransform[0][3], mHeadTransform[1][3], mHeadTransform[2][3] );


	// Find out where the player's head (driven by the HMD) is in the world.
	// We can't move this with animations or effects without causing nausea, so we need to move
	// the whole body so that the animated head is in the right place to match the player-controlled head.
	Vector vRealPivotPoint;

	// figure out where to put the body from the aim angles
	Vector vForward, vRight, vUp;
	AngleVectors( MainViewAngles(), &vForward, &vRight, &vUp );
	
	vRealPivotPoint = MainViewOrigin() - ( vUp * cl_meathook_neck_pivot_ingame_up.GetFloat() ) - ( vForward * cl_meathook_neck_pivot_ingame_fwd.GetFloat() );		

	Vector vDeltaToAdd = vRealPivotPoint - vHeadTransformTranslation;


	// Now add this offset to the entire skeleton.
	for (int i = 0; i < hdr->numbones(); i++)
	{
		// Only update bones reference by the bone mask.
		if ( !( hdr->boneFlags( i ) & boneMask ) )
		{
			continue;
		}
		matrix3x4_t& bone = GetBoneForWrite( i );
		Vector vBonePos;
		MatrixGetTranslation ( bone, vBonePos );
		vBonePos += vDeltaToAdd;
		MatrixSetTranslation ( vBonePos, bone );
	}

	// Then scale the head to zero, but leave its position - forms a "neck stub".
	// This prevents us rendering junk all over the screen, e.g. inside of mouth, etc.
	MatrixScaleByZero( mHeadTransform );

	// TODO: right now we nuke the hats by shrinking them to nothing,
	// but it feels like we should do something more sensible.
	// For example, for one sniper taunt he takes his hat off and waves it - would be nice to see it then.
	int iHelm = LookupBone( "prp_helmet" );
	if ( iHelm != -1 )
	{
		// Scale the helmet.
		matrix3x4_t  &transformhelmet = GetBoneForWrite( iHelm );
		MatrixScaleByZero( transformhelmet );
	}

	iHelm = LookupBone( "prp_hat" );
	if ( iHelm != -1 )
	{
		matrix3x4_t  &transformhelmet = GetBoneForWrite( iHelm );
		MatrixScaleByZero( transformhelmet );
	}
}



void CC_DumpClientSoundscapeData( const CCommand& args )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	Msg("Client Soundscape data dump:\n");
	Msg("   Position: %.2f %.2f %.2f\n", pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, pPlayer->GetAbsOrigin().z );
	Msg("   soundscape index: %d\n", pPlayer->m_Local.m_audio.soundscapeIndex.Get() );
	Msg("   entity index: %d\n", pPlayer->m_Local.m_audio.ent.Get() ? pPlayer->m_Local.m_audio.ent->entindex() : -1 );
	if ( pPlayer->m_Local.m_audio.ent.Get() )
	{
		Msg("   entity pos: %.2f %.2f %.2f\n", pPlayer->m_Local.m_audio.ent.Get()->GetAbsOrigin().x, pPlayer->m_Local.m_audio.ent.Get()->GetAbsOrigin().y, pPlayer->m_Local.m_audio.ent.Get()->GetAbsOrigin().z );
		if ( pPlayer->m_Local.m_audio.ent.Get()->IsDormant() )
		{
			Msg("     ENTITY IS DORMANT\n");
		}
	}
	bool bFoundOne = false;
	for ( int i = 0; i < NUM_AUDIO_LOCAL_SOUNDS; i++ )
	{
		if ( pPlayer->m_Local.m_audio.localBits & (1<<i) )
		{
			if ( !bFoundOne )
			{
				Msg("   Sound Positions:\n");
				bFoundOne = true;
			}

			Vector vecPos = pPlayer->m_Local.m_audio.localSound[i];
			Msg("   %d: %.2f %.2f %.2f\n", i, vecPos.x,vecPos.y, vecPos.z );
		}
	}

	Msg("End dump.\n");
}
static ConCommand soundscape_dumpclient("soundscape_dumpclient", CC_DumpClientSoundscapeData, "Dumps the client's soundscape data.\n", FCVAR_CHEAT);

const Vector &C_BasePlayer::GetRenderOrigin( void )
{
	return BaseClass::GetRenderOrigin();
}
