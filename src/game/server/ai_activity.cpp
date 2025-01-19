//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Activities that are available to all NPCs.
//
//=============================================================================//

#include "cbase.h"
#include "ai_activity.h"
#include "ai_basenpc.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Init static variables
//=============================================================================
CStringRegistry* CAI_BaseNPC::m_pActivitySR	= NULL;
int				 CAI_BaseNPC::m_iNumActivities = 0;

#ifdef _DEBUG
static bool g_bRegisteringAliases = false;
#endif

//-----------------------------------------------------------------------------
// Purpose: Add an activity to the activity string registry and increment
//			the acitivty counter
//-----------------------------------------------------------------------------
void CAI_BaseNPC::AddActivityToSR(const char *actName, Activity actID) 
{
	Assert( m_pActivitySR );
	if ( !m_pActivitySR )
		return;

	// technically order isn't dependent, but it's too damn easy to forget to add new ACT_'s to all three lists.

	// NOTE: This assertion generally means that the activity enums are out of order or that new enums were not added to all
	//		 relevant tables.  Make sure that you have included all new enums in:
	//			game_shared/ai_activity.h
	//			game_shared/activitylist.cpp
	//			dlls/ai_activity.cpp
	MEM_ALLOC_CREDIT();

#ifdef _DEBUG
	if(!g_bRegisteringAliases) {
		static unsigned short lastActID = (unsigned short)-3;
		Assert( lastActID == (unsigned short)-3 || ((unsigned short)actID < (unsigned short)LAST_SHARED_ACTIVITY && actID == (Activity)(lastActID + 1)) );
		lastActID = (unsigned short)actID;
	}
#endif

	m_pActivitySR->AddString(actName, actID);
	m_iNumActivities++;
}

//-----------------------------------------------------------------------------
// Purpose: Given and activity ID, return the activity name
//-----------------------------------------------------------------------------
const char *CAI_BaseNPC::GetActivityName(Activity actID) 
{
	if ( actID == ACT_INVALID )
		return "ACT_INVALID";

	// m_pActivitySR only contains public activities, ActivityList_NameForIndex() has them all
	const char *name = ActivityList_NameForIndex(actID);	

	if( !name )
	{
		AssertOnce( !"CAI_BaseNPC::GetActivityName() returning NULL!" );
	}

	return name;
}

//-----------------------------------------------------------------------------
// Purpose: Gets an activity ID or registers a new private one if it doesn't exist
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetOrRegisterActivity( const char *actName )
{
	Activity actID = GetActivityID( actName );
	if (actID == ACT_INVALID)
	{
		actID = ActivityList_RegisterPrivateActivity( actName );
		AddActivityToSR( actName, actID );
	}

	return actID;
}

//-----------------------------------------------------------------------------
// Purpose: Given and activity name, return the activity ID
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetActivityID(const char* actName) 
{
	Assert( m_pActivitySR );
	if ( !m_pActivitySR )
		return ACT_INVALID;

	return (Activity)m_pActivitySR->GetStringID(actName);
}

#define ADD_ACTIVITY_TO_SR(activityname) AddActivityToSR(#activityname,activityname)

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitDefaultActivitySR(void) 
{
	#define ACTIVITY_ENUM(name, ...) \
		ADD_ACTIVITY_TO_SR( name );
	#define ACTIVITY_ENUM_ALIAS(name, value)

	#include "ai_activity_enum.inc"

	#define ACTIVITY_ENUM(name, ...)
	#define ACTIVITY_ENUM_ALIAS(name, value) \
		AddActivityToSR( #name, value );

#ifdef _DEBUG
	g_bRegisteringAliases = true;
#endif
	#include "ai_activity_enum.inc"
#ifdef _DEBUG
	g_bRegisteringAliases = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: This is a multi-purpose table which links NPC activities to their gesture variants.
//-----------------------------------------------------------------------------
const CAI_BaseNPC::actlink_t CAI_BaseNPC::gm_ActivityGestureLinks[] =
{
	#define ACTIVITY_ENUM_GESTURE_LINK(name, gesture) \
		{ name, gesture },
	#define ACTIVITY_ENUM_GESTURE_LINK_REMAP(name, gesture) \
		{ name, gesture },

	#include "ai_activity_enum.inc"
};

Activity CAI_BaseNPC::GetGestureVersionOfActivity( Activity inActivity )
{
	const actlink_t *pTable = gm_ActivityGestureLinks;
	int actCount = ARRAYSIZE( gm_ActivityGestureLinks );

	for ( int i = 0; i < actCount; i++, pTable++ )
	{
		if ( inActivity == pTable->sequence )
		{
			return pTable->gesture;
		}
	}

	return ACT_INVALID;
}

Activity CAI_BaseNPC::GetSequenceVersionOfGesture( Activity inActivity )
{
	const actlink_t *pTable = gm_ActivityGestureLinks;
	int actCount = ARRAYSIZE( gm_ActivityGestureLinks );

	for (int i = 0; i < actCount; i++, pTable++)
	{
		if (inActivity == pTable->gesture)
		{
			return pTable->sequence;
		}
	}

	return ACT_INVALID;
}

CON_COMMAND(dumpactivitygesturelinks, "")
{
	for(int i = 0; i < ARRAYSIZE( CAI_BaseNPC::gm_ActivityGestureLinks ); ++i) {
		DevMsg("%s %i - %s %i\n", CAI_BaseNPC::GetActivityName(CAI_BaseNPC::gm_ActivityGestureLinks[i].sequence), CAI_BaseNPC::gm_ActivityGestureLinks[i].sequence, CAI_BaseNPC::GetActivityName(CAI_BaseNPC::gm_ActivityGestureLinks[i].gesture), CAI_BaseNPC::gm_ActivityGestureLinks[i].gesture);
	}
}