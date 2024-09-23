//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IRECASTMGR_H
#define IRECASTMGR_H

#pragma once

#include "tier0/platform.h"

class dtNavMesh;
class dtNavMeshQuery;
class IMapMesh;

enum MapMeshType_t
{
	RECAST_MAPMESH_NPC,
	RECAST_MAPMESH_NPC_FLUID,
	RECAST_MAPMESH_PLAYER,
	RECAST_MAPMESH_NUM,
};

//keep this in-sync with recast_mesh.cpp
enum
{
	RECAST_NAVMESH_HUMAN, // Combine, Stalker, Zombie...
	RECAST_NAVMESH_SMALL_CENTERED, // Scanner
	RECAST_NAVMESH_WIDE_HUMAN, // Vortigaunt
	RECAST_NAVMESH_TINY, // Headcrab
	RECAST_NAVMESH_WIDE_SHORT, // Bullsquid
	RECAST_NAVMESH_MEDIUM, // Cremator
	RECAST_NAVMESH_TINY_CENTERED, // Manhack 
	RECAST_NAVMESH_LARGE, // Antlion Guard
	RECAST_NAVMESH_LARGE_CENTERED, // Mortar Synth
	RECAST_NAVMESH_MEDIUM_TALL, // Hunter
	RECAST_NAVMESH_TINY_FLUID, // Blob
	RECAST_NAVMESH_MEDIUMBIG, // Infested drone
	RECAST_NAVMESH_PLAYER,
	RECAST_NAVMESH_NUM,
};

typedef int NavMeshType_t;

const NavMeshType_t RECAST_NAVMESH_INVALID = (NavMeshType_t)-1;

abstract_class IRecastMgr
{
public:
	// Used for debugging purposes on client
	virtual dtNavMesh* GetNavMesh( NavMeshType_t type ) = 0;
	virtual dtNavMeshQuery* GetNavMeshQuery( NavMeshType_t type ) = 0;
	virtual IMapMesh* GetMapMesh( MapMeshType_t type ) = 0;
};

#endif // IRECASTMGR_H