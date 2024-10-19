#ifndef C_HEIST_PLAYER_H
#define C_HEIST_PLAYER_H

#pragma once

#include "c_baseplayer.h"
#include "heist_player_shared.h"

class C_HeistPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS(C_HeistPlayer, C_BasePlayer);
	DECLARE_CLIENTCLASS();

	C_HeistPlayer();
	~C_HeistPlayer();

	static C_HeistPlayer *GetLocalHeistPlayer()
	{ return (C_HeistPlayer *)C_BasePlayer::GetLocalPlayer(); }

	void PostThink() override;

private:
	C_HeistPlayer(const C_HeistPlayer &);
};

inline C_HeistPlayer *ToHeistPlayer(C_BaseEntity *pEntity)
{
	if(!pEntity || !pEntity->IsPlayer()) {
		return NULL;
	}

	return assert_cast<C_HeistPlayer *>(pEntity);
}

typedef C_HeistPlayer CSharedHeistPlayer;

#endif
