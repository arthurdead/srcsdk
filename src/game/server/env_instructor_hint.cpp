//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity for creating instructor hints entirely with map logic
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvInstructorHint : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvInstructorHint, CPointEntity );
	DECLARE_MAPENTITY();

private:
	void InputShowHint( inputdata_t &&inputdata );
	void InputEndHint( inputdata_t &&inputdata );
	
	string_t	m_iszReplace_Key;
	string_t	m_iszHintTargetEntity;
	int			m_iTimeout;
	string_t	m_iszIcon_Onscreen;
	string_t	m_iszIcon_Offscreen;
	string_t	m_iszCaption;
	string_t	m_iszActivatorCaption;
	color32		m_Color;
	float		m_fIconOffset;
	float		m_fRange;
	uint8		m_iPulseOption;
	uint8		m_iAlphaOption;
	uint8		m_iShakeOption;
	bool		m_bStatic;
	bool		m_bNoOffscreen;
	bool		m_bForceCaption;
	string_t	m_iszBinding;
	bool		m_bAllowNoDrawTarget;
	bool		m_bLocalPlayerOnly;
};

LINK_ENTITY_TO_CLASS( env_instructor_hint, CEnvInstructorHint );

BEGIN_MAPENTITY( CEnvInstructorHint )

	DEFINE_KEYFIELD_AUTO( m_iszReplace_Key, "hint_replace_key" ),
	DEFINE_KEYFIELD_AUTO( m_iszHintTargetEntity, "hint_target" ),
	DEFINE_KEYFIELD_AUTO( m_iTimeout, "hint_timeout" ),
	DEFINE_KEYFIELD_AUTO( m_iszIcon_Onscreen, "hint_icon_onscreen" ),
	DEFINE_KEYFIELD_AUTO( m_iszIcon_Offscreen, "hint_icon_offscreen" ),
	DEFINE_KEYFIELD_AUTO( m_iszCaption, "hint_caption" ),
	DEFINE_KEYFIELD_AUTO( m_iszActivatorCaption, "hint_activator_caption" ),
	DEFINE_KEYFIELD_AUTO( m_Color, "hint_color" ),
	DEFINE_KEYFIELD_AUTO( m_fIconOffset, "hint_icon_offset" ),
	DEFINE_KEYFIELD_AUTO( m_fRange, "hint_range" ),
	DEFINE_KEYFIELD_AUTO( m_iPulseOption, "hint_pulseoption" ),
	DEFINE_KEYFIELD_AUTO( m_iAlphaOption, "hint_alphaoption" ),
	DEFINE_KEYFIELD_AUTO( m_iShakeOption, "hint_shakeoption" ),
	DEFINE_KEYFIELD_AUTO( m_bStatic, "hint_static" ),
	DEFINE_KEYFIELD_AUTO( m_bNoOffscreen, "hint_nooffscreen" ),
	DEFINE_KEYFIELD_AUTO( m_bForceCaption, "hint_forcecaption" ),
	DEFINE_KEYFIELD_AUTO( m_iszBinding, "hint_binding" ),
	DEFINE_KEYFIELD_AUTO( m_bAllowNoDrawTarget, "hint_allow_nodraw_target" )	
	DEFINE_KEYFIELD_AUTO( m_bLocalPlayerOnly, "hint_local_player_only" ),
	
	DEFINE_INPUTFUNC( FIELD_STRING,	"ShowHint",	InputShowHint ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EndHint",	InputEndHint ),

END_MAPENTITY()


#define LOCATOR_ICON_FX_PULSE_SLOW		0x00000001
#define LOCATOR_ICON_FX_ALPHA_SLOW		0x00000008
#define LOCATOR_ICON_FX_SHAKE_NARROW	0x00000040
#define LOCATOR_ICON_FX_STATIC			0x00000100	// This icon draws at a fixed location on the HUD.

//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CEnvInstructorHint::InputShowHint( inputdata_t &&inputdata )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "instructor_server_hint_create", false );
	if ( event )
	{
		CBaseEntity *pTargetEntity = gEntList.FindEntityByName( NULL, m_iszHintTargetEntity );
		if( pTargetEntity == NULL && !m_bStatic )
			pTargetEntity = inputdata.pActivator;

		if( pTargetEntity == NULL )
			pTargetEntity = GetWorldEntity();

		char szColorString[128];
		Q_snprintf( szColorString, sizeof( szColorString ), "%.3d,%.3d,%.3d", m_Color.r, m_Color.g, m_Color.b );

		int iFlags = 0;
		
		iFlags |= (m_iPulseOption == 0) ? 0 : (LOCATOR_ICON_FX_PULSE_SLOW << (m_iPulseOption - 1));
		iFlags |= (m_iAlphaOption == 0) ? 0 : (LOCATOR_ICON_FX_ALPHA_SLOW << (m_iAlphaOption - 1));
		iFlags |= (m_iShakeOption == 0) ? 0 : (LOCATOR_ICON_FX_SHAKE_NARROW << (m_iShakeOption - 1));
		iFlags |= m_bStatic ? LOCATOR_ICON_FX_STATIC : 0;

		CBasePlayer *pActivator = NULL;
		bool bFilterByActivator = m_bLocalPlayerOnly;

		if ( inputdata.value.StringID() != NULL_STRING )
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String() );
			pActivator = dynamic_cast<CBasePlayer*>( pTarget );
			if ( pActivator )
			{
				bFilterByActivator = true;
			}
		}
		else
		{
			Warning( "Failed to play server side instructor hint: no player specified for hint\n" );
			Assert( 0 );
		}

		const char *pActivatorCaption = m_iszActivatorCaption.ToCStr();
		if ( !pActivatorCaption || pActivatorCaption[ 0 ] == '\0' )
		{
			pActivatorCaption = m_iszCaption.ToCStr();
		}

		event->SetString( "hint_name", GetEntityName().ToCStr() );
		event->SetString( "hint_replace_key", m_iszReplace_Key.ToCStr() );
		event->SetInt( "hint_target", pTargetEntity->entindex() );
		event->SetInt( "hint_activator_userid", ( pActivator ? pActivator->GetUserID() : 0 ) );
		event->SetInt( "hint_timeout", m_iTimeout );
		event->SetString( "hint_icon_onscreen", m_iszIcon_Onscreen.ToCStr() );
		event->SetString( "hint_icon_offscreen", m_iszIcon_Offscreen.ToCStr() );
		event->SetString( "hint_caption", m_iszCaption.ToCStr() );
		event->SetString( "hint_activator_caption", pActivatorCaption );
		event->SetString( "hint_color", szColorString );
		event->SetFloat( "hint_icon_offset", m_fIconOffset );
		event->SetFloat( "hint_range", m_fRange );
		event->SetInt( "hint_flags", iFlags );
		event->SetString( "hint_binding", m_iszBinding.ToCStr() );
		event->SetBool( "hint_allow_nodraw_target", m_bAllowNoDrawTarget );
		event->SetBool( "hint_nooffscreen", m_bNoOffscreen );
		event->SetBool( "hint_forcecaption", m_bForceCaption );
		event->SetBool( "hint_local_player_only", bFilterByActivator );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvInstructorHint::InputEndHint( inputdata_t &&inputdata )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "instructor_server_hint_stop", false );
	if ( event )
	{
		event->SetString( "hint_name", GetEntityName().ToCStr() );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: A generic target entity that gets replicated to the client for instructor hint targetting
//-----------------------------------------------------------------------------
class CInfoInstructorHintTarget : public CPointEntity
{
public:
	DECLARE_CLASS( CInfoInstructorHintTarget, CPointEntity );

	virtual EdictStateFlags_t UpdateTransmitState( void )	// set transmit filter to transmit always
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

LINK_ENTITY_TO_CLASS( info_target_instructor_hint, CInfoInstructorHintTarget );
