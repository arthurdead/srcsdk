#include "entityoutput.h"
#include "eventqueue.h"
#include "entityinput.h"
#include "gamestringpool.h"
#include "entitydefs.h"
#include "tier0/vprof.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
#include "cdll_util.h"
#include "cdll_client_int.h"
#include "c_baseentity.h"
#else
#include "util.h"
#include "baseentity.h"
#include "env_debughistory.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_ENTITYIO, "Entity I/O Server" );
#else
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_ENTITYIO, "Entity I/O Client" );
#endif

// this memory pool stores blocks around the size of CEventAction/inputitem_t structs
// can be used for other blocks; will error if to big a block is tried to be allocated
CUtlMemoryPool g_EntityListPool( MAX(sizeof(CEventAction),sizeof(CMultiInputVar::inputitem_t)), 512, CUtlMemoryPool::GROW_FAST, "g_EntityListPool" );

// ID Stamp used to uniquely identify every output
int CEventAction::s_iNextIDStamp = 0;


//-----------------------------------------------------------------------------
// Purpose: Creates an event action and assigns it an unique ID stamp.
// Input  : ActionData - the map file data block descibing the event action.
//-----------------------------------------------------------------------------
CEventAction::CEventAction( const char *ActionData )
{
	m_pNext = NULL;
	m_iIDStamp = ++s_iNextIDStamp;

	m_flDelay = 0;
	m_iTarget = NULL_STRING;
	m_iParameter = NULL_STRING;
	m_iTargetInput = NULL_STRING;
	m_nTimesToFire = EVENT_FIRE_ALWAYS;

	if (ActionData == NULL)
		return;

	char szToken[256];

	char chDelim = VMF_IOPARAM_STRING_DELIMITER;
	if (!strchr(ActionData, VMF_IOPARAM_STRING_DELIMITER))
	{
		chDelim = ',';
	}

	//
	// Parse the target name.
	//
	const char *psz = nexttoken(szToken, ActionData, chDelim, sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iTarget = AllocPooledString(szToken);
	}

	//
	// Parse the input name.
	//
	psz = nexttoken(szToken, psz, chDelim, sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iTargetInput = AllocPooledString(szToken);
	}
	else
	{
		m_iTargetInput = AllocPooledString("Use");
	}

	//
	// Parse the parameter override.
	//
	psz = nexttoken(szToken, psz, chDelim, sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iParameter = AllocPooledString(szToken);
	}

	//
	// Parse the delay.
	//
	psz = nexttoken(szToken, psz, chDelim, sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_flDelay = atof(szToken);
	}

	//
	// Parse the number of times to fire.
	//
	psz = nexttoken(szToken, psz, chDelim, sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_nTimesToFire = atoi(szToken);
		if (m_nTimesToFire == 0)
		{
			m_nTimesToFire = EVENT_FIRE_ALWAYS;
		}
	}
}

CEventAction::CEventAction( const CEventAction &p_EventAction )
{
	m_pNext = NULL;
	m_iIDStamp = ++s_iNextIDStamp;

	m_flDelay = p_EventAction.m_flDelay;
	m_iTarget = p_EventAction.m_iTarget;
	m_iParameter = p_EventAction.m_iParameter;
	m_iTargetInput = p_EventAction.m_iTargetInput;
	m_nTimesToFire = p_EventAction.m_nTimesToFire;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the highest-valued delay in our list of event actions.
//-----------------------------------------------------------------------------
float CBaseEntityOutput::GetMaxDelay(void)
{
	float flMaxDelay = 0;
	CEventAction *ev = m_ActionList;

	while (ev != NULL)
	{
		if (ev->m_flDelay > flMaxDelay)
		{
			flMaxDelay = ev->m_flDelay;
		}
		ev = ev->m_pNext;
	}

	return(flMaxDelay);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CBaseEntityOutput::~CBaseEntityOutput()
{
	CEventAction *ev = m_ActionList;
	while (ev != NULL)
	{
		CEventAction *pNext = ev->m_pNext;	
		delete ev;
		ev = pNext;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fires the event, causing a sequence of action to occur in other ents.
// Input  : pActivator - Entity that initiated this sequence of actions.
//			pCaller - Entity that is actually causing the event.
//-----------------------------------------------------------------------------
void CBaseEntityOutput::FireOutput(variant_t Value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay)
{
	//
	// Iterate through all eventactions and fire them off.
	//
	CEventAction *ev = m_ActionList;
	CEventAction *prev = NULL;
	
	while (ev != NULL)
	{
		if (ev->m_iParameter == NULL_STRING)
		{
			//
			// Post the event with the default parameter.
			//
			g_EventQueue.AddEvent( STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), Value, ev->m_flDelay + fDelay, pActivator, pCaller, ev->m_iIDStamp );
		}
		else
		{
			//
			// Post the event with a parameter override.
			//
			variant_t ValueOverride;
			ValueOverride.SetString( ev->m_iParameter );
			// I found this while making point_advanced_finder. FireOutput()'s own delay parameter doesn't work with...uh...parameters.
			g_EventQueue.AddEvent( STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), ValueOverride, ev->m_flDelay + fDelay, pActivator, pCaller, ev->m_iIDStamp );
		}

		if ( ev->m_flDelay )
		{
			char szBuffer[256];
			Q_snprintf( szBuffer,
						sizeof(szBuffer),
						"(%0.2f) output: (%s,%s) -> (%s,%s,%.1f)(%s)\n",
#ifdef TF_DLL
						engine->GetServerTime(),
#else
						gpGlobals->curtime,
#endif
						pCaller ? pCaller->GetClassname() : "NULL",
						pCaller ? pCaller->GetEntityNameAsCStr() : "NULL",
						STRING(ev->m_iTarget),
						STRING(ev->m_iTargetInput),
						ev->m_flDelay,
						STRING(ev->m_iParameter) );

			Log_Msg( LOG_ENTITYIO, "%s", szBuffer );
		#ifdef GAME_DLL
			ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );
		#endif
		}
		else
		{
			char szBuffer[256];
			Q_snprintf( szBuffer,
						sizeof(szBuffer),
						"(%0.2f) output: (%s,%s) -> (%s,%s)(%s)\n",
#ifdef TF_DLL
						engine->GetServerTime(),
#else
						gpGlobals->curtime,
#endif
						pCaller ? pCaller->GetClassname() : "NULL",
						pCaller ? pCaller->GetEntityNameAsCStr() : "NULL", STRING(ev->m_iTarget),
						STRING(ev->m_iTargetInput),
						STRING(ev->m_iParameter) );

			Log_Msg( LOG_ENTITYIO, "%s", szBuffer );
		#ifdef GAME_DLL
			ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );
		#endif
		}

	#ifdef GAME_DLL
		if ( pCaller && pCaller->m_debugOverlays & OVERLAY_MESSAGE_BIT)
		{
			pCaller->DrawOutputOverlay(ev);
		}
	#endif

		//
		// Remove the event action from the list if it was set to be fired a finite
		// number of times (and has been).
		//
		bool bRemove = false;
		if (ev->m_nTimesToFire != EVENT_FIRE_ALWAYS)
		{
			ev->m_nTimesToFire--;
			if (ev->m_nTimesToFire == 0)
			{
				char szBuffer[256];
				Q_snprintf( szBuffer, sizeof(szBuffer), "Removing from action list: (%s,%s) -> (%s,%s)\n", pCaller ? pCaller->GetClassname() : "NULL", pCaller ? pCaller->GetEntityNameAsCStr() : "NULL", STRING(ev->m_iTarget), STRING(ev->m_iTargetInput));
				Log_Msg( LOG_ENTITYIO, "%s", szBuffer );
			#ifdef GAME_DLL
				ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );
			#endif
				bRemove = true;
			}
		}

		if (!bRemove)
		{
			prev = ev;
			ev = ev->m_pNext;
		}
		else
		{
			if (prev != NULL)
			{
				prev->m_pNext = ev->m_pNext;
			}
			else
			{
				m_ActionList = ev->m_pNext;
			}

			CEventAction *next = ev->m_pNext;
			delete ev;
			ev = next;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Parameterless firing of an event
// Input  : pActivator - 
//			pCaller - 
//-----------------------------------------------------------------------------
void COutputEvent::FireOutput(CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay)
{
	variant_t Val;
	Val.Set( FIELD_VOID, NULL );
	CBaseEntityOutput::FireOutput(Val, pActivator, pCaller, fDelay);
}


void CBaseEntityOutput::ParseEventAction( const char *EventData )
{
	AddEventAction( new CEventAction( EventData ) );
}

void CBaseEntityOutput::AddEventAction( CEventAction *pEventAction )
{
	pEventAction->m_pNext = m_ActionList;
	m_ActionList = pEventAction;
}

void CBaseEntityOutput::RemoveEventAction( CEventAction *pEventAction )
{
	CEventAction *pAction = GetFirstAction();
	CEventAction *pPrevAction = NULL;
	while ( pAction )
	{
		if ( pAction == pEventAction )
		{
			if ( !pPrevAction )
			{
				m_ActionList = NULL;
			}
			else
			{
				pPrevAction->m_pNext = pAction->m_pNext;
			}
			return;
		}
		pAction = pAction->m_pNext;
	}
}

const CEventAction *CBaseEntityOutput::GetActionForTarget( string_t iSearchTarget ) const
{
	for ( CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext )
	{
		if ( ev->m_iTarget == iSearchTarget )
			return ev;
	}

	return NULL;
}

int CBaseEntityOutput::NumberOfElements( void )
{
	int count = 0;
	for ( CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext )
	{
		count++;
	}
	return count;
}

/// Delete every single action in the action list. 
void CBaseEntityOutput::DeleteAllElements( void ) 
{
	// walk front to back, deleting as we go. We needn't fix up pointers because
	// EVERYTHING will die.

	CEventAction *pNext = m_ActionList;
	// wipe out the head
	m_ActionList = NULL;
	while (pNext)
	{
		CEventAction *strikeThis = pNext;
		pNext = pNext->m_pNext;
		delete strikeThis;
	}

}

/// EVENTS save/restore parsing wrapper

class CEventsSaveDataOps : public ICustomFieldOps
{
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CEventsSaveDataOps does not support arrays");
		
		// check all the elements of the array (usually only 1)
		CBaseEntityOutput *ev = (CBaseEntityOutput*)fieldInfo.pField;
		const int fieldSize = fieldInfo.pTypeDesc->fieldSize;
		for ( int i = 0; i < fieldSize; i++, ev++ )
		{
			// It's not empty if it has events or if it has a non-void variant value
			if (( ev->NumberOfElements() != 0 ) || ( ev->ValueFieldType() != FIELD_VOID ))
				return 0;
		}

		// variant has no data
		return 1;
	}

	virtual void MakeEmpty( const FieldInfo_t &fieldInfo )
	{
		// Don't no how to. This is okay, since objects of this type
		// are always born clean before restore, and not reused
	}

	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue )
	{
		CBaseEntityOutput *ev = (CBaseEntityOutput*)fieldInfo.pField;
		ev->ParseEventAction( szValue );
		return true;
	}
};

CEventsSaveDataOps g_EventsDataOps;
ICustomFieldOps *eventFuncs = &g_EventsDataOps;

//-----------------------------------------------------------------------------
//			CMultiInputVar implementation
//
// Purpose: holds a list of inputs and their ID tags
//			used for entities that hold inputs from a set of other entities
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: destructor, frees the data list
//-----------------------------------------------------------------------------
CMultiInputVar::~CMultiInputVar()
{
	if ( m_InputList )
	{
		while ( m_InputList->next != NULL )
		{
			inputitem_t *input = m_InputList->next;
			m_InputList->next = input->next;
			delete input;
		}
		delete m_InputList;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the data set with a new value
// Input  : newVal - the new value to add to or update in the list
//			outputID - the source of the value
//-----------------------------------------------------------------------------
void CMultiInputVar::AddValue( variant_t newVal, int outputID )
{
	// see if it's already in the list
	inputitem_t *inp;
	for ( inp = m_InputList; inp != NULL; inp = inp->next )
	{
		// already in list, so just update this link
		if ( inp->outputID == outputID )
		{
			inp->value = newVal;
			return;
		}
	}

	// add to start of list
	inp = new inputitem_t;
	inp->value = newVal;
	inp->outputID = outputID;
	if ( !m_InputList )
	{
		m_InputList = inp;
		inp->next = NULL;
	}
	else
	{
		inp->next = m_InputList;
		m_InputList = inp;
	}
}


//-----------------------------------------------------------------------------
//			CEventQueue implementation
//
// Purpose: holds and executes a global prioritized queue of entity actions
//-----------------------------------------------------------------------------
DEFINE_FIXEDSIZE_ALLOCATOR( EventQueuePrioritizedEvent_t, 128, CUtlMemoryPool::GROW_SLOW );

CEventQueue g_EventQueue;

CEventQueue::CEventQueue()
{
	m_Events.m_flFireTime = -FLT_MAX;
	m_Events.m_pNext = NULL;

	Init();
}

CEventQueue::~CEventQueue()
{
	Clear();
}

void CEventQueue::Init( void )
{
	Clear();
}

void CEventQueue::Clear( void )
{
	// delete all the events in the queue
	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;
	
	while ( pe != NULL )
	{
		EventQueuePrioritizedEvent_t *next = pe->m_pNext;
		delete pe;
		pe = next;
	}

	m_Events.m_pNext = NULL;
}

void CEventQueue::Dump( void )
{
	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

	Log_Msg( LOG_ENTITYIO, "Dumping event queue. Current time is: %.2f\n",
#ifdef TF_DLL
		engine->GetServerTime()
#else
		gpGlobals->curtime
#endif
		);

	while ( pe != NULL )
	{
		EventQueuePrioritizedEvent_t *next = pe->m_pNext;

		Log_Msg( LOG_ENTITYIO, "   (%.2f) Target: '%s', Input: '%s', Parameter '%s'. Activator: '%s', Caller '%s'.  \n", 
			pe->m_flFireTime, 
			STRING(pe->m_iTarget), 
			STRING(pe->m_iTargetInput), 
			pe->m_VariantValue.String(),
			pe->m_pActivator ? pe->m_pActivator->GetDebugName() : "None", 
			pe->m_pCaller ? pe->m_pCaller->GetDebugName() : "None"  );

		pe = next;
	}

	Log_Msg( LOG_ENTITYIO, "Finished dump.\n");
}


//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via string name
//-----------------------------------------------------------------------------
EventQueuePrioritizedEvent_t *CEventQueue::AddEvent( const char *target, const char *targetInput, variant_t Value, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID )
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
#ifdef TF_DLL
	newEvent->m_flFireTime = engine->GetServerTime() + fireDelay;	// priority key in the priority queue
#else
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// priority key in the priority queue
#endif
	newEvent->m_iTarget = MAKE_STRING( target );
	newEvent->m_pEntTarget = NULL;
	newEvent->m_iTargetInput = MAKE_STRING( targetInput );
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent( newEvent );

	return newEvent;
}

//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via pointer
//-----------------------------------------------------------------------------
EventQueuePrioritizedEvent_t *CEventQueue::AddEvent( CSharedBaseEntity *target, const char *targetInput, variant_t Value, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID )
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
#ifdef TF_DLL
	newEvent->m_flFireTime = engine->GetServerTime() + fireDelay;	// primary priority key in the priority queue
#else
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// primary priority key in the priority queue
#endif
	newEvent->m_iTarget = NULL_STRING;
	newEvent->m_pEntTarget = target;
	newEvent->m_iTargetInput = MAKE_STRING( targetInput );
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent( newEvent );

	return newEvent;
}

EventQueuePrioritizedEvent_t *CEventQueue::AddEvent( CSharedBaseEntity *target, const char *action, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID )
{
	variant_t Value;
	Value.Set( FIELD_VOID, NULL );
	EventQueuePrioritizedEvent_t *newEvent = AddEvent( target, action, Value, fireDelay, pActivator, pCaller, outputID );
	return newEvent;
}


//-----------------------------------------------------------------------------
// Purpose: private function, adds an event into the list
// Input  : *newEvent - the (already built) event to add
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent( EventQueuePrioritizedEvent_t *newEvent )
{
	// loop through the actions looking for a place to insert
	EventQueuePrioritizedEvent_t *pe;
	for ( pe = &m_Events; pe->m_pNext != NULL; pe = pe->m_pNext )
	{
		if ( pe->m_pNext->m_flFireTime > newEvent->m_flFireTime )
		{
			break;
		}
	}

	Assert( pe );

	// insert
	newEvent->m_pNext = pe->m_pNext;
	newEvent->m_pPrev = pe;
	pe->m_pNext = newEvent;
	if ( newEvent->m_pNext )
	{
		newEvent->m_pNext->m_pPrev = newEvent;
	}
}

void CEventQueue::RemoveEvent( EventQueuePrioritizedEvent_t *pe )
{
	Assert( pe->m_pPrev );
	pe->m_pPrev->m_pNext = pe->m_pNext;
	if ( pe->m_pNext )
	{
		pe->m_pNext->m_pPrev = pe->m_pPrev;
	}
}


//-----------------------------------------------------------------------------
// Purpose: fires off any events in the queue who's fire time is (or before) the present time
//-----------------------------------------------------------------------------
void CEventQueue::ServiceEvents( void )
{
#ifdef GAME_DLL
	if (!CBaseEntity::Debug_ShouldStep())
	{
		return;
	}
#endif

	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

#ifdef TF_DLL
	while ( pe != NULL && pe->m_flFireTime <= engine->GetServerTime() )
#else
	while ( pe != NULL && pe->m_flFireTime <= gpGlobals->curtime )
#endif
	{
		MDLCACHE_CRITICAL_SECTION();

		bool targetFound = false;

		// find the targets
		if ( pe->m_iTarget != NULL_STRING )
		{
			// In the context the event, the searching entity is also the caller
			CSharedBaseEntity *pSearchingEntity = pe->m_pCaller;

			// This is a hack to access the entity from a FIELD_EHANDLE input
			if ( FStrEq( STRING( pe->m_iTarget ), "!output" ) )
			{
				pe->m_VariantValue.Convert( FIELD_EHANDLE );
				CSharedBaseEntity *target = pe->m_VariantValue.Entity();

				// pump the action into the target
				target->AcceptInput( STRING( pe->m_iTargetInput ), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID );
				targetFound = true;
			}
			else
			{
				CSharedBaseEntity *target = NULL;
				while ( 1 )
				{
					target = g_pEntityList->FindEntityByName( target, pe->m_iTarget, pSearchingEntity, pe->m_pActivator, pe->m_pCaller );
					if ( !target )
						break;

					// pump the action into the target
					target->AcceptInput( STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID );
					targetFound = true;
				}
			}
		}

		// direct pointer
		if ( pe->m_pEntTarget != NULL )
		{
			pe->m_pEntTarget->AcceptInput( STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID );
			targetFound = true;
		}

		if ( !targetFound )
		{
			// See if we can find a target if we treat the target as a classname
			if ( pe->m_iTarget != NULL_STRING )
			{
				CSharedBaseEntity *target = NULL;
				while ( 1 )
				{
					target = g_pEntityList->FindEntityByClassname( target, STRING(pe->m_iTarget) );
					if ( !target )
						break;

					// pump the action into the target
					target->AcceptInput( STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID );
					targetFound = true;
				}
			}
		}

		if ( !targetFound )
		{
			const char *pClass ="", *pName = "";
			
			// might be NULL
			if ( pe->m_pCaller )
			{
				pClass = pe->m_pCaller->GetClassname();
				pName = pe->m_pCaller->GetEntityNameAsCStr();
			}
			
			char szBuffer[256];
			Q_snprintf( szBuffer, sizeof(szBuffer), "unhandled input: (%s) -> (%s), from (%s,%s); target entity not found\n", STRING(pe->m_iTargetInput), STRING(pe->m_iTarget), pClass, pName );
			Log_Warning( LOG_ENTITYIO, "%s", szBuffer );
		#ifdef GAME_DLL
			ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );
		#endif
		}

		// remove the event from the list (remembering that the queue may have been added to)
		RemoveEvent( pe );
		delete pe;

	#ifdef GAME_DLL
		//
		// If we are in debug mode, exit the loop if we have fired the correct number of events.
		//
		if (CBaseEntity::Debug_IsPaused())
		{
			if (!CBaseEntity::Debug_Step())
			{
				break;
			}
		}
	#endif

		// restart the list (to catch any new items have probably been added to the queue)
		pe = m_Events.m_pNext;	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dumps the contents of the Entity I/O event queue to the console.
//-----------------------------------------------------------------------------
CON_COMMAND_SHARED(dumpeventqueue, "Dump the contents of the Entity I/O event queue to the console.")
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	g_EventQueue.Dump();
}

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events from the I/O queue that were added by the
//			given caller.
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEvents( CSharedBaseEntity *pCaller )
{
	if (!pCaller)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while (pCur != NULL)
	{
		bool bDelete = false;
		if (pCur->m_pCaller == pCaller)
		{
			// Pointers match; make sure everything else matches.
			if (IDENT_STRINGS(pCur->m_pCaller->GetEntityNameAsTStr(), pCaller->GetEntityNameAsTStr()) &&
				IDENT_STRINGS(pCur->m_pCaller->GetClassnameStr(), pCaller->GetClassnameStr()))
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent( pCurSave );
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events of the specified type from the I/O queue of the specified target
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEventOn( CSharedBaseEntity *pTarget, const char *sInputName )
{
	if (!pTarget)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	int inputLen = strlen(sInputName);

	while (pCur != NULL)
	{
		bool bDelete = false;
		if (pCur->m_pEntTarget == pTarget)
		{
			if ( !Q_strncmp( STRING(pCur->m_iTargetInput), sInputName, inputLen ) )
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent( pCurSave );
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the target has any pending inputs.
// Input  : *pTarget - 
//			*sInputName - NULL for any input, or a specified one
//-----------------------------------------------------------------------------
bool CEventQueue::HasEventPending( CSharedBaseEntity *pTarget, const char *sInputName )
{
	if (!pTarget)
		return false;

	if ( !sInputName )
		return true;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	int inputLen = strlen(sInputName);

	while (pCur != NULL)
	{
		if (pCur->m_pEntTarget == pTarget)
		{
			if ( !sInputName )
				return true;

			if ( !Q_strncmp( STRING(pCur->m_iTargetInput), sInputName, inputLen ) )
				return true;
		}

		pCur = pCur->m_pNext;
	}

	return false;
}

void ServiceEventQueue( void )
{
	VPROF("ServiceEventQueue()");

	g_EventQueue.ServiceEvents();
}

//-----------------------------------------------------------------------------
// Remove pending events on entity by input.
//
// Also removes events that were targeted with their debug name (classname when unnamed).
// E.g. CancelEventsByInput( pRelay, "Trigger" ) removes all pending logic_relay "Trigger" events.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEventsByInput( CSharedBaseEntity *pTarget, const char *szInput )
{
	if ( !pTarget )
		return;

	string_t iszDebugName = MAKE_STRING( pTarget->GetDebugName() );
	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while ( pCur )
	{
		bool bRemove = false;

		if ( pTarget == pCur->m_pEntTarget || pCur->m_iTarget == iszDebugName )
		{
			if ( !V_strncmp( STRING(pCur->m_iTargetInput), szInput, strlen(szInput) ) )
			{
				bRemove = true;
			}
		}

		EventQueuePrioritizedEvent_t *pPrev = pCur;
		pCur = pCur->m_pNext;

		if ( bRemove )
		{
			DeleteEvent(pPrev);
		}
	}
}

void CEventQueue::DeleteEvent( EventQueuePrioritizedEvent_t *pe )
{
	Assert( pe );
	RemoveEvent(pe);
	delete pe;
}

float CEventQueue::GetTimeLeft( EventQueuePrioritizedEvent_t *pe )
{
	Assert( pe );
	return (pe->m_flFireTime - gpGlobals->curtime);
}

#pragma push_macro("new")
#pragma push_macro("delete")
#undef new
#undef delete

//-----------------------------------------------------------------------------
// Purpose: allocates memory from the entitylist pool
//-----------------------------------------------------------------------------
void *CMultiInputVar::inputitem_t::operator new( size_t stAllocateBlock )
{
	return g_EntityListPool.Alloc( stAllocateBlock );
}

void *CMultiInputVar::inputitem_t::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	return g_EntityListPool.Alloc( stAllocateBlock );
}

//-----------------------------------------------------------------------------
// Purpose: frees memory from the entitylist pool
//-----------------------------------------------------------------------------
void CMultiInputVar::inputitem_t::operator delete( void *pMem )
{
	g_EntityListPool.Free( pMem );
}

void *CEventAction::operator new( size_t stAllocateBlock )
{
	return g_EntityListPool.Alloc( stAllocateBlock );
}

void *CEventAction::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	return g_EntityListPool.Alloc( stAllocateBlock );
}

void CEventAction::operator delete( void *pMem )
{
	g_EntityListPool.Free( pMem );
}

#pragma pop_macro("delete")
#pragma pop_macro("new")
