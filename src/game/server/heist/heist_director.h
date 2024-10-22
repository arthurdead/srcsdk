#ifndef HEIST_DIRECTOR_H
#define HEIST_DIRECTOR_H

#pragma once

#include "igamesystem.h"
#include "heist_shareddefs.h"
#include "util_shared.h"

class CHeistPlayer;

class CMissionDirector : public CAutoGameSystemPerFrame
{
public:
	CMissionDirector();

	void MakeMissionLoud();
	int GetMissionState() const;

	bool IsMissionLoud() const
	{ return GetMissionState() == MISSION_STATE_LOUD; }

	void PlayerSpawned(CHeistPlayer *pPlayer);

	void FrameUpdatePostEntityThink() override;
	void LevelInitPostEntity() override;

private:
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_casing", mission_casing, "", FCVAR_NONE)
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_loud", mission_loud, "", FCVAR_NONE)

	int m_AssaultID;

	enum
	{
		ASSAULT_INVALID,
		ASSAULT_WAITING,
		ASSAULT_WAITING_ALL_TO_SPAWN,
		ASSAULT_WAITING_ALL_TO_DIE,
	};

	int m_AssaultStatus;

	int m_AssaultSpawned;

	CountdownTimer m_AssaultTimer;
	CountdownTimer m_SpawnTimer;
};

extern CMissionDirector *MissionDirector();

#endif
