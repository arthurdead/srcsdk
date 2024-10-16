//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TAKEDAMAGEINFO_H
#define TAKEDAMAGEINFO_H
#pragma once


#include "networkvar.h" // todo: change this when DECLARE_CLASS is moved into a better location.
#include "ehandle.h"

// Used to initialize m_flBaseDamage to something that we know pretty much for sure
// hasn't been modified by a user. 
#define BASEDAMAGE_NOT_SPECIFIED	FLT_MAX

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
class CTakeDamageInfo;
typedef CTakeDamageInfo CSharedTakeDamageInfo;
class CMultiDamage;
typedef CMultiDamage CSharedMultiDamage;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
class C_TakeDamageInfo;
typedef C_TakeDamageInfo CSharedTakeDamageInfo;
class C_MultiDamage;
typedef C_MultiDamage CSharedMultiDamage;
#endif

#ifdef CLIENT_DLL
	#define CTakeDamageInfo C_TakeDamageInfo
#endif

class CTakeDamageInfo
{
public:
	DECLARE_CLASS_NOBASE( CTakeDamageInfo );

	CTakeDamageInfo();
	CTakeDamageInfo( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0 );
	CTakeDamageInfo( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0 );
	CTakeDamageInfo( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
	CTakeDamageInfo( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );

#ifdef CLIENT_DLL
	#undef CTakeDamageInfo
#endif

	// Inflictor is the weapon or rocket (or player) that is dealing the damage.
	CSharedBaseEntity*	GetInflictor() const;
	void			SetInflictor( CSharedBaseEntity *pInflictor );

	// Weapon is the weapon that did the attack.
	// For hitscan weapons, it'll be the same as the inflictor. For projectile weapons, the projectile 
	// is the inflictor, and this contains the weapon that created the projectile.
	CSharedBaseEntity*	GetWeapon() const;
	void			SetWeapon( CSharedBaseEntity *pWeapon );

	// Attacker is the character who originated the attack (like a player or an AI).
	CSharedBaseEntity*	GetAttacker() const;
	void			SetAttacker( CSharedBaseEntity *pAttacker );

	float			GetDamage() const;
	void			SetDamage( float flDamage );
	float			GetMaxDamage() const;
	void			SetMaxDamage( float flMaxDamage );
	void			ScaleDamage( float flScaleAmount );
	void			AddDamage( float flAddAmount );
	void			SubtractDamage( float flSubtractAmount );
	float			GetDamageBonus() const;
	CSharedBaseEntity		*GetDamageBonusProvider() const;
	void			SetDamageBonus( float flBonus, CSharedBaseEntity *pProvider = NULL );

	float			GetBaseDamage() const;
	bool			BaseDamageIsValid() const;

	Vector			GetDamageForce() const;
	void			SetDamageForce( const Vector &damageForce );
	void			ScaleDamageForce( float flScaleAmount );
	float			GetDamageForForceCalc() const;
	void			SetDamageForForceCalc( const float flScaleAmount );

	Vector			GetDamagePosition() const;
	void			SetDamagePosition( const Vector &damagePosition );

	Vector			GetReportedPosition() const;
	void			SetReportedPosition( const Vector &reportedPosition );

	int				GetDamageType() const;
	void			SetDamageType( int bitsDamageType );
	void			AddDamageType( int bitsDamageType );
	int				GetDamageCustom( void ) const;
	void			SetDamageCustom( int iDamageCustom );
	int				GetDamageStats( void ) const;
	void			SetDamageStats( int iDamageStats );
	void			SetForceFriendlyFire( bool bValue ) { m_bForceFriendlyFire = bValue; }
	bool			IsForceFriendlyFire( void ) const { return m_bForceFriendlyFire; }

	int				GetAmmoType() const;
	void			SetAmmoType( int iAmmoType );
	const char *	GetAmmoName() const;

	float			GetRadius() const;
	void			SetRadius( float fRadius );

	int				GetPlayerPenetrationCount() const { return m_iPlayerPenetrationCount; }
	void			SetPlayerPenetrationCount( int iPlayerPenetrationCount ) { m_iPlayerPenetrationCount = iPlayerPenetrationCount; }
	
	int				GetDamagedOtherPlayers() const     { return m_iDamagedOtherPlayers; }
	void			SetDamagedOtherPlayers( int iVal ) { m_iDamagedOtherPlayers = iVal; }

	void			Set( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0 );
	void			Set( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0 );
	void			Set( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
	void			Set( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );

	void			AdjustPlayerDamageInflictedForSkillLevel();
	void			AdjustPlayerDamageTakenForSkillLevel();

	// Given a damage type (composed of the #defines above), fill out a string with the appropriate text.
	// For designer debug output.
	static void		DebugGetDamageTypeString(unsigned int DamageType, char *outbuf, int outbuflength );


//private:
	void			CopyDamageToBaseDamage();

protected:
	void			Init( CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );

	Vector			m_vecDamageForce;
	Vector			m_vecDamagePosition;
	Vector			m_vecReportedPosition;	// Position players are told damage is coming from
	EHANDLE			m_hInflictor;
	EHANDLE			m_hAttacker;
	EHANDLE			m_hWeapon;
	float			m_flDamage;
	float			m_flMaxDamage;
	float			m_flBaseDamage;			// The damage amount before skill leve adjustments are made. Used to get uniform damage forces.
	int				m_bitsDamageType;
	int				m_iDamageCustom;
	int				m_iDamageStats;
	int				m_iAmmoType;			// AmmoType of the weapon used to cause this damage, if any
	int				m_iDamagedOtherPlayers;
	int				m_iPlayerPenetrationCount;
	float			m_flDamageBonus;		// Anything that increases damage (crit) - store the delta
	EHANDLE			m_hDamageBonusProvider;	// Who gave us the ability to do extra damage?
	bool			m_bForceFriendlyFire;	// Ideally this would be a dmg type, but we can't add more
	float			m_flRadius;

	float			m_flDamageForForce;
};

//-----------------------------------------------------------------------------
// Purpose: Multi damage. Used to collect multiple damages in the same frame (i.e. shotgun pellets)
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
	#define CMultiDamage C_MultiDamage
#endif

class CMultiDamage : public CSharedTakeDamageInfo
{
public:
	DECLARE_CLASS( CMultiDamage, CSharedTakeDamageInfo );
	CMultiDamage();

#ifdef CLIENT_DLL
	#undef CMultiDamage
#endif

	bool			IsClear( void ) { return (m_hTarget == NULL); }
	CSharedBaseEntity		*GetTarget() const;
	void			SetTarget( CSharedBaseEntity *pTarget );

	void			Init( CSharedBaseEntity *pTarget, CSharedBaseEntity *pInflictor, CSharedBaseEntity *pAttacker, CSharedBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );

protected:
	EHANDLE			m_hTarget;
};

extern CSharedMultiDamage g_MultiDamage;

// Multidamage accessors
void ClearMultiDamage( void );
void ApplyMultiDamage( void );
void AddMultiDamage( const CSharedTakeDamageInfo &info, CSharedBaseEntity *pEntity );

//-----------------------------------------------------------------------------
// Purpose: Utility functions for physics damage force calculation 
//-----------------------------------------------------------------------------
float ImpulseScale( float flTargetMass, float flDesiredSpeed );
void CalculateExplosiveDamageForce( CSharedTakeDamageInfo *info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateBulletDamageForce( CSharedTakeDamageInfo *info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateMeleeDamageForce( CSharedTakeDamageInfo *info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void GuessDamageForce( CSharedTakeDamageInfo *info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale = 1.0 );


// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //

inline CSharedBaseEntity* CSharedTakeDamageInfo::GetInflictor() const
{
	return m_hInflictor.Get();
}


inline void CSharedTakeDamageInfo::SetInflictor( CSharedBaseEntity *pInflictor )
{
	m_hInflictor = pInflictor;
}


inline CSharedBaseEntity* CSharedTakeDamageInfo::GetAttacker() const
{
	return m_hAttacker.Get();
}


inline void CSharedTakeDamageInfo::SetAttacker( CSharedBaseEntity *pAttacker )
{
	m_hAttacker = pAttacker;
}

inline CSharedBaseEntity* CSharedTakeDamageInfo::GetWeapon() const
{
	return m_hWeapon.Get();
}


inline void CSharedTakeDamageInfo::SetWeapon( CSharedBaseEntity *pWeapon )
{
	m_hWeapon = pWeapon;
}


inline float CSharedTakeDamageInfo::GetDamage() const
{
	return m_flDamage;
}

inline void CSharedTakeDamageInfo::SetDamage( float flDamage )
{
	m_flDamage = flDamage;
}

inline float CSharedTakeDamageInfo::GetMaxDamage() const
{
	return m_flMaxDamage;
}

inline void CSharedTakeDamageInfo::SetMaxDamage( float flMaxDamage )
{
	m_flMaxDamage = flMaxDamage;
}

inline void CSharedTakeDamageInfo::ScaleDamage( float flScaleAmount )
{
	m_flDamage *= flScaleAmount;
}

inline void CSharedTakeDamageInfo::AddDamage( float flAddAmount )
{
	m_flDamage += flAddAmount;
}

inline void CSharedTakeDamageInfo::SubtractDamage( float flSubtractAmount )
{
	m_flDamage -= flSubtractAmount;
}

inline float CSharedTakeDamageInfo::GetDamageBonus() const
{
	return m_flDamageBonus;
}

inline CSharedBaseEntity *CSharedTakeDamageInfo::GetDamageBonusProvider() const
{
	return m_hDamageBonusProvider.Get();
}

inline void CSharedTakeDamageInfo::SetDamageBonus( float flBonus, CSharedBaseEntity *pProvider /*= NULL*/ )
{
	m_flDamageBonus = flBonus;
	m_hDamageBonusProvider = pProvider;
}

inline float CSharedTakeDamageInfo::GetBaseDamage() const
{
	if( BaseDamageIsValid() )
		return m_flBaseDamage;

	// No one ever specified a base damage, so just return damage.
	return m_flDamage;
}

inline bool CSharedTakeDamageInfo::BaseDamageIsValid() const
{
	return (m_flBaseDamage != BASEDAMAGE_NOT_SPECIFIED);
}

inline Vector CSharedTakeDamageInfo::GetDamageForce() const
{
	return m_vecDamageForce;
}

inline void CSharedTakeDamageInfo::SetDamageForce( const Vector &damageForce )
{
	m_vecDamageForce = damageForce;
}

inline void	CSharedTakeDamageInfo::ScaleDamageForce( float flScaleAmount )
{
	m_vecDamageForce *= flScaleAmount;
}

inline float CSharedTakeDamageInfo::GetDamageForForceCalc() const
{
	return m_flDamageForForce;
}

inline void CSharedTakeDamageInfo::SetDamageForForceCalc( float flDamage )
{
	m_flDamageForForce = flDamage;
}

inline Vector CSharedTakeDamageInfo::GetDamagePosition() const
{
	return m_vecDamagePosition;
}


inline void CSharedTakeDamageInfo::SetDamagePosition( const Vector &damagePosition )
{
	m_vecDamagePosition = damagePosition;
}

inline Vector CSharedTakeDamageInfo::GetReportedPosition() const
{
	return m_vecReportedPosition;
}


inline void CSharedTakeDamageInfo::SetReportedPosition( const Vector &reportedPosition )
{
	m_vecReportedPosition = reportedPosition;
}


inline void CSharedTakeDamageInfo::SetDamageType( int bitsDamageType )
{
	m_bitsDamageType = bitsDamageType;
}

inline int CSharedTakeDamageInfo::GetDamageType() const
{
	return m_bitsDamageType;
}

inline void	CSharedTakeDamageInfo::AddDamageType( int bitsDamageType )
{
	m_bitsDamageType |= bitsDamageType;
}

inline int CSharedTakeDamageInfo::GetDamageCustom() const
{
	return m_iDamageCustom;
}

inline void CSharedTakeDamageInfo::SetDamageCustom( int iDamageCustom )
{
	m_iDamageCustom = iDamageCustom;
}

inline int CSharedTakeDamageInfo::GetDamageStats() const
{
	return m_iDamageStats;
}

inline void CSharedTakeDamageInfo::SetDamageStats( int iDamageCustom )
{
	m_iDamageStats = iDamageCustom;
}

inline int CSharedTakeDamageInfo::GetAmmoType() const
{
	return m_iAmmoType;
}

inline void CSharedTakeDamageInfo::SetAmmoType( int iAmmoType )
{
	m_iAmmoType = iAmmoType;
}

inline void CSharedTakeDamageInfo::CopyDamageToBaseDamage()
{ 
	m_flBaseDamage = m_flDamage;
}

inline float CSharedTakeDamageInfo::GetRadius() const
{
	return m_flRadius;
}

inline void CSharedTakeDamageInfo::SetRadius( float flRadius )
{
	m_flRadius = flRadius;
}

// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //
inline CSharedBaseEntity *CSharedMultiDamage::GetTarget() const
{
	return m_hTarget.Get();
}

inline void CSharedMultiDamage::SetTarget( CSharedBaseEntity *pTarget )
{
	m_hTarget = pTarget;
}


#endif // TAKEDAMAGEINFO_H
