//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "decals.h"
#include "basegrenade_shared.h"
#include "shake.h"
#include "engine/IEngineSound.h"

#if !defined( CLIENT_DLL )

#include "soundent.h"
#include "entitylist.h"
#include "gamestats.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion
extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

#if !defined( CLIENT_DLL )

// Global Savedata for friction modifier
BEGIN_MAPENTITY( CBaseGrenade )

	DEFINE_KEYFIELD( m_DmgRadius, FIELD_FLOAT, "Radius" ),

	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDamage", InputSetDamage ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Detonate", InputDetonate ),

	DEFINE_OUTPUT( m_OnDetonate, "OnDetonate" ),
	DEFINE_OUTPUT( m_OnDetonate_OutPosition, "OnDetonate_OutPosition" ),

END_MAPENTITY()

void SendProxy_CropFlagsToPlayerFlagBitsLength( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( BaseGrenade, DT_BaseGrenade )

BEGIN_NETWORK_TABLE( CSharedBaseGrenade, DT_BaseGrenade )
#if !defined( CLIENT_DLL )
	// Excludes
	SendPropExclude( SENDEXLCUDE( DT_BaseEntity, m_angRotation ) ),
	SendPropExclude( SENDEXLCUDE( DT_BaseEntity, m_hOwnerEntity ) ), // Already got m_hThrower
	SendPropExclude( SENDEXLCUDE( DT_ServerAnimationData, m_flCycle ) ),	
	SendPropExclude( SENDEXLCUDE( DT_AnimTimeMustBeFirst, m_flAnimTime ) ),

	SendPropFloat( SENDINFO( m_flDamage ), 10, SPROP_ROUNDDOWN, 0.0, 256.0f ),
	SendPropFloat( SENDINFO( m_DmgRadius ), 10, SPROP_ROUNDDOWN, 0.0, 1024.0f ),
	SendPropInt( SENDINFO( m_bIsLive ), 1, SPROP_UNSIGNED ),
//	SendPropTime( SENDINFO( m_flDetonateTime ) ),
	SendPropEHandle( SENDINFO( m_hThrower ) ),

	// Resend smaller
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesX ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesY ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 10, SPROP_CHANGES_OFTEN, CBaseEntity::SendProxy_AnglesZ ),
#else
	RecvPropFloat( RECVINFO( m_flDamage ) ),
	RecvPropFloat( RECVINFO( m_DmgRadius ) ),
	RecvPropInt( RECVINFO( m_bIsLive ) ),
//	RecvPropTime( RECVINFO( m_flDetonateTime ) ),
	RecvPropEHandle( RECVINFO( m_hThrower ) ),

	RecvPropFloat( RECVINFO_VECTORELEM_NAME_VECTORELEM( m_angNetworkAngles, 0, m_angRotation, 0 ) ),
	RecvPropFloat( RECVINFO_VECTORELEM_NAME_VECTORELEM( m_angNetworkAngles, 1, m_angRotation, 1 ) ),
	RecvPropFloat( RECVINFO_VECTORELEM_NAME_VECTORELEM( m_angNetworkAngles, 2, m_angRotation, 2 ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_SERVERCLASS( grenade, CBaseGrenade );

#if defined( CLIENT_DLL )

BEGIN_PREDICTION_DATA( C_BaseGrenade  )

	DEFINE_FIELD_FLAGS( m_hThrower, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_bIsLive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_DmgRadius, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
//	DEFINE_FIELD_FLAGS_TOL( m_flDetonateTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_FIELD_FLAGS( m_flDamage, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD_FLAGS_TOL( m_vecVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_FIELD_FLAGS_TOL( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),

//	DEFINE_FIELD( m_fRegisteredSound, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_iszBounceSound, FIELD_STRING ),

END_PREDICTION_DATA()

#endif

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CSharedBaseGrenade::Explode( trace_t *pTrace, int bitsDamageType )
{
#if !defined( CLIENT_DLL )
	
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + (pTrace->plane.normal * 0.6) );
	}

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin, MASK_WATER );

	// Since this code only runs on the server, make sure it shows the tempents it creates.
	// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
	CDisablePredictionFiltering disabler;

	if ( pTrace->fraction != 1.0 )
	{
		Vector vecNormal = pTrace->plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace->surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );

		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE|TE_EXPLFLAG_DLIGHT,
			m_DmgRadius,
			m_flDamage,
			&vecNormal,
			(char) pdata->game.material );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE|TE_EXPLFLAG_DLIGHT,
			m_DmgRadius,
			m_flDamage );
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
#endif

	// Use the thrower's position as the reported position
	Vector vecReported = m_hThrower ? m_hThrower->GetAbsOrigin() : vec3_origin;
	
	CTakeDamageInfo info( this, m_hThrower, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );
	//info.SetForceFriendlyFire( m_bForceFriendlyFire );

	RadiusDamage( info, GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );

	UTIL_DecalTrace( pTrace, "Scorch" );

	EmitSound( "BaseGrenade.Explode" );

	SetThink( &CSharedBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	SetSolid( SOLID_NONE );
	
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );

	// Because the grenade is zipped out of the world instantly, the EXPLOSION sound that it makes for
	// the AI is also immediately destroyed. For this reason, we now make the grenade entity inert and
	// throw it away in 1/10th of a second instead of right away. Removing the grenade instantly causes
	// intermittent bugs with env_microphones who are listening for explosions. They will 'randomly' not
	// hear explosion sounds when the grenade is removed and the SoundEnt thinks (and removes the sound)
	// before the env_microphone thinks and hears the sound.
	SetNextThink( gpGlobals->curtime + 0.1 );

#if defined( HL2_DLL )
	CBasePlayer *pPlayer = ToBasePlayer( m_hThrower.Get() );
	if ( pPlayer )
	{
		gamestats->Event_WeaponHit( pPlayer, true, "weapon_frag", info );
	}
#endif

#endif
}


void CSharedBaseGrenade::Smoke( void )
{
	Vector vecAbsOrigin = GetAbsOrigin();
	if ( UTIL_PointContents ( vecAbsOrigin, MASK_WATER ) & MASK_WATER )
	{
		UTIL_Bubbles( vecAbsOrigin - Vector( 64, 64, 64 ), vecAbsOrigin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		CPVSFilter filter( vecAbsOrigin );

		te->Smoke( filter, 0.0, 
			&vecAbsOrigin, g_sModelIndexSmoke,
			m_DmgRadius * 0.03,
			24 );
	}
#if !defined( CLIENT_DLL )
	SetThink ( &CSharedBaseGrenade::SUB_Remove );
#endif
	SetNextThink( gpGlobals->curtime );
}

void CSharedBaseGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseGrenade::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Support player pickup
	if ( useType == USE_TOGGLE )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
			return;
		}
	}

	// Pass up so we still call any custom Use function
	BaseClass::Use( pActivator, pCaller, useType, value );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Timed grenade, this think is called when time runs out.
//-----------------------------------------------------------------------------
void CSharedBaseGrenade::DetonateUse( CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CSharedBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime );
}

void CSharedBaseGrenade::PreDetonate( void )
{
#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this );
#endif

	SetThink( &CSharedBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime + 1.5 );
}


void CSharedBaseGrenade::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	if( tr.startsolid )
	{
		// Since we blindly moved the explosion origin vertically, we may have inadvertently moved the explosion into a solid,
		// in which case nothing is going to be harmed by the grenade's explosion because all subsequent traces will startsolid.
		// If this is the case, we do the downward trace again from the actual origin of the grenade. (sjb) 3/8/2007  (for ep2_outland_09)
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	}

	Explode( &tr, DMG_BLAST );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}


//
// Contact grenade, explode when it touches something
// 
void CSharedBaseGrenade::ExplodeTouch( CSharedBaseEntity *pOther )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	Explode( &tr, DMG_BLAST );
}


void CSharedBaseGrenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length( ), 0.2, this );
#endif

	SetNextThink( gpGlobals->curtime + 0.2 );

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
	}
}


void CSharedBaseGrenade::BounceTouch( CSharedBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() )
		return;

	// only do damage if we're moving fairly fast
	if ( (pOther->m_takedamage != DAMAGE_NO) && (m_flNextAttack < gpGlobals->curtime && GetAbsVelocity().Length() > 100))
	{
		if (m_hThrower)
		{
#if !defined( CLIENT_DLL )
			trace_t tr;
			tr = CBaseEntity::GetTouchTrace( );
			ClearMultiDamage( );
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward, NULL, NULL );
			CTakeDamageInfo info( this, m_hThrower, 1, DMG_CLUB );
			CalculateMeleeDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );
			pOther->DispatchTraceAttack( info, forward, &tr ); 
			ApplyMultiDamage();
#endif
		}
		m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// m_vecAngVelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = GetAbsVelocity(); 
	vecTestVelocity.z *= 0.45;

	if ( !m_bHasWarnedAI && vecTestVelocity.Length() <= 60 )
	{
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), m_flDamage / 0.4, 0.3, this );
#endif
		m_bHasWarnedAI = true;
	}

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.8 );

		// SetSequence( random_valve->RandomInt( 1, 1 ) ); // FIXME: missing tumble animations
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	float playbackrate = GetAbsVelocity().Length() / 200.0;
	if (playbackrate > 1.0)
		playbackrate = 1;
	else if (playbackrate < 0.5)
		playbackrate = 0;
	SetPlaybackRate(playbackrate);
}



void CSharedBaseGrenade::SlideTouch( CSharedBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() )
		return;

	// m_vecAngVelocity = Vector (300, 300, 300);

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.95 );  

		if (GetAbsVelocity().x != 0 || GetAbsVelocity().y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CSharedBaseGrenade::BounceSound( void )
{
	// Doesn't need to do anything anymore! Physics makes the sound.
}

void CSharedBaseGrenade::TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	//
	// Emit a danger sound one second before exploding.
	//
	if (m_flDetonateTime - 1 < gpGlobals->curtime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * (m_flDetonateTime - gpGlobals->curtime), 400, 0.1, this );
#endif
	}

	if (m_flDetonateTime <= gpGlobals->curtime)
	{
		SetThink( &CSharedBaseGrenade::Detonate );
	}

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
		SetPlaybackRate(0.2);
	}
}

void CSharedBaseGrenade::Precache( void )
{
	BaseClass::Precache( );

	PrecacheScriptSound( "BaseGrenade.Explode" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatCharacter
//-----------------------------------------------------------------------------
CSharedBaseCombatCharacter *CSharedBaseGrenade::GetThrower( void )
{
	CSharedBaseCombatCharacter *pResult = ToBaseCombatCharacter( m_hThrower );
	if ( !pResult && GetOwnerEntity() != NULL )
	{
		pResult = ToBaseCombatCharacter( GetOwnerEntity() );
	}
	return pResult;
}

//-----------------------------------------------------------------------------

void CSharedBaseGrenade::SetThrower( CSharedBaseCombatCharacter *pThrower )
{
	m_hThrower = pThrower;

	// if this is the first thrower, set it as the original thrower
	if ( NULL == m_hOriginalThrower )
	{
		m_hOriginalThrower = pThrower;
	}
}

#ifdef CLIENT_DLL
	#define CBaseGrenade C_BaseGrenade
#endif

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CSharedBaseGrenade::~CBaseGrenade(void)
{
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CSharedBaseGrenade::CBaseGrenade(void)
{
	m_hThrower			= NULL;
	m_hOriginalThrower	= NULL;
	m_bIsLive			= false;
	m_DmgRadius			= 100;
	m_flDetonateTime	= 0;
	m_bHasWarnedAI		= false;

	SetSimulatedEveryTick( true );
}

#ifdef CLIENT_DLL
	#undef CBaseGrenade
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseGrenade::InputSetDamage( inputdata_t &inputdata )
{
	SetDamage( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseGrenade::InputDetonate( inputdata_t &inputdata )
{
	Detonate();
}
#endif
