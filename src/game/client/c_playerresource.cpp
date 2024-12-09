//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include "c_team.h"
#include "gamestringpool.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_DEBUG_NAME "WWWWWWWWWWWWWWW"

ConVar cl_names_debug( "cl_names_debug", "0", FCVAR_DEVELOPMENTONLY );

void RecvProxy_ChangedTeam( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// Have the regular proxy store the data.
	RecvProxy_Int32( pData, pStruct, pOut );

	if ( g_PR )
	{
		g_PR->TeamChanged();
	}
}

const float PLAYER_RESOURCE_THINK_INTERVAL = 0.2f;

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray3( RECVINFO_ARRAY(m_iPing), RecvPropInt( RECVINFO(m_iPing[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iScore), RecvPropInt( RECVINFO(m_iScore[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iDeaths), RecvPropInt( RECVINFO(m_iDeaths[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bConnected), RecvPropInt( RECVINFO(m_bConnected[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeam), RecvPropInt( RECVINFO(m_iTeam[0]), 0, RecvProxy_ChangedTeam)),
	RecvPropArray3( RECVINFO_ARRAY(m_bAlive), RecvPropInt( RECVINFO(m_bAlive[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iHealth), RecvPropInt( RECVINFO(m_iHealth[0]))),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_PlayerResource )

	DEFINE_ARRAY_FLAGS( m_szName, FIELD_STRING, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_iPing, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_iScore, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_iDeaths, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_bConnected, FIELD_BOOLEAN, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_iTeam, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_bAlive, FIELD_BOOLEAN, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),
	DEFINE_ARRAY_FLAGS( m_iHealth, FIELD_INTEGER, MAX_PLAYERS+1, FTYPEDESC_PRIVATE ),

END_PREDICTION_DATA()	

C_PlayerResource *g_PR;

IGameResources * GameResources( void ) { return g_PR; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	if(!g_PR)
		g_PR = this;

	memset( m_iPing, 0, sizeof( m_iPing ) );
//	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
	memset( m_iScore, 0, sizeof( m_iScore ) );
	memset( m_iDeaths, 0, sizeof( m_iDeaths ) );
	memset( m_bConnected, 0, sizeof( m_bConnected ) );
	memset( m_iTeam, 0, sizeof( m_iTeam ) );
	memset( m_bAlive, 0, sizeof( m_bAlive ) );
	memset( m_iHealth, 0, sizeof( m_iHealth ) );
	m_szUnconnectedName = NULL_STRING;
	
	for ( int i=0; i<MAX_TEAMS; i++ )
	{
		m_Colors[i] = COLOR_GREY;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
	if(g_PR == this)
		g_PR = NULL;
}

void C_PlayerResource::Spawn()
{
	if(g_PR && g_PR != this) {
		UTIL_Remove(this);
		return;
	}

	BaseClass::Spawn();
}

void C_PlayerResource::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetContextThink( &C_PlayerResource::PlayerNameThink, gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL, "PlayerNameThink" );
	}
}

void C_PlayerResource::UpdatePlayerName( int slot )
{
	if ( slot < 1 || slot > MAX_PLAYERS )
	{
		Error( "UpdatePlayerName with bogus slot %d\n", slot );
		return;
	}
	if (!m_szUnconnectedName )
		m_szUnconnectedName = AllocPooledString( PLAYER_UNCONNECTED_NAME );
	
	player_info_t sPlayerInfo;
	if ( IsConnected( slot ) && engine->GetPlayerInfo( slot, &sPlayerInfo ) )
	{
		m_szName[slot] = AllocPooledString( sPlayerInfo.name );
	}
	else 
	{
		m_szName[slot] = m_szUnconnectedName;
	}
}

void C_PlayerResource::PlayerNameThink()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		UpdatePlayerName( i );
	}

	SetNextThink( gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL, "PlayerNameThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_PlayerResource::GetPlayerName( int iIndex )
{
	if ( cl_names_debug.GetInt() )
		return PLAYER_DEBUG_NAME;

	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return PLAYER_ERROR_NAME;
	}
	
	if ( !IsConnected( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	// X360TBD: Network - figure out why the name isn't set
	if ( !m_szName[ iIndex ] || !Q_stricmp( STRING( m_szName[ iIndex ] ), PLAYER_UNCONNECTED_NAME ) )
	{
		// If you get a full "reset" uncompressed update from server, then you can have NULLNAME show up in the scoreboard
		UpdatePlayerName( iIndex );
	}

	// This gets updated in ClientThink, so it could be up to 1 second out of date, oh well.
	return STRING( m_szName[iIndex] );
}

bool C_PlayerResource::IsAlive(int iIndex )
{
	return m_bAlive[iIndex];
}

Team_t C_PlayerResource::GetTeam(int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}

const char * C_PlayerResource::GetTeamName(Team_t index)
{
	C_Team *team = GetGlobalTeamByTeam( index );

	if ( !team )
		return "Unknown";

	return team->Get_Name();
}

int C_PlayerResource::GetTeamScore(Team_t index)
{
	C_Team *team = GetGlobalTeamByTeam( index );

	if ( !team )
		return 0;

	return team->Get_Score();
}

int C_PlayerResource::GetFrags(int index )
{
	return 666;
}

bool C_PlayerResource::IsLocalPlayer(int index)
{
	C_BasePlayer *pPlayer =	C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return false;

	return ( index == pPlayer->entindex() );
}


bool C_PlayerResource::IsHLTV(int index)
{
	if ( !IsConnected( index ) )
		return false;

	player_info_t sPlayerInfo;
	
	if ( engine->GetPlayerInfo( index, &sPlayerInfo ) )
	{
		return sPlayerInfo.ishltv;
	}
	
	return false;
}

bool C_PlayerResource::IsReplay(int index)
{
#if defined( REPLAY_ENABLED )
	if ( !IsConnected( index ) )
		return false;

	player_info_t sPlayerInfo;

	if ( engine->GetPlayerInfo( index, &sPlayerInfo ) )
	{
		return sPlayerInfo.isreplay;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsFakePlayer( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return false;

	// Yuck, make sure it's up to date
	player_info_t sPlayerInfo;
	if ( engine->GetPlayerInfo( iIndex, &sPlayerInfo ) )
	{
		return sPlayerInfo.fakeplayer;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPing( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
/*-----------------------------------------------------------------------------
int	C_PlayerResource::GetPacketloss( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iPacketloss[iIndex];
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPlayerScore( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetDeaths( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetHealth( int iIndex )
{
	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iHealth[iIndex];
}

const Color &C_PlayerResource::GetTeamColor(Team_t index )
{
	if ( index < 0 || index >= MAX_TEAMS )
	{
		Assert( false );
		static Color blah;
		return blah;
	}
	else
	{
		return m_Colors[index];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsConnected( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_bConnected[iIndex];
}
