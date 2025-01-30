#ifndef SUBS_H
#define SUBS_H

#pragma once

#include "baseentity.h"

class CBaseDMStart : public CPointEntity
{
public:
	DECLARE_CLASS( CBaseDMStart, CPointEntity );

	bool IsTriggered( CBaseEntity *pEntity );

	DECLARE_MAPENTITY();

	string_t m_Master;

private:
};

enum SFPlayerStart_t : unsigned char
{
	SF_PLAYER_START_MASTER =	1
};

FLAGENUM_OPERATORS( SFPlayerStart_t, unsigned char )

class CPlayerStart : public CPointEntity
{
public:
	DECLARE_CLASS( CPlayerStart, CPointEntity );

	DECLARE_SPAWNFLAGS( SFPlayerStart_t )
private:
};

#endif
