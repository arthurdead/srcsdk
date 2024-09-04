//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef WEAPON_IFMBASE_H
#define WEAPON_IFMBASE_H
#pragma once

#include "basecombatweapon_shared.h"

#if defined( CLIENT_DLL )
	#define CWeaponIFMBase C_WeaponIFMBase
#endif

class CWeaponIFMBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponIFMBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponIFMBase();

	// All predicted weapons need to implement and return true
	virtual bool	IsPredicted() const;
	
//	virtual void	FallInit( void );
	
public:
#if defined( CLIENT_DLL )
	virtual bool	ShouldPredict();
	virtual void	OnDataChanged( DataUpdateType_t type );
#else
	virtual void	Spawn();

	// FIXME: How should this work? This is a hack to get things working
	virtual const unsigned char *GetEncryptionKey( void ) { return NULL; }
#endif

private:
	CWeaponIFMBase( const CWeaponIFMBase & );
};


#endif // WEAPON_IFMBASE_H
