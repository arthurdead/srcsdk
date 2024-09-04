//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include <bitbuf.h>
#include "voice_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RegisterUserMessages( void );

//-----------------------------------------------------------------------------
// Purpose: Force registration on .dll load
// FIXME:  Should this be a client/server system?
//-----------------------------------------------------------------------------
CUserMessages::CUserMessages()
{
	Register( "SayText", -1 );	
	Register( "SayText2", -1 );
	Register( "TextMsg", -1 );

	// Voting
	Register( "CallVoteFailed", -1 );
	Register( "VoteStart", -1 );
	Register( "VotePass", -1 );
	Register( "VoteFailed", 2 );
	Register( "VoteSetup", -1 );  // Initiates client-side voting UI

	Register( "HudText", -1 );	
	Register( "GameTitle", 0 );	// show game title
	Register( "HudMsg", -1 );

	Register( "ResetHUD", 1 );	// called every respawn
	Register( "SendAudio", -1 );	// play radion command
	Register( "ShowMenu", -1 );	// show hud menu

	Register( "Shake", 13 );		// shake view
	Register( "Fade", 10 );	// fade HUD in/out
	Register( "ShakeDir", -1 ); // directional shake
	Register( "Tilt", 22 );

	Register( "VGUIMenu", -1 );	// Show VGUI menu
	Register( "Rumble", 3 );	// Send a rumble to a controller

	Register( "CloseCaption", -1 ); // Show a caption (by string id number)(duration in 10th of a second)
	Register( "CloseCaptionDirect", -1 ); // Show a forced caption (by string id number)(duration in 10th of a second)

	Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	Register( "RequestState", 0 );

	Register( "HintText", -1 );	// Displays hint text display
	Register( "KeyHintText", -1 );	// Displays hint text display

	Register( "PlayerAnimEvent", -1 );	// jumping, firing, reload, etc.

	Register( "ItemPickup", -1 );	// for item history on screen
	Register( "AmmoDenied", 2 );

	Register( "SquadMemberDied", 0 );

	Register( "CurrentTimescale", 4 );	// Send one float for the new timescale
	Register( "DesiredTimescale", 13 );	// Send timescale and some blending vars

	Register( "CreditsMsg", 1 );
	Register( "LogoTimeMsg", 4 );
	Register( "AchievementEvent", -1 );

	Register( "Damage", -1 );

	// Game specific registration function;
	RegisterUserMessages();
}

CUserMessages::~CUserMessages()
{
	int c = m_UserMessages.Count();
	for ( int i = 0; i < c; ++i )
	{
		delete m_UserMessages[ i ];
	}
	m_UserMessages.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::LookupUserMessage( const char *name )
{
	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		return -1;
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::GetUserMessageSize( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	CUserMessage *e = m_UserMessages[ index ];
	return e->size;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CUserMessages::GetUserMessageName( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageName( %i ) out of range!!!\n", index );
	}

	return m_UserMessages.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::IsValidIndex( int index )
{
	return m_UserMessages.IsValidIndex( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			size - -1 for variable size
//-----------------------------------------------------------------------------
void CUserMessages::Register( const char *name, int size )
{
	Assert( name );
	int idx = m_UserMessages.Find( name );
	if ( idx != m_UserMessages.InvalidIndex() )
	{
		Error( "CUserMessages::Register '%s' already registered\n", name );
	}

	CUserMessage * entry = new CUserMessage;
	entry->size = size;
	entry->name = name;

	m_UserMessages.Insert( name, entry );
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			hook - 
//-----------------------------------------------------------------------------
void CUserMessages::HookMessage( const char *name, pfnUserMsgHook hook )
{
	Assert( name );
	Assert( hook );

	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		DevMsg( "CUserMessages::HookMessage:  no such message %s\n", name );
		Assert( 0 );
		return;
	}

	int i = m_UserMessages[ idx ]->clienthooks.AddToTail();
	m_UserMessages[ idx ]->clienthooks[i] = hook;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::DispatchUserMessage( int msg_type, bf_read &msg_data )
{
	if ( msg_type < 0 || msg_type >= (int)m_UserMessages.Count() )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Bogus msg type %i (max == %i)\n", msg_type, m_UserMessages.Count() );
		Assert( 0 );
		return false;
	}

	CUserMessage *entry = m_UserMessages[ msg_type ];

	if ( !entry )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Missing client entry for msg type %i\n", msg_type );
		Assert( 0 );
		return false;
	}

	if ( entry->clienthooks.Count() == 0 )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  missing client hook for %s\n", GetUserMessageName(msg_type) );
		Assert( 0 );
		return false;
	}

	for (int i = 0; i < entry->clienthooks.Count(); i++  )
	{
		bf_read msg_copy = msg_data;

		pfnUserMsgHook hook = entry->clienthooks[i];
		(*hook)( msg_copy );
	}
	return true;
}
#endif

// Singleton
static CUserMessages g_UserMessages;
// Expose to rest of .dll
CUserMessages *usermessages = &g_UserMessages;
