//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "clientsteamcontext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_STEAM, "Steam" );


static CClientSteamContext g_ClientSteamContext;
CClientSteamContext  &ClientSteamContext()
{
	return g_ClientSteamContext;
}

CSteamAPIContext *steamapicontext = &g_ClientSteamContext;

//-----------------------------------------------------------------------------
CClientSteamContext::CClientSteamContext() 
:
	m_CallbackSteamServersDisconnected( this, &CClientSteamContext::OnSteamServersDisconnected ),
	m_CallbackSteamServerConnectFailure( this, &CClientSteamContext::OnSteamServerConnectFailure ),
	m_CallbackSteamServersConnected( this, &CClientSteamContext::OnSteamServersConnected )
{
	m_bActive = false;
	m_bLoggedOn = false;
	m_nAppID = 0;
	m_nUniverse = k_EUniverseInvalid;
}


//-----------------------------------------------------------------------------
CClientSteamContext::~CClientSteamContext()
{
}


//-----------------------------------------------------------------------------
// Purpose: Unload the steam3 engine
//-----------------------------------------------------------------------------
void CClientSteamContext::Shutdown()
{	
	if ( !m_bActive )
		return;

	m_bActive = false;
	m_bLoggedOn = false;

	Clear(); // Steam API context shutdown
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the steam3 connection
//-----------------------------------------------------------------------------
void CClientSteamContext::Activate()
{
	if ( m_bActive )
		return;

	m_bActive = true;

	SteamAPI_InitSafe(); // ignore failure, that will fall out later when they don't get a valid logon cookie
	SteamAPI_SetTryCatchCallbacks( false ); // We don't use exceptions, so tell steam not to use try/catch in callback handlers
	Init(); // Steam API context init
	
	UpdateLoggedOnState();
	Log_Msg( LOG_STEAM,"CClientSteamContext logged on = %d\n", m_bLoggedOn );
}

void CClientSteamContext::UpdateLoggedOnState()
{
	bool bPreviousLoggedOn = m_bLoggedOn;
	m_bLoggedOn = ( SteamUser() && SteamUtils() && SteamUser()->BLoggedOn() );

	if ( !bPreviousLoggedOn && m_bLoggedOn )
	{
		// update Steam info
		m_SteamIDLocalPlayer = SteamUser()->GetSteamID();
		m_nUniverse = SteamUtils()->GetConnectedUniverse();
		m_nAppID = SteamUtils()->GetAppID();
	}

	if ( bPreviousLoggedOn != m_bLoggedOn )
	{
		// Notify any listeners of the change in logged on state
		SteamLoggedOnChange_t loggedOnChange;
		loggedOnChange.bPreviousLoggedOn = bPreviousLoggedOn;
		loggedOnChange.bLoggedOn = m_bLoggedOn;
		InvokeCallbacks( loggedOnChange );
	}
}

void CClientSteamContext::OnSteamServersDisconnected( SteamServersDisconnected_t *pDisconnected )
{
	UpdateLoggedOnState();
	Log_Msg( LOG_STEAM, "CClientSteamContext OnSteamServersDisconnected logged on = %d\n", m_bLoggedOn );
}

void CClientSteamContext::OnSteamServerConnectFailure( SteamServerConnectFailure_t *pConnectFailure )
{
	UpdateLoggedOnState();
	Log_Msg( LOG_STEAM, "CClientSteamContext OnSteamServerConnectFailure logged on = %d\n", m_bLoggedOn );
}

void CClientSteamContext::OnSteamServersConnected( SteamServersConnected_t *pConnected )
{
	UpdateLoggedOnState();
	Log_Msg( LOG_STEAM, "CClientSteamContext OnSteamServersConnected logged on = %d\n", m_bLoggedOn );
}

void CClientSteamContext::InstallCallback( CUtlDelegate< void ( const SteamLoggedOnChange_t & ) > delegate )
{
	m_LoggedOnCallbacks.AddToTail( delegate );
}

void CClientSteamContext::RemoveCallback( CUtlDelegate< void ( const SteamLoggedOnChange_t & ) > delegate )
{
	m_LoggedOnCallbacks.FindAndRemove( delegate );
}

void CClientSteamContext::InvokeCallbacks( const SteamLoggedOnChange_t &loggedOnStatus )
{
	for ( int i = 0; i < m_LoggedOnCallbacks.Count(); ++i )
	{
		m_LoggedOnCallbacks[i]( loggedOnStatus );
	}
}