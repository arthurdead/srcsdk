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

abstract_class IRecastMgr
{
public:
	// Used for debugging purposes on client
	virtual dtNavMesh* GetNavMesh( const char *meshName ) = 0;
	virtual dtNavMeshQuery* GetNavMeshQuery( const char *meshName ) = 0;
	virtual IMapMesh* GetMapMesh() = 0;
};

#endif // IRECASTMGR_H