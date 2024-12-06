//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "entitylist.h"
#include "SkyCamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// automatically hooks in the system's callbacks
CHandle<CSkyCamera> g_hActiveSkybox = NULL;

CEntityClassList<CSkyCamera> g_SkyList;
template <> CSkyCamera *CEntityClassList<CSkyCamera>::m_pClassList = NULL;

//-----------------------------------------------------------------------------
// Retrives the current skycamera
//-----------------------------------------------------------------------------
CSkyCamera*	GetCurrentSkyCamera()
{
	if (g_hActiveSkybox.Get() == NULL)
	{
		g_hActiveSkybox = GetSkyCameraList();
	}
	return g_hActiveSkybox.Get();
}

CSkyCamera*	GetSkyCameraList()
{
	return g_SkyList.m_pClassList;
}

//=============================================================================

LINK_ENTITY_TO_CLASS( sky_camera, CSkyCamera );

BEGIN_MAPENTITY( CSkyCamera )

	DEFINE_KEYFIELD( m_skyboxData.scale, FIELD_INTEGER, "scale" ),

	// fog data for 3d skybox
	DEFINE_KEYFIELD( m_bUseAngles,						FIELD_BOOLEAN,	"use_angles" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.enable,			FIELD_BOOLEAN, "fogenable" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.blend,			FIELD_BOOLEAN, "fogblend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.dirPrimary,		FIELD_VECTOR, "fogdir" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorPrimary,		FIELD_COLOR32, "fogcolor" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorSecondary,	FIELD_COLOR32, "fogcolor2" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.start,			FIELD_FLOAT, "fogstart" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.end,				FIELD_FLOAT, "fogend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.maxdensity,		FIELD_FLOAT, "fogmaxdensity" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.HDRColorScale,	FIELD_FLOAT, "HDRColorScale" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.farz, FIELD_FLOAT, "farz" ),
	DEFINE_KEYFIELD( m_skyboxData.skycolor, FIELD_COLOR32, "skycolor" ),
	DEFINE_KEYFIELD( m_bUseAnglesForSky, FIELD_BOOLEAN, "use_angles_for_sky" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ForceUpdate", InputForceUpdate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartUpdating", InputStartUpdating ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopUpdating", InputStopUpdating ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ActivateSkybox", InputActivateSkybox ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DeactivateSkybox", InputDeactivateSkybox ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFogStartDist", InputSetFogStartDist ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFogEndDist", InputSetFogEndDist ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFogMaxDensity", InputSetFogMaxDensity ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOnFog", InputTurnOnFog ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOffFog", InputTurnOffFog ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetFogColor", InputSetFogColor ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetFogColorSecondary", InputSetFogColorSecondary ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "CopyFogController", InputCopyFogController ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "CopyFogControllerWithScale", InputCopyFogControllerWithScale ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFarZ", InputSetFarZ ),

	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetSkyColor", InputSetSkyColor ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetScale", InputSetScale ),

END_MAPENTITY()

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CSkyCamera::CSkyCamera()
{
	g_SkyList.Insert( this );
	m_skyboxData.fog.maxdensity = 1.0f;
	m_skyboxData.fog.HDRColorScale = 1.0f;
	m_skyboxData.skycolor.Init(0, 0, 0, 0);
}

CSkyCamera::~CSkyCamera()
{
	g_SkyList.Remove( this );
}

void CSkyCamera::Spawn( void ) 
{ 
	if (HasSpawnFlags(SF_SKY_MASTER))
		g_hActiveSkybox = this;

	if (HasSpawnFlags(SF_SKY_START_UPDATING))
	{
		SetCameraEntityMode();

		SetThink( &CSkyCamera::UpdateThink );
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}
	else
	{
		SetCameraPositionMode();
	}
	m_skyboxData.area = engine->GetArea( m_skyboxData.origin );
	
	Precache();
}


//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CSkyCamera::Activate( ) 
{
	BaseClass::Activate();

	if ( m_bUseAngles )
	{
		AngleVectors( GetAbsAngles(), &m_skyboxData.fog.dirPrimary.GetForModify() );
		m_skyboxData.fog.dirPrimary.GetForModify() *= -1.0f; 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSkyCamera::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
	if (!BaseClass::AcceptInput( szInputName, pActivator, pCaller, Value, outputID ))
		return false;

	if (g_hActiveSkybox == this)
	{
		// Most inputs require an update
		DoUpdate( true );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSkyCamera::SetCameraEntityMode()
{
	m_skyboxData.skycamera = this;

	// Ensure the viewrender knows whether this should be using angles
	if (m_bUseAnglesForSky)
		m_skyboxData.angles.SetX( 1 );
	else
		m_skyboxData.angles.SetX( 0 );
}

void CSkyCamera::SetCameraPositionMode()
{
	// Must be absolute now that the sky_camera can be parented
	m_skyboxData.skycamera = NULL;
	m_skyboxData.origin = GetAbsOrigin();
	if (m_bUseAnglesForSky)
		m_skyboxData.angles = GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Update sky position mid-game
//-----------------------------------------------------------------------------
bool CSkyCamera::DoUpdate( bool bUpdateData )
{
	// Now that sky camera updating uses an entity handle directly transmitted to the client,
	// this thinking is only used to update area and other parameters

	// Getting into another area is unlikely, but if it's not expensive, I guess it's okay.
	int area = engine->GetArea( GetAbsOrigin() );
	if (m_skyboxData.area != area)
	{
		m_skyboxData.area = area;
		bUpdateData = true;
	}

	if ( m_bUseAngles )
	{
		Vector fogForward;
		AngleVectors( GetAbsAngles(), &fogForward );
		fogForward *= -1.0f;

		if ( m_skyboxData.fog.dirPrimary.Get() != fogForward )
		{
			m_skyboxData.fog.dirPrimary = fogForward;
			bUpdateData = true;
		}
	}

	if (bUpdateData)
	{
		// Updates client data, this completely ignores m_pOldSkyCamera
		CBasePlayer *pPlayer = NULL;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = UTIL_PlayerByIndex(i);
			if (pPlayer)
				pPlayer->m_Local.m_skybox3d.GetForModify() = m_skyboxData;
		}
	}

	// Needed for entity interpolation
	SetSimulationTime( gpGlobals->curtime );

	return bUpdateData;
}

void CSkyCamera::UpdateThink()
{
	if (DoUpdate())
	{
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.2f );
	}
}

void CSkyCamera::InputForceUpdate( inputdata_t &inputdata )
{
	if (m_skyboxData.skycamera == NULL)
	{
		m_skyboxData.origin = GetAbsOrigin();
		if (m_bUseAnglesForSky)
			m_skyboxData.angles = GetAbsAngles();
	}

	DoUpdate( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSkyCamera::InputStartUpdating( inputdata_t &inputdata )
{
	if (GetCurrentSkyCamera() == this)
	{
		SetCameraEntityMode();
		DoUpdate( true );

		SetThink( &CSkyCamera::UpdateThink );
		SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	}

	// If we become the current sky camera later, remember that we want to update
	AddSpawnFlags( SF_SKY_START_UPDATING );

	// Must update transmit state so we show up on the client
	DispatchUpdateTransmitState();
}

void CSkyCamera::InputStopUpdating( inputdata_t &inputdata )
{
	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
	RemoveSpawnFlags( SF_SKY_START_UPDATING );
	DispatchUpdateTransmitState();

	SetCameraPositionMode();
	DoUpdate( true );
}

//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CSkyCamera::InputActivateSkybox( inputdata_t &inputdata )
{
	CSkyCamera *pActiveSky = GetCurrentSkyCamera();
	if (pActiveSky && pActiveSky->GetNextThink() != TICK_NEVER_THINK && pActiveSky != this)
	{
		// Deactivate that skybox
		pActiveSky->SetThink( NULL );
		pActiveSky->SetNextThink( TICK_NEVER_THINK );
	}

	g_hActiveSkybox = this;

	if (HasSpawnFlags( SF_SKY_START_UPDATING ))
		InputStartUpdating( inputdata );
}

//-----------------------------------------------------------------------------
// Deactivate!
//-----------------------------------------------------------------------------
void CSkyCamera::InputDeactivateSkybox( inputdata_t &inputdata )
{
	if (GetCurrentSkyCamera() == this)
	{
		g_hActiveSkybox = NULL;

		// ClientData doesn't catch this immediately
		CBasePlayer *pPlayer = NULL;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			pPlayer = UTIL_PlayerByIndex( i );
			if (pPlayer)
				pPlayer->m_Local.m_skybox3d.area = 255;
		}
	}

	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
}

//------------------------------------------------------------------------------
// Purpose: Input handlers for setting fog stuff.
//------------------------------------------------------------------------------
void CSkyCamera::InputSetFogStartDist( inputdata_t &inputdata ) { m_skyboxData.fog.start = inputdata.value.Float(); }
void CSkyCamera::InputSetFogEndDist( inputdata_t &inputdata ) { m_skyboxData.fog.end = inputdata.value.Float(); }
void CSkyCamera::InputSetFogMaxDensity( inputdata_t &inputdata ) { m_skyboxData.fog.maxdensity = inputdata.value.Float(); }
void CSkyCamera::InputTurnOnFog( inputdata_t &inputdata ) { m_skyboxData.fog.enable = true; }
void CSkyCamera::InputTurnOffFog( inputdata_t &inputdata ) { m_skyboxData.fog.enable = false; }
void CSkyCamera::InputSetFogColor( inputdata_t &inputdata ) { m_skyboxData.fog.colorPrimary = inputdata.value.Color32(); }
void CSkyCamera::InputSetFogColorSecondary( inputdata_t &inputdata ) { m_skyboxData.fog.colorSecondary = inputdata.value.Color32(); }

void CSkyCamera::InputSetFarZ( inputdata_t &inputdata ) { m_skyboxData.fog.farz = inputdata.value.Int(); }

void CSkyCamera::InputCopyFogController( inputdata_t &inputdata )
{
	CFogController *pFogController = dynamic_cast<CFogController*>(inputdata.value.Entity().Get());
	if (!pFogController)
		return;

	m_skyboxData.fog.dirPrimary = pFogController->m_fog.dirPrimary;
	m_skyboxData.fog.colorPrimary = pFogController->m_fog.colorPrimary;
	m_skyboxData.fog.colorSecondary = pFogController->m_fog.colorSecondary;
	//m_skyboxData.fog.colorPrimaryLerpTo = pFogController->m_fog.colorPrimaryLerpTo;
	//m_skyboxData.fog.colorSecondaryLerpTo = pFogController->m_fog.colorSecondaryLerpTo;
	m_skyboxData.fog.start = pFogController->m_fog.start;
	m_skyboxData.fog.end = pFogController->m_fog.end;
	m_skyboxData.fog.farz = pFogController->m_fog.farz;
	m_skyboxData.fog.maxdensity = pFogController->m_fog.maxdensity;

	//m_skyboxData.fog.startLerpTo = pFogController->m_fog.startLerpTo;
	//m_skyboxData.fog.endLerpTo = pFogController->m_fog.endLerpTo;
	//m_skyboxData.fog.lerptime = pFogController->m_fog.lerptime;
	//m_skyboxData.fog.duration = pFogController->m_fog.duration;
	//m_skyboxData.fog.enable = pFogController->m_fog.enable;
	m_skyboxData.fog.blend = pFogController->m_fog.blend;
}

void CSkyCamera::InputCopyFogControllerWithScale( inputdata_t &inputdata )
{
	CFogController *pFogController = dynamic_cast<CFogController*>(inputdata.value.Entity().Get());
	if (!pFogController)
		return;

	m_skyboxData.fog.dirPrimary = pFogController->m_fog.dirPrimary;
	m_skyboxData.fog.colorPrimary = pFogController->m_fog.colorPrimary;
	m_skyboxData.fog.colorSecondary = pFogController->m_fog.colorSecondary;
	//m_skyboxData.fog.colorPrimaryLerpTo = pFogController->m_fog.colorPrimaryLerpTo;
	//m_skyboxData.fog.colorSecondaryLerpTo = pFogController->m_fog.colorSecondaryLerpTo;
	m_skyboxData.fog.start = pFogController->m_fog.start * m_skyboxData.scale;
	m_skyboxData.fog.end = pFogController->m_fog.end * m_skyboxData.scale;
	m_skyboxData.fog.farz = pFogController->m_fog.farz != -1 ? (pFogController->m_fog.farz * m_skyboxData.scale) : pFogController->m_fog.farz;
	m_skyboxData.fog.maxdensity = pFogController->m_fog.maxdensity;

	//m_skyboxData.fog.startLerpTo = pFogController->m_fog.startLerpTo;
	//m_skyboxData.fog.endLerpTo = pFogController->m_fog.endLerpTo;
	//m_skyboxData.fog.lerptime = pFogController->m_fog.lerptime;
	//m_skyboxData.fog.duration = pFogController->m_fog.duration;
	//m_skyboxData.fog.enable = pFogController->m_fog.enable;
	m_skyboxData.fog.blend = pFogController->m_fog.blend;
}