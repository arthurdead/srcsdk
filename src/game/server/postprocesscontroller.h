//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef POSTPROCESSCONTROLLER_H
#define POSTPROCESSCONTROLLER_H
#pragma once

#include "GameEventListener.h"
#include "postprocess_shared.h"
#include "baseentity.h"
#include "fogcontroller.h"

// Spawn Flags
#define SF_POSTPROCESS_MASTER		0x0001

//=============================================================================
//
// Class Postprocess Controller:
//
class CPostProcessController : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_MAPENTITY();
	DECLARE_CLASS( CPostProcessController, CBaseEntity );

	CPostProcessController();
	virtual ~CPostProcessController();

	// Parse data from a map file
	virtual void Activate();
	virtual EdictStateFlags_t UpdateTransmitState();

	// Input handlers
	void InputSetFadeTime( inputdata_t &&inputdata );
	void InputSetLocalContrastStrength( inputdata_t &&inputdata );
	void InputSetLocalContrastEdgeStrength( inputdata_t &&inputdata );
	void InputSetVignetteStart( inputdata_t &&inputdata );
	void InputSetVignetteEnd( inputdata_t &&inputdata );
	void InputSetVignetteBlurStrength( inputdata_t &&inputdata );
	void InputSetFadeToBlackStrength( inputdata_t &&inputdata );
	void InputSetDepthBlurFocalDistance( inputdata_t &&inputdata );
	void InputSetDepthBlurStrength( inputdata_t &&inputdata );
	void InputSetScreenBlurStrength( inputdata_t &&inputdata );
	void InputSetFilmGrainStrength( inputdata_t &&inputdata );

	void InputTurnOn( inputdata_t &&inputdata );
	void InputTurnOff( inputdata_t &&inputdata );

	void Spawn( void );

	bool IsMaster( void ) const { return HasSpawnFlags( SF_FOG_MASTER ); }

public:
	CNetworkArray( float, m_flPostProcessParameters, POST_PROCESS_PARAMETER_COUNT );

	CNetworkVar( bool, m_bMaster );
};

//=============================================================================
//
// Postprocess Controller System.
//
class CPostProcessSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CPostProcessSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CPostProcessSystem()
	{
		m_hMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent( IGameEvent *pEvent );
	CPostProcessController *GetMasterPostProcessController( void )			{ return m_hMasterController; }

private:

	void InitMasterController( void );
	CHandle< CPostProcessController > m_hMasterController;
};

CPostProcessSystem *PostProcessSystem( void );


#endif // POSTPROCESSCONTROLLER_H
