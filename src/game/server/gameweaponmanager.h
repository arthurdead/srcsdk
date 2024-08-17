//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef GAMEWEAPONMANAGER_H
#define GAMEWEAPONMANAGER_H

#pragma once

void CreateWeaponManager( const char *pWeaponName, int iMaxPieces );

class CBaseCombatWeapon;

void WeaponManager_AmmoMod( CBaseCombatWeapon *pWeapon );

void WeaponManager_AddManaged( CBaseEntity *pWeapon );
void WeaponManager_RemoveManaged( CBaseEntity *pWeapon );

#endif // GAMEWEAPONMANAGER_H
