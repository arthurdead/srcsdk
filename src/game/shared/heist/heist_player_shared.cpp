#include "cbase.h"
#include "heist_player_shared.h"
#include "heist_gamerules.h"

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#include "heist_director.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CSharedHeistPlayer::Weapon_ShouldSelectItem( CSharedBaseCombatWeapon *pWeapon )
{
	return BaseClass::Weapon_ShouldSelectItem( pWeapon );
}

void CSharedHeistPlayer::SelectItem( CSharedBaseCombatWeapon *pWeapon )
{
	// Observers can't select things.
	if( GetObserverMode() != OBS_MODE_NONE ) {
		return;
	}

	if ( !Weapon_ShouldSelectItem( pWeapon ) )
		return;

	// FIX, this needs to queue them up and delay
	// Make sure the current weapon can be holstered
	if ( GetActiveWeapon() )
	{
		if ( !GetActiveWeapon()->CanHolster() && !pWeapon->ForceWeaponSwitch() )
			return;

		ResetAutoaim( );
	}

	if( IsSuitEquipped() ) {
		Weapon_Switch( pWeapon, VIEWMODEL_WEAPON, true );
	} else {
		Weapon_Switch( pWeapon, VIEWMODEL_WEAPON, false );
		if(HeistGameRules()->GetMissionState() != MISSION_STATE_NONE) {
			EquipMask();
		}
	}
}

void CSharedHeistPlayer::EquipMask()
{
	if(m_bMaskingUp)
		return;

	CSharedBaseViewModel *pViewModel = GetViewModel(VIEWMODEL_HANDS, false);
	if(!pViewModel)
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

			bool finished = pViewModel->IsSequenceFinished();

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
