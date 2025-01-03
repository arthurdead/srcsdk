//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "cbase.h"
#include "baseparticleentity.h"

#ifdef CLIENT_DLL
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( BaseParticleEntity, DT_BaseParticleEntity )

BEGIN_NETWORK_TABLE( CSharedBaseParticleEntity, DT_BaseParticleEntity )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(	C_BaseParticleEntity )
END_PREDICTION_DATA()

REGISTER_EFFECT( C_BaseParticleEntity );
#endif

#if defined( CLIENT_DLL )
	#define CBaseParticleEntity C_BaseParticleEntity
#endif

CSharedBaseParticleEntity::CBaseParticleEntity( void )
{
#if defined( CLIENT_DLL )
	m_bSimulate = true;
	m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID;
#endif
}

CSharedBaseParticleEntity::~CBaseParticleEntity( void )
{
#if defined( CLIENT_DLL )
	if ( ToolsEnabled() && ( m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID ) && clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "ParticleSystem_Destroy" );
		msg->SetInt( "id", m_nToolParticleEffectId );
		m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID; 
	}
#endif
}

#if defined( CLIENT_DLL )
	#undef CBaseParticleEntity
#endif

#if !defined( CLIENT_DLL )
int CSharedBaseParticleEntity::UpdateTransmitState( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return SetTransmitState( FL_EDICT_DONTSEND );

	if ( IsEFlagSet( EFL_IN_SKYBOX ) )
		return SetTransmitState( FL_EDICT_ALWAYS );

	// cull against PVS
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

void CSharedBaseParticleEntity::Activate()
{
#if !defined( CLIENT_DLL )
	BaseClass::Activate();
#endif
}	


void CSharedBaseParticleEntity::Think()
{
	UTIL_Remove( this );
}


void CSharedBaseParticleEntity::FollowEntity(CSharedBaseEntity *pEntity)
{
	BaseClass::FollowEntity( pEntity );
	SetLocalOrigin( vec3_origin );
}


void CSharedBaseParticleEntity::SetLifetime(float lifetime)
{
	if(lifetime == -1)
		SetNextThink( TICK_NEVER_THINK );
	else
		SetNextThink( gpGlobals->curtime + lifetime );
}

#if defined( CLIENT_DLL )
const Vector &CSharedBaseParticleEntity::GetSortOrigin()
{
	// By default, we do the cheaper behavior of getting the root parent's abs origin, so we don't have to
	// setup any bones along the way. If this screws anything up, we can always make it an option.
	return GetRootMoveParent()->GetAbsOrigin();
}

void CSharedBaseParticleEntity::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	// If you derive from CBaseParticleEntity, you must implement simulation and rendering.
	Assert( false );
}

void CSharedBaseParticleEntity::RenderParticles( CParticleRenderIterator *pIterator )
{
	// If you derive from CBaseParticleEntity, you must implement simulation and rendering.
	Assert( false );
}

#endif
