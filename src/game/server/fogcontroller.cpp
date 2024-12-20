//========= Copyright Valve Corporation, All rights reserved. ============//
//
// An entity that allows level designer control over the fog parameters.
//
//=============================================================================

#include "cbase.h"
#include "fogcontroller.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "player.h"
#include "world.h"
#include "ndebugoverlay.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFogSystem s_FogSystem( "FogSystem" );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CFogSystem *FogSystem( void )
{
	return &s_FogSystem;
}

LINK_ENTITY_TO_CLASS( env_fog_controller, CFogController );

BEGIN_MAPENTITY( CFogController )

	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetStartDist",	InputSetStartDist ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetEndDist",	InputSetEndDist ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetMaxDensity",	InputSetMaxDensity ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOn",		InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOff",		InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColor",		InputSetColor ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorSecondary",	InputSetColorSecondary ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetFarZ",		InputSetFarZ ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"SetAngles",	InputSetAngles ),

	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorLerpTo",		InputSetColorLerpTo ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorSecondaryLerpTo",	InputSetColorSecondaryLerpTo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetStartDistLerpTo",	InputSetStartDistLerpTo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetEndDistLerpTo",	InputSetEndDistLerpTo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetMaxDensityLerpTo",	InputSetMaxDensityLerpTo ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"StartFogTransition", InputStartFogTransition ),

	DEFINE_KEYFIELD_AUTO( m_bUseAngles, "use_angles" ),
	DEFINE_KEYFIELD( m_fog.colorPrimary,	FIELD_COLOR32,	"fogcolor" ),
	DEFINE_KEYFIELD( m_fog.colorSecondary,	FIELD_COLOR32,	"fogcolor2" ),
	DEFINE_KEYFIELD( m_fog.dirPrimary,		FIELD_VECTOR,	"fogdir" ),
	DEFINE_KEYFIELD( m_fog.enable,			FIELD_BOOLEAN,	"fogenable" ),
	DEFINE_KEYFIELD( m_fog.blend,			FIELD_BOOLEAN,	"fogblend" ),
	DEFINE_KEYFIELD( m_fog.start,			FIELD_FLOAT,	"fogstart" ),
	DEFINE_KEYFIELD( m_fog.end,				FIELD_FLOAT,	"fogend" ),
	DEFINE_KEYFIELD( m_fog.maxdensity,		FIELD_FLOAT,	"fogmaxdensity" ),
	DEFINE_KEYFIELD( m_fog.farz,			FIELD_FLOAT,	"farz" ),
	DEFINE_KEYFIELD( m_fog.duration,		FIELD_FLOAT,	"foglerptime" ),
	DEFINE_KEYFIELD( m_fog.HDRColorScale,		FIELD_FLOAT,	"HDRColorScale" ),

END_MAPENTITY()

IMPLEMENT_SERVERCLASS_ST_NOBASE( CFogController, DT_FogController )
// fog data
	SendPropInt( SENDINFO_STRUCTELEM( m_fog, enable ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_STRUCTELEM( m_fog, blend ), 1, SPROP_UNSIGNED ),
	SendPropVector( SENDINFO_STRUCTELEM(m_fog, dirPrimary), -1, SPROP_COORD),
	SendPropColor32( SENDINFO_STRUCTELEM( m_fog, colorPrimary ) ),
	SendPropColor32( SENDINFO_STRUCTELEM( m_fog, colorSecondary ) ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, start ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, end ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, maxdensity ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, farz ), 0, SPROP_NOSCALE ),

	SendPropColor32( SENDINFO_STRUCTELEM( m_fog, colorPrimaryLerpTo ) ),
	SendPropColor32( SENDINFO_STRUCTELEM( m_fog, colorSecondaryLerpTo ) ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, startLerpTo ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, endLerpTo ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, maxdensityLerpTo ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, lerptime ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, duration ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_fog, HDRColorScale ), 0, SPROP_NOSCALE ),
END_SEND_TABLE()

CFogController::CFogController()
{
	// Make sure that old maps without fog fields don't get wacked out fog values.
	m_fog.enable = false;
	m_fog.maxdensity = 1.0f;
	m_fog.HDRColorScale = 1.0f;
}


CFogController::~CFogController()
{
}

void CFogController::Spawn( void )
{
	BaseClass::Spawn();

	m_fog.colorPrimaryLerpTo = m_fog.colorPrimary;
	m_fog.colorSecondaryLerpTo = m_fog.colorSecondary;
}

//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CFogController::Activate( ) 
{
	BaseClass::Activate();

	if ( m_bUseAngles )
	{
		AngleVectors( GetAbsAngles(), &m_fog.dirPrimary.GetForModify() );
		m_fog.dirPrimary.GetForModify() *= -1.0f; 
	}	    
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EdictStateFlags_t CFogController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog start distance.
//------------------------------------------------------------------------------
void CFogController::InputSetStartDist( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.start = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog end distance.
//------------------------------------------------------------------------------
void CFogController::InputSetEndDist( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.end = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Input handler for setting the maximum density of the fog. This lets us bring
// the start distance in without the scene fogging too much.
//------------------------------------------------------------------------------
void CFogController::InputSetMaxDensity( inputdata_t &&inputdata )
{
	m_fog.maxdensity = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning on the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOn( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.enable = true;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning off the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOff( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.enable = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the primary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColor( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.colorPrimary = inputdata.value.Color32();
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the secondary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColorSecondary( inputdata_t &&inputdata )
{
	// Get the world entity.
	m_fog.colorSecondary = inputdata.value.Color32();
}

void CFogController::InputSetFarZ( inputdata_t &&inputdata )
{
	m_fog.farz = inputdata.value.Int();
}


//------------------------------------------------------------------------------
// Purpose: Sets the angles to use for the secondary fog direction.
//------------------------------------------------------------------------------
void CFogController::InputSetAngles( inputdata_t &&inputdata )
{
	const char *pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector( angles.Base(), pAngles );

	Vector vTemp;
	AngleVectors( angles, &vTemp );
	SetAbsAngles( angles );

	AngleVectors( GetAbsAngles(), &m_fog.dirPrimary.GetForModify() );
	m_fog.dirPrimary.GetForModify() *= -1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFogController::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"State: %s",(m_fog.enable)?"On":"Off");
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Start: %3.0f",m_fog.start.Get());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"End  : %3.0f",m_fog.end.Get());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 color = m_fog.colorPrimary;
		Q_snprintf(tempstr,sizeof(tempstr),"1) Red  : %i",color.r());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Green: %i",color.g());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Blue : %i",color.b());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color = m_fog.colorSecondary;
		Q_snprintf(tempstr,sizeof(tempstr),"2) Red  : %i",color.r());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Green: %i",color.g());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Blue : %i",color.b());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"HDR Color Scale: %0.3f",m_fog.HDRColorScale.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

#define FOG_CONTROLLER_COLORPRIMARY_LERP		(1 << 0)
#define FOG_CONTROLLER_COLORSECONDARY_LERP		(1 << 1)
#define FOG_CONTROLLER_START_LERP				(1 << 2)
#define FOG_CONTROLLER_END_LERP					(1 << 3)
#define FOG_CONTROLLER_MAXDENSITY_LERP			(1 << 4)

void CFogController::InputSetColorLerpTo( inputdata_t &&inputdata )
{
	m_iChangedVariables |= FOG_CONTROLLER_COLORPRIMARY_LERP;
	m_fog.colorPrimaryLerpTo = data.value.Color32();
}

void CFogController::InputSetColorSecondaryLerpTo( inputdata_t &&inputdata )
{
	m_iChangedVariables |= FOG_CONTROLLER_COLORSECONDARY_LERP;
	m_fog.colorSecondaryLerpTo = data.value.Color32();
}

void CFogController::InputSetStartDistLerpTo( inputdata_t &&inputdata )
{
	m_iChangedVariables |= FOG_CONTROLLER_START_LERP;
	m_fog.startLerpTo = data.value.Float();
}

void CFogController::InputSetEndDistLerpTo( inputdata_t &&inputdata )
{
	m_iChangedVariables |= FOG_CONTROLLER_END_LERP;
	m_fog.endLerpTo = data.value.Float();
}

void CFogController::InputSetMaxDensityLerpTo( inputdata_t &&inputdata )
{
	m_iChangedVariables |= FOG_CONTROLLER_MAXDENSITY_LERP;
	m_fog.maxdensityLerpTo = data.value.Float();
}

void CFogController::InputStartFogTransition( inputdata_t &&inputdata )
{
	SetThink( &CFogController::SetLerpValues );

	m_fog.lerptime = gpGlobals->curtime + m_fog.duration + 0.1;
    SetNextThink( gpGlobals->curtime + m_fog.duration );
}

void CFogController::SetLerpValues( void )
{
	if ( m_iChangedVariables & FOG_CONTROLLER_COLORPRIMARY_LERP )
	{
		m_fog.colorPrimary = m_fog.colorPrimaryLerpTo;
	}

	if ( m_iChangedVariables & FOG_CONTROLLER_COLORSECONDARY_LERP )
	{
		m_fog.colorSecondary = m_fog.colorSecondaryLerpTo;
	} 

	if ( m_iChangedVariables & FOG_CONTROLLER_START_LERP )
	{
		m_fog.start = m_fog.startLerpTo;
	}

	if ( m_iChangedVariables & FOG_CONTROLLER_END_LERP )
	{
		m_fog.end = m_fog.endLerpTo;
	}

	if ( m_iChangedVariables & FOG_CONTROLLER_MAXDENSITY_LERP )
	{
		m_fog.maxdensity = m_fog.maxdensityLerpTo;
	}

	m_iChangedVariables = 0;
	m_fog.lerptime = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: Clear out the fog controller.
//-----------------------------------------------------------------------------
void CFogSystem::LevelInitPreEntity( void )
{
	m_hMasterController = NULL;
	ListenForGameEvent( "round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: On level load find the master fog controller.  If no controller is 
//			set as Master, use the first fog controller found.
//-----------------------------------------------------------------------------
void CFogSystem::LevelInitPostEntity( void )
{
	InitMasterController();
}

//-----------------------------------------------------------------------------
// Purpose: Find the master controller.  If no controller is 
//			set as Master, use the first controller found.
//-----------------------------------------------------------------------------
void CFogSystem::InitMasterController( void )
{
	CFogController *pFogController = NULL;
	do
	{
		pFogController = static_cast<CFogController*>( gEntList.FindEntityByClassname( pFogController, "env_fog_controller" ) );
		if ( pFogController )
		{
			if ( m_hMasterController.Get() == NULL )
			{
				m_hMasterController = pFogController;
			}
			else
			{
				if ( pFogController->IsMaster() )
				{
					m_hMasterController = pFogController;
				}
			}
		}
	} while ( pFogController );

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && ( pPlayer->m_Local.m_PlayerFog.m_hCtrl.Get() == NULL ) )
		{
			pPlayer->InitFogController();
		}
	}
}

class CFogTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFogTrigger, CBaseTrigger );
	DECLARE_MAPENTITY();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *other );
	virtual void EndTouch( CBaseEntity *other );

	fogparams_t *GetFog( void )
	{
		return &m_fog;
	}

private:
	fogparams_t	m_fog;
};

LINK_ENTITY_TO_CLASS( trigger_fog, CFogTrigger );

BEGIN_MAPENTITY( CFogTrigger )

	DEFINE_KEYFIELD( m_fog.colorPrimary,	FIELD_COLOR32,	"fogcolor" ),
	DEFINE_KEYFIELD( m_fog.colorSecondary,	FIELD_COLOR32,	"fogcolor2" ),
	DEFINE_KEYFIELD( m_fog.dirPrimary,		FIELD_VECTOR,	"fogdir" ),
	DEFINE_KEYFIELD( m_fog.enable,			FIELD_BOOLEAN,	"fogenable" ),
	DEFINE_KEYFIELD( m_fog.blend,			FIELD_BOOLEAN,	"fogblend" ),
	DEFINE_KEYFIELD( m_fog.start,			FIELD_FLOAT,	"fogstart" ),
	DEFINE_KEYFIELD( m_fog.end,				FIELD_FLOAT,	"fogend" ),
	DEFINE_KEYFIELD( m_fog.farz,			FIELD_FLOAT,	"farz" ),

END_MAPENTITY()


//--------------------------------------------------------------------------------------------------------
void CFogTrigger::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL );

	BaseClass::Spawn();
	InitTrigger();
}


//--------------------------------------------------------------------------------------------------------
void CFogTrigger::StartTouch( CBaseEntity *other )
{
	if ( !PassesTriggerFilters( other ) )
		return;

	BaseClass::StartTouch( other );

	CBaseCombatCharacter *character = other->MyCombatCharacterPointer();
	if ( !character )
		return;

	character->OnFogTriggerStartTouch( this );
}


//--------------------------------------------------------------------------------------------------------
void CFogTrigger::EndTouch( CBaseEntity *other )
{
	if ( !PassesTriggerFilters( other ) )
		return;

	BaseClass::EndTouch( other );

	CBaseCombatCharacter *character = other->MyCombatCharacterPointer();
	if ( !character )
		return;

	character->OnFogTriggerEndTouch( this );
}

bool GetWorldFogParams( CBaseCombatCharacter *character, fogparams_t &fog )
{

	fogparams_t *targetFog = NULL;
	if ( character && character->GetFogTrigger() )
		{
		CFogTrigger *trigger = dynamic_cast< CFogTrigger * >(character->GetFogTrigger());
		if ( trigger )
		{
			targetFog = trigger->GetFog();
		}
	}

	if ( !targetFog && FogSystem()->GetMasterFogController() )
	{
		targetFog = &(FogSystem()->GetMasterFogController()->m_fog);
	}

	if ( targetFog )
	{
		if ( *targetFog != fog )
		{
			fog = *targetFog;
			return true;
		}
	}
	else
	{
		if ( fog.farz != -1 || fog.enable != false )
		{
			// No fog controller in this level. Use default fog parameters.
			fog.farz = -1;
			fog.enable = false;
			return true;
		}
	}

	return false;
}
