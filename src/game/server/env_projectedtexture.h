#ifndef ENV_PROJECTEDTEXTURE_H
#define ENV_PROJECTEDTEXTURE_H
#pragma once

#include "baseentity.h"

#define ENV_PROJECTEDTEXTURE_STARTON				( 1 << 0)
#define ENV_PROJECTEDTEXTURE_ALWAYSUPDATE			( 1 << 1)
#define ENV_PROJECTEDTEXTURE_VOLUMETRICS_START_ON	( 1 << 2)
#define ENV_PROJECTEDTEXTURE_UBERLIGHT				( 1 << 3)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvProjectedTexture : public CPointEntity
{
	DECLARE_CLASS( CEnvProjectedTexture, CPointEntity );
public:
	DECLARE_MAPENTITY();
	DECLARE_SERVERCLASS();

	CEnvProjectedTexture();
	bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

	// Always transmit to clients
	virtual int UpdateTransmitState();
	void Spawn( void );
	virtual void Activate( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputAlwaysUpdateOn( inputdata_t &inputdata );
	void InputAlwaysUpdateOff( inputdata_t &inputdata );
	void InputSetFOV( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata );
	void InputSetCameraSpace( inputdata_t &inputdata );
	void InputSetLightOnlyTarget( inputdata_t &inputdata );
	void InputSetLightWorld( inputdata_t &inputdata );
	void InputSetEnableShadows( inputdata_t &inputdata );
	void InputSetLightColor( inputdata_t &inputdata );
	void InputSetSpotlightTexture( inputdata_t &inputdata );
	void InputSetAmbient( inputdata_t &inputdata );
	void InputSetEnableVolumetrics( inputdata_t &inputdata );
	void InputEnableUberLight( inputdata_t &inputdata );
	void InputDisableUberLight( inputdata_t &inputdata );
	void InputSetNearZ( inputdata_t &inputdata );
	void InputSetFarZ( inputdata_t &inputdata );
	void InputSetBrightnessScale( inputdata_t &inputdata );

	void InitialThink( void );
	void FlickerThink( void );

	CNetworkHandle( CBaseEntity, m_hTargetEntity );

private:

	CNetworkVar( bool, m_bState );
	CNetworkVar( bool, m_bAlwaysUpdate );
	CNetworkVar( float, m_flLightFOV );
	CNetworkVar( bool, m_bEnableShadows );
	CNetworkVar( bool, m_bSimpleProjection );
	CNetworkVar( bool, m_bLightOnlyTarget );
	CNetworkVar( bool, m_bLightWorld );
	CNetworkVar( bool, m_bCameraSpace );
	CNetworkVar( float, m_flBrightnessScale );
	CNetworkColor32( m_LightColor );
	CNetworkVar( float, m_flColorTransitionTime );
	CNetworkVar( float, m_flAmbient );
	CNetworkString( m_SpotlightTextureName, MAX_PATH );
	CNetworkVar( int, m_nSpotlightTextureFrame );
	CNetworkVar( float, m_flNearZ );
	CNetworkVar( float, m_flFarZ );
	CNetworkVar( int, m_nShadowQuality );
	CNetworkVar( float, m_flProjectionSize );
	CNetworkVar( float, m_flRotation );

	CNetworkVar( bool, m_bFlicker );

	CNetworkVar( bool, m_bEnableVolumetrics );
	CNetworkVar( bool, m_bEnableVolumetricsLOD );
	CNetworkVar( float, m_flVolumetricsFadeDistance );
	CNetworkVar( int, m_iVolumetricsQuality );
	CNetworkVar( float, m_flVolumetricsQualityBias );
	CNetworkVar( float, m_flVolumetricsMultiplier );

	CNetworkVar( bool, m_bUberlight );
	CNetworkVar( float, m_fNearEdge );
	CNetworkVar( float, m_fFarEdge );
	CNetworkVar( float, m_fCutOn );
	CNetworkVar( float, m_fCutOff );
	CNetworkVar( float, m_fShearx );
	CNetworkVar( float, m_fSheary );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fWedge );
	CNetworkVar( float, m_fHeight );
	CNetworkVar( float, m_fHedge );
	CNetworkVar( float, m_fRoundness );

	friend void CC_CreateFlashlight( const CCommand &args );
};


#endif	// ENV_PROJECTEDTEXTURE_H