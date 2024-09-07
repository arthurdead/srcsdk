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
INIT_PRIORITY(101) static CClientEffectPrecacheSystem	s_ClientEffectPrecacheSystem;
CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem( void )
{
	return &s_ClientEffectPrecacheSystem;
}

bool CClientEffectPrecacheSystem::Init()
{
	for ( int i = 0; i < m_Effects.Count(); i++ )
	{
		m_Effects[i]->Init();
	}

	return true;
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

CClientEffect::CClientEffect( const char *pName, bool (*pCondFunc)() )
{
	m_bPrecached = false;
	m_pCondFunc = pCondFunc;

	//Register with the main effect system
	ClientEffectPrecacheSystem()->Register( this, pName );
}

CClientEffect::CClientEffect( const char *pName )
	: CClientEffect(pName, NULL)
{
}

CClientEffect::~CClientEffect()
{
}

void CClientEffect::Init()
{
	m_Materials.SetCount( m_MaterialsNames.Count() );
	Precache();
}

void CClientEffect::AddMaterial( const char *materialName )
{
	m_MaterialsNames.AddToTail( materialName );
}

void CClientEffect::Release()
{
	Shutdown();
	m_Materials.Purge();
}

void CClientEffect::Precache()
{
	if(m_bPrecached)
		return;

	for(int i = 0; i < m_MaterialsNames.Count(); ++i) {
		IMaterial	*material = NULL;

		if(!m_pCondFunc || m_pCondFunc()) {
			material = materials->FindMaterial( m_MaterialsNames[i].Get(), TEXTURE_GROUP_CLIENT_EFFECTS );
		}

		if ( !IsErrorMaterial( material ) )
		{
			m_Materials[i] = material;
			m_Materials[i]->IncrementReferenceCount();
		}
		else
		{
			m_Materials[i] = NULL;
		}
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
			m_Materials[i]->DeleteIfUnreferenced();
			m_Materials[i] = NULL;
		}
	}
}