//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "func_ladder.h"
#include "gamemovement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined( CLIENT_DLL )
/*static*/ ConVar sv_showladders( "sv_showladders", "0", 0, "Show bbox and dismount points for all ladders (must be set before level load.)\n" );
#endif

CUtlVector< CSharedFuncLadder * >	CSharedFuncLadder::s_Ladders;

#if defined( CLIENT_DLL )
#define CFuncLadder C_FuncLadder
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedFuncLadder::CFuncLadder() :
	m_bDisabled( false )
{
	s_Ladders.AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedFuncLadder::~CFuncLadder()
{
	s_Ladders.FindAndRemove( this );
}

#if defined( CLIENT_DLL )
#undef CFuncLadder
#endif

int CSharedFuncLadder::GetLadderCount()
{
	return s_Ladders.Count();
}

CSharedFuncLadder *CSharedFuncLadder::GetLadder( int index )
{
	if ( index < 0 || index >= s_Ladders.Count() )
		return NULL;

	return s_Ladders[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::Spawn()
{
	BaseClass::Spawn();

	// Entity is symbolid
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	
	//AddFlag( FL_WORLDBRUSH );
	SetModelName( NULL_STRING );

	// Make entity invisible
	AddEffects( EF_NODRAW );
	// No model but should still network
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs = VEC_HULL_MAX;

	// This will swap them if they are inverted
	SetEndPoints( m_vecPlayerMountPositionTop, m_vecPlayerMountPositionBottom );

#if !defined( CLIENT_DLL )
	trace_t bottomtrace, toptrace;
	UTIL_TraceHull( m_vecPlayerMountPositionBottom, m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &bottomtrace );
	UTIL_TraceHull( m_vecPlayerMountPositionTop, m_vecPlayerMountPositionTop, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &toptrace );

	if ( bottomtrace.startsolid || toptrace.startsolid )
	{
		if ( bottomtrace.startsolid )
		{
			DevMsg( 1, "Warning, funcladder with blocked bottom point (%.2f %.2f %.2f) stuck in (%s)\n",
				m_vecPlayerMountPositionBottom.GetX(),
				m_vecPlayerMountPositionBottom.GetY(),
				m_vecPlayerMountPositionBottom.GetZ(),
				bottomtrace.m_pEnt 
					? 
					UTIL_VarArgs( "%s/%s", bottomtrace.m_pEnt->GetClassname(), bottomtrace.m_pEnt->GetEntityName().ToCStr() ) 
					: 
					"NULL" );
		}
		if ( toptrace.startsolid )
		{
			DevMsg( 1, "Warning, funcladder with blocked top point (%.2f %.2f %.2f) stuck in (%s)\n",
				m_vecPlayerMountPositionTop.GetX(),
				m_vecPlayerMountPositionTop.GetY(),
				m_vecPlayerMountPositionTop.GetZ(),
				toptrace.m_pEnt 
					? 
					UTIL_VarArgs( "%s/%s", toptrace.m_pEnt->GetClassname(), toptrace.m_pEnt->GetEntityName().ToCStr() ) 
					: 
					"NULL" );
		}

		// Force geometry overlays on, but only if developer 2 is set...
		if ( sv_showladders.GetBool() )
		{
			m_debugOverlays |= OVERLAY_TEXT_BIT;
		}
	}

	m_vecPlayerMountPositionTop -= GetAbsOrigin();
	m_vecPlayerMountPositionBottom -= GetAbsOrigin();

	// Compute mins, maxs of points
	// 
	Vector mins( MAX_COORD_INTEGER, MAX_COORD_INTEGER, MAX_COORD_INTEGER );
	Vector maxs( -MAX_COORD_INTEGER, -MAX_COORD_INTEGER, -MAX_COORD_INTEGER );
	int i;
	for ( i = 0; i < 3; i++ )
	{
		if ( m_vecPlayerMountPositionBottom[ i ] < mins[ i ] )
		{
			mins[ i ] = m_vecPlayerMountPositionBottom[ i ];
		}
		if ( m_vecPlayerMountPositionBottom[ i ] > maxs[ i ] )
		{
			maxs[ i ] = m_vecPlayerMountPositionBottom[ i ];
		}
		if ( m_vecPlayerMountPositionTop[ i ] < mins[ i ] )
		{
			mins[ i ] = m_vecPlayerMountPositionTop[ i ];
		}
		if ( m_vecPlayerMountPositionTop[ i ] > maxs[ i ] )
		{
			maxs[ i ] = m_vecPlayerMountPositionTop[ i ];
		}
	}

	// Expand mins/maxs by player hull size
	mins += playerMins;
	maxs += playerMaxs;

	UTIL_SetSize( this, mins, maxs );

	m_bFakeLadder = HasSpawnFlags(SF_LADDER_DONTGETON);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned or after reload from .sav file
//-----------------------------------------------------------------------------
void CSharedFuncLadder::Activate()
{
	// Chain to base class
	BaseClass::Activate();

#if !defined( CLIENT_DLL )
	// Re-hook up ladder dismount points
	SearchForDismountPoints();

	// Show debugging UI if it's active
	if ( sv_showladders.GetBool() )
	{
		m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::SearchForDismountPoints()
{
#if !defined( CLIENT_DLL )
	CUtlVector< InfoLadderDismountHandle > allNodes;

	Vector topPos;
	Vector bottomPos;

	GetTopPosition( topPos );
	GetBottomPosition( bottomPos );

	float dismount_radius = 100.0f;

	Vector vecBottomToTop = topPos - bottomPos;
	float ladderLength = VectorNormalize( vecBottomToTop );

	float recheck = 40.0f;

	// add both sets of nodes
	FindNearbyDismountPoints( topPos, dismount_radius, m_Dismounts );
	FindNearbyDismountPoints( bottomPos, dismount_radius, m_Dismounts );

	while ( 1 )
	{
		ladderLength -= recheck;
		if ( ladderLength <= 0.0f )
			break;
		bottomPos += recheck * vecBottomToTop;
		FindNearbyDismountPoints( bottomPos, dismount_radius, m_Dismounts );
	}
#endif
}

void CSharedFuncLadder::SetEndPoints( const Vector& p1, const Vector& p2 )
{
	m_vecPlayerMountPositionTop = p1;
	m_vecPlayerMountPositionBottom = p2;

	if ( m_vecPlayerMountPositionBottom.GetZ() > m_vecPlayerMountPositionTop.GetZ() )
	{
		Vector temp = m_vecPlayerMountPositionBottom;
		m_vecPlayerMountPositionBottom = m_vecPlayerMountPositionTop;
		m_vecPlayerMountPositionTop = temp;
	}

#if !defined( CLIENT_DLL)
	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs = VEC_HULL_MAX;

	trace_t result;
	UTIL_TraceHull( m_vecPlayerMountPositionTop + Vector( 0, 0, 4 ), m_vecPlayerMountPositionTop, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( !result.startsolid )
	{
		m_vecPlayerMountPositionTop = result.endpos;
	}

	UTIL_TraceHull( m_vecPlayerMountPositionBottom + Vector( 0, 0, 4 ), m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &result );

	if ( !result.startsolid )
	{
		m_vecPlayerMountPositionBottom = result.endpos;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::DrawDebugGeometryOverlays()
{
#if !defined( CLIENT_DLL )

	BaseClass::DrawDebugGeometryOverlays();

	Vector playerMins = VEC_HULL_MIN;
	Vector playerMaxs  = VEC_HULL_MAX;

	Vector topPosition;
	Vector bottomPosition;

	GetTopPosition( topPosition );
	GetBottomPosition( bottomPosition );

	NDebugOverlay::Box( topPosition, playerMins, playerMaxs, 255,0,0,127, 0 );
	NDebugOverlay::Box( bottomPosition, playerMins, playerMaxs, 0,0,255,127, 0 );

	NDebugOverlay::EntityBounds(this, 200, 180, 63, 63, 0);

	trace_t bottomtrace;
	UTIL_TraceHull( m_vecPlayerMountPositionBottom, m_vecPlayerMountPositionBottom, 
		playerMins, playerMaxs, MASK_PLAYERSOLID_BRUSHONLY, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &bottomtrace );

	int c = m_Dismounts.Count();
	for ( int i = 0 ; i < c ; i++ )
	{
		CInfoLadderDismount *pt = m_Dismounts[ i ];
		if ( !pt )
			continue;

		NDebugOverlay::Box(pt->GetAbsOrigin(),Vector( -16, -16, 0 ), Vector( 16, 16, 8 ), 150,0,0, 63, 0);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::GetTopPosition( Vector& org )
{
	ComputeAbsPosition( m_vecPlayerMountPositionTop + GetLocalOrigin(), &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::GetBottomPosition( Vector& org )
{
	ComputeAbsPosition( m_vecPlayerMountPositionBottom + GetLocalOrigin(), &org );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bottomToTopVec - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::ComputeLadderDir( Vector& bottomToTopVec )
{
	Vector top;
	Vector bottom;

	GetTopPosition( top );
	GetBottomPosition( bottom );

	bottomToTopVec = top - bottom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSharedFuncLadder::GetDismountCount() const
{
	return m_Dismounts.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CInfoLadderDismountHandle
//-----------------------------------------------------------------------------
CSharedInfoLadderDismount *CSharedFuncLadder::GetDismount( int index )
{
	if ( index < 0 || index >= m_Dismounts.Count() )
		return NULL;
	return m_Dismounts[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			radius - 
//			list - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::FindNearbyDismountPoints( const Vector& origin, float radius, CUtlVector< InfoLadderDismountHandle >& list )
{
#if !defined( CLIENT_DLL )
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassnameWithin( pEntity, "info_ladder_dismount", origin, radius)) != NULL )
	{
		CInfoLadderDismount *landingspot = static_cast< CInfoLadderDismount * >( pEntity );
		Assert( landingspot );

		// If spot has a target, then if the target is not this ladder, don't add to our list.
		if ( landingspot->m_target != NULL_STRING )
		{
			if ( landingspot->GetNextTarget() != this )
			{
				continue;
			}
		}

		InfoLadderDismountHandle handle;
		handle = landingspot;
		if ( list.Find( handle ) == list.InvalidIndex() )
		{
			list.AddToTail( handle  );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::InputEnable( inputdata_t &&inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::InputDisable( inputdata_t &&inputdata )
{
	m_bDisabled = true;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::InputForcePlayerOn( inputdata_t &&inputdata )
{
	//TODO Arthurdead!!!!
	//static_cast<CGameMovement*>(g_pGameMovement)->ForcePlayerOntoLadder(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::InputCheckPlayerOn( inputdata_t &&inputdata )
{
	//TODO Arthurdead!!!!
	//static_cast<CGameMovement*>(g_pGameMovement)->MountPlayerOntoLadder(this);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::PlayerGotOn( CSharedBasePlayer *pPlayer )
{
#if !defined( CLIENT_DLL )
	m_OnPlayerGotOnLadder.FireOutput(this, pPlayer);
	pPlayer->EmitSound( "Ladder.StepRight" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CSharedFuncLadder::PlayerGotOff( CSharedBasePlayer *pPlayer )
{
#if !defined( CLIENT_DLL )
	m_OnPlayerGotOffLadder.FireOutput(this, pPlayer);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedFuncLadder::DontGetOnLadder( void ) const
{
	return m_bFakeLadder;
}

#if !defined(CLIENT_DLL)
const char *CSharedFuncLadder::GetSurfacePropName()
{
	if ( !m_surfacePropName )
		return NULL;
	return m_surfacePropName.ToCStr();
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( FuncLadder, DT_FuncLadder );

BEGIN_NETWORK_TABLE( CSharedFuncLadder, DT_FuncLadder )
#if !defined( CLIENT_DLL )
	SendPropVector( SENDINFO( m_vecPlayerMountPositionTop ), SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecPlayerMountPositionBottom ), SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecLadderDir ), SPROP_COORD ),
	SendPropBool( SENDINFO( m_bFakeLadder ) ),
//	SendPropStringT( SENDINFO(m_surfacePropName) ),
#else
	RecvPropVector( RECVINFO( m_vecPlayerMountPositionTop ) ),
	RecvPropVector( RECVINFO( m_vecPlayerMountPositionBottom )),
	RecvPropVector( RECVINFO( m_vecLadderDir )),
	RecvPropBool( RECVINFO( m_bFakeLadder ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_SERVERCLASS( func_useableladder, CFuncLadder );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_MAPENTITY( CSharedFuncLadder, MAPENT_SOLIDCLASS, "Ladder. Players will be able to freely along one side of this brush, as if it was a ladder. If you are using a model prop for the visual representation of the ladder in the map, apply the toolsinvisibleladder material to the climbable side of the func_ladder brush." )
	DEFINE_MAP_FIELD( m_vecPlayerMountPositionTop,
		"point0",
		"Start",
		"Ladder start point."
	),
	DEFINE_MAP_FIELD( m_vecPlayerMountPositionBottom,
		"point1",
		"End"
		"Ladder end point."
	),

	DEFINE_MAP_FIELD( m_bDisabled,
		"StartDisabled",
		"Start Disabled",
		"0"
	),

#if !defined( CLIENT_DLL )
	DEFINE_MAP_FIELD( m_surfacePropName,
		"ladderSurfaceProperties",
		"Surface properties (optional)"
	),
	DEFINE_MAP_INPUT_FUNC( InputEnable,
		FIELD_VOID,
		"Enable",
		"Enable this ladder."
	),
	DEFINE_MAP_INPUT_FUNC( InputDisable,
		FIELD_VOID,
		"Disable",
		"Disable this ladder."
	),

	DEFINE_MAP_INPUT_FUNC( InputForcePlayerOn,
		FIELD_VOID,
		"ForcePlayerOn"
	),
	DEFINE_MAP_INPUT_FUNC( InputCheckPlayerOn,
		FIELD_VOID,
		"CheckPlayerOn"
	),

	DEFINE_MAP_OUTPUT( m_OnPlayerGotOnLadder,
		"OnPlayerGotOnLadder",
		"Fired whenever a player gets on this ladder."
	),
	DEFINE_MAP_OUTPUT( m_OnPlayerGotOffLadder,
		"OnPlayerGotOffLadder",
		"Fired whenever a player gets off this ladder."
	),
#endif

END_MAPENTITY()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedInfoLadderDismount::DrawDebugGeometryOverlays()
{
#if !defined( CLIENT_DLL )
	BaseClass::DrawDebugGeometryOverlays();

	if ( developer->GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, 0 ), Vector( 16, 16, 8 ), 127, 127, 127, 127, 0 );
	}
#endif
}

#if defined( GAME_DLL )
EdictStateFlags_t CSharedFuncLadder::UpdateTransmitState()
{
	// transmit if in PVS for clientside prediction
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( InfoLadderDismount, DT_InfoLadderDismount );

BEGIN_NETWORK_TABLE( CSharedInfoLadderDismount, DT_InfoLadderDismount )
END_NETWORK_TABLE()

LINK_ENTITY_TO_SERVERCLASS( info_ladder_dismount, CInfoLadderDismount );

#if defined(GAME_DLL)
const char *FuncLadder_GetSurfaceprops(CBaseEntity *pLadderEntity)
{
	CFuncLadder *pLadder = dynamic_cast<CFuncLadder *>(pLadderEntity);
	if ( pLadder )
	{
		if ( pLadder->GetSurfacePropName() )
			return pLadder->GetSurfacePropName();
	}
	return "ladder";
}
#endif
