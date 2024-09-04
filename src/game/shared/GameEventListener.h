//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAME_EVENT_LISTENER_H
#define GAME_EVENT_LISTENER_H
#pragma once

#include "igameevents.h"
extern IGameEventManager2 *gameeventmanager;

// A safer method than inheriting straight from IGameEventListener2.
// Avoids requiring the user to remove themselves as listeners in 
// their deconstructor, and sets the serverside variable based on
// our dll location.
class CGameEventListener : public IGameEventListener2
{
public:
	CGameEventListener() : m_bRegisteredForEvents(false)
	{
		m_nDebugID = EVENT_DEBUG_ID_INIT;
	}

	virtual ~CGameEventListener()
	{
		m_nDebugID = EVENT_DEBUG_ID_SHUTDOWN;
		StopListeningForAllEvents();
	}

	void ListenForGameEvent( const char *name )
	{
		m_bRegisteredForEvents = true;

		if ( gameeventmanager ) {
		#ifdef CLIENT_DLL
			gameeventmanager->AddListener( this, name, false );
		#else
			gameeventmanager->AddListener( this, name, true );
		#endif
		}
	}

	void StopListeningForAllEvents()
	{
		// remove me from list
		if ( m_bRegisteredForEvents )
		{
			if ( gameeventmanager )
				gameeventmanager->RemoveListener( this );
			m_bRegisteredForEvents = false;
		}
	}

	// Intentionally abstract
	virtual void FireGameEvent( IGameEvent *event ) = 0;
	int m_nDebugID;
	virtual int GetEventDebugID( void )			{ return m_nDebugID; }

private:

	// Have we registered for any events?
	bool m_bRegisteredForEvents;
};

#endif
