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

#if defined( CLIENT_DLL )
	#undef CRecastMgrEnt
#endif

	DECLARE_NETWORKCLASS();

	virtual void Spawn();
};

CSharedRecastMgrEnt *GetRecastMgrEnt();

#endif // RECAST_MGR_ENT_H