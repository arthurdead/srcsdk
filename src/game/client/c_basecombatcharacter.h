//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the client-side representation of CBaseCombatCharacter.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASECOMBATCHARACTER_H
#define C_BASECOMBATCHARACTER_H

#pragma once

#include "shareddefs.h"
#include "c_baseflex.h"
#include "basecombatcharacter_shared.h"

class C_BaseCombatWeapon;

class CNavArea;

#define BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE 0.9f

class C_BaseCombatCharacter : public C_BaseFlex
{
	DECLARE_CLASS( C_BaseCombatCharacter, C_BaseFlex );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

					C_BaseCombatCharacter( void );
	virtual			~C_BaseCombatCharacter( void );

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual bool	IsBaseCombatCharacter( void ) { return true; };
	virtual C_BaseCombatCharacter *MyCombatCharacterPointer( void ) { return this; }

	// -----------------------
	// Vision
	// -----------------------
	enum FieldOfViewCheckType { USE_FOV, DISREGARD_FOV };
	bool IsAbleToSee( const CBaseEntity *entity, FieldOfViewCheckType checkFOV );	// Visible starts with line of sight, and adds all the extra game checks like fog, smoke, camo...
	bool IsAbleToSee( C_BaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV );	// Visible starts with line of sight, and adds all the extra game checks like fog, smoke, camo...

	virtual bool IsLookingTowards( const CBaseEntity *target, float cosTolerance = BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE ) const;	// return true if our view direction is pointing at the given target, within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
	virtual bool IsLookingTowards( const Vector &target, float cosTolerance = BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE ) const;	// return true if our view direction is pointing at the given target, within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.

	virtual bool IsInFieldOfView( CBaseEntity *entity ) const;	// Calls IsLookingAt with the current field of view.  
	virtual bool IsInFieldOfView( const Vector &pos ) const;

	enum LineOfSightCheckType
	{
		IGNORE_NOTHING,
		IGNORE_ACTORS
	};
	virtual bool IsLineOfSightClear( CBaseEntity *entity, LineOfSightCheckType checkType = IGNORE_NOTHING ) const;// strictly LOS check with no other considerations
	virtual bool IsLineOfSightClear( const Vector &pos, LineOfSightCheckType checkType = IGNORE_NOTHING, CBaseEntity *entityToIgnore = NULL ) const;


	// -----------------------
	// Ammo
	// -----------------------
	void				RemoveAmmo( int iCount, int iAmmoIndex );
	void				RemoveAmmo( int iCount, const char *szName );
	void				RemoveAllAmmo( );
	int					GetAmmoCount( int iAmmoIndex ) const;
	int					GetAmmoCount( char *szName ) const;

	virtual C_BaseCombatWeapon*	Weapon_OwnsThisType( const char *pszWeapon, int iSubType = 0 ) const;  // True if already owns a weapon of this class
	virtual int			Weapon_GetSlot( const char *pszWeapon, int iSubType = 0 ) const;  // Returns -1 if they don't have one
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual bool		Weapon_CanSwitchTo(C_BaseCombatWeapon *pWeapon);
	
	// I can't use my current weapon anymore. Switch me to the next best weapon.
	bool SwitchToNextBestWeapon(C_BaseCombatWeapon *pCurrent);

	virtual C_BaseCombatWeapon	*GetActiveWeapon( void ) const;
	int					WeaponCount() const;
	virtual C_BaseCombatWeapon	*GetWeapon( int i ) const;

	// This is a sort of hack back-door only used by physgun!
	void SetAmmoCount( int iCount, int iAmmoIndex );

	float				GetNextAttack() const { return m_flNextAttack; }
	void				SetNextAttack( float flWait ) { m_flNextAttack = flWait; }

	virtual int			BloodColor();

	// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
	void SetBloodColor( int nBloodColor );

	virtual void		DoMuzzleFlash();

	virtual void DoImpactEffect(trace_t &tr, int nDamageType);

	virtual void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);

	virtual	Vector		Weapon_ShootPosition( );		// gun position at current position/orientation

	// Weapons
	void				Weapon_SetActivity( Activity newActivity, float duration );
	virtual void		Weapon_FrameUpdate( void );

#ifdef USE_NAV_MESH
	// Nav mesh
	virtual CNavArea *GetLastKnownArea( void ) const		{ return m_lastNavArea; }		// return the last nav area the player occupied - NULL if unknown
	virtual bool IsAreaTraversable( const CNavArea *area ) const;							// return true if we can use the given area 
	virtual void ClearLastKnownArea( void );
	virtual void UpdateLastKnownArea( void );										// invoke this to update our last known nav area (since there is no think method chained to CBaseCombatCharacter)
	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea ) { }	// invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)
	virtual void OnNavAreaRemoved( CNavArea *removedArea );
#endif // USE_NAV_MESH

public:

	float			m_flNextAttack;

protected:

	int			m_bloodColor;			// color of blood particless

#ifdef USE_NAV_MESH
	// last known navigation area of player - NULL if unknown
	CNavArea *m_lastNavArea;
	int m_registeredNavTeam;	// ugly, but needed to clean up player team counts in nav mesh
#endif // USE_NAV_MESH

public:
	Vector		m_HackedGunPos;			// HACK until we can query end of gun

private:
	bool				ComputeLOS( const Vector &vecEyePosition, const Vector &vecTarget ) const;

	CNetworkArray( int, m_iAmmo, MAX_AMMO_TYPES );

	CHandle<C_BaseCombatWeapon>		m_hMyWeapons[MAX_WEAPONS];
	CHandle< C_BaseCombatWeapon > m_hActiveWeapon;

private:
	C_BaseCombatCharacter( const C_BaseCombatCharacter & ); // not defined, not accessible


//-----------------------
#ifdef INVASION_CLIENT_DLL
public:
	virtual void	Release( void );
	virtual void	SetDormant( bool bDormant );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );

	// TF2 Powerups
	virtual bool	CanBePoweredUp( void ) { return true; }
	bool			HasPowerup( int iPowerup ) { return ( m_iPowerups & (1 << iPowerup) ) != 0; };
	virtual void	PowerupStart( int iPowerup, bool bInitial );
	virtual void	PowerupEnd( int iPowerup );
	void			RemoveAllPowerups( void );

	// Powerup effects
	void			AddEMPEffect( float flSize );
	void			AddBuffEffect( float flSize );

	C_WeaponCombatShield		*GetShield( void );

public:
	int				m_iPowerups;
	int				m_iPrevPowerups;
#endif

};

inline C_BaseCombatCharacter *ToBaseCombatCharacter( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsBaseCombatCharacter() )
		return NULL;

#if _DEBUG
	return dynamic_cast<C_BaseCombatCharacter *>( pEntity );
#else
	return static_cast<C_BaseCombatCharacter *>( pEntity );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int	C_BaseCombatCharacter::WeaponCount() const
{
	return MAX_WEAPONS;
}

EXTERN_RECV_TABLE(DT_BaseCombatCharacter);

#endif // C_BASECOMBATCHARACTER_H
