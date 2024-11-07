#ifndef HEIST_DIRECTOR_H
#define HEIST_DIRECTOR_H

#pragma once

#include "igamesystem.h"
#include "heist_shareddefs.h"
#include "util_shared.h"
#include "tier1/UtlStringMap.h"
#include "tier1/utlvector.h"
#include "tier1/utlstack.h"

DECLARE_LOGGING_CHANNEL( LOG_MISSION );

class CHeistPlayer;

class IMissionSpawner
{
public:
	virtual bool Parse(KeyValues *params) = 0;

	virtual bool IsDone() const = 0;

	virtual void Start() = 0;
	virtual void Update() = 0;
};

struct MissionLocation
{
	string_t targetname;
	Vector origin;
	QAngle angles;
};

struct MissionAssault
{
	float m_StartTime;
	const CUtlVector<IMissionSpawner *> *m_SpawnerSet;
};

using AllocSpawner_t = IMissionSpawner *(*)();

template <typename T>
IMissionSpawner *DefaultSpawnerAllocator();

class CMissionDirector : public CAutoGameSystemPerFrame
{
public:
	CMissionDirector();

	bool Init() override;

	void MakeMissionLoud();
	int GetMissionState() const;

	bool IsMissionLoud() const
	{ return GetMissionState() == MISSION_STATE_LOUD; }

	void PlayerSpawned(CHeistPlayer *pPlayer);

	void FrameUpdatePostEntityThink() override;
	void LevelInitPostEntity() override;

	void RegisterSpawnerFactory( const char *name, AllocSpawner_t func );

	const MissionLocation *FindLocation( const char *name ) const;

private:
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_casing", mission_casing, "", FCVAR_NONE)
	CON_COMMAND_MEMBER_F(CMissionDirector, "mission_loud", mission_loud, "", FCVAR_NONE)

	bool LoadMissionFile();

	enum
	{
		ASSAULT_FSM_INVALID,
		ASSAULT_FSM_ACTIVE,
		ASSAULT_FSM_SWITCH_TO_NEXT,
	};

	int m_nAssaultFSM;
	int m_iCurrentAssault;
	int m_iCurrentSpawner;

	IntervalTimer m_AssaultTimer;

	CUtlStringMap< AllocSpawner_t > m_SpawnerAllocators;

	CUtlVector< MissionAssault > m_Assaults;

	CUtlStringMap< MissionLocation > m_Locations;
	CUtlStringMap< IMissionSpawner * > m_Spawners;
	CUtlStringMap< CUtlVector<IMissionSpawner *> > m_SpawnerSets;
};

extern CMissionDirector *MissionDirector();

class CBasicMissionSpawner : public IMissionSpawner
{
public:
	bool Parse(KeyValues *params) override;

	bool IsDone() const override;

	void Start() override;
	void Update() override;

private:
	CUtlVector< const MissionLocation * > m_Locations;

	struct EntityInfo
	{
		KeyValues::AutoDelete keyvalues;
	};

	CUtlStringMap< EntityInfo > m_Entities;
};

#include "tier0/memdbgon.h"

template <typename T>
IMissionSpawner *DefaultSpawnerAllocator()
{ return new T; }

#include "tier0/memdbgoff.h"

#endif
