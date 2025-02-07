//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A point entity that periodically emits sparks and "bzzt" sounds.
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVSPARK_H
#define ENVSPARK_H
#pragma once

#include "baseentity.h"

enum SFSpark_t : unsigned char
{
	SF_SPARK_START_ON			= (1 << 0),
	SF_SPARK_GLOW				= (1 << 1),
	SF_SPARK_SILENT			= (1 << 2),
	SF_SPARK_DIRECTIONAL		= (1 << 3),
};

FLAGENUM_OPERATORS( SFSpark_t, unsigned char )

class CEnvSpark : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvSpark, CPointEntity );

	CEnvSpark( void );

	DECLARE_SPAWNFLAGS( SFSpark_t )

	void	Spawn( void );
	void	Precache( void );
	void	SparkThink( void );

	void	StartSpark( void );
	void	StopSpark( void );

	// Input handlers
	void InputStartSpark( inputdata_t &&inputdata );
	void InputStopSpark( inputdata_t &&inputdata );
	void InputToggleSpark( inputdata_t &&inputdata );
	void InputSparkOnce( inputdata_t &&inputdata );

	bool IsSparking( void ){ return ( GetNextThink() != TICK_NEVER_THINK ); }
	
	DECLARE_MAPENTITY();

	float			m_flDelay;
	modelindex_t				m_nGlowSpriteIndex;
	int				m_nMagnitude;
	int				m_nTrailLength;

	COutputEvent	m_OnSpark;
};

#endif // ENVSPARK_H