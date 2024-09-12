//========= Copyright (C) 2011, CSProMod Team, All rights reserved. =========//
//
// Purpose: provide world light related functions to the client
// 
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//
#ifndef WORLDLIGHT_H
#define WORLDLIGHT_H

#pragma once

#include "igamesystem.h" // CAutoGameSystem

class Vector;
struct dworldlight_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWorldLights : public CAutoGameSystemPerFrame
{
public:
	CWorldLights();
	~CWorldLights() { Clear(); }

	//-------------------------------------------------------------------------
	// Find the brightest light source at a point
	//-------------------------------------------------------------------------
	bool GetBrightestLightSource( const Vector& vecPosition, Vector& vecLightPos, Vector& vecLightBrightness ) const;
	void FindBrightestLightSource(const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness, int nCluster) const;
	bool GetCumulativeLightSource(const Vector &vecPosition, Vector &vecLightPos, float flMinBrightnessSqr) const;

	// CAutoGameSystem overrides
public:
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity() { Clear(); }

private:
	void Clear();

	int m_nWorldLights;
	dworldlight_t *m_pWorldLights;

	int m_iSunIndex = -1; // The sun's personal index

	struct clusterLightList_t
	{
		unsigned short	lightCount;
		unsigned short	firstLight;
	};

	CUtlVector<clusterLightList_t>		m_WorldLightsInCluster;
	CUtlVector<unsigned short>			m_WorldLightsIndexList;
};

//-----------------------------------------------------------------------------
// Singleton exposure
//-----------------------------------------------------------------------------
extern CWorldLights *g_pWorldLights;

#endif // WORLDLIGHT_H