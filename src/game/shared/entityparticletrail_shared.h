//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYPARTICLETRAIL_SHARED_H
#define ENTITYPARTICLETRAIL_SHARED_H

#pragma once

#include "networkvar.h"
#include "datamap.h"

#ifdef CLIENT_DLL
#include "dt_recv.h"
#else
#include "dt_send.h"
#endif

//-----------------------------------------------------------------------------
// For networking this bad boy
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_EntityParticleTrailInfo );
#else
EXTERN_SEND_TABLE( DT_EntityParticleTrailInfo );
#endif


//-----------------------------------------------------------------------------
// Particle trail info
//-----------------------------------------------------------------------------
struct EntityParticleTrailInfo_t
{
	EntityParticleTrailInfo_t();

	DECLARE_CLASS_NOBASE( EntityParticleTrailInfo_t );
	DECLARE_SIMPLE_DATADESC();

	string_t m_strMaterialName;
	CNetworkVarForDerived( float, m_flLifetime );
	CNetworkVarForDerived( float, m_flStartSize );
	CNetworkVarForDerived( float, m_flEndSize );
};

struct NetworkedEntityParticleTrailInfo_t : public EntityParticleTrailInfo_t, public INetworkableObject
{
	DECLARE_CLASS( NetworkedEntityParticleTrailInfo_t, EntityParticleTrailInfo_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flLifetime )
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flStartSize )
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flEndSize )
};


#endif // ENTITYPARTICLETRAIL_SHARED_H
