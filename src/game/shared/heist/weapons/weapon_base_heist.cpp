#include "weapon_base_heist.h"
#include "predictable_entity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_NETWORK_TABLE(CSharedBaseHeistWeapon, DT_BaseHeistWeapon)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(BaseHeistWeapon, DT_BaseHeistWeapon)

LINK_ENTITY_TO_CLASS_ALIASED(weapon_heist_base, BaseHeistWeapon);

BEGIN_PREDICTION_DATA( C_BaseHeistWeapon )
END_PREDICTION_DATA()

bool CSharedBaseHeistWeapon::CanDeploy()
{
	CSharedBasePlayer *pOwner = GetPlayerOwner();
	if(pOwner) {
		if(!pOwner->IsSuitEquipped()) {
			return false;
		}
	}

	return BaseClass::CanDeploy();
}

bool CSharedBaseHeistWeapon::ShouldBlockPrimaryFire()
{
	CSharedBasePlayer *pOwner = GetPlayerOwner();
	if(pOwner) {
		if(!pOwner->IsSuitEquipped()) {
			return true;
		}
	}

	return BaseClass::ShouldBlockPrimaryFire();
}

bool CSharedBaseHeistWeapon::CanReload()
{
	CSharedBasePlayer *pOwner = GetPlayerOwner();
	if(pOwner) {
		if(!pOwner->IsSuitEquipped()) {
			return false;
		}
	}

	return BaseClass::CanReload();
}

bool CSharedBaseHeistWeapon::CanPerformSecondaryAttack() const
{
	CSharedBasePlayer *pOwner = GetPlayerOwner();
	if(pOwner) {
		if(!pOwner->IsSuitEquipped()) {
			return false;
		}
	}

	return BaseClass::CanPerformSecondaryAttack();
}
