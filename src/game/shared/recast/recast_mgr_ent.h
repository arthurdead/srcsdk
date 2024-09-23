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

#define SF_DISABLE_MESH_FLAGS_START (1 << 0)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
#define CRecastMgrEnt C_RecastMgrEnt
#endif
class CRecastMgrEnt : public CPointEntity
{
public:
	DECLARE_CLASS( CRecastMgrEnt, CPointEntity );
	DECLARE_NETWORKCLASS();

	CRecastMgrEnt();
	~CRecastMgrEnt();

	virtual void Spawn();
};

CRecastMgrEnt *GetRecastMgrEnt();

#endif // RECAST_MGR_ENT_H