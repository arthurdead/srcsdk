//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
// $NoKeywords: $
//=============================================================================//

#ifndef DENSITY_WEIGHT_MAP_H
#define DENSITY_WEIGHT_MAP_H
#pragma once

#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include "networkvar.h"

#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#endif

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

enum density_type_t : unsigned char
{
	DENSITY_GAUSSIAN = 0, // Use a 2d gaussian function. Sigma based on BoundingRadius.
	DENSITY_GAUSSIANECLIPSE, // Uses mins/maxs, for rectangle shaped objects (buildings).
	DENSITY_NONE,
};

class DensityWeightsMap
{
public:
	DECLARE_CLASS_NOBASE( DensityWeightsMap );
	DensityWeightsMap();
	~DensityWeightsMap();

	void Init( CSharedBaseEntity *pOuter );
	void Destroy();
	void Clear();
	void SetType( density_type_t iType );
	density_type_t GetType();
	void OnCollisionSizeChanged();
	float Get( const Vector &vPos );
	int GetSizeX() { return m_iSizeX; }
	int GetSizeY() { return m_iSizeY; }

	void RecalculateWeights( const Vector &vMins, const Vector &vMaxs );
	void FillGaussian();
	void FillGaussianEclipse();

	void DebugPrintWeights();

private:
	CSharedBaseEntity *m_pOuter;
	Vector m_vMins, m_vMaxs;
	density_type_t m_iType;

	float **m_pWeights;
	int m_iSizeX, m_iSizeY;
	int m_iHalfSizeX, m_iHalfSizeY;
	float m_fXOffset, m_fYOffset;
};

inline void DensityWeightsMap::Clear()
{
	m_vMins = vec3_origin;
	m_vMaxs = vec3_origin;
}

inline density_type_t DensityWeightsMap::GetType()
{
	return m_iType;
}

#endif // DENSITY_WEIGHT_MAP_H