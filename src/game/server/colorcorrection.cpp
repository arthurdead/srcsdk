//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
// $NoKeywords: $
//===========================================================================//

#include <string.h>

#include "cbase.h"
#include "colorcorrection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define COLOR_CORRECTION_ENT_THINK_RATE TICK_INTERVAL

static const char *s_pFadeInContextThink = "ColorCorrectionFadeInThink";
static const char *s_pFadeOutContextThink = "ColorCorrectionFadeOutThink";
 
//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(color_correction, CColorCorrection);

BEGIN_MAPENTITY( CColorCorrection )

	DEFINE_KEYFIELD_AUTO( m_MinFalloff, "minfalloff" ),
	DEFINE_KEYFIELD_AUTO( m_MaxFalloff, "maxfalloff" ),
	DEFINE_KEYFIELD_AUTO( m_flMaxWeight, "maxweight" ),
	DEFINE_KEYFIELD_AUTO( m_flFadeInDuration, "fadeInDuration" ),
	DEFINE_KEYFIELD_AUTO( m_flFadeOutDuration, "fadeOutDuration" ),
	DEFINE_KEYFIELD_AUTO( m_lookupFilename, "filename" ),

	DEFINE_KEYFIELD_AUTO( m_bEnabled, "enabled" ),
	DEFINE_KEYFIELD_AUTO( m_bStartDisabled, "StartDisabled" ),
	DEFINE_KEYFIELD_AUTO( m_bExclusive, "exclusive" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeInDuration", InputSetFadeInDuration ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFadeOutDuration", InputSetFadeOutDuration ),

END_MAPENTITY()

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
IMPLEMENT_SERVERCLASS_ST_NOBASE(CColorCorrection, DT_ColorCorrection)
	SendPropVector( SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat(  SENDINFO(m_MinFalloff) ),
	SendPropFloat(  SENDINFO(m_MaxFalloff) ),
	SendPropFloat(  SENDINFO(m_flCurWeight) ),
	SendPropFloat(  SENDINFO(m_flMaxWeight) ),
	SendPropFloat(  SENDINFO(m_flFadeInDuration) ),
	SendPropFloat(  SENDINFO(m_flFadeOutDuration) ),
	SendPropString( SENDINFO(m_netlookupFilename) ),
	SendPropBool( SENDINFO(m_bEnabled) ),
	SendPropBool( SENDINFO(m_bMaster) ),
	SendPropBool( SENDINFO(m_bClientSide) ),
	SendPropBool( SENDINFO(m_bExclusive) ),
END_SEND_TABLE()


CColorCorrection::CColorCorrection() : BaseClass()
{
	m_bEnabled = true;
	m_MinFalloff = 0.0f;
	m_MaxFalloff = 1000.0f;
	m_flMaxWeight = 1.0f;
	m_flCurWeight.Set( 0.0f );
	m_flFadeInDuration = 0.0f;
	m_flFadeOutDuration = 0.0f;
	m_flStartFadeInWeight = 0.0f;
	m_flStartFadeOutWeight = 0.0f;
	m_flTimeStartFadeIn = 0.0f;
	m_flTimeStartFadeOut = 0.0f;
	m_netlookupFilename.GetForModify()[0] = 0;
	m_bMaster = false;
	m_bClientSide = false;
	m_bExclusive = false;
	m_lookupFilename = NULL_STRING;
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
EdictStateFlags_t CColorCorrection::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CColorCorrection::Spawn( void )
{
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT | EFL_DIRTY_ABSTRANSFORM );
	Precache();
	SetSolid( SOLID_NONE );

	// To fade in/out the weight.
	SetContextThink( &CColorCorrection::FadeInThink, TICK_NEVER_THINK, s_pFadeInContextThink );
	SetContextThink( &CColorCorrection::FadeOutThink, TICK_NEVER_THINK, s_pFadeOutContextThink );
	
	if( m_bStartDisabled )
	{
		m_bEnabled = false;
		m_flCurWeight.Set ( 0.0f );
	}
	else
	{
		m_bEnabled = true;
		m_flCurWeight.Set ( 1.0f );
	}

	m_bMaster = IsMaster();
	m_bClientSide = IsClientSide();

	BaseClass::Spawn();
}

void CColorCorrection::Activate( void )
{
	BaseClass::Activate();

	Q_strncpy( m_netlookupFilename.GetForModify(), STRING( m_lookupFilename ), MAX_PATH );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up internal vars needed for fade in lerping
//-----------------------------------------------------------------------------
void CColorCorrection::FadeIn ( void )
{
	if ( m_bClientSide || ( m_bEnabled && m_flCurWeight >= m_flMaxWeight ) )
		return;

	m_bEnabled = true;
	m_flTimeStartFadeIn = gpGlobals->curtime;
	m_flStartFadeInWeight = m_flCurWeight;
	SetNextThink ( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeInContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up internal vars needed for fade out lerping
//-----------------------------------------------------------------------------
void CColorCorrection::FadeOut ( void )
{
	if ( m_bClientSide || ( !m_bEnabled && m_flCurWeight <= 0.0f ) )
		return;

	m_bEnabled = false;
	m_flTimeStartFadeOut = gpGlobals->curtime;
	m_flStartFadeOutWeight = m_flCurWeight;
	SetNextThink ( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeOutContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Fades lookup weight from CurWeight->MaxWeight
//-----------------------------------------------------------------------------
void CColorCorrection::FadeInThink( void )
{
	// Check for conditions where we shouldnt fade in
	if (		m_flFadeInDuration <= 0 ||  // not set to fade in
		 m_flCurWeight >= m_flMaxWeight ||  // already past max weight
							!m_bEnabled ||  // fade in/out mutex
				  m_flMaxWeight == 0.0f ||  // min==max
  m_flStartFadeInWeight >= m_flMaxWeight )  // already at max weight
	{
		SetNextThink ( TICK_NEVER_THINK, s_pFadeInContextThink );
		return;
	}
	
	// If we started fading in without fully fading out, use a truncated duration
    float flTimeToFade = m_flFadeInDuration;
	if ( m_flStartFadeInWeight > 0.0f )
	{	
		float flWeightRatio		= m_flStartFadeInWeight / m_flMaxWeight;
		flWeightRatio = clamp ( flWeightRatio, 0.0f, 0.99f );
		flTimeToFade			= m_flFadeInDuration * (1.0 - flWeightRatio);
	}	
	
	Assert ( flTimeToFade > 0.0f );
	float flFadeRatio = (gpGlobals->curtime - m_flTimeStartFadeIn) / flTimeToFade;
	flFadeRatio = clamp ( flFadeRatio, 0.0f, 1.0f );
	m_flStartFadeInWeight = clamp ( m_flStartFadeInWeight, 0.0f, 1.0f );

	m_flCurWeight = Lerp( flFadeRatio, m_flStartFadeInWeight, m_flMaxWeight.Get() );

	SetNextThink( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeInContextThink );
}

//-----------------------------------------------------------------------------
// Purpose: Fades lookup weight from CurWeight->0.0 
//-----------------------------------------------------------------------------
void CColorCorrection::FadeOutThink( void )
{
	// Check for conditions where we shouldn't fade out
	if ( m_flFadeOutDuration <= 0 || // not set to fade out
			m_flCurWeight <= 0.0f || // already faded out
					   m_bEnabled || // fade in/out mutex
		   m_flMaxWeight == 0.0f  || // min==max
	 m_flStartFadeOutWeight <= 0.0f )// already at min weight
	{
		SetNextThink ( TICK_NEVER_THINK, s_pFadeOutContextThink );
		return;
	}

	// If we started fading out without fully fading in, use a truncated duration
    float flTimeToFade = m_flFadeOutDuration;
	if ( m_flStartFadeOutWeight < m_flMaxWeight )
	{	
		float flWeightRatio		= m_flStartFadeOutWeight / m_flMaxWeight;
		flWeightRatio = clamp ( flWeightRatio, 0.01f, 1.0f );
		flTimeToFade			= m_flFadeOutDuration * flWeightRatio;
	}	
	
	Assert ( flTimeToFade > 0.0f );
	float flFadeRatio = (gpGlobals->curtime - m_flTimeStartFadeOut) / flTimeToFade;
	flFadeRatio = clamp ( flFadeRatio, 0.0f, 1.0f );
	m_flStartFadeOutWeight = clamp ( m_flStartFadeOutWeight, 0.0f, 1.0f );

	m_flCurWeight = Lerp( 1.0f - flFadeRatio, 0.0f, m_flStartFadeOutWeight );

	SetNextThink( gpGlobals->curtime + COLOR_CORRECTION_ENT_THINK_RATE, s_pFadeOutContextThink );
}

//------------------------------------------------------------------------------
// Purpose : Input handlers
//------------------------------------------------------------------------------
void CColorCorrection::InputEnable( inputdata_t &&inputdata )
{
	m_bEnabled = true;

	if ( m_flFadeInDuration > 0.0f )
	{
		FadeIn();
	}
	else
	{
		m_flCurWeight = m_flMaxWeight;
	}
	
}

void CColorCorrection::InputDisable( inputdata_t &&inputdata )
{
	m_bEnabled = false;

	if ( m_flFadeOutDuration > 0.0f )
	{
		FadeOut();
	}
	else
	{
		m_flCurWeight = 0.0f;
	}
	
}

void CColorCorrection::InputSetFadeInDuration( inputdata_t &&inputdata )
{
	m_flFadeInDuration = inputdata.value.Float();
}

void CColorCorrection::InputSetFadeOutDuration( inputdata_t &&inputdata )
{
	m_flFadeOutDuration = inputdata.value.Float();
}

CColorCorrectionSystem s_ColorCorrectionSystem( "ColorCorrectionSystem" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CColorCorrectionSystem *ColorCorrectionSystem( void )
{
	return &s_ColorCorrectionSystem;
}


//-----------------------------------------------------------------------------
// Purpose: Clear out the fog controller.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::LevelInitPreEntity( void )
{
	m_hMasterController = NULL;
	ListenForGameEvent( "round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: Find the master controller.  If no controller is 
//			set as Master, use the first controller found.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::InitMasterController( void )
{
	CColorCorrection *pColorCorrection = NULL;
	do
	{
		pColorCorrection = dynamic_cast<CColorCorrection*>( gEntList.FindEntityByClassname( pColorCorrection, "color_correction" ) );
		if ( pColorCorrection )
		{
			if ( m_hMasterController.Get() == NULL )
			{
				m_hMasterController = pColorCorrection;
			}
			else
			{
				if ( pColorCorrection->IsMaster() )
				{
					m_hMasterController = pColorCorrection;
				}
			}
		}
	} while ( pColorCorrection );
}

//-----------------------------------------------------------------------------
// Purpose: On a multiplayer map restart, re-find the master controller.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::FireGameEvent( IGameEvent *pEvent )
{
	InitMasterController();
}

//-----------------------------------------------------------------------------
// Purpose: On level load find the master fog controller.  If no controller is 
//			set as Master, use the first fog controller found.
//-----------------------------------------------------------------------------
void CColorCorrectionSystem::LevelInitPostEntity( void )
{
	InitMasterController();
}
