//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "entityoutput.h"
//#include "convar.h"
#include "env_dof_controller.h"
#include "ai_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_dof_controller, CEnvDOFController );

BEGIN_MAPENTITY( CEnvDOFController )
	
	DEFINE_KEYFIELD_AUTO( m_bDOFEnabled, "enabled" ),
	DEFINE_KEYFIELD_AUTO( m_flNearBlurDepth, "near_blur" ),
	DEFINE_KEYFIELD_AUTO( m_flNearFocusDepth, "near_focus" ),
	DEFINE_KEYFIELD_AUTO( m_flFarFocusDepth, "far_focus" ),
	DEFINE_KEYFIELD_AUTO( m_flFarBlurDepth, "far_blur" ),
	DEFINE_KEYFIELD_AUTO( m_flNearBlurRadius, "near_radius" ),
	DEFINE_KEYFIELD_AUTO( m_flFarBlurRadius, "far_radius" ),
	DEFINE_KEYFIELD_AUTO( m_strFocusTargetName, "focus_target" ),
	DEFINE_KEYFIELD_AUTO( m_flFocusTargetRange, "focus_range" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearBlurDepth",		InputSetNearBlurDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearFocusDepth",	InputSetNearFocusDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarFocusDepth",		InputSetFarFocusDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarBlurDepth",		InputSetFarBlurDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearBlurRadius",	InputSetNearBlurRadius ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarBlurRadius",		InputSetFarBlurRadius ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetFocusTarget",		InputSetFocusTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFocusTargetRange",	InputSetFocusTargetRange ),

END_MAPENTITY()

IMPLEMENT_SERVERCLASS_ST( CEnvDOFController, DT_EnvDOFController )
	SendPropInt( SENDINFO(m_bDOFEnabled), 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flNearBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flNearFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flNearBlurRadius), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarBlurRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::Activate( void )
{
	BaseClass::Activate();

	// Find our target entity and hold on to it
	m_hFocusTarget = gEntList.FindEntityByName( NULL, m_strFocusTargetName );

	// Update if we have a focal target
	if ( m_hFocusTarget )
	{
		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EdictStateFlags_t CEnvDOFController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetNearBlurDepth( inputdata_t &&inputdata )
{
	m_flNearBlurDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetNearFocusDepth( inputdata_t &&inputdata )
{
	m_flNearFocusDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFarFocusDepth( inputdata_t &&inputdata )
{
	m_flFarFocusDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFarBlurDepth( inputdata_t &&inputdata )
{
	m_flFarBlurDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetNearBlurRadius( inputdata_t &&inputdata )
{
	m_flNearBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFarBlurRadius( inputdata_t &&inputdata )
{
	m_flFarBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvDOFController::SetControllerState( DOFControlSettings_t setting )
{
	m_flNearBlurDepth = setting.flNearBlurDepth;
	m_flNearBlurRadius = setting.flNearBlurRadius;
	m_flNearFocusDepth = setting.flNearFocusDistance;
	
	m_flFarBlurDepth = setting.flFarBlurDepth;
	m_flFarBlurRadius = setting.flFarBlurRadius;
	m_flFarFocusDepth = setting.flFarFocusDistance;

	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

const float BLUR_DEPTH = 500.0f;

//-----------------------------------------------------------------------------
// Purpose: Blend the parameters to the specified value
//-----------------------------------------------------------------------------
void CEnvDOFController::UpdateParamBlend( void )
{ 
	// Update our focal target if we have one
	if ( m_hFocusTarget )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		float flDistToFocus = ( m_hFocusTarget->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length();
		m_flFarFocusDepth.GetForModify() = flDistToFocus + m_flFocusTargetRange;
		m_flFarBlurDepth.GetForModify() = m_flFarFocusDepth + BLUR_DEPTH;
	}

	SetThink( &CEnvDOFController::UpdateParamBlend );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Set the "focus" target entity
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFocusTarget( inputdata_t &&inputdata )
{
	m_hFocusTarget = gEntList.FindEntityByName( NULL, inputdata.value.String() );
	
	// Update if we have a focal target
	if ( m_hFocusTarget )
	{
		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the range behind the focus entity that we'll blur (in units)
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFocusTargetRange( inputdata_t &&inputdata )
{
	m_flFocusTargetRange = inputdata.value.Float();
}
