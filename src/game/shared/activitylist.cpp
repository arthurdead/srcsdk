//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ai_activity.h"
#include "activitylist.h"
#include "stringregistry.h"

#include "filesystem.h"
#include <KeyValues.h>
#include "utldict.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IFileSystem *filesystem;

// NOTE: If CStringRegistry allowed storing arbitrary data, we could just use that.
// in this case we have the "isPrivate" member and the replacement rules 
// (activityIndex can be reused by private activities), so a custom table is necessary
struct activitylist_t
{
	Activity			activityIndex;
	unsigned short		stringKey;
	bool				isPrivate;
};

CUtlVector<activitylist_t> g_ActivityList;

static CUtlDict< CActivityRemapCache, int > m_ActivityRemapDatabase;

// This stores the actual activity names.  Also, the string ID in the registry is simply an index 
// into the g_ActivityList array.
CStringRegistry	g_ActivityStrings;

// this is just here to accelerate adds
static Activity g_HighestActivity = (Activity)0;

int g_nActivityListVersion = 1;


void ActivityList_Init( void )
{
	g_HighestActivity = (Activity)0;
}

void ActivityList_Free( void )
{
	g_ActivityStrings.ClearStrings();
	g_ActivityList.Purge();
	m_ActivityRemapDatabase.Purge();

	// So studiohdrs can reindex activity indices
	++g_nActivityListVersion;
}

// add a new activity to the database
activitylist_t *ActivityList_AddActivityEntry( const char *pName, Activity iActivityIndex, bool isPrivate )
{
	MEM_ALLOC_CREDIT();
	int index = g_ActivityList.AddToTail();
	activitylist_t *pList = &g_ActivityList[index];
	pList->activityIndex = iActivityIndex;
	pList->stringKey = g_ActivityStrings.AddString( pName, index );
	pList->isPrivate = isPrivate;
	
	// UNDONE: This implies that ALL shared activities are added before ANY custom activities
	// UNDONE: Segment these instead?  It's a 32-bit int, how many activities do we need?
	if ( iActivityIndex > g_HighestActivity )
	{
		g_HighestActivity = iActivityIndex;
	}

	return pList;
}

// get the database entry from a string
static activitylist_t *ListFromString( const char *pString )
{
	// just use the string registry to do this search/map
	int stringID = g_ActivityStrings.GetStringID( pString );
	if ( stringID < 0 )
		return NULL;

	return &g_ActivityList[stringID];
}

// Get the database entry for an index
static activitylist_t *ListFromActivity( Activity activityIndex )
{
	// ugly linear search
	for ( int i = 0; i < g_ActivityList.Size(); i++ )
	{
		if ( g_ActivityList[i].activityIndex == activityIndex )
		{
			return &g_ActivityList[i];
		}
	}

	return NULL;
}

bool ActivityList_RegisterSharedActivity( const char *pszActivityName, Activity iActivityIndex )
{
	// UNDONE: Do we want to do these checks when not in developer mode? or maybe DEBUG only?
	// They really only matter when you change the list of code controlled activities.  IDs
	// for content controlled activities never collide because they are generated.

#ifdef _DEBUG
	// technically order isn't dependent, but it's too damn easy to forget to add new ACT_'s to all three lists.
	static Activity lastActivityIndex = (Activity)-1;
	Assert( iActivityIndex < LAST_SHARED_ACTIVITY && (iActivityIndex == lastActivityIndex + 1 || iActivityIndex == 0) );
	lastActivityIndex = iActivityIndex;
#endif

#if defined _DEBUG && 0 //need to allow aliases
	// first, check to make sure the slot we're asking for is free. It must be for 
	// a shared activity.
	activitylist_t *pList = ListFromString( pszActivityName );
	if ( !pList )
	{
		pList = ListFromActivity( iActivityIndex );
	}

	if ( pList )
	{
		Warning( "***\nShared activity collision! %s<->%s\n***\n", pszActivityName, g_ActivityStrings.GetStringForKey( pList->stringKey ) );
		Assert(0);
		return false;
	}
	// ----------------------------------------------------------------
#endif

	return ( ActivityList_AddActivityEntry( pszActivityName, iActivityIndex, false ) != NULL );
}


Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName )
{
	activitylist_t *pList = ListFromString( pszActivityName );
	if ( pList )
	{
		// this activity is already in the list. If the activity we collided with is also private, 
		// then the collision is OK. Otherwise, it's a bug.
		if ( pList->isPrivate )
		{
			return (Activity)pList->activityIndex;
		}
		else
		{
			// this private activity collides with a shared activity. That is not allowed.
			Warning( "***\nShared<->Private Activity collision!\n***\n" );
			Assert(0);
			return ACT_INVALID;
		}
	}

	pList = ActivityList_AddActivityEntry( pszActivityName, (Activity)(g_HighestActivity+1), true );
	return (Activity)pList->activityIndex;
}

// Get the index for a given activity name
// Done at load time for all models
Activity ActivityList_IndexForName( const char *pszActivityName )
{
	// this is a fast O(lgn) search (actually does 2 O(lgn) searches)
	activitylist_t *pList = ListFromString( pszActivityName );

	if ( pList )
	{
		return pList->activityIndex;
	}

	return (Activity)kActivityLookup_Missing;
}

// Get the name for a given index
// This should only be used in debug code, it does a linear search
// But at least it only compares integers
const char *ActivityList_NameForIndex( Activity activityIndex )
{
	activitylist_t *pList = ListFromActivity( activityIndex );
	if ( pList )
	{
		return g_ActivityStrings.GetStringForKey( pList->stringKey );
	}
	return NULL;
}

void ActivityList_RegisterSharedActivities( void )
{
	#define ACTIVITY_ENUM(name, ...) \
		REGISTER_SHARED_ACTIVITY( name );
	#define ACTIVITY_ENUM_ALIAS(name, value) \
		ActivityList_RegisterSharedActivity( #name, value );

	#include "ai_activity_enum.inc"

	AssertMsg( g_HighestActivity == LAST_SHARED_ACTIVITY - 1, "Not all activities from ai_activity.h registered in activitylist.cpp" ); 

	Assert(g_HighestActivity <= (Activity)(unsigned short)-1);
} 


void UTIL_LoadActivityRemapFile( const char *filename, const char *section, CUtlVector <CActivityRemap> &entries )
{
	int iIndex = m_ActivityRemapDatabase.Find( filename );

	if ( iIndex != m_ActivityRemapDatabase.InvalidIndex() )
	{
		CActivityRemapCache *actRemap = &m_ActivityRemapDatabase[iIndex];
		entries.AddVectorToTail( actRemap->m_cachedActivityRemaps );
		return;
	}

	KeyValues *pkvFile = new KeyValues( section );

	if ( pkvFile->LoadFromFile( filesystem, filename, NULL ) )
	{
		KeyValues *pTestKey = pkvFile->GetFirstSubKey();

		CActivityRemapCache actRemap;

		while ( pTestKey )
		{
			Activity ActBase = (Activity)ActivityList_IndexForName( pTestKey->GetName() );

			if ( ActBase != ACT_INVALID )
			{
				KeyValues *pRemapKey = pTestKey->GetFirstSubKey();

				CActivityRemap actMap;
				actMap.activity = ActBase;

				while ( pRemapKey )
				{
					const char *pKeyName = pRemapKey->GetName();
					const char *pKeyValue = pRemapKey->GetString();

					if ( !stricmp( pKeyName, "remapactivity" ) )
					{
						Activity Act = (Activity)ActivityList_IndexForName( pKeyValue );

						if ( Act == ACT_INVALID )
						{
							actMap.mappedActivity = ActivityList_RegisterPrivateActivity( pKeyValue );
						}
						else
						{
							actMap.mappedActivity = Act;
						}
					}
					else if ( !stricmp( pKeyName, "extra" ) )
					{
						actMap.SetExtraKeyValueBlock( pRemapKey->MakeCopy() );
					}

					pRemapKey = pRemapKey->GetNextKey();
				}

				entries.AddToTail( actMap );
			}

			pTestKey = pTestKey->GetNextKey();
		}

		actRemap.m_cachedActivityRemaps.AddVectorToTail( entries );
		m_ActivityRemapDatabase.Insert( filename, actRemap );
	}
}

Activity ActivityList_HighestIndex()
{
	return g_HighestActivity;
}

#ifdef GAME_DLL
CON_COMMAND(sv_dumpmodelactivites, "")
#else
CON_COMMAND(cl_dumpmodelactivites, "")
#endif
{
	for(int i = 0; i < g_ActivityList.Count(); ++i) {
		DevMsg("%s - %i %i\n", g_ActivityStrings.GetStringForKey(g_ActivityList[i].stringKey), g_ActivityList[i].activityIndex, g_ActivityList[i].isPrivate);
	}
}