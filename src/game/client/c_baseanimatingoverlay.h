//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef C_BASEANIMATINGOVERLAY_H
#define C_BASEANIMATINGOVERLAY_H
#pragma once

#include "c_baseanimating.h"

// For shared code.
class C_BaseAnimatingOverlay;
typedef C_BaseAnimatingOverlay CSharedBaseAnimatingOverlay;

class C_BaseAnimatingOverlay : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_BaseAnimatingOverlay, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();


	C_BaseAnimatingOverlay() : C_BaseAnimatingOverlay( 0 ) {}
	C_BaseAnimatingOverlay( int iEFlags );

	virtual CStudioHdr *OnNewModel();

	C_AnimationLayer* GetAnimOverlay( int i );
	void SetNumAnimOverlays( int num );	// This makes sure there is space for this # of layers.
	int GetNumAnimOverlays() const;
	void			SetOverlayPrevEventCycle( int nSlot, float flValue );

	virtual bool	Interpolate( float flCurrentTime );

	virtual void	GetRenderBounds( Vector& theMins, Vector& theMaxs );

	void			CheckForLayerChanges( CStudioHdr *hdr, float currentTime );
	void			CheckInterpChanges( void );
	void			CheckForLayerPhysicsInvalidate( void );

	// model specific
	virtual void	AccumulateLayers( IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime );
	virtual void DoAnimationEvents( CStudioHdr *pStudioHdr );

	enum
	{
		MAX_OVERLAYS = 15,
	};

	CUtlVector < C_AnimationLayer >	m_AnimOverlay;

	CUtlVector < CInterpolatedVar< C_AnimationLayer > >	m_iv_AnimOverlay;

	float m_flOverlayPrevEventCycle[ MAX_OVERLAYS ];

private:
	C_BaseAnimatingOverlay( const C_BaseAnimatingOverlay & ); // not defined, not accessible

	friend void ResizeAnimationLayerCallback( void *pStruct, int offsetToUtlVector, int len );
};

typedef C_ClientOnlyWrapper<C_BaseAnimatingOverlay> C_ClientOnlyBaseAnimatingOverlay;

EXTERN_RECV_TABLE(DT_BaseAnimatingOverlay);

inline void C_BaseAnimatingOverlay::SetOverlayPrevEventCycle( int nSlot, float flValue )
{
	m_flOverlayPrevEventCycle[nSlot] = flValue;
}

#endif // C_BASEANIMATINGOVERLAY_H




