//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that simulates bullets that are underwater.
//
//=============================================================================//

#ifndef WEAPON_WATERBULLET_H
#define WEAPON_WATERBULLET_H
#pragma once

#include "baseanimating.h"

#define WATER_BULLET_BUBBLES_PER_INCH 0.05f

//=========================================================
//=========================================================
class CWaterBullet : public CBaseAnimating
{
	DECLARE_CLASS( CWaterBullet, CBaseAnimating );

public:
	void Precache();
	void Spawn( const Vector &vecOrigin, const Vector &vecDir );
	void Touch( CBaseEntity *pOther );
	void BulletThink();

	DECLARE_SERVERCLASS();
};

#endif // WEAPON_WATERBULLET_H
