//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "clientalphaproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_Func_LOD : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_Func_LOD, C_BaseEntity );
	DECLARE_CLIENTCLASS();

					C_Func_LOD();

// C_BaseEntity overrides.
public:

	virtual void	OnDataChanged( DataUpdateType_t type );

public:
// Replicated vars from the server.
// These are documented in the server-side entity.
public:
	float m_flDisappearMinDist;
	float m_flDisappearMaxDist;
};


ConVar lod_TransitionDist("lod_TransitionDist", "800");


// ------------------------------------------------------------------------- //
// Tables.
// ------------------------------------------------------------------------- //

// Datatable..
IMPLEMENT_CLIENTCLASS_DT(C_Func_LOD, DT_Func_LOD, CFunc_LOD)
	RecvPropFloat(RECVINFO(m_flDisappearMinDist)),
	RecvPropFloat(RECVINFO(m_flDisappearMaxDist)),
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_Func_LOD implementation.
// ------------------------------------------------------------------------- //

C_Func_LOD::C_Func_LOD()
{
	m_flDisappearMinDist = 5000.0f;
	m_flDisappearMaxDist = m_flDisappearMinDist + lod_TransitionDist.GetFloat();
}

void C_Func_LOD::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	bool bCreate = (type == DATA_UPDATE_CREATED) ? true : false;
	VPhysicsShadowDataChanged(bCreate, this);

	// Copy in fade parameters
	SetDistanceFade( m_flDisappearMinDist, m_flDisappearMaxDist );
	SetGlobalFadeScale( 1.0f );
}
