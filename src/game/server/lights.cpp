//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: spawn and think functions for editor-placed lights
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "lights.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_LIGHTS, "Lights Server" );

static const char *g_DefaultLightstyles[] =
{
	// 0 normal
	"m",
	// 1 FLICKER (first variety)
	"mmnmmommommnonmmonqnmmo",
	// 2 SLOW STRONG PULSE
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	// 3 CANDLE (first variety)
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	// 4 FAST STROBE
	"mamamamamama",
	// 5 GENTLE PULSE 1
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	// 6 FLICKER (second variety)
	"nmonqnmomnmomomno",
	// 7 CANDLE (second variety)
	"mmmaaaabcdefgmmmmaaaammmaamm",
	// 8 CANDLE (third variety)
	"mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa",
	// 9 SLOW STROBE (fourth variety)
	"aaaaaaaazzzzzzzz",
	// 10 FLUORESCENT FLICKER
	"mmamammmmammamamaaamammma",
	// 11 SLOW PULSE NOT FADE TO BLACK
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	"mmnnmmnnnmmnn",
};

const char *GetDefaultLightstyleString( int styleIndex )
{
	if ( styleIndex < ARRAYSIZE(g_DefaultLightstyles) )
	{
		return g_DefaultLightstyles[styleIndex];
	}
	return "m";
}

void SetupDefaultLightstyle()
{
	//
	// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
	//
	COM_TimestampedLog( "LightStyles" );
	for ( int i = 0; i < ARRAYSIZE(g_DefaultLightstyles); i++ )
	{
		engine->LightStyle( i, GetDefaultLightstyleString(i) );
	}

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	engine->LightStyle(63, "a");
}

LINK_ENTITY_TO_CLASS( light, CLight );

BEGIN_MAPENTITY( CLight )

	DEFINE_KEYFIELD_AUTO( m_iStyle, "style" ),
	DEFINE_KEYFIELD_AUTO( m_iDefaultStyle, "defaultstyle" ),
	DEFINE_KEYFIELD_AUTO( m_iszPattern, "pattern" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPattern", InputSetPattern ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FadeToPattern", InputFadeToPattern ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOff", InputTurnOff ),

END_MAPENTITY()

//
// Cache user-entity-field values until spawn is called.
//
bool CLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "pitch"))
	{
		QAngle angles = GetAbsAngles();
		angles.x = atof(szValue);
		SetAbsAngles( angles );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

// Light entity
// If targeted, it will toggle between on or off.
void CLight::Spawn( void )
{
	if (!GetEntityName())
	{       // inert light
		UTIL_Remove( this );
		return;
	}
	
	if (m_iStyle >= 32)
	{
		if ( m_iszPattern == NULL_STRING && m_iDefaultStyle > 0 )
		{
			m_iszPattern = MAKE_STRING(GetDefaultLightstyleString(m_iDefaultStyle));
		}

		if (HasSpawnFlags( SF_LIGHT_START_OFF))
			engine->LightStyle(m_iStyle, "a");
		else if (m_iszPattern != NULL_STRING)
			engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		else
			engine->LightStyle(m_iStyle, "m");
	}
}


void CLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType, !HasSpawnFlags( SF_LIGHT_START_OFF) ) )
			return;

		Toggle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light on
//-----------------------------------------------------------------------------
void CLight::TurnOn( void )
{
	if ( m_iszPattern != NULL_STRING )
	{
		engine->LightStyle( m_iStyle, (char *) STRING( m_iszPattern ) );
	}
	else
	{
		engine->LightStyle( m_iStyle, "m" );
	}

	RemoveSpawnFlags(  SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light off
//-----------------------------------------------------------------------------
void CLight::TurnOff( void )
{
	engine->LightStyle( m_iStyle, "a" );
	AddSpawnFlags(  SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the light on/off
//-----------------------------------------------------------------------------
void CLight::Toggle( void )
{
	//Toggle it
	if ( HasSpawnFlags(  SF_LIGHT_START_OFF ) )
	{
		TurnOn();
	}
	else
	{
		TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnon" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOn( inputdata_t &&inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnoff" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOff( inputdata_t &&inputdata )
{
	TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "toggle" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputToggle( inputdata_t &&inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting a light pattern
//-----------------------------------------------------------------------------
void CLight::InputSetPattern( inputdata_t &&inputdata )
{
	m_iszPattern = inputdata.value.StringID();
	engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));

	// Light is on if pattern is set
	RemoveSpawnFlags( SF_LIGHT_START_OFF);
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for fading from first value in old pattern to 
//			first value in new pattern
//-----------------------------------------------------------------------------
void CLight::InputFadeToPattern( inputdata_t &&inputdata )
{
	m_iCurrentFade	= (STRING(m_iszPattern))[0];
	m_iTargetFade	= inputdata.value.String()[0];
	m_iszPattern	= inputdata.value.StringID();
	SetThink(&CLight::FadeThink);
	SetNextThink( gpGlobals->curtime );

	// Light is on if pattern is set
	RemoveSpawnFlags( SF_LIGHT_START_OFF);
}


//------------------------------------------------------------------------------
// Purpose : Fade light to new starting pattern value then stop thinking
//------------------------------------------------------------------------------
void CLight::FadeThink(void)
{
	if (m_iCurrentFade < m_iTargetFade)
	{
		m_iCurrentFade++;
	}
	else if (m_iCurrentFade > m_iTargetFade)
	{
		m_iCurrentFade--;
	}

	// If we're done fading instantiate our light pattern and stop thinking
	if (m_iCurrentFade == m_iTargetFade)
	{
		engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		SetNextThink( TICK_NEVER_THINK );
	}
	// Otherwise instantiate our current fade value and keep thinking
	else
	{
		char sCurString[2];
		sCurString[0] = m_iCurrentFade;
		sCurString[1] = 0;
		engine->LightStyle(m_iStyle, sCurString);

		// UNDONE: Consider making this settable war to control fade speed
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );
LINK_ENTITY_TO_CLASS( light_glspot, CLight );
LINK_ENTITY_TO_CLASS( light_directional, CLight );

#define EnvLightBase CLogicalEntity

class CEnvLight : public EnvLightBase
{
public:
	DECLARE_CLASS( CEnvLight, EnvLightBase );
	DECLARE_NETWORKCLASS();

	CEnvLight();
	~CEnvLight();

	bool	KeyValue( const char *szKeyName, const char *szValue ); 

	virtual void Spawn();

private:
	CNetworkQAngle( m_angSunAngles );
	CNetworkVector( m_vecLight );
	CNetworkVector( m_vecAmbient );
	CNetworkVar( bool, m_bCascadedShadowMappingEnabled );
	bool m_bHasHDRLightSet;
	bool m_bHasHDRAmbientSet;
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CEnvLight, DT_CEnvLight )
	SendPropQAngles( SENDINFO( m_angSunAngles ) ),
	SendPropVector( SENDINFO( m_vecLight ) ),
	SendPropVector( SENDINFO( m_vecAmbient ) ),
	SendPropBool( SENDINFO( m_bCascadedShadowMappingEnabled ) ),
END_SEND_TABLE()

CEnvLight *g_pCSMEnvLight = NULL;

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

CEnvLight::CEnvLight() : m_bHasHDRLightSet( false ), m_bHasHDRAmbientSet( false )
{
	if(g_pCSMEnvLight == NULL)
		g_pCSMEnvLight = this;
}

CEnvLight::~CEnvLight()
{
	if(g_pCSMEnvLight == this)
		g_pCSMEnvLight = NULL;
}

void CEnvLight::Spawn( void )
{
	if(g_pCSMEnvLight && g_pCSMEnvLight != this) {
		UTIL_Remove(this);
		return;
	}

	SetName( MAKE_STRING( "light_environment" ) );

	BaseClass::Spawn( );

	m_bCascadedShadowMappingEnabled = HasSpawnFlags( 0x01 );
}

static Vector ConvertLightmapGammaToLinear( int *iColor4 )
{
	Vector vecColor;
	for ( int i = 0; i < 3; ++i )
	{
		vecColor[i] = powf( iColor4[i] / 255.0f, 2.2f );
	}
	vecColor *= iColor4[3] / 255.0f;
	return vecColor;
}

bool CEnvLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "pitch" ) )
	{
		m_angSunAngles.SetX( -atof( szValue ) );
	}
	else if ( FStrEq( szKeyName, "angles" ) )
	{
		Vector vecParsed;
		UTIL_StringToVector( vecParsed.Base(), szValue );
		m_angSunAngles.SetY( vecParsed.y );
	}
	else if ( FStrEq( szKeyName, "_light" ) || FStrEq( szKeyName, "_lightHDR" ) )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, szValue );

		if ( iParsed[0] <= 0 || iParsed[1] <= 0 || iParsed[2] <= 0 )
			return true;

		if ( FStrEq( szKeyName, "_lightHDR" ) )
		{
			// HDR overrides LDR
			m_bHasHDRLightSet = true;
		}
		else if ( m_bHasHDRLightSet )
		{
			// If this is LDR and we got HDR already, bail out.
			return true;
		}

		m_vecLight = ConvertLightmapGammaToLinear( iParsed );
		Log_Msg( LOG_LIGHTS,"Parsed light_environment light: %i %i %i %i\n",
			 iParsed[0], iParsed[1], iParsed[2], iParsed[3] );
	}
	else if ( FStrEq( szKeyName, "_ambient" ) || FStrEq( szKeyName, "_ambientHDR" ) )
	{
		int iParsed[4];
		UTIL_StringToIntArray( iParsed, 4, szValue );

		if ( iParsed[0] <= 0 || iParsed[1] <= 0 || iParsed[2] <= 0 )
			return true;

		if ( FStrEq( szKeyName, "_ambientHDR" ) )
		{
			// HDR overrides LDR
			m_bHasHDRLightSet = true;
		}
		else if ( m_bHasHDRLightSet )
		{
			// If this is LDR and we got HDR already, bail out.
			return true;
		}

		m_vecAmbient = ConvertLightmapGammaToLinear( iParsed );
		Log_Msg( LOG_LIGHTS,"Parsed light_environment ambient: %i %i %i %i\n",
			 iParsed[0], iParsed[1], iParsed[2], iParsed[3] );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}
