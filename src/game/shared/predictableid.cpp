//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "checksum_crc.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Helper class for resetting instance numbers, etc.
//-----------------------------------------------------------------------------
class CPredictableIdHelper
{
public:
	CPredictableIdHelper()
	{
		Reset( -1 );
	}

	void	Reset( unsigned short command )
	{
		m_nCurrentCommand = command;
		m_nCount = 0;
		memset( m_Entries, 0, sizeof( m_Entries ) );
	}

	unsigned char		AddEntry( unsigned short command, unsigned short hash )
	{
		// Clear list if command number changes
		if ( command != m_nCurrentCommand )
		{
			Reset( command );
		}

		entry *e = FindOrAddEntry( hash );
		if ( !e )
			return 0;
		e->count++;
		return e->count-1;
	}

private:

	enum
	{
		MAX_ENTRIES = 255,
	};

	struct entry
	{
		unsigned short		hash : 12;
		unsigned char		count;
	};

	entry			*FindOrAddEntry( unsigned short hash )
	{
		int i;
		for ( i = 0; i < m_nCount; i++ )
		{
			entry *e = &m_Entries[ i ];
			if ( e->hash == hash )
				return e;
		}

		if ( m_nCount >= MAX_ENTRIES )
		{
			// Assert( 0 );
			return NULL;
		}

		entry *e = &m_Entries[ m_nCount++ ];
		e->hash = hash;
		e->count = 0;
		return e;
	}

	unsigned short				m_nCurrentCommand : 10;
	unsigned char				m_nCount;
	entry			m_Entries[ MAX_ENTRIES ];
};

static CPredictableIdHelper g_Helper;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPredictableId::CPredictableId( void )
{
	ack_ = false;
	player_ = 0;
	command_ = 0;
	hash_ = 0;
	instance_ = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPredictableId::ResetInstanceCounters( void )
{
	g_Helper.Reset( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : playerIndex - 
//-----------------------------------------------------------------------------
void CPredictableId::SetPlayer( unsigned char playerIndex )
{
	player_ = playerIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
unsigned char CPredictableId::GetPlayer( void ) const
{
	return player_;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
unsigned short CPredictableId::GetCommandNumber( void ) const
{
	return command_;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : commandNumber - 
//-----------------------------------------------------------------------------
void CPredictableId::SetCommandNumber( unsigned short commandNumber )
{
	command_ = commandNumber;
}

/*
bool CPredictableId::IsCommandNumberEqual( int testNumber ) const
{
	if ( ( testNumber & ((1<<10) - 1) ) == m_PredictableID.command )
		return true;

	return false;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//			*module - 
//			line - 
// Output : static int
//-----------------------------------------------------------------------------
static CRC32_t ClassFileLineHash( const char *classname, const char *module, int line )
{
	CRC32_t retval;

	CRC32_Init( &retval );

	char tempbuffer[ 512 ];
	
	// ACK, have to go lower case due to issues with .dsp having different cases of drive
	//  letters, etc.!!!
	Q_strncpy( tempbuffer, classname, sizeof( tempbuffer ) );
	Q_strlower( tempbuffer );
	CRC32_ProcessBuffer( &retval, (void *)tempbuffer, Q_strlen( tempbuffer ) );
	
	Q_strncpy( tempbuffer, module, sizeof( tempbuffer ) );
	Q_strlower( tempbuffer );
	CRC32_ProcessBuffer( &retval, (void *)tempbuffer, Q_strlen( tempbuffer ) );
	
	CRC32_ProcessBuffer( &retval, (void *)&line, sizeof( int ) );

	CRC32_Final( &retval );

	return retval;
}

//-----------------------------------------------------------------------------
// Purpose: Create a predictable id of the specified parameter set
// Input  : player - 
//			command - 
//			*classname - 
//			*module - 
//			line - 
//-----------------------------------------------------------------------------
void CPredictableId::Init( unsigned char player, unsigned short command, const char *classname, const char *module, int line )
{
	SetPlayer( player );
	SetCommandNumber( command );

	hash_ = (unsigned short)ClassFileLineHash( classname, module, line );

	// Use helper to determine instance number this command
	unsigned char instance = g_Helper.AddEntry( command, hash_ );

	// Set appropriate instance number
	SetInstanceNumber( instance );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
unsigned short CPredictableId::GetHash( void ) const
{
	return hash_;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : counter - 
//-----------------------------------------------------------------------------
void CPredictableId::SetInstanceNumber( unsigned char counter )
{
	instance_ = counter;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
unsigned char CPredictableId::GetInstanceNumber( void ) const
{
	return instance_;
}

// Client only
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ack - 
//-----------------------------------------------------------------------------
void CPredictableId::SetAcknowledged( bool ack )
{
	ack_ = ack;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPredictableId::GetAcknowledged( void ) const
{
	return ack_;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if one id is == another, ignores Acknowledged state
// Input  : other - 
// Output : bool CPredictableId::operator
//-----------------------------------------------------------------------------
bool CPredictableId::operator ==( const CPredictableId& other ) const
{
	if ( this == &other )
		return true;

	if ( GetPlayer() != other.GetPlayer() )
		return false;
	if ( GetCommandNumber() != other.GetCommandNumber() )
		return false;
	if ( GetHash() != other.GetHash() )
		return false;
	if ( GetInstanceNumber() != other.GetInstanceNumber() )
		return false;
	return true;
}

bool CPredictableId::operator !=( const CPredictableId& other ) const
{
	return !operator ==( other );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CPredictableId::Describe( void ) const
{
	static char desc[ 128 ];

	CSharedBasePlayer *pPlayer = UTIL_PlayerByIndex( GetPlayer() );

	Q_snprintf( desc, sizeof( desc ), "pl(%s)#%hhu cmd(%hu) hash(%hu) inst(%hhu) ack(%s)",
		pPlayer ? pPlayer->GetPlayerName() : "NULL", GetPlayer(),
		GetCommandNumber(),
		GetHash(),
		GetInstanceNumber() ,
		GetAcknowledged() ? "true" : "false" );

	return desc;
}
