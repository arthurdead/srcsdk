//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTABLEID_H
#define PREDICTABLEID_H
#pragma once

#include "tier0/platform.h"
#include "datamap.h"

//-----------------------------------------------------------------------------
// Purpose: Wraps 32bit predictID to allow access and creation
//-----------------------------------------------------------------------------
class CPredictableId
{
public:
	// Construction
	CPredictableId( void );

	static void		ResetInstanceCounters( void );

	// Call this to set from data
	void			Init( unsigned char player, unsigned short command, const char *classname, const char *module, int line );

	// Get player index
	unsigned char				GetPlayer( void ) const;
	// Get hash value
	unsigned short				GetHash( void ) const;
	// Get index number
	unsigned char				GetInstanceNumber( void ) const;
	// Get command number
	unsigned short				GetCommandNumber( void ) const;

	// Check command number
//	bool			IsCommandNumberEqual( int testNumber ) const;

	// Client only
	void			SetAcknowledged( bool ack );
	bool			GetAcknowledged( void ) const;

	char const		*Describe( void ) const;

	// Equality test
	bool operator ==( const CPredictableId& other ) const;
	bool operator !=( const CPredictableId& other ) const;
private:
	void			SetCommandNumber( unsigned short commandNumber );
	void			SetPlayer( unsigned char playerIndex );
	void			SetInstanceNumber( unsigned char counter );

	// Encoding bits, should total 32
	unsigned short hash_ : 12;
	unsigned short command_ : 10;
	unsigned char player_ : 5;
	unsigned char instance_ : 4;
	bool ack_ : 1;
};

inline const CPredictableId INVALID_PREDICTABLE_ID;

// This can be empty, the class has a proper constructor
FORCEINLINE void NetworkVarConstruct( CPredictableId &x ) { x = INVALID_PREDICTABLE_ID; }

template< class Changer >
class CNetworkVarBase<CPredictableId, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkPredictableIdBase : public CNetworkVarBaseImpl< CPredictableId, Changer >
{
	typedef CNetworkVarBaseImpl< CPredictableId, Changer > base;

public:
	using CNetworkVarBaseImpl<CPredictableId, Changer>::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template<typename C>
struct NetworkVarType<CNetworkPredictableIdBase<C>>
{
	using type = CPredictableId;
};

DECLARE_FIELD_INFO( FIELD_PREDICTABLEID,		 CPredictableId ) 
DECLARE_FIELD_NETWORK_INFO( FIELD_PREDICTABLEID, CNetworkPredictableIdBase )

#endif // PREDICTABLEID_H
