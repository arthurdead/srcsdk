//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFunc_LOD : public CBaseEntity
{
	DECLARE_MAPENTITY();
	DECLARE_CLASS( CFunc_LOD, CBaseEntity );
public:
	DECLARE_SERVERCLASS();

					CFunc_LOD();
	virtual 		~CFunc_LOD();


	// When the viewer is between:
	// (0 and m_fNonintrusiveDist):					the bmodel is forced to be visible
	// (m_fNonintrusiveDist and m_fDisappearDist):	the bmodel is trying to appear or disappear nonintrusively
	//												(waits until it's out of the view frustrum or until there's a lot of motion)
	// (m_fDisappearDist+):							the bmodel is forced to be invisible
	CNetworkVar( float, m_flDisappearMinDist );
	CNetworkVar( float, m_flDisappearMaxDist );

// CBaseEntity overrides.
public:

	virtual void	Spawn();
	bool			CreateVPhysics();
	virtual void	Activate();
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
};


IMPLEMENT_SERVERCLASS_ST(CFunc_LOD, DT_Func_LOD)
	SendPropFloat(SENDINFO(m_flDisappearMinDist), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flDisappearMaxDist), 0, SPROP_NOSCALE),
END_SEND_TABLE()


LINK_ENTITY_TO_CLASS(func_lod, CFunc_LOD);

BEGIN_MAPENTITY( CFunc_LOD )

	DEFINE_KEYFIELD( m_flDisappearMinDist,	FIELD_FLOAT, "DisappearMinDist" ),
	DEFINE_KEYFIELD( m_flDisappearMaxDist,	FIELD_FLOAT, "DisappearMaxDist" ),

END_MAPENTITY()

// ------------------------------------------------------------------------------------- //
// CFunc_LOD implementation.
// ------------------------------------------------------------------------------------- //
CFunc_LOD::CFunc_LOD()
{
}


CFunc_LOD::~CFunc_LOD()
{
}


void CFunc_LOD::Spawn()
{
	// Bind to our bmodel.
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	BaseClass::Spawn();

	CreateVPhysics();
}

bool CFunc_LOD::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

void CFunc_LOD::Activate()
{
	BaseClass::Activate();
}


bool CFunc_LOD::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "DisappearDist"))
	{
		m_flDisappearMinDist = (float)atof(szValue);
		m_flDisappearMaxDist = m_flDisappearMinDist + 800;
	}
	else if (FStrEq(szKeyName, "DisappearMaxDist"))
	{
		m_flDisappearMaxDist = (float)atof(szValue);
	}
	else if (FStrEq(szKeyName, "DisappearMinDist")) // Forwards compatibility
	{
		m_flDisappearMinDist = (float)atof(szValue);
	}
	else if (FStrEq(szKeyName, "Solid"))
	{
		if (atoi(szValue) != 0)
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
		}
	}
	else
	{
		return BaseClass::KeyValue(szKeyName, szValue);
	}

	return true;
}
			  
