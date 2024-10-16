//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BASEHANDLE_H
#define BASEHANDLE_H
#pragma once


#include "const.h"
#include "tier0/dbg.h"

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

namespace std
{
	using nullptr_t = decltype(nullptr);
}

class IHandleEntity;

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#elif defined CLIENT_DLL
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

// -------------------------------------------------------------------------------------------------- //
// CBaseHandle.
// -------------------------------------------------------------------------------------------------- //

class CBaseHandle
{
friend class CBaseEntityList;

public:

	CBaseHandle();
	CBaseHandle( const CBaseHandle &other );
	explicit CBaseHandle( unsigned long value );
	CBaseHandle( int iEntry, int iSerialNumber );
	explicit CBaseHandle( const IHandleEntity *pVal );

#if defined GAME_DLL || defined CLIENT_DLL
	explicit CBaseHandle( const CSharedBaseEntity *pVal );
#endif

	explicit CBaseHandle( std::nullptr_t );

	void Init( int iEntry, int iSerialNumber );
	void Term();

	// Even if this returns true, Get() still can return return a non-null value.
	// This just tells if the handle has been initted with any values.
	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	int ToInt() const;
	bool operator !=( const CBaseHandle &other ) const;
	bool operator ==( const CBaseHandle &other ) const;
	bool operator ==( const IHandleEntity* pEnt ) const;
	bool operator !=( const IHandleEntity* pEnt ) const;

#if defined GAME_DLL || defined CLIENT_DLL
	bool operator ==( const CSharedBaseEntity* pEnt ) const;
	bool operator !=( const CSharedBaseEntity* pEnt ) const;
#endif

	bool operator ==( IHandleEntity* pEnt ) const;
	bool operator !=( IHandleEntity* pEnt ) const;

#if defined GAME_DLL || defined CLIENT_DLL
	bool operator ==( CSharedBaseEntity* pEnt ) const;
	bool operator !=( CSharedBaseEntity* pEnt ) const;
#endif

	bool operator ==( std::nullptr_t ) const;
	bool operator !=( std::nullptr_t ) const;

	// Assign a value to the handle.
	CBaseHandle& operator=( const IHandleEntity *pEntity );

#if defined GAME_DLL || defined CLIENT_DLL
	CBaseHandle& operator=( const CSharedBaseEntity *pEntity );
#endif

	CBaseHandle& operator=( const CBaseHandle &pEntity );
	CBaseHandle& operator=( std::nullptr_t );

	void Set( const CBaseHandle &pEntity );
	void Set( const IHandleEntity *pEntity );

#if defined GAME_DLL || defined CLIENT_DLL
	void Set( const CSharedBaseEntity *pEntity );
#endif

	void Set( std::nullptr_t );

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	IHandleEntity* Get() const;

	bool operator!() const
	{ return Get() == NULL; }

	explicit operator bool() const
	{ return Get() != NULL; }

	explicit operator IHandleEntity *() const
	{ return Get(); }
	explicit operator const IHandleEntity *() const
	{ return Get(); }

protected:
	// The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
	unsigned long	m_Index;
};

inline const CBaseHandle NULL_BASEHANDLE;


#include "ihandleentity.h"


inline CBaseHandle::CBaseHandle()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle( const CBaseHandle &other )
{
	m_Index = other.m_Index;
}

inline CBaseHandle::CBaseHandle( unsigned long value )
{
	m_Index = value;
}

inline CBaseHandle::CBaseHandle( std::nullptr_t )
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle( const IHandleEntity *pObj )
{
	m_Index = pObj->GetRefEHandle().m_Index;
}

#if defined GAME_DLL || defined CLIENT_DLL
inline CBaseHandle::CBaseHandle( const CSharedBaseEntity *pObj )
	: CBaseHandle((const IHandleEntity *)pObj)
{
}
#endif

inline CBaseHandle::CBaseHandle( int iEntry, int iSerialNumber )
{
	Init( iEntry, iSerialNumber );
}

inline void CBaseHandle::Init( int iEntry, int iSerialNumber )
{
	Assert( iEntry >= 0 );
	Assert( iSerialNumber >= 0 );

	if( iEntry >= ENGINE_NUM_ENT_ENTRIES )
	{
		Assert( iEntry < GAME_NUM_ENT_ENTRIES );
		Assert( (iEntry & GAME_ENT_ENTRY_MASK) == iEntry);
		Assert( iSerialNumber < (1 << GAME_NUM_SERIAL_NUM_BITS) );

		m_Index = iEntry | (iSerialNumber << GAME_NUM_SERIAL_NUM_SHIFT_BITS);
		m_Index |= GAME_EHANDLE_TEST_BIT;
	}
	else
	{
		Assert( (iEntry & ENGINE_ENT_ENTRY_MASK) == iEntry);
		Assert( iSerialNumber < (1 << ENGINE_NUM_SERIAL_NUM_BITS) );

		m_Index = iEntry | (iSerialNumber << ENGINE_NUM_ENT_ENTRY_BITS);
		m_Index &= ~GAME_EHANDLE_TEST_BIT;
	}
}

inline void CBaseHandle::Term()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CBaseHandle::IsValid() const
{
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CBaseHandle::GetEntryIndex() const
{
	if(m_Index == INVALID_EHANDLE_INDEX)
		return ENGINE_NUM_ENT_ENTRIES-1;
	if( m_Index & GAME_EHANDLE_TEST_BIT )
		return (m_Index & ~GAME_EHANDLE_TEST_BIT) & GAME_ENT_ENTRY_MASK;
	return m_Index & ENGINE_ENT_ENTRY_MASK;
}

inline int CBaseHandle::GetSerialNumber() const
{
	if( m_Index & GAME_EHANDLE_TEST_BIT )
		return (m_Index & ~GAME_EHANDLE_TEST_BIT) >> GAME_NUM_SERIAL_NUM_SHIFT_BITS;
	return m_Index >> ENGINE_NUM_ENT_ENTRY_BITS;
}

inline int CBaseHandle::ToInt() const
{
	return (int)m_Index;
}

inline bool CBaseHandle::operator !=( const CBaseHandle &other ) const
{
	return m_Index != other.m_Index;
}

inline bool CBaseHandle::operator ==( const CBaseHandle &other ) const
{
	return m_Index == other.m_Index;
}

inline bool CBaseHandle::operator ==( const IHandleEntity* pEnt ) const
{
	return Get() == pEnt;
}

inline bool CBaseHandle::operator !=( const IHandleEntity* pEnt ) const
{
	return Get() != pEnt;
}

#if defined GAME_DLL || defined CLIENT_DLL
inline bool CBaseHandle::operator ==( const CSharedBaseEntity* pEnt ) const
{
	return (const CSharedBaseEntity*)Get() == pEnt;
}

inline bool CBaseHandle::operator !=( const CSharedBaseEntity* pEnt ) const
{
	return (const CSharedBaseEntity*)Get() != pEnt;
}
#endif

inline bool CBaseHandle::operator ==( IHandleEntity* pEnt ) const
{
	return Get() == pEnt;
}

inline bool CBaseHandle::operator !=( IHandleEntity* pEnt ) const
{
	return Get() != pEnt;
}

#if defined GAME_DLL || defined CLIENT_DLL
inline bool CBaseHandle::operator ==( CSharedBaseEntity* pEnt ) const
{
	return (const CSharedBaseEntity*)Get() == pEnt;
}

inline bool CBaseHandle::operator !=( CSharedBaseEntity* pEnt ) const
{
	return (const CSharedBaseEntity*)Get() != pEnt;
}
#endif

inline bool CBaseHandle::operator ==( std::nullptr_t ) const
{
	return Get() == NULL;
}

inline bool CBaseHandle::operator !=( std::nullptr_t ) const
{
	return Get() != NULL;
}

inline CBaseHandle& CBaseHandle::operator=( const IHandleEntity *pEntity )
{
	Set( pEntity );
	return *this;
}

#if defined GAME_DLL || defined CLIENT_DLL
inline CBaseHandle& CBaseHandle::operator=( const CSharedBaseEntity *pEntity )
{
	Set( pEntity );
	return *this;
}
#endif

inline CBaseHandle& CBaseHandle::operator=( const CBaseHandle &other )
{
	m_Index = other.m_Index;
	return *this;
}

inline CBaseHandle& CBaseHandle::operator=( std::nullptr_t )
{
	m_Index = INVALID_EHANDLE_INDEX;
	return *this;
}

#if defined GAME_DLL || defined CLIENT_DLL
inline void CBaseHandle::Set( const CSharedBaseEntity *pEntity ) 
{ 
	Set( (const IHandleEntity *)pEntity );
}
#endif

inline void CBaseHandle::Set( const IHandleEntity *pEntity ) 
{ 
	if ( pEntity )
	{
		m_Index = pEntity->GetRefEHandle().m_Index;
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}
}

inline void CBaseHandle::Set( const CBaseHandle &other ) 
{ 
	m_Index = other.m_Index;
}

inline void CBaseHandle::Set( std::nullptr_t ) 
{ 
	m_Index = INVALID_EHANDLE_INDEX;
}

#endif // BASEHANDLE_H
