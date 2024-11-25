//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "entityoutput.h"
#include "convar.h"
#include "env_tonemap_controller.h"
#include "tier0/vprof.h"

#include "player.h"	//Tony; need player.h so we can trigger inputs on the player, from our inputs!

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static float s_hdr_tonemapscale = 1.0f;
static float s_hdr_manual_tonemap_rate = 1.0f;

#ifndef SWDS
extern ConVar *mat_hdr_tonemapscale;
extern ConVar *mat_hdr_manual_tonemap_rate;
#endif

// 0 - eyes fully closed / fully black
// 1 - nominal 
// 16 - eyes wide open / fully white

LINK_ENTITY_TO_CLASS( env_tonemap_controller, CEnvTonemapController );

BEGIN_MAPENTITY( CEnvTonemapController )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTonemapScale", InputSetTonemapScale ),
	DEFINE_INPUTFUNC( FIELD_STRING, "BlendTonemapScale", InputBlendTonemapScale ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTonemapRate", InputSetTonemapRate ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAutoExposureMin", InputSetAutoExposureMin ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAutoExposureMax", InputSetAutoExposureMax ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UseDefaultAutoExposure", InputUseDefaultAutoExposure ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UseDefaultBloomScale", InputUseDefaultBloomScale ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBloomScale", InputSetBloomScale ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBloomScaleRange", InputSetBloomScaleRange ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBloomExponent", InputSetBloomExponent ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBloomSaturation", InputSetBloomSaturation ),
END_MAPENTITY()

IMPLEMENT_SERVERCLASS_ST( CEnvTonemapController, DT_EnvTonemapController )
	SendPropInt( SENDINFO(m_bUseCustomAutoExposureMin), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bUseCustomAutoExposureMax), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bUseCustomBloomScale), 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flCustomAutoExposureMin), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flCustomAutoExposureMax), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flCustomBloomScale), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flCustomBloomScaleMinimum), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flBloomExponent), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flBloomSaturation), 0, SPROP_NOSCALE),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvTonemapController::CEnvTonemapController()
{
	m_flBloomExponent = 2.5f;
	m_flBloomSaturation = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEnvTonemapController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Set the tonemap scale to the specified value
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetTonemapScale( inputdata_t &inputdata )
{
	float flRemapped = inputdata.value.Float();

	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( ( gpGlobals->maxClients > 1 ) )
	{
		if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
		{
			//			DevMsg("activator is a player: InputSetTonemapScale\n");
			CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
			if ( pPlayer )
			{
				pPlayer->GetToneMapParams().m_flTonemapScale = flRemapped;
				return;
			}
		}
	}

	s_hdr_tonemapscale = flRemapped;

#ifndef SWDS
	if( !g_bTextMode )
		mat_hdr_tonemapscale->SetValue( flRemapped );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Blend the tonemap scale to the specified value
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputBlendTonemapScale( inputdata_t &inputdata )
{
	//Tony; TODO!!! -- tonemap scale blending does _not_ work properly in multiplayer..
	if ( gpGlobals->maxClients > 1 && ( inputdata.pActivator == NULL || !inputdata.pActivator->IsPlayer() ) )
		return;

	float tonemap_end_value, tonemap_end_time;
	int nargs = sscanf( inputdata.value.String(), "%f %f", &tonemap_end_value, &tonemap_end_time );
	if ( nargs != 2 )
	{
		Warning( "%s (%s) received BlendTonemapScale input without 2 arguments. Syntax: <target tonemap scale> <blend time>\n", GetClassname(), GetDebugName() );
		return;
	}

	if ( gpGlobals->maxClients == 1 )
	{
		m_flBlendTonemapEnd = tonemap_end_value;
		m_flBlendEndTime = gpGlobals->curtime + tonemap_end_time;
		m_flBlendStartTime = gpGlobals->curtime;

	#ifndef SWDS
		if( !g_bTextMode ) {
			m_flBlendTonemapStart = mat_hdr_tonemapscale->GetFloat();
		} else
	#endif
		{
			m_flBlendTonemapStart = s_hdr_tonemapscale;
		}
	}
	else
	{
		CBasePlayer* pl = ToBasePlayer( inputdata.pActivator );

		blend_t blend;
		blend.flBlendTonemapEnd = tonemap_end_value;
		blend.flBlendEndTime = gpGlobals->curtime + tonemap_end_time;
		blend.flBlendStartTime = gpGlobals->curtime;
		blend.flBlendTonemapStart = pl->GetToneMapParams().m_flTonemapScale;
		blend.hPlayer = pl;
		AUTO_LOCK( m_aTonemapBlends.Mutex_t );
		m_aTonemapBlends.AddToTail( blend );
	}

	// Start thinking
	SetNextThink( gpGlobals->curtime + 0.1 );
	if ( gpGlobals->maxClients > 1 )
		SetThink( &CEnvTonemapController::UpdateTonemapScaleBlendMultiplayer );
	else
		SetThink( &CEnvTonemapController::UpdateTonemapScaleBlend );
}

//-----------------------------------------------------------------------------
// Purpose: set a base and minimum bloom scale
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetBloomScaleRange( inputdata_t &inputdata )
{
	float bloom_max=1, bloom_min=1;
	int nargs=sscanf("%f %f",inputdata.value.String(), bloom_max, bloom_min );
	if (nargs != 2)
	{
		Warning("%s (%s) received SetBloomScaleRange input without 2 arguments. Syntax: <max bloom> <min bloom>\n", GetClassname(), GetDebugName() );
		return;
	}
	m_flCustomBloomScale=bloom_max;
	m_flCustomBloomScaleMinimum=bloom_min;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetTonemapRate( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( ( gpGlobals->maxClients > 1 ) )
	{
		if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
		{
			//			DevMsg("activator is a player: InputSetTonemapRate\n");
			CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
			if ( pPlayer )
			{
				pPlayer->GetToneMapParams().m_flTonemapRate = inputdata.value.Float();
				return;
			}
		}
	}

	// TODO: There should be a better way to do this.
	float flTonemapRate = inputdata.value.Float();

	s_hdr_manual_tonemap_rate = flTonemapRate;

#ifndef SWDS
	if( !g_bTextMode )
		mat_hdr_manual_tonemap_rate->SetValue( flTonemapRate );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Blend the tonemap scale to the specified value
//-----------------------------------------------------------------------------
void CEnvTonemapController::UpdateTonemapScaleBlend( void )
{ 
	float flRemapped = RemapValClamped( gpGlobals->curtime, m_flBlendStartTime, m_flBlendEndTime, m_flBlendTonemapStart, m_flBlendTonemapEnd );

	s_hdr_tonemapscale = flRemapped;

#ifndef SWDS
	if( !g_bTextMode )
		mat_hdr_tonemapscale->SetValue( flRemapped );
#endif

	//Msg("Setting tonemap scale to %f (curtime %f, %f -> %f)\n", flRemapped, gpGlobals->curtime, m_flBlendStartTime, m_flBlendEndTime ); 

	// Stop when we're out of the blend range
	if ( gpGlobals->curtime >= m_flBlendEndTime )
		return;

	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CEnvTonemapController::UpdateTonemapScaleBlendMultiplayer( void )
{
	VPROF_BUDGET( "CEnvTonemapController::UpdateTonemapScaleBlendMultiplayer", "Tonemap Controller" );
	CUtlVector<int> toDelete;
	{
		AUTO_LOCK( m_aTonemapBlends.Mutex_t );
		FOR_EACH_VEC( m_aTonemapBlends, i )
		{
			const blend_t& blend = m_aTonemapBlends[i];
			if ( !blend.hPlayer || gpGlobals->curtime > blend.flBlendEndTime )
			{
				toDelete.AddToTail( i );
				continue;
			}

			float flRemapped = RemapValClamped( gpGlobals->curtime, blend.flBlendStartTime, blend.flBlendEndTime, blend.flBlendTonemapStart, blend.flBlendTonemapEnd );
			blend.hPlayer->GetToneMapParams().m_flTonemapScale = flRemapped;
		}
	}

	if ( !toDelete.IsEmpty() )
	{
		AUTO_LOCK( m_aTonemapBlends.Mutex_t );
		for ( int i = toDelete.Count() - 1; i >= 0; --i )
		{
			m_aTonemapBlends.Remove( toDelete[i] );
		}
	}

	if ( m_aTonemapBlends.Size() > 0 )
		SetNextThink( gpGlobals->curtime + 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: Set the auto exposure min to the specified value
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetAutoExposureMin( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( ( gpGlobals->maxClients > 1 ) )
	{
		if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
		{
			//			DevMsg("activator is a player: InputSetAutoExposureMin\n");
			CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
			if ( pPlayer )
			{
				pPlayer->GetToneMapParams().m_flAutoExposureMin = inputdata.value.Float();
				return;
			}
		}
	}

	m_flCustomAutoExposureMin = inputdata.value.Float();
	m_bUseCustomAutoExposureMin = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set the auto exposure max to the specified value
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetAutoExposureMax( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( ( gpGlobals->maxClients > 1 ) )
	{
		if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
		{
			//			DevMsg("activator is a player: InputSetAutoExposureMax\n");
			CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
			if ( pPlayer )
			{
				pPlayer->GetToneMapParams().m_flAutoExposureMax = inputdata.value.Float();
				return;
			}
		}
	}

	m_flCustomAutoExposureMax = inputdata.value.Float();
	m_bUseCustomAutoExposureMax = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputUseDefaultAutoExposure( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( ( gpGlobals->maxClients > 1 ) )
	{
		if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
		{
			//			DevMsg("activator is a player: InputUseDefaultAutoExposure\n");
			CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
			if ( pPlayer )
			{
				pPlayer->GetToneMapParams().m_flAutoExposureMin = -1.f;
				pPlayer->GetToneMapParams().m_flAutoExposureMax = -1.f;
				pPlayer->GetToneMapParams().m_flTonemapRate = -1.f;
				return;
			}
		}
	}

	m_bUseCustomAutoExposureMin = false;
	m_bUseCustomAutoExposureMax = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetBloomScale( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
	{
		//			DevMsg("activator is a player: InputSetBloomScale\n");
		CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
		if ( pPlayer )
		{
			pPlayer->GetToneMapParams().m_flBloomScale = inputdata.value.Float();
			return;
		}
	}

	m_flCustomBloomScale = inputdata.value.Float();
	m_flCustomBloomScaleMinimum = m_flCustomBloomScale;
	m_bUseCustomBloomScale = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputUseDefaultBloomScale( inputdata_t &inputdata )
{
	//Tony; in multiplayer, we check to see if the activator is a player, if they are, we trigger an input on them, and then get out.
	//if there is no activator, or the activator is not a player; ie: LogicAuto, we set the 'global' values.
	if ( inputdata.pActivator != NULL && inputdata.pActivator->IsPlayer() )
	{
		//			DevMsg("activator is a player: InputUseDefaultBloomScale\n");
		CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
		if ( pPlayer )
		{
			pPlayer->GetToneMapParams().m_flBloomScale = -1.f;
			return;
		}
	}

	m_bUseCustomBloomScale = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetBloomExponent( inputdata_t &inputdata )
{
	m_flBloomExponent = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvTonemapController::InputSetBloomSaturation( inputdata_t &inputdata )
{
	m_flBloomSaturation = inputdata.value.Float();
}

//--------------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( trigger_tonemap, CTonemapTrigger );

BEGIN_MAPENTITY( CTonemapTrigger )
	DEFINE_KEYFIELD( m_tonemapControllerName,	FIELD_STRING,	"TonemapName" ),
END_MAPENTITY()


//--------------------------------------------------------------------------------------------------------
void CTonemapTrigger::Spawn( void )
{
	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );

	BaseClass::Spawn();
	InitTrigger();

	m_hTonemapController = gEntList.FindEntityByName( NULL, m_tonemapControllerName );
}


//--------------------------------------------------------------------------------------------------------
void CTonemapTrigger::StartTouch( CBaseEntity *other )
{
	if ( !PassesTriggerFilters( other ) )
		return;

	BaseClass::StartTouch( other );

	CBasePlayer *player = ToBasePlayer( other );
	if ( !player )
		return;

	player->OnTonemapTriggerStartTouch( this );
}


//--------------------------------------------------------------------------------------------------------
void CTonemapTrigger::EndTouch( CBaseEntity *other )
{
	if ( !PassesTriggerFilters( other ) )
		return;

	BaseClass::EndTouch( other );

	CBasePlayer *player = ToBasePlayer( other );
	if ( !player )
		return;

	player->OnTonemapTriggerEndTouch( this );
}


//-----------------------------------------------------------------------------
// Purpose: Clear out the tonemap controller.
//-----------------------------------------------------------------------------
void CTonemapSystem::LevelInitPreEntity( void )
{
	m_hMasterController = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: On level load find the master fog controller.  If no controller is
//			set as Master, use the first fog controller found.
//-----------------------------------------------------------------------------
void CTonemapSystem::LevelInitPostEntity( void )
{
	// Overall master controller
	CEnvTonemapController *pTonemapController = NULL;
	do
	{
		pTonemapController = static_cast<CEnvTonemapController*>( gEntList.FindEntityByClassname( pTonemapController, "env_tonemap_controller" ) );
		if ( pTonemapController )
		{
			if ( m_hMasterController == NULL )
			{
				m_hMasterController = pTonemapController;
			}
			else
			{
				if ( pTonemapController->IsMaster() )
				{
					m_hMasterController = pTonemapController;
				}
			}
		}
	} while ( pTonemapController );
}

//--------------------------------------------------------------------------------------------------------
CTonemapSystem s_TonemapSystem( "TonemapSystem" );


//--------------------------------------------------------------------------------------------------------
CTonemapSystem *TheTonemapSystem( void )
{
	return &s_TonemapSystem;
}
