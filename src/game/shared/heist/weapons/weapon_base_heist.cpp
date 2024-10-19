#include "weapon_base_heist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CBaseHeistWeapon::CanDeploy()
{
#if 0
	CSharedBasePlayer *pPlayer  = GetPlayerOwner();
	if(pPlayer) {
		CSharedBaseViewModel *pViewModel = pPlayer->GetViewModel(VIEWMODEL_HANDS, false);
		if(!pPlayer->IsSuitEquipped() || (pViewModel && !pViewModel->IsEffectActive(EF_NODRAW))) {
			return false;
		}
	}
#endif

	return BaseClass::CanDeploy();
}

bool CBaseHeistWeapon::Deploy()
{
	CSharedBasePlayer *pPlayer  = GetPlayerOwner();
	if(pPlayer) {
		CSharedBaseViewModel *pViewModel = pPlayer->GetViewModel(VIEWMODEL_HANDS, false);
		if(!pPlayer->IsSuitEquipped() || (pViewModel && !pViewModel->IsEffectActive(EF_NODRAW))) {
		#ifdef GAME_DLL
			if(!pViewModel || pViewModel->IsEffectActive(EF_NODRAW)) {
				pPlayer->EquipSuit(true);
			}
		#endif
			return false;
		}
	}

	return BaseClass::Deploy();
}
