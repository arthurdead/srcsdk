//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "eventlist.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NOTE: If CStringRegistry allowed storing arbitrary data, we could just use that.
// in this case we have the "isPrivate" member and the replacement rules 
// (eventIndex can be reused by private activities), so a custom table is necessary
struct eventlist_t
{
	Animevent			eventIndex;
	int					iType;
	unsigned short		stringKey;
	bool				isPrivate;
};

CUtlVector<eventlist_t> g_EventList;

// This stores the actual event names.  Also, the string ID in the registry is simply an index 
// into the g_EventList array.
CStringRegistry	g_EventStrings;

// this is just here to accelerate adds
static Animevent g_HighestEvent = (Animevent)0;

int g_nEventListVersion = 1;


void EventList_Init( void )
{
	g_HighestEvent = (Animevent)0;
}

void EventList_Free( void )
{
	g_EventStrings.ClearStrings();
	g_EventList.Purge();

	// So studiohdrs can reindex event indices
	++g_nEventListVersion;
}

// add a new event to the database
eventlist_t *EventList_AddEventEntry( const char *pName, Animevent iEventIndex, bool isPrivate, int iType )
{
	MEM_ALLOC_CREDIT();
	int index = g_EventList.AddToTail();
	eventlist_t *pList = &g_EventList[index];
	pList->eventIndex = iEventIndex;
	pList->stringKey = g_EventStrings.AddString( pName, index );
	pList->isPrivate = isPrivate;
	pList->iType = iType;	

	// UNDONE: This implies that ALL shared activities are added before ANY custom activities
	// UNDONE: Segment these instead?  It's a 32-bit int, how many activities do we need?
	if ( iEventIndex > g_HighestEvent )
	{
		g_HighestEvent = iEventIndex;
	}

	return pList;
}

// get the database entry from a string
static eventlist_t *ListFromString( const char *pString )
{
	// just use the string registry to do this search/map
	int stringID = g_EventStrings.GetStringID( pString );
	if ( stringID < 0 )
		return NULL;

	return &g_EventList[stringID];
}

// Get the database entry for an index
static eventlist_t *ListFromEvent( Animevent eventIndex )
{
	if(eventIndex == AE_INVALID)
		return NULL;

	// ugly linear search
	for ( int i = 0; i < g_EventList.Size(); i++ )
	{
		if ( g_EventList[i].eventIndex == eventIndex )
		{
			return &g_EventList[i];
		}
	}

	return NULL;
}

int EventList_GetEventType( Animevent eventIndex )
{
	if(eventIndex == AE_INVALID)
		return 0;

	eventlist_t *pEvent = ListFromEvent( eventIndex );

	if ( pEvent )
	{
		return pEvent->iType;
	}

	return 0;
}


bool EventList_RegisterSharedEvent( const char *pszEventName, Animevent iEventIndex, int iType )
{
#ifdef _DEBUG
	// UNDONE: Do we want to do these checks when not in developer mode? or maybe DEBUG only?
	// They really only matter when you change the list of code controlled activities.  IDs
	// for content controlled activities never collide because they are generated.

	// first, check to make sure the slot we're asking for is free. It must be for 
	// a shared event.
	eventlist_t *pList = ListFromString( pszEventName );
	if ( !pList )
	{
		pList = ListFromEvent( iEventIndex );
	}

	//Already in list.
	if ( pList )
	{
		Warning( "***\nShared event collision! %s<->%s\n***\n", pszEventName, g_EventStrings.GetStringForKey( pList->stringKey ) );
		Assert(0);
		return false;
	}
	// ----------------------------------------------------------------
#endif

	return ( EventList_AddEventEntry( pszEventName, iEventIndex, false, iType ) != NULL );
}

Animevent EventList_RegisterPrivateEvent( const char *pszEventName )
{
	eventlist_t *pList = ListFromString( pszEventName );
	if ( pList )
	{
		// this activity is already in the list. If the activity we collided with is also private, 
		// then the collision is OK. Otherwise, it's a bug.
		if ( pList->isPrivate )
		{
			return (Animevent)pList->eventIndex;
		}
		else
		{
			// this private activity collides with a shared activity. That is not allowed.
			Warning( "***\nShared<->Private Event collision!\n***\n" );
			Assert(0);
			return AE_INVALID;
		}
	}

	pList = EventList_AddEventEntry( pszEventName, (Animevent)(g_HighestEvent+1), true, AE_TYPE_SERVER );
	return (Animevent)pList->eventIndex;
}

// Get the index for a given Event name
// Done at load time for all models
Animevent EventList_IndexForName( const char *pszEventName )
{
	// this is a fast O(lgn) search (actually does 2 O(lgn) searches)
	eventlist_t *pList = ListFromString( pszEventName );

	if ( pList )
	{
		return pList->eventIndex;
	}

	return AE_INVALID;
}

// Get the name for a given index
// This should only be used in debug code, it does a linear search
// But at least it only compares integers
const char *EventList_NameForIndex( Animevent eventIndex )
{
	eventlist_t *pList = ListFromEvent( eventIndex );
	if ( pList )
	{
		return g_EventStrings.GetStringForKey( pList->stringKey );
	}
	return NULL;
}

//for studio.h
const char *EventList_NameForIndex( int eventIndex )
{ return EventList_NameForIndex((Animevent)eventIndex); }

void EventList_RegisterSharedEvents( void )
{
	#define EVENTLIST_ENUM(name, type, ...) \
		REGISTER_SHARED_ANIMEVENT(name, type);

	#include "eventlist_enum.inc"

	Assert(g_HighestEvent <= (Animevent)(unsigned short)-1);
}

#ifdef GAME_DLL
CON_COMMAND(sv_dumpmodelevents, "")
#else
CON_COMMAND(cl_dumpmodelevents, "")
#endif
{
	for(int i = 0; i < g_EventList.Count(); ++i) {
		DevMsg("%s - %i ", g_EventStrings.GetStringForKey(g_EventList[i].stringKey), g_EventList[i].eventIndex);
		if((g_EventList[i].iType & AE_TYPE_SERVER) != 0) {
			DevMsg("AE_TYPE_SERVER|");
		}
		if((g_EventList[i].iType & AE_TYPE_SCRIPTED) != 0) {
			DevMsg("AE_TYPE_SCRIPTED|");
		}
		if((g_EventList[i].iType & AE_TYPE_SHARED) != 0) {
			DevMsg("AE_TYPE_SHARED|");
		}
		if((g_EventList[i].iType & AE_TYPE_WEAPON) != 0) {
			DevMsg("AE_TYPE_WEAPON|");
		}
		if((g_EventList[i].iType & AE_TYPE_CLIENT) != 0) {
			DevMsg("AE_TYPE_CLIENT|");
		}
		if((g_EventList[i].iType & AE_TYPE_FACEPOSER) != 0) {
			DevMsg("AE_TYPE_FACEPOSER|");
		}
		DevMsg(" %i\n", g_EventList[i].isPrivate);
	}
}