//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MGR_ENT_H
#define RECAST_MGR_ENT_H

#pragma once

#ifdef CLIENT_DLL
#include "c_baseentity.h"
#else
#include "baseentity.h"
#endif
#include "recast/recast_imgr.h"

enum SFRecastMgr_t : uint64
{
	SF_DISABLE_MESH_FLAGS_START = (1 << 0),

	SF_DISABLE_MESH_HUMAN = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_HUMAN),
	SF_DISABLE_MESH_SMALL_CENTERED = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_SMALL_CENTERED),
	SF_DISABLE_MESH_WIDE_HUMAN = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_WIDE_HUMAN),
	SF_DISABLE_MESH_TINY = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_TINY),
	SF_DISABLE_MESH_WIDE_SHORT = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_WIDE_SHORT),
	SF_DISABLE_MESH_MEDIUM = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_MEDIUM),
	SF_DISABLE_MESH_TINY_CENTERED = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_TINY_CENTERED),
	SF_DISABLE_MESH_LARGE = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_LARGE),
	SF_DISABLE_MESH_LARGE_CENTERED = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_LARGE_CENTERED),
	SF_DISABLE_MESH_MEDIUM_TALL = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_MEDIUM_TALL),
	SF_DISABLE_MESH_TINY_FLUID = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_TINY_FLUID),
	SF_DISABLE_MESH_MEDIUMBIG = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_MEDIUMBIG),
	SF_DISABLE_MESH_PLAYER = (SF_DISABLE_MESH_FLAGS_START << RECAST_NAVMESH_PLAYER),
};

FLAGENUM_OPERATORS( SFRecastMgr_t, uint64 )

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
class C_RecastMgrEnt;
typedef C_RecastMgrEnt CSharedRecastMgrEnt;
#else
class CRecastMgrEnt;
typedef CRecastMgrEnt CSharedRecastMgrEnt;
#endif

#if defined( CLIENT_DLL )
#define CRecastMgrEnt C_RecastMgrEnt
#endif

class CRecastMgrEnt : public CSharedPointEntity
{
public:
	DECLARE_CLASS( CRecastMgrEnt, CSharedPointEntity );
	CRecastMgrEnt();
	~CRecastMgrEnt();

	DECLARE_SPAWNFLAGS( SFRecastMgr_t )

#if defined( CLIENT_DLL )
	#undef CRecastMgrEnt
#endif

	DECLARE_NETWORKCLASS();

	virtual void Spawn();
};

CSharedRecastMgrEnt *GetRecastMgrEnt();

#endif // RECAST_MGR_ENT_H