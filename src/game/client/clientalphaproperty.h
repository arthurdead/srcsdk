//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef CLIENTALPHAPROPERTY_H
#define CLIENTALPHAPROPERTY_H
#pragma once

#include "iclientalphaproperty.h"
#include "iclientrenderable.h"
#include "iclientunknown.h"
#include "cdll_client_int.h"

#define CLIENT_ALPHA_DISTANCE_FADE_MODE_BIT_COUNT 1

enum
{
	ALPHAPROP_FLAGS_NONE = 0,
	ALPHAPROP_FLAGS_ALWAYS_IGNORE_Z = (1 << 0),
};

//-----------------------------------------------------------------------------
// Implementation class
//-----------------------------------------------------------------------------
class CClientAlphaProperty : public IClientAlphaPropertyEx
{
	// Inherited from IClientAlphaProperty
public:
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod() { return this; }
	virtual IClientUnknown*	GetIClientUnknown();
	virtual IClientUnknownMod*	GetIClientUnknownMod();
	virtual void SetAlphaModulation( uint8 a );
	virtual void SetRenderFX( RenderFx_t nRenderFx, RenderMode_t nRenderMode, float flStartTime = FLT_MAX, float flDuration = 0.0f );
	virtual void SetFade( float flGlobalFadeScale, float flDistFadeMinDist, float flDistFadeMaxDist );	
	virtual void SetDesyncOffset( int nOffset );
	virtual void EnableRenderAlphaOverride( bool bEnable );
	virtual void EnableShadowRenderAlphaOverride( bool bEnable );
	virtual void SetDistanceFadeMode( ClientAlphaDistanceFadeMode_t nFadeMode );

	// Other public methods
public:
	CClientAlphaProperty( );
	void Init( IClientUnknown *pUnk );
	void Init( IClientUnknownEx *pUnk );

	void SetRenderFX( RenderFx_t nRenderFx, float flStartTime = FLT_MAX, float flDuration = 0.0f )
	{ SetRenderFX(nRenderFx, GetRenderMode(), flStartTime, flDuration); }

	void SetRenderMode( RenderMode_t nRenderMode )
	{ SetRenderFX(GetRenderFX(), nRenderMode, FLT_MAX, 0.0f); }

	void SetFade( float flDistFadeMinDist, float flDistFadeMaxDist )
	{ SetFade(GetGlobalFadeScale(), flDistFadeMinDist, flDistFadeMaxDist); }

	void SetFade( float flGlobalFadeScale )
	{ SetFade(flGlobalFadeScale, GetMinFadeDist(), GetMaxFadeDist()); }

	// NOTE: Only the client shadow manager should ever call this method!
	void SetShadowHandle( ClientShadowHandle_t hShadowHandle );

	// Returns the current alpha modulation (no fades or render FX taken into account)
	uint8 GetAlphaModulation() const;

	uint8 GetAlphaBlend() const;

	void ComputeAlphaBlend();

	// Compute the render alpha (after fades + render FX are applied)
	uint8 ComputeRenderAlpha( bool bShadow = false );

	// Returns alpha fade
	float GetMinFadeDist() const;
	float GetMaxFadeDist() const;
	float GetGlobalFadeScale() const;

	// Should this ignore the Z buffer?
	bool IgnoresZBuffer( void ) const;

	RenderMode_t GetRenderMode( void ) const;

	RenderFx_t GetRenderFX( void ) const;

private:
	// NOTE: Be careful if you add data to this class.
	// It needs to be no more than 32 bytes, which it is right now
	// (remember the vtable adds 4 bytes). Try to restrict usage
	// to reserved areas or figure out a way of compressing existing fields

	// ^^^^^ Arthurdead: No it doesnt?

	IClientUnknown *m_pOuter;

	ClientShadowHandle_t m_hShadowHandle;
	uint16 m_nRenderFX : 5;
	uint16 m_nRenderMode : 4;
	uint16 m_bAlphaOverride : 1;
	uint16 m_bShadowAlphaOverride : 1;
	uint16 m_nDistanceFadeMode : CLIENT_ALPHA_DISTANCE_FADE_MODE_BIT_COUNT;
	uint16 m_nFlags : 4;

	uint16 m_nDesyncOffset;
	uint8 m_nAlphaModulation;
	uint8 m_nAlphaBlend;

	uint16 m_nDistFadeStart;
	uint16 m_nDistFadeEnd;

	float m_flFadeScale;
	float m_flRenderFxStartTime;
	float m_flRenderFxDuration;

	int m_nFXComputeFrame;

	IClientUnknownMod *m_pOuterMod;
	IClientRenderableMod *m_pRenderMod;

	friend class CClientLeafSystem;
};

//COMPILE_TIME_ASSERT(sizeof(CClientAlphaProperty) == 32);

// Returns the current alpha modulation
inline uint8 CClientAlphaProperty::GetAlphaModulation() const
{
	return m_nAlphaModulation;
}

inline uint8 CClientAlphaProperty::GetAlphaBlend() const
{
	//Assert( m_nFXComputeFrame == gpGlobals->framecount );
	return m_nAlphaBlend;
}

inline float CClientAlphaProperty::GetMinFadeDist() const
{
	return m_nDistFadeStart;
}

inline float CClientAlphaProperty::GetMaxFadeDist() const
{
	return m_nDistFadeEnd;
}

inline float CClientAlphaProperty::GetGlobalFadeScale() const
{
	return m_flFadeScale;
}

inline RenderMode_t CClientAlphaProperty::GetRenderMode( void ) const
{
	return (RenderMode_t)m_nRenderMode;
}

inline RenderFx_t CClientAlphaProperty::GetRenderFX( void ) const
{
	return (RenderFx_t)m_nRenderFX;
}

#endif // CLIENTALPHAPROPERTY_H
