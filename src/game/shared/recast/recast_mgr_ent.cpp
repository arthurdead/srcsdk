//========= Copyright, Sandern Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "recast_mgr_ent.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( RecastMgrEnt, DT_RecastMgrEnt );
BEGIN_NETWORK_TABLE( CSharedRecastMgrEnt, DT_RecastMgrEnt )
END_NETWORK_TABLE()

LINK_ENTITY_TO_SERVERCLASS( recast_mgr, CRecastMgrEnt );

static CSharedRecastMgrEnt *s_pRecastMgrEnt = NULL;

CSharedRecastMgrEnt *GetRecastMgrEnt()
{
	return s_pRecastMgrEnt;
}

#if defined( CLIENT_DLL )
#define CRecastMgrEnt C_RecastMgrEnt
#endif

CSharedRecastMgrEnt::CRecastMgrEnt()
{
	if(!s_pRecastMgrEnt)
		s_pRecastMgrEnt = this;
}

CSharedRecastMgrEnt::~CRecastMgrEnt()
{
	if( s_pRecastMgrEnt == this )
		s_pRecastMgrEnt = NULL;
}

#if defined( CLIENT_DLL )
#undef CRecastMgrEnt
#endif

void CSharedRecastMgrEnt::Spawn()
{
	if(s_pRecastMgrEnt && s_pRecastMgrEnt != this) {
		UTIL_Remove(this);
		return;
	}

	BaseClass::Spawn();
}