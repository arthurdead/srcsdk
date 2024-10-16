//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEPROJECTILE_H
#define BASEPROJECTILE_H
#pragma once

#ifdef GAME_DLL
#include "baseanimating.h"
#else
#include "c_baseanimating.h"
#endif

#ifdef CLIENT_DLL
class C_BaseProjectile;
typedef C_BaseProjectile CSharedBaseProjectile;
#else
class CBaseProjectile;
typedef CBaseProjectile CSharedBaseProjectile;
#endif

//=============================================================================
//
// Base Projectile.
//
//=============================================================================
#ifdef GAME_DLL
DECLARE_AUTO_LIST( IBaseProjectileAutoList );
#endif

#ifdef CLIENT_DLL
	#define CBaseProjectile C_BaseProjectile
#endif

class CBaseProjectile : public CSharedBaseAnimating
#ifdef GAME_DLL
, public IBaseProjectileAutoList
#endif
{
public:
	DECLARE_CLASS( CBaseProjectile, CSharedBaseAnimating );
	CBaseProjectile();

#ifdef CLIENT_DLL
	#undef CBaseProjectile
#endif

	DECLARE_NETWORKCLASS();

	virtual void Spawn();

#ifdef GAME_DLL
	virtual int GetBaseProjectileType() const { return -1; } // no base
	virtual int GetProjectileType() const { return -1; } // no type
	virtual int GetDestroyableHitCount( void ) const { return m_iDestroyableHitCount; }
	void IncrementDestroyableHitCount( void ) { ++m_iDestroyableHitCount; }

	virtual bool CanCollideWithTeammates() const { return m_bCanCollideWithTeammates; }
	virtual float GetCollideWithTeammatesDelay() const { return 0.25f; }
#endif // GAME_DLL

	virtual bool IsDestroyable( void ) { return false; }
	virtual void Destroy( bool bBlinkOut = true, bool bBreakRocket = false ) {}
	virtual void SetLauncher( CSharedBaseEntity *pLauncher );
	CSharedBaseEntity *GetOriginalLauncher() const { return m_hOriginalLauncher.Get(); }

protected:
#ifdef GAME_DLL
	void CollideWithTeammatesThink();

	int m_iDestroyableHitCount;
#endif // GAME_DLL

private:

#ifdef GAME_DLL
	void	ResetCollideWithTeammates();

	bool					m_bCanCollideWithTeammates;
#endif // GAME_DLL

	CNetworkHandle( CSharedBaseEntity, m_hOriginalLauncher );
};

#endif // BASEPROJECTILE_H
