//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_DEBUGDRAWMESH_H
#define RECAST_DEBUGDRAWMESH_H

#pragma once

#include "recast_debugdraw.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/MaterialSystemUtil.h"

/// Mesh debug draw implementation.
class DebugDrawMesh : public duDebugDraw
{
public:
	DebugDrawMesh();
	~DebugDrawMesh();

	virtual void depthMask(bool state);
	virtual void texture(bool state);
	virtual void begin(duDebugDrawPrimitives prim, int num, float size = 1.0f);
	virtual void vertex(const float* pos, unsigned int color);
	virtual void vertex(const float x, const float y, const float z, unsigned int color);
	virtual void vertex(const float* pos, unsigned int color, const float* uv);
	virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	virtual void end();

private:
	CMaterialReference m_mat;
	bool m_bBindTexture;
	IMesh *m_pMesh;
	CMeshBuilder m_meshBuilder;
};


#endif // RECAST_DEBUGDRAWMESH_H