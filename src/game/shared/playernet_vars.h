//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERNET_VARS_H
#define PLAYERNET_VARS_H
#pragma once

#include "shared_classnames.h"
#include "networkvar.h"
#ifndef CLIENT_DLL
#include "datamap.h"
#endif

#define NUM_AUDIO_LOCAL_SOUNDS	8

// These structs are contained in each player's local data and shared by the client & server
struct fogparams_t
{
	DECLARE_CLASS_NOBASE( fogparams_t );

	bool operator !=( const fogparams_t& other ) const;

	CNetworkVectorForDerived( dirPrimary );
	CNetworkColor32ForDerived( colorPrimary );
	CNetworkColor32ForDerived( colorSecondary );
	CNetworkColor32ForDerived( colorPrimaryLerpTo );
	CNetworkColor32ForDerived( colorSecondaryLerpTo );
	CNetworkVarForDerived( float, start );
	CNetworkVarForDerived( float, end );
	CNetworkVarForDerived( float, farz );
	CNetworkVarForDerived( float, maxdensity );

	CNetworkVarForDerived( float, startLerpTo );
	CNetworkVarForDerived( float, endLerpTo );
	CNetworkVarForDerived( float, maxdensityLerpTo );
	CNetworkVarForDerived( float, lerptime );
	CNetworkVarForDerived( float, duration );
	CNetworkVarForDerived( bool, enable );
	CNetworkVarForDerived( bool, blend );

	CNetworkVarForDerived( float, HDRColorScale );
};

struct networked_fogparams_t : public fogparams_t, public INetworkableObject
{
	DECLARE_CLASS( networked_fogparams_t, fogparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( dirPrimary );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( colorPrimary );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( colorSecondary );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( colorPrimaryLerpTo );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( colorSecondaryLerpTo );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( start );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( end );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( farz );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( maxdensity );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( startLerpTo );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( endLerpTo );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( maxdensityLerpTo );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( lerptime );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( duration );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( enable );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( blend );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( HDRColorScale );
};

// Crappy. Needs to be here because it wants to use 
#ifdef CLIENT_DLL
class C_FogController;
typedef C_FogController CSharedFogController;
#else
class CFogController;
typedef CFogController CSharedFogController;
#endif

struct fogplayerparams_t : public INetworkableObject
{
	DECLARE_CLASS_NOBASE( fogplayerparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	CNetworkHandle( CSharedFogController, m_hCtrl );
	float					m_flTransitionTime;

	color32					m_OldColor;
	float					m_flOldStart;
	float					m_flOldEnd;
	float					m_flOldMaxDensity;
	float					m_flOldHDRColorScale;
	float					m_flOldFarZ;

	color32					m_NewColor;
	float					m_flNewStart;
	float					m_flNewEnd;
	float					m_flNewMaxDensity;
	float					m_flNewHDRColorScale;
	float					m_flNewFarZ;

	fogplayerparams_t()
	{
		m_hCtrl.Set( NULL );
		m_flTransitionTime = -1.0f;
		m_OldColor.SetColor( 0, 0, 0, 0 );
		m_flOldStart = 0.0f;
		m_flOldEnd = 0.0f;
		m_flOldMaxDensity = 1.0f;
		m_flOldHDRColorScale = 1.0f;
		m_flOldFarZ = 0;
		m_NewColor.SetColor( 255, 255, 255, 255 );
		m_flNewStart = 0.0f;
		m_flNewEnd = 0.0f;
		m_flNewMaxDensity = 1.0f;
		m_flNewHDRColorScale = 1.0f;
		m_flNewFarZ = 0;
	}
};

struct sky3dparams_t
{
	DECLARE_CLASS_NOBASE( sky3dparams_t );

	// 3d skybox camera data
	CNetworkVarForDerived( int, scale );
	CNetworkVectorForDerived( origin );
	CNetworkVarForDerived( int, area );

	// Skybox angle support
	CNetworkQAngleForDerived( angles );

	// Skybox entity-based option
	CNetworkHandleForDerived( CSharedBaseEntity, skycamera );

	// Sky clearcolor
	CNetworkColor32ForDerived( skycolor );

	// 3d skybox fog data
	CNetworkVarEmbeddedCopyableForDerived( networked_fogparams_t, fog );
};

struct networked_sky3dparams_t : public sky3dparams_t, public INetworkableObject
{
	DECLARE_CLASS( networked_sky3dparams_t, sky3dparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	// 3d skybox camera data
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( scale );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( origin );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( area );

	// Skybox angle support
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( angles );

	// Skybox entity-based option
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( skycamera );

	// Sky clearcolor
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( skycolor );

	// 3d skybox fog data
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( fog );
};

struct audioparams_t : public INetworkableObject
{
	DECLARE_CLASS_NOBASE( audioparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	CNetworkArray( Vector, localSound, NUM_AUDIO_LOCAL_SOUNDS )
	CNetworkVar( int, soundscapeIndex );	// index of the current soundscape from soundscape.txt
	CNetworkVar( int, localBits );			// if bits 0,1,2,3 are set then position 0,1,2,3 are valid/used
	CNetworkHandle( CSharedBaseEntity, ent );		// the entity setting the soundscape
};

//Tony; new tonemap information.
// In single player the values are coped directly from the single env_tonemap_controller entity.
// This will allow the controller to work as it always did.
// That way nothing in ep2 will break. With these new params, the controller can properly be used in mp.
//
//
// Map specific objectives, such as blowing out a wall ( and bringing in more light )
// can still change values on a particular controller as necessary via inputs, but the
// effects will not directly affect any players who are referencing this controller
// unless the option to update on inputs is set. ( otherwise the values are simply cached
// and changes only take effect when the players controller target is changed )
//
struct tonemap_params_t : public INetworkableObject
{
	DECLARE_CLASS_NOBASE( tonemap_params_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	tonemap_params_t()
	{
		m_flAutoExposureMin = -1.0f;
		m_flAutoExposureMax = -1.0f;
		m_flTonemapScale = -1.0f;
		m_flBloomScale = -1.0f;
		m_flTonemapRate = -1.0f;
	}

	//Tony; all of these are initialized to -1!
	CNetworkVar( float, m_flTonemapScale );
	CNetworkVar( float, m_flTonemapRate );
	CNetworkVar( float, m_flBloomScale );

	CNetworkVar( float, m_flAutoExposureMin );
	CNetworkVar( float, m_flAutoExposureMax );

	// BLEND TODO
	//
	//	//Tony; Time it takes for a blend to finish, default to 0; this is for the the effect of InputBlendTonemapScale.
	//	//When
	//	CNetworkVar( float, m_flBlendTime );

	//Tony; these next 4 variables do not have to be networked; but I want to update them on the client whenever m_flBlendTime changes.
	//TODO
};

#endif // PLAYERNET_VARS_H
