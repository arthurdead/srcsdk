//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "fx.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern modelindex_t g_sModelIndexBloodDrop;	
extern modelindex_t g_sModelIndexBloodSpray;

//-----------------------------------------------------------------------------
// Purpose: Blood sprite
//-----------------------------------------------------------------------------
class C_TEBloodSprite : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEBloodSprite, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEBloodSprite( void );
	virtual			~C_TEBloodSprite( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	Vector			m_vecDirection;
	color32 m_clr;
	modelindex_t				m_nDropModel;
	modelindex_t				m_nSprayModel;
	int				m_nSize;
};


// Expose it to the engine.
IMPLEMENT_CLIENTCLASS_EVENT( C_TEBloodSprite, DT_TEBloodSprite, CTEBloodSprite );


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE(C_TEBloodSprite, DT_TEBloodSprite)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecDirection)),
	RecvPropColor32( RECVINFO(m_clr) ),
	RecvPropInt( RECVINFO(m_nSprayModel)),
	RecvPropInt( RECVINFO(m_nDropModel)),
	RecvPropInt( RECVINFO(m_nSize)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBloodSprite::C_TEBloodSprite( void )
{
	m_vecOrigin.Init();
	m_vecDirection.Init();

	m_clr.SetColor( 0, 0, 0, 0 );
	m_nSize = 0;
	m_nSprayModel = INVALID_MODEL_INDEX;
	m_nDropModel = INVALID_MODEL_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBloodSprite::~C_TEBloodSprite( void )
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordBloodSprite( const Vector &start, const Vector &direction, 
	color32 clr, modelindex_t nSprayModelIndex, modelindex_t nDropModelIndex, int size )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		const model_t* pSprayModel = (nSprayModelIndex.IsValid()) ? modelinfo->GetModel( nSprayModelIndex ) : NULL;
		const model_t* pDropModel = (nDropModelIndex.IsValid()) ? modelinfo->GetModel( nDropModelIndex ) : NULL;
		const char *pSprayModelName = pSprayModel ? modelinfo->GetModelName( pSprayModel ) : "";
		const char *pDropModelName = pDropModel ? modelinfo->GetModelName( pDropModel ) : "";

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_BLOOD_SPRITE );
 		msg->SetString( "name", "TE_BloodSprite" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );
		msg->SetColor( "color", clr );
  		msg->SetString( "spraymodel", pSprayModelName );
 		msg->SetString( "dropmodel", pDropModelName );
		msg->SetInt( "size", size );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
void TE_BloodSprite( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector *dir, color32 clr, int size )
{
	Vector	offset = *org + ( (*dir) * 4.0f );

	tempents->BloodSprite( offset, clr, g_sModelIndexBloodSpray, g_sModelIndexBloodDrop, size );	
	FX_Blood( offset, (Vector &)*dir, clr );
	RecordBloodSprite( *org, *dir, clr, g_sModelIndexBloodSpray, g_sModelIndexBloodDrop, size );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEBloodSprite::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEBloodSprite::PostDataUpdate" );

	Vector	offset = m_vecOrigin + ( m_vecDirection * 4.0f );

	tempents->BloodSprite( offset, m_clr, m_nSprayModel, m_nDropModel, m_nSize );	
	FX_Blood( offset, m_vecDirection, m_clr );
	RecordBloodSprite( m_vecOrigin, m_vecDirection, m_clr, m_nSprayModel, m_nDropModel, m_nSize );
}

void TE_BloodSprite( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecDirection;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	vecDirection.x = pKeyValues->GetFloat( "directionx" );
	vecDirection.y = pKeyValues->GetFloat( "directiony" );
	vecDirection.z = pKeyValues->GetFloat( "directionz" );
	Color c = pKeyValues->GetColor( "color" );
//	const char *pSprayModelName = pKeyValues->GetString( "spraymodel" );
//	const char *pDropModelName = pKeyValues->GetString( "dropmodel" );
	int nSize = pKeyValues->GetInt( "size" );
	TE_BloodSprite( filter, 0.0f, &vecOrigin, &vecDirection, c, nSize );
}