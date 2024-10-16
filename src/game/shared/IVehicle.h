//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVEHICLE_H
#define IVEHICLE_H

#pragma once

#include "baseplayer_shared.h"

class CUserCmd;
class IMoveHelper;
class CMoveData;

#ifdef GAME_DLL
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
#else
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
#endif

// This is used by the player to access vehicles. It's an interface so the
// vehicles are not restricted in what they can derive from.
abstract_class IVehicle
{
public:
	// Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
	virtual CSharedBaseCombatCharacter*	GetPassenger( int nRole = VEHICLE_ROLE_DRIVER ) = 0;
	virtual int						GetPassengerRole( CSharedBaseCombatCharacter *pPassenger ) = 0;
	
	// Where is the passenger seeing from?
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL ) = 0;

	// Does the player use his normal weapons while in this mode?
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) = 0;

	// Process movement
	virtual void			SetupMove( CSharedBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) = 0;
	virtual void			ProcessMovement( CSharedBasePlayer *pPlayer, CMoveData *pMoveData ) = 0;
	virtual void			FinishMove( CSharedBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) = 0;

	// Process input
	virtual void			ItemPostFrame( CSharedBasePlayer *pPlayer ) = 0;
};


#endif // IVEHICLE_H
