
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef TIER1_ILOCALIZE_H
#define TIER1_ILOCALIZE_H

#pragma once

#include "appframework/IAppSystem.h"
#include <tier1/KeyValues.h>
#include "localize/ilocalize.h"

#ifdef GC

	typedef char locchar_t;

	#define loc_snprintf	Q_snprintf
	#define loc_sprintf_safe V_sprintf_safe
	#define loc_sncat		Q_strncat
	#define loc_scat_safe	V_strcat_safe
	#define loc_sncpy		Q_strncpy
	#define loc_scpy_safe	V_strcpy_safe
	#define loc_strlen		Q_strlen
	#define LOCCHAR( x )	x

#else

	typedef wchar_t locchar_t;

	#define loc_snprintf	V_snwprintf
	#define loc_sprintf_safe V_swprintf_safe
	#define loc_sncat		V_wcsncat
	#define loc_scat_safe	V_wcscat_safe
	#define loc_sncpy		Q_wcsncpy
	#define loc_scpy_safe	V_wcscpy_safe
	#define loc_strlen		Q_wcslen
	#define LOCCHAR(x)		L ## x

#endif

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------

template < typename T >
class TypedKeyValuesStringHelper
{
public:
	static const T *Read( KeyValues *pKeyValues, const char *pKeyName, const T *pDefaultValue );
	static void	Write( KeyValues *pKeyValues, const char *pKeyName, const T *pValue );
};

// --------------------------------------------------------------------------

template < >
class TypedKeyValuesStringHelper<char>
{
public:
	static const char *Read( KeyValues *pKeyValues, const char *pKeyName, const char *pDefaultValue ) { return pKeyValues->GetString( pKeyName, pDefaultValue ); }
	static void Write( KeyValues *pKeyValues, const char *pKeyName, const char *pValue ) { pKeyValues->SetString( pKeyName, pValue ); }
};

// --------------------------------------------------------------------------

template < >
class TypedKeyValuesStringHelper<wchar_t>
{
public:
	static const wchar_t *Read( KeyValues *pKeyValues, const char *pKeyName, const wchar_t *pDefaultValue ) { return pKeyValues->GetWString( pKeyName, pDefaultValue ); }
	static void Write( KeyValues *pKeyValues, const char *pKeyName, const wchar_t *pValue ) { pKeyValues->SetWString( pKeyName, pValue ); }
};

// --------------------------------------------------------------------------
// Purpose: CLocalizedStringArg<> is a class that will take a variable of any
//			arbitary type and convert it to a string of whatever character type
//			we're using for localization (locchar_t).
//
//			Independently it isn't very useful, though it can be used to sort-of-
//			intelligently fill out the correct format string. It's designed to be
//			used for the arguments of CConstructLocalizedString, which can be of
//			arbitrary number and type.
//
//			If you pass in a (non-specialized) pointer, the code will assume that
//			you meant that pointer to be used as a localized string. This will
//			still fail to compile if some non-string type is passed in, but will
//			handle weird combinations of const/volatile/whatever automatically.
// --------------------------------------------------------------------------

// The base implementation doesn't do anything except fail to compile if you
// use it. Getting an "incomplete type" error here means that you tried to construct
// a localized string with a type that doesn't have a specialization.
template < typename T >
class CLocalizedStringArg;

// --------------------------------------------------------------------------

template < typename T >
class CLocalizedStringArgStringImpl
{
public:
	enum { kIsValid = true };

	CLocalizedStringArgStringImpl( const locchar_t *pStr ) : m_pStr( pStr ) { }

	const locchar_t *GetLocArg() const { Assert( m_pStr ); return m_pStr; }

private:
	const locchar_t *m_pStr;
};

// --------------------------------------------------------------------------

template < typename T >
class CLocalizedStringArg<T *> : public CLocalizedStringArgStringImpl<T>
{
public:
	CLocalizedStringArg( const locchar_t *pStr ) : CLocalizedStringArgStringImpl<T>( pStr ) { }
};

// --------------------------------------------------------------------------

template < typename T >
class CLocalizedStringArgPrintfImpl
{
public:
	enum { kIsValid = true };

	CLocalizedStringArgPrintfImpl( T value, const locchar_t *loc_Format ) { loc_snprintf( m_cBuffer, kBufferSize, loc_Format, value ); }

	const locchar_t *GetLocArg() const { return m_cBuffer; }

private:
	enum { kBufferSize = 128, };
	locchar_t m_cBuffer[ kBufferSize ];
};

// --------------------------------------------------------------------------

template < >
class CLocalizedStringArg<uint16> : public CLocalizedStringArgPrintfImpl<uint16>
{
public:
	CLocalizedStringArg( uint16 unValue ) : CLocalizedStringArgPrintfImpl<uint16>( unValue, LOCCHAR("%u") ) { }
};

// --------------------------------------------------------------------------

template < >
class CLocalizedStringArg<uint32> : public CLocalizedStringArgPrintfImpl<uint32>
{
public:
	CLocalizedStringArg( uint32 unValue ) : CLocalizedStringArgPrintfImpl<uint32>( unValue, LOCCHAR("%u") ) { }
};

// --------------------------------------------------------------------------

template < >
class CLocalizedStringArg<uint64> : public CLocalizedStringArgPrintfImpl<uint64>
{
public:
	CLocalizedStringArg( uint64 unValue ) : CLocalizedStringArgPrintfImpl<uint64>( unValue, LOCCHAR("%llu") ) { }
};

// --------------------------------------------------------------------------

template < >
class CLocalizedStringArg<float> : public CLocalizedStringArgPrintfImpl<float>
{
public:
	// Display one decimal point if we've got a value less than one, and no point
	// if we're greater than one or are effectively zero.
	CLocalizedStringArg( float fValue )
		: CLocalizedStringArgPrintfImpl<float>( fValue,
												fabsf( fValue ) <= FLT_EPSILON || fabsf( fValue ) >= 1.0f ? LOCCHAR("%.0f") : LOCCHAR("%.1f") )
	{
		//
	}
};

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
class CConstructLocalizedString
{
public:
	template < typename T >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0 )
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer, sizeof( m_loc_Buffer ), loc_Format, 1, CLocalizedStringArg<T>( arg0 ).GetLocArg() );
		}
	}

	template < typename T, typename U >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0, U arg1 )
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<U>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer, sizeof( m_loc_Buffer ), loc_Format, 2, CLocalizedStringArg<T>( arg0 ).GetLocArg(), CLocalizedStringArg<U>( arg1 ).GetLocArg() );
		}
	}

	template < typename T, typename U, typename V >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0, U arg1, V arg2 )
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<U>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<V>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer,
										  sizeof( m_loc_Buffer ),
										  loc_Format,
										  3,
										  CLocalizedStringArg<T>( arg0 ).GetLocArg(),
										  CLocalizedStringArg<U>( arg1 ).GetLocArg(),
										  CLocalizedStringArg<V>( arg2 ).GetLocArg() );
		}
	}

	template < typename T, typename U, typename V, typename W >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0, U arg1, V arg2, W arg3 )
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<U>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<V>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<W>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer,
										  sizeof( m_loc_Buffer ),
										  loc_Format,
										  4,
										  CLocalizedStringArg<T>( arg0 ).GetLocArg(),
										  CLocalizedStringArg<U>( arg1 ).GetLocArg(),
										  CLocalizedStringArg<V>( arg2 ).GetLocArg(),
										  CLocalizedStringArg<W>( arg3 ).GetLocArg() );
		}
	}

	template < typename T, typename U, typename V, typename W, typename X, typename Y >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0, U arg1, V arg2, W arg3, X arg4, Y arg5 )
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<U>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<V>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<W>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<X>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<Y>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer,
										  sizeof( m_loc_Buffer ),
										  loc_Format,
										  6,
										  CLocalizedStringArg<T>( arg0 ).GetLocArg(),
										  CLocalizedStringArg<U>( arg1 ).GetLocArg(),
										  CLocalizedStringArg<V>( arg2 ).GetLocArg(),
										  CLocalizedStringArg<W>( arg3 ).GetLocArg(),
										  CLocalizedStringArg<X>( arg4 ).GetLocArg(),
										  CLocalizedStringArg<Y>( arg5 ).GetLocArg() );
		}
	}

	template < typename T, typename U, typename V, typename W, typename X, typename Y, typename Z >
	CConstructLocalizedString( const locchar_t *loc_Format, T arg0, U arg1, V arg2, W arg3, X arg4, Y arg5, Z arg6)
	{
		COMPILE_TIME_ASSERT( CLocalizedStringArg<T>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<U>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<V>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<W>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<X>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<Y>::kIsValid );
		COMPILE_TIME_ASSERT( CLocalizedStringArg<Z>::kIsValid );

		m_loc_Buffer[0] = '\0';

		if ( loc_Format )
		{
			::ILocalize::ConstructString( m_loc_Buffer,
				sizeof( m_loc_Buffer ),
				loc_Format,
				7,
				CLocalizedStringArg<T>( arg0 ).GetLocArg(),
				CLocalizedStringArg<U>( arg1 ).GetLocArg(),
				CLocalizedStringArg<V>( arg2 ).GetLocArg(),
				CLocalizedStringArg<W>( arg3 ).GetLocArg(),
				CLocalizedStringArg<X>( arg4 ).GetLocArg(),
				CLocalizedStringArg<Y>( arg5 ).GetLocArg(), 
				CLocalizedStringArg<Z>( arg6 ).GetLocArg() );
		}
	}

	CConstructLocalizedString( const locchar_t *loc_Format, KeyValues *pKeyValues )
	{
		m_loc_Buffer[0] = '\0';

		if ( loc_Format && pKeyValues )
		{
			::ILocalize::ConstructString( m_loc_Buffer, sizeof( m_loc_Buffer ), loc_Format, pKeyValues );
		}
	}

	operator const locchar_t *() const
	{
		return m_loc_Buffer;
	}

private:
	enum { kBufferSize = 512, };
	locchar_t m_loc_Buffer[ kBufferSize ];
};

#endif // TIER1_ILOCALIZE_H
