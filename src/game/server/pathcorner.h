#ifndef PATHCORNER_H
#define PATHCORNER_H

#pragma once

#include "baseentity.h"
#include "trains.h"

class CPathCorner : public CPointEntity
{
public:
	DECLARE_CLASS( CPathCorner, CPointEntity );

	DECLARE_SPAWNFLAGS( SFCorner_t )

	void	Spawn( );
	float	GetDelay( void ) { return m_flWait; }
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	// Input handlers	
	void InputSetNextPathCorner( inputdata_t &&inputdata );
	void InputInPass( inputdata_t &&inputdata );

	DECLARE_MAPENTITY();

private:
	float			m_flWait;
	COutputEvent	m_OnPass;
};

#endif
