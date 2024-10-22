#include "cbase.h"
#include "heist_player_shared.h"

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CSharedHeistPlayer::SelectItem( const char *pstr, int iSubType )
{
	if (!pstr)
		return;

	CSharedBaseCombatWeapon *pItem = Weapon_OwnsThisType( pstr, iSubType );

	if (!pItem)
		return;

	if( GetObserverMode() != OBS_MODE_NONE )
		return;// Observers can't select things.

	if ( !Weapon_ShouldSelectItem( pItem ) )
		return;

	// FIX, this needs to queue them up and delay
	// Make sure the current weapon can be holstered
	if ( GetActiveWeapon() )
	{
		if ( !GetActiveWeapon()->CanHolster() && !pItem->ForceWeaponSwitch() )
			return;

		ResetAutoaim( );
	}

	if( IsSuitEquipped() ) {
		Weapon_Switch( pItem, VIEWMODEL_WEAPON, true );
	} else {
		Weapon_Switch( pItem, VIEWMODEL_WEAPON, false );
		EquipMask();
	}
}

void CSharedHeistPlayer::EquipMask()
{
	CSharedBaseViewModel *pViewModel = GetViewModel(VIEWMODEL_HANDS, false);
	if(!pViewModel->IsEffectActive(EF_NODRAW))
		return;

	pViewModel->RemoveEffects(EF_NODRAW);
	pViewModel->SendViewModelMatchingSequence(0);

	float duration = gpGlobals->curtime + pViewModel->SequenceDuration();
	SetNextAttack(duration);
}
