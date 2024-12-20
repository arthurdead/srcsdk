//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side CBasePlayer.
//
//			- Manages the player's flashlight effect.
//
//=============================================================================//

#ifndef C_BASEPLAYER_H
#define C_BASEPLAYER_H
#pragma once

#include "c_playerlocaldata.h"
#include "c_basecombatcharacter.h"
#include "PlayerState.h"
#include "usercmd.h"
#include "shareddefs.h"
#include "timedevent.h"
#include "smartptr.h"
#include "fx_water.h"
#include "hintsystem.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "c_env_fog_controller.h"
#include "igameevents.h"
#include "GameEventListener.h"
#include "vphysics_interface.h"
#include "baseplayer_shared.h"
#include "c_colorcorrection.h"
#include "c_postprocesscontroller.h"
#include "playeranimstate.h"
#include "util_shared.h"

class C_BaseCombatWeapon;
class C_BaseViewModel;
class C_FuncLadder;
class CFlashlightEffect;
struct Beam_t;
class CHintSystem;

extern int g_nKillCamMode;
extern int g_nKillCamTarget1;
extern int g_nKillCamTarget2;

class C_CommandContext
{
public:
	bool			needsprocessing;

	CUserCmd		cmd;
	int				command_number;
};

class C_PredictionError
{
public:
	float	time;
	Vector	error;
};

#define CHASE_CAM_DISTANCE_MIN	16.0f
#define CHASE_CAM_DISTANCE_MAX	96.0f
#define WALL_OFFSET				6.0f

enum PlayerRenderMode_t
{
	PLAYER_RENDER_NONE = 0,
	PLAYER_RENDER_FIRSTPERSON,
	PLAYER_RENDER_THIRDPERSON,
};

bool IsInFreezeCam( void );

//-----------------------------------------------------------------------------
// Purpose: Base Player class
//-----------------------------------------------------------------------------
class C_BasePlayer : public C_BaseCombatCharacter, public CGameEventListener
{
public:
	DECLARE_CLASS( C_BasePlayer, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_BasePlayer();
	virtual			~C_BasePlayer();

	virtual bool		PostConstructor( const char *szClassname );

	virtual void	Spawn( void );
	virtual void	SharedSpawn(); // Shared between client and server.
	virtual void	Respawn();
	virtual bool	GetSteamID( CSteamID *pID );
	Class_T		Classify( void ) { return CLASS_PLAYER; }

	virtual const Vector &GetRenderOrigin();

	// IClientEntity overrides.
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	virtual void NotifyShouldTransmit(ShouldTransmitState_t state);
	
	virtual void	ReceiveMessage( int classID, bf_read &msg );

	virtual void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual void	GetToolRecordingState( KeyValues *msg );

	virtual float GetPlayerMaxSpeed();

	float		StepHeight() const			{ return m_Local.m_flStepSize; }

	void	SetAnimationExtension( const char *pExtension );

	C_BaseViewModel		*GetViewModel( int viewmodelindex = VIEWMODEL_WEAPON, bool bObserverOK=true );
	C_BaseCombatWeapon	*GetActiveWeapon( void ) const;
	const char			*GetTracerType( void );

	// View model prediction setup
	virtual void		CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual void		CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles);
	

	// Handle view smoothing when going up stairs
	void				SmoothViewOnStairs( Vector& eyeOrigin );
	virtual float		CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);
	void				CalcViewRoll( QAngle& eyeAngles );
	virtual void		CalcViewBob( Vector& eyeOrigin );
	void				CreateWaterEffects( void );

	virtual void			SetPlayerUnderwater( bool state );
	void					UpdateUnderwaterState( void );
	bool					IsPlayerUnderwater( void ) { return m_bPlayerUnderwater; }

	virtual	C_BaseCombatCharacter *ActivePlayerCombatCharacter( void ) { return this; }

	virtual Vector			Weapon_ShootPosition();
	virtual bool			Weapon_CanUse( C_BaseCombatWeapon *pWeapon );
	virtual void			Weapon_DropPrimary( void ) {}

	virtual Vector			GetAutoaimVector( float flScale );
	void					SetSuitUpdate(const char *name, int fgroup, int iNoRepeat);

	// Input handling
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	virtual void	AvoidPhysicsProps( CUserCmd *pCmd );
	
	virtual void	PlayerUse( void );
	C_BaseEntity		*FindUseEntity( void );
	virtual bool	IsUseableEntity( C_BaseEntity *pEntity, unsigned int requiredCaps );

	// Data handlers
	virtual bool	IsPlayer( void ) const { return true; }
	virtual int		GetHealth() const { return m_iHealth; }

	int		GetBonusProgress() const { return m_iBonusProgress; }
	int		GetBonusChallenge() const { return m_iBonusChallenge; }

	// observer mode
	virtual int			GetObserverMode() const;
	void				SetObserverMode ( int iNewMode );
	virtual C_BaseEntity	*GetObserverTarget() const;
	void			SetObserverTarget( EHANDLE hObserverTarget );

	bool			AudioStateIsUnderwater( const Vector &vecMainViewOrigin ) const;

	bool IsObserver() const;
	bool IsHLTV() const;
	bool IsReplay() const;
	void ResetObserverMode();
	bool IsBot( void ) const { return false; }

	// Eye position..
	virtual Vector		 EyePosition();
	virtual const QAngle &EyeAngles();		// Direction of eyes
	void				 EyePositionAndVectors( Vector *pPosition, Vector *pForward, Vector *pRight, Vector *pUp );
	virtual const QAngle &LocalEyeAngles();		// Direction of eyes

	virtual const QAngle& GetRenderAngles();

	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual void UpdateClientSideAnimation();

	virtual float GetServerIntendedCycle() { return m_flServerCycle; }
	virtual void SetServerIntendedCycle(float cycle) { m_flServerCycle = cycle; }

	virtual void CalculateIKLocks(float currentTime);

	// This can be overridden to return something other than m_pRagdoll if the mod uses separate 
	// entities for ragdolls.
	virtual IRagdoll* GetRepresentativeRagdoll() const;

	// override the initial bone position for ragdolls
	virtual bool GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt ) OVERRIDE;

	// Returns eye vectors
	void			EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL );
	void			CacheVehicleView( void );	// Calculate and cache the position of the player in the vehicle


	bool			IsSuitEquipped( void ) { return m_Local.m_bWearingSuit; };

	// Team handlers
	virtual void	TeamChange( int iNewTeam );

	// Flashlight
	virtual void	UpdateFlashlight( void );
	void	TurnOffFlashlight( void );	// TERROR
	virtual const char *GetFlashlightTextureName( void ) const { return NULL; } // TERROR
	virtual float GetFlashlightFOV( void ) const { return 0.0f; } // TERROR
	virtual float GetFlashlightFarZ( void ) const { return 0.0f; } // TERROR
	virtual float GetFlashlightLinearAtten( void ) const { return 0.0f; } // TERROR
	virtual bool CastsFlashlightShadows( void ) const { return true; } // TERROR
	virtual void GetFlashlightOffset( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Vector *pVecOffset ) const;

	const Vector &GetFlashlightOrigin() const { return m_vecFlashlightOrigin; }

private:
	Vector	m_vecFlashlightOrigin;
	Vector	m_vecFlashlightForward;
	Vector	m_vecFlashlightUp;
	Vector	m_vecFlashlightRight;
	Beam_t *m_pFlashlightBeam;

public:
	// Weapon selection code
	virtual bool				IsAllowedToSwitchWeapons( void ) { return !IsObserver(); }
	virtual C_BaseCombatWeapon	*GetActiveWeaponForSelection( void );

	// Returns the view model if this is the local player. If you're in third person or 
	// this is a remote player, it returns the active weapon
	// (and its appropriate left/right weapon if this is TF2).
	virtual C_BaseAnimating*	GetRenderedWeaponModel();

	virtual bool				IsOverridingViewmodel( void ) { return false; };
	virtual int					ViewModel_DrawModel( C_BaseViewModel *pViewmodel, int flags, const RenderableInstance_t &instance ) { return 0; };

	virtual float				GetDefaultAnimSpeed( void ) { return 1.0; }

	void						SetMaxSpeed( float flMaxSpeed ) { m_flMaxspeed = flMaxSpeed; }
	float						MaxSpeed() const		{ return m_flMaxspeed; }

	bool	ShouldPlayerDrawParticles( void );

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType();

	virtual bool				ShouldReceiveProjectedTextures( int flags );

	void						CheckForLocalPlayer();

	static bool					IsLocalPlayer( const C_BaseEntity *pl );
	bool						IsLocalPlayer( void ) const;

	virtual CStudioHdr *OnNewModel();

	// Global/static methods
	virtual void				ThirdPersonSwitch( bool bThirdperson );
	static bool					LocalPlayerInFirstPersonView();
	static bool					ShouldDrawLocalPlayer();
	static C_BasePlayer			*GetLocalPlayer( void );
	int							GetUserID( void ) const;
	virtual bool				CanSetSoundMixer( void );
	// return the entity used for soundscape radius checks
	virtual C_BaseEntity		*GetSoundscapeListener();
	virtual vision_filter_t					GetVisionFilterFlags();
	bool						HasVisionFilterFlags( vision_filter_t nFlags ) { return ( GetVisionFilterFlags() & nFlags ) == nFlags; }
	virtual void				CalculateVisionUsingCurrentFlags( void );
	void						BuildFirstPersonMeathookTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, const char *pchHeadBoneName );

	// Specific queries about this player.
	bool						InFirstPersonView();
	bool						ShouldDrawThisPlayer();

	virtual IClientModelRenderable*	GetClientModelRenderable();
	virtual bool				PreRender();

	// Called by the view model if its rendering is being overridden.
	virtual uint8 ViewModel_OverrideRenderAlpha( uint8 nAlpha );
	virtual RenderableTranslucencyType_t ViewModel_ComputeTranslucencyType( void );
	virtual int				ViewModel_GetRenderFlags( void );

	void						AddToPlayerSimulationList( C_BaseEntity *other );
	void						SimulatePlayerSimulatedEntities( void );
	void						RemoveFromPlayerSimulationList( C_BaseEntity *ent );
	void						ClearPlayerSimulationList( void );

	virtual void				PhysicsSimulate( void );
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const { return MASK_PLAYERSOLID; }

	// Prediction stuff
	virtual bool				ShouldPredict( void );
	virtual C_BasePlayer		*GetPredictionOwner( void );

	virtual void				PreThink( void );
	virtual void				PostThink( void );

	virtual void		  SetViewOffset( const Vector& v );

	virtual void				ItemPreFrame( void );
	virtual void				ItemPostFrame( void );
	virtual void				AbortReload( void );

	virtual void				SelectLastItem(void);
	virtual void				Weapon_SetLast( C_BaseCombatWeapon *pWeapon );
	virtual bool				Weapon_ShouldSetLast( C_BaseCombatWeapon *pOldWeapon, C_BaseCombatWeapon *pNewWeapon ) { return true; }
	virtual bool				Weapon_ShouldSelectItem( C_BaseCombatWeapon *pWeapon );
	virtual	WeaponSwitchResult_t				Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = VIEWMODEL_WEAPON, bool bDeploy = true );		// Switch to given weapon if has ammo (false if failed)
	virtual C_BaseCombatWeapon *GetLastWeapon( void ) { return m_hLastWeapon.Get(); }
	void						ResetAutoaim( void );
	virtual void 				SelectItem( const char *pstr, int iSubType = 0 ) final;
	virtual void 				SelectItem( C_BaseCombatWeapon *pItem );

	virtual void				UpdateClientData( void );

	virtual float				GetFOV( void );	
	int							GetDefaultFOV( void ) const;
	virtual bool				IsZoomed( void )	{ return false; }
	bool						SetFOV( C_BaseEntity *pRequester, int FOV, float zoomRate = 0.0f, int iZoomStart = 0 );
	void						ClearZoomOwner( void );

	float						GetFOVDistanceAdjustFactor();

	virtual void				ViewPunch( const QAngle &angleOffset );
	void						ViewPunchReset( float tolerance = 0 );

	virtual void						UpdateButtonState( uint64 nUserCmdButtonMask );
	int							GetImpulse( void ) const;

	virtual bool				Simulate();

	virtual bool				ShouldInterpolate();

	virtual bool				ShouldDraw();
	virtual int					DrawModel( int flags, const RenderableInstance_t &instance );

	// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
	virtual void				OverrideView( CViewSetup *pSetup );

	C_BaseEntity				*GetViewEntity( void ) const { return m_hViewEntity; }

	// returns the player name
	const char *				GetPlayerName();
	virtual const Vector		GetPlayerMins( void ) const; // uses local player
	virtual const Vector		GetPlayerMaxs( void ) const; // uses local player

	virtual void				UpdateCollisionBounds( void );

	// Is the player dead?
	bool				IsPlayerDead();
	bool				IsPoisoned( void ) { return m_Local.m_bPoisoned; }

	virtual C_BaseEntity* GetUseEntity( void ) const;
	virtual C_BaseEntity* GetPotentialUseEntity( void ) const;

	// Vehicles...
	IClientVehicle			*GetVehicle();
	const IClientVehicle	*GetVehicle() const;

	bool			IsInAVehicle() const	{ return ( NULL != m_hVehicle.Get() ) ? true : false; }
	virtual void	SetVehicleRole( int nRole );
	void					LeaveVehicle( void );

	bool					UsingStandardWeaponsInVehicle( void );

	float					GetTimeBase( void ) const;
	float					GetFinalPredictedTime() const;

	bool					IsInVGuiInputMode() const;
	bool					IsInViewModelVGuiInputMode() const;

	C_CommandContext		*GetCommandContext();

	// Get the command number associated with the current usercmd we're running (if in predicted code).
	int CurrentCommandNumber() const;
	const CUserCmd *GetCurrentUserCommand() const;

	virtual const QAngle&	GetPunchAngle();
	void SetPunchAngle( const QAngle &angle );

	float					GetWaterJumpTime() const;
	void					SetWaterJumpTime( float flWaterJumpTime );
	float					GetSwimSoundTime( void ) const;
	void					SetSwimSoundTime( float flSwimSoundTime );

	float					GetDeathTime( void ) { return m_flDeathTime; }

	void		SetPreviouslyPredictedOrigin( const Vector &vecAbsOrigin );
	const Vector &GetPreviouslyPredictedOrigin() const;

	// CS wants to allow small FOVs for zoomed-in AWPs.
	virtual float GetMinFOV() const;

	virtual void DoMuzzleFlash();
	virtual void PlayPlayerJingle();

	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity  );
	virtual void PlayStepSound( const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual surfacedata_t * GetFootstepSurface( const Vector &origin, const char *surfaceName );
	virtual void GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char *GetOverrideStepSound( const char *pszBaseStepSoundName ) { return pszBaseStepSoundName; }

	virtual void OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume ) {}

	// Called by prediction when it detects a prediction correction.
	// vDelta is the line from where the client had predicted the player to at the usercmd in question,
	// to where the server says the client should be at said usercmd.
	void NotePredictionError( const Vector &vDelta );
	
	// Called by the renderer to apply the prediction error smoothing.
	void GetPredictionErrorSmoothingVector( Vector &vOffset ); 

	virtual void ExitLadder() {}
	surfacedata_t *GetLadderSurface( const Vector &origin );

	surfacedata_t *GetSurfaceData( void ) { return m_pSurfaceData; }

	void	ForceButtons( uint64 nButtons );
	void	UnforceButtons( uint64 nButtons );

	void SetLadderNormal( const Vector &vecLadderNormal ) { m_vecLadderNormal = vecLadderNormal; }
	const Vector &GetLadderNormal( void ) const { return m_vecLadderNormal; }
	int GetLadderSurfaceProps( void ) const { return m_ladderSurfaceProps; }

	// Hints
	virtual CHintSystem		*Hints( void ) { return NULL; }
	bool					ShouldShowHints( void ) { return Hints() ? Hints()->ShouldShowHints() : false; }
	bool 					HintMessage( int hint, bool bForce = false, bool bOnlyIfClear = false ) { return Hints() ? Hints()->HintMessage( hint, bForce, bOnlyIfClear ) : false; }
	void 					HintMessage( const char *pMessage ) { if (Hints()) Hints()->HintMessage( pMessage ); }

	virtual	IMaterial *GetHeadLabelMaterial( void );

	// Fog
	virtual fogparams_t				*GetFogParams( void ) { return &m_CurrentFog; }
	void					FogControllerChanged( bool bSnap );
	void					UpdateFogController( void );
	void					UpdateFogBlend( void );

	C_PostProcessController* GetActivePostProcessController() const;
	C_ColorCorrection*		GetActiveColorCorrection() const;

	float					GetFOVTime( void ){ return m_flFOVTime; }

	PlayerRenderMode_t 		GetPlayerRenderMode();

	virtual void			OnAchievementAchieved( int iAchievement ) {}
	
	bool					ShouldAnnounceAchievement( void ){ return m_flNextAchievementAnnounceTime < gpGlobals->curtime; }
	void					SetNextAchievementAnnounceTime( float flTime ){ m_flNextAchievementAnnounceTime = flTime; }

	bool					HasFiredWeapon( void ) { return m_bFiredWeapon; }
	void					SetFiredWeapon( bool bFlag ) { m_bFiredWeapon = bFlag; }

	virtual bool			CanUseFirstPersonCommand( void ){ return true; }

protected:
	fogparams_t				m_CurrentFog;
	EHANDLE					m_hOldFogController;

public:
	// RecvProxies
	static void RecvProxy_LocalOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void RecvProxy_LocalOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void RecvProxy_NonLocalOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void RecvProxy_NonLocalOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut );
#ifdef DT_CELL_COORD_SUPPORTED
	static void RecvProxy_NonLocalCellOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void RecvProxy_NonLocalCellOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

	virtual bool ShouldRegenerateOriginFromCellBits() const;

	int m_StuckLast;
	
	// Data for only the local player
	CNetworkVarEmbedded( CPlayerLocalData, m_Local );

	// Data common to all other players, too
	CPlayerState			pl;

	// Player FOV values
	int						m_iFOV;				// field of view
	int						m_iFOVStart;		// starting value of the FOV changing over time (client only)
	float					m_flFOVTime;		// starting time of the FOV zoom
	int						m_iDefaultFOV;		// default FOV if no other zooms are occurring
	EHANDLE					m_hZoomOwner;		// This is a pointer to the entity currently controlling the player's zoom
												// Only this entity can change the zoom state once it has ownership

	// For weapon prediction
	bool			m_fOnTarget;		//Is the crosshair on a target?
	
	char			m_szAnimExtension[32];

	uint64				m_afButtonLast;
	uint64				m_afButtonPressed;
	uint64				m_afButtonReleased;

	uint64				m_afButtonForced;	// These are forced onto the player's inputs

	uint64				m_nButtons;

	int						m_nOldTickBase;

	CUserCmd		*m_pCurrentCommand;

	EHANDLE			m_hViewEntity;

	// Movement constraints
	EHANDLE			m_hConstraintEntity;
	Vector			m_vecConstraintCenter;
	float			m_flConstraintRadius;
	float			m_flConstraintWidth;
	float			m_flConstraintSpeedFactor;
	bool			m_bConstraintPastRadius;

	// Transmitted from the server for internal player spawnflags.
	// See baseplayer_shared.h for more details.
	int				m_spawnflags;

	inline bool		HasSpawnFlags( int flags ) { return (m_spawnflags & flags) != 0; }
	inline void		RemoveSpawnFlags( int flags ) { m_spawnflags &= ~flags; }
	inline void		AddSpawnFlags( int flags ) { m_spawnflags |= flags; }

	// Allows the player's model to draw on non-main views, like monitors or mirrors.
	bool			m_bDrawPlayerModelExternally;

	bool			m_bInTriggerFall;

protected:

	//Tony; made all of these virtual so mods can override.
	virtual void				CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void				CalcVehicleView(IClientVehicle *pVehicle, Vector& eyeOrigin, QAngle& eyeAngles,
							float& zNear, float& zFar, float& fov );
	virtual void		CalcObserverView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual Vector		GetChaseCamViewOffset( C_BaseEntity *target );
	virtual void				CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void		CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual float		GetDeathCamInterpolationTime();

	virtual void		CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void				CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void		CalcFreezeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	// Check to see if we're in vgui input mode...
	void DetermineVguiInputMode( CUserCmd *pCmd );

	// Used by prediction, sets the view angles for the player
	virtual void SetLocalViewAngles( const QAngle &viewAngles );
	virtual void SetViewAngles( const QAngle& ang );

	// used by client side player footsteps 
	surfacedata_t* GetGroundSurface();

	virtual void	FireGameEvent( IGameEvent *event );

	int GetIDTarget() const { return m_iIDEntIndex; }

protected:
	// Did we just enter a vehicle this frame?
	bool			JustEnteredVehicle();

// DATA
	int				m_iObserverMode;	// if in spectator mode != 0
	EHANDLE			m_hObserverTarget;	// current observer target
	float			m_flObserverChaseDistance; // last distance to observer traget
	Vector			m_vecFreezeFrameStart;
	float			m_flFreezeFrameStartTime;	// Time at which we entered freeze frame observer mode
	float			m_flFreezeFrameDistance;
	bool			m_bWasFreezeFraming; 
	float			m_flDeathTime;		// last time player died

	float			m_flStepSoundTime;
	bool			m_IsFootprintOnLeft;

private:
	// Make sure no one calls this...
	C_BasePlayer& operator=( const C_BasePlayer& src );
	C_BasePlayer( const C_BasePlayer & ); // not defined, not accessible

	// Vehicle stuff.
	EHANDLE			m_hVehicle;
	EHANDLE			m_hOldVehicle;
	EHANDLE			m_hUseEntity;
	
	float			m_flMaxspeed;

	int				m_iBonusProgress;
	int				m_iBonusChallenge;

	CInterpolatedVar< Vector >	m_iv_vecViewOffset;

	// Not replicated
	Vector			m_vecWaterJumpVel;
	float			m_flWaterJumpTime;  // used to be called teleport_time
	int				m_nImpulse;

	float			m_flSwimSoundTime;
	CNetworkVector( m_vecLadderNormal );
	CNetworkVar( int, m_ladderSurfaceProps );
	
	QAngle			m_vecOldViewAngles;

	bool			m_bWasFrozen;
	int				m_flPhysics;

	int				m_nTickBase;
	int				m_nFinalPredictedTick;

	EHANDLE			m_pCurrentVguiScreen;

	bool			m_bFiredWeapon;


	// Player flashlight dynamic light pointers
	bool			m_bFlashlightEnabled;

	typedef CHandle<C_BaseCombatWeapon> BaseCombatWeaponHandle;
	CNetworkHandle( C_BaseCombatWeapon, m_hLastWeapon );

	CUtlVector< CHandle< C_BaseEntity > > m_SimulatedByThisPlayer;

	// players own view models, left & right hand
	CHandle< C_BaseViewModel >	m_hViewModel[ MAX_VIEWMODELS ];		
	
	float					m_flOldPlayerZ;
	float					m_flOldPlayerViewOffsetZ;
	
	Vector	m_vecVehicleViewOrigin;		// Used to store the calculated view of the player while riding in a vehicle
	QAngle	m_vecVehicleViewAngles;		// Vehicle angles
	float	m_flVehicleViewFOV;
	int		m_nVehicleViewSavedFrame;	// Used to mark which frame was the last one the view was calculated for

	// For UI purposes...
	int				m_iOldAmmo[ MAX_AMMO_TYPES ];

	C_CommandContext		m_CommandContext;

	// For underwater effects
	float							m_flWaterSurfaceZ;
	bool							m_bResampleWaterSurface;
	TimedEvent						m_tWaterParticleTimer;
	CSmartPtr<WaterDebrisEffect>	m_pWaterEmitter;

	bool							m_bPlayerUnderwater;

	friend class CPrediction;
	friend class C_GameMovement;
	
	// Accessors for gamemovement
	float GetStepSize( void ) const { return m_Local.m_flStepSize; }

	float m_flNextAvoidanceTime;
	float m_flAvoidanceRight;
	float m_flAvoidanceForward;
	float m_flAvoidanceDotForward;
	float m_flAvoidanceDotRight;

protected:
	virtual bool IsDucked( void ) const { return m_Local.m_bDucked; }
	virtual bool IsDucking( void ) const { return m_Local.m_bDucking; }
	virtual float GetFallVelocity( void ) { return m_Local.m_flFallVelocity; }
	bool ForceSetupBonesAtTimeFakeInterpolation( matrix3x4_t *pBonesOut, float curtimeOffset );

	float m_flLaggedMovementValue;

	// These are used to smooth out prediction corrections. They're most useful when colliding with
	// vphysics objects. The server will be sending constant prediction corrections, and these can help
	// the errors not be so jerky.
	Vector m_vecPredictionError;
	float m_flPredictionErrorTime;
	
	Vector m_vecPreviouslyPredictedOrigin; // Used to determine if non-gamemovement game code has teleported, or tweaked the player's origin

	char m_szLastPlaceName[MAX_PLACE_NAME_LENGTH];	// received from the server

	// Texture names and surface data, used by CGameMovement
	int				m_surfaceProps;
	surfacedata_t*	m_pSurfaceData;
	float			m_surfaceFriction;
	char			m_chTextureType;

	bool			m_bSentFreezeFrame;
	float			m_flFreezeZOffset;

	float			m_flNextAchievementAnnounceTime;

	vision_filter_t				m_nForceVisionFilterFlags; // Force our vision filter to a specific setting
	vision_filter_t				m_nLocalPlayerVisionFlags;

	bool					m_bIsLocalPlayer;

	virtual CPlayerAnimState *CreateAnimState();
	CPlayerAnimState* m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar<QAngle>	m_iv_angEyeAngles;

	void ThinkIDTarget();
	void UpdateIDTarget();
	Vector m_vLookAtTarget;
	int m_iIDEntIndex;

	int m_iSpawnInterpCounter;
	int m_iSpawnInterpCounterCache;

	int m_cycleLatch;
	float m_flServerCycle;

	CNetworkHandle( C_ColorCorrection, m_hColorCorrectionCtrl );		// active FXVolume color correction
	CNetworkHandle( C_BaseEntity, m_hTonemapController );
	CNetworkHandle( C_PostProcessController, m_hPostProcessCtrl );	// active postprocessing controller

	friend void GetTonemapSettingsFromEnvTonemapController();

private:

	struct StepSoundCache_t
	{
		StepSoundCache_t() : m_usSoundNameIndex( 0 ) {}
		CSoundParameters	m_SoundParameters;
		unsigned short		m_usSoundNameIndex;
	};
	// One for left and one for right side of step
	StepSoundCache_t		m_StepSoundCache[ 2 ];

public:

	const char *GetLastKnownPlaceName( void ) const	{ return m_szLastPlaceName; }	// return the last nav place name the player occupied

	float GetLaggedMovementValue( void ){ return m_flLaggedMovementValue;	}
	bool  ShouldGoSouth( Vector vNPCForward, Vector vNPCRight ); //Such a bad name.

	void SetOldPlayerZ( float flOld ) { m_flOldPlayerZ = flOld;	}

	const fogplayerparams_t& GetPlayerFog() const { return m_Local.m_PlayerFog; }

private:
	friend class C_MoveHelper;

};

EXTERN_RECV_TABLE(DT_BasePlayer);

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline C_BasePlayer *ToBasePlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#if _DEBUG
	Assert( dynamic_cast<C_BasePlayer *>( pEntity ) != NULL );
#endif

	return static_cast<C_BasePlayer *>( pEntity );
}

inline const C_BasePlayer *ToBasePlayer( const C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#if _DEBUG
	Assert( dynamic_cast<const C_BasePlayer *>( pEntity ) != NULL );
#endif

	return static_cast<const C_BasePlayer *>( pEntity );
}

inline C_BaseEntity *C_BasePlayer::GetUseEntity() const
{ 
	return m_hUseEntity;
}

inline C_BaseEntity* C_BasePlayer::GetPotentialUseEntity( void ) const 
{ 
	return GetUseEntity();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Iterate over all active players in the game, invoking functor on each.
 * If functor returns false, stop iteration and return false.
 */
template < typename Functor >
bool ForEachPlayer( Functor &func )
{
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		C_BasePlayer *player = static_cast<C_BasePlayer *>( UTIL_PlayerByIndex( i ) );

		if (player == NULL)
			continue;

		if (!player->IsPlayer())
			continue;

		if (func( player ) == false)
			return false;
	}

	return true;
}

inline IClientVehicle *C_BasePlayer::GetVehicle() 
{ 
	C_BaseEntity *pVehicleEnt = m_hVehicle.Get();
	return pVehicleEnt ? pVehicleEnt->GetClientVehicle() : NULL;
}

inline bool C_BasePlayer::IsObserver() const 
{ 
	return (GetObserverMode() != OBS_MODE_NONE); 
}

inline int C_BasePlayer::GetImpulse( void ) const 
{ 
	return m_nImpulse; 
}


inline C_CommandContext* C_BasePlayer::GetCommandContext()
{
	return &m_CommandContext;
}

inline int C_BasePlayer::CurrentCommandNumber() const
{
	Assert( m_pCurrentCommand );
	if ( !m_pCurrentCommand )
		return 0;
	return m_pCurrentCommand->command_number;
}

inline const CUserCmd *C_BasePlayer::GetCurrentUserCommand() const
{
	Assert( m_pCurrentCommand );
	return m_pCurrentCommand;
}

inline C_ColorCorrection* C_BasePlayer::GetActiveColorCorrection() const
{
	return m_hColorCorrectionCtrl.Get();
}

inline C_PostProcessController* C_BasePlayer::GetActivePostProcessController() const
{
	return m_hPostProcessCtrl.Get();
}

#endif // C_BASEPLAYER_H
