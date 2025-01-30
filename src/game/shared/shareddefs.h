//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definitions that are shared by the game DLL and the client DLL.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHAREDDEFS_H
#define SHAREDDEFS_H
#pragma once

#include "mathlib/vector.h"
#include "tier1/utlvector.h"
#include "tier1/convar.h"
#include "bittools.h"
#include "engine/ivmodelinfo.h"
#include "networkvar.h"
#include "datamap.h"
#include "soundflags.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

enum AmmoIndex_t : unsigned short;

#define TICK_INTERVAL			(gpGlobals->interval_per_tick)

#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )

#define TICK_ALWAYS_THINK	(-1293)
#define TICK_NEVER_THINK		(-1)

#define ANIMATION_CYCLE_BITS		15
#define ANIMATION_CYCLE_MINFRAC		(1.0f / (1<<ANIMATION_CYCLE_BITS))

// Matching the high level concept is significantly better than other criteria
// FIXME:  Could do this in the script file by making it required and bumping up weighting there instead...
#define CONCEPT_WEIGHT 5.0f

// Each mod defines these for itself.
class CViewVectors
{
public:
	CViewVectors() {}

	CViewVectors( 
		Vector vHullMin,
		Vector vHullMax,
		Vector vView,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight );

	// Height above entity position where the viewer's eye is.
	Vector m_vHullMin;
	Vector m_vHullMax;
	Vector m_vView;

	float m_flRadius;
	float m_flRadius2D;
	float m_flWidth;
	float m_flHeight;
	float m_flLength;

	Vector m_vDuckHullMin;
	Vector m_vDuckHullMax;
	Vector m_vDuckView;

	float m_flDuckRadius;
	float m_flDuckRadius2D;
	float m_flDuckWidth;
	float m_flDuckHeight;
	float m_flDuckLength;

	Vector m_vObsHullMin;
	Vector m_vObsHullMax;
	
	Vector m_vDeadViewHeight;
};

extern CViewVectors g_DefaultViewVectors;

#define VIEW_VECTORS ( GameRules() ? GameRules()->GetViewVectors() : &g_DefaultViewVectors )

// Height above entity position where the viewer's eye is.
#define VEC_HULL_MIN		VIEW_VECTORS->m_vHullMin
#define VEC_HULL_MAX		VIEW_VECTORS->m_vHullMax
#define VEC_VIEW			VIEW_VECTORS->m_vView

#define VEC_DUCK_HULL_MIN	VIEW_VECTORS->m_vDuckHullMin
#define VEC_DUCK_HULL_MAX	VIEW_VECTORS->m_vDuckHullMax
#define VEC_DUCK_VIEW		VIEW_VECTORS->m_vDuckView

#define VEC_OBS_HULL_MIN	VIEW_VECTORS->m_vObsHullMin
#define VEC_OBS_HULL_MAX	VIEW_VECTORS->m_vObsHullMax

#define VEC_DEAD_VIEWHEIGHT	VIEW_VECTORS->m_vDeadViewHeight

// If the player (enemy bots) are scaled, adjust the hull
#define VEC_VIEW_SCALED( player )				( VEC_VIEW * (player)->GetModelScale() )
#define VEC_HULL_MIN_SCALED( player )			( VEC_HULL_MIN * (player)->GetModelScale() )
#define VEC_HULL_MAX_SCALED( player )			( VEC_HULL_MAX * (player)->GetModelScale() )

#define VEC_DUCK_HULL_MIN_SCALED( player )		( VEC_DUCK_HULL_MIN * (player)->GetModelScale() )
#define VEC_DUCK_HULL_MAX_SCALED( player )		( VEC_DUCK_HULL_MAX * (player)->GetModelScale() )
#define VEC_DUCK_VIEW_SCALED( player )			( VEC_DUCK_VIEW * (player)->GetModelScale() )

#define VEC_OBS_HULL_MIN_SCALED( player )		( VEC_OBS_HULL_MIN * (player)->GetModelScale() )
#define VEC_OBS_HULL_MAX_SCALED( player )		( VEC_OBS_HULL_MAX * (player)->GetModelScale() )

#define VEC_DEAD_VIEWHEIGHT_SCALED( player )	( VEC_DEAD_VIEWHEIGHT * (player)->GetModelScale() )

#define PlayerDuckHeight VIEW_VECTORS->m_flDuckHeight
#define PlayerStandEyeHeight VEC_VIEW.z
#define PlayerDuckEyeHeight VEC_DUCK_VIEW.z

#define WATERJUMP_HEIGHT			8

#define MAX_CLIMB_SPEED		200

#define TIME_TO_DUCK		0.4
#define TIME_TO_DUCK_MS		400

#define TIME_TO_UNDUCK		0.2
#define TIME_TO_UNDUCK_MS	200

inline float FractionDucked( int msecs )
{
	return clamp( (float)msecs / (float)TIME_TO_DUCK_MS, 0.0f, 1.0f );
}

inline float FractionUnDucked( int msecs )
{
	return clamp( (float)msecs / (float)TIME_TO_UNDUCK_MS, 0.0f, 1.0f );
}

#define MAX_WEAPON_SLOTS		6	// hud item selection slots
#define MAX_WEAPON_POSITIONS	20	// max number of items within a slot
#define MAX_ITEM_TYPES			6	// hud item selection slots
#define MAX_WEAPONS				48	// Max number of weapons available

#define MAX_ITEMS				5	// hard coded item types

#define WEAPON_NOCLIP			-1	// clip sizes set to this tell the weapon it doesn't use a clip

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

// Vote creation or processing failure codes
enum vote_create_failed_t : unsigned char
{
	VOTE_FAILED_GENERIC = 0,
	VOTE_FAILED_TRANSITIONING_PLAYERS,
	VOTE_FAILED_RATE_EXCEEDED,
	VOTE_FAILED_YES_MUST_EXCEED_NO,
	VOTE_FAILED_QUORUM_FAILURE,
	VOTE_FAILED_ISSUE_DISABLED,
	VOTE_FAILED_MAP_NOT_FOUND,
	VOTE_FAILED_MAP_NAME_REQUIRED,
	VOTE_FAILED_ON_COOLDOWN,
	VOTE_FAILED_TEAM_CANT_CALL,
	VOTE_FAILED_WAITINGFORPLAYERS,
	VOTE_FAILED_PLAYERNOTFOUND,
	VOTE_FAILED_CANNOT_KICK_ADMIN,
	VOTE_FAILED_SCRAMBLE_IN_PROGRESS,
	VOTE_FAILED_SPECTATOR,
	VOTE_FAILED_NEXTLEVEL_SET,
	VOTE_FAILED_MAP_NOT_VALID,
	VOTE_FAILED_CANNOT_KICK_FOR_TIME,
	VOTE_FAILED_CANNOT_KICK_DURING_ROUND,
	VOTE_FAILED_VOTE_IN_PROGRESS,
	VOTE_FAILED_KICK_LIMIT_REACHED,

	// TF-specific?
	VOTE_FAILED_MODIFICATION_ALREADY_ACTIVE,
};

enum
{
#ifdef STAGING_ONLY
	SERVER_MODIFICATION_ITEM_DURATION_IN_MINUTES = 2
#else
	SERVER_MODIFICATION_ITEM_DURATION_IN_MINUTES = 120
#endif
};

#define MAX_VOTE_DETAILS_LENGTH 64
#define INVALID_ISSUE			-1
#define MAX_VOTE_OPTIONS		5
#define DEDICATED_SERVER		99

enum CastVote : unsigned char
{
	VOTE_OPTION1,  // Use this for Yes
	VOTE_OPTION2,  // Use this for No
	VOTE_OPTION3,
	VOTE_OPTION4,
	VOTE_OPTION5,
	VOTE_UNCAST
};

//===================================================================================================================
// Close caption flags
#define CLOSE_CAPTION_WARNIFMISSING	( 1<<0 )
#define CLOSE_CAPTION_FROMPLAYER	( 1<<1 )
#define CLOSE_CAPTION_GENDER_MALE	( 1<<2 )
#define CLOSE_CAPTION_GENDER_FEMALE	( 1<<3 )

//===================================================================================================================
// Hud Element hiding flags
enum Hidehud_t : unsigned short
{
	HIDEHUD_WEAPONSELECTION =		( 1<<0 ),	// Hide ammo count & weapon selection
	HIDEHUD_FLASHLIGHT =			( 1<<1 ),
	HIDEHUD_ALL =					( 1<<2 ),
	HIDEHUD_HEALTH =				( 1<<3 ),	// Hide health & armor / suit battery
	HIDEHUD_PLAYERDEAD =			( 1<<4 ),	// Hide when local player's dead
	HIDEHUD_NEEDSUIT =			( 1<<5 ),	// Hide when the local player doesn't have the HEV suit
	HIDEHUD_MISCSTATUS =			( 1<<6 ),	// Hide miscellaneous status elements (trains, pickup history, death notices, etc)
	HIDEHUD_CHAT =				( 1<<7 ),	// Hide all communication elements (saytext, voice icon, etc)
	HIDEHUD_CROSSHAIR =			( 1<<8 ),	// Hide crosshairs
	HIDEHUD_VEHICLE_CROSSHAIR =	( 1<<9 ),	// Hide vehicle crosshair
	HIDEHUD_INVEHICLE =			( 1<<10 ),
	HIDEHUD_BONUS_PROGRESS =		( 1<<11 ),	// Hide bonus progress display (for bonus map challenges)

	HIDEHUD_BITCOUNT =			12,
};

FLAGENUM_OPERATORS( Hidehud_t, unsigned short )

//===================================================================================================================
// suit usage bits
#define bits_SUIT_DEVICE_SPRINT		0x00000001
#define bits_SUIT_DEVICE_FLASHLIGHT	0x00000002
#define bits_SUIT_DEVICE_BREATHER	0x00000004

// Custom suit power devices
#define bits_SUIT_DEVICE_CUSTOM0	0x00000008
#define bits_SUIT_DEVICE_CUSTOM1	0x00000010
#define bits_SUIT_DEVICE_CUSTOM2	0x00000020

#define MAX_SUIT_DEVICES			6		// Mapbase boosts this to 6 for the custom devices

#define MAX_PLACE_NAME_LENGTH		18

#define MAX_FOV						90

//===================================================================================================================
// Team Defines

enum Team_t : unsigned char
{
	TEAM_ANY = (unsigned char)-2,
	TEAM_INVALID = (unsigned char)-1,

	TEAM_UNASSIGNED = 0,
	TEAM_NEUTRAL = 0,
	TEAM_SPECTATOR = 1,

	NUM_SHARED_TEAMS,
	LAST_SHARED_TEAM = TEAM_SPECTATOR,

	FIRST_GAME_TEAM = (LAST_SHARED_TEAM+1),
	SECOND_GAME_TEAM = (LAST_SHARED_TEAM+2),

#ifdef HEIST_DLL
	TEAM_CIVILIANS = TEAM_NEUTRAL,
	TEAM_HEISTERS = FIRST_GAME_TEAM,
	TEAM_POLICE = SECOND_GAME_TEAM,
	TEAM_RIVALS,

	NUM_HEIST_TEAMS,
#endif

	TEAMNUM_NUM_BITS = 6,
};

#define MAX_TEAMS				32	// Max number of teams in a game
#define MAX_TEAM_NAME_LENGTH	32	// Max length of a team's name

// Weapon m_iState
enum WeaponState_t : unsigned char
{
	WEAPON_IS_ONTARGET =				0x40,

	WEAPON_NOT_CARRIED =				0,	// Weapon is on the ground
	WEAPON_IS_CARRIED_BY_PLAYER =		1,	// This client is carrying this weapon.
	WEAPON_IS_ACTIVE =				2,	// This client is carrying this weapon and it's the currently held weapon
};

// -----------------------------------------
// Skill Level
// -----------------------------------------
enum Skill_t : unsigned char
{
	SKILL_EASY = 1,
	SKILL_MEDIUM = 2,
	SKILL_HARD = 3,
};

// Weapon flags
// -----------------------------------------
//	Flags - NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE
// -----------------------------------------
#define ITEM_FLAG_SELECTONEMPTY		(1<<0)
#define ITEM_FLAG_NOAUTORELOAD		(1<<1)
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	(1<<2)
#define ITEM_FLAG_LIMITINWORLD		(1<<3)
#define ITEM_FLAG_EXHAUSTIBLE		(1<<4)	// A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_DOHITLOCATIONDMG	(1<<5)	// This weapon take hit location into account when applying damage
#define ITEM_FLAG_NOAMMOPICKUPS		(1<<6)	// Don't draw ammo pickup sprites/sounds when ammo is received
#define ITEM_FLAG_NOITEMPICKUP		(1<<7)	// Don't draw weapon pickup when this weapon is picked up by the player
// NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE


// Humans only have left and right hands, though we might have aliens with more
//  than two, sigh
#define MAX_VIEWMODELS			2

#define VIEWMODEL_WEAPON 0
#define VIEWMODEL_HANDS 1

#define MAX_BEAM_ENTS			10

#define TRACER_TYPE_DEFAULT		0x00000001
#define TRACER_TYPE_WATERBULLET	0x00000002

#define MUZZLEFLASH_TYPE_DEFAULT	0x00000001

// Muzzle flash definitions (for the flags field of the "MuzzleFlash" DispatchEffect)
enum Muzzleflash_t : unsigned short
{
	MUZZLEFLASH_SHOTGUN,
	MUZZLEFLASH_PISTOL,
	MUZZLEFLASH_357,
	MUZZLEFLASH_RPG,
	MUZZLEFLASH_SMG1,

	MUZZLEFLASH_FIRSTPERSON		= 0x100,
};

// Tracer Flags
#define TRACER_FLAG_WHIZ			0x0001
#define TRACER_FLAG_USEATTACHMENT	0x0002

#define TRACER_DONT_USE_ATTACHMENT	-1

// Entity Dissolve types
enum EntityDissolve_t : unsigned char
{
	ENTITY_DISSOLVE_NORMAL = 0,
	ENTITY_DISSOLVE_ELECTRICAL,
	ENTITY_DISSOLVE_ELECTRICAL_LIGHT,
	ENTITY_DISSOLVE_CORE,

	LAST_DISSOLVE_TYPE = ENTITY_DISSOLVE_CORE,

	// NOTE: Be sure to up the bits if you make more dissolve types
	ENTITY_DISSOLVE_BITS = MINIMUM_BITS_NEEDED(LAST_DISSOLVE_TYPE),
};

#define PLAYER_FATAL_FALL_SPEED		1024 // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580 // approx 20 feet
#define PLAYER_LAND_ON_FLOATING_OBJECT	200 // Can go another 200 units without getting hurt
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHOLD 350 // won't punch player's screen/make scrape noise unless player falling at least this fast.
#define DAMAGE_FOR_FALL_SPEED		100.0f / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED ) // damage per unit per second.


#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669
#define AUTOAIM_20DEGREES 0.3490658503989

#define AUTOAIM_SCALE_DEFAULT		1.0f
#define AUTOAIM_SCALE_DIRECT_ONLY	0.0f

// settings for m_takedamage
enum Takedamage_t : unsigned char
{
	DAMAGE_NO =				0,
	DAMAGE_EVENTS_ONLY =		1,		// Call damage functions, but don't modify health
	DAMAGE_YES =				2,
	DAMAGE_AIM =				3,
};

// Spectator Movement modes
enum ObserverMode_t : unsigned char
{
	OBS_MODE_NONE = 0,	// not in spectator mode
	OBS_MODE_DEATHCAM,	// special mode for death cam animation
	OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
	OBS_MODE_FIXED,		// view from a fixed camera position
	OBS_MODE_IN_EYE,	// follow a player in first person view
	OBS_MODE_CHASE,		// follow a player in third person view
	OBS_MODE_POI,		// PASSTIME point of interest - game objective, big fight, anything interesting; added in the middle of the enum due to tons of hard-coded "<ROAMING" enum compares
	OBS_MODE_ROAMING,	// free roaming

	NUM_OBSERVER_MODES,

	LAST_PLAYER_OBSERVERMODE = OBS_MODE_ROAMING,
};

// Force Camera Restrictions with mp_forcecamera
enum ObserverRestriction_t : unsigned char
{
	OBS_ALLOW_ALL = 0,	// allow all modes, all targets
	OBS_ALLOW_TEAM,		// allow only own team & first person, no PIP
	OBS_ALLOW_NONE,		// don't allow any spectating after death (fixed & fade to black)

	OBS_ALLOW_NUM_MODES,
};

enum
{
	TYPE_TEXT = 0,	// just display this plain text
	TYPE_INDEX,		// lookup text & title in stringtable
	TYPE_URL,		// show this URL
	TYPE_FILE,		// show this local file
} ;

//=============================================================================
// HPE_BEGIN:
// [Forrest] Replaced text window command string with TEXTWINDOW_CMD enumeration
// of options.  Passing a command string is dangerous and allowed a server network
// message to run arbitrary commands on the client.
//=============================================================================
enum
{
	TEXTWINDOW_CMD_NONE = 0,
	TEXTWINDOW_CMD_JOINGAME,
	TEXTWINDOW_CMD_CHANGETEAM,
	TEXTWINDOW_CMD_IMPULSE101,
	TEXTWINDOW_CMD_MAPINFO,
	TEXTWINDOW_CMD_CLOSED_HTMLPAGE,
	TEXTWINDOW_CMD_CHOOSETEAM,
};
//=============================================================================
// HPE_END
//=============================================================================

// VGui Screen Flags
enum VGUIScreenFlags_t : unsigned char
{
	VGUI_SCREEN_NO_FLAGS = 0,

	VGUI_SCREEN_ACTIVE = 0x1,
	VGUI_SCREEN_VISIBLE_TO_TEAMMATES = 0x2,
	VGUI_SCREEN_ATTACHED_TO_VIEWMODEL = 0x4,
	VGUI_SCREEN_TRANSPARENT = 0x8,
	VGUI_SCREEN_ONLY_USABLE_BY_OWNER = 0x10,

	VGUI_SCREEN_LAST_FLAG = VGUI_SCREEN_ONLY_USABLE_BY_OWNER,

	VGUI_SCREEN_MAX_BITS = MINIMUM_BITS_NEEDED(VGUI_SCREEN_LAST_FLAG),
};

FLAGENUM_OPERATORS( VGUIScreenFlags_t, unsigned char )

enum USE_TYPE : unsigned char
{
	USE_OFF = 0, 
	USE_ON = 1, 
	USE_SET = 2, 
	USE_TOGGLE = 3
};

// basic team colors
#define COLOR_RED		Color(255, 64, 64, 255)
#define COLOR_BLUE		Color(153, 204, 255, 255)
#define COLOR_YELLOW	Color(255, 178, 0, 255)
#define COLOR_GREEN		Color(153, 255, 153, 255)
#define COLOR_GREY		Color(204, 204, 204, 255)
#define COLOR_WHITE		Color(255, 255, 255, 255)
#define COLOR_BLACK		Color(0, 0, 0, 255)

// All NPCs need this data
enum BloodColor_t : unsigned char
{
	DONT_BLEED = (unsigned char)-1,

	BLOOD_COLOR_RED = 0,
	BLOOD_COLOR_YELLOW,
	BLOOD_COLOR_GREEN,
	BLOOD_COLOR_BRIGHTGREEN,

	BLOOD_COLOR_SLIME,
	BLOOD_COLOR_FROZEN,

	BLOOD_COLOR_MECH,
	BLOOD_COLOR_ALIENINSECT,
	BLOOD_COLOR_UNDEAD,
};

//-----------------------------------------------------------------------------
// Vehicles may have more than one passenger.
// This enum may be expanded by derived classes
//-----------------------------------------------------------------------------
enum PassengerRole_t : unsigned char
{
	VEHICLE_ROLE_NONE = (unsigned char)-1,

	VEHICLE_ROLE_DRIVER = 0,	// Only one driver

	LAST_SHARED_VEHICLE_ROLE,
};

//-----------------------------------------------------------------------------
// Water splash effect flags
//-----------------------------------------------------------------------------
enum
{
	FX_WATER_IN_SLIME = 0x1,
};


// Shared think context stuff
#define	MAX_CONTEXT_LENGTH		32
#define NO_THINK_CONTEXT	-1

// entity capabilities
enum EntityCaps_t : uint64
{
	FCAP_NONE = 0,

// These are caps bits to indicate what an object's capabilities (currently used for +USE, save/restore and level transitions)
	FCAP_MUST_SPAWN = 0x00000001,		// Spawn after restore
	FCAP_ACROSS_TRANSITION = 0x00000002,		// should transfer between transitions 
	// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
	FCAP_FORCE_TRANSITION = 0x00000004,		// ALWAYS goes across transitions
	FCAP_NOTIFY_ON_TRANSITION = 0x00000008,		// Entity will receive Inside/Outside transition inputs when a transition occurs

	FCAP_IMPULSE_USE = 0x00000010,		// can be used by the player
	FCAP_CONTINUOUS_USE = 0x00000020,		// can be used by the player
	FCAP_ONOFF_USE = 0x00000040,		// can be used by the player
	FCAP_DIRECTIONAL_USE = 0x00000080,		// Player sends +/- 1 when using (currently only tracktrains)
	// NOTE: Normally +USE only works in direct line of sight.  Add these caps for additional searches
	FCAP_USE_ONGROUND = 0x00000100,
	FCAP_USE_IN_RADIUS = 0x00000200,

	FCAP_MASTER = 0x10000000,		// Can be used to "master" other entities (like multisource)
	FCAP_WCEDIT_POSITION = 0x40000000,		// Can change position and update Hammer in edit mode
};

FLAGENUM_OPERATORS( EntityCaps_t, uint64 )

// entity flags, CBaseEntity::m_iEFlags
enum EntityFlags_t : uint64
{
	EFL_NONE = 0,

	EFL_KILLME	=				(1<<0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT	=				(1<<1),	// Entity is dormant, no updates to client
	EFL_SETTING_UP_BONES =		(1<<2),	// Set while a model is setting up its bones.
	EFL_KEEP_ON_RECREATE_ENTITIES = (1<<3), // This is a special entity that should not be deleted when we restart entities only

	EFL_HAS_PLAYER_CHILD=		(1<<4),	// One of the child entities is a player.

	EFL_DIRTY_SHADOWUPDATE =	(1<<5),	// Client only- need shadow manager to update the shadow...
	EFL_NOTIFY =				(1<<6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT =	(1<<7),

	EFL_NOT_RENDERABLE =			(1<<8),	// This is set on bots that are frozen.
	EFL_NOT_NETWORKED =			(1<<9),	// Non-networked entity.
	
	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM =	(1<<10),
	EFL_DIRTY_ABSVELOCITY =		(1<<11),
	EFL_DIRTY_ABSANGVELOCITY =	(1<<12),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS	= (1<<13),
	EFL_DIRTY_SPATIAL_PARTITION = (1<<14),
	EFL_NOT_COLLIDEABLE						= (1<<15),

	EFL_IN_SKYBOX =				(1<<16),	// This is set if the entity detects that it's in the skybox.
											// This forces it to pass the "in PVS" for transmission.
	EFL_USE_PARTITION_WHEN_NOT_SOLID = (1<<17),	// Entities with this flag set show up in the partition even when not solid
	EFL_TOUCHING_FLUID =		(1<<18),	// Used to determine if an entity is floating

	// FIXME: Not really sure where I should add this...
	EFL_NO_ROTORWASH_PUSH =		(1<<19),		// I shouldn't be pushed by the rotorwash
	EFL_NO_THINK_FUNCTION =		(1<<20),
	EFL_NO_GAME_PHYSICS_SIMULATION = (1<<21),

	EFL_CHECK_UNTOUCH =			(1<<22),
	EFL_DONTBLOCKLOS =			(1<<23),		// I shouldn't block NPC line-of-sight
	EFL_DONTWALKON =			(1<<24),		// NPC;s should not walk on this entity
	EFL_NO_DISSOLVE =			(1<<25),		// These guys shouldn't dissolve
	EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1<<26),	// Mega physcannon can't ragdoll these guys.
	EFL_NO_WATER_VELOCITY_CHANGE  =	(1<<27),	// Don't adjust this entity's velocity when transitioning into water
	EFL_NO_PHYSCANNON_INTERACTION =	(1<<28),	// Physcannon can't pick these up or punt them
	EFL_NO_DAMAGE_FORCES =		(1<<29),	// Doesn't accept forces from physics damage
};

FLAGENUM_OPERATORS( EntityFlags_t, uint64 )

//
// Constants shared by the engine and dlls
// This header file included by engine files and DLL files.
// Most came from server.h

// CBaseEntity::m_fFlags
// PLAYER SPECIFIC FLAGS FIRST BECAUSE WE USE ONLY A FEW BITS OF NETWORK PRECISION
// This top block is for singleplayer games only....no HL2:DM (which defines HL2_DLL)

enum EntityBehaviorFlags_t : uint64
{
	FL_NO_ENTITY_FLAGS = 0,

	FL_ONGROUND = (1<<0),	// At rest / on the ground
	FL_DUCKING = (1<<1),	// Player flag -- Player is fully crouched
	FL_ANIMDUCKING = (1<<2),	// Player flag -- Player is in the process of crouching or uncrouching but could be in transition
	// examples:                                   Fully ducked:  FL_DUCKING &  FL_ANIMDUCKING
	//           Previously fully ducked, unducking in progress:  FL_DUCKING & !FL_ANIMDUCKING
	//                                           Fully unducked: !FL_DUCKING & !FL_ANIMDUCKING
	//           Previously fully unducked, ducking in progress: !FL_DUCKING &  FL_ANIMDUCKING
	FL_WATERJUMP = (1<<3),	// player jumping out of water
	FL_ONTRAIN = (1<<4), // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
	FL_INRAIN = (1<<5),	// Indicates the entity is standing in rain
	FL_FROZEN = (1<<6), // Player is frozen for 3rd person camera
	FL_ATCONTROLS = (1<<7), // Player can't move, but keeps key inputs for controlling another entity
	FL_CLIENT = (1<<8),	// Is a player
	FL_FAKECLIENT = (1<<9),	// Fake client, simulated server side; don't send network messages to them
	// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
	FL_INWATER = (1<<10),	// In water

	// NOTE if you move things up, make sure to change this value
	PLAYER_FLAG_BITS = 11,

	FL_FLY = (1<<11),	// Changes the SV_Movestep() behavior to not need to be on ground
	FL_SWIM = (1<<12),	// Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
	FL_CONVEYOR = (1<<13),
	FL_NPC = (1<<14),
	FL_GODMODE = (1<<15),
	FL_NOTARGET = (1<<16),
	FL_AIMTARGET = (1<<17),	// set if the crosshair needs to aim onto the entity
	FL_PARTIALGROUND = (1<<18),	// not all corners are valid
	FL_STATICPROP = (1<<19),	// Eetsa static prop!		
	FL_FREEZING = (1<<20), // We're becoming frozen!
	FL_GRENADE = (1<<21),
	FL_STEPMOVEMENT = (1<<22),	// Changes the SV_Movestep() behavior to not do any processing
	FL_DONTTOUCH = (1<<23),	// Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
	FL_BASEVELOCITY = (1<<24),	// Base velocity has been applied this frame (used to convert base velocity into momentum)
	FL_WORLDBRUSH = (1<<25),	// Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
	FL_OBJECT = (1<<26), // Terrible name. This is an object that NPCs should see. Missiles, for example.
	FL_ONFIRE = (1<<27),	// You know...
	FL_DISSOLVING = (1<<28), // We're dissolving!
	FL_TRANSRAGDOLL = (1<<29), // In the process of turning into a client side ragdoll.
	FL_UNBLOCKABLE_BY_PLAYER = (1<<30), // pusher that can't be blocked by the player
};

FLAGENUM_OPERATORS( EntityBehaviorFlags_t, uint64 )

//-----------------------------------------------------------------------------
// EFFECTS
//-----------------------------------------------------------------------------
enum BloodSprayFlags_t : unsigned char
{
	FX_BLOODSPRAY_DROPS	= 0x01,
	FX_BLOODSPRAY_GORE	= 0x02,
	FX_BLOODSPRAY_CLOUD	= 0x04,
	FX_BLOODSPRAY_ALL		= 0xFF,
};

FLAGENUM_OPERATORS( BloodSprayFlags_t, unsigned char )

//-----------------------------------------------------------------------------
#define MAX_SCREEN_OVERLAYS		10

// These are the types of data that hang off of CBaseEntities and the flag bits used to mark their presence
enum DataObject_t : unsigned char
{
	GROUNDLINK = 0,
	TOUCHLINK,
	STEPSIMULATION,
	MODELSCALE,
	POSITIONWATCHER,
	PHYSICSPUSHLIST,
	VPHYSICSUPDATEAI,
	VPHYSICSWATCHER,

	// Must be last and <= 32
	NUM_DATAOBJECT_TYPES,
};

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#elif defined CLIENT_DLL
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

//-----------------------------------------------------------------------------
// Bullet firing information
//-----------------------------------------------------------------------------

enum FireBulletsFlags_t : unsigned char
{
	FIRE_BULLETS_NO_FLAGS = 0,
	FIRE_BULLETS_FIRST_SHOT_ACCURATE = 0x1,		// Pop the first shot with perfect accuracy
	FIRE_BULLETS_DONT_HIT_UNDERWATER = 0x2,		// If the shot hits its target underwater, don't damage it
	FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS = 0x4,	// If the shot hits water surface, still call DoImpactEffect
	FIRE_BULLETS_TEMPORARY_DANGER_SOUND = 0x8,		// Danger sounds added from this impact can be stomped immediately if another is queued
	FIRE_BULLETS_NO_PIERCING_SPARK = 0x16,	// do a piercing spark effect when a bullet penetrates an alien
	FIRE_BULLETS_HULL = 0x32,	// bullet trace is a hull rather than a line
	FIRE_BULLETS_ANGULAR_SPREAD = 0x64,	// bullet spread is based on uniform random change to angles rather than gaussian search
	FIRE_BULLETS_NO_AUTO_GIB_TYPE = 0x10,		// Don't automatically add DMG_ALWAYSGIB or DMG_NEVERGIB if m_flDamage is set
};

FLAGENUM_OPERATORS( FireBulletsFlags_t, unsigned char )

struct FireBulletsInfo_t
{
	FireBulletsInfo_t();

	FireBulletsInfo_t( int nShots, const Vector &vecSrc, const Vector &vecDir, const Vector &vecSpread, float flDistance, AmmoIndex_t nAmmoType, bool bPrimaryAttack = true );

	FireBulletsInfo_t( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, AmmoIndex_t iAmmoType, int iTracerFreq, float flDamage, CSharedBaseEntity *pAttacker, bool bFirstShotAccurate, bool bPrimaryAttack );

	~FireBulletsInfo_t() {}

	int m_iShots;
	Vector m_vecSrc;
	Vector m_vecDirShooting;
	Vector m_vecSpread;
	float m_flDistance;
	AmmoIndex_t m_iAmmoType;
	int m_iTracerFreq;
	float m_flDamage;
	float m_flPlayerDamage;	// Damage to be used instead of m_flDamage if we hit a player
	FireBulletsFlags_t m_nFlags;			// See FireBulletsFlags_t
	float m_flDamageForceScale;
	CSharedBaseEntity *m_pAttacker;
	CSharedBaseEntity *m_pAdditionalIgnoreEnt;
	bool m_bPrimaryAttack;
	bool m_bUseServerRandomSeed;

	// This variable is like m_pAdditionalIgnoreEnt, but it's a list of entities instead of just one.
	// Since func_tanks already use m_pAdditionalIgnoreEnt for parents, they needed another way to stop hitting their controllers.
	// After much trial and error, I decided to just add more excluded entities to the bullet firing info.
	// It could've just been a single entity called "m_pAdditionalIgnoreEnt2", but since these are just pointers,
	// I planned ahead and made it a CUtlVector instead.
	CUtlVector<CSharedBaseEntity*> *m_pIgnoreEntList;

	int GetShots() { return m_iShots; }
	void SetShots( int value ) { m_iShots = value; }

	Vector GetSource() { return m_vecSrc; }
	void SetSource( Vector value ) { m_vecSrc = value; }
	Vector GetDirShooting() { return m_vecDirShooting; }
	void SetDirShooting( Vector value ) { m_vecDirShooting = value; }
	Vector GetSpread() { return m_vecSpread; }
	void SetSpread( Vector value ) { m_vecSpread = value; }

	float GetDistance() { return m_flDistance; }
	void SetDistance( float value ) { m_flDistance = value; }

	AmmoIndex_t GetAmmoType() { return m_iAmmoType; }
	void SetAmmoType( AmmoIndex_t value ) { m_iAmmoType = value; }

	int GetTracerFreq() { return m_iTracerFreq; }
	void SetTracerFreq( int value ) { m_iTracerFreq = value; }

	float GetDamage() { return m_flDamage; }
	void SetDamage( float value ) { m_flDamage = value; }
	int GetPlayerDamage() { return m_flPlayerDamage; }
	void SetPlayerDamage( float value ) { m_flPlayerDamage = value; }

	FireBulletsFlags_t GetFlags() { return m_nFlags; }
	void SetFlags( FireBulletsFlags_t value ) { m_nFlags = value; }

	float GetDamageForceScale() { return m_flDamageForceScale; }
	void SetDamageForceScale( float value ) { m_flDamageForceScale = value; }

	bool GetPrimaryAttack() { return m_bPrimaryAttack; }
	void SetPrimaryAttack( bool value ) { m_bPrimaryAttack = value; }
};

//-----------------------------------------------------------------------------
// Purpose: Data for making the MOVETYPE_STEP entities appear to simulate every frame
//  We precompute the simulation and then meter it out each tick during networking of the 
//  entities origin and orientation.  Uses a bit more bandwidth, but it solves the NPCs interacting
//  with elevators/lifts bugs.
//-----------------------------------------------------------------------------
struct StepSimulationStep
{
	int			nTickCount;
	Vector		vecOrigin;
	Quaternion	qRotation;
};

struct StepSimulationData
{
	// Are we using the Step Simulation Data
	bool		m_bOriginActive;
	bool		m_bAnglesActive;

	// This is the pre-pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous2;

	// This is the pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous;

	// This is a potential mid-think position, orientation (Quaternion) and tick count
	// Used to mark motion discontinuities that happen between thinks
	StepSimulationStep	m_Discontinuity;

	// This is the goal or post-Think position and orientation (and Quaternion for blending) and next think time tick
	StepSimulationStep	m_Next;
	QAngle		m_angNextRotation;

	// This variable is used so that we only compute networked origin/angles once per tick
	int			m_nLastProcessTickCount;
	// The computed/interpolated network origin/angles to use
	Vector		m_vecNetworkOrigin;
	QAngle		m_angNetworkAngles;

	int			m_networkCell[3];
};

//-----------------------------------------------------------------------------
// Purpose: Simple state tracking for changing model sideways shrinkage during barnacle swallow
//-----------------------------------------------------------------------------
struct ModelScale
{
	float		m_flModelScaleStart;
	float		m_flModelScaleGoal;
	float		m_flModelScaleFinishTime;
	float		m_flModelScaleStartTime;
};

struct CSoundParameters;

//-----------------------------------------------------------------------------
// Purpose: Aggregates and sets default parameters for EmitSound function calls
//-----------------------------------------------------------------------------
struct EmitSound_t
{
	EmitSound_t();

	EmitSound_t( const CSoundParameters &src );

	SoundChannel_t							m_nChannel;
	char const					*m_pSoundName;
	float						m_flVolume;
	soundlevel_t				m_SoundLevel;
	SoundFlags_t							m_nFlags;
	int							m_nPitch;
	int							m_nSpecialDSP;
	const Vector				*m_pOrigin;
	float						m_flSoundTime; ///< NOT DURATION, but rather, some absolute time in the future until which this sound should be delayed
	float						*m_pflSoundDuration;
	bool						m_bEmitCloseCaption;
	bool						m_bWarnOnMissingCloseCaption;
	bool						m_bWarnOnDirectWaveReference;
	int							m_nSpeakerEntity;
	mutable CUtlVector< Vector >	m_UtlVecSoundOrigin;  ///< Actual sound origin(s) (can be multiple if sound routed through speaker entity(ies) )
	mutable HSOUNDSCRIPTHANDLE		m_hSoundScriptHandle;
};

#define MAX_ACTORS_IN_SCENE 16

//-----------------------------------------------------------------------------
// Multiplayer specific defines
//-----------------------------------------------------------------------------
#define MAX_CONTROL_POINTS			8
#define MAX_CONTROL_POINT_GROUPS	8

// Maximum number of points that a control point may need owned to be cappable
#define MAX_PREVIOUS_POINTS			3

// The maximum number of teams the control point system knows how to deal with
#define MAX_CONTROL_POINT_TEAMS		8

// Maximum length of the cap layout string
#define MAX_CAPLAYOUT_LENGTH		32

// Maximum length of the current round printname
#define MAX_ROUND_NAME				32

// Maximum length of the current round name
#define MAX_ROUND_IMAGE_NAME		64

// Score added to the team score for a round win
#define TEAMPLAY_ROUND_WIN_SCORE	1

enum
{
	CP_WARN_NORMAL = 0,
	CP_WARN_FINALCAP,
	CP_WARN_NO_ANNOUNCEMENTS
};

//-----------------------------------------------------------------------------
// Round timer states
//-----------------------------------------------------------------------------
enum
{
	RT_STATE_SETUP,		// Timer is in setup mode
	RT_STATE_NORMAL,	// Timer is in normal mode
};

//-----------------------------------------------------------------------------
// Cell origin values
//-----------------------------------------------------------------------------
#define CELL_COUNT( bits ) ( (MAX_COORD_INTEGER*2) / (1 << (bits)) ) // How many cells on an axis based on the bit size of the cell
#define CELL_COUNT_BITS( bits ) MINIMUM_BITS_NEEDED( CELL_COUNT( bits ) ) // How many bits are necessary to respresent that cell
#define CELL_BASEENTITY_ORIGIN_CELL_BITS 5 // default amount of entropy bits for base entity

enum
{
	SIMULATION_TIME_WINDOW_BITS = 8,
};

#define TEAM_TRAIN_MAX_TEAMS			4
#define TEAM_TRAIN_MAX_HILLS			5
#define TEAM_TRAIN_FLOATS_PER_HILL		2
#define TEAM_TRAIN_HILLS_ARRAY_SIZE		TEAM_TRAIN_MAX_TEAMS * TEAM_TRAIN_MAX_HILLS * TEAM_TRAIN_FLOATS_PER_HILL

enum Class_T : unsigned char
{
	CLASS_ANY = (unsigned char)-2,
	CLASS_INVALID = (unsigned char)-1,

	CLASS_NONE = 0,
	CLASS_PLAYER,

	NUM_SHARED_ENTITY_CLASSES,
	LAST_SHARED_ENTITY_CLASS = CLASS_PLAYER,

	FIRST_GAME_CLASS = (LAST_SHARED_ENTITY_CLASS+1),

#ifdef HEIST_DLL
	CLASS_HEISTER = FIRST_GAME_CLASS,
	CLASS_CIVILIAN,
	CLASS_POLICE,
	CLASS_RIVALS,

	NUM_HEIST_ENTITY_CLASSES,
#endif
};

// Factions
enum Faction_T : unsigned char
{
	FACTION_ANY = (unsigned char)-2,
	FACTION_INVALID = (unsigned char)-1,

	FACTION_NONE = 0, // Not assigned a faction.  Entities not assigned a faction will not do faction tests.

	NUM_SHARED_FACTIONS,
	LAST_SHARED_FACTION = FACTION_NONE,

	FIRST_GAME_FACTION = (LAST_SHARED_FACTION+1),

#ifdef HEIST_DLL
	FACTION_CIVILIANS = FIRST_GAME_FACTION,
	FACTION_HEISTERS,
	FACTION_APEX_SECURITY,
	FACTION_ABBADON_CIRCLE,
	FACTION_GOLDEN_HOTEL,
	FACTION_STEEL_N_SPEAR_PMC,
	FACTION_NY_SWAT,

	NUM_HEIST_FACTIONS,
#endif
};

enum
{
	HILL_TYPE_NONE = 0,
	HILL_TYPE_UPHILL,
	HILL_TYPE_DOWNHILL,
};

#define NOINTERP_PARITY_MAX			4
#define NOINTERP_PARITY_MAX_BITS	MINIMUM_BITS_NEEDED(NOINTERP_PARITY_MAX)

//-----------------------------------------------------------------------------
// For invalidate physics recursive
//-----------------------------------------------------------------------------
enum InvalidatePhysicsBits_t : unsigned char
{
	POSITION_CHANGED	= 0x1,
	ANGLES_CHANGED		= 0x2,
	VELOCITY_CHANGED	= 0x4,
	ANIMATION_CHANGED	= 0x8,		// Means cycle has changed, or any other event which would cause render-to-texture shadows to need to be rerendeded
	BOUNDS_CHANGED		= 0x10,		// Means render bounds have changed, so shadow decal projection is required, etc.
	SEQUENCE_CHANGED	= 0x20,		// Means sequence has changed, only interesting when surrounding bounds depends on sequence																				
};

FLAGENUM_OPERATORS( InvalidatePhysicsBits_t, unsigned char )

//-----------------------------------------------------------------------------
// Generic activity lookup support
//-----------------------------------------------------------------------------
enum
{
	kActivityLookup_Unknown = -2,			// hasn't been searched for
	kActivityLookup_Missing = -1,			// has been searched for but wasn't found
};

//-----------------------------------------------------------------------------
// Vision Filters.
//-----------------------------------------------------------------------------
#define MAX_HOLIDAY_STRING 64

enum EHolidayIndex : unsigned int
{
	HOLIDAY_ID_INVALID = (unsigned int)-1,

	HOLIDAY_ID_NONE = 0,

	HOLIDAY_ID_GAME_RELEASE,
	HOLIDAY_ID_HALLOWEEN,
	HOLIDAY_ID_CHRISTMAS,
	HOLIDAY_ID_VALENTINES,
	HOLIDAY_ID_FULL_MOON,
	HOLIDAY_ID_APRIL_FOOLS,

	NUM_SHARED_HOLIDAYS,

	LAST_SHARED_HOLIDAY_ID = HOLIDAY_ID_APRIL_FOOLS,

	FIRST_GAME_HOLIDAY_ID = (LAST_SHARED_HOLIDAY_ID+1),

	MAX_SUPPORTED_HOLIDAYS = 32,
};

enum EHolidayFlags : unsigned int
{
	HOLIDAY_FLAG_NONE =                   0,

	HOLIDAY_FLAG_GAME_RELEASE =     (1 << (HOLIDAY_ID_GAME_RELEASE >> 1)),
	HOLIDAY_FLAG_HALLOWEEN =        (1 << (HOLIDAY_ID_HALLOWEEN >> 1)),
	HOLIDAY_FLAG_CHRISTMAS =        (1 << (HOLIDAY_ID_CHRISTMAS >> 1)),
	HOLIDAY_FLAG_VALENTINES =       (1 << (HOLIDAY_ID_VALENTINES >> 1)),
	HOLIDAY_FLAG_FULL_MOON =        (1 << (HOLIDAY_ID_FULL_MOON >> 1)),
	HOLIDAY_FLAG_APRIL_FOOLS =      (1 << (HOLIDAY_ID_APRIL_FOOLS >> 1)),

	HOLIDAY_FLAG_HALLOWEEN_OR_FULL_MOON = (HOLIDAY_FLAG_HALLOWEEN|HOLIDAY_FLAG_FULL_MOON),
	HOLIDAY_FLAG_HALLOWEEN_OR_FULL_MOON_OR_VALENTINES = (HOLIDAY_FLAG_HALLOWEEN_OR_FULL_MOON|HOLIDAY_FLAG_VALENTINES),

	LAST_SHARED_HOLIDAY_FLAG = HOLIDAY_FLAG_APRIL_FOOLS,

	FIRST_GAME_HOLIDAY_FLAG = (LAST_SHARED_HOLIDAY_FLAG << 1),
};

FLAGENUM_OPERATORS( EHolidayFlags, unsigned int )

//undefined behavior if more then one
typedef EHolidayFlags EHolidayFlag;

#ifdef CLIENT_DLL
//the engine checks this cvar for 0x01
extern ConVar localplayer_visionflags;
#endif

enum vision_filter_t : unsigned int
{
	VISION_FILTER_NONE = 0,

	VISION_FILTER_ENGINE_MAT_REPLACEMENT = 0x01,

	VISION_FILTER_GAME_RELEASE =   (HOLIDAY_FLAG_GAME_RELEASE << 1),
	VISION_FILTER_HALLOWEEN =      (HOLIDAY_FLAG_HALLOWEEN << 1),
	VISION_FILTER_CHRISTMAS =      (HOLIDAY_FLAG_CHRISTMAS << 1),
	VISION_FILTER_VALENTINES =     (HOLIDAY_FLAG_VALENTINES << 1),
	VISION_FILTER_FULL_MOON =      (HOLIDAY_FLAG_FULL_MOON << 1),
	VISION_FILTER_APRIL_FOOLS =    (HOLIDAY_FLAG_APRIL_FOOLS << 1),

	VISION_FILTER_HALLOWEEN_OR_FULL_MOON = (VISION_FILTER_HALLOWEEN|VISION_FILTER_FULL_MOON),
	VISION_FILTER_HALLOWEEN_OR_FULL_MOON_OR_VALENTINES = (VISION_FILTER_HALLOWEEN_OR_FULL_MOON|VISION_FILTER_VALENTINES),

	VISION_FILTER_LAST_HOLIDAY_FLAG = VISION_FILTER_APRIL_FOOLS,

	VISION_FILTER_LOW_VIOLENCE = (VISION_FILTER_LAST_HOLIDAY_FLAG << 1),

	LAST_SHARED_VISION_FILTER_FLAG = VISION_FILTER_LOW_VIOLENCE,

	NUM_SHARED_VISION_FILTERS = (NUM_SHARED_HOLIDAYS+2),

	MAX_SUPPORTED_VISION_FILTERS = 32,

	FRIST_GAME_VISION_FILTER = (LAST_SHARED_VISION_FILTER_FLAG << 1),
};

DECLARE_FIELD_ENUM( vision_filter_t )

struct VisionModelIndex_t
{
public:
	DECLARE_CLASS_NOBASE( VisionModelIndex_t );
	DECLARE_EMBEDDED_NETWORKVAR_NOCHECK()

	VisionModelIndex_t(const VisionModelIndex_t &) = delete;
	VisionModelIndex_t &operator=(const VisionModelIndex_t &) = delete;
	VisionModelIndex_t(VisionModelIndex_t &&) = delete;
	VisionModelIndex_t &operator=(VisionModelIndex_t &&) = delete;

private:
#ifdef GAME_DLL
	friend class CBaseEntity;
#else
	friend class C_BaseEntity;
#endif

	VisionModelIndex_t( vision_filter_t flags_, modelindex_t mdlindex )
		: flags( flags_ ), modelindex( mdlindex )
	{
	}

	vision_filter_t flags;
	modelindex_t modelindex;
};

enum view_id_t : unsigned char
{
	VIEW_ID_ILLEGAL = (unsigned char)-2,
	VIEW_ID_NONE = (unsigned char)-1,
	VIEW_ID_MAIN = 0,
	VIEW_ID_3DSKY = 1,
	VIEW_ID_MONITOR = 2,
	VIEW_ID_REFLECTION = 3,
	VIEW_ID_REFRACTION = 4,
	VIEW_ID_INTRO_PLAYER = 5,
	VIEW_ID_INTRO_CAMERA = 6,
	VIEW_ID_SHADOW_DEPTH_TEXTURE = 7,
	VIEW_ID_SSAO = 8,
	VIEW_ID_COUNT,
	VIEW_ID_BITS = MINIMUM_BITS_NEEDED(VIEW_ID_COUNT),
};

enum view_flags_t : unsigned short
{
	VIEW_FLAG_NONE = 0,

	VIEW_FLAG_MAIN = (1 << VIEW_ID_MAIN),
	VIEW_FLAG_3DSKY = (1 << VIEW_ID_3DSKY),
	VIEW_FLAG_MONITOR = (1 << VIEW_ID_MONITOR),
	VIEW_FLAG_REFLECTION = (1 << VIEW_ID_REFLECTION),
	VIEW_FLAG_REFRACTION = (1 << VIEW_ID_REFRACTION),
	VIEW_FLAG_INTRO_PLAYER = (1 << VIEW_ID_INTRO_PLAYER),
	VIEW_FLAG_INTRO_CAMERA = (1 << VIEW_ID_INTRO_CAMERA),
	VIEW_FLAG_SHADOW_DEPTH_TEXTURE = (1 << VIEW_ID_SHADOW_DEPTH_TEXTURE),
	VIEW_FLAG_ID_SSAO = (1 << VIEW_ID_SSAO),

	VIEW_LAST_FLAG = VIEW_FLAG_ID_SSAO,

	VIEW_FLAG_BITS = MINIMUM_BITS_NEEDED(VIEW_LAST_FLAG),
};

FLAGENUM_OPERATORS( view_flags_t, unsigned short )
DECLARE_FIELD_ENUM( view_flags_t )

enum tprbGameInfo_e
{
	// Teamplay Roundbased Game rules shared
	TPRBGAMEINFO_GAMESTATE = 1,					//gets the state of the current game (waiting for players, setup, active, overtime, stalemate, roundreset)
	TPRBGAMEINFO_RESERVED1,
	TPRBGAMEINFO_RESERVED2,
	TPRBGAMEINFO_RESERVED3,
	TPRBGAMEINFO_RESERVED4,
	TPRBGAMEINFO_RESERVED5,
	TPRBGAMEINFO_RESERVED6,
	TPRBGAMEINFO_RESERVED7,
	TPRBGAMEINFO_RESERVED8,

	TPRBGAMEINFO_LASTGAMEINFO,
};
// Mark it off so valvegame_plugin_def.h ignores it, if both headers are included in a plugin.
#define TPRBGAMEINFO_x 1

//Tony; (t)eam(p)lay(r)ound(b)ased gamerules -- Game Info values
#define TPRB_STATE_WAITING				(1<<0)
#define TPRB_STATE_SETUP				(1<<1)
#define TPRB_STATE_ACTIVE				(1<<2)
#define TPRB_STATE_ROUNDWON				(1<<3)
#define TPRB_STATE_OVERTIME				(1<<4)
#define TPRB_STATE_STALEMATE			(1<<5)
#define TPRB_STATE_ROUNDRESET			(1<<6)
#define TPRB_STATE_WAITINGREADYSTART	(1<<7)


#endif // SHAREDDEFS_H
