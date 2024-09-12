//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "EnvMessage.h"
#include "engine/IEngineSound.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "Color.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_message, CMessage );

BEGIN_MAPENTITY( CMessage )

	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_sNoise, FIELD_SOUNDNAME, "messagesound" ),
	DEFINE_KEYFIELD( m_MessageAttenuation, FIELD_INTEGER, "messageattenuation" ),
	DEFINE_KEYFIELD( m_MessageVolume, FIELD_FLOAT, "messagevolume" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ShowMessage", InputShowMessage ),

	DEFINE_OUTPUT(m_OnShowMessage, "OnShowMessage"),

END_MAPENTITY()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessage::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	switch( m_MessageAttenuation )
	{
	case 1: // Medium radius
		m_Radius = ATTN_STATIC;
		break;
	
	case 2:	// Large radius
		m_Radius = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		m_Radius = ATTN_NONE;
		break;
	
	default:
	case 0: // Small radius
		m_Radius = SNDLVL_IDLE;
		break;
	}
	m_MessageAttenuation = 0;

	// Remap volume from [0,10] to [0,1].
	m_MessageVolume *= 0.1;

	// No volume, use normal
	if ( m_MessageVolume <= 0 )
	{
		m_MessageVolume = 1.0;
	}
}


void CMessage::Precache( void )
{
	if ( m_sNoise != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_sNoise) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CMessage::InputShowMessage( inputdata_t &inputdata )
{
	CBaseEntity *pPlayer = NULL;

	if ( m_spawnflags & SF_MESSAGE_ALL )
	{
		UTIL_ShowMessageAll( STRING( m_iszMessage ) );
	}
	else
	{
		if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
		{
			pPlayer = inputdata.pActivator;
		}
		else
		{
			pPlayer = NULL;
		}

		if ( pPlayer && pPlayer->IsPlayer() )
		{
			UTIL_ShowMessage( STRING( m_iszMessage ), ToBasePlayer( pPlayer ) );
		}
	}

	if ( m_sNoise != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );
		
		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_sNoise);
		ep.m_flVolume = m_MessageVolume;
		ep.m_SoundLevel = ATTN_TO_SNDLVL( m_Radius );

		EmitSound( filter, entindex(), ep );
	}

	if ( m_spawnflags & SF_MESSAGE_ONCE )
	{
		UTIL_Remove( this );
	}

	m_OnShowMessage.FireOutput( inputdata.pActivator, this );
}


void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	inputdata_t inputdata;

	inputdata.pActivator	= NULL;
	inputdata.pCaller		= NULL;

	InputShowMessage( inputdata );
}


class CCredits : public CPointEntity
{
public:
	DECLARE_CLASS( CMessage, CPointEntity );
	DECLARE_MAPENTITY();

	void	Spawn( void );
	void	InputRollCredits( inputdata_t &inputdata );
	void	InputRollOutroCredits( inputdata_t &inputdata );
	void	InputShowLogo( inputdata_t &inputdata );
	void	InputSetLogoLength( inputdata_t &inputdata );

	COutputEvent m_OnCreditsDone;

private:

	void	PrecacheCreditsThink();

	void		RollOutroCredits();

	bool		m_bRolledOutroCredits;
	float		m_flLogoLength;

	// Custom credits.txt, defaults to that
	string_t	m_iszCreditsFile;
};

LINK_ENTITY_TO_CLASS( env_credits, CCredits );

BEGIN_MAPENTITY( CCredits )
	DEFINE_INPUTFUNC( FIELD_VOID, "RollCredits", InputRollCredits ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RollOutroCredits", InputRollOutroCredits ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowLogo", InputShowLogo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetLogoLength", InputSetLogoLength ),
	DEFINE_OUTPUT( m_OnCreditsDone, "OnCreditsDone"),

	DEFINE_KEYFIELD( m_iszCreditsFile, FIELD_STRING, "CreditsFile" ),

END_MAPENTITY()

void CCredits::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	// Ensures the player has time to spawn
	SetContextThink( &CCredits::PrecacheCreditsThink, gpGlobals->curtime + 0.5f, "PrecacheCreditsContext" );
}

static void CreditsDone_f( void )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CCredits *pCredits = (CCredits*)gEntList.FindEntityByClassname( NULL, "env_credits" );

	if ( pCredits )
	{
		pCredits->m_OnCreditsDone.FireOutput( pCredits, pCredits );
	}
}

static ConCommand creditsdone("creditsdone", CreditsDone_f );

void CCredits::PrecacheCreditsThink()
{
	CBasePlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	if (!pPlayer)
	{
		Warning( "%s: No player\n", GetDebugName() );
		return;
	}

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();
	
	UserMessageBegin( user, "CreditsMsg" );
		WRITE_BYTE( 4 );
		WRITE_STRING( STRING(m_iszCreditsFile) );
	MessageEnd();

	SetContextThink( NULL, TICK_NEVER_THINK, "PrecacheCreditsContext" );
}

void CCredits::RollOutroCredits()
{
	CRecipientFilter filter; 
	filter.AddAllPlayers(); 
	filter.MakeReliable(); 
	UserMessageBegin( filter, "CreditsMsg" ); 
		WRITE_BYTE( 3 );
		WRITE_STRING( STRING(m_iszCreditsFile) );
	MessageEnd();
}

void CCredits::InputRollOutroCredits( inputdata_t &inputdata )
{
	RollOutroCredits();

	// In case we save restore
	m_bRolledOutroCredits = true;

	gamestats->Event_Credits();
}

void CCredits::InputShowLogo( inputdata_t &inputdata )
{
	CRecipientFilter filter; 
	filter.AddAllPlayers(); 
	filter.MakeReliable(); 

	if ( m_flLogoLength )
	{
		UserMessageBegin( filter, "LogoTimeMsg" ); 
			WRITE_FLOAT( m_flLogoLength );
			WRITE_STRING( STRING(m_iszCreditsFile) );
		MessageEnd();
	}
	else
	{
		UserMessageBegin( filter, "CreditsMsg" ); 
			WRITE_BYTE( 1 );
			WRITE_STRING( STRING(m_iszCreditsFile) );
		MessageEnd();
	}
}

void CCredits::InputSetLogoLength( inputdata_t &inputdata )
{
	m_flLogoLength = inputdata.value.Float();
}

void CCredits::InputRollCredits( inputdata_t &inputdata )
{
	CRecipientFilter filter; 
	filter.AddAllPlayers(); 
	filter.MakeReliable(); 

	UserMessageBegin( filter, "CreditsMsg" ); 
	WRITE_BYTE( 2 );
	WRITE_STRING( STRING(m_iszCreditsFile) );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Play the outtro stats at the end of the campaign
//-----------------------------------------------------------------------------
class COuttroStats : public CPointEntity
{
public:
	DECLARE_CLASS( COuttroStats, CPointEntity );
	DECLARE_MAPENTITY();

	void Spawn( void );
	void InputRollCredits( inputdata_t &inputdata );
	void InputRollStatsCrawl( inputdata_t &inputdata );
	void InputSkipStateChanged( inputdata_t &inputdata );

	void SkipThink( void );
	void CalcSkipState( int &skippingPlayers, int &totalPlayers );

	COutputEvent m_OnOuttroStatsDone;
};

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( env_outtro_stats, COuttroStats );

BEGIN_MAPENTITY( COuttroStats )
	DEFINE_INPUTFUNC( FIELD_VOID, "RollStatsCrawl", InputRollStatsCrawl ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RollCredits", InputRollCredits ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SkipStateChanged", InputSkipStateChanged ),
	DEFINE_OUTPUT( m_OnOuttroStatsDone, "OnOuttroStatsDone"),
END_MAPENTITY()

//-----------------------------------------------------------------------------
void COuttroStats::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
void COuttroStats::InputRollStatsCrawl( inputdata_t &inputdata )
{
	CReliableBroadcastRecipientFilter players;
	UserMessageBegin( players, "StatsCrawlMsg" );
	MessageEnd();

	SetThink( &COuttroStats::SkipThink );
	SetNextThink( gpGlobals->curtime + 1.0 );
}

//-----------------------------------------------------------------------------
void COuttroStats::InputRollCredits( inputdata_t &inputdata )
{
	CReliableBroadcastRecipientFilter players;
	UserMessageBegin( players, "creditsMsg" );
	MessageEnd();
}

void COuttroStats::SkipThink( void )
{
	// if all valid players are skipping, then end
	int iNumSkips = 0;
	int iNumPlayers = 0;
	CalcSkipState( iNumSkips, iNumPlayers );

	if ( iNumSkips >= iNumPlayers )
	{
//		TheDirector->StartScenarioExit();
	}
	else
		SetNextThink( gpGlobals->curtime + 1.0 );
}

void COuttroStats::CalcSkipState( int &skippingPlayers, int &totalPlayers )
{
	// calc skip state
	skippingPlayers = 0;
	totalPlayers = 0;
}

void COuttroStats::InputSkipStateChanged( inputdata_t &inputdata )
{
	int iNumSkips = 0;
	int iNumPlayers = 0;
	CalcSkipState( iNumSkips, iNumPlayers );

	DevMsg( "COuttroStats: Skip state changed. %d players, %d skips\n", iNumPlayers, iNumSkips );
	// Don't send to players in singleplayer
	if ( iNumPlayers > 1 )
	{
		CReliableBroadcastRecipientFilter players;
		UserMessageBegin( players, "StatsSkipState" );
			WRITE_BYTE( iNumSkips );
			WRITE_BYTE( iNumPlayers );
		MessageEnd();
	}
}

void CC_Test_Outtro_Stats( const CCommand& args )
{
	CBaseEntity *pOuttro = gEntList.FindEntityByClassname( NULL, "env_outtro_stats" );
	if ( pOuttro )
	{
		variant_t emptyVariant;
		pOuttro->AcceptInput( "RollStatsCrawl", NULL, NULL, emptyVariant, 0 );
	}
}
static ConCommand test_outtro_stats("test_outtro_stats", CC_Test_Outtro_Stats, 0, FCVAR_CHEAT);