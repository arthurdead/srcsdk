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
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern modelindex_t g_sModelIndexBloodDrop;		// (in combatweapon.cpp) holds the sprite index for the initial blood
extern modelindex_t g_sModelIndexBloodSpray;	// (in combatweapon.cpp) holds the sprite index for splattered blood

//-----------------------------------------------------------------------------
// Purpose: Display's a blood sprite
//-----------------------------------------------------------------------------
class CTEBloodSprite : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEBloodSprite, CBaseTempEntity );

					CTEBloodSprite( const char *name );
	virtual			~CTEBloodSprite( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecDirection );
	CNetworkModelIndex( m_nSprayModel );
	CNetworkModelIndex( m_nDropModel );
	CNetworkColor32( m_clr );
	CNetworkVar( int, m_nSize );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBloodSprite::CTEBloodSprite( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_nSprayModel = INVALID_MODEL_INDEX;
	m_nDropModel = INVALID_MODEL_INDEX;
	m_clr.SetColor( 0, 0, 0, 0 );
	m_nSize = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBloodSprite::~CTEBloodSprite( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBloodSprite::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_clr.SetColor( 255, 255, 63, 255 );
	m_nSize	= 16;
	m_vecOrigin = current_origin;
	
	m_nSprayModel = g_sModelIndexBloodSpray;
	m_nDropModel = g_sModelIndexBloodDrop;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEBloodSprite, DT_TEBloodSprite)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecDirection), -1, SPROP_COORD),
	SendPropColor32( SENDINFO(m_clr) ),
	SendPropModelIndex( SENDINFO(m_nSprayModel) ),
	SendPropModelIndex( SENDINFO(m_nDropModel) ),
	SendPropInt( SENDINFO(m_nSize), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton
static CTEBloodSprite g_TEBloodSprite( "Blood Sprite" );

//-----------------------------------------------------------------------------
// Purpose: Public interface
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*org - 
//			r - 
//			g - 
//			b - 
//			a - 
//			size - 
//-----------------------------------------------------------------------------
void TE_BloodSprite( IRecipientFilter& filter, float delay,
	const Vector *org, const Vector *dir, color32 clr, int size )
{
	// Set up parameters
	g_TEBloodSprite.m_vecOrigin		= *org;
	g_TEBloodSprite.m_vecDirection	= *dir;
	g_TEBloodSprite.m_clr = clr;
	g_TEBloodSprite.m_nSize = size;

	// Implicit
	g_TEBloodSprite.m_nSprayModel = g_sModelIndexBloodSpray;
	g_TEBloodSprite.m_nDropModel = g_sModelIndexBloodDrop;

	// Create it
	g_TEBloodSprite.Create( filter, delay );
}