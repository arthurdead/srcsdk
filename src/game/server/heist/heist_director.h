#ifndef HEIST_DIRECTOR_H
#define HEIST_DIRECTOR_H

#pragma once

#include "igamesystem.h"
#include "heist_shareddefs.h"

class CHeistPlayer;

class CMissionDirector : public CAutoGameSystemPerFrame
{
public:
	void MakeMissionLoud();
	int GetMissionState() const;

	bool IsMissionLoud() const
	{ return GetMissionState() == MISSION_STATE_LOUD; }

	void PlayerSpawned(CHeistPlayer *pPlayer);

private:
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_casing", mission_casing, "", FCVAR_NONE)
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_loud", mission_loud, "", FCVAR_NONE)
};

extern CMissionDirector *MissionDirector();

#endif
