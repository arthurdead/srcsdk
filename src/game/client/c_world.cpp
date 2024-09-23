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

LINK_ENTITY_TO_CLASS(worldspawn, C_World);

static IClientNetworkable* ClientWorldFactory( int entnum, int serialNum )
{
	Assert( g_pClientWorld != NULL );
	Assert( entnum == 0 );

	if(!g_pClientWorld) {
		g_pClientWorld = (C_World *)CreateEntityByName("worldspawn");
		g_pClientWorld->SetLocalOrigin( vec3_origin );
		g_pClientWorld->SetLocalAngles( vec3_angle );
		g_pClientWorld->ParseWorldMapData( engine->GetMapEntitiesString() );
	}

	if(!g_pClientWorld->InitializeAsServerEntity( 0, serialNum )) {
		UTIL_Remove(g_pClientWorld);
		g_pClientWorld = NULL;
		return NULL;
	}

	return g_pClientWorld;
}


IMPLEMENT_CLIENTCLASS_FACTORY( C_World, DT_World, CWorld, ClientWorldFactory );

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
	Assert( !g_pClientWorld );

	g_pClientWorld = this;

	m_flWaveHeight = 0.0f;

	m_nEntIndex = 0;

	//cl_entitylist->AddNetworkableEntity( this, 0 );

	m_nCreationTick = gpGlobals->tickcount;
}

C_World::~C_World( void )
{
	Assert( g_pClientWorld == this );

	g_pClientWorld = NULL;
}

bool C_World::InitializeAsServerEntity( int entnum, int iSerialNum )
{
	Assert( entnum == 0 );

	//cl_entitylist->ForceEntSerialNumber( 0, iSerialNum );

	//m_RefEHandle.Init( 0, iSerialNum );

	cl_entitylist->AddNetworkableEntity( this, 0, iSerialNum );

	return true;
}

void C_World::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
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

void C_World::Spawn( void )
{
	if(g_pClientWorld && g_pClientWorld != this) {
		UTIL_Remove(this);
		return;
	}

	Precache();

	if ( m_bStartDark )
	{
		ScreenFade_t sf;
		memset( &sf, 0, sizeof( sf ) );
		sf.a = 255;
		sf.r = 0;
		sf.g = 0;
		sf.b = 0;
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

//-----------------------------------------------------------------------------
// Parses worldspawn data from BSP on the client
//-----------------------------------------------------------------------------
void C_World::ParseWorldMapData( const char *pMapData )
{
	char szTokenBuffer[MAPKEY_MAXLENGTH];
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Error( "MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		CEntityMapData entData( (char*)pMapData );
		char className[MAPKEY_MAXLENGTH];

		if (!entData.ExtractValue( "classname", className ))
		{
			Error( "classname missing from entity!\n" );
		}

		if ( !Q_strcmp( className, "worldspawn" ) )
		{
			// Set up keyvalues.
			ParseMapData( &entData );
			return;
		}
	}
}

C_World *GetClientWorldEntity()
{
	Assert( g_pClientWorld != NULL );
	return g_pClientWorld;
}

