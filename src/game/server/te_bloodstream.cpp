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
#include "cbase.h"
#include "te_particlesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches blood stream tempentity
//-----------------------------------------------------------------------------
class CTEBloodStream : public CTEParticleSystem
{
public:
	DECLARE_CLASS( CTEBloodStream, CTEParticleSystem );

					CTEBloodStream( const char *name );
	virtual			~CTEBloodStream( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecDirection );
	CNetworkColor32( m_clr );
	CNetworkVar( int, m_nAmount );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBloodStream::CTEBloodStream( const char *name ) :
	BaseClass( name )
{
	m_vecDirection.Init();
	m_clr.SetColor( 0, 0, 0, 0 );
	m_nAmount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBloodStream::~CTEBloodStream( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBloodStream::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_clr.SetColor( 247, 0, 0, 255 );
	m_nAmount	= random_valve->RandomInt(50, 150);
	m_vecOrigin = current_origin;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;	
	VectorNormalize( forward );

	m_vecOrigin += forward * 50;

	m_vecDirection = UTIL_RandomBloodVector();

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBloodStream, DT_TEBloodStream)
	SendPropVector( SENDINFO(m_vecDirection), 11, 0, -10.0, 10.0 ),
	SendPropColor32( SENDINFO(m_clr) ),
	SendPropInt( SENDINFO(m_nAmount), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton to fire TEBloodStream objects
static CTEBloodStream g_TEBloodStream( "Blood Stream" );

//-----------------------------------------------------------------------------
// Purpose: Creates a blood stream
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*org - 
//			*dir - 
//			r - 
//			g - 
//			b - 
//			a - 
//			amount - 
//-----------------------------------------------------------------------------
void TE_BloodStream( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector* dir, color32 clr, int amount )
{
	g_TEBloodStream.m_vecOrigin = *org;
	g_TEBloodStream.m_vecDirection = *dir;	
	g_TEBloodStream.m_clr = clr;
	g_TEBloodStream.m_nAmount = amount;

	// Send it over the wire
	g_TEBloodStream.Create( filter, delay );
}