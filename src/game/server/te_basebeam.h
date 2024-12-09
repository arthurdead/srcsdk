//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

//-----------------------------------------------------------------------------
// Purpose: Dispatches a beam ring between two entities
//-----------------------------------------------------------------------------
#if !defined( TE_BASEBEAM_H )
#define TE_BASEBEAM_H
#pragma once

#include "basetempentity.h"

abstract_class CTEBaseBeam : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTEBaseBeam, CBaseTempEntity );
	DECLARE_SERVERCLASS();


public:
					CTEBaseBeam( const char *name );
	virtual			~CTEBaseBeam( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles ) = 0;
	
public:
	CNetworkModelIndex( m_nModelIndex );
	CNetworkModelIndex( m_nHaloIndex );
	CNetworkVar( int, m_nStartFrame );
	CNetworkVar( int, m_nFrameRate );
	CNetworkVar( float, m_fLife );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fEndWidth );
	CNetworkVar( int, m_nFadeLength );
	CNetworkVar( float, m_fAmplitude );
	CNetworkColor32( m_clr );
	CNetworkVar( int, m_nSpeed );
	CNetworkVar( int, m_nFlags );
};

EXTERN_SEND_TABLE(DT_BaseBeam);

#endif // TE_BASEBEAM_H