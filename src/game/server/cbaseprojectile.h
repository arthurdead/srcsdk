//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base class for simple projectiles
//
// $NoKeywords: $
//=============================================================================//

#ifndef CBASEPROJECTILE_H
#define CBASEPROJECTILE_H
#pragma once

#include "baseanimating.h"

enum MoveType_t;
enum MoveCollide_t;

//=============================================================================
//=============================================================================
class CBaseProjectile : public CBaseAnimating
{
	DECLARE_CLASS( CBaseProjectile, CBaseAnimating );

public:
	void Touch( CBaseEntity *pOther );
	virtual void HandleTouch( CBaseEntity *pOther );

	void Think();
	virtual void HandleThink();

	void Spawn(	char *pszModel,
		const Vector &vecOrigin,
		const Vector &vecVelocity,
		edict_t *pOwner,
		MoveType_t	iMovetype,
		MoveCollide_t nMoveCollide,
		int	iDamage,
		int iDamageType,
		CBaseEntity *pIntendedTarget = NULL );

	virtual void Precache( void ) {};

	int	m_iDmg;
	int m_iDmgType;
	EHANDLE m_hIntendedTarget;
};

#endif // CBASEPROJECTILE_H
