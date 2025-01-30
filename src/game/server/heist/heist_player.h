#ifndef HEIST_PLAYER_H
#define HEIST_PLAYER_H

#pragma once

#include "player.h"

class CHeistPlayer : public CBaseExpresserPlayer
{
public:
	DECLARE_CLASS(CHeistPlayer, CBaseExpresserPlayer);
	DECLARE_SERVERCLASS();

	CHeistPlayer();
	~CHeistPlayer() override;

	void UpdateOnRemove() override;
	void Precache() override;
	void Spawn() override;

	Class_T Classify() override;

	void Weapon_FrameUpdate() override;

	void PreThink() override;
	void PostThink() override;

	void SelectItem( CBaseCombatWeapon *pWeapon ) override;
	bool Weapon_ShouldSelectItem( CBaseCombatWeapon *pWeapon ) override;

	void EquipMask();

private:
	CNetworkVar(bool, m_bMaskingUp);
	CNetworkScale(m_flLeaning);
};

inline CHeistPlayer *ToHeistPlayer(CBaseEntity *pEntity)
{
	if(!pEntity || !pEntity->IsPlayer()) {
		return NULL;
	}

	return assert_cast<CHeistPlayer *>(pEntity);
}

#endif
