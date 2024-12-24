//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// #include "BaseAnimating.h"

#ifndef BASE_ANIMATING_OVERLAY_H
#define BASE_ANIMATING_OVERLAY_H
#pragma once

#include "baseanimating.h"
#include "ai_activity.h"

class CBaseAnimatingOverlay;
typedef CBaseAnimatingOverlay CSharedBaseAnimatingOverlay;

enum animlayerindex_t : unsigned char;
inline const animlayerindex_t INVALID_ANIMLAYER = (animlayerindex_t)-1;

UNORDEREDENUM_OPERATORS( animlayerindex_t, unsigned char )

enum AnimLayerFlags_t : unsigned char
{
	ANIM_LAYER_NO_FLAGS = 0,
	ANIM_LAYER_ACTIVE = 0x0001,
	ANIM_LAYER_AUTOKILL = 0x0002,
	ANIM_LAYER_KILLME = 0x0004,
	ANIM_LAYER_CHECKACCESS = 0x0010,
	ANIM_LAYER_DYING = 0x0020,
	ANIM_LAYER_NOEVENTS = 0x0040,
};

FLAGENUM_OPERATORS( AnimLayerFlags_t, unsigned char )

class CAnimationLayer : public CMemZeroOnNew, public INetworkableObject
{
public:	
	DECLARE_CLASS_NOBASE( CAnimationLayer );
	// For CNetworkVars.
	DECLARE_EMBEDDED_NETWORKVAR();

	CAnimationLayer() = delete;
	CAnimationLayer(const CAnimationLayer &) = delete;
	CAnimationLayer(CAnimationLayer &&) = delete;
	CAnimationLayer &operator=(const CAnimationLayer &) = delete;
	CAnimationLayer &operator=(CAnimationLayer &&) = delete;

	void	Init( CBaseAnimatingOverlay *pOverlay );

	// float	SetBlending( int iBlender, float flValue, CBaseAnimating *pOwner );
	void	StudioFrameAdvance( float flInterval, CBaseAnimating *pOwner );
	void	DispatchAnimEvents( CBaseAnimating *eventHandler, CBaseAnimating *pOwner );
	void	SetOrder( int nOrder );

	float GetFadeout( float flCurTime );

public:	

	AnimLayerFlags_t		m_fFlags;

	bool	m_bSequenceFinished;
	bool	m_bLooping;
	
	CNetworkSequence( m_nSequence );
	CNetworkAnimCycle( m_flCycle );
	CNetworkAnimCycle( m_flPrevCycle );
	CNetworkScale( m_flWeight );
	
	float	m_flPlaybackRate;

	float	m_flBlendIn; // start and end blend frac (0.0 for now blend)
	float	m_flBlendOut; 

	float	m_flKillRate;
	float	m_flKillDelay;

	float	m_flLayerAnimtime;
	float	m_flLayerFadeOuttime;

	// For checking for duplicates
	Activity	m_nActivity;

	// order of layering on client
	int		m_nPriority;
	CNetworkVar( int, m_nOrder );

	bool	IsActive( void ) const { return ((m_fFlags & ANIM_LAYER_ACTIVE) != ANIM_LAYER_NO_FLAGS); }
	bool	IsAutokill( void ) const { return ((m_fFlags & ANIM_LAYER_AUTOKILL) != ANIM_LAYER_NO_FLAGS); }
	bool	IsKillMe( void ) const { return ((m_fFlags & ANIM_LAYER_KILLME) != ANIM_LAYER_NO_FLAGS); }
	bool	IsAutoramp( void ) const { return (m_flBlendIn != 0.0 || m_flBlendOut != 0.0); }
	void	KillMe( void ) { m_fFlags |= ANIM_LAYER_KILLME; }
	void	Dying( void ) { m_fFlags |= ANIM_LAYER_DYING; }
	bool	IsDying( void ) const { return ((m_fFlags & ANIM_LAYER_DYING) != ANIM_LAYER_NO_FLAGS); }
	void	Dead( void ) { m_fFlags &= ~ANIM_LAYER_DYING; }
	bool	NoEvents( void ) const { return ((m_fFlags & ANIM_LAYER_NOEVENTS) != ANIM_LAYER_NO_FLAGS); }

	void	SetSequence( sequence_t nSequence );
	void	SetCycle( float flCycle );
	void	SetPrevCycle( float flCycle );
	void	SetPlaybackRate( float flPlaybackRate );
	void	SetWeight( float flWeight );

	sequence_t		GetSequence( ) const;
	float	GetCycle( ) const;
	float	GetPrevCycle( ) const;
	float	GetPlaybackRate( ) const;
	float	GetWeight( ) const;

	bool	IsAbandoned( void ) const;
	void	MarkActive( void );

	float	m_flLastEventCheck;

	float	m_flLastAccess;
};

FORCEINLINE void CAnimationLayer::SetSequence( sequence_t nSequence )
{
	m_nSequence = nSequence;
}

FORCEINLINE void CAnimationLayer::SetCycle( float flCycle )
{
	m_flCycle = flCycle;
}

FORCEINLINE void CAnimationLayer::SetWeight( float flWeight )
{
	m_flWeight = flWeight;
}

FORCEINLINE void CAnimationLayer::SetPrevCycle( float flPrevCycle )
{
	m_flPrevCycle = flPrevCycle;
}

FORCEINLINE void CAnimationLayer::SetPlaybackRate( float flPlaybackRate )
{
	m_flPlaybackRate = flPlaybackRate;
}

FORCEINLINE sequence_t	CAnimationLayer::GetSequence( ) const
{
	return m_nSequence;
}

FORCEINLINE float CAnimationLayer::GetCycle( ) const
{
	return m_flCycle;
}

FORCEINLINE float CAnimationLayer::GetPrevCycle( ) const
{
	return m_flPrevCycle;
}

FORCEINLINE float CAnimationLayer::GetPlaybackRate( ) const
{
	return m_flPlaybackRate;
}

FORCEINLINE float CAnimationLayer::GetWeight( ) const
{
	return m_flWeight;
}

inline float CAnimationLayer::GetFadeout( float flCurTime )
{
	float s;

	if (m_flLayerFadeOuttime <= 0.0f)
	{
		s = 0;
	}
	else
	{
		// blend in over 0.2 seconds
		s = 1.0 - (flCurTime - m_flLayerAnimtime) / m_flLayerFadeOuttime;
		if (s > 0 && s <= 1.0)
		{
			// do a nice spline curve
			s = 3 * s * s - 2 * s * s * s;
		}
		else if ( s > 1.0f )
		{
			// Shouldn't happen, but maybe curtime is behind animtime?
			s = 1.0f;
		}
	}
	return s;
}

class CBaseAnimatingOverlay : public CBaseAnimating
{
public:
	DECLARE_CLASS( CBaseAnimatingOverlay, CBaseAnimating );

	enum 
	{
		MAX_OVERLAYS = 15,
	};

private:
	CNetworkUtlVector( CAnimationLayer, m_AnimOverlay );
	//int				m_nActiveLayers;
	//int				m_nActiveBaseLayers;

public:
	
	virtual CBaseAnimatingOverlay *	GetBaseAnimatingOverlay() { return this; }

	virtual void	SetModel( const char *szModelName );

	virtual void	StudioFrameAdvance();
	virtual	void	DispatchAnimEvents ( CBaseAnimating *eventHandler );
	virtual void	GetSkeleton( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], int boneMask );

	animlayerindex_t		AddGestureSequence( sequence_t sequence, bool autokill = true );
	animlayerindex_t		AddGestureSequence( sequence_t sequence, float flDuration, bool autokill = true );
	animlayerindex_t		AddGesture( Activity activity, bool autokill = true );
	animlayerindex_t		AddGesture( Activity activity, float flDuration, bool autokill = true );
	bool	IsPlayingGesture( Activity activity );
	void	RestartGesture( Activity activity, bool addifmissing = true, bool autokill = true );
	void	RemoveGesture( Activity activity );
	void	RemoveAllGestures( void );

	animlayerindex_t		AddLayeredSequence( sequence_t sequence, int iPriority );

	void	SetLayerPriority( animlayerindex_t iLayer, int iPriority );

	bool	IsValidLayer( animlayerindex_t iLayer );

	void	SetLayerDuration( animlayerindex_t iLayer, float flDuration );
	float	GetLayerDuration( animlayerindex_t iLayer );

	void	SetLayerCycle( animlayerindex_t iLayer, float flCycle );
	void	SetLayerCycle( animlayerindex_t iLayer, float flCycle, float flPrevCycle );
	void	SetLayerCycle( animlayerindex_t iLayer, float flCycle, float flPrevCycle, float flLastEventCheck );
	float	GetLayerCycle( animlayerindex_t iLayer );

	void	SetLayerPlaybackRate( animlayerindex_t iLayer, float flPlaybackRate );
	void	SetLayerWeight( animlayerindex_t iLayer, float flWeight );
	float	GetLayerWeight( animlayerindex_t iLayer );
	void	SetLayerBlendIn( animlayerindex_t iLayer, float flBlendIn );
	void	SetLayerBlendOut( animlayerindex_t iLayer, float flBlendOut );
	void	SetLayerAutokill( animlayerindex_t iLayer, bool bAutokill );
	void	SetLayerLooping( animlayerindex_t iLayer, bool bLooping );
	void	SetLayerNoEvents( animlayerindex_t iLayer, bool bNoEvents );

	bool	IsLayerFinished( animlayerindex_t iLayer );

	Activity	GetLayerActivity( animlayerindex_t iLayer );
	sequence_t			GetLayerSequence( animlayerindex_t iLayer );

	animlayerindex_t		FindGestureLayer( Activity activity );

	void	RemoveLayer( animlayerindex_t iLayer, float flKillRate = 0.2, float flKillDelay = 0.0 );
	void	FastRemoveLayer( animlayerindex_t iLayer );

	const CAnimationLayer *GetAnimOverlay( animlayerindex_t iIndex ) const;
	CAnimationLayer *GetAnimOverlayForModify( animlayerindex_t iIndex );
	int GetNumAnimOverlays() const;
	void SetNumAnimOverlays( int num );

	void VerifyOrder( void );

	bool	HasActiveLayer( void );

private:
	animlayerindex_t		AllocateLayer( int iPriority = 0 ); // lower priorities are processed first

	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();
};

EXTERN_SEND_TABLE(DT_BaseAnimatingOverlay);

inline int CBaseAnimatingOverlay::GetNumAnimOverlays() const
{
	return m_AnimOverlay.Count();
}

// ------------------------------------------------------------------------------------------ //
// CAnimationLayer inlines.
// ------------------------------------------------------------------------------------------ //

inline void CAnimationLayer::SetOrder( int nOrder )
{
	m_nOrder = nOrder;
}

#endif // BASE_ANIMATING_OVERLAY_H
