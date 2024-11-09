//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Declares basic entity communications classes, for input/output of data
//			between entities
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENTITYOUTPUT_H
#define ENTITYOUTPUT_H

#pragma once

#include "string_t.h"
#include "datamap.h"
#include "variant_t.h"

DECLARE_LOGGING_CHANNEL( LOG_ENTITYIO );

#define EVENT_FIRE_ALWAYS	-1

//-----------------------------------------------------------------------------
// Purpose: A COutputEvent consists of an array of these CEventActions. 
//			Each CEventAction holds the information to fire a single input in 
//			a target entity, after a specific delay.
//-----------------------------------------------------------------------------
class CEventAction
{
public:
	CEventAction( const char *ActionData = NULL );
	CEventAction( const CEventAction &p_EventAction );

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext; 

	// allocates memory from engine.MPool/g_EntityListPool
	static void *operator new( size_t stAllocateBlock );
	static void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	static void operator delete( void *pMem );
	static void operator delete( void *pMem , int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }
};


//-----------------------------------------------------------------------------
// Purpose: Stores a list of connections to other entities, for data/commands to be
//			communicated along.
//-----------------------------------------------------------------------------
class CBaseEntityOutput
{
public:
	~CBaseEntityOutput();

	void ParseEventAction( const char *EventData );
	void AddEventAction( CEventAction *pEventAction );
	void RemoveEventAction( CEventAction *pEventAction );

	int NumberOfElements( void );

	float GetMaxDelay( void );

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	void FireOutput( variant_t Value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay = 0 );

	/// Delete every single action in the action list. 
	void DeleteAllElements( void ) ;

	// Needed for ReplaceOutput, hopefully not bad
	CEventAction *GetActionList() { return m_ActionList; }
	void SetActionList(CEventAction *newlist) { m_ActionList = newlist; }

	CEventAction *GetFirstAction() { return m_ActionList; }

	const CEventAction *GetActionForTarget( string_t iSearchTarget ) const;
protected:
	variant_t m_Value;
	CEventAction *m_ActionList;

	CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	CBaseEntityOutput( CBaseEntityOutput& ); // protect from accidental copying
};


//-----------------------------------------------------------------------------
// Purpose: wraps variant_t data handling in convenient, compiler type-checked template
//-----------------------------------------------------------------------------
template< class Type, fieldtype_t fieldType >
class CEntityOutputTemplate : public CBaseEntityOutput
{
public:
	//
	// Sets an initial value without firing the output.
	//
	void Init( Type value ) 
	{
		m_Value.Set( fieldType, &value );
	}

	//
	// Sets a value and fires the output.
	//
	void Set( Type value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller ) 
	{
		m_Value.Set( fieldType, &value );
		FireOutput( m_Value, pActivator, pCaller );
	}

	//
	// Returns the current value.
	//
	Type Get( void )
	{
		return *((Type*)&m_Value);
	}
};


//
// Template specializations for type Vector, so we can implement Get, Set, and Init differently.
//
template<>
class CEntityOutputTemplate<class Vector, FIELD_VECTOR> : public CBaseEntityOutput
{
public:
	void Init( const Vector &value )
	{
		m_Value.SetVector3D( value );
	}

	void Set( const Vector &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetVector3D( value );
		FireOutput( m_Value, pActivator, pCaller );
	}

	void Get( Vector &vec )
	{
		m_Value.Vector3D(vec);
	}
};

template<>
class CEntityOutputTemplate<class QAngle, FIELD_VECTOR> : public CBaseEntityOutput
{
public:
	void Init( const QAngle &value )
	{
		m_Value.SetAngle3D( value );
	}

	void Set( const QAngle &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetAngle3D( value );
		FireOutput( m_Value, pActivator, pCaller );
	}

	void Get( QAngle &vec )
	{
		m_Value.Angle3D(vec);
	}
};


template<>
class CEntityOutputTemplate<class Vector, FIELD_POSITION_VECTOR> : public CBaseEntityOutput
{
public:
	void Init( const Vector &value )
	{
		m_Value.SetPositionVector3D( value );
	}

	void Set( const Vector &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetPositionVector3D( value );
		FireOutput( m_Value, pActivator, pCaller );
	}

	void Get( Vector &vec )
	{
		m_Value.Vector3D(vec);
	}
};


//-----------------------------------------------------------------------------
// Purpose: parameterless entity event
//-----------------------------------------------------------------------------
class COutputEvent : public CBaseEntityOutput
{
public:
	// void Firing, no parameter
	void FireOutput( CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay = 0 );
};


// useful typedefs for allowed output data types
typedef CEntityOutputTemplate<variant_t,FIELD_INPUT>		COutputVariant;
typedef CEntityOutputTemplate<int,FIELD_INTEGER>			COutputInt;
typedef CEntityOutputTemplate<int64,FIELD_INTEGER64>			COutputInt64;
typedef CEntityOutputTemplate<float,FIELD_FLOAT>			COutputFloat;
typedef CEntityOutputTemplate<string_t,FIELD_STRING>		COutputString;
typedef CEntityOutputTemplate<EHANDLE,FIELD_EHANDLE>		COutputEHANDLE;
typedef CEntityOutputTemplate<Vector,FIELD_VECTOR>			COutputVector;
typedef CEntityOutputTemplate<QAngle,FIELD_VECTOR>			COutputQAngle;
typedef CEntityOutputTemplate<Vector,FIELD_POSITION_VECTOR>	COutputPositionVector;
typedef CEntityOutputTemplate<color32,FIELD_COLOR32>		COutputColor32;

#endif // ENTITYOUTPUT_H
