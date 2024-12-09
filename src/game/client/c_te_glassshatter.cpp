//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "particle_simple3d.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "fx.h"
#include "tier0/vprof.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PI 3.14159265359
#define GLASS_SHARD_MIN_LIFE 2
#define GLASS_SHARD_MAX_LIFE 5
#define GLASS_SHARD_NOISE	 0.3
#define GLASS_SHARD_GRAVITY  500
#define GLASS_SHARD_DAMPING	 0.3

#include "clienteffectprecachesystem.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGlassShatter )
CLIENTEFFECT_MATERIAL( "effects/fleck_glass1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_glass2" )
CLIENTEFFECT_MATERIAL( "effects/fleck_tile1" )
CLIENTEFFECT_MATERIAL( "effects/fleck_tile2" )
CLIENTEFFECT_REGISTER_END()

ConVar fx_glass_velocity_cap("fx_glass_velocity_cap", "0", 0, "Maximum downwards speed of shattered glass particles");

//###################################################
// > C_TEShatterSurface
//###################################################
class C_TEShatterSurface : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEShatterSurface, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TEShatterSurface( void );
	~C_TEShatterSurface( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

private:
	// Recording 
	void RecordShatterSurface( );

public:
	Vector					m_vecOrigin;
	QAngle					m_vecAngles;
	Vector					m_vecForce;
	Vector					m_vecForcePos;
	float					m_flWidth;
	float					m_flHeight;
	float					m_flShardSize;
	PMaterialHandle			m_pMaterialHandle;
	int						m_nSurfaceType;
	color24					m_uchFrontColor;
	color24					m_uchBackColor;
};


//------------------------------------------------------------------------------
// Networking
//------------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEShatterSurface, DT_TEShatterSurface, CTEShatterSurface)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecAngles)),
	RecvPropVector( RECVINFO(m_vecForce)),
	RecvPropVector( RECVINFO(m_vecForcePos)),
	RecvPropFloat( RECVINFO(m_flWidth)),
	RecvPropFloat( RECVINFO(m_flHeight)),
	RecvPropFloat( RECVINFO(m_flShardSize)),
	RecvPropInt( RECVINFO(m_nSurfaceType)),	
	RecvPropColor24( RECVINFO(m_uchFrontColor)),
	RecvPropColor24( RECVINFO(m_uchBackColor)),
END_RECV_TABLE()


//------------------------------------------------------------------------------
// Constructor, destructor
//------------------------------------------------------------------------------
C_TEShatterSurface::C_TEShatterSurface( void )
{
	m_vecOrigin.Init();
	m_vecAngles.Init();
	m_vecForce.Init();
	m_vecForcePos.Init();
	m_flWidth			= 16.0;
	m_flHeight			= 16.0;
	m_flShardSize		= 3;
	m_nSurfaceType		= SHATTERSURFACE_GLASS;
	m_uchFrontColor.SetColor( 255, 255, 255 );
	m_uchBackColor.SetColor( 255, 255, 255 );
}

C_TEShatterSurface::~C_TEShatterSurface()
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
void C_TEShatterSurface::RecordShatterSurface( )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		Color front( m_uchFrontColor.r(), m_uchFrontColor.g(), m_uchFrontColor.b(), 255 );
		Color back( m_uchBackColor.r(), m_uchBackColor.g(), m_uchBackColor.b(), 255 );

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_SHATTER_SURFACE );
 		msg->SetString( "name", "TE_ShatterSurface" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", m_vecOrigin.x );
		msg->SetFloat( "originy", m_vecOrigin.y );
		msg->SetFloat( "originz", m_vecOrigin.z );
		msg->SetFloat( "anglesx", m_vecAngles.x );
		msg->SetFloat( "anglesy", m_vecAngles.y );
		msg->SetFloat( "anglesz", m_vecAngles.z );
		msg->SetFloat( "forcex", m_vecForce.x );
		msg->SetFloat( "forcey", m_vecForce.y );
		msg->SetFloat( "forcez", m_vecForce.z );
		msg->SetFloat( "forceposx", m_vecForcePos.x );
		msg->SetFloat( "forceposy", m_vecForcePos.y );
		msg->SetFloat( "forceposz", m_vecForcePos.z );
		msg->SetColor( "frontcolor", front );
		msg->SetColor( "backcolor", back );
		msg->SetFloat( "width", m_flWidth );
		msg->SetFloat( "height", m_flHeight );
		msg->SetFloat( "size", m_flShardSize );
		msg->SetInt( "surfacetype", m_nSurfaceType );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEShatterSurface::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEShatterSurface::PostDataUpdate" );

	RecordShatterSurface();

	CSmartPtr<CSimple3DEmitter> pGlassEmitter = CSimple3DEmitter::Create( "C_TEShatterSurface 1" );
	pGlassEmitter->SetSortOrigin( m_vecOrigin );

	Vector vecColor;
	engine->ComputeLighting( m_vecOrigin, NULL, true, vecColor );

	// HACK: Blend a little toward white to match the materials...
	VectorLerp( vecColor, Vector( 1, 1, 1 ), 0.3, vecColor );

	PMaterialHandle *hMaterial;
	if (m_nSurfaceType == SHATTERSURFACE_GLASS)
	{
		hMaterial = g_Mat_Fleck_Glass;
	}
	else
	{
		hMaterial = g_Mat_Fleck_Tile;
	}

	// ---------------------------------------------------
	// Figure out number of particles required to fill space
	// ---------------------------------------------------
	int nNumWide = m_flWidth  / m_flShardSize;
	int nNumHigh = m_flHeight / m_flShardSize;

	Vector vWidthStep,vHeightStep;
	AngleVectors(m_vecAngles,NULL,&vWidthStep,&vHeightStep);
	vWidthStep	*= m_flShardSize;
	vHeightStep *= m_flShardSize;

	// ---------------------
	// Create glass shards
	// ----------------------
	Vector vCurPos = m_vecOrigin;
	vCurPos.x += 0.5*m_flShardSize;
	vCurPos.z += 0.5*m_flShardSize;

	float flMinSpeed = 9999999999.0f;
	float flMaxSpeed = 0;

	Particle3D *pParticle = NULL;

	for (int width=0;width<nNumWide;width++)
	{
		for (int height=0;height<nNumHigh;height++)
		{			
			pParticle = (Particle3D *) pGlassEmitter->AddParticle( sizeof(Particle3D), hMaterial[random_valve->RandomInt(0,1)], vCurPos );

			Vector vForceVel = Vector(0,0,0);
			if (random_valve->RandomInt(0, 3) != 0)
			{
				float flForceDistSqr = (vCurPos - m_vecForcePos).LengthSqr();
				vForceVel = m_vecForce;
				if (flForceDistSqr > 0 )
				{
					vForceVel *= ( 40.0f / flForceDistSqr );
				}
			}

			// cap the Z velocity of the shards
			if (fx_glass_velocity_cap.GetFloat() > 0 && vForceVel.z < -fx_glass_velocity_cap.GetFloat())
			{
				vForceVel.z = random_valve->RandomFloat(-fx_glass_velocity_cap.GetFloat(), -(fx_glass_velocity_cap.GetFloat() * 0.66f));
			}

			if (pParticle)
			{
				pParticle->m_flLifeRemaining	= random_valve->RandomFloat(GLASS_SHARD_MIN_LIFE,GLASS_SHARD_MAX_LIFE);
				pParticle->m_vecVelocity		= vForceVel;
				pParticle->m_vecVelocity	   += RandomVector(-25,25);
				pParticle->m_uchSize			= m_flShardSize + random_valve->RandomFloat(-0.5*m_flShardSize,0.5*m_flShardSize);
				pParticle->m_vAngles			= m_vecAngles;
				pParticle->m_flAngSpeed			= random_valve->RandomFloat(-400,400);

				unsigned char fr	= (byte)(m_uchFrontColor.r() * vecColor.x );
				unsigned char fg	= (byte)(m_uchFrontColor.g() * vecColor.y );
				unsigned char fb	= (byte)(m_uchFrontColor.b() * vecColor.z );
				pParticle->m_uchFrontColor.SetColor( fr, fg, fb );
				unsigned char br	= (byte)(m_uchBackColor.r() * vecColor.x );
				unsigned char bg	= (byte)(m_uchBackColor.g() * vecColor.y );
				unsigned char bb	= (byte)(m_uchBackColor.b() * vecColor.z );
				pParticle->m_uchBackColor.SetColor( br, bg, bb );
			}

			// Keep track of min and max speed for collision detection
			float  flForceSpeed = vForceVel.Length();
			if (flForceSpeed > flMaxSpeed)
			{
				flMaxSpeed = flForceSpeed;
			}
			if (flForceSpeed < flMinSpeed)
			{
				flMinSpeed = flForceSpeed;
			}

			vCurPos += vHeightStep;
		}
		vCurPos	 -= nNumHigh*vHeightStep;
		vCurPos	 += vWidthStep;
	}

	// --------------------------------------------------
	// Set collision parameters
	// --------------------------------------------------
	Vector vMoveDir = m_vecForce;
	VectorNormalize(vMoveDir);

	pGlassEmitter->m_ParticleCollision.Setup( m_vecOrigin, &vMoveDir, GLASS_SHARD_NOISE, 
												flMinSpeed, flMaxSpeed, GLASS_SHARD_GRAVITY, GLASS_SHARD_DAMPING );
}

void TE_ShatterSurface( IRecipientFilter& filter, float delay,
	const Vector* pos, const QAngle* angle, const Vector* vForce, const Vector* vForcePos, 
	float width, float height, float shardsize, ShatterSurface_t surfacetype,
	color24 front_clr, color24 back_clr)
{
	// Major hack to simulate receiving network message
	__g_C_TEShatterSurface.m_vecOrigin = *pos;
	__g_C_TEShatterSurface.m_vecAngles = *angle;
	__g_C_TEShatterSurface.m_vecForce = *vForce;
	__g_C_TEShatterSurface.m_vecForcePos = *vForcePos;
	__g_C_TEShatterSurface.m_flWidth = width;
	__g_C_TEShatterSurface.m_flHeight = height;
	__g_C_TEShatterSurface.m_flShardSize = shardsize;
	__g_C_TEShatterSurface.m_nSurfaceType = surfacetype;
	__g_C_TEShatterSurface.m_uchFrontColor = front_clr;
	__g_C_TEShatterSurface.m_uchBackColor = back_clr;

	__g_C_TEShatterSurface.PostDataUpdate( DATA_UPDATE_CREATED );
}

void TE_ShatterSurface( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecForce, vecForcePos;
	QAngle angles;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	angles.x = pKeyValues->GetFloat( "anglesx" );
	angles.y = pKeyValues->GetFloat( "anglesy" );
	angles.z = pKeyValues->GetFloat( "anglesz" );
	vecForce.x = pKeyValues->GetFloat( "forcex" );
	vecForce.y = pKeyValues->GetFloat( "forcey" );
	vecForce.z = pKeyValues->GetFloat( "forcez" );
	vecForcePos.x = pKeyValues->GetFloat( "forceposx" );
	vecForcePos.y = pKeyValues->GetFloat( "forceposy" );
	vecForcePos.z = pKeyValues->GetFloat( "forceposz" );
	Color front = pKeyValues->GetColor( "frontcolor" );
	Color back = pKeyValues->GetColor( "backcolor" );
	float flWidth = pKeyValues->GetFloat( "width" );
	float flHeight = pKeyValues->GetFloat( "height" );
	float flSize = pKeyValues->GetFloat( "size" );
	ShatterSurface_t nSurfaceType = (ShatterSurface_t)pKeyValues->GetInt( "surfacetype" );
	TE_ShatterSurface( filter, 0.0f, &vecOrigin, &angles, &vecForce, &vecForcePos,
		flWidth, flHeight, flSize, nSurfaceType, front,
		back );
}
