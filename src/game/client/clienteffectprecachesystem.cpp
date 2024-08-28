//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with precaching requests from client effects
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "clienteffectprecachesystem.h"
#include "particles/particles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Global singelton accessor
static CClientEffectPrecacheSystem	s_ClientEffectPrecacheSystem;
CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem( void )
{
	return &s_ClientEffectPrecacheSystem;
}

//-----------------------------------------------------------------------------
// Purpose: Precache all the registered effects
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelInitPreEntity( void )
{
	//Precache all known effects
	for ( int i = 0; i < m_Effects.Count(); i++ )
	{
		m_Effects[i]->Precache();
	}
	
	//FIXME: Double check this
	//Finally, force the cache of these materials
	materials->CacheUsedMaterials();

	// Now, cache off our material handles
	FX_CacheMaterialHandles();
}

//-----------------------------------------------------------------------------
// Purpose: Nothing to do here
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelShutdownPreEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Dereference all the registered effects
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelShutdownPostEntity( void )
{
	// mark all known effects as free
	for ( int i = 0; i < m_Effects.Count(); i++ )
	{
		m_Effects[i]->Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Purges the effect list
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::Shutdown( void )
{
	// mark all known effects as free
	for ( int i = 0; i < m_Effects.Count(); i++ )
	{
		m_Effects[i]->Shutdown();
		m_Effects[i]->Release();
	}

	//Release all effects
	m_Effects.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Adds the effect to the list to be precached
// Input  : *effect - system to precache
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::Register( IClientEffect *effect, const char *pName )
{
	//Hold onto this effect for precaching later
	m_Effects.Insert( pName, effect );
}

IClientEffect *CClientEffectPrecacheSystem::Find( const char *pName )
{
	int i = m_Effects.Find( pName );
	if(i == m_Effects.InvalidIndex())
		return NULL;

	return m_Effects[i];
}

CClientEffect::CClientEffect( const char *pName )
{
	m_bPrecached = false;

	//Register with the main effect system
	ClientEffectPrecacheSystem()->Register( this, pName );
}

CClientEffect::~CClientEffect()
{
	for(int i = 0; i < m_Materials.Count(); ++i)
	{
		if(m_Materials[i]) {
			m_Materials[i]->DeleteIfUnreferenced();
		}
	}
}

void CClientEffect::AddMaterial( const char *materialName )
{
	IMaterial	*material = materials->FindMaterial( materialName, TEXTURE_GROUP_CLIENT_EFFECTS );
	if ( !IsErrorMaterial( material ) )
	{
		m_Materials.AddToTail( material );
	}
}

void CClientEffect::Release()
{
	for(int i = 0; i < m_Materials.Count(); ++i)
	{
		if(m_Materials[i]) {
			m_Materials[i]->DeleteIfUnreferenced();
		}
	}
}

void CClientEffect::Precache()
{
	if(m_bPrecached)
		return;

	for(int i = 0; i < m_Materials.Count(); ++i)
	{
		if(m_Materials[i])
			m_Materials[i]->IncrementReferenceCount();
	}

	m_bPrecached = true;
}

void CClientEffect::Shutdown()
{
	if(!m_bPrecached)
		return;

	for(int i = 0; i < m_Materials.Count(); ++i)
	{
		if(m_Materials[i]) {
			m_Materials[i]->DecrementReferenceCount();
		}
	}

	m_bPrecached = false;
}