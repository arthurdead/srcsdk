//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FOGCONTROLLER_H
#define FOGCONTROLLER_H
#pragma once

#include "playernet_vars.h"
#include "igamesystem.h"
#include "baseentity.h"
#include "GameEventListener.h"

bool GetWorldFogParams( CBaseCombatCharacter *character, fogparams_t &fog );

// Spawn Flags
#define SF_FOG_MASTER		0x0001

//=============================================================================
//
// Class Fog Controller:
// Compares a set of integer inputs to the one main input
// Outputs true if they are all equivalant, false otherwise
//
class CFogController : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_MAPENTITY();
	DECLARE_CLASS( CFogController, CBaseEntity );

	CFogController();
	~CFogController();

	// Parse data from a map file
	virtual void Activate();
	virtual EdictStateFlags_t UpdateTransmitState();

	// Input handlers
	void InputSetStartDist( inputdata_t &&inputdata );
	void InputSetEndDist( inputdata_t &&inputdata );
	void InputTurnOn( inputdata_t &&inputdata );
	void InputTurnOff( inputdata_t &&inputdata );
	void InputSetColor( inputdata_t &&inputdata );
	void InputSetColorSecondary( inputdata_t &&inputdata );
	void InputSetFarZ( inputdata_t &&inputdata );
	void InputSetAngles( inputdata_t &&inputdata );
	void InputSetMaxDensity( inputdata_t &&inputdata );

	void InputSetColorLerpTo( inputdata_t &&inputdata );
	void InputSetColorSecondaryLerpTo( inputdata_t &&inputdata );
	void InputSetStartDistLerpTo( inputdata_t &&inputdata );
	void InputSetEndDistLerpTo( inputdata_t &&inputdata );
	void InputSetMaxDensityLerpTo( inputdata_t &&inputdata );

	void InputStartFogTransition( inputdata_t &&inputdata );

	int DrawDebugTextOverlays(void);

	void SetLerpValues( void );
	void Spawn( void );

	bool IsMaster( void )					{ return HasSpawnFlags( SF_FOG_MASTER ); }

public:

	CNetworkVarEmbedded( networked_fogparams_t, m_fog );
	bool					m_bUseAngles;
	int						m_iChangedVariables;
};

//=============================================================================
//
// Fog Controller System.
//
class CFogSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CFogSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CFogSystem()
	{
		m_hMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent( IGameEvent *pEvent ) { InitMasterController(); }
	CFogController *GetMasterFogController( void )			{ return m_hMasterController.Get(); }

private:

	void InitMasterController( void );
	CHandle< CFogController > m_hMasterController;
};

CFogSystem *FogSystem( void );

#endif // FOGCONTROLLER_H
