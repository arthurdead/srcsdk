//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_OFFMESH_CONNECTION_H
#define RECAST_OFFMESH_CONNECTION_H

#pragma once

#include "baseentity.h"

#define SF_OFFMESHCONN_HUMAN				0x000001
#define SF_OFFMESHCONN_MEDIUM				0x000002
#define SF_OFFMESHCONN_LARGE				0x000004
#define SF_OFFMESHCONN_VERYLARGE			0x000008
#define SF_OFFMESHCONN_AIR					0x000010

#define SF_OFFMESHCONN_JUMPEDGE				0x000020

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class COffMeshConnection : public CServerOnlyPointEntity
{
public:
	DECLARE_CLASS( COffMeshConnection, CServerOnlyPointEntity );
};

#endif // RECAST_OFFMESH_CONNECTION_H