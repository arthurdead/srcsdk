//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particle_light.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_particlelight, CParticleLight );


//Save/restore
BEGIN_MAPENTITY( CParticleLight )

	//Keyvalue fields
	DEFINE_KEYFIELD_AUTO( m_flIntensity, "Intensity" ),
	DEFINE_KEYFIELD_AUTO( m_vColor, "Color" ),
	DEFINE_KEYFIELD_AUTO( m_PSName, "PSName" ),
	DEFINE_KEYFIELD( m_bDirectional,	FIELD_BOOLEAN,	"Directional" )

END_MAPENTITY()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
CParticleLight::CParticleLight()
{
	m_flIntensity = 5000;
	m_vColor.Init( 1, 0, 0 );
	m_PSName = NULL_STRING;
	m_bDirectional = false;
}


