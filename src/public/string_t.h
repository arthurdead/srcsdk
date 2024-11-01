//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the more complete set of operations on the string_t defined
// 			These should be used instead of direct manipulation to allow more
//			flexibility in future ports or optimization.
//
// $NoKeywords: $
//=============================================================================//

#ifndef STRING_T_H
#define STRING_T_H

#pragma once

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

namespace std
{
	using nullptr_t = decltype(nullptr);
}

//-----------------------------------------------------------------------------

struct castable_string_t;

struct string_t
{
public:
	bool operator!() const							{ return ( pszValue == NULL );			}
	operator bool() const							{ return ( pszValue != NULL );			}
	bool operator==( const string_t &rhs ) const	{ return ( pszValue == rhs.pszValue );	}
	bool operator!=( const string_t &rhs ) const	{ return ( pszValue != rhs.pszValue );	}
	bool operator==( const char *rhs ) const	{ return ( pszValue == rhs );	}
	bool operator!=( const char *rhs ) const	{ return ( pszValue != rhs );	}

	bool operator<( const string_t &rhs ) const		{ return ((void *)pszValue < (void *)rhs.pszValue ); }
	bool operator<=( const string_t &rhs ) const		{ return ((void *)pszValue <= (void *)rhs.pszValue ); }
	bool operator>( const string_t &rhs ) const		{ return ((void *)pszValue > (void *)rhs.pszValue ); }
	bool operator>=( const string_t &rhs ) const		{ return ((void *)pszValue >= (void *)rhs.pszValue ); }

	bool operator==( std::nullptr_t ) const	{ return ( pszValue == NULL );	}
	bool operator!=( std::nullptr_t ) const	{ return ( pszValue != NULL );	}

	string_t &operator=( std::nullptr_t ) { pszValue = NULL; return *this; }
	string_t &operator=( const string_t &rhs ) { pszValue = rhs.pszValue; return *this; }
	string_t &operator=( const char *pszFrom ) = delete;

	string_t()							{ pszValue = NULL; }
	string_t( std::nullptr_t )	{ pszValue = NULL; }
	string_t( const char *pszFrom ) = delete;
	string_t( const string_t &rhs )	{ pszValue = rhs.pszValue; }

	const char *ToCStr() const						{ return ( pszValue ) ? pszValue : ""; 	}

protected:
	const char *pszValue;
};

inline bool operator==(std::nullptr_t, const string_t &rhs)
{ return !rhs.operator bool(); }
inline bool operator!=(std::nullptr_t, const string_t &rhs)
{ return rhs.operator bool(); }
inline bool operator==(const char *lhs, const string_t &rhs)
{ return lhs == rhs.ToCStr(); }
inline bool operator!=(const char *lhs, const string_t &rhs)
{ return lhs != rhs.ToCStr(); }

//-----------------------------------------------------------------------------

struct castable_string_t : public string_t // string_t is used in unions, hence, no constructor allowed
{
	castable_string_t( const char *pszFrom )	{ pszValue = (pszFrom && *pszFrom) ? pszFrom : NULL; }
};

//-----------------------------------------------------------------------------
// Purpose: The correct way to specify the NULL string as a constant.
//-----------------------------------------------------------------------------

#define NULL_STRING			NULL

//-----------------------------------------------------------------------------
// Purpose: Given a string_t, make a C string. By convention the result string 
// 			pointer should be considered transient and should not be stored.
//-----------------------------------------------------------------------------

#define STRING( string_t_obj )	(string_t_obj).ToCStr()

//-----------------------------------------------------------------------------
// Purpose: Given a C string, obtain a string_t
//-----------------------------------------------------------------------------

#define MAKE_STRING( c_str )	castable_string_t( ( c_str ) )

//-----------------------------------------------------------------------------

#define IDENT_STRINGS( s1, s2 )	( (s1) == (s2) )

//-----------------------------------------------------------------------------

inline void NetworkVarConstruct( string_t &x ) { x = NULL_STRING; }

//=============================================================================

#endif // STRING_T_H
