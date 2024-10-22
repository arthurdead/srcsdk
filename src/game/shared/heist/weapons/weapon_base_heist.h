#ifndef WEAPON_BASE_HEIST_H
#define WEAPON_BASE_HEIST_H

#pragma once

#include "basecombatweapon_shared.h"

#ifdef CLIENT_DLL
class C_BaseHeistWeapon;
typedef C_BaseHeistWeapon CSharedBaseHeistWeapon;
#else
class CBaseHeistWeapon;
typedef CBaseHeistWeapon CSharedBaseHeistWeapon;
#endif

#ifdef CLIENT_DLL
	#define CBaseHeistWeapon C_BaseHeistWeapon
#endif

class CBaseHeistWeapon : public CSharedBaseCombatWeapon
{
public:
	DECLARE_CLASS(CBaseHeistWeapon, CSharedBaseCombatWeapon)

#ifdef CLIENT_DLL
	#undef CBaseHeistWeapon
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	bool CanDeploy() override;
	bool ShouldBlockPrimaryFire() override;
	bool CanReload() override;
	bool CanPerformSecondaryAttack() const override;
};

#endif
