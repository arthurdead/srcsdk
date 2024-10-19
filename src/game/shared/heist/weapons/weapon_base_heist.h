#ifndef WEAPON_BASE_HEIST_H
#define WEAPON_BASE_HEIST_H

#pragma once

#include "basecombatweapon_shared.h"

class CBaseHeistWeapon : public CSharedBaseCombatWeapon
{
public:
	DECLARE_CLASS(CBaseHeistWeapon, CSharedBaseCombatWeapon)

	bool CanDeploy() override;
	bool Deploy() override;
};

#endif
