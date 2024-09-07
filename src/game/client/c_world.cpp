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
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CWorld
#undef CWorld
#endif

static C_World *g_pClientWorld;

LINK_ENTITY_TO_CLASS(worldspawn, C_World);

void ClientWorldFactoryInit()
{
	g_pClientWorld = CREATE_ENTITY(C_World, "worldspawn");
}

void ClientWorldFactoryShutdown()
{
	UTIL_Remove( g_pClientWorld );
	g_pClientWorld = NULL;
}

static IClientNetworkable* ClientWorldFactory( int entnum, int serialNum )
{
	Assert( g_pClientWorld != NULL );

	if(!g_pClientWorld->InitializeAsServerEntity( entnum, serialNum ))
		return NULL;

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
END_RECV_TABLE()


C_World::C_World( void )
{
}

C_World::~C_World( void )
{
}

bool C_World::InitializeAsServerEntity( int entnum, int iSerialNum )
{
	m_flWaveHeight = 0.0f;
	ActivityList_Init();
	EventList_Init();

	return BaseClass::InitializeAsServerEntity( entnum, iSerialNum );
}

void C_World::UpdateOnRemove()
{
	ActivityList_Free();
	BaseClass::UpdateOnRemove();
}

void C_World::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
}

void C_World::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Always force reset to normal mode upon receipt of world in new map
	if ( updateType == DATA_UPDATE_CREATED )
	{
		modemanager->SwitchMode( CLIENTMODE_NORMAL, true );

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
}

void C_World::RegisterSharedActivities( void )
{
	ActivityList_RegisterSharedActivities();
	EventList_RegisterSharedEvents();
}

// -----------------------------------------
//	Sprite Index info
// -----------------------------------------
int		g_sModelIndexLaser;			// holds the index for the laser beam
int		g_sModelIndexLaserDot;		// holds the index for the laser beam dot
int		g_sModelIndexFireball;		// holds the index for the fireball
int		g_sModelIndexSmoke;			// holds the index for the smoke cloud
int		g_sModelIndexWExplosion;	// holds the index for the underwater explosion
int		g_sModelIndexBubbles;		// holds the index for the bubbles model
int		g_sModelIndexBloodDrop;		// holds the sprite index for the initial blood
int		g_sModelIndexBloodSpray;	// holds the sprite index for splattered blood

//-----------------------------------------------------------------------------
// Purpose: Precache global weapon sounds
//-----------------------------------------------------------------------------
void W_Precache(void)
{
	PrecacheFileWeaponInfoDatabase( filesystem, GameRules()->GetEncryptionKey() );

	g_sModelIndexFireball = modelinfo->GetModelIndex ("sprites/zerogxplode.vmt");// fireball
	g_sModelIndexWExplosion = modelinfo->GetModelIndex ("sprites/WXplo1.vmt");// underwater fireball
	g_sModelIndexSmoke = modelinfo->GetModelIndex ("sprites/steam1.vmt");// smoke
	g_sModelIndexBubbles = modelinfo->GetModelIndex ("sprites/bubble.vmt");//bubbles
	g_sModelIndexBloodSpray = modelinfo->GetModelIndex ("sprites/bloodspray.vmt"); // initial blood
	g_sModelIndexBloodDrop = modelinfo->GetModelIndex ("sprites/blood.vmt"); // splattered blood 
	g_sModelIndexLaser = modelinfo->GetModelIndex( "sprites/laserbeam.vmt" );
	g_sModelIndexLaserDot = modelinfo->GetModelIndex("sprites/laserdot.vmt");
}

void C_World::Precache( void )
{
	// UNDONE: Make most of these things server systems or precache_registers
	// =================================================
	//	Activities
	// =================================================
	ActivityList_Free();
	EventList_Free();

	RegisterSharedActivities();

	// Get weapon precaches
	W_Precache();	

	// Call all registered precachers.
	CPrecacheRegister::Precache();
}

void C_World::Spawn( void )
{
	Precache();
}



C_World *GetClientWorldEntity()
{
	Assert( g_pClientWorld != NULL );
	return g_pClientWorld;
}

