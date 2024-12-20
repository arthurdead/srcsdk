//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A special kind of beam effect that traces from its start position to
//			its end position and stops if it hits anything.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "EnvLaser.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_laser, CEnvLaser );

BEGIN_MAPENTITY( CEnvLaser )

	DEFINE_KEYFIELD_AUTO( m_iszLaserTarget, "LaserTarget" ),
	DEFINE_KEYFIELD_AUTO( m_iszSpriteName, "EndSprite" ),
	DEFINE_KEYFIELD_AUTO( m_flStartFrame, "framestart" ),

	// Input functions
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	DEFINE_OUTPUT( m_OnTouchedByEntity, "OnTouchedByEntity" ),

END_MAPENTITY()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::Spawn( void )
{
	if ( !GetModelName() )
	{
		SetThink( &CEnvLaser::SUB_Remove );
		return;
	}

	SetSolid( SOLID_NONE );							// Remove model & collisions
	SetThink( &CEnvLaser::StrikeThink );

	SetEndWidth( GetWidth() );				// Note: EndWidth is not scaled

	PointsInit( GetLocalOrigin(), GetLocalOrigin() );

	Precache( );

	if ( !m_pSprite && m_iszSpriteName != NULL_STRING )
	{
		m_pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteName), GetAbsOrigin(), TRUE );
	}
	else
	{
		m_pSprite = NULL;
	}

	if ( m_pSprite )
	{
		m_pSprite->SetParent( GetMoveParent() );
		m_pSprite->SetTransparency( kRenderGlow, GetRenderColorR(), GetRenderColorG(), GetRenderColorB(), GetRenderAlpha(), GetRenderFX() );
	}

	if ( GetEntityName() != NULL_STRING && !(m_spawnflags & SF_BEAM_STARTON) )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::Precache( void )
{
	SetModelIndex( PrecacheModel( STRING( GetModelName() ) ) );
	if ( m_iszSpriteName != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteName) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEnvLaser::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "width"))
	{
		SetWidth( atof(szValue) );
	}
	else if (FStrEq(szKeyName, "NoiseAmplitude"))
	{
		SetNoise( atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "TextureScroll"))
	{
		SetScrollRate( atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "texture"))
	{
		SetModelName( AllocPooledString(szValue) );
	}
	else
	{
		BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether the laser is currently active.
//-----------------------------------------------------------------------------
int CEnvLaser::IsOn( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return 0;
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::InputTurnOn( inputdata_t &&inputdata )
{
	if (!IsOn())
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::InputTurnOff( inputdata_t &&inputdata )
{
	if (IsOn())
	{
		TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::InputToggle( inputdata_t &&inputdata )
{
	if ( IsOn() )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::TurnOff( void )
{
	AddEffects( EF_NODRAW );
	if ( m_pSprite )
		m_pSprite->TurnOff();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::TurnOn( void )
{
	RemoveEffects( EF_NODRAW );
	if ( m_pSprite )
		m_pSprite->TurnOn();

	m_flFireTime = gpGlobals->curtime;

	SetThink( &CEnvLaser::StrikeThink );

	//
	// Call StrikeThink here to update the end position, otherwise we will see
	// the beam in the wrong place for one frame since we cleared the nodraw flag.
	//
	StrikeThink();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::FireAtPoint( trace_t &tr )
{
	SetAbsEndPos( tr.endpos );
	if ( m_pSprite )
	{
		UTIL_SetOrigin( m_pSprite, tr.endpos );
	}

	// Apply damage and do sparks every 1/10th of a second.
	if ( gpGlobals->curtime >= m_flFireTime + 0.1 )
	{
		if ( tr.fraction != 1.0 && tr.m_pEnt && !tr.m_pEnt->IsWorld() )
		{
			m_OnTouchedByEntity.FireOutput( tr.m_pEnt, this );
		}
		
		BeamDamage( &tr );
		DoSparks( GetAbsStartPos(), tr.endpos );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvLaser::StrikeThink( void )
{
	CBaseEntity *pEnd = RandomTargetname( STRING( m_iszLaserTarget ) );

	Vector vecFireAt = GetAbsEndPos();
	if ( pEnd )
	{
		vecFireAt = pEnd->GetAbsOrigin();
	}

	trace_t tr;

	UTIL_TraceLine( GetAbsOrigin(), vecFireAt, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	FireAtPoint( tr );
	SetNextThink( gpGlobals->curtime );
}


