//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "datacache/imdlcache.h"
#include "activitylist.h"
#include "playeranimstate.h"
#include "collisionproperty.h"

#ifdef CLIENT_DLL
	#include "prediction.h"
#endif

#if !defined( CLIENT_DLL )

// Game DLL Headers
#include "soundent.h"
#include "eventqueue.h"
#include "fmtstr.h"
#include "gameweaponmanager.h"

#endif

#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The minimum time a hud hint for a weapon should be on screen. If we switch away before
// this, then teh hud hint counter will be deremented so the hint will be shown again, as
// if it had never been seen. The total display time for a hud hint is specified in client
// script HudAnimations.txt (which I can't read here). 
#define MIN_HUDHINT_DISPLAY_TIME 7.0f

#define HIDEWEAPON_THINK_CONTEXT			"BaseCombatWeapon_HideThink"

extern bool UTIL_ItemCanBeTouchedByPlayer( CSharedBaseEntity *pItem, CSharedBasePlayer *pPlayer );

#ifdef _DEBUG
ConVar weapon_criticals_force_random( "weapon_criticals_force_random", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif // _DEBUG
ConVar weapon_criticals_bucket_cap( "weapon_criticals_bucket_cap", "1000.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar weapon_criticals_bucket_bottom( "weapon_criticals_bucket_bottom", "-250.0", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar weapon_criticals_bucket_default( "weapon_criticals_bucket_default", "300.0", FCVAR_REPLICATED | FCVAR_CHEAT );

#ifdef CLIENT_DLL
	#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

CSharedBaseCombatWeapon::CBaseCombatWeapon()
{
#ifdef CLIENT_DLL
	SetPredictionEligible(true);
#endif

	// Constructor must call this
	// CONSTRUCT_PREDICTABLE( CBaseCombatWeapon );

	// Some default values.  There should be set in the particular weapon classes
	m_fMinRange1		= 65;
	m_fMinRange2		= 65;
	m_fMaxRange1		= 1024;
	m_fMaxRange2		= 1024;

	m_bReloadsSingly	= false;

	// Defaults to zero
	m_nViewModelIndex	= 0;

	m_bFlipViewModel	= false;

#if defined( CLIENT_DLL )
	m_iState = m_iOldState = WEAPON_NOT_CARRIED;
	m_iClip1 = -1;
	m_iClip2 = -1;
	m_iPrimaryAmmoType = -1;
	m_iSecondaryAmmoType = -1;
#endif

#if !defined( CLIENT_DLL )
	m_pConstraint = NULL;
	OnBaseCombatWeaponCreated( this );
#endif

	m_hWeaponFileInfo = GetInvalidWeaponInfoHandle();

	UseClientSideAnimation();

	m_flCritTokenBucket = weapon_criticals_bucket_default.GetFloat();
	m_nCritChecks = 1;
	m_nCritSeedRequests = 0;

#ifdef GAME_DLL
	AddSolidFlags(FSOLID_TRIGGER);

	m_flNextResetCheckTime = 0.0f;
#endif

	m_bHolstered = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon::~CBaseCombatWeapon( void )
{
#if !defined( CLIENT_DLL )
	//Remove our constraint, if we have one
	if ( m_pConstraint != NULL )
	{
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
	OnBaseCombatWeaponDestroyed( this );
#endif
}

#ifdef CLIENT_DLL
	#undef CBaseCombatWeapon
#endif

void CSharedBaseCombatWeapon::Activate( void )
{
	BaseClass::Activate();

#ifndef CLIENT_DLL
	if ( GetOwnerEntity() )
		return;

	if ( GameRules()->IsAllowedToSpawn( this ) == false )
	{
		UTIL_Remove( this );
		return;
	}
#endif

}
void CSharedBaseCombatWeapon::GiveDefaultAmmo( void )
{
	// If I use clips, set my clips to the default
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 = AutoFiresFullClip() ? 0 : GetDefaultClip1();
	}
	else
	{
		SetPrimaryAmmoCount( GetDefaultClip1() );
		m_iClip1 = WEAPON_NOCLIP;
	}
	if ( UsesClipsForAmmo2() )
	{
		m_iClip2 = GetDefaultClip2();
	}
	else
	{
		SetSecondaryAmmoCount( GetDefaultClip2() );
		m_iClip2 = WEAPON_NOCLIP;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set mode to world model and start falling to the ground
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Spawn( void )
{
	bool bPrecacheAllowed = CSharedBaseEntity::IsPrecacheAllowed();
	if (!bPrecacheAllowed)
	{
		tmEnter( TELEMETRY_LEVEL1, TMZF_NONE, "LateWeaponPrecache" );
	}

	Precache();

	if (!bPrecacheAllowed)
	{
		tmLeave( TELEMETRY_LEVEL1 );
	}


	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	m_flNextEmptySoundTime = 0.0f;

	// Weapons won't show up in trace calls if they are being carried...
	RemoveEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	m_iState = WEAPON_NOT_CARRIED;
	SetGlobalFadeScale( 0.0f );

	// Assume 
	m_nViewModelIndex = 0;

	// Don't reset to default ammo if we're supposed to use the keyvalue
	if (!HasSpawnFlags( SF_WEAPON_PRESERVE_AMMO ))
		GiveDefaultAmmo();

	if ( GetWorldModel() )
	{
		SetModel( (GetDroppedModel() && GetDroppedModel()[0]) ? GetDroppedModel() : GetWorldModel() );
	}

#if !defined( CLIENT_DLL )
	if ( GetWpnData().szAIAddOn[ 0 ] != '\0' )
	{
		SetAIAddOn( AllocPooledString( GetWpnData().szAIAddOn ) );
	}

	FallInit();
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetBlocksLOS( false );

	// Default to non-removeable, because we don't want the
	// game_weapon_manager entity to remove weapons that have
	// been hand-placed by level designers. We only want to remove
	// weapons that have been dropped by NPC's.
	SetRemoveable( false );
#endif

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 36 );

	// Use more efficient bbox culling on the client. Otherwise, it'll setup bones for most
	// characters even when they're not in the frustum.
	AddEffects( EF_BONEMERGE_FASTCULL );

	m_iReloadHudHintCount = 0;
	m_iAltFireHudHintCount = 0;
	m_flHudHintMinDisplayTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: get this game's encryption key for decoding weapon kv files
// Output : virtual const unsigned char
//-----------------------------------------------------------------------------
const unsigned char *CSharedBaseCombatWeapon::GetEncryptionKey( void ) 
{ 
	return GameRules()->GetEncryptionKey(); 
}

const char*	CSharedBaseCombatWeapon::GetWeaponScriptName()
{
#ifdef CLIENT_DLL
	if(!HasClassname())
		return "weapon_default";
#endif

	return GetClassname();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Precache( void )
{
#if defined( CLIENT_DLL )
	Assert( Q_strlen( GetWeaponScriptName() ) > 0 );
	// Msg( "Client got %s\n", GetClassname() );
#endif
	m_iPrimaryAmmoType = m_iSecondaryAmmoType = -1;

	// Add this weapon to the weapon registry, and get our index into it
	// Get weapon data from script file
	if ( ReadWeaponDataFromFileForSlot( g_pFullFileSystem, GetWeaponScriptName(), &m_hWeaponFileInfo, GetEncryptionKey() ) )
	{
		// Get the ammo indexes for the ammo's specified in the data file
		if ( GetWpnData().szAmmo1[0] )
		{
			m_iPrimaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo1 );
			if (m_iPrimaryAmmoType == -1)
			{
				Log_Error(LOG_WEAPONPARSE,"ERROR: Weapon (%s) using undefined primary ammo type (%s)\n",GetClassname(), GetWpnData().szAmmo1);
			}
 		}
		if ( GetWpnData().szAmmo2[0] )
		{
			m_iSecondaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo2 );
			if (m_iSecondaryAmmoType == -1)
			{
				Log_Error(LOG_WEAPONPARSE,"ERROR: Weapon (%s) using undefined secondary ammo type (%s)\n",GetClassname(),GetWpnData().szAmmo2);
			}

		}
		// Precache models (preload to avoid hitch)
		m_iViewModelIndex = 0;
		m_iWorldModelIndex = 0;
		if ( GetViewModel() && GetViewModel()[0] )
		{
			m_iViewModelIndex = CSharedBaseEntity::PrecacheModel( GetViewModel() );
		}
		if ( GetWorldModel() && GetWorldModel()[0] )
		{
			m_iWorldModelIndex = CSharedBaseEntity::PrecacheModel( GetWorldModel() );
		}

		m_iDroppedModelIndex = 0;
		if ( GetDroppedModel() && GetDroppedModel()[0] )
		{
			m_iDroppedModelIndex = CSharedBaseEntity::PrecacheModel( GetDroppedModel() );
		}
		else
		{
			// Use the world model index
			m_iDroppedModelIndex = m_iWorldModelIndex;
		}

		// Precache sounds, too
		for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i )
		{
			const char *shootsound = GetShootSound( i );
			if ( shootsound && shootsound[0] )
			{
				CSharedBaseEntity::PrecacheScriptSound( shootsound );
			}
		}
	}
	else
	{
		// Couldn't read data file, remove myself
		Log_Error(LOG_WEAPONPARSE, "Error reading weapon data file for: %s\n", GetWeaponScriptName() );
	//	Remove( );	//don't remove, this gets released soon!
	}

	const char *pszTracerName = GetTracerType();
	if ( pszTracerName )
	{
		PrecacheEffect( pszTracerName );
	}

	PrecacheEffect( "ParticleTracer" );
	PrecacheParticleSystem( "weapon_tracers" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets ammo based on mapper value
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetAmmoFromMapper( float flAmmo, bool bSecondary )
{
	int iFinalAmmo;
	if (flAmmo > 0.0f && flAmmo < 1.0f)
	{
		// Ratio from max ammo
		iFinalAmmo = ((float)(!bSecondary ? GetMaxClip1() : GetMaxClip2()) * flAmmo);
	}
	else
	{
		// Actual ammo value
		iFinalAmmo = (int)flAmmo;
	}

	if(!bSecondary)
		m_iClip1 = iFinalAmmo;
	else
		m_iClip2 = iFinalAmmo;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "SetAmmo1") )
	{
		SetAmmoFromMapper(atof(szValue));
	}
	if ( FStrEq(szKeyName, "SetAmmo2") )
	{
		SetAmmoFromMapper(atof(szValue), true);
	}
	else if ( FStrEq(szKeyName, "spawnflags") )
	{
		SetSpawnFlags( atoi(szValue) );
#ifndef CLIENT_DLL
		// Some spawnflags have to be on the client right now
		if (GetSpawnFlags() != 0)
			DispatchUpdateTransmitState();
#endif
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	return BaseClass::GetKeyValue(szKeyName, szValue, iMaxLen);
}

//-----------------------------------------------------------------------------
// Purpose: Get my data in the file weapon info array
//-----------------------------------------------------------------------------
const FileWeaponInfo_t &CSharedBaseCombatWeapon::GetWpnData( void ) const
{
	return *GetFileWeaponInfoFromHandle( m_hWeaponFileInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetViewModel( int /*viewmodelindex = 0 -- this is ignored in the base class here*/ ) const
{
	return GetWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetWorldModel( void ) const
{
	return GetWpnData().szWorldModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetAnimPrefix( void ) const
{
	return GetWpnData().szAnimationPrefix;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetPrintName( void ) const
{
	return GetWpnData().szPrintName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetMaxClip1( void ) const
{
	return GetWpnData().iMaxClip1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetMaxClip2( void ) const
{
	return GetWpnData().iMaxClip2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetDefaultClip1( void ) const
{
	return GetWpnData().iDefaultClip1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetDefaultClip2( void ) const
{
	return GetWpnData().iDefaultClip2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::UsesClipsForAmmo1( void ) const
{
	return ( GetMaxClip1() != WEAPON_NOCLIP );
}

bool CSharedBaseCombatWeapon::IsMeleeWeapon() const
{
	return GetWpnData().m_bMeleeWeapon;
}

float CSharedBaseCombatWeapon::GetViewmodelFOVOverride() const
{
	return GetWpnData().m_flViewmodelFOV;
}

float CSharedBaseCombatWeapon::GetBobScale() const
{
	return GetWpnData().m_flBobScale;
}

float CSharedBaseCombatWeapon::GetSwayScale() const
{
	return GetWpnData().m_flSwayScale;
}

float CSharedBaseCombatWeapon::GetSwaySpeedScale() const
{
	return GetWpnData().m_flSwaySpeedScale;
}

const char *CSharedBaseCombatWeapon::GetDroppedModel() const
{
	return GetWpnData().szDroppedModel;
}

bool CSharedBaseCombatWeapon::UsesHands() const
{
	return GetWpnData().m_bUsesHands;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::UsesClipsForAmmo2( void ) const
{
	return ( GetMaxClip2() != WEAPON_NOCLIP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetWeight( void ) const
{
	return GetWpnData().iWeight;
}

//-----------------------------------------------------------------------------
// Purpose: Whether this weapon can be autoswitched to when the player runs out
//			of ammo in their current weapon or they pick this weapon up.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::AllowsAutoSwitchTo( void ) const
{
	return GetWpnData().bAutoSwitchTo;
}

//-----------------------------------------------------------------------------
// Purpose: Whether this weapon can be autoswitched away from when the player
//			runs out of ammo in this weapon or picks up another weapon or ammo.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::AllowsAutoSwitchFrom( void ) const
{
	return GetWpnData().bAutoSwitchFrom;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetWeaponFlags( void ) const
{
	return GetWpnData().iFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetSlot( void ) const
{
	return GetWpnData().iSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetPosition( void ) const
{
	return GetWpnData().iPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetName( void ) const
{
	return GetWpnData().szClassName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteActive( void ) const
{
	return GetWpnData().iconActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteInactive( void ) const
{
	return GetWpnData().iconInactive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteAmmo( void ) const
{
	return GetWpnData().iconAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteAmmo2( void ) const
{
	return GetWpnData().iconAmmo2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteCrosshair( void ) const
{
	return GetWpnData().iconCrosshair;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteAutoaim( void ) const
{
	return GetWpnData().iconAutoaim;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteZoomedCrosshair( void ) const
{
	return GetWpnData().iconZoomedCrosshair;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CSharedBaseCombatWeapon::GetSpriteZoomedAutoaim( void ) const
{
	return GetWpnData().iconZoomedAutoaim;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetShootSound( int iIndex ) const
{
	return GetWpnData().aShootSounds[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetRumbleEffect() const
{
	return GetWpnData().iRumbleEffect;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBaseCombatCharacter	*CSharedBaseCombatWeapon::GetOwner() const
{
	return ToBaseCombatCharacter( m_hOwner.Get() );
}	

CSharedBasePlayer	*CSharedBaseCombatWeapon::GetPlayerOwner() const
{
	return ToBasePlayer( m_hOwner.Get() );
}	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : BaseCombatCharacter - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetOwner( CSharedBaseCombatCharacter *owner )
{
	if ( !owner )
	{ 
#ifndef CLIENT_DLL
		// Make sure the weapon updates its state when it's removed from the player
		// We have to force an active state change, because it's being dropped and won't call UpdateClientData()
		int iOldState = m_iState;
		m_iState = WEAPON_NOT_CARRIED;
		OnActiveStateChanged( iOldState );
#endif

		// make sure we clear out our HideThink if we have one pending
		SetContextThink( NULL, 0, HIDEWEAPON_THINK_CONTEXT );
	}

	m_hOwner = owner;
	
#ifndef CLIENT_DLL
	DispatchUpdateTransmitState();
#else
	UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Return false if this weapon won't let the player switch away from it
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::IsAllowedToSwitch( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon can be selected via the weapon selection
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::CanBeSelected( void )
{
	if ( !VisibleInWeaponSelection() )
		return false;

	return HasAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::HasAmmo( void )
{
	// Weapons with no ammo types can always be selected
	if ( m_iPrimaryAmmoType == -1 && m_iSecondaryAmmoType == -1  )
		return true;
	if ( GetWeaponFlags() & ITEM_FLAG_SELECTONEMPTY )
		return true;

	CSharedBasePlayer *player = ToBasePlayer( GetOwner() );
	if ( !player )
		return false;
	return ( m_iClip1 > 0 || player->GetAmmoCount( m_iPrimaryAmmoType ) || m_iClip2 > 0 || player->GetAmmoCount( m_iSecondaryAmmoType ) );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon should be seen, and hence be selectable, in the weapon selection
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::VisibleInWeaponSelection( void )
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::HasWeaponIdleTimeElapsed( void )
{
	if ( gpGlobals->curtime > m_flTimeWeaponIdle )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetWeaponIdleTime( float time )
{
	m_flTimeWeaponIdle = time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CSharedBaseCombatWeapon::GetWeaponIdleTime( void )
{
	return m_flTimeWeaponIdle;
}

//-----------------------------------------------------------------------------
// Purpose: Drop/throw the weapon with the given velocity.
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Drop( const Vector &vecVelocity )
{
#if !defined( CLIENT_DLL )

	// Once somebody drops a gun, it's fair game for removal when/if
	// a game_weapon_manager does a cleanup on surplus weapons in the
	// world.
	SetRemoveable( true );
	WeaponManager_AmmoMod( this );

	//If it was dropped then there's no need to respawn it.
	AddSpawnFlags( SF_NORESPAWN );

	StopAnimation();
	StopFollowingEntity( );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	// clear follow stuff, setup for collision
	SetGravity(1.0);
	m_iState = WEAPON_NOT_CARRIED;
	RemoveEffects( EF_NODRAW );
	FallInit();
	SetGroundEntity( NULL );
	SetThink( &CSharedBaseCombatWeapon::SetPickupTouch );
	SetTouch(NULL);

	RemoveSpawnFlags( SF_WEAPON_NO_PLAYER_PICKUP );

	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj != NULL )
	{
		AngularImpulse	angImp( 200, 200, 200 );
		pObj->AddVelocity( &vecVelocity, &angImp );
	}
	else
	{
		SetAbsVelocity( vecVelocity );
	}

	CBaseEntity *pOwner = GetOwnerEntity();

	SetNextThink( gpGlobals->curtime + 1.0f );
	SetOwnerEntity( NULL );
	SetOwner( NULL );

	m_bInReload = false;

	m_OnDropped.FireOutput(pOwner, this);

	// If we're not allowing to spawn due to the gamerules,
	// remove myself when I'm dropped by an NPC.
	if ( pOwner && pOwner->IsNPC() )
	{
		if ( GameRules()->IsAllowedToSpawn( this ) == false )
		{
			UTIL_Remove( this );
			return;
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::OnPickedUp( CSharedBaseCombatCharacter *pNewOwner )
{
#if !defined( CLIENT_DLL )
	RemoveEffects( EF_ITEM_BLINK );

	if( pNewOwner->IsPlayer() )
	{
		m_OnPlayerPickup.FireOutput(pNewOwner, this);

		// Play the pickup sound for 1st-person observers
		CRecipientFilter filter;
		for ( int i=1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *player = UTIL_PlayerByIndex(i);
			if ( player && !player->IsAlive() && player->GetObserverMode() == OBS_MODE_IN_EYE )
			{
				filter.AddRecipient( player );
			}
		}
		if ( filter.GetRecipientCount() )
		{
			CBaseEntity::EmitSound( filter, pNewOwner->entindex(), "Player.PickupWeapon" );
		}

		// Robin: We don't want to delete weapons the player has picked up, so 
		// clear the name of the weapon. This prevents wildcards that are meant 
		// to find NPCs finding weapons dropped by the NPCs as well.
		// Level designers might want some weapons to preserve their original names, however.
		if ( !HasSpawnFlags(SF_WEAPON_PRESERVE_NAME) )
			SetName( NULL_STRING );
	}
	else
	{
		m_OnNPCPickup.FireOutput(pNewOwner, this);
	}

#ifdef HL2MP
	HL2MPRules()->RemoveLevelDesignerPlacedObject( this );
#endif

	// Someone picked me up, so make it so that I can't be removed.
	SetRemoveable( false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecTracerSrc - 
//			&tr - 
//			iTracerType - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CSharedBaseEntity *pOwner = GetOwner();

	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	const char *pszTracerName = GetTracerType();
	int iParticleID = 0;
	if ( !pszTracerName )
	{
		 pszTracerName = "ParticleTracer";
		 iParticleID = GetParticleSystemIndex( "weapon_tracers" );
	}

	Vector vNewSrc = vecTracerSrc;
	int iEntIndex = entindex();

#ifdef CLIENT_DLL
	C_BasePlayer *player = ToBasePlayer( pOwner );
	if ( C_BasePlayer::IsLocalPlayer( player ) )
	{
		CSharedBaseEntity *vm = player->GetViewModel();
		if ( vm )
		{
			iEntIndex = vm->entindex();
		}
	}
#endif

	int iAttachment = GetTracerAttachment();

	UTIL_Tracer( vNewSrc, tr.endpos, iEntIndex, iAttachment, 0.0f, true, pszTracerName, iParticleID );
}

void CSharedBaseCombatWeapon::GiveTo( CSharedBaseEntity *pOther, bool bDeploy )
{
#if !defined( CLIENT_DLL )
	// Can't pick up dissolving weapons
	if ( IsDissolving() )
		return;

	// if it's not a player, ignore
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

	if( UTIL_ItemCanBeTouchedByPlayer(this, pPlayer) )
	{
		// This makes sure the player could potentially take the object
		// before firing the cache interaction output. That doesn't mean
		// the player WILL end up taking the object, but cache interactions
		// are fired as soon as you prove you have found the object, not
		// when you finally acquire it.
		m_OnCacheInteraction.FireOutput( pOther, this );
	}

	if( HasSpawnFlags(SF_WEAPON_NO_PLAYER_PICKUP) )
		return;

	if (pPlayer->BumpWeapon(this, bDeploy))
	{
		OnPickedUp( pPlayer );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Default Touch function for player picking up a weapon (not AI)
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::DefaultTouch( CSharedBaseEntity *pOther )
{
	GiveTo( pOther );
}

//---------------------------------------------------------
// It's OK for base classes to override this completely 
// without calling up. (sjb)
//---------------------------------------------------------
bool CSharedBaseCombatWeapon::ShouldDisplayAltFireHUDHint()
{
	if( m_iAltFireHudHintCount >= WEAPON_RELOAD_HUD_HINT_COUNT )
		return false;

	if( UsesSecondaryAmmo() && HasSecondaryAmmo() )
	{
		return true;
	}

	if( !UsesSecondaryAmmo() && HasPrimaryAmmo() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::DisplayAltFireHudHint()
{
#if !defined( CLIENT_DLL )
	CFmtStr hint;
	hint.sprintf( "#valve_hint_alt_%s", GetClassname() );
	UTIL_HudHintText( GetOwner(), hint.Access() );
	m_iAltFireHudHintCount++;
	m_bAltFireHudHintDisplayed = true;
	m_flHudHintMinDisplayTime = gpGlobals->curtime + MIN_HUDHINT_DISPLAY_TIME;
#endif//CLIENT_DLL
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::RescindAltFireHudHint()
{
#if !defined( CLIENT_DLL )
	Assert(m_bAltFireHudHintDisplayed);
	
	UTIL_HudHintText( GetOwner(), "" );
	--m_iAltFireHudHintCount;
	m_bAltFireHudHintDisplayed = false;
#endif//CLIENT_DLL
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::ShouldDisplayReloadHUDHint()
{
	if( m_iReloadHudHintCount >= WEAPON_RELOAD_HUD_HINT_COUNT )
		return false;

	CSharedBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner != NULL && pOwner->IsPlayer() && UsesClipsForAmmo1() && m_iClip1 < (GetMaxClip1() / 2) )
	{
		// I'm owned by a player, I use clips, I have less then half a clip loaded. Now, does the player have more ammo?
		if ( pOwner )
		{
			if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) 
				return true;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::DisplayReloadHudHint()
{
#if !defined( CLIENT_DLL )
	UTIL_HudHintText( GetOwner(), "valve_hint_reload" );
	m_iReloadHudHintCount++;
	m_bReloadHudHintDisplayed = true;
	m_flHudHintMinDisplayTime = gpGlobals->curtime + MIN_HUDHINT_DISPLAY_TIME;
#endif//CLIENT_DLL
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::RescindReloadHudHint()
{
#if !defined( CLIENT_DLL )
	Assert(m_bReloadHudHintDisplayed);

	UTIL_HudHintText( GetOwner(), "" );
	--m_iReloadHudHintCount;
	m_bReloadHudHintDisplayed = false;
#endif//CLIENT_DLL
}


void CSharedBaseCombatWeapon::SetPickupTouch( void )
{
#if !defined( CLIENT_DLL )
	SetTouch(&CSharedBaseCombatWeapon::DefaultTouch);

	if ( GetSpawnFlags() & SF_NORESPAWN )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 30.0f );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WeaponClass_t CSharedBaseCombatWeapon::WeaponClassify()
{
	return WEPCLASS_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WeaponClass_t CSharedBaseCombatWeapon::WeaponClassFromString(const char *str)
{
	if (FStrEq(str, "WEPCLASS_HANDGUN"))
		return WEPCLASS_HANDGUN;
	else if (FStrEq(str, "WEPCLASS_RIFLE"))
		return WEPCLASS_RIFLE;
	else if (FStrEq(str, "WEPCLASS_SHOTGUN"))
		return WEPCLASS_SHOTGUN;
	else if (FStrEq(str, "WEPCLASS_HEAY"))
		return WEPCLASS_HEAVY;

	else if (FStrEq(str, "WEPCLASS_MELEE"))
		return WEPCLASS_MELEE;

	return WEPCLASS_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::SupportsBackupActivity(Activity activity)
{
	// Derived classes should override this.

	return true;
}

acttable_t *CSharedBaseCombatWeapon::GetBackupActivityList()
{
	return NULL;
}

int CSharedBaseCombatWeapon::GetBackupActivityListCount()
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
acttable_t *CSharedBaseCombatWeapon::GetDefaultBackupActivityList( acttable_t *pTable, int &actCount )
{

	actCount = 0;
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Become a child of the owner (MOVETYPE_FOLLOW)
//			disables collisions, touch functions, thinking
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Equip( CSharedBaseCombatCharacter *pOwner )
{
	// Attach the weapon to an owner
	SetAbsVelocity( vec3_origin );
	RemoveSolidFlags( FSOLID_TRIGGER );
	FollowEntity( pOwner );
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	// Break any constraint I might have to the world.
	RemoveEffects( EF_ITEM_BLINK );

#if !defined( CLIENT_DLL )
	if ( m_pConstraint != NULL )
	{
		RemoveSpawnFlags( SF_WEAPON_START_CONSTRAINED );
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
#endif

	// Ammo may be overridden to 0, in which case we shouldn't autoswitch
	if (m_iClip1 <= 0 && m_iClip2 <= 0)
		AddSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY);

	m_flNextPrimaryAttack		= gpGlobals->curtime;
	m_flNextSecondaryAttack		= gpGlobals->curtime;
	SetTouch( NULL );
	SetThink( NULL );
#if !defined( CLIENT_DLL )
	VPhysicsDestroyObject();
#endif

	if ( pOwner->IsPlayer() )
	{
		SetModel( GetViewModel() );
	}
	else
	{
		// Make the weapon ready as soon as any NPC picks it up.
		m_flNextPrimaryAttack = gpGlobals->curtime;
		m_flNextSecondaryAttack = gpGlobals->curtime;
		SetModel( GetWorldModel() );
	}
}

void CSharedBaseCombatWeapon::SetActivity( Activity act, float duration ) 
{ 
	//Adrian: Oh man...
#if !defined( CLIENT_DLL )
	SetModel( GetWorldModel() );
#endif
	
	int sequence = SelectWeightedSequence( act ); 
	
	// FORCE IDLE on sequences we don't have (which should be many)
	if ( sequence == ACTIVITY_NOT_AVAILABLE )
		sequence = SelectWeightedSequence( ACT_VM_IDLE );

	//Adrian: Oh man again...
#if !defined( CLIENT_DLL )
	if ( GetOwner()->IsPlayer() ) 
		SetModel( GetViewModel() );
#endif

	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		SetSequence( sequence );
		SetActivity( act ); 
		SetCycle( 0 );
		ResetSequenceInfo( );

		if ( duration > 0 )
		{
			// FIXME: does this even make sense in non-shoot animations?
			float playbackrate = SequenceDuration( sequence ) / duration;
			playbackrate = MIN( playbackrate, 12.f );  // FIXME; magic number!, network encoding range
			SetPlaybackRate(playbackrate);
		}
		else
		{
			SetPlaybackRate(1.0);
		}
	}
}

//====================================================================================
// WEAPON CLIENT HANDLING
//====================================================================================
int CSharedBaseCombatWeapon::UpdateClientData( CSharedBasePlayer *pPlayer )
{
	int iNewState = WEAPON_IS_CARRIED_BY_PLAYER;

	if ( pPlayer->GetActiveWeapon() == this )
	{
		if ( pPlayer->m_fOnTarget ) 
		{
			iNewState = WEAPON_IS_ONTARGET;
		}
		else
		{
			iNewState = WEAPON_IS_ACTIVE;
		}
	}
	else
	{
		iNewState = WEAPON_IS_CARRIED_BY_PLAYER;
	}

	if ( m_iState != iNewState )
	{
		int iOldState = m_iState;
		m_iState = iNewState;
		OnActiveStateChanged( iOldState );
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetViewModelIndex( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	m_nViewModelIndex = index;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iActivity - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SendViewModelAnim( int nSequence )
{
#if defined( CLIENT_DLL )
	if ( !IsPredicted() )
		return;
#endif
	
	if ( nSequence < 0 )
		return;

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;
	
	CSharedBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex, false );
	
	if ( vm == NULL )
		return;

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SendViewModelMatchingSequence( nSequence );
}

float CSharedBaseCombatWeapon::GetViewModelSequenceDuration()
{
	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	{
		Assert( false );
		return 0;
	}
	
	CSharedBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return 0;
	}

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	return vm->SequenceDuration();
}

bool CSharedBaseCombatWeapon::IsViewModelSequenceFinished( void ) const
{
	// These are not valid activities and always complete immediately
	if ( GetActivity() == ACT_RESET || GetActivity() == ACT_INVALID )
		return true;

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	{
		Assert( false );
		return false;
	}
	
	CSharedBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return false;
	}

	return vm->IsSequenceFinished();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetViewModel()
{
	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;
	CSharedBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex, false );
	if ( vm == NULL )
		return;
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SetWeaponModel( GetViewModel( m_nViewModelIndex ), this );
}

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::SendWeaponAnim( Activity iActivity )
{
	//iActivity = TranslateViewmodelHandActivity( (Activity)iActivity );

	//For now, just set the ideal activity and be done with it
	return SetIdealActivity( (Activity) iActivity );
}

//====================================================================================
// WEAPON SELECTION
//====================================================================================

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::HasAnyAmmo( void )
{
	// If I don't use ammo of any kind, I can always fire
	if ( !UsesPrimaryAmmo() && !UsesSecondaryAmmo() )
		return true;

	// Otherwise, I need ammo of either type
	return ( HasPrimaryAmmo() || HasSecondaryAmmo() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::HasPrimaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo1() )
	{
		if ( m_iClip1 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	CSharedBaseCombatCharacter		*pOwner = GetOwner();
	if ( pOwner )
	{
		if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 ) 
			return true;
	}
	else
	{
		// No owner, so return how much primary ammo I have along with me.
		if( GetPrimaryAmmoCount() > 0 )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::HasSecondaryAmmo( void )
{
	// If I use a clip, and have some ammo in it, then I have ammo
	if ( UsesClipsForAmmo2() )
	{
		if ( m_iClip2 > 0 )
			return true;
	}

	// Otherwise, I have ammo if I have some in my ammo counts
	CSharedBaseCombatCharacter		*pOwner = GetOwner();
	if ( pOwner )
	{
		if ( pOwner->GetAmmoCount( m_iSecondaryAmmoType ) > 0 ) 
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the weapon actually uses primary ammo
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::UsesPrimaryAmmo( void )
{
	if ( m_iPrimaryAmmoType < 0 )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the weapon actually uses secondary ammo
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::UsesSecondaryAmmo( void )
{
	if ( m_iSecondaryAmmoType < 0 )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::SetWeaponVisible( bool visible )
{
	CSharedBaseViewModel *vm = NULL;

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
		if ( vm )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( vm )
		{
			vm->AddEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::IsWeaponVisible( void )
{
	CSharedBaseViewModel *vm = NULL;
	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
		if ( vm )
		{
#ifdef CLIENT_DLL
			return !vm->IsDormant() && !vm->IsEffectActive(EF_NODRAW);
#else
			return ( !vm->IsEffectActive(EF_NODRAW) );
#endif
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: If the current weapon has more ammo, reload it. Otherwise, switch 
//			to the next best weapon we've got. Returns true if it took any action.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::ReloadOrSwitchWeapons( void )
{
	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// weapon isn't useable, switch.
		// Ammo might be overridden to 0, in which case we shouldn't do this
		if ( ( (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false ) && !HasSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY) && ( GameRules()->SwitchToNextBestWeapon( pOwner, this ) ) )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( UsesClipsForAmmo1() && !AutoFiresFullClip() && 
			 (m_iClip1 == 0) && 
			 (GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) == false && 
			 m_flNextPrimaryAttack < gpGlobals->curtime && 
			 m_flNextSecondaryAttack < gpGlobals->curtime )
		{
			// if we're successfully reloading, we're done
			if ( Reload() )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szViewModel - 
//			*szWeaponModel - 
//			iActivity - 
//			*szAnimExt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::DefaultDeploy( const char *szViewModel, const char *szWeaponModel, Activity iActivity, const char *szAnimExt )
{
	// Msg( "deploy %s at %f\n", GetClassname(), gpGlobals->curtime );

	// Weapons that don't autoswitch away when they run out of ammo 
	// can still be deployed when they have no ammo.
	if ( !HasAnyAmmo() && AllowsAutoSwitchFrom() )
		return false;

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		// Dead men deploy no weapons
		if ( pOwner->IsAlive() == false )
			return false;

		pOwner->SetAnimationExtension( szAnimExt );

		SetViewModel();
		SendWeaponAnim( iActivity );

		pOwner->SetNextAttack( gpGlobals->curtime + SequenceDuration() );
	}

	// Can't shoot again until we've finished deploying
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
	m_flHudHintMinDisplayTime = 0;

	m_bAltFireHudHintDisplayed = false;
	m_bReloadHudHintDisplayed = false;
	m_flHudHintPollTime = gpGlobals->curtime + 5.0f;
	
	WeaponSound( DEPLOY );

	SetWeaponVisible( true );

	SetContextThink( NULL, 0, HIDEWEAPON_THINK_CONTEXT );

	m_bHolstered = false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::Deploy( )
{
	MDLCACHE_CRITICAL_SECTION();
	bool bResult = DefaultDeploy( GetViewModel(), GetWorldModel(), GetDrawActivity(), GetAnimPrefix() );

	// override pose parameters
	PoseParameterOverride( false );

	return bResult;
}

Activity CSharedBaseCombatWeapon::GetDrawActivity( void )
{
	return ACT_VM_DRAW;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::Holster( CSharedBaseCombatWeapon *pSwitchingTo, bool bInstant )
{
	MDLCACHE_CRITICAL_SECTION();

	// cancel any reload in progress.
	m_bInReload = false; 
	m_bFiringWholeClip = false;

	// kill any think functions
	SetThink(NULL);

	// Send holster animation
	if( !bInstant )
		SendWeaponAnim( ACT_VM_HOLSTER );

	// Some weapon's don't have holster anims yet, so detect that
	float flSequenceDuration = 0;
	if ( !bInstant && GetActivity() == ACT_VM_HOLSTER )
	{
		flSequenceDuration = SequenceDuration();
	}

	CSharedBaseCombatCharacter *pOwner = GetOwner();
	if( pOwner )
	{
		pOwner->SetNextAttack( gpGlobals->curtime + flSequenceDuration );
	}

	// If we don't have a holster anim, hide immediately to avoid timing issues
	if ( bInstant || !flSequenceDuration )
	{
		SetWeaponVisible( false );
	}
	else
	{
		// Hide the weapon when the holster animation's finished
		SetContextThink( &CSharedBaseCombatWeapon::HideThink, gpGlobals->curtime + flSequenceDuration, HIDEWEAPON_THINK_CONTEXT );
	}

	// if we were displaying a hud hint, squelch it.
	if (m_flHudHintMinDisplayTime && gpGlobals->curtime < m_flHudHintMinDisplayTime)
	{
		if( m_bAltFireHudHintDisplayed )
			RescindAltFireHudHint();

		if( m_bReloadHudHintDisplayed )
			RescindReloadHudHint();
	}

	if (HasSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY))
		RemoveSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY);

	// reset pose parameters
	PoseParameterOverride( true );

	m_bHolstered = true;

	return true;
}

#ifdef CLIENT_DLL

void CSharedBaseCombatWeapon::BoneMergeFastCullBloat( Vector &localMins, Vector &localMaxs, const Vector &thisEntityMins, const Vector &thisEntityMaxs ) const
{
	// The default behavior pushes it out by BONEMERGE_FASTCULL_BBOX_EXPAND in all directions, but we can do better
	// since we know the weapon will never point behind him.

	localMaxs.x += 20;	// Leaves some space in front for long weapons.
	
	localMins.y -= 20;	// Fatten it to his left and right since he can rotate that way.
	localMaxs.y += 20;	

	localMaxs.z += 15;	// Leave some space at the top.
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::InputHideWeapon( inputdata_t &inputdata )
{
	// Only hide if we're still the active weapon. If we're not the active weapon
	if ( GetOwner() && GetOwner()->GetActiveWeapon() == this )
	{
		SetWeaponVisible( false );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::HideThink( void )
{
	// Only hide if we're still the active weapon. If we're not the active weapon
	if ( GetOwner() && GetOwner()->GetActiveWeapon() == this )
	{
		SetWeaponVisible( false );
	}
}

bool CSharedBaseCombatWeapon::CanReload( void )
{
	if ( AutoFiresFullClip() && m_bFiringWholeClip )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Anti-hack
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::AddToCritBucket( float flAmount )
{
	float flCap = weapon_criticals_bucket_cap.GetFloat();

	// Regulate crit frequency to reduce client-side seed hacking
	if ( m_flCritTokenBucket < flCap )
	{
		// Treat raw damage as the resource by which we add or subtract from the bucket
		m_flCritTokenBucket += flAmount;
		m_flCritTokenBucket = Min( m_flCritTokenBucket, flCap );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Anti-hack
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::IsAllowedToWithdrawFromCritBucket( float flDamage )
{
	// Note: If we're in this block of code, the assumption is that the
	// seed said we should grant a random crit.  If allowed, the cost
	// will be deducted here.

	// Track each seed request - in cases where a player is hacking, we'll 
	// see a silly ratio.
	m_nCritSeedRequests++;

	// Adjust token cost based on the ratio of requests vs granted, except
	// melee, which crits much more than ranged (as high as 60% chance)
	float flMult = ( IsMeleeWeapon() ) ? 0.5f : RemapValClamped( ( (float)m_nCritSeedRequests / (float)m_nCritChecks ), 0.1f, 1.f, 1.f, 3.f );

	// Would this take us below our limit?
	float flCost = ( flDamage * DAMAGE_CRIT_MULTIPLIER ) * flMult;
	if ( flCost > m_flCritTokenBucket )
		return false;

	// Withdraw
	RemoveFromCritBucket( flCost );

	float flBottom = weapon_criticals_bucket_bottom.GetFloat();
	if ( m_flCritTokenBucket < flBottom )
		m_flCritTokenBucket = flBottom;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::ItemPreFrame( void )
{
	MaintainIdealActivity();
}

bool CSharedBaseCombatWeapon::CanPerformSecondaryAttack() const
{
	return m_flNextSecondaryAttack <= gpGlobals->curtime;
}

//====================================================================================
// WEAPON BEHAVIOUR
//====================================================================================
void CSharedBaseCombatWeapon::ItemPostFrame( void )
{
	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	UpdateAutoFire();

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if ((pOwner->m_nButtons & IN_ATTACK2) && CanPerformSecondaryAttack() )
	{
		if (pOwner->HasSpawnFlags(SF_PLAYER_SUPPRESS_FIRING))
		{
			// Don't do anything, just cancel the whole function
			return;
		}
		else if (UsesSecondaryAmmo() && pOwner->GetAmmoCount(m_iSecondaryAmmoType)<=0 )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			bFired = ShouldBlockPrimaryFire();

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if ( UsesClipsForAmmo2() )
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	
	if ( !bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		if (pOwner->HasSpawnFlags( SF_PLAYER_SUPPRESS_FIRING ))
		{
			// Don't do anything, just cancel the whole function
			return;
		}
		// Clip empty? Or out of ammo on a no-clip weapon?
		else if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType)<=0 )) )
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ( ( pOwner->m_afButtonPressed & IN_ATTACK ) || ( pOwner->m_afButtonReleased & IN_ATTACK2 ) )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			if( PrimaryAttack() )
			{
				if ( AutoFiresFullClip() )
				{
					m_bFiringWholeClip = true;
				}

#ifdef CLIENT_DLL
				pOwner->SetFiredWeapon( true );
#endif
			}
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	//  Can only start the Reload Cycle after the firing cycle
	if ( ( pOwner->m_nButtons & IN_RELOAD ) && m_flNextPrimaryAttack <= gpGlobals->curtime && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (CanReload() && pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}

void CSharedBaseCombatWeapon::HandleFireOnEmpty()
{
	// If we're already firing on empty, reload if we can
	if ( m_bFireOnEmpty )
	{
		ReloadOrSwitchWeapons();
		m_fFireDuration = 0.0f;
	}
	else
	{
		if (m_flNextEmptySoundTime < gpGlobals->curtime)
		{
			WeaponSound(EMPTY);
			m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
		}
		m_bFireOnEmpty = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each frame by the player PostThink, if the player's not ready to attack yet
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::ItemBusyFrame( void )
{
	UpdateAutoFire();
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting bullet type
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CSharedBaseCombatWeapon::GetBulletType( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting spread
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector& CSharedBaseCombatWeapon::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CSharedBaseCombatWeapon::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t defaultWeaponProficiencyTable[] =
	{
		{ 1.0, 1.0	},
		{ 1.0, 1.0	},
		{ 1.0, 1.0	},
		{ 1.0, 1.0	},
		{ 1.0, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(defaultWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);
	return defaultWeaponProficiencyTable;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting firerate
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CSharedBaseCombatWeapon::GetFireRate( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for playing shoot sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;

	CSoundParameters params;
	
	if ( !GetParametersForSound( shootsound, params, NULL ) )
		return;

	if ( params.play_to_owner_only )
	{
		// Am I only to play to my owner?
		if ( GetOwner() && GetOwner()->IsPlayer() )
		{
			CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );
			if ( IsPredicted() && CSharedBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, NULL, soundtime );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
			if ( IsPredicted() && CSharedBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, &GetOwner()->GetAbsOrigin(), soundtime ); 

#if !defined( CLIENT_DLL )
			if( sound_type == EMPTY )
			{
				CSoundEnt::InsertSound( SOUND_COMBAT, GetOwner()->GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.2, GetOwner() );
			}
#endif
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			CPASAttenuationFilter filter( this, params.soundlevel );
			if ( IsPredicted() && CSharedBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, entindex(), shootsound, &GetAbsOrigin(), soundtime ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a sound played by this weapon.
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::StopWeaponSound( WeaponSound_t sound_type )
{
	//if ( IsPredicted() )
	//	return;

	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;
	
	CSoundParameters params;
	if ( !GetParametersForSound( shootsound, params, NULL ) )
		return;

	// Am I only to play to my owner?
	if ( params.play_to_owner_only )
	{
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			StopSound( entindex(), shootsound );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::DefaultReload( int iClipSize1, int iClipSize2, Activity iActivity )
{
	CSharedBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;

	// If I don't have any spare ammo, I can't reload
	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	bool bReload = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary	= MIN(iClipSize1 - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
		if ( primary != 0 )
		{
			bReload = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = MIN(iClipSize2 - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
		if ( secondary != 0 )
		{
			bReload = true;
		}
	}

	if ( !bReload )
		return false;

#ifdef CLIENT_DLL
	// Play reload
	WeaponSound( RELOAD );
#endif
	SendWeaponAnim( iActivity );
	if(pOwner->IsPlayer()) {
		((CSharedBasePlayer *)pOwner)->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
	}

	MDLCACHE_CRITICAL_SECTION();
	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pOwner->SetNextAttack( flSequenceEndTime );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;

	m_bInReload = true;

	return true;
}

bool CSharedBaseCombatWeapon::ReloadsSingly( void ) const
{
	return m_bReloadsSingly;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::Reload( void )
{
	return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Reload_NPC( bool bPlaySound )
{
	if (bPlaySound)
		WeaponSound( RELOAD_NPC );

	if (UsesClipsForAmmo1())
	{
		m_iClip1 = GetMaxClip1();
	}
	else
	{
		// For weapons which don't use clips, give the owner ammo.
		if (GetOwner())
			GetOwner()->SetAmmoCount( GetDefaultClip1(), m_iPrimaryAmmoType );
	}
}

//=========================================================
void CSharedBaseCombatWeapon::WeaponIdle( void )
{
	//Idle again if we've finished
	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}


//=========================================================
Activity CSharedBaseCombatWeapon::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//=========================================================
Activity CSharedBaseCombatWeapon::GetSecondaryAttackActivity( void )
{
	return ACT_VM_SECONDARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: Adds in view kick and weapon accuracy degradation effect
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::AddViewKick( void )
{
	//NOTENOTE: By default, weapon will not kick up (defined per weapon)
}

//-----------------------------------------------------------------------------
// Purpose: Get the string to print death notices with
//-----------------------------------------------------------------------------
const char *CSharedBaseCombatWeapon::GetDeathNoticeName( void )
{
#if !defined( CLIENT_DLL )
	return (char*)STRING( m_iszName );
#else
	return "GetDeathNoticeName not implemented on client yet";
#endif
}

//====================================================================================
// WEAPON RELOAD TYPES
//====================================================================================
void CSharedBaseCombatWeapon::CheckReload( void )
{
	if ( m_bReloadsSingly )
	{
		CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if ( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && m_iClip1 > 0 )
			{
				m_bInReload = false;
				return;
			}

			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if (m_iClip1 < GetMaxClip1())
			{
				// Add them to the clip
				m_iClip1 += 1;
				pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::FinishReload( void )
{
	CSharedBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner)
	{
		// If I use primary clips, reload primary
		if ( UsesClipsForAmmo1() )
		{
			int primary	= MIN( GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));	
			m_iClip1 += primary;
			pOwner->RemoveAmmo( primary, m_iPrimaryAmmoType);
		}

		// If I use secondary clips, reload secondary
		if ( UsesClipsForAmmo2() )
		{
			int secondary = MIN( GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
			m_iClip2 += secondary;
			pOwner->RemoveAmmo( secondary, m_iSecondaryAmmoType );
		}

		if ( m_bReloadsSingly )
		{
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Abort any reload we have in progress
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::AbortReload( void )
{
#ifdef CLIENT_DLL
	StopWeaponSound( RELOAD ); 
#endif
	m_bInReload = false;
}

void CSharedBaseCombatWeapon::UpdateAutoFire( void )
{
	if ( !AutoFiresFullClip() )
		return;

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( m_iClip1 == 0 )
	{
		// Ready to reload again
		m_bFiringWholeClip = false;
	}

	if ( m_bFiringWholeClip )
	{
		// If it's firing the clip don't let them repress attack to reload
		pOwner->m_nButtons &= ~IN_ATTACK;
	}

	// Don't use the regular reload key
	if ( pOwner->m_nButtons & IN_RELOAD )
	{
		pOwner->m_nButtons &= ~IN_RELOAD;
	}

	// Try to fire if there's ammo in the clip and we're not holding the button
	bool bReleaseClip = m_iClip1 > 0 && !( pOwner->m_nButtons & IN_ATTACK );

	if ( !bReleaseClip )
	{
		if ( CanReload() && ( pOwner->m_nButtons & IN_ATTACK ) )
		{
			// Convert the attack key into the reload key
			pOwner->m_nButtons |= IN_RELOAD;
		}

		// Don't allow attack button if we're not attacking
		pOwner->m_nButtons &= ~IN_ATTACK;
	}
	else
	{
		// Fake the attack key
		pOwner->m_nButtons |= IN_ATTACK;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Primary fire button attack
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CSharedBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return false;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = MIN( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetBulletSpread();
#endif // CLIENT_DLL

	pPlayer->FireBullets( info );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	//Add our view kick in
	AddViewKick();

	return true;
}

void CSharedBaseCombatWeapon::BaseForceFire( CSharedBaseCombatCharacter *pOperator, CSharedBaseEntity *pTarget )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	pOperator->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	//pOperator->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;

	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), info.m_vecSrc, angShootDir );

	if ( pTarget )
	{
		info.m_vecDirShooting = pTarget->WorldSpaceCenter() - info.m_vecSrc;
		VectorNormalize( info.m_vecDirShooting );
	}
	else
	{
		AngleVectors( angShootDir, &info.m_vecDirShooting );
	}

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = MIN( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN( info.m_iShots, pOperator->GetAmmoCount( m_iPrimaryAmmoType ) );
		pOperator->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pOperator->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetBulletSpread();
#endif // CLIENT_DLL

	pOperator->FireBullets( info );
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame to check if the weapon is going through transition animations
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::MaintainIdealActivity( void )
{
	// Must be transitioning
	if ( GetActivity() != ACT_TRANSITION )
		return;

	// Must not be at our ideal already 
	if ( ( GetActivity() == m_IdealActivity ) && ( GetSequence() == m_nIdealSequence ) )
		return;
	
	// Must be finished with the current animation
	if ( IsViewModelSequenceFinished() == false )
		return;

	// Move to the next animation towards our ideal
	SendWeaponAnim( m_IdealActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ideal activity for the weapon to be in, allowing for transitional animations inbetween
// Input  : ideal - activity to end up at, ideally
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::SetIdealActivity( Activity ideal )
{
	MDLCACHE_CRITICAL_SECTION();
	int	idealSequence = SelectWeightedSequence( ideal );

	if ( idealSequence == -1 )
		return false;

	//Take the new activity
	m_IdealActivity	 = ideal;
	m_nIdealSequence = idealSequence;

	//Find the next sequence in the potential chain of sequences leading to our ideal one
	int nextSequence = FindTransitionSequence( GetSequence(), m_nIdealSequence, NULL );

	// Don't use transitions when we're deploying
	if ( ideal != ACT_VM_DRAW && IsWeaponVisible() && nextSequence != m_nIdealSequence )
	{
		//Set our activity to the next transitional animation
		SetActivity( ACT_TRANSITION );
		SetSequence( nextSequence );	
		SendViewModelAnim( nextSequence );
	}
	else
	{
		//Set our activity to the ideal
		SetActivity( m_IdealActivity );
		SetSequence( m_nIdealSequence );	
		SendViewModelAnim( m_nIdealSequence );
	}

	//Set the next time the weapon will idle
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	return true;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = NULL;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}


//-----------------------------------------------------------------------------
// Locking a weapon is an exclusive action. If you lock a weapon, that means 
// you are preventing others from doing so for themselves.
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::Lock( float lockTime, CSharedBaseEntity *pLocker )
{
	m_flUnlockTime = gpGlobals->curtime + lockTime;
	m_hLocker.Set( pLocker );
}

//-----------------------------------------------------------------------------
// If I'm still locked for a period of time, tell everyone except the person
// that locked me that I'm not available. 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatWeapon::IsLocked( CSharedBaseEntity *pAsker )
{
	return ( m_flUnlockTime > gpGlobals->curtime && m_hLocker != pAsker );
}

bool CSharedBaseCombatWeapon::CanBePickedUpByNPCs(void)
{
	return GetWpnData().m_nWeaponRestriction != WPNRESTRICT_PLAYER_ONLY;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CSharedBaseCombatWeapon::ActivityOverride( Activity baseAct, bool *pRequired )
{
	int actCount = ActivityListCount();
	acttable_t *pTable = ActivityList();

	for ( int i = 0; i < actCount; i++ )
	{
		const acttable_t& act = pTable[i];
		if ( baseAct == act.baseAct )
		{
			if (pRequired)
			{
				*pRequired = act.required;
			}
			return (Activity)act.weaponAct;
		}
	}
	return baseAct;
}

void CSharedBaseCombatWeapon::Operator_FrameUpdate( CSharedBaseCombatCharacter *pOperator )
{
	StudioFrameAdvance( ); // animate

	if ( IsSequenceFinished() )
	{
		if ( SequenceLoops() )
		{
			// animation does loop, which means we're playing subtle idle. Might need to fidget.
			int iSequence = SelectWeightedSequence( GetActivity() );
			if ( iSequence != ACTIVITY_NOT_AVAILABLE )
			{
				ResetSequence( iSequence );	// Set to new anim (if it's there)
			}
		}
#if 0
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			SelectHeaviestSequence( GetActivity() );
		}
#endif
	}

#ifndef CLIENT_DLL
	// Animation events are passed back to the weapon's owner/operator
	DispatchAnimEvents( pOperator );
#endif // CLIENT_DLL

	CSharedBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	CSharedBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	// Update and dispatch the viewmodel events
	if ( vm != NULL )
	{
		vm->StudioFrameAdvance();
#ifndef CLIENT_DLL
		vm->DispatchAnimEvents( this );
#endif // CLIENT_DLL
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSharedBaseCombatWeapon::PoseParameterOverride( bool bReset )
{
	CSharedBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	CStudioHdr *pStudioHdr = pOwner->GetModelPtr();
	if ( !pStudioHdr )
		return;
	
	int iCount = PoseParamListCount();
	poseparamtable_t *pPoseParamList = PoseParamList();
	if ( pPoseParamList )
	{
		for ( int i=0; i<iCount; ++i )
		{
			int iPoseParam = pOwner->LookupPoseParameter( pStudioHdr, pPoseParamList[i].pszName );
		
			if ( iPoseParam != -1 )
				pOwner->SetPoseParameter( iPoseParam, bReset ? 0 : pPoseParamList[i].flValue );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDmgAccumulator::CDmgAccumulator( void )
{
#ifdef GAME_DLL
	SetDefLessFunc( m_TargetsDmgInfo );
#endif // GAME_DLL

	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDmgAccumulator::~CDmgAccumulator()
{
	// Did a weapon get deleted while aggregating CTakeDamageInfo events?
	Assert( !m_bActive );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Collect trace attacks for weapons that fire multiple bullets per attack that also penetrate
//-----------------------------------------------------------------------------
void CDmgAccumulator::AccumulateMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	Assert( m_bActive );

#if defined( GAME_DLL )
	int iIndex = m_TargetsDmgInfo.Find( pEntity->entindex() );
	if ( iIndex == m_TargetsDmgInfo.InvalidIndex() )
	{
		m_TargetsDmgInfo.Insert( pEntity->entindex(), info );
	}
	else
	{
		CTakeDamageInfo &entityInfo = m_TargetsDmgInfo[iIndex];

		// Update
		entityInfo.AddDamageType( info.GetDamageType() );
		entityInfo.SetDamage( entityInfo.GetDamage() + info.GetDamage() );
		entityInfo.SetDamageForce( entityInfo.GetDamageForce() + info.GetDamageForce() );
		entityInfo.SetDamagePosition( info.GetDamagePosition() );
		entityInfo.SetReportedPosition( info.GetReportedPosition() );
		entityInfo.SetMaxDamage( MAX( entityInfo.GetMaxDamage(), info.GetDamage() ) );
		entityInfo.SetAmmoType( info.GetAmmoType() );
	}
#endif	// GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Send aggregate info
//-----------------------------------------------------------------------------
void CDmgAccumulator::Process( void )
{
	FOR_EACH_MAP( m_TargetsDmgInfo, i )
	{
		CBaseEntity *pEntity = UTIL_EntityByIndex( m_TargetsDmgInfo.Key( i ) );
		if ( pEntity )
		{
			AddMultiDamage( m_TargetsDmgInfo[i], pEntity );
		}
	}

	m_bActive = false;
	m_TargetsDmgInfo.Purge();
}
#endif // GAME_DLL

#if defined( CLIENT_DLL )

BEGIN_PREDICTION_DATA( C_BaseCombatWeapon )

	DEFINE_FIELD_FLAGS( m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	// Networked
	DEFINE_FIELD_FLAGS( m_hOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( m_hWeaponFileInfo, FIELD_SHORT ),
	DEFINE_FIELD_FLAGS( m_iState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			 
	DEFINE_FIELD_FLAGS( m_iViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_FIELD_FLAGS( m_iWorldModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_FIELD_FLAGS_TOL( m_flNextPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_FIELD_FLAGS_TOL( m_flNextSecondaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_FIELD_FLAGS_TOL( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),

	DEFINE_FIELD_FLAGS( m_iPrimaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_iSecondaryAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_iClip1, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			
	DEFINE_FIELD_FLAGS( m_iClip2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			

	DEFINE_FIELD_FLAGS( m_nViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	// Not networked

	DEFINE_FIELD_FLAGS( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_bInReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFireOnEmpty, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFiringWholeClip, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextEmptySoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( m_fFireDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_iszName, FIELD_INTEGER ),		
	DEFINE_FIELD( m_bFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAltFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fMinRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMinRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMaxRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMaxRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( m_bReloadsSingly, FIELD_BOOLEAN ),	
	DEFINE_FIELD( m_bRemoveable, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iPrimaryAmmoCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSecondaryAmmoCount, FIELD_INTEGER ),

	//DEFINE_PHYSPTR( m_pConstraint ),

	// DEFINE_FIELD( m_iOldState, FIELD_INTEGER ),
	// DEFINE_FIELD( m_bJustRestored, FIELD_BOOLEAN ),

	// DEFINE_FIELD( m_OnPlayerPickup, COutputEvent ),
	// DEFINE_FIELD( m_pConstraint, FIELD_INTEGER ),

END_PREDICTION_DATA()

#endif	// ! CLIENT_DLL

// Special hack since we're aliasing the name C_BaseCombatWeapon with a macro on the client
IMPLEMENT_NETWORKCLASS_ALIASED( BaseCombatWeapon, DT_BaseCombatWeapon )

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Save Data for Base Weapon object
//-----------------------------------------------------------------------------// 
BEGIN_MAPENTITY( CBaseCombatWeapon )

	DEFINE_INPUTFUNC( FIELD_VOID, "HideWeapon", InputHideWeapon ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAmmo1", InputSetAmmo1 ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAmmo2", InputSetAmmo2 ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GiveDefaultAmmo", InputGiveDefaultAmmo ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePlayerPickup", InputEnablePlayerPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePlayerPickup", InputDisablePlayerPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableNPCPickup", InputEnableNPCPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableNPCPickup", InputDisableNPCPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BreakConstraint", InputBreakConstraint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForcePrimaryFire", InputForcePrimaryFire ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceSecondaryFire", InputForceSecondaryFire ),

	// Outputs
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse"),
	DEFINE_OUTPUT( m_OnPlayerPickup, "OnPlayerPickup"),
	DEFINE_OUTPUT( m_OnNPCPickup, "OnNPCPickup"),
	DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),
	DEFINE_OUTPUT( m_OnDropped, "OnDropped" ),

END_MAPENTITY()

//-----------------------------------------------------------------------------
// Purpose: Only send to local player if this weapon is the active weapon
// Input  : *pStruct - 
//			*pVarData - 
//			*pRecipients - 
//			objectID - 
// Output : void*
//-----------------------------------------------------------------------------
void* SendProxy_SendActiveLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer /*&& pPlayer->GetActiveWeapon() == pWeapon*/ )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}
	
	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendActiveLocalWeaponDataTable );

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
		else if (pWeapon->HasSpawnFlags( SF_WEAPON_PRESERVE_AMMO ))
		{
			// Ammo values are sent to the client using this proxy.
			// Preserved ammo values from the server need to be sent to the client ASAP to avoid HUD issues, etc.
			// I've tried many nasty hacks, but this is the one that works well enough and there's not much else we could do.
			pRecipients->SetAllRecipients();
			return (void*)pVarData;
		}
	}
	
	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalWeaponDataTable );

//-----------------------------------------------------------------------------
// Purpose: Only send to non-local players
//-----------------------------------------------------------------------------
void* SendProxy_SendNonLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->ClearRecipient( pPlayer->GetClientIndex() );
			return ( void * )pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalWeaponDataTable );

#else
void CSharedBaseCombatWeapon::RecvProxy_WeaponState( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseCombatWeapon *pWeapon = (C_BaseCombatWeapon*)pStruct;
	pWeapon->m_iState = pData->m_Value.m_Int;
	pWeapon->UpdateVisibility();
}
#endif

#if PREDICTION_ERROR_CHECK_LEVEL > 1
#define SendPropTime SendPropFloat
#define RecvPropTime RecvPropFloat
#endif

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CSharedBaseCombatWeapon, DT_LocalActiveWeaponData )
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flNextPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flNextSecondaryAttack ) ),
	SendPropInt( SENDINFO( m_nNextThinkTick ) ),
	SendPropTime( SENDINFO( m_flTimeWeaponIdle ) ),

	SendPropExclude( SENDEXLCUDE( DT_AnimTimeMustBeFirst, m_flAnimTime ) ),

	SendPropBool( SENDINFO( m_bHolstered ) ),
#else
	RecvPropTime( RECVINFO( m_flNextPrimaryAttack ) ),
	RecvPropTime( RECVINFO( m_flNextSecondaryAttack ) ),
	RecvPropInt( RECVINFO( m_nNextThinkTick ) ),
	RecvPropTime( RECVINFO( m_flTimeWeaponIdle ) ),
	RecvPropBool( RECVINFO( m_bHolstered ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CSharedBaseCombatWeapon, DT_LocalWeaponData )
#if !defined( CLIENT_DLL )
	SendPropIntWithMinusOneFlag( SENDINFO(m_iClip1 ), 8 ),
	SendPropIntWithMinusOneFlag( SENDINFO(m_iClip2 ), 8 ),
	SendPropInt( SENDINFO(m_iPrimaryAmmoType ), 8 ),
	SendPropInt( SENDINFO(m_iSecondaryAmmoType ), 8 ),

	SendPropInt( SENDINFO( m_nViewModelIndex ), VIEWMODEL_INDEX_BITS, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_bFlipViewModel ) ),

	SendPropExclude( SENDEXLCUDE( DT_AnimTimeMustBeFirst, m_flAnimTime ) ),

#else
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iClip1 )),
	RecvPropIntWithMinusOneFlag( RECVINFO(m_iClip2 )),
	RecvPropInt( RECVINFO(m_iPrimaryAmmoType )),
	RecvPropInt( RECVINFO(m_iSecondaryAmmoType )),

	RecvPropInt( RECVINFO( m_nViewModelIndex ) ),

	RecvPropBool( RECVINFO( m_bFlipViewModel ) ),

#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CSharedBaseCombatWeapon, DT_BaseCombatWeapon)
#if !defined( CLIENT_DLL )
	SendPropDataTable("LocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalWeaponData), SendProxy_SendLocalWeaponDataTable ),
	SendPropDataTable("LocalActiveWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalActiveWeaponData), SendProxy_SendActiveLocalWeaponDataTable ),
	SendPropModelIndex( SENDINFO(m_iViewModelIndex) ),
	SendPropModelIndex( SENDINFO(m_iWorldModelIndex) ),
	SendPropModelIndex( SENDINFO(m_iDroppedModelIndex) ),
	SendPropInt( SENDINFO(m_iState ), 8, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO(m_hOwner) ),
#else
	RecvPropDataTable("LocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalWeaponData)),
	RecvPropDataTable("LocalActiveWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalActiveWeaponData)),
	RecvPropInt( RECVINFO(m_iViewModelIndex)),
	RecvPropInt( RECVINFO(m_iWorldModelIndex)),
	RecvPropInt( RECVINFO(m_iDroppedModelIndex) ),
	RecvPropInt( RECVINFO(m_iState), 0, &CSharedBaseCombatWeapon::RecvProxy_WeaponState ),
	RecvPropEHandle( RECVINFO(m_hOwner ) ),
#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )

int Studio_FindAttachment( const CStudioHdr *pStudioHdr, const char *pAttachmentName );

int CSharedBaseCombatWeapon::LookupAttachment( const char *pAttachmentName )
{
	int newIndex = GetWorldModelIndex();
	if ( newIndex != GetModelIndex() )
	{
		const model_t *pWorldModel = modelinfo->GetModel( newIndex );
		if ( pWorldModel )
		{
			MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle( pWorldModel );
			if ( MDLHANDLE_INVALID != hStudioHdr )
			{
				const studiohdr_t *pStudioHdr = g_pMDLCache->GetStudioHdr( hStudioHdr );
				if ( pStudioHdr )
				{
					CStudioHdr studioHdrContainer( pStudioHdr, g_pMDLCache );
					int iRet = Studio_FindAttachment( &studioHdrContainer, pAttachmentName ) + 1;
					return iRet;
				}
			}
		}
	}

	return BaseClass::LookupAttachment( pAttachmentName );
}
#endif
