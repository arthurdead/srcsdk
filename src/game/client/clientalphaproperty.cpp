//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "clientalphaproperty.h"
#include "const.h"
#include "iclientshadowmgr.h"
#include "iclientunknown.h"
#include "iclientrenderable.h"
#include "view.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Client alpha property starts here
//-----------------------------------------------------------------------------
CClientAlphaProperty::CClientAlphaProperty( )
{
	m_pOuter = NULL;
	m_pOuterMod = NULL;
	m_nRenderFX = kRenderFxNone;
	m_nRenderMode = kRenderNormal;
	m_nDesyncOffset = 0;
	m_hShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_nAlphaModulation = 255;
	m_nAlphaBlend = 255;
	m_flFadeScale = 0.0f;	// By default, things don't fade out automagically
	m_nDistFadeStart = 0;
	m_nDistFadeEnd = 0;
	m_bAlphaOverride = false;
	m_bShadowAlphaOverride = false;
	m_nDistanceFadeMode = CLIENT_ALPHA_DISTANCE_FADE_USE_CENTER;
	m_nFlags = ALPHAPROP_FLAGS_NONE;
}

void CClientAlphaProperty::Init( IClientUnknownEx *pUnk )
{
	m_pOuter = pUnk;
	m_pOuterMod = pUnk;
	m_pRenderMod = m_pOuterMod->GetClientRenderableMod();
}

void CClientAlphaProperty::Init( IClientUnknown *pUnk )
{
	m_pOuter = pUnk;
	m_pOuterMod = dynamic_cast<IClientUnknownMod *>(pUnk);
	if(m_pOuterMod)
		m_pRenderMod = m_pOuterMod->GetClientRenderableMod();
	else
		m_pRenderMod = dynamic_cast<IClientRenderableMod *>(pUnk->GetClientRenderable());
}

IClientUnknown*	CClientAlphaProperty::GetIClientUnknown()
{
	return m_pOuter;
}

IClientUnknownMod*	CClientAlphaProperty::GetIClientUnknownMod()
{
	return m_pOuterMod;
}

void CClientAlphaProperty::SetShadowHandle( ClientShadowHandle_t hShadowHandle )
{
	m_hShadowHandle = hShadowHandle;
}

void CClientAlphaProperty::SetAlphaModulation( uint8 a )
{
	m_nAlphaModulation = a;

}

void CClientAlphaProperty::EnableRenderAlphaOverride( bool bEnable )
{
	m_bAlphaOverride = bEnable;
}

void CClientAlphaProperty::EnableShadowRenderAlphaOverride( bool bEnable )
{
	m_bShadowAlphaOverride = bEnable;
}

// Sets an FX function
void CClientAlphaProperty::SetRenderFX( RenderFx_t nRenderFx, RenderMode_t nRenderMode, float flStartTime, float flDuration )
{
	bool bStartTimeUnspecified = ( flStartTime == FLT_MAX );
	bool bRenderFxChanged = ( m_nRenderFX != nRenderFx );

	switch( nRenderFx )
	{
	case kRenderFxFadeIn:
	case kRenderFxFadeOut:
		Assert( !bStartTimeUnspecified || !bRenderFxChanged );
		if ( bStartTimeUnspecified )
		{
			flStartTime = gpGlobals->curtime;
		}
		break;

	case kRenderFxFadeSlow:
	case kRenderFxSolidSlow:
		Assert( !bStartTimeUnspecified || !bRenderFxChanged );
		if ( bStartTimeUnspecified )
		{
			flStartTime = gpGlobals->curtime;
		}
		flDuration = 4.0f;
		break;

	case kRenderFxFadeFast:
	case kRenderFxSolidFast:
		Assert( !bStartTimeUnspecified || !bRenderFxChanged );
		if ( bStartTimeUnspecified )
		{
			flStartTime = gpGlobals->curtime;
		}
		flDuration = 1.0f;
		break;
	}

	m_nRenderMode = nRenderMode;
	m_nRenderFX = nRenderFx;
	if ( bRenderFxChanged || !bStartTimeUnspecified )
	{
		m_flRenderFxStartTime = flStartTime;
		m_flRenderFxDuration = flDuration;
	}
}

void CClientAlphaProperty::SetDesyncOffset( int nOffset )
{
	if(nOffset == -1)
		nOffset = rand() % 1024;

	m_nDesyncOffset = nOffset;
}

void CClientAlphaProperty::SetDistanceFadeMode( ClientAlphaDistanceFadeMode_t nFadeMode )
{
	// Necessary since m_nDistanceFadeMode is stored in 1 bit
	COMPILE_TIME_ASSERT( CLIENT_ALPHA_DISTANCE_FADE_MODE_COUNT <= ( 1 << CLIENT_ALPHA_DISTANCE_FADE_MODE_BIT_COUNT ) );
	m_nDistanceFadeMode = nFadeMode;
}


// Sets fade parameters
void CClientAlphaProperty::SetFade( float flGlobalFadeScale, float flDistFadeStart, float flDistFadeEnd )
{
	if( flDistFadeStart > flDistFadeEnd )
	{
		V_swap( flDistFadeStart, flDistFadeEnd );
	}

	// If a negative value is provided for the min fade distance, then base it off the max.
	if( flDistFadeStart < 0 )
	{
		flDistFadeStart = flDistFadeEnd + flDistFadeStart;
		if( flDistFadeStart < 0 )
		{
			flDistFadeStart = 0;
		}
	}

	Assert( flDistFadeStart >= 0 && flDistFadeStart <= 65535 );
	Assert( flDistFadeEnd >= 0 && flDistFadeEnd <= 65535 );

	m_nDistFadeStart = (uint16)flDistFadeStart;
	m_nDistFadeEnd = (uint16)flDistFadeEnd;
	m_flFadeScale = flGlobalFadeScale;
}


//-----------------------------------------------------------------------------
// Computes alpha value based on render fx
//-----------------------------------------------------------------------------
void CClientAlphaProperty::ComputeAlphaBlend( )
{
	if ( m_nRenderMode == kRenderNone || m_nRenderMode == kRenderEnvironmental )
	{
		m_nAlphaBlend = 0;
		return;
	}

	float flOffset = ((int)m_nDesyncOffset) * 363.0;// Use ent index to de-sync these fx

	switch( m_nRenderFX ) 
	{
	case kRenderFxPulseSlowWide:
		m_nAlphaBlend = m_nAlphaModulation + 0x40 * sin( gpGlobals->curtime * 2 + flOffset );	
		break;

	case kRenderFxPulseFastWide:
		m_nAlphaBlend = m_nAlphaModulation + 0x40 * sin( gpGlobals->curtime * 8 + flOffset );
		break;

	case kRenderFxPulseFastWider:
		m_nAlphaBlend = ( 0xff * fabs(sin( gpGlobals->curtime * 12 + flOffset ) ) );
		break;

	case kRenderFxPulseSlow:
		m_nAlphaBlend = m_nAlphaModulation + 0x10 * sin( gpGlobals->curtime * 2 + flOffset );
		break;

	case kRenderFxPulseFast:
		m_nAlphaBlend = m_nAlphaModulation + 0x10 * sin( gpGlobals->curtime * 8 + flOffset );
		break;

	case kRenderFxFadeOut:
	case kRenderFxFadeFast:
	case kRenderFxFadeSlow:
		{
			float flElapsed = gpGlobals->curtime - m_flRenderFxStartTime;
			float flVal = RemapValClamped( flElapsed, 0, m_flRenderFxDuration, m_nAlphaModulation, 0 );
			flVal = clamp( flVal, 0, 255 );
			m_nAlphaBlend = (int)flVal;
		}
		break;

	case kRenderFxFadeIn:
	case kRenderFxSolidFast:
	case kRenderFxSolidSlow:
		{
			float flElapsed = gpGlobals->curtime - m_flRenderFxStartTime;
			float flVal = RemapValClamped( flElapsed, 0, m_flRenderFxDuration, 0, m_nAlphaModulation );
			flVal = clamp( flVal, 0, 255 );
			m_nAlphaBlend = (int)flVal;
		}
		break;

	case kRenderFxStrobeSlow:
		m_nAlphaBlend = 20 * sin( gpGlobals->curtime * 4 + flOffset );
		m_nAlphaBlend = ( m_nAlphaBlend < 0 ) ? 0 : m_nAlphaModulation;
		break;

	case kRenderFxStrobeFast:
		m_nAlphaBlend = 20 * sin( gpGlobals->curtime * 16 + flOffset );
		m_nAlphaBlend = ( m_nAlphaBlend < 0 ) ? 0 : m_nAlphaModulation;
		break;

	case kRenderFxStrobeFaster:
		m_nAlphaBlend = 20 * sin( gpGlobals->curtime * 36 + flOffset );
		m_nAlphaBlend = ( m_nAlphaBlend < 0 ) ? 0 : m_nAlphaModulation;
		break;

	case kRenderFxFlickerSlow:
		m_nAlphaBlend = 20 * (sin( gpGlobals->curtime * 2 ) + sin( gpGlobals->curtime * 17 + flOffset ));
		m_nAlphaBlend = ( m_nAlphaBlend < 0 ) ? 0 : m_nAlphaModulation;
		break;

	case kRenderFxFlickerFast:
		m_nAlphaBlend = 20 * (sin( gpGlobals->curtime * 16 ) + sin( gpGlobals->curtime * 23 + flOffset ));
		m_nAlphaBlend = ( m_nAlphaBlend < 0 ) ? 0 : m_nAlphaModulation;
		break;

	case kRenderFxHologram:
	case kRenderFxDistort:
		{
			Vector	tmp;
			float	dist;

			VectorCopy( m_pOuter->GetClientRenderable()->GetRenderOrigin(), tmp );
			VectorSubtract( tmp, CurrentViewOrigin(), tmp );
			dist = DotProduct( tmp, CurrentViewForward() );
			
			// Turn off distance fade
			if ( m_nRenderFX == kRenderFxDistort )
			{
				dist = 1;
			}
			if ( dist <= 0 )
			{
				m_nAlphaBlend = 0;
			}
			else 
			{
				uint8 tmpAlpha = 180;
				if ( dist <= 100 )
					m_nAlphaBlend = tmpAlpha;
				else
					m_nAlphaBlend = (int) ((1.0 - (dist - 100) * (1.0 / 400.0)) * tmpAlpha);
				m_nAlphaBlend += random->RandomInt(-32,31);
			}
		}
		break;

	case kRenderFxNone:
	default:
		m_nAlphaBlend = ( m_nRenderMode == kRenderNormal ) ? 255 : m_nAlphaModulation;
		break;	
	}

	m_nAlphaBlend = clamp( m_nAlphaBlend, 0, 255 );
}

uint8 CClientAlphaProperty::ComputeRenderAlpha( bool bShadow )
{
	ComputeAlphaBlend();

	uint8 nRenderAlpha = m_nAlphaBlend;

	if( (bShadow && m_bShadowAlphaOverride) && m_pRenderMod )
	{
		nRenderAlpha = m_pRenderMod->OverrideShadowRenderAlpha( nRenderAlpha );
		nRenderAlpha = clamp( nRenderAlpha, 0, 255 );
	}
	else if( (!bShadow && m_bAlphaOverride) && m_pRenderMod )
	{
		nRenderAlpha = m_pRenderMod->OverrideRenderAlpha( nRenderAlpha );
		nRenderAlpha = clamp( nRenderAlpha, 0, 255 );
	}

	return nRenderAlpha;
}

bool CClientAlphaProperty::IgnoresZBuffer( void ) const
{
	return (
		(m_nRenderMode == kRenderGlow || m_nRenderMode == kRenderWorldGlow) ||
		((m_nFlags & ALPHAPROP_FLAGS_ALWAYS_IGNORE_Z) != 0)
	);
}
