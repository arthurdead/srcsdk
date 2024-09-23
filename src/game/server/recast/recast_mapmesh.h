//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MAPMESH_H
#define RECAST_MAPMESH_H

#pragma once

#include "recast/recast_imapmesh.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include "tier1/utlvector.h"
#include "bspflags.h"
#include "vcollide.h"
#include "recast/recast_imgr.h"

class CBaseEntity;

class CMapMesh : public IMapMesh
{
public:
	CMapMesh( MapMeshType_t type, bool bLog = true );
	~CMapMesh();

	bool Load( bool bDynamicOnly = false );
	void Clear( bool bDynamicOnly = false );
	void SetLog( bool bLog ) { m_bLog = bLog; }

	const float *GetVerts();
	int GetNumVerts();
	const int *GetTris();
	int GetNumTris();
	const float *GetNorms();
	const rcChunkyTriMesh *GetChunkyMesh();

	void SetBounds( const Vector &vMins, const Vector &vMaxs );
	const Vector &GetMinBounds();
	const Vector &GetMaxBounds();

	void AddEntity( CBaseEntity *pEnt );
	void AddEntityBBox( CBaseEntity *pEnt, const Vector &vMins, const Vector &vMaxs );

	const CUtlVector< Vector > &GetSampleOrigins();

private:
	bool IsTriangleInValidArea( const Vector *vTriangle, bool bCheckNoArea = true );
	void AddCollisionModelToMesh( const matrix3x4_t &transform, CPhysCollide const *pCollisionModel, 
			CUtlVector<float> &verts, CUtlVector<int> &triangles, int filterContents = CONTENTS_EMPTY );

	virtual bool GenerateDispVertsAndTris( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateStaticPropData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateDynamicPropData( CUtlVector<float> &verts, CUtlVector<int> &triangles );
	virtual bool GenerateBrushData( void *fileContent, CUtlVector<float> &verts, CUtlVector<int> &triangles );

private:
	MapMeshType_t m_Type;

	bool m_bLog;

	CUtlVector< float > m_Vertices;
	CUtlVector< int > m_Triangles; // Indices into m_Vertices
	CUtlVector< float > m_Normals;

	rcChunkyTriMesh* m_chunkyMesh;

	int m_iStaticVertCountEnd;
	int m_iStaticTrisCountEnd;

	Vector m_vMeshMins;
	Vector m_vMeshMaxs;

	CUtlVector< Vector > m_sampleOrigins;
};

inline void CMapMesh::SetBounds( const Vector &vMins, const Vector &vMaxs )
{
	m_vMeshMins = vMins;
	m_vMeshMaxs = vMaxs;
}

inline const Vector &CMapMesh::GetMinBounds()
{
	return m_vMeshMins;
}

inline const Vector &CMapMesh::GetMaxBounds()
{
	return m_vMeshMaxs;
}

inline const float *CMapMesh::GetVerts()
{
	return m_Vertices.Base();
}
inline int CMapMesh::GetNumVerts()
{
	return m_Vertices.Count() / 3;
}

inline const int *CMapMesh::GetTris()
{
	return m_Triangles.Base();
}
inline int CMapMesh::GetNumTris()
{
	return m_Triangles.Count() / 3;
}

inline const float *CMapMesh::GetNorms()
{
	return m_Normals.Base();
}

inline const rcChunkyTriMesh *CMapMesh::GetChunkyMesh()
{
	return m_chunkyMesh;
}

inline const CUtlVector< Vector > &CMapMesh::GetSampleOrigins()
{
	return m_sampleOrigins;
}

#endif // RECAST_MAPMESH_H