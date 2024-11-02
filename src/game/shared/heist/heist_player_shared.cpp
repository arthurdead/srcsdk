#include "cbase.h"
#include "heist_player_shared.h"

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#include "heist_director.h"
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
	if(!pViewModel)
		return;

	if(!pViewModel->IsEffectActive(EF_NODRAW))
		return;

	pViewModel->RemoveEffects(EF_NODRAW);
	pViewModel->SendViewModelMatchingSequence(0);

	float duration = gpGlobals->curtime + pViewModel->SequenceDuration();
	SetNextAttack(duration);

	m_bMaskingUp = true;
}

void CSharedHeistPlayer::Weapon_FrameUpdate()
{
	BaseClass::Weapon_FrameUpdate();

	if(m_bMaskingUp) {
		CSharedBaseViewModel *pViewModel = GetViewModel(VIEWMODEL_HANDS, false);
		if(pViewModel) {
			pViewModel->StudioFrameAdvance();

			bool finished = (pViewModel->IsSequenceFinished() || pViewModel->GetCycle() >= 0.8f);

		#ifdef GAME_DLL
			if(finished) {
				MissionDirector()->MakeMissionLoud();
			}
		#endif

			if(finished) {
				pViewModel->AddEffects(EF_NODRAW);

			#ifdef GAME_DLL
				BaseClass::EquipSuit(false);
			#endif

				m_bMaskingUp = false;

				CSharedBaseCombatWeapon *pWeapon = GetActiveWeapon();
				if(pWeapon) {
					pWeapon->Deploy();
				} else {
					SwitchToNextBestWeapon(NULL);
				}
			}
		}
	}
}
