//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"
#include "querycache.h"
#ifdef GAME_DLL
#include "lightcache.h"
#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool GetWorldFogParams( CSharedBaseCombatCharacter *character, fogparams_t &fog );

ConVar NavObscureRange("ai_obscure_range", "400", FCVAR_CHEAT|FCVAR_REPLICATED);

//-----------------------------------------------------------------------------
// Purpose: Switches to the best weapon that is also better than the given weapon.
// Input  : pCurrent - The current weapon used by the player.
// Output : Returns true if the weapon was switched, false if there was no better
//			weapon to switch to.
//-----------------------------------------------------------------------------
bool CSharedBaseCombatCharacter::SwitchToNextBestWeapon(CSharedBaseCombatWeapon *pCurrent)
{
	CSharedBaseCombatWeapon *pNewWeapon = GameRules()->GetNextBestWeapon(this, pCurrent);
	
	if ( ( pNewWeapon != NULL ) && ( pNewWeapon != pCurrent ) )
	{
		return Weapon_Switch( pNewWeapon ) != WEAPON_SWITCH_FAILED;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Switches to the given weapon (providing it has ammo)
// Input  :
// Output : true is switch succeeded
//-----------------------------------------------------------------------------
int CSharedBaseCombatCharacter::Weapon_Switch( CSharedBaseCombatWeapon *pWeapon, int viewmodelindex /*=VIEWMODEL_WEAPON*/, bool bDeploy ) 
{
	if ( pWeapon == NULL )
		return WEAPON_SWITCH_FAILED;

	// Already have it out?
	if ( m_hActiveWeapon.Get() == pWeapon )
	{
		if ( bDeploy && m_hActiveWeapon->CanDeploy() ) {
			if( m_hActiveWeapon->Deploy( ) ) {
				return WEAPON_SWITCH_DEPLOYED;
			} else {
				return WEAPON_SWITCH_FAILED;
			}
		} else {
			m_hActiveWeapon->Holster( NULL, true );
			return WEAPON_SWITCH_HOLSTERED;
		}
	}

	if (!Weapon_CanSwitchTo(pWeapon))
	{
		return WEAPON_SWITCH_FAILED;
	}

	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->Holster( pWeapon ) )
			return WEAPON_SWITCH_FAILED;
	}

	m_hActiveWeapon = pWeapon;

	if( bDeploy && pWeapon->CanDeploy() ) {
		if( pWeapon->Deploy() ) {
			return WEAPON_SWITCH_DEPLOYED;
		} else {
			return WEAPON_SWITCH_FAILED;
		}
	} else {
		pWeapon->Holster( NULL, true );
		return WEAPON_SWITCH_HOLSTERED;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon.
// Input  : pWeapon - 
//-----------------------------------------------------------------------------
bool CSharedBaseCombatCharacter::Weapon_CanSwitchTo( CSharedBaseCombatWeapon *pWeapon )
{
	if (IsPlayer())
	{
		CSharedBasePlayer *pPlayer = (CSharedBasePlayer *)this;
#if !defined( CLIENT_DLL )
		IServerVehicle *pVehicle = pPlayer->GetVehicle();
#else
		IClientVehicle *pVehicle = pPlayer->GetVehicle();
#endif
		if (pVehicle && !pPlayer->UsingStandardWeaponsInVehicle())
			return false;
	}

	if ( !pWeapon->HasAnyAmmo() && !GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) && !pWeapon->HasSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY) )
		return false;
	
	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->CanHolster() && !pWeapon->ForceWeaponSwitch() )
			return false;

		if ( IsPlayer() )
		{
			CSharedBasePlayer *pPlayer = (CSharedBasePlayer *)this;
			// check if active weapon force the last weapon to switch
			if ( m_hActiveWeapon->ForceWeaponSwitch() )
			{
				// last weapon wasn't allowed to switch, don't allow to switch to new weapon
				CSharedBaseCombatWeapon *pLastWeapon = pPlayer->GetLastWeapon();
				if ( pLastWeapon && pWeapon != pLastWeapon && !pLastWeapon->CanHolster() && !pWeapon->ForceWeaponSwitch() )
				{
					return false;
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon *CSharedBaseCombatCharacter::GetActiveWeapon() const
{
	return m_hActiveWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon *CSharedBaseCombatCharacter::GetWeapon( int i ) const
{
	Assert( (i >= 0) && (i < MAX_WEAPONS) );
	return m_hMyWeapons[i].Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iCount - 
//			iAmmoIndex - 
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::RemoveAmmo( int iCount, int iAmmoIndex )
{
	if (iCount <= 0)
		return;

	if ( iAmmoIndex < 0 )
		return;

	// Infinite ammo?
	if ( GetAmmoDef()->CanCarryInfiniteAmmo( iAmmoIndex ) )
		return;

	// Ammo pickup sound
	m_iAmmo.Set( iAmmoIndex, MAX( m_iAmmo[iAmmoIndex] - iCount, 0 ) );
}

void CSharedBaseCombatCharacter::RemoveAmmo( int iCount, const char *szName )
{
	RemoveAmmo( iCount, GetAmmoDef()->Index(szName) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::RemoveAllAmmo( )
{
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_iAmmo.Set( i, 0 );
	}
}

//-----------------------------------------------------------------------------
// FIXME: This is a sort of hack back-door only used by physgun!
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::SetAmmoCount( int iCount, int iAmmoIndex )
{
	// NOTE: No sound, no max check! Seems pretty bogus to me!
	m_iAmmo.Set( iAmmoIndex, iCount );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of a particular type owned
//			owned by the character
// Input  :	Ammo Index
// Output :	The amount of ammo
//-----------------------------------------------------------------------------
int CSharedBaseCombatCharacter::GetAmmoCount( int iAmmoIndex ) const
{
	if ( iAmmoIndex == -1 )
		return 0;

	// Infinite ammo?
	if ( GetAmmoDef()->CanCarryInfiniteAmmo( iAmmoIndex ) )
		return 999;

	return m_iAmmo[ iAmmoIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	CSharedBaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( GetAmmoDef()->Index(szName) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapon if already owns a weapon of this class
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon* CSharedBaseCombatCharacter::Weapon_OwnsThisType( const char *pszWeapon, int iSubType ) const
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs( m_hMyWeapons[i], pszWeapon ) )
		{
			// Make sure it matches the subtype
			if ( m_hMyWeapons[i]->GetSubType() == iSubType )
				return m_hMyWeapons[i];
		}
	}
	return NULL;
}

int CSharedBaseCombatCharacter::Weapon_GetSlot( const char *pszWeapon, int iSubType ) const
{
	for ( int i = 0; i < MAX_WEAPONS; i++ ) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs( m_hMyWeapons[i], pszWeapon ) )
		{
			// Make sure it matches the subtype
			if ( m_hMyWeapons[i]->GetSubType() == iSubType )
			{
				return i;
			}
		}
	}

	return -1;
}

int CSharedBaseCombatCharacter::BloodColor()
{
	return m_bloodColor;
}


//-----------------------------------------------------------------------------
// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}

//-----------------------------------------------------------------------------
/**
	The main visibility check.  Checks all the entity specific reasons that could 
	make IsVisible fail.  Then checks points in space to get environmental reasons.
	This is LOS, plus invisibility and fog and smoke and such.
*/

enum VisCacheResult_t
{
	VISCACHE_UNKNOWN = 0,
	VISCACHE_IS_VISIBLE,
	VISCACHE_IS_NOT_VISIBLE,
};

enum
{
	VIS_CACHE_INVALID = 0x80000000
};

#define VIS_CACHE_ENTRY_LIFE .090f

class CCombatCharVisCache : public CAutoGameSystemPerFrame
{
public:
	virtual void FrameUpdatePreEntityThink();
	virtual void LevelShutdownPreEntity();

	int LookupVisibility( const CSharedBaseCombatCharacter *pChar1, CSharedBaseCombatCharacter *pChar2 );
	VisCacheResult_t HasVisibility( int iCache ) const;
	void RegisterVisibility( int iCache, bool bChar1SeesChar2, bool bChar2SeesChar1 );

private:
	struct VisCacheEntry_t
	{
		CHandle< CSharedBaseCombatCharacter >	m_hEntity1;
		CHandle< CSharedBaseCombatCharacter >	m_hEntity2;
		float							m_flTime;
		bool							m_bEntity1CanSeeEntity2;
		bool							m_bEntity2CanSeeEntity1;
	};

	class CVisCacheEntryLess
	{
	public:
		CVisCacheEntryLess( int ) {}
		bool operator!() const { return false; }
		bool operator()( const VisCacheEntry_t &lhs, const VisCacheEntry_t &rhs ) const
		{
			return ( memcmp( &lhs, &rhs, offsetof( VisCacheEntry_t, m_flTime ) ) < 0 );
		}
	};

	CUtlRBTree< VisCacheEntry_t, unsigned short, CVisCacheEntryLess > m_VisCache;

	mutable int m_nTestCount;
	mutable int m_nHitCount;
};

void CCombatCharVisCache::FrameUpdatePreEntityThink()
{
	//	Msg( "test: %d/%d\n", m_nHitCount, m_nTestCount );

	// Lazy retirement of vis cache
	// NOTE: 256 was chosen heuristically based on a playthrough where 200
	// was the max # in the viscache where nothing could be retired.
	if ( m_VisCache.Count() < 256 )
		return;

	int nMaxIndex = m_VisCache.MaxElement() - 1;
	for ( int i = 0; i < 8; ++i )
	{
		int n = RandomInt( 0, nMaxIndex );
		if ( !m_VisCache.IsValidIndex( n ) )
			continue;

		const VisCacheEntry_t &entry = m_VisCache[n];
		if ( !entry.m_hEntity1.IsValid() || !entry.m_hEntity2.IsValid() || ( gpGlobals->curtime - entry.m_flTime > 10.0f ) )
		{
			m_VisCache.RemoveAt( n );
		}
	}
}

void CCombatCharVisCache::LevelShutdownPreEntity()
{
	m_VisCache.Purge();
}

int CCombatCharVisCache::LookupVisibility( const CSharedBaseCombatCharacter *pChar1, CSharedBaseCombatCharacter *pChar2 )
{
	VisCacheEntry_t cacheEntry;
	if ( pChar1 < pChar2 )
	{
		cacheEntry.m_hEntity1 = pChar1;
		cacheEntry.m_hEntity2 = pChar2;
	}
	else
	{
		cacheEntry.m_hEntity1 = pChar2;
		cacheEntry.m_hEntity2 = pChar1;
	}

	int iCache = m_VisCache.Find( cacheEntry );
	if ( iCache == m_VisCache.InvalidIndex() )
	{
		if ( m_VisCache.Count() == m_VisCache.InvalidIndex() )
			return VIS_CACHE_INVALID;

		iCache = m_VisCache.Insert( cacheEntry );
		m_VisCache[iCache].m_flTime = gpGlobals->curtime - 2.0f * VIS_CACHE_ENTRY_LIFE;
	}

	return ( pChar1 < pChar2 ) ? iCache : - iCache - 1;
}

VisCacheResult_t CCombatCharVisCache::HasVisibility( int iCache ) const
{
	if ( iCache == VIS_CACHE_INVALID )
		return VISCACHE_UNKNOWN;
	
	m_nTestCount++;

	bool bReverse = ( iCache < 0 );
	if ( bReverse )
	{
		iCache = - iCache - 1;
	}

	const VisCacheEntry_t &entry = m_VisCache[iCache];
	if ( gpGlobals->curtime - entry.m_flTime > VIS_CACHE_ENTRY_LIFE )
		return VISCACHE_UNKNOWN;

	m_nHitCount++;

	bool bIsVisible = !bReverse ? entry.m_bEntity1CanSeeEntity2 : entry.m_bEntity2CanSeeEntity1;
	return bIsVisible ? VISCACHE_IS_VISIBLE : VISCACHE_IS_NOT_VISIBLE;
}

void CCombatCharVisCache::RegisterVisibility( int iCache, bool bEntity1CanSeeEntity2, bool bEntity2CanSeeEntity1 )
{
	if ( iCache == VIS_CACHE_INVALID )
		return;

	bool bReverse = ( iCache < 0 );
	if ( bReverse )
	{
		iCache = - iCache - 1;
	}

	VisCacheEntry_t &entry = m_VisCache[iCache];
	entry.m_flTime = gpGlobals->curtime;
	if ( !bReverse )
	{
		entry.m_bEntity1CanSeeEntity2 = bEntity1CanSeeEntity2;
		entry.m_bEntity2CanSeeEntity1 = bEntity2CanSeeEntity1;
	}
	else
	{
		entry.m_bEntity1CanSeeEntity2 = bEntity2CanSeeEntity1;
		entry.m_bEntity2CanSeeEntity1 = bEntity1CanSeeEntity2;
	}
}

static CCombatCharVisCache s_CombatCharVisCache;

bool CSharedBaseCombatCharacter::IsAbleToSee( const CSharedBaseEntity *pEntity, FieldOfViewCheckType checkFOV )
{
	CSharedBaseCombatCharacter *pBCC = const_cast<CSharedBaseEntity *>( pEntity )->MyCombatCharacterPointer();
	if ( pBCC )
		return IsAbleToSee( pBCC, checkFOV );

	// Test this every time; it's cheap.
	Vector vecEyePosition = EyePosition();
	Vector vecTargetPosition = pEntity->WorldSpaceCenter();

	Vector vecEyeToTarget;
	VectorSubtract( vecTargetPosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;

	if ( !ComputeLOS( vecEyePosition, vecTargetPosition ) )
		return false;

	if ( flDistToOther > NavObscureRange.GetFloat() )
	{
		if ( ComputeTargetIsInDarkness( vecEyePosition, vecTargetPosition ) )
			return false;
	}

	return ( checkFOV != USE_FOV || IsInFieldOfView( vecTargetPosition ) );
}

static void ComputeSeeTestPosition( Vector *pEyePosition, CSharedBaseCombatCharacter *pBCC )
{
#if defined(GAME_DLL) && 0
	if ( pBCC->IsPlayer() )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pBCC );
		*pEyePosition = !pPlayer->IsDead() ? pPlayer->EyePosition() : pPlayer->GetDeathPosition();
	}
	else
#endif
	{
		*pEyePosition = pBCC->EyePosition();
	}
}

bool CSharedBaseCombatCharacter::IsAbleToSee( CSharedBaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV )
{
	Vector vecEyePosition, vecOtherEyePosition;
	ComputeSeeTestPosition( &vecEyePosition, this );
	ComputeSeeTestPosition( &vecOtherEyePosition, pBCC );

	Vector vecEyeToTarget;
	VectorSubtract( vecOtherEyePosition, vecEyePosition, vecEyeToTarget );
	float flDistToOther = VectorNormalize( vecEyeToTarget ); 

	// Test this every time; it's cheap.
	// We can't see because they are too far in the fog
	if ( IsHiddenByFog( flDistToOther ) )
		return false;

	// Check this every time also, it's cheap; check to see if the enemy is in an obscured area.
	bool bIsInNavObscureRange = ( flDistToOther > NavObscureRange.GetFloat() );

	// Check if we have a cached-off visibility
	int iCache = s_CombatCharVisCache.LookupVisibility( this, pBCC );
	VisCacheResult_t nResult = s_CombatCharVisCache.HasVisibility( iCache );

	// Compute symmetric visibility
	if ( nResult == VISCACHE_UNKNOWN )
	{
		bool bThisCanSeeOther = false, bOtherCanSeeThis = false;
		if ( ComputeLOS( vecEyePosition, vecOtherEyePosition ) )
		{
			if ( bIsInNavObscureRange )
			{
				bThisCanSeeOther = !ComputeTargetIsInDarkness( vecEyePosition, vecOtherEyePosition );
				bOtherCanSeeThis = !ComputeTargetIsInDarkness( vecOtherEyePosition, vecEyePosition );
			}
			else
			{
				bThisCanSeeOther = true;
				bOtherCanSeeThis = true;
			}
		}

		s_CombatCharVisCache.RegisterVisibility( iCache, bThisCanSeeOther, bOtherCanSeeThis );
		nResult = bThisCanSeeOther ? VISCACHE_IS_VISIBLE : VISCACHE_IS_NOT_VISIBLE;
	}

	if ( nResult == VISCACHE_IS_VISIBLE )
		return ( checkFOV != USE_FOV || IsInFieldOfView( pBCC ) );

	return false;
}

class CTraceFilterNoCombatCharacters : public CTraceFilterSimple
{
public:
	CTraceFilterNoCombatCharacters( const IHandleEntity *passentity = NULL, int collisionGroup = COLLISION_GROUP_NONE )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
		{
			CSharedBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
			if ( !pEntity )
				return false;

			if ( pEntity->MyCombatCharacterPointer() || pEntity->MyCombatWeaponPointer() )
				return false;

			// Honor BlockLOS - this lets us see through partially-broken doors, etc
			if ( !pEntity->BlocksLOS() )
				return false;

			return true;
		}

		return false;
	}
};

class CTraceFilterSkipTwoEntitiesNoCombatCharacters : public CTraceFilterNoCombatCharacters
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoEntitiesNoCombatCharacters, CTraceFilterNoCombatCharacters );
	
	CTraceFilterSkipTwoEntitiesNoCombatCharacters( const IHandleEntity *passentity = NULL, const IHandleEntity *passentity2 = NULL, int collisionGroup = COLLISION_GROUP_NONE );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity2( const IHandleEntity *pPassEntity2 ) { m_pPassEnt2 = pPassEntity2; }

private:
	const IHandleEntity *m_pPassEnt2;
};

CTraceFilterSkipTwoEntitiesNoCombatCharacters::CTraceFilterSkipTwoEntitiesNoCombatCharacters( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup ) :
	BaseClass( passentity, collisionGroup ), m_pPassEnt2(passentity2)
{
}

bool CTraceFilterSkipTwoEntitiesNoCombatCharacters::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	Assert( pHandleEntity );
	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt2 ) )
		return false;

	return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
}

bool CSharedBaseCombatCharacter::ComputeLOS( const Vector &vecEyePosition, const Vector &vecTarget ) const
{
	// We simply can't see because the world is in the way.
	trace_t result;
	CTraceFilterNoCombatCharacters traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEyePosition, vecTarget, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, &traceFilter, &result );
	return ( result.fraction == 1.0f );
}

bool CSharedBaseCombatCharacter::ComputeTargetIsInDarkness( const Vector &vecEyePosition, const Vector &vecTargetPos ) const
{
	// Check light info
	const float flMinLightIntensity = 0.1f;

	if ( GetLightIntensity( vecTargetPos ) >= flMinLightIntensity )
		return false;

	CTraceFilterNoNPCsOrPlayer lightingFilter( this, COLLISION_GROUP_NONE );

	Vector vecSightDirection;
	VectorSubtract( vecTargetPos, vecEyePosition, vecSightDirection );
	VectorNormalize( vecSightDirection );

	trace_t result;
	UTIL_TraceLine( vecTargetPos, vecTargetPos + vecSightDirection * 32768.0f, MASK_AI_VISION, &lightingFilter, &result );
	if ( ( result.fraction < 1.0f ) && ( ( result.surface.flags & SURF_SKY ) == 0 ) )
	{
		// Target is in darkness, the wall behind him is too, and we are too far away
		if ( GetLightIntensity( result.endpos ) < flMinLightIntensity )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
/**
	Return true if our view direction is pointing at the given target, 
	within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
*/
bool CSharedBaseCombatCharacter::IsLookingTowards( const CSharedBaseEntity *target, float cosTolerance ) const
{
	return IsLookingTowards( target->WorldSpaceCenter(), cosTolerance ) || IsLookingTowards( target->EyePosition(), cosTolerance ) || IsLookingTowards( target->GetAbsOrigin(), cosTolerance );
}


//-----------------------------------------------------------------------------
/**
	Return true if our view direction is pointing at the given target, 
	within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
*/
bool CSharedBaseCombatCharacter::IsLookingTowards( const Vector &target, float cosTolerance ) const
{
	Vector toTarget = target - EyePosition();
	toTarget.NormalizeInPlace();

	Vector forward;
	AngleVectors( EyeAngles(), &forward );

	return ( DotProduct( forward, toTarget ) >= cosTolerance );
}


//-----------------------------------------------------------------------------
/**
	Returns true if we are looking towards something within a tolerence determined 
	by our field of view
*/
bool CSharedBaseCombatCharacter::IsInFieldOfView( CSharedBaseEntity *entity ) const
{
	CSharedBasePlayer *pPlayer = ToBasePlayer( const_cast< CSharedBaseCombatCharacter* >( this ) );
	float flTolerance = pPlayer ? cos( (float)pPlayer->GetFOV() * 0.5f ) : BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE;

	Vector vecForward;
	Vector vecEyePosition = EyePosition();
	AngleVectors( EyeAngles(), &vecForward );

	// FIXME: Use a faster check than this!

	// Check 3 spots, or else when standing right next to someone looking at their eyes, 
	// the angle will be too great to see their center.
	Vector vecToTarget = entity->GetAbsOrigin() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	if ( DotProduct( vecForward, vecToTarget ) >= flTolerance )
		return true;

	vecToTarget = entity->WorldSpaceCenter() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	if ( DotProduct( vecForward, vecToTarget ) >= flTolerance )
		return true;

	vecToTarget = entity->EyePosition() - vecEyePosition;
	vecToTarget.NormalizeInPlace();
	return ( DotProduct( vecForward, vecToTarget ) >= flTolerance );
}

//-----------------------------------------------------------------------------
/**
	Returns true if we are looking towards something within a tolerence determined 
	by our field of view
*/
bool CSharedBaseCombatCharacter::IsInFieldOfView( const Vector &pos ) const
{
	CSharedBasePlayer *pPlayer = ToBasePlayer( const_cast< CSharedBaseCombatCharacter* >( this ) );

	if ( pPlayer )
		return IsLookingTowards( pos, cos( (float)pPlayer->GetFOV() * 0.5f ) );

	return IsLookingTowards( pos );
}

static bool TraceFilterNoCombatCharacters( IHandleEntity *pServerEntity, int contentsMask )
{
	// Honor BlockLOS also to allow seeing through partially-broken doors
	CSharedBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
	return ( entity->MyCombatCharacterPointer() == NULL && !entity->MyCombatWeaponPointer() && entity->BlocksLOS() );
}

//-----------------------------------------------------------------------------
/**
	Strictly checks Line of Sight only.
*/

bool CSharedBaseCombatCharacter::IsLineOfSightClear( CSharedBaseEntity *entity, LineOfSightCheckType checkType ) const
{
	if( checkType == IGNORE_ACTORS )
	{
		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_WORLDSPACE_CENTER,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, TraceFilterNoCombatCharacters, 1.0 ) )
			return true;

		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_EYEPOSITION,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, TraceFilterNoCombatCharacters, 1.0 ) )
			return true;

		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_ABSORIGIN,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, TraceFilterNoCombatCharacters, 1.0 ) )
			return true;
	}
	else
	{
		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_WORLDSPACE_CENTER,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE, NULL, 1.0 ) )
			return true;

		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_EYEPOSITION,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE, NULL, 1.0 ) )
			return true;

		if( IsLineOfSightBetweenTwoEntitiesClear( const_cast<CSharedBaseCombatCharacter *>(this), EOFFSET_MODE_EYEPOSITION,
			entity, EOFFSET_MODE_ABSORIGIN,
			const_cast<CSharedBaseCombatCharacter *>(this), COLLISION_GROUP_NONE,
			MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE, NULL, 1.0 ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
/**
	Strictly checks Line of Sight only.
*/
bool CSharedBaseCombatCharacter::IsLineOfSightClear( const Vector &pos, LineOfSightCheckType checkType, CSharedBaseEntity *entityToIgnore ) const
{
#if defined(GAME_DLL) && defined(COUNT_BCC_LOS)
	static int count, frame;
	if ( frame != gpGlobals->framecount )
	{
		Msg( ">> %d\n", count );
		frame = gpGlobals->framecount;
		count = 0;
	}
	count++;
#endif

	if( checkType == IGNORE_ACTORS )
	{
		trace_t trace;
		CTraceFilterSkipTwoEntitiesNoCombatCharacters traceFilter( this, entityToIgnore, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), pos, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_MONSTER, &traceFilter, &trace );

		return trace.fraction == 1.0f;
	}
	else
	{
		trace_t trace;
		CTraceFilterSkipTwoEntities traceFilter( this, entityToIgnore, COLLISION_GROUP_NONE );
		UTIL_TraceLine( EyePosition(), pos, MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE, &traceFilter, &trace );

		return trace.fraction == 1.0f;
	}
}


/*
//---------------------------------------------------------------------------------------------------------------------------
surfacedata_t * CSharedBaseCombatCharacter::GetGroundSurface( void ) const
{
	Vector start( vec3_origin );
	Vector end( 0, 0, -64 );

	Vector vecMins, vecMaxs;
	CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );

	Ray_t ray;
	ray.Init( start, end, vecMins, vecMaxs );

	trace_t	trace;
	UTIL_TraceRay( ray, MASK_SOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

	if ( trace.fraction == 1.0f )
		return NULL;	// no ground

	return physprops->GetSurfaceData( trace.surface.surfaceProps );
}
*/

void CSharedBaseCombatCharacter::Weapon_FrameUpdate( void )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->Operator_FrameUpdate( this );
	}
}

#define FINDNAMEDENTITY_MAX_ENTITIES	32
//-----------------------------------------------------------------------------
// Purpose: FindNamedEntity has been moved from CAI_BaseNPC to CBaseCombatCharacter so players can use it.
//			Coincidentally, everything that it did on NPCs could be done on BaseCombatCharacters with no consequences.
// Input  :
// Output :
//-----------------------------------------------------------------------------
CSharedBaseEntity *CSharedBaseCombatCharacter::FindNamedEntity( const char *szName, IEntityFindFilter *pFilter )
{
	const char *name = szName;
	if (name[0] == '!')
		name++;

	if ( !stricmp( name, "player" ))
	{
		return NULL;
	}
	else if ( !stricmp( name, "self" ) || !stricmp( name, "target1" ) )
	{
		return this;
	}
#ifdef GAME_DLL
	else if ( !stricmp( name, "enemy" ) )
	{
		return GetEnemy();
	}
	else if ( !stricmp( name, "nearestfriend" ) || !strnicmp( name, "friend", 6 ) )
	{
		// Just look for the nearest friendly NPC within 500 units
		// (most of this was stolen from CAI_PlayerAlly::FindSpeechTarget())
		const Vector &	vAbsOrigin = GetAbsOrigin();
		float 			closestDistSq = Square(500.0);
		CSharedBaseEntity *	pNearest = NULL;
		float			distSq;
		int				i;
		for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pNPC = (g_AI_Manager.AccessAIs())[i];

			if ( pNPC == this )
				continue;

			distSq = ( vAbsOrigin - pNPC->GetAbsOrigin() ).LengthSqr();
				
			if ( distSq > closestDistSq )
				continue;

			if ( IRelationType( pNPC ) == D_LI )
			{
				closestDistSq = distSq;
				pNearest = pNPC;
			}
		}

		if (stricmp(name, "friend_npc") != 0)
		{
			// Okay, find the nearest friendly client.
			for ( i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CSharedBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
				if ( pPlayer )
				{
					// Don't get players with notarget
					if (pPlayer->GetFlags() & FL_NOTARGET)
						continue;

					distSq = ( vAbsOrigin - pPlayer->GetAbsOrigin() ).LengthSqr();
					
					if ( distSq > closestDistSq )
						continue;

					if ( IRelationType( pPlayer ) == D_LI )
					{
						closestDistSq = distSq;
						pNearest = pPlayer;
					}
				}
			}
		}

		return pNearest;
	}
#endif
	else if (!stricmp( name, "weapon" ))
	{
		return GetActiveWeapon();
	}

	// HACKHACK: FindEntityProcedural can go through this now, so running this code could cause an infinite loop.
	// As a result, FindEntityProcedural currently identifies itself with this entity filter.
	else if (!pFilter || !dynamic_cast<CNullEntityFilter*>(pFilter))
	{
		// search for up to 32 entities with the same name and choose one randomly
		CSharedBaseEntity *entityList[ FINDNAMEDENTITY_MAX_ENTITIES ];
		CSharedBaseEntity *entity;
		int	iCount;

		entity = NULL;
		for( iCount = 0; iCount < FINDNAMEDENTITY_MAX_ENTITIES; iCount++ )
		{
			entity = g_pEntityList->FindEntityByName( entity, szName, this, NULL, NULL, pFilter );
			if ( !entity )
			{
				break;
			}
			entityList[ iCount ] = entity;
		}

		if ( iCount > 0 )
		{
			int index = RandomInt( 0, iCount - 1 );
			entity = entityList[ index ];
			return entity;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: return true if given target cant be seen because of fog
//-----------------------------------------------------------------------------
bool CSharedBaseCombatCharacter::IsHiddenByFog( const Vector &target ) const
{
	float range = EyePosition().DistTo( target );
	return IsHiddenByFog( range );
}

//-----------------------------------------------------------------------------
// Purpose: return true if given target cant be seen because of fog
//-----------------------------------------------------------------------------
bool CSharedBaseCombatCharacter::IsHiddenByFog( CSharedBaseEntity *target ) const
{
	if ( !target )
		return false;

	float range = EyePosition().DistTo( target->WorldSpaceCenter() );
	return IsHiddenByFog( range );
}

//-----------------------------------------------------------------------------
// Purpose: return true if given target cant be seen because of fog
//-----------------------------------------------------------------------------
bool CSharedBaseCombatCharacter::IsHiddenByFog( float range ) const
{
	if ( GetFogObscuredRatio( range ) >= 1.0f )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: return 0-1 ratio where zero is not obscured, and 1 is completely obscured
//-----------------------------------------------------------------------------
float CSharedBaseCombatCharacter::GetFogObscuredRatio( const Vector &target ) const
{
	float range = EyePosition().DistTo( target );
	return GetFogObscuredRatio( range );
}

//-----------------------------------------------------------------------------
// Purpose: return 0-1 ratio where zero is not obscured, and 1 is completely obscured
//-----------------------------------------------------------------------------
float CSharedBaseCombatCharacter::GetFogObscuredRatio( CSharedBaseEntity *target ) const
{
	if ( !target )
		return false;

	float range = EyePosition().DistTo( target->WorldSpaceCenter() );
	return GetFogObscuredRatio( range );
}

//-----------------------------------------------------------------------------
// Purpose: return 0-1 ratio where zero is not obscured, and 1 is completely obscured
//-----------------------------------------------------------------------------
float CSharedBaseCombatCharacter::GetFogObscuredRatio( float range ) const
{
	fogparams_t fog;
	GetFogParams( &fog );

	if ( !fog.enable )
		return 0.0f;

	if ( range <= fog.start )
		return 0.0f;

	if ( range >= fog.end )
		return 1.0f;

	float ratio = (range - fog.start) / (fog.end - fog.start);
	ratio = MIN( ratio, fog.maxdensity.Get() );
	return ratio;
}

bool CSharedBaseCombatCharacter::GetFogParams( fogparams_t *fog ) const
{
	if ( !fog )
		return false;

	return GetWorldFogParams( const_cast< CSharedBaseCombatCharacter * >( this ), *fog );
}

//-----------------------------------------------------------------------------
// Purpose: track the last trigger_fog touched by this character
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::OnFogTriggerStartTouch( CSharedBaseEntity *fogTrigger )
{
	m_hTriggerFogList.AddToHead( fogTrigger );
}

//-----------------------------------------------------------------------------
// Purpose: track the last trigger_fog touched by this character
//-----------------------------------------------------------------------------
void CSharedBaseCombatCharacter::OnFogTriggerEndTouch( CSharedBaseEntity *fogTrigger )
{
	m_hTriggerFogList.FindAndRemove( fogTrigger );
}

//-----------------------------------------------------------------------------
// Purpose: track the last trigger_fog touched by this character
//-----------------------------------------------------------------------------
CSharedBaseEntity *CSharedBaseCombatCharacter::GetFogTrigger( void )
{
	float bestDist = 999999.0f;
	CSharedBaseEntity *bestTrigger = NULL;

	for ( int i=0; i<m_hTriggerFogList.Count(); ++i )
	{
		CSharedBaseEntity *fogTrigger = m_hTriggerFogList[i];
		if ( fogTrigger != NULL )
		{
			float dist = WorldSpaceCenter().DistTo( fogTrigger->WorldSpaceCenter() );
			if ( dist < bestDist )
			{
				bestDist = dist;
				bestTrigger = fogTrigger;
			}
		}
	}

	if ( bestTrigger )
	{
		m_hLastFogTrigger = bestTrigger;
	}

	return m_hLastFogTrigger;
}
