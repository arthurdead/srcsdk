//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EVENTLIST_H
#define EVENTLIST_H
#pragma once

#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL( LOG_ANIMEVENT );

#define AE_TYPE_SERVER			( 1 << 0 )
#define AE_TYPE_SCRIPTED		( 1 << 1 )		// see scriptevent.h
#define AE_TYPE_SHARED			( 1 << 2 )
#define AE_TYPE_WEAPON			( 1 << 3 )
#define AE_TYPE_CLIENT			( 1 << 4 )
#define AE_TYPE_FACEPOSER		( 1 << 5 )

//( 1 << 10 ) is used by the engine
#define AE_TYPE_RESERVED1 NEW_EVENT_STYLE

//#define EVENT_SPECIFIC			0
//#define EVENT_SCRIPTED			1000		// see scriptevent.h
//#define EVENT_SHARED			2000
//#define EVENT_WEAPON			3000
// NOTE: MUST BE THE LAST WEAPON EVENT -- ONLY WEAPON EVENTS BETWEEN EVENT_WEAPON AND THIS
//#define EVENT_WEAPON_LAST		3999
//#define EVENT_CLIENT			5000

#define AE_NOT_AVAILABLE		-1

typedef enum
{
	#define EVENTLIST_ENUM(name, type, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "eventlist_enum.inc"
} Animevent;

typedef struct evententry_s evententry_t;

//=========================================================
//=========================================================
extern void EventList_Init( void );
extern void EventList_Free( void );
extern bool EventList_RegisterSharedEvent( const char *pszEventName, Animevent iEventIndex, int iType = 0 );
extern Animevent EventList_RegisterPrivateEvent( const char *pszEventName );
extern Animevent EventList_IndexForName( const char *pszEventName );
extern const char *EventList_NameForIndex( Animevent iEventIndex );
Animevent EventList_RegisterPrivateEvent( const char *pszEventName );

// This macro guarantees that the names of each event and the constant used to
// reference it in the code are identical.
#define REGISTER_SHARED_ANIMEVENT( _n, b ) EventList_RegisterSharedEvent(#_n, _n, b );
#define REGISTER_PRIVATE_ANIMEVENT( _n ) _n = EventList_RegisterPrivateEvent( #_n );

// Implemented in shared code
extern void EventList_RegisterSharedEvents( void );
extern int EventList_GetEventType( Animevent eventIndex );


#endif // EVENTLIST_H
