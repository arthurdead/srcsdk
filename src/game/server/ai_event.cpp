//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Events that are available to all NPCs.
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "eventlist.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Init static variables
//=============================================================================
CStringRegistry* CAI_BaseNPC::m_pEventSR	= NULL;
int				 CAI_BaseNPC::m_iNumEvents	= 0;

//-----------------------------------------------------------------------------
// Purpose: Add an activity to the activity string registry and increment
//			the acitivty counter
//-----------------------------------------------------------------------------
void CAI_BaseNPC::AddEventToSR(const char *eventName, Animevent eventID) 
{
	MEM_ALLOC_CREDIT();
	Assert( m_pEventSR );

	m_pEventSR->AddString( eventName, eventID );
	m_iNumEvents++;
}

//-----------------------------------------------------------------------------
// Purpose: Given and activity ID, return the activity name
//-----------------------------------------------------------------------------
const char *CAI_BaseNPC::GetEventName(Animevent eventID) 
{
	const char *name = m_pEventSR->GetStringText( eventID );	
	return name;
}

//-----------------------------------------------------------------------------
// Purpose: Given and activity name, return the activity ID
//-----------------------------------------------------------------------------
Animevent CAI_BaseNPC::GetEventID(const char* eventName) 
{
	return (Animevent)m_pEventSR->GetStringID( eventName );
}

