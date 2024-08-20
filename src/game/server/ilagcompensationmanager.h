//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ILAGCOMPENSATIONMANAGER_H
#define ILAGCOMPENSATIONMANAGER_H
#pragma once

#include "tier0/platform.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"

class CBasePlayer;
class CBaseEntity;
class CUserCmd;

enum LagCompensationType
{
	LAG_COMPENSATE_BOUNDS,
	LAG_COMPENSATE_HITBOXES,
	LAG_COMPENSATE_HITBOXES_ALONG_RAY,
};

//-----------------------------------------------------------------------------
// Purpose: This is also an IServerSystem
//-----------------------------------------------------------------------------
abstract_class ILagCompensationManager
{
public:
	// Called during player movement to set up/restore after lag compensation
	virtual void	StartLagCompensation(
		CBasePlayer *player,
		LagCompensationType lagCompensationType,
		const Vector& weaponPos = vec3_origin,
		const QAngle &weaponAngles = vec3_angle,
		float weaponRange = 0.0f ) = 0;
	virtual void	FinishLagCompensation( CBasePlayer *player ) = 0;
	virtual bool	IsCurrentlyDoingLagCompensation() const = 0;

	// Mappers can flag certain additional entities to lag compensate, this handles them
	virtual void	AddAdditionalEntity( CBaseEntity *pEntity ) = 0;
	virtual void	RemoveAdditionalEntity( CBaseEntity *pEntity ) = 0;
};

extern ILagCompensationManager *lagcompensation;

#endif // ILAGCOMPENSATIONMANAGER_H
