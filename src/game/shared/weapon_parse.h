//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_PARSE_H
#define WEAPON_PARSE_H
#pragma once

#include "shareddefs.h"
#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL( LOG_WEAPONPARSE );

class IFileSystem;

typedef unsigned short WEAPON_FILE_INFO_HANDLE;

// -----------------------------------------------------------
// Weapon sound types
// Used to play sounds defined in the weapon's classname.txt file
// This needs to match pWeaponSoundCategories in weapon_parse.cpp
// ------------------------------------------------------------
typedef enum {
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE, // Can't be "DOUBLE" because windows.h uses it.
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	DEPLOY,
	FAST_RELOAD,

	// Add new shoot sound types here

	NUM_SHOOT_SOUND_TYPES,
} WeaponSound_t;

int GetWeaponSoundFromString( const char *pszString );

#define MAX_SHOOT_SOUNDS	16			// Maximum number of shoot sounds per shoot type

#define MAX_WEAPON_STRING	80
#define MAX_WEAPON_PREFIX	16
#define MAX_WEAPON_AMMO_NAME		32

#define WEAPON_PRINTNAME_MISSING "!!! Missing printname on weapon"

class CHudTexture;
class KeyValues;

enum WeaponUsageRestricions_e
{
	WPNRESTRICT_NONE = 0,
	WPNRESTRICT_PLAYER_ONLY,
	WPNRESTRICT_NPCS_ONLY,

	NUM_WEAPON_RESTRICTION_TYPES
};

enum
{
	WEAPONINFO_UNPARSED,
	WEAPONINFO_PARSED_MOD,
	WEAPONINFO_PARSED_MOD_ENCRYPTED,
	WEAPONINFO_PARSED_MAP,
	WEAPONINFO_PARSED_MOD_MAP_OVERWRITTEN,
};

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the weapon's script file. 
// It's cached so we only read each weapon's script file once.
// Each game provides a CreateWeaponInfo function so it can have game-specific
// data (like CS move speeds) in the weapon script.
//-----------------------------------------------------------------------------
class FileWeaponInfo_t
{
public:

	FileWeaponInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szWeaponName );

	
public:
#ifdef CLIENT_DLL
	KeyValues *pKeyValuesData;
#endif

	int					nParsedScript;

	bool					bLoadedHudElements;

// SHARED
	char					szClassName[MAX_WEAPON_STRING];
	char					szPrintName[MAX_WEAPON_STRING];			// Name for showing in HUD, etc.

	char					szViewModel[MAX_WEAPON_STRING];			// View model of this weapon
	char					szWorldModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player
	char					szAnimationPrefix[MAX_WEAPON_PREFIX];	// Prefix of the animations that should be used by the player carrying this weapon
	int						iSlot;									// inventory slot.
	int						iPosition;								// position in the inventory slot.
	int						iMaxClip1;								// max primary clip size (-1 if no clip)
	int						iMaxClip2;								// max secondary clip size (-1 if no clip)
	int						iDefaultClip1;							// amount of primary ammo in the gun when it's created
	int						iDefaultClip2;							// amount of secondary ammo in the gun when it's created
	int						iWeight;								// this value used to determine this weapon's importance in autoselection.
	int						iRumbleEffect;							// Which rumble effect to use when fired? (xbox)
	bool					bAutoSwitchTo;							// whether this weapon should be considered for autoswitching to
	bool					bAutoSwitchFrom;						// whether this weapon can be autoswitched away from when picking up another weapon or ammo
	int						iFlags;									// miscellaneous weapon flags
	char					szAmmo1[MAX_WEAPON_AMMO_NAME];			// "primary" ammo type
	char					szAmmo2[MAX_WEAPON_AMMO_NAME];			// "secondary" ammo type
	char					szAIAddOn[MAX_WEAPON_STRING];			// addon that this weapon can become

	// Sound blocks
	char					aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];	

	int						iAmmoType;
	int						iAmmo2Type;
	bool					m_bMeleeWeapon;		// Melee weapons can always "fire" regardless of ammo.

	// This tells if the weapon was built right-handed (defaults to true).
	// This helps cl_righthand make the decision about whether to flip the model or not.
	bool					m_bBuiltRightHanded;
	bool					m_bAllowFlipping;	// False to disallow flipping the model, regardless of whether
												// it is built left or right handed.

	float					m_flViewmodelFOV;
	float					m_flBobScale;
	float					m_flSwayScale;
	float					m_flSwaySpeedScale;

	char					szDroppedModel[MAX_WEAPON_STRING];		// Model of this weapon when dropped on the ground

	bool					m_bUsesHands;

	int						m_nWeaponRestriction;

// CLIENT DLL
	// Sprite data, read from the data file
	int						iSpriteCount;
	CHudTexture						*iconActive;
	CHudTexture	 					*iconInactive;
	CHudTexture 					*iconAmmo;
	CHudTexture 					*iconAmmo2;
	CHudTexture 					*iconCrosshair;
	CHudTexture 					*iconAutoaim;
	CHudTexture 					*iconZoomedCrosshair;
	CHudTexture 					*iconZoomedAutoaim;
	CHudTexture						*iconSmall;

// TF2 specific
	bool					bShowUsageHint;							// if true, then when you receive the weapon, show a hint about it

// SERVER DLL
	float m_flWeaponFOV;		//Tony; added weapon fov, SDK uses models from a couple different games, so FOV is different.
};

// The weapon parse function
bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey );

bool ReadModWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey );
bool ReadMapWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle );

// If weapon info has been loaded for the specified class name, this returns it.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name );

FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle );
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void );

void PrecacheModFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey, const char *filename );
void PrecacheMapFileWeaponInfoDatabase( IFileSystem *filesystem, const char *filename );

// Each game implements this. It can return a derived class and override Parse() if it wants.
extern FileWeaponInfo_t* CreateWeaponInfo();


#endif // WEAPON_PARSE_H
