//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "usercmd.h"
#include "bitbuf.h"
#include "checksum_md5.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TF2 specific, need enough space for OBJ_LAST items from tf_shareddefs.h
#define WEAPON_SUBTYPE_BITS	6

//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
ConVar net_showusercmd( "net_showusercmd", "0", 0, "Show user command encoding" );
#define LogUserCmd( msg, ... ) if ( net_showusercmd.GetInt() ) { ConDMsg( msg, __VA_ARGS__ ); }
#else
#define LogUserCmd( msg, ... ) 
#endif

void CUserCmd::Reset()
{
	command_number = 0;
	tick_count = 0;
	viewangles.Init();
	forwardmove = 0.0f;
	sidemove = 0.0f;
	upmove = 0.0f;
	buttons = 0;
	impulse = 0;
	weaponselect = 0;
	weaponsubtype = 0;
	random_seed = 0;
#ifdef GAME_DLL
	server_random_seed = 0;
#endif
	mousedx = 0;
	mousedy = 0;

#ifdef CLIENT_DLL
	hasbeenpredicted = false;
#endif
}

CUserCmd& CUserCmd::operator =( const CUserCmd& src )
{
	if ( this == &src )
		return *this;

	command_number		= src.command_number;
	tick_count			= src.tick_count;
	viewangles			= src.viewangles;
	forwardmove			= src.forwardmove;
	sidemove			= src.sidemove;
	upmove				= src.upmove;
	buttons				= src.buttons;
	impulse				= src.impulse;
	weaponselect		= src.weaponselect;
	weaponsubtype		= src.weaponsubtype;
	random_seed			= src.random_seed;
#ifdef GAME_DLL
	server_random_seed = src.server_random_seed;
#endif
	mousedx				= src.mousedx;
	mousedy				= src.mousedy;

#ifdef CLIENT_DLL
	hasbeenpredicted	= src.hasbeenpredicted;
#endif

	return *this;
}

CRC32_t CUserCmd::GetChecksum( void ) const
{
	CRC32_t crc;

	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, &command_number, sizeof( command_number ) );
	CRC32_ProcessBuffer( &crc, &tick_count, sizeof( tick_count ) );
	CRC32_ProcessBuffer( &crc, &viewangles, sizeof( viewangles ) );    
	CRC32_ProcessBuffer( &crc, &forwardmove, sizeof( forwardmove ) );   
	CRC32_ProcessBuffer( &crc, &sidemove, sizeof( sidemove ) );      
	CRC32_ProcessBuffer( &crc, &upmove, sizeof( upmove ) );         
	CRC32_ProcessBuffer( &crc, &buttons, sizeof( buttons ) );		
	CRC32_ProcessBuffer( &crc, &impulse, sizeof( impulse ) );        
	CRC32_ProcessBuffer( &crc, &weaponselect, sizeof( weaponselect ) );	
	CRC32_ProcessBuffer( &crc, &weaponsubtype, sizeof( weaponsubtype ) );
	CRC32_ProcessBuffer( &crc, &random_seed, sizeof( random_seed ) );
	CRC32_ProcessBuffer( &crc, &mousedx, sizeof( mousedx ) );
	CRC32_ProcessBuffer( &crc, &mousedy, sizeof( mousedy ) );
	CRC32_Final( &crc );

	return crc;
}

// Allow command, but negate gameplay-affecting values
void CUserCmd::MakeInert( void )
{
	viewangles = vec3_angle;
	forwardmove = 0.f;
	sidemove = 0.f;
	upmove = 0.f;
	buttons = 0;
	impulse = 0;
}

//-----------------------------------------------------------------------------
static bool WriteUserCmdDeltaInt( bf_write *buf, const char *what, int from, int to, int bits = 32 )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %d -> %d\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to, bits );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaULongLong( bf_write *buf, const char *what, uint64 from, uint64 to )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %lli -> %lli\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteULongLong( to );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaShort( bf_write *buf, const char *what, int from, int to )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %d -> %d\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteShort( to );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaFloat( bf_write *buf, const char *what, float from, float to )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %2.2f -> %2.2f\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteFloat( to );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaCoord( bf_write *buf, const char *what, float from, float to )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %2.2f -> %2.2f\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteBitCoord( to );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaAngle( bf_write *buf, const char *what, float from, float to, int bits )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s %2.2f -> %2.2f\n", what, from, to );

		buf->WriteOneBit( 1 );
		buf->WriteBitAngle( to, bits );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

static bool WriteUserCmdDeltaVec3Coord( bf_write *buf, const char *what, const Vector &from, const Vector &to )
{
	if ( from != to )
	{
		LogUserCmd( "\t%s [%2.2f, %2.2f, %2.2f] -> [%2.2f, %2.2f, %2.2f]\n", what, from.x, from.y, from.z, to.x, to.y, to.z );

		buf->WriteOneBit( 1 );
		buf->WriteBitVec3Coord( to );
		return true;
	}

	buf->WriteOneBit( 0 );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Write a delta compressed user command.
// Input  : *buf - 
//			*to - 
//			*from - 
// Output : static
//-----------------------------------------------------------------------------
void WriteUsercmd( bf_write *buf, const CUserCmd *to, const CUserCmd *from )
{
	LogUserCmd("WriteUsercmd: from=%d to=%d\n", from->command_number, to->command_number );

	WriteUserCmdDeltaInt( buf, "command_number", from->command_number + 1, to->command_number, 32 );
	WriteUserCmdDeltaInt( buf, "tick_count", from->tick_count + 1, to->tick_count, 32 );

	WriteUserCmdDeltaFloat( buf, "viewangles[0]", from->viewangles[0], to->viewangles[0] );
	WriteUserCmdDeltaFloat( buf, "viewangles[1]", from->viewangles[1], to->viewangles[1] );
	WriteUserCmdDeltaFloat( buf, "viewangles[2]", from->viewangles[2], to->viewangles[2] );

	WriteUserCmdDeltaFloat( buf, "forwardmove", from->forwardmove, to->forwardmove );
	WriteUserCmdDeltaFloat( buf, "sidemove", from->sidemove, to->sidemove );
	WriteUserCmdDeltaFloat( buf, "upmove", from->upmove, to->upmove );
	WriteUserCmdDeltaULongLong( buf, "buttons", from->buttons, to->buttons );
	WriteUserCmdDeltaInt( buf, "impulse", from->impulse, to->impulse, 8 );


	if ( WriteUserCmdDeltaInt( buf, "weaponselect", from->weaponselect, to->weaponselect, MAX_EDICT_BITS ) )
	{
		WriteUserCmdDeltaInt( buf, "weaponsubtype", from->weaponsubtype, to->weaponsubtype, WEAPON_SUBTYPE_BITS );
	}


	// TODO: Can probably get away with fewer bits.
	WriteUserCmdDeltaShort( buf, "mousedx", from->mousedx, to->mousedx );
	WriteUserCmdDeltaShort( buf, "mousedy", from->mousedy, to->mousedy );
}

//-----------------------------------------------------------------------------
// Purpose: Read in a delta compressed usercommand.
// Input  : *buf - 
//			*move - 
//			*from - 
// Output : static void ReadUsercmd
//-----------------------------------------------------------------------------
void ReadUsercmd( bf_read *buf, CUserCmd *move, CUserCmd *from, CSharedBasePlayer *pPlayer )
{
	// Assume no change
	*move = *from;

	if ( buf->ReadOneBit() )
	{
		move->command_number = buf->ReadUBitLong( 32 );
	}
	else
	{
		// Assume steady increment
		move->command_number = from->command_number + 1;
	}

	if ( buf->ReadOneBit() )
	{
		move->tick_count = buf->ReadUBitLong( 32 );
	}
	else
	{
		// Assume steady increment
		move->tick_count = from->tick_count + 1;
	}

	// Read direction
	if ( buf->ReadOneBit() )
	{
		move->viewangles[0] = buf->ReadFloat();
	}

	if ( buf->ReadOneBit() )
	{
		move->viewangles[1] = buf->ReadFloat();
	}

	if ( buf->ReadOneBit() )
	{
		move->viewangles[2] = buf->ReadFloat();
	}

	// Moved value validation and clamping to CBasePlayer::ProcessUsercmds()

	// Read movement
	if ( buf->ReadOneBit() )
	{
		move->forwardmove = buf->ReadFloat();
	}

	if ( buf->ReadOneBit() )
	{
		move->sidemove = buf->ReadFloat();
	}

	if ( buf->ReadOneBit() )
	{
		move->upmove = buf->ReadFloat();
	}

	// read buttons
	if ( buf->ReadOneBit() )
	{
		move->buttons = buf->ReadULongLong();
	}

	if ( buf->ReadOneBit() )
	{
		move->impulse = buf->ReadUBitLong( 8 );
	}


	if ( buf->ReadOneBit() )
	{
		move->weaponselect = buf->ReadUBitLong( MAX_EDICT_BITS );
		if ( buf->ReadOneBit() )
		{
			move->weaponsubtype = buf->ReadUBitLong( WEAPON_SUBTYPE_BITS );
		}
	}


	move->random_seed = MD5_PseudoRandom( move->command_number ) & 0x7fffffff;

	if ( buf->ReadOneBit() )
	{
		move->mousedx = buf->ReadShort();
	}

	if ( buf->ReadOneBit() )
	{
		move->mousedy = buf->ReadShort();
	}
}
