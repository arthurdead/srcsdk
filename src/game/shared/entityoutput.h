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

	fieldtype_t ValueBaseFieldType() { return m_Value.baseFieldType(); }
	fieldtype_t ValueRawFieldType() { return m_Value.rawFieldType(); }

	/// Delete every single action in the action list. 
	void DeleteAllElements( void ) ;

	// Needed for ReplaceOutput, hopefully not bad
	CEventAction *GetActionList() { return m_ActionList; }
	void SetActionList(CEventAction *newlist) { m_ActionList = newlist; }

	CEventAction *GetFirstAction() { return m_ActionList; }

	const CEventAction *GetActionForTarget( string_t iSearchTarget ) const;

protected:
	void FireOutput( const variant_t &Value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay = 0 );

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
class CEntityOutputTemplate;


//
// Template specializations for type Vector, so we can implement Get, Set, and Init differently.
//
template<>
class CEntityOutputTemplate<Vector, FIELD_VECTOR> : public CBaseEntityOutput
{
public:
	void Init( const Vector &value )
	{
		m_Value.SetVector3D( value );
	}

	void Set( const Vector &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetVector3D( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	Vector Get() const
	{
		return m_Value.Vector3D();
	}
};

template<>
class CEntityOutputTemplate<QAngle, FIELD_QANGLE> : public CBaseEntityOutput
{
public:
	void Init( const QAngle &value )
	{
		m_Value.SetAngle3D( value );
	}

	void Set( const QAngle &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetAngle3D( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	QAngle Get() const
	{
		return m_Value.Angle3D();
	}
};


template<>
class CEntityOutputTemplate<Vector, FIELD_VECTOR_WORLDSPACE> : public CBaseEntityOutput
{
public:
	void Init( const Vector &value )
	{
		m_Value.SetPositionVector3D( value );
	}

	void Set( const Vector &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetPositionVector3D( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	Vector Get() const
	{
		return m_Value.Vector3D();
	}
};

template<>
class CEntityOutputTemplate<float, FIELD_FLOAT> : public CBaseEntityOutput
{
public:
	void Init( float value )
	{
		m_Value.SetFloat( value );
	}

	void Set( float value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetFloat( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	float Get() const
	{
		return m_Value.Float();
	}
};

template<>
class CEntityOutputTemplate<int, FIELD_INTEGER> : public CBaseEntityOutput
{
public:
	void Init( int value )
	{
		m_Value.SetInt( value );
	}

	void Set( int value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetInt( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	int Get() const
	{
		return m_Value.Int();
	}
};

template<>
class CEntityOutputTemplate<variant_t, FIELD_VARIANT> : public CBaseEntityOutput
{
public:
	void Init( const variant_t &value )
	{
		m_Value = value;
	}

	void Init( variant_t &&value )
	{
		m_Value = Move(value);
	}

	void Set( const variant_t &value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value = value;
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	void Set( variant_t &&value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value = Move(value);
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	const variant_t &Get() const
	{
		return m_Value;
	}
};

template<>
class CEntityOutputTemplate<EHANDLE, FIELD_EHANDLE> : public CBaseEntityOutput
{
public:
	void Init( EHANDLE value )
	{
		m_Value.SetEntityH( value );
	}

	void Init( CSharedBaseEntity *value )
	{
		m_Value.SetEntityH( value );
	}

	void Set( EHANDLE value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetEntityH( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	void Set( CSharedBaseEntity *value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
	{
		m_Value.SetEntityH( value );
		CBaseEntityOutput::FireOutput( m_Value, pActivator, pCaller );
	}

	CSharedBaseEntity *Get() const
	{
		return m_Value.EntityP();
	}
};

//-----------------------------------------------------------------------------
// Purpose: parameterless entity event
//-----------------------------------------------------------------------------
class COutputEvent : public CBaseEntityOutput
{
public:
	void FireOutput( const variant_t &Value, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay = 0 ) = delete;

	// void Firing, no parameter
	void FireOutput( CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, float fDelay = 0 );
};


// useful typedefs for allowed output data types
typedef CEntityOutputTemplate<variant_t,FIELD_VARIANT>		COutputVariant;
typedef CEntityOutputTemplate<int,FIELD_INTEGER>			COutputInt;
typedef CEntityOutputTemplate<modelindex_t,FIELD_MODELINDEX>			COutputModelIndex;
typedef CEntityOutputTemplate<char,FIELD_CHARACTER>			COutputChar;
typedef CEntityOutputTemplate<unsigned int,FIELD_UINTEGER>			COutputUInt;
typedef CEntityOutputTemplate<signed char,FIELD_SCHARACTER>			COutputSChar;
typedef CEntityOutputTemplate<unsigned char,FIELD_UCHARACTER>			COutputUChar;
typedef CEntityOutputTemplate<int64,FIELD_INTEGER64>			COutputInt64;
typedef CEntityOutputTemplate<uint64,FIELD_INTEGER64>			COutputUInt64;
typedef CEntityOutputTemplate<short,FIELD_SHORT>			COutputShort;
typedef CEntityOutputTemplate<unsigned short,FIELD_USHORT>			COutputUShort;
typedef CEntityOutputTemplate<float,FIELD_FLOAT>			COutputFloat;
typedef CEntityOutputTemplate<string_t,FIELD_POOLED_STRING>		COutputStringT;
typedef CEntityOutputTemplate<EHANDLE,FIELD_EHANDLE>		COutputEHANDLE;
typedef CEntityOutputTemplate<Vector,FIELD_VECTOR>			COutputVector;
typedef CEntityOutputTemplate<QAngle,FIELD_QANGLE>			COutputQAngle;
typedef CEntityOutputTemplate<Vector,FIELD_VECTOR_WORLDSPACE>	COutputPositionVector;
typedef CEntityOutputTemplate<color32,FIELD_COLOR32>		COutputColor32;
typedef CEntityOutputTemplate<ColorRGBExp32,FIELD_COLOR32E>		COutputColor32E;
typedef CEntityOutputTemplate<color24,FIELD_COLOR24>		COutputColor24;

#endif // ENTITYOUTPUT_H
