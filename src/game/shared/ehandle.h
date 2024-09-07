//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EHANDLE_H
#define EHANDLE_H
#pragma once

#if defined( _DEBUG ) && defined( GAME_DLL )
#include "tier0/dbg.h"
#endif


#include "const.h"
#include "basehandle.h"

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

namespace std
{
	using nullptr_t = decltype(nullptr);
}

#ifdef GAME_DLL
class CBaseEntity;
#else
class C_BaseEntity;
#define CBaseEntity C_BaseEntity
#endif

class IHandleEntity;

// -------------------------------------------------------------------------------------------------- //
// CHandle.
// -------------------------------------------------------------------------------------------------- //
template< class T >
class CHandleImpl : public CBaseHandle
{
public:
	using CBaseHandle::operator==;
	using CBaseHandle::operator!=;

	CHandleImpl();
	CHandleImpl( int iEntry, int iSerialNumber );
	CHandleImpl( unsigned long value ) = delete;
	CHandleImpl( const CBaseHandle &other );
	CHandleImpl( const CHandleImpl &handle );
	CHandleImpl( const T *pVal );
	CHandleImpl( T *pVal );
	CHandleImpl( const IHandleEntity *pVal ) = delete;
	CHandleImpl( IHandleEntity *pVal ) = delete;
	CHandleImpl( std::nullptr_t );

	// The index should have come from a call to ToInt(). If it hasn't, you're in trouble.
	static CHandleImpl<T> FromIndex( int index );

	T*		Get() const;
	void Set( const CBaseHandle &pEntity ) = delete;
	void Set( const IHandleEntity *pEntity ) = delete;
	void	Set( const T* pVal );
	void	Set( const CHandleImpl &other );
	void	Set( std::nullptr_t );

	operator T*() const;
	explicit operator const T*() const;

	explicit operator IHandleEntity*() const;
	explicit operator const IHandleEntity*() const;

	bool	operator !() const;
	explicit operator bool() const;
	bool operator !=( const CBaseHandle &other ) const;
	bool operator ==( const CBaseHandle &other ) const;
	bool operator !=( const CHandleImpl &other ) const;
	bool operator ==( const CHandleImpl &other ) const;
	bool	operator==( const T *val ) const;
	bool	operator!=( const T *val ) const;
	bool	operator==( T *val ) const;
	bool	operator!=( T *val ) const;
	bool	operator==( const IHandleEntity *val ) const;
	bool	operator!=( const IHandleEntity *val ) const;
	bool	operator==( IHandleEntity *val ) const;
	bool	operator!=( IHandleEntity *val ) const;
	bool	operator==( std::nullptr_t ) const;
	bool	operator!=( std::nullptr_t ) const;

	CBaseHandle& operator=( const IHandleEntity *pEntity ) = delete;
	CHandleImpl& operator=( const CBaseHandle &pEntity );

	CHandleImpl& operator=( const T *val );
	CHandleImpl& operator=( const CHandleImpl &val );
	CHandleImpl& operator=( std::nullptr_t );

	T*		operator->() const;
};

template< class T >
class CHandle : public CHandleImpl<T>
{
public:
	using CHandleImpl<T>::CHandleImpl;
	using CHandleImpl<T>::operator=;
	using CHandleImpl<T>::operator==;
	using CHandleImpl<T>::operator!=;
	using CHandleImpl<T>::Set;

	CHandle( const CBaseEntity *pVal ) = delete;
	CHandle( CBaseEntity *pVal ) = delete;

	CBaseHandle& operator=( const CBaseEntity *pEntity ) = delete;

	bool operator !=( const CHandleImpl<CBaseEntity> &other ) const;
	bool operator ==( const CHandleImpl<CBaseEntity> &other ) const;

	explicit operator CBaseEntity*() const;
	explicit operator const CBaseEntity*() const;

	bool	operator==( const CBaseEntity *val ) const;
	bool	operator!=( const CBaseEntity *val ) const;
	bool	operator==( CBaseEntity *val ) const;
	bool	operator!=( CBaseEntity *val ) const;
};

template<>
class CHandle<CBaseEntity> : public CHandleImpl<CBaseEntity>
{
public:
	using CHandleImpl<CBaseEntity>::CHandleImpl;
	using CHandleImpl<CBaseEntity>::operator=;
	using CHandleImpl<CBaseEntity>::operator==;
	using CHandleImpl<CBaseEntity>::operator!=;
	using CHandleImpl<CBaseEntity>::Set;
};

#ifdef GAME_DLL
typedef CHandle<CBaseEntity> EHANDLE;
#else
typedef CHandle<C_BaseEntity> EHANDLE;
#endif

inline const EHANDLE NULL_EHANDLE;

template< class T >
inline const CHandle<T> NULL_HANDLE;

// ----------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------- //

template<class T>
CHandleImpl<T>::CHandleImpl()
{
}

template<class T>
CHandleImpl<T>::CHandleImpl( const CHandleImpl &handle )
	: CBaseHandle( handle )
{
}

template<class T>
CHandleImpl<T>::CHandleImpl( const CBaseHandle &handle )
	: CBaseHandle( handle )
{
}

template<class T>
CHandleImpl<T>::CHandleImpl( int iEntry, int iSerialNumber )
	: CBaseHandle( iEntry, iSerialNumber )
{
}

template<class T>
CHandleImpl<T>::CHandleImpl( const T *pObj )
{
	Set( pObj );
}

template<class T>
CHandleImpl<T>::CHandleImpl( T *pObj )
{
	Set( pObj );
}

template<class T>
CHandleImpl<T>::CHandleImpl( std::nullptr_t )
{
	Term();
}

template<class T>
inline CHandleImpl<T> CHandleImpl<T>::FromIndex( int index )
{
	CHandleImpl<T> ret;
	ret.m_Index = index;
	return ret;
}


template<class T>
inline T* CHandleImpl<T>::Get() const
{
	return (T*)CBaseHandle::Get();
}


template<class T>
inline CHandleImpl<T>::operator T *() const
{ 
	return Get( ); 
}

template<class T>
inline CHandleImpl<T>::operator const IHandleEntity *() const
{ 
	return Get( ); 
}

template<class T>
inline CHandleImpl<T>::operator IHandleEntity *() const
{ 
	return Get( ); 
}

template<class T>
inline CHandle<T>::operator const CBaseEntity *() const
{ 
	return this->Get( ); 
}

template<class T>
inline CHandle<T>::operator CBaseEntity *() const
{ 
	return this->Get( ); 
}

template<class T>
inline CHandleImpl<T>::operator const T *() const
{ 
	return Get( ); 
}

template<class T>
inline bool CHandleImpl<T>::operator !() const
{
	return Get() == NULL;
}

template<class T>
inline CHandleImpl<T>::operator bool() const
{
	return Get() != NULL;
}

template<class T>
inline bool CHandleImpl<T>::operator==( const CBaseHandle &other ) const
{
	return CBaseHandle::operator==(other);
}

template<class T>
inline bool CHandleImpl<T>::operator!=( const CBaseHandle &other ) const
{
	return CBaseHandle::operator!=(other);
}

template<class T>
inline bool CHandleImpl<T>::operator==( const CHandleImpl &other ) const
{
	return CBaseHandle::operator==( static_cast<const CBaseHandle &>(other) );
}

template<class T>
inline bool CHandleImpl<T>::operator!=( const CHandleImpl &other ) const
{
	return CBaseHandle::operator!=( static_cast<const CBaseHandle &>(other) );
}

template<class T>
inline bool CHandle<T>::operator==( const CHandleImpl<CBaseEntity> &other ) const
{
	return CBaseHandle::operator==( static_cast<const CBaseHandle &>(other) );
}

template<class T>
inline bool CHandle<T>::operator!=( const CHandleImpl<CBaseEntity> &other ) const
{
	return CBaseHandle::operator!=( static_cast<const CBaseHandle &>(other) );
}

template<class T>
inline bool CHandleImpl<T>::operator==( const T *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandleImpl<T>::operator!=( const T *val ) const
{
	return Get() != val;
}

template<class T>
inline bool CHandleImpl<T>::operator==( T *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandleImpl<T>::operator!=( T *val ) const
{
	return Get() != val;
}

template<class T>
inline bool CHandle<T>::operator==( const CBaseEntity *val ) const
{
	return this->Get() == val;
}

template<class T>
inline bool CHandle<T>::operator!=( const CBaseEntity *val ) const
{
	return this->Get() != val;
}

template<class T>
inline bool CHandle<T>::operator==( CBaseEntity *val ) const
{
	return this->Get() == val;
}

template<class T>
inline bool CHandle<T>::operator!=( CBaseEntity *val ) const
{
	return this->Get() != val;
}

template<class T>
inline bool CHandleImpl<T>::operator==( const IHandleEntity *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandleImpl<T>::operator!=( const IHandleEntity *val ) const
{
	return Get() != val;
}

template<class T>
inline bool CHandleImpl<T>::operator==( IHandleEntity *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandleImpl<T>::operator!=( IHandleEntity *val ) const
{
	return Get() != val;
}

template<class T>
inline bool CHandleImpl<T>::operator==( std::nullptr_t ) const
{
	return Get() == NULL;
}

template<class T>
inline bool CHandleImpl<T>::operator!=( std::nullptr_t ) const
{
	return Get() != NULL;
}

template<class T>
void CHandleImpl<T>::Set( const T* pVal )
{
	CBaseHandle::Set( reinterpret_cast<const IHandleEntity*>(pVal) );
}

template<class T>
void CHandleImpl<T>::Set( const CHandleImpl &other )
{
	CBaseHandle::Set( static_cast<const CBaseHandle &>(other) );
}

template<class T>
void CHandleImpl<T>::Set( std::nullptr_t )
{
	Term();
}

template<class T>
inline CHandleImpl<T>& CHandleImpl<T>::operator=( const T *val )
{
	Set( val );
	return *this;
}

template<class T>
inline CHandleImpl<T>& CHandleImpl<T>::operator=( const CHandleImpl &other )
{
	CBaseHandle::operator=( static_cast<const CBaseHandle &>(other) );
	return *this;
}

template<class T>
inline CHandleImpl<T>& CHandleImpl<T>::operator=( const CBaseHandle &other )
{
	CBaseHandle::operator=( other );
	return *this;
}

template<class T>
inline CHandleImpl<T>& CHandleImpl<T>::operator=( std::nullptr_t )
{
	Term();
	return *this;
}

template<class T>
T* CHandleImpl<T>::operator -> () const
{
	return Get();
}

#endif // EHANDLE_H
