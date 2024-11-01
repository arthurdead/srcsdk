//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A global class that holds a prioritized queue of entity I/O events.
//			Events can be posted with a nonzero delay, which determines how long
//			they are held before being dispatched to their recipients.
//
//			The queue is serviced once per server frame.
//
//=============================================================================//

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H
#pragma once

#include "mempool.h"
#include "variant_t.h"
#include "ehandle.h"

struct EventQueuePrioritizedEvent_t
{
	float m_flFireTime;
	string_t m_iTarget;
	string_t m_iTargetInput;
	EHANDLE m_pActivator;
	EHANDLE m_pCaller;
	int m_iOutputID;
	EHANDLE m_pEntTarget;  // a pointer to the entity to target; overrides m_iTarget

	variant_t m_VariantValue;	// variable-type parameter

	EventQueuePrioritizedEvent_t *m_pNext;
	EventQueuePrioritizedEvent_t *m_pPrev;

	DECLARE_FIXEDSIZE_ALLOCATOR( PrioritizedEvent_t );
};

class CEventQueue
{
public:
	// pushes an event into the queue, targeting a string name (m_iName), or directly by a pointer
	EventQueuePrioritizedEvent_t *AddEvent( const char *target, const char *action, variant_t Value, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID = 0 );
	EventQueuePrioritizedEvent_t *AddEvent( CSharedBaseEntity *target, const char *action, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID = 0 );
	EventQueuePrioritizedEvent_t *AddEvent( CSharedBaseEntity *target, const char *action, variant_t Value, float fireDelay, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, int outputID = 0 );

	void CancelEvents( CSharedBaseEntity *pCaller );
	void CancelEventOn( CSharedBaseEntity *pTarget, const char *sInputName );
	bool HasEventPending( CSharedBaseEntity *pTarget, const char *sInputName );

	// services the queue, firing off any events who's time hath come
	void ServiceEvents( void );

	// debugging
	void ValidateQueue( void );

	CEventQueue();
	~CEventQueue();

	void Init( void );
	void Clear( void ); // resets the list

	void Dump( void );

	void CancelEventsByInput( CSharedBaseEntity *pTarget, const char *szInput );
	void DeleteEvent( EventQueuePrioritizedEvent_t *event );
	float GetTimeLeft( EventQueuePrioritizedEvent_t *event );

private:

	void AddEvent( EventQueuePrioritizedEvent_t *event );
	void RemoveEvent( EventQueuePrioritizedEvent_t *pe );

	EventQueuePrioritizedEvent_t m_Events;
	int m_iListCount;
};

extern CEventQueue g_EventQueue;


#endif // EVENTQUEUE_H

