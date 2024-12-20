//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the Ammo at the Index passed in
//-----------------------------------------------------------------------------
Ammo_t *CAmmoDef::GetAmmoOfIndex(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return NULL;

	return &m_AmmoType[ (unsigned int)nAmmoIndex ];
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
const char* CAmmoDef::Name(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return NULL;

	return m_AmmoType[(unsigned int)nAmmoIndex].pName;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
AmmoIndex_t CAmmoDef::Index(const char *psz)
{
	unsigned int i;

	if (!psz)
		return AMMO_INVALID_INDEX;

	for (i = 0; i < m_nAmmoIndex; i++)
	{
		if (stricmp( psz, m_AmmoType[i].pName ) == 0)
			return (AmmoIndex_t)i;
	}

	return AMMO_INVALID_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::PlrDamage(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[(unsigned int)nAmmoIndex].pPlrDmg == USE_CVAR )
	{
		if ( m_AmmoType[(unsigned int)nAmmoIndex].pPlrDmgCVar )
		{
			return m_AmmoType[(unsigned int)nAmmoIndex].pPlrDmgCVar->GetFloat();
		}

		return 0;
	}
	else
	{
		return m_AmmoType[(unsigned int)nAmmoIndex].pPlrDmg;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::NPCDamage(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[(unsigned int)nAmmoIndex].pNPCDmg == USE_CVAR )
	{
		if ( m_AmmoType[(unsigned int)nAmmoIndex].pNPCDmgCVar )
		{
			return m_AmmoType[(unsigned int)nAmmoIndex].pNPCDmgCVar->GetFloat();
		}

		return 0;
	}
	else
	{
		return m_AmmoType[(unsigned int)nAmmoIndex].pNPCDmg;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MaxCarry(AmmoIndex_t nAmmoIndex, const CSharedBaseCombatCharacter *owner)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarry == USE_CVAR )
	{
		if ( m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarryCVar )
			return m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarryCVar->GetInt();

		return 0;
	}
	else
	{
		return m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarry;
	}
}

bool CAmmoDef::CanCarryInfiniteAmmo(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return false;

	int maxCarry = m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarry;
	if ( maxCarry == USE_CVAR )
	{
		if ( m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarryCVar )
		{
			maxCarry = m_AmmoType[(unsigned int)nAmmoIndex].pMaxCarryCVar->GetInt();
		}
	}
	return maxCarry == INFINITE_AMMO ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
DamageTypes_t	CAmmoDef::DamageType(AmmoIndex_t nAmmoIndex)
{
	if ((unsigned int)nAmmoIndex >= m_nAmmoIndex)
		return DMG_GENERIC;

	return m_AmmoType[(unsigned int)nAmmoIndex].nDamageType;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
AmmoFlags_t CAmmoDef::Flags(AmmoIndex_t nAmmoIndex)
{
	if ((unsigned int)nAmmoIndex >= m_nAmmoIndex)
		return AMMO_NONE;

	return m_AmmoType[(unsigned int)nAmmoIndex].nFlags;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MinSplashSize(AmmoIndex_t nAmmoIndex)
{
	if ((unsigned int)nAmmoIndex >= m_nAmmoIndex)
		return 4;

	return m_AmmoType[(unsigned int)nAmmoIndex].nMinSplashSize;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MaxSplashSize(AmmoIndex_t nAmmoIndex)
{
	if ((unsigned int)nAmmoIndex >= m_nAmmoIndex)
		return 8;

	return m_AmmoType[(unsigned int)nAmmoIndex].nMaxSplashSize;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
AmmoTracer_t	CAmmoDef::TracerType(AmmoIndex_t nAmmoIndex)
{
	if ((unsigned int)nAmmoIndex >= m_nAmmoIndex)
		return TRACER_NONE;

	return m_AmmoType[(unsigned int)nAmmoIndex].eTracerType;
}

float CAmmoDef::DamageForce(AmmoIndex_t nAmmoIndex)
{
	if ( (unsigned int)nAmmoIndex >= m_nAmmoIndex )
		return 0;

	return m_AmmoType[(unsigned int)nAmmoIndex].physicsForceImpulse;
}

//-----------------------------------------------------------------------------
// Purpose: Create an Ammo type with the name, decal, and tracer.
// Does not increment m_nAmmoIndex because the functions below do so and 
//  are the only entry point.
//-----------------------------------------------------------------------------
bool CAmmoDef::AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, AmmoFlags_t nFlags, int minSplashSize, int maxSplashSize )
{
	if (m_nAmmoIndex+1 == MAX_AMMO_TYPES)
		return false;

	++m_nAmmoIndex;

	int len = strlen(name);
	m_AmmoType[m_nAmmoIndex].pName = new char[len+1];
	Q_strncpy(m_AmmoType[m_nAmmoIndex].pName, name,len+1);
	m_AmmoType[m_nAmmoIndex].nDamageType	= damageType;
	m_AmmoType[m_nAmmoIndex].eTracerType	= tracerType;
	m_AmmoType[m_nAmmoIndex].nMinSplashSize	= minSplashSize;
	m_AmmoType[m_nAmmoIndex].nMaxSplashSize	= maxSplashSize;
	m_AmmoType[m_nAmmoIndex].nFlags	= nFlags;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add an ammo type with it's damage & carrying capability specified via cvars
//-----------------------------------------------------------------------------
void CAmmoDef::AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, 
	char const* plr_cvar, char const* npc_cvar, char const* carry_cvar, 
	float physicsForceImpulse, AmmoFlags_t nFlags, int minSplashSize, int maxSplashSize)
{
	if ( AddAmmoType( name, damageType, tracerType, nFlags, minSplashSize, maxSplashSize ) == false )
		return;

	if (plr_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pPlrDmgCVar	= g_pCVar->FindVarBase(plr_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pPlrDmgCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,plr_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pPlrDmg = USE_CVAR;
	}
	if (npc_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pNPCDmgCVar	= g_pCVar->FindVarBase(npc_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pNPCDmgCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,npc_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pNPCDmg = USE_CVAR;
	}
	if (carry_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pMaxCarryCVar= g_pCVar->FindVarBase(carry_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pMaxCarryCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,carry_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pMaxCarry = USE_CVAR;
	}
	m_AmmoType[m_nAmmoIndex].physicsForceImpulse = physicsForceImpulse;
}

//-----------------------------------------------------------------------------
// Purpose: Add an ammo type with it's damage & carrying capability specified via integers
//-----------------------------------------------------------------------------
void CAmmoDef::AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, 
	int plr_dmg, int npc_dmg, int carry, float physicsForceImpulse, 
	AmmoFlags_t nFlags, int minSplashSize, int maxSplashSize )
{
	if ( AddAmmoType( name, damageType, tracerType, nFlags, minSplashSize, maxSplashSize ) == false )
		return;

	m_AmmoType[m_nAmmoIndex].pPlrDmg = plr_dmg;
	m_AmmoType[m_nAmmoIndex].pNPCDmg = npc_dmg;
	m_AmmoType[m_nAmmoIndex].pMaxCarry = carry;
	m_AmmoType[m_nAmmoIndex].physicsForceImpulse = physicsForceImpulse;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAmmoDef::CAmmoDef(void)
{
	m_nAmmoIndex = -1;
	memset( m_AmmoType, 0, sizeof( m_AmmoType ) );
}

CAmmoDef::~CAmmoDef( void )
{
	for ( int i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		delete[] m_AmmoType[ i ].pName;
	}
}


