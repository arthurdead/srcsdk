//========= Copyright Valve Corporation, All rights reserved. ============//
//
// An entity that allows level designer control over the fog parameters.
//
//=============================================================================

#include "cbase.h"
#include "c_env_fog_controller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( FogController, DT_FogController )

//-----------------------------------------------------------------------------
// Datatable
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( C_FogController, DT_FogController )
	// fog data
	RecvPropInt( RECVINFO_STRUCTELEM( m_fog, enable ) ),
	RecvPropInt( RECVINFO_STRUCTELEM( m_fog, blend ) ),
	RecvPropVector( RECVINFO_STRUCTELEM( m_fog, dirPrimary ) ),
	RecvPropColor32( RECVINFO_STRUCTELEM( m_fog, colorPrimary ) ),
	RecvPropColor32( RECVINFO_STRUCTELEM( m_fog, colorSecondary ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, start ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, end ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, farz ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, maxdensity ) ),

	RecvPropColor32( RECVINFO_STRUCTELEM( m_fog, colorPrimaryLerpTo ) ),
	RecvPropColor32( RECVINFO_STRUCTELEM( m_fog, colorSecondaryLerpTo ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, startLerpTo ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, endLerpTo ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, maxdensityLerpTo ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, lerptime ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, duration ) ),
	RecvPropFloat( RECVINFO_STRUCTELEM( m_fog, HDRColorScale ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_FogController::C_FogController()
{
	// Make sure that old maps without fog fields don't get wacked out fog values.
	m_fog.enable = false;
	m_fog.maxdensity = 1.0f;
	m_fog.HDRColorScale = 1.f;
}

bool GetWorldFogParams( C_BaseCombatCharacter *character, fogparams_t &fog )
{
	return false;
}
