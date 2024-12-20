//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ====
//
// Purpose:
//
//=============================================================================

#ifndef LOGIC_PLAYERPROXY_H
#define LOGIC_PLAYERPROXY_H
#pragma once

#include "baseentity.h"

//-----------------------------------------------------------------------------
// Purpose: Used to relay outputs/inputs from the player to the world and vice versa
//-----------------------------------------------------------------------------
class CLogicPlayerProxy : public CLogicalEntity
{
	DECLARE_CLASS( CLogicPlayerProxy, CLogicalEntity );
	DECLARE_MAPENTITY();

public:
	// FIXME: Subclass


	COutputEvent m_OnFlashlightOn;
	COutputEvent m_OnFlashlightOff;

	COutputEvent m_PlayerHasAmmo;
	COutputEvent m_PlayerHasNoAmmo;
	COutputEvent m_PlayerDied;

	COutputInt m_RequestedPlayerHealth;

	void InputSetFlashlightSlowDrain( inputdata_t &&inputdata );
	void InputSetFlashlightNormalDrain( inputdata_t &&inputdata );
	void InputLowerWeapon( inputdata_t &&inputdata );
	void InputSetLocatorTargetEntity( inputdata_t &&inputdata );

	void InputRequestPlayerHealth( inputdata_t &&inputdata );
	void InputSetPlayerHealth( inputdata_t &&inputdata );
	void InputRequestAmmoState( inputdata_t &&inputdata );
	void InputEnableCappedPhysicsDamage( inputdata_t &&inputdata );
	void InputDisableCappedPhysicsDamage( inputdata_t &&inputdata );

	void Activate( void );

	bool PassesDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info );

	EHANDLE m_hPlayer;
};


#endif	// LOGIC_PLAYERPROXY_H