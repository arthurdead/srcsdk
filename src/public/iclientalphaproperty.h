//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ICLIENTALPHAPROPERTY_H
#define ICLIENTALPHAPROPERTY_H
#pragma once

#include "mathlib/mathlib.h"
#include "interface.h"
#include "appframework/IAppSystem.h"
#include "const.h"

class IClientUnknown;
class IClientUnknownMod;

enum ClientAlphaDistanceFadeMode_t
{
	CLIENT_ALPHA_DISTANCE_FADE_USE_CENTER = 0,
	CLIENT_ALPHA_DISTANCE_FADE_USE_NEAREST_BBOX,

	CLIENT_ALPHA_DISTANCE_FADE_MODE_COUNT,
};



//-----------------------------------------------------------------------------
// NOTE: Clients are *not* expected to implement this interface
// Instead, these are managed completely by the client DLL.
// Use the IClientTranslucency manager to allocate + free IClientTranslucency objects
//-----------------------------------------------------------------------------
abstract_class IClientAlphaProperty
{
public:
	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;

	// Sets a constant alpha modulation value
	virtual void SetAlphaModulation( uint8 a ) = 0;

	// Sets an FX function
	// NOTE: kRenderFxFadeSlow, kRenderFxFadeFast, kRenderFxSolidSlow, kRenderFxSolidFast all need a start time only.
	// kRenderFxFadeIn/kRenderFxFadeOut needs start time + duration
	// All other render fx require no parameters
	virtual void SetRenderFX( RenderFx_t nRenderFx, RenderMode_t nRenderMode, float flStartTime = FLT_MAX, float flDuration = 0.0f ) = 0;

	// Sets fade parameters
	virtual void SetFade( float flGlobalFadeScale, float flDistFadeStart, float flDistFadeEnd ) = 0;	

	// Sets desync offset, used to make sine waves not match
	virtual void SetDesyncOffset( int nOffset ) = 0;

	// Allows the owner to override alpha.
	// The method IClientRenderable::OverrideAlphaModulation will be called
	// to allow the owner to optionally return a different alpha modulation
	virtual void EnableRenderAlphaOverride( bool bEnable ) = 0;

	// Allows the owner to override projected shadow alpha.
	// The method IClientRenderable::OverrideShadowAlphaModulation will be called
	// to allow the owner to optionally return a different alpha modulation for the shadow
	virtual void EnableShadowRenderAlphaOverride( bool bEnable ) = 0;

	// Sets the distance fade mode
	virtual void SetDistanceFadeMode( ClientAlphaDistanceFadeMode_t nFadeMode ) = 0;
};

abstract_class IClientAlphaPropertyMod
{
public:
	virtual IClientUnknown*	GetIClientUnknown() = 0;
	virtual IClientUnknownMod*	GetIClientUnknownMod();

	virtual void ComputeAlphaBlend( ) = 0;
	virtual uint8 GetAlphaBlend() const = 0;
	virtual bool IgnoresZBuffer( void ) const = 0;
};

abstract_class IClientAlphaPropertyEx : public IClientAlphaProperty, public IClientAlphaPropertyMod
{
public:
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod() { return this; }

	virtual IClientUnknown*	GetIClientUnknown() = 0;
	virtual IClientUnknownMod*	GetIClientUnknownMod();
};

//-----------------------------------------------------------------------------
// Manager used to deal with client translucency
//-----------------------------------------------------------------------------
#define CLIENT_ALPHA_PROPERTY_MGR_INTERFACE_VERSION "ClientAlphaPropertyMgrV001"

abstract_class IClientAlphaPropertyMgr
{
public:
	// Class factory
	virtual IClientAlphaProperty *CreateClientAlphaProperty( IClientUnknown *pUnk ) = 0;
	virtual void DestroyClientAlphaProperty( IClientAlphaProperty *pAlphaProperty ) = 0;
};

#ifndef SWDS
extern IClientAlphaPropertyMgr *g_pClientAlphaPropertyMgr;
#endif


#endif // ICLIENTALPHAPROPERTY_H
