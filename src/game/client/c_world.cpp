//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_world.h"
#include "ivmodemanager.h"
#include "activitylist.h"
#include "decals.h"
#include "engine/ivmodelinfo.h"
#include "ivieweffects.h"
#include "shake.h"
#include "eventlist.h"
#include "mapentities_shared.h"
#include "clientalphaproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CWorld
#undef CWorld
#endif

C_World *g_pClientWorld = NULL;

modelindex_t g_sModelIndexWorld = INVALID_MODEL_INDEX;

LINK_ENTITY_TO_SERVERCLASS( worldspawn, CWorld );

IMPLEMENT_CLIENTCLASS( C_World, DT_World, CWorld );

BEGIN_RECV_TABLE( C_World, DT_World )
	RecvPropFloat(RECVINFO(m_flWaveHeight)),
	RecvPropVector(RECVINFO(m_WorldMins)),
	RecvPropVector(RECVINFO(m_WorldMaxs)),
	RecvPropInt(RECVINFO(m_bStartDark)),
	RecvPropFloat(RECVINFO(m_flMaxOccludeeArea)),
	RecvPropFloat(RECVINFO(m_flMinOccluderArea)),
	RecvPropFloat(RECVINFO(m_flMaxPropScreenSpaceWidth)),
	RecvPropFloat(RECVINFO(m_flMinPropScreenSpaceWidth)),
	RecvPropString(RECVINFO(m_iszDetailSpriteMaterial)),
	RecvPropInt(RECVINFO(m_bColdWorld)),
	RecvPropInt(RECVINFO(m_iTimeOfDay)),
	RecvPropString(RECVINFO(m_iszChapterTitle)),
END_RECV_TABLE()


C_World::C_World( void )
{
	if(!g_pClientWorld) {
		g_pClientWorld = this;
		AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	m_flWaveHeight = 0.0f;
}

C_World::~C_World( void )
{
	if(g_pClientWorld == this) {
		g_pClientWorld = NULL;
	}
}

//TODO!!! Arthurdead: make the world not networked

void C_World::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
}

bool C_World::PostConstructor( const char *szClassname )
{
	if(!BaseClass::PostConstructor( szClassname ))
		return false;

	return true;
}

void C_World::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}

void C_World::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
}

void C_World::Precache( void )
{
}

extern void OnWorldSpawned();

void C_World::Spawn( void )
{
	if(g_pClientWorld && g_pClientWorld != this) {
		UTIL_Remove(this);
		return;
	}

	OnWorldSpawned();

	Precache();

	if ( m_bStartDark )
	{
		ScreenFade_t sf;
		memset( (void *)&sf, 0, sizeof( sf ) );
		sf.color.SetColor( 0, 0, 0, 255 );
		sf.duration = (float)(1<<SCREENFADE_FRACBITS) * 5.0f;
		sf.holdTime = (float)(1<<SCREENFADE_FRACBITS) * 1.0f;
		sf.fadeFlags = FFADE_IN | FFADE_PURGE;
		GetViewEffects()->Fade( sf );
	}

	OcclusionParams_t params;
	params.m_flMaxOccludeeArea = m_flMaxOccludeeArea;
	params.m_flMinOccluderArea = m_flMinOccluderArea;
	engine->SetOcclusionParameters( params );

	modelinfo->SetLevelScreenFadeRange( m_flMinPropScreenSpaceWidth, m_flMaxPropScreenSpaceWidth );
}

//-----------------------------------------------------------------------------
// Parse data from a map file
//-----------------------------------------------------------------------------
bool C_World::KeyValue( const char *szKeyName, const char *szValue ) 
{
	return BaseClass::KeyValue( szKeyName, szValue );
}

C_World *GetClientWorldEntity()
{
	Assert( g_pClientWorld != NULL );
	return g_pClientWorld;
}

