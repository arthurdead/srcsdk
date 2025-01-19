//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#pragma once
#ifndef AI_LOOKTARGET_H
#define AI_LOOKTARGET_H

#include "baseentity.h"

enum SFLookTarget_t : unsigned char
{
	SF_LOOKTARGET_ONLYONCE = 0x00000001,
};

FLAGENUM_OPERATORS( SFLookTarget_t, unsigned char )

//=============================================================================
//=============================================================================
class CAI_LookTarget : public CPointEntity
{
public:
	DECLARE_CLASS( CAI_LookTarget, CPointEntity );
	DECLARE_MAPENTITY();

	DECLARE_SPAWNFLAGS( SFLookTarget_t )

	CAI_LookTarget() { m_flTimeNextAvailable = -1; }

	// Debugging
	int DrawDebugTextOverlays(void);

	// Accessors & Availability
	bool IsEligible( CBaseEntity *pLooker );
	bool IsEnabled() { return !m_bDisabled; }
	bool IsAvailable() { return (gpGlobals->curtime > m_flTimeNextAvailable); }
	void Reserve( float flDuration );

	// Searching
	static CAI_LookTarget *GetFirstLookTarget();
	static CAI_LookTarget *GetNextLookTarget( CAI_LookTarget *pCurrentTarget );

	int		m_iContext;
	int		m_iPriority;

	void	Enable()	{ m_bDisabled = false; }
	void	Disable()	{ m_bDisabled = true; }

private:
	bool	m_bDisabled;
	float	m_flTimeNextAvailable;
	float	m_flMaxDist;
};

#endif//AI_LOOKTARGET_H