//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FUNC_LADDER_H
#define FUNC_LADDER_H
#pragma once

#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#endif

#if defined( CLIENT_DLL )
class C_InfoLadderDismount;
typedef C_InfoLadderDismount CSharedInfoLadderDismount;
class C_FuncLadder;
typedef C_FuncLadder CSharedFuncLadder;
#else
class CInfoLadderDismount;
typedef CInfoLadderDismount CSharedInfoLadderDismount;
class CFuncLadder;
typedef CFuncLadder CSharedFuncLadder;
#endif

#if defined( CLIENT_DLL )
#define CInfoLadderDismount C_InfoLadderDismount
#endif

class CInfoLadderDismount : public CSharedBaseEntity
{
public:
	DECLARE_CLASS( CInfoLadderDismount, CSharedBaseEntity );

#if defined( CLIENT_DLL )
	#undef CInfoLadderDismount
#endif

	DECLARE_NETWORKCLASS();

	virtual void DrawDebugGeometryOverlays();
};

typedef CHandle< CSharedInfoLadderDismount > InfoLadderDismountHandle;

// Spawnflags
#define SF_LADDER_DONTGETON			1			// Set for ladders that are acting as automount points, but not really ladders

#if defined( CLIENT_DLL )
#define CFuncLadder C_FuncLadder
#endif

//-----------------------------------------------------------------------------
// Purpose: A player-climbable ladder
//-----------------------------------------------------------------------------
class CFuncLadder : public CSharedBaseEntity
{
public:
	DECLARE_CLASS( CFuncLadder, CSharedBaseEntity );
	CFuncLadder();
	~CFuncLadder();

#if defined( CLIENT_DLL )
	#undef CFuncLadder
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_MAPENTITY();

	virtual void Spawn();

	virtual void DrawDebugGeometryOverlays(void);

	int					GetDismountCount() const;
	CSharedInfoLadderDismount	*GetDismount( int index );

	void	GetTopPosition( Vector& org );
	void	GetBottomPosition( Vector& org );
	void	ComputeLadderDir( Vector& bottomToTopVec );

	void	SetEndPoints( const Vector& p1, const Vector& p2 );

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

#ifdef GAME_DLL
	void	InputForcePlayerOn( inputdata_t &inputdata );
	void	InputCheckPlayerOn( inputdata_t &inputdata );
#endif

	bool	IsEnabled() const;

	void	PlayerGotOn( CSharedBasePlayer *pPlayer );
	void	PlayerGotOff( CSharedBasePlayer *pPlayer );

	virtual void Activate();

	bool	DontGetOnLadder( void ) const;

	static int GetLadderCount();
	static CSharedFuncLadder *GetLadder( int index );
	static CUtlVector< CSharedFuncLadder * >	s_Ladders;
public:

	void FindNearbyDismountPoints( const Vector& origin, float radius, CUtlVector< InfoLadderDismountHandle >& list );
	const char *GetSurfacePropName();

private:


	void	SearchForDismountPoints();

	// Movement vector from "bottom" to "top" of ladder
	CNetworkVector( m_vecLadderDir );

	// Dismount points near top/bottom of ladder, precomputed
	CUtlVector< InfoLadderDismountHandle > m_Dismounts;

	// Endpoints for checking for mount/dismount
	CNetworkVector( m_vecPlayerMountPositionTop );
	CNetworkVector( m_vecPlayerMountPositionBottom );

	bool		m_bDisabled;
	CNetworkVar( bool,	m_bFakeLadder );

#if defined( GAME_DLL )
	string_t	m_surfacePropName;
	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent	m_OnPlayerGotOnLadder;
	COutputEvent	m_OnPlayerGotOffLadder;

	virtual int UpdateTransmitState();
#endif
};

inline bool CSharedFuncLadder::IsEnabled() const
{
	return !m_bDisabled;
}

const char *FuncLadder_GetSurfaceprops(CSharedBaseEntity *pLadderEntity);

#endif // FUNC_LADDER_H
