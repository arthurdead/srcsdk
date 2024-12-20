//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A message forwarder. Fires an OnTrigger output when triggered, and can be
// disabled to prevent forwarding outputs.
//
// Useful as an intermediary between one entity and another for turning on or
// off an I/O connection, or as a container for holding a set of outputs that
// can be triggered from multiple places.
//
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "soundent.h"
#include "logicrelay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int SF_REMOVE_ON_FIRE				= 0x001;	// Relay will remove itself after being triggered.
const int SF_ALLOW_FAST_RETRIGGER		= 0x002;	// Unless set, relay will disable itself until the last output is sent.

LINK_ENTITY_TO_CLASS(logic_relay, CLogicRelay);


BEGIN_MAPENTITY( CLogicRelay )

	DEFINE_KEYFIELD_AUTO( m_bDisabled, "StartDisabled" ),
#if RELAY_QUEUE_SYSTEM
	DEFINE_KEYFIELD_AUTO( m_bQueueTrigger, "QueueDisabledTrigger" ),
#endif

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "EnableRefire", InputEnableRefire),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Trigger", InputTrigger),
	DEFINE_INPUTFUNC(FIELD_INPUT, "TriggerWithParameter", InputTriggerWithParameter),
	DEFINE_INPUTFUNC(FIELD_VOID, "CancelPending", InputCancelPending),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
	DEFINE_OUTPUT(m_OnTriggerParameter, "OnTriggerParameter"),
	DEFINE_OUTPUT(m_OnSpawn, "OnSpawn"),

END_MAPENTITY()



//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLogicRelay::CLogicRelay(void)
{
}


//------------------------------------------------------------------------------
// Kickstarts a think if we have OnSpawn connections.
//------------------------------------------------------------------------------
void CLogicRelay::Activate()
{
	BaseClass::Activate();
	
	if ( m_OnSpawn.NumberOfElements() > 0)
	{
		SetNextThink( gpGlobals->curtime + 0.01 );
	}
}


//-----------------------------------------------------------------------------
// If we have OnSpawn connections, this is called shortly after spawning to
// fire the OnSpawn output.
//-----------------------------------------------------------------------------
void CLogicRelay::Think()
{
	// Fire an output when we spawn. This is used for self-starting an entity
	// template -- since the logic_relay is inside the template, it gets all the
	// name and I/O connection fixup, so can target other entities in the template.
	m_OnSpawn.FireOutput( this, this );

	// We only get here if we had OnSpawn connections, so this is safe.
	if ( HasSpawnFlags( SF_REMOVE_ON_FIRE ) )
	{
		UTIL_Remove(this);
	}
}


//------------------------------------------------------------------------------
// Purpose: Turns on the relay, allowing it to fire outputs.
//------------------------------------------------------------------------------
void CLogicRelay::InputEnable( inputdata_t &&inputdata )
{
	m_bDisabled = false;
#if RELAY_QUEUE_SYSTEM
	if (m_bQueueWaiting)
		m_OnTrigger.FireOutput( inputdata.pActivator, this );
#endif
}

//------------------------------------------------------------------------------
// Purpose: Enables us to fire again. This input is only posted from our Trigger
//			function to prevent rapid refire.
//------------------------------------------------------------------------------
void CLogicRelay::InputEnableRefire( inputdata_t &&inputdata )
{ 
	m_bWaitForRefire = false;
}


//------------------------------------------------------------------------------
// Purpose: Cancels any I/O events in the queue that were fired by us.
//------------------------------------------------------------------------------
void CLogicRelay::InputCancelPending( inputdata_t &&inputdata )
{ 
	g_EventQueue.CancelEvents( this );

	// Stop waiting; allow another Trigger.
	m_bWaitForRefire = false;
}


//------------------------------------------------------------------------------
// Purpose: Turns off the relay, preventing it from firing outputs.
//------------------------------------------------------------------------------
void CLogicRelay::InputDisable( inputdata_t &&inputdata )
{ 
	m_bDisabled = true;
}


//------------------------------------------------------------------------------
// Purpose: Toggles the enabled/disabled state of the relay.
//------------------------------------------------------------------------------
void CLogicRelay::InputToggle( inputdata_t &&inputdata )
{ 
	m_bDisabled = !m_bDisabled;
#if RELAY_QUEUE_SYSTEM
	if (m_bQueueWaiting)
		m_OnTrigger.FireOutput( inputdata.pActivator, this );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRelay::InputTrigger( inputdata_t &&inputdata )
{
	if ((!m_bDisabled) && (!m_bWaitForRefire))
	{
		m_OnTrigger.FireOutput( inputdata.pActivator, this );
		
		if (HasSpawnFlags( SF_REMOVE_ON_FIRE) )
		{
			UTIL_Remove(this);
		}
		else if (!HasSpawnFlags( SF_ALLOW_FAST_RETRIGGER))
		{
			//
			// Disable the relay so that it cannot be refired until after the last output
			// has been fired and post an input to re-enable ourselves.
			//
			m_bWaitForRefire = true;
			g_EventQueue.AddEvent(this, "EnableRefire", m_OnTrigger.GetMaxDelay() + 0.001, this, this);
#if RELAY_QUEUE_SYSTEM
			if (m_bQueueTrigger)
				m_flRefireTime = gpGlobals->curtime + m_OnTrigger.GetMaxDelay() + 0.002;
#endif
		}
	}
#if RELAY_QUEUE_SYSTEM
	else if (m_bQueueTrigger)
	{
		if (m_bDisabled)
			m_bQueueWaiting = true;
		else // m_bWaitForRefire
			m_OnTrigger.FireOutput( inputdata.pActivator, this, (gpGlobals->curtime - m_flRefireTime) );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRelay::InputTriggerWithParameter( inputdata_t &&inputdata )
{
	if ((!m_bDisabled) && (!m_bWaitForRefire))
	{
		m_OnTriggerParameter.Set( inputdata.value, inputdata.pActivator, this );
		
		if (HasSpawnFlags( SF_REMOVE_ON_FIRE))
		{
			UTIL_Remove(this);
		}
		else if (!HasSpawnFlags( SF_ALLOW_FAST_RETRIGGER))
		{
			//
			// Disable the relay so that it cannot be refired until after the last output
			// has been fired and post an input to re-enable ourselves.
			//
			m_bWaitForRefire = true;
			g_EventQueue.AddEvent(this, "EnableRefire", m_OnTrigger.GetMaxDelay() + 0.001, this, this);
		}
	}
}

LINK_ENTITY_TO_CLASS(logic_relay_queue, CLogicRelayQueue);


BEGIN_MAPENTITY( CLogicRelayQueue )

	DEFINE_KEYFIELD_AUTO( m_bDisabled, "StartDisabled" ),

	DEFINE_INPUT(m_iMaxQueueItems, FIELD_INTEGER, "SetMaxQueueItems"),
	DEFINE_KEYFIELD_AUTO( m_bDontQueueWhenDisabled, "DontQueueWhenDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "EnableRefire", InputEnableRefire),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Trigger", InputTrigger),
	DEFINE_INPUTFUNC(FIELD_INPUT, "TriggerWithParameter", InputTriggerWithParameter),
	DEFINE_INPUTFUNC(FIELD_VOID, "CancelPending", InputCancelPending),
	DEFINE_INPUTFUNC(FIELD_VOID, "ClearQueue", InputClearQueue),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
	DEFINE_OUTPUT(m_OnTriggerParameter, "OnTriggerParameter"),

END_MAPENTITY()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLogicRelayQueue::CLogicRelayQueue(void)
{
}


//------------------------------------------------------------------------------
// Purpose: Turns on the relay, allowing it to fire outputs.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputEnable( inputdata_t &&inputdata )
{
	m_bDisabled = false;

	if (!m_bWaitForRefire && m_QueueItems.Count() > 0)
		HandleNextQueueItem();
}

//------------------------------------------------------------------------------
// Purpose: Enables us to fire again. This input is only posted from our Trigger
//			function to prevent rapid refire.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputEnableRefire( inputdata_t &&inputdata )
{ 
	m_bWaitForRefire = false;

	if (!m_bDisabled && m_QueueItems.Count() > 0)
		HandleNextQueueItem();
}


//------------------------------------------------------------------------------
// Purpose: Cancels any I/O events in the queue that were fired by us.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputCancelPending( inputdata_t &&inputdata )
{ 
	g_EventQueue.CancelEvents( this );

	// Stop waiting; allow another Trigger.
	m_bWaitForRefire = false;

	if (!m_bDisabled && m_QueueItems.Count() > 0)
		HandleNextQueueItem();
}


//------------------------------------------------------------------------------
// Purpose: Clears the queue.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputClearQueue( inputdata_t &&inputdata )
{
	m_QueueItems.RemoveAll();
}


//------------------------------------------------------------------------------
// Purpose: Turns off the relay, preventing it from firing outputs.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputDisable( inputdata_t &&inputdata )
{ 
	m_bDisabled = true;
}


//------------------------------------------------------------------------------
// Purpose: Toggles the enabled/disabled state of the relay.
//------------------------------------------------------------------------------
void CLogicRelayQueue::InputToggle( inputdata_t &&inputdata )
{ 
	m_bDisabled = !m_bDisabled;

	if (!m_bDisabled && !m_bWaitForRefire && m_QueueItems.Count() > 0)
		HandleNextQueueItem();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRelayQueue::InputTrigger( inputdata_t &&inputdata )
{
	if ((!m_bDisabled) && (!m_bWaitForRefire))
	{
		m_OnTrigger.FireOutput( inputdata.pActivator, this );

		//
		// Disable the relay so that it cannot be refired until after the last output
		// has been fired and post an input to re-enable ourselves.
		//
		m_bWaitForRefire = true;
		g_EventQueue.AddEvent(this, "EnableRefire", m_OnTrigger.GetMaxDelay() + 0.001, this, this);
	}
	else if ( (!m_bDisabled || !m_bDontQueueWhenDisabled) && (m_QueueItems.Count() < m_iMaxQueueItems) )
	{
		AddQueueItem(inputdata.pActivator, inputdata.nOutputID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRelayQueue::InputTriggerWithParameter( inputdata_t &&inputdata )
{
	if ((!m_bDisabled) && (!m_bWaitForRefire))
	{
		m_OnTriggerParameter.Set( inputdata.value, inputdata.pActivator, this );
		
		//
		// Disable the relay so that it cannot be refired until after the last output
		// has been fired and post an input to re-enable ourselves.
		//
		m_bWaitForRefire = true;
		g_EventQueue.AddEvent(this, "EnableRefire", m_OnTrigger.GetMaxDelay() + 0.001, this, this);
	}
	else if ( (!m_bDisabled || !m_bDontQueueWhenDisabled) && (m_QueueItems.Count() < m_iMaxQueueItems) )
	{
		AddQueueItem(inputdata.pActivator, inputdata.nOutputID, Move(inputdata.value));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles next queue item.
//-----------------------------------------------------------------------------
void CLogicRelayQueue::HandleNextQueueItem()
{
	LogicRelayQueueInfo_t &info = m_QueueItems.Element(0);

	//if (!info.TriggerWithParameter)
	//{
	//	m_OnTrigger.FireOutput(info.pActivator, this);
	//}
	//else
	//{
	//	m_OnTriggerParameter.Set(info.value, info.pActivator, this);
	//}

	AcceptInput(info.TriggerWithParameter ? "TriggerWithParameter" : "Trigger", info.pActivator, this, Move(info.value), info.outputID);

	m_QueueItems.Remove(0);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a queue item.
//-----------------------------------------------------------------------------
void CLogicRelayQueue::AddQueueItem(CBaseEntity *pActivator, int outputID, variant_t &&value)
{
	LogicRelayQueueInfo_t info;
	info.pActivator = pActivator;
	info.outputID = outputID;

	info.value = Move(value);
	info.TriggerWithParameter = true;

	m_QueueItems.AddToTail(info);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a queue item without a parameter.
//-----------------------------------------------------------------------------
void CLogicRelayQueue::AddQueueItem(CBaseEntity *pActivator, int outputID)
{
	LogicRelayQueueInfo_t info;
	info.pActivator = pActivator;
	info.outputID = outputID;

	info.TriggerWithParameter = false;

	m_QueueItems.AddToTail(info);
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CLogicRelayQueue::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];

		if (m_QueueItems.Count() > 0)
		{
			Q_snprintf(tempstr, sizeof(tempstr), "Queue Items: %i (%i)", m_QueueItems.Count(), m_iMaxQueueItems);
			EntityText(text_offset, tempstr, 0);
			text_offset++;

			for (int i = 0; i < m_QueueItems.Count(); i++)
			{
				Q_snprintf(tempstr, sizeof(tempstr), "	Input: %s, Activator: %s, Output ID: %i",
					m_QueueItems[i].TriggerWithParameter ? "TriggerWithParameter" : "Trigger",
					m_QueueItems[i].pActivator ? m_QueueItems[i].pActivator->GetDebugName() : "None",
					m_QueueItems[i].outputID);
				EntityText(text_offset, tempstr, 0);
				text_offset++;
			}
		}
		else
		{
			Q_snprintf(tempstr, sizeof(tempstr), "Queue Items: 0 (%i)", m_iMaxQueueItems);
			EntityText(text_offset, tempstr, 0);
			text_offset++;
		}
	}
	return text_offset;
}