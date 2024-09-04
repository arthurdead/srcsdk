//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MOVETYPE_PUSH_H
#define MOVETYPE_PUSH_H
#pragma once

#include "mathlib/vector.h"
#include "ehandle.h"

const int MAX_PUSHED_ENTITIES = 32;
struct physicspushlist_t
{
	float	localMoveTime;
	Vector	localOrigin;
	QAngle	localAngles;
	int		pushedCount;
	EHANDLE	pushedEnts[MAX_PUSHED_ENTITIES];
	Vector	pushVec[MAX_PUSHED_ENTITIES];
};

#endif // MOVETYPE_PUSH_H
