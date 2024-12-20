//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEGRENADE_SHARED_H
#define BASEGRENADE_SHARED_H
#pragma once

#include "baseprojectile.h"

#if defined( CLIENT_DLL )

class C_BaseGrenade;
typedef C_BaseGrenade CSharedBaseGrenade;

#include "c_basecombatcharacter.h"

#else

class CBaseGrenade;
typedef CBaseGrenade CSharedBaseGrenade;

#include "basecombatcharacter.h"
#include "player_pickup.h"

#endif

#define BASEGRENADE_EXPLOSION_VOLUME	1024

class CTakeDamageInfo;

#ifdef CLIENT_DLL
	#define CBaseGrenade C_BaseGrenade
#endif

class CBaseGrenade : public CSharedBaseProjectile
#ifdef GAME_DLL
, public CDefaultPlayerPickupVPhysics
#endif
{
public:
	DECLARE_CLASS( CBaseGrenade, CSharedBaseProjectile );
	CBaseGrenade(void);
	~CBaseGrenade(void);
private:
	CBaseGrenade( const CBaseGrenade & ); // not defined, not accessible

public:
#ifdef CLIENT_DLL
	#undef CBaseGrenade
#endif

	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();
#if !defined( CLIENT_DLL )
	DECLARE_MAPENTITY();
#endif

	virtual void		Precache( void );

	virtual void		Explode( trace_t *pTrace, int bitsDamageType );
	void				Smoke( void );

	void				BounceTouch( CSharedBaseEntity *pOther );
	void				SlideTouch( CSharedBaseEntity *pOther );
	void				ExplodeTouch( CSharedBaseEntity *pOther );
	void				DangerSoundThink( void );
	void				PreDetonate( void );
	virtual void		Detonate( void );
	void				DetonateUse( CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, USE_TYPE useType, float value );
	void				TumbleThink( void );

	virtual Vector		GetBlastForce() { return vec3_origin; }

	virtual void		BounceSound( void );
	virtual int			BloodColor( void ) { return DONT_BLEED; }
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual float		GetShakeAmplitude( void ) { return 25.0; }
	virtual float		GetShakeRadius( void ) { return 750.0; }

	// Damage accessors.
	virtual float GetDamage()
	{
		return m_flDamage;
	}
	virtual float GetDamageRadius()
	{
		return m_DmgRadius;
	}

	virtual void SetDamage(float flDamage)
	{
		m_flDamage = flDamage;
	}

	virtual void SetDamageRadius(float flDamageRadius)
	{
		m_DmgRadius = flDamageRadius;
	}

	// Bounce sound accessors.
	void SetBounceSound( const char *pszBounceSound ) 
	{
		m_iszBounceSound = MAKE_STRING( pszBounceSound );
	}

	CSharedBaseCombatCharacter *GetThrower( void );
	void				  SetThrower( CSharedBaseCombatCharacter *pThrower );
	CSharedBaseEntity *GetOriginalThrower() { return m_hOriginalThrower.Get(); }

	float				GetDetonateTime() { return m_flDetonateTime; }
	bool				HasWarnedAI() { return m_bHasWarnedAI; }
	bool				IsLive() { return m_bIsLive; }
	float				GetWarnAITime() { return m_flWarnAITime; }

#if !defined( CLIENT_DLL )
	// Allow +USE pickup
	int ObjectCaps() 
	{ 
		return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS);
	}

	void				Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
#endif

#ifdef GAME_DLL
	void				InputSetDamage( inputdata_t &&inputdata );
	void				InputDetonate( inputdata_t &&inputdata );
#endif

public:
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_fFlags );
	
	bool				m_bHasWarnedAI;				// whether or not this grenade has issued its DANGER sound to the world sound list yet.
	CNetworkVar( bool, m_bIsLive );					// Is this grenade live, or can it be picked up?
	CNetworkVar( float, m_DmgRadius );				// How far do I do damage?
	CNetworkVar( float, m_flNextAttack );
	float				m_flDetonateTime;			// Time at which to detonate.
	float				m_flWarnAITime;				// Time at which to warn the AI

protected:

	CNetworkVar( float, m_flDamage );		// Damage to inflict.
	string_t m_iszBounceSound;	// The sound to make on bouncing.  If not NULL, overrides the BounceSound() function.

#if !defined(CLIENT_DLL)
	COutputEvent m_OnDetonate;
	COutputVector m_OnDetonate_OutPosition;
#endif

private:
	CNetworkHandle( CSharedBaseEntity, m_hThrower );					// Who threw this grenade
	EHANDLE			m_hOriginalThrower;							// Who was the original thrower of this grenade
};

#endif // BASEGRENADE_SHARED_H
