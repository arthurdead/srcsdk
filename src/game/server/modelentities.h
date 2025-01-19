//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MODELENTITIES_H
#define MODELENTITIES_H
#pragma once

#include "baseentity.h"
#include "baseentity_shared.h"

//!! replace this with generic start enabled/disabled
enum SFBrush_t : unsigned char
{
	SF_WALL_START_OFF = 0x0001,
	SF_IGNORE_PLAYERUSE = 0x0002
};

FLAGENUM_OPERATORS( SFBrush_t, unsigned char )

//-----------------------------------------------------------------------------
// Purpose: basic solid geometry
// enabled state:	brush is visible
// disabled state:	brush not visible
//-----------------------------------------------------------------------------
class CFuncBrush : public CBaseEntity
{
public:
	DECLARE_CLASS( CFuncBrush, CBaseEntity );

	virtual void Spawn( void );
	bool CreateVPhysics( void );

	DECLARE_SPAWNFLAGS( SFBrush_t )

	virtual EntityCaps_t ObjectCaps( void ) { return HasSpawnFlags(SF_IGNORE_PLAYERUSE) ? BaseClass::ObjectCaps() : BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; }

	virtual int DrawDebugTextOverlays( void );

	virtual void TurnOff( void );
	virtual void TurnOn( void );

	// Input handlers
	void InputTurnOff( inputdata_t &&inputdata );
	void InputTurnOn( inputdata_t &&inputdata );
	void InputToggle( inputdata_t &&inputdata );
	void InputSetExcluded( inputdata_t &&inputdata );
	void InputSetInvert( inputdata_t &&inputdata );

	enum BrushSolidities_e : unsigned char
	{
		BRUSHSOLID_TOGGLE = 0,
		BRUSHSOLID_NEVER  = 1,
		BRUSHSOLID_ALWAYS = 2,
	};

	BrushSolidities_e m_iSolidity;
	int m_iDisabled;
	bool m_bSolidBsp;
	string_t m_iszExcludedClass;
	bool m_bInvertExclusion;

	DECLARE_MAPENTITY();
	DECLARE_SERVERCLASS();

	virtual bool IsOn( void ) const;
};


#endif // MODELENTITIES_H
