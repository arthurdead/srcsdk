//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_OFFMESH_CONNECTION_H
#define RECAST_OFFMESH_CONNECTION_H

#pragma once

#include "baseentity.h"

#define SF_OFFMESHCONN_JUMPEDGE         (1 << 0)

#define SF_OFFMESHCONN_TYPE_FLAGS_START (1 << 1)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class COffMeshConnection : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( COffMeshConnection, CServerOnlyPointEntity );
};

#endif // RECAST_OFFMESH_CONNECTION_H