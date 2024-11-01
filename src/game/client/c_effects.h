//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_EFFECTS_H
#define C_EFFECTS_H
#pragma once

#include "env_wind_shared.h"
#include "precipitation_shared.h"
#include "particles_simple.h"
#include "c_baseentity.h"

//-----------------------------------------------------------------------------
// Precipitation particle type
//-----------------------------------------------------------------------------

class CPrecipitationParticle
{
public:
	Vector	m_Pos;
	Vector	m_Velocity;
	float	m_SpawnTime;				// Note: Tweak with this to change lifetime
	float	m_Mass;
	float	m_Ramp;
	
	float	m_flCurLifetime;
	float	m_flMaxLifetime;
};

class CClient_Precipitation;

//===========
// Snow fall
//===========
class SnowFallEffect : public CSimpleEmitter
{
public:

	SnowFallEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	static CSmartPtr<SnowFallEffect> Create( const char *pDebugName );

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta ) OVERRIDE;

	void SimulateParticles( CParticleSimulateIterator *pIterator ) OVERRIDE;

	int	GetParticleCount( void );

	void SetBounds( const Vector &vecMin, const Vector &vecMax );

	RenderableTranslucencyType_t ComputeTranslucencyType( void ) { return RENDERABLE_IS_OPAQUE; }

private:

	SnowFallEffect( const SnowFallEffect & );
};

class CSnowFallManager : public C_ClientOnlyLogicalEntity
{
public:
	DECLARE_CLASS(CSnowFallManager, C_ClientOnlyLogicalEntity)

	CSnowFallManager();
	~CSnowFallManager();

	bool CreateEmitter( void );

	void Spawn( void ) OVERRIDE;
	void Think() OVERRIDE;

	void AddSnowFallEntity( CClient_Precipitation *pSnowEntity );

	// Snow Effect
	enum
	{
		SNOWFALL_NONE = 0,
		SNOWFALL_AROUND_PLAYER,
		SNOWFALL_IN_ENTITY,
	};

	RenderableTranslucencyType_t ComputeTranslucencyType( void ) { return RENDERABLE_IS_OPAQUE; }

private:

	bool CreateSnowFallEmitter( void );
	void CreateSnowFall( void );
	void CreateSnowFallParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale, C_BasePlayer *pLocalPlayer );
	void CreateOutsideVolumeSnowParticles( float flCurrentTime, float flRadius, float flZoomScale, C_BasePlayer *pLocalPlayer );
	void CreateInsideVolumeSnowParticles( float flCurrentTime, float flRadius, const Vector &vecEyePos, const Vector &vecForward, float flZoomScale, C_BasePlayer *pLocalPlayer );
	void CreateSnowParticlesSphere( float flRadius, C_BasePlayer *pLocalPlayer );
	void CreateSnowParticlesRay( float flRadius, const Vector &vecEyePos, const Vector &vecForward, C_BasePlayer *pLocalPlayer );
	void CreateSnowFallParticle( const Vector &vecParticleSpawn, int iBBox, C_BasePlayer *pLocalPlayer );

	int StandingInSnowVolume( const Vector &vecPoint );
	void FindSnowVolumes( const Vector &vecCenter, float flRadius, const Vector &vecEyePos, const Vector &vecForward );

	void UpdateBounds( const Vector &vecSnowMin, const Vector &vecSnowMax );

private:

	enum { MAX_SNOW_PARTICLES = 500 };
	enum { MAX_SNOW_LIST = 32 };

	TimedEvent						m_tSnowFallParticleTimer;
	TimedEvent						m_tSnowFallParticleTraceTimer;

	int								m_iSnowFallArea;
	CSmartPtr<SnowFallEffect>		m_pSnowFallEmitter;
	Vector							m_vecSnowFallEmitOrigin;
	float							m_flSnowRadius;

	Vector							m_vecMin;
	Vector							m_vecMax;

	int								m_nActiveSnowCount;
	int								m_aActiveSnow[MAX_SNOW_LIST];

	bool							m_bRayParticles;

	struct SnowFall_t
	{
		PMaterialHandle			m_hMaterial;
		CClient_Precipitation	*m_pEntity;
		CSmartPtr<SnowFallEffect> m_pEffect;
		Vector					m_vecMin;
		Vector					m_vecMax;
	};

	CUtlVector<SnowFall_t>		m_aSnow;
};

extern CSnowFallManager *s_pSnowFallMgr;
extern bool SnowFallManagerCreate( CClient_Precipitation *pSnowEntity );
extern void SnowFallManagerDestroy( void );

class AshDebrisEffect : public CSimpleEmitter
{
public:
	AshDebrisEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static CSmartPtr<AshDebrisEffect> Create( const char *pDebugName );

	virtual float UpdateAlpha( const SimpleParticle *pParticle );
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta );

private:
	AshDebrisEffect( const AshDebrisEffect & );
};

//-----------------------------------------------------------------------------
// Precipitation blocker entity
//-----------------------------------------------------------------------------
class C_PrecipitationBlocker : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PrecipitationBlocker, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_PrecipitationBlocker();
	virtual ~C_PrecipitationBlocker();
};

//-----------------------------------------------------------------------------
// Precipitation base entity
//-----------------------------------------------------------------------------

class CClient_Precipitation : public C_BaseEntity
{
class CPrecipitationEffect;
friend class CClient_Precipitation::CPrecipitationEffect;

public:
	DECLARE_CLASS( CClient_Precipitation, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	CClient_Precipitation() : CClient_Precipitation( 0 ) {}
	CClient_Precipitation( int iEFlags );
	virtual ~CClient_Precipitation();

	// Inherited from C_BaseEntity
	virtual void Precache( );
	virtual RenderableTranslucencyType_t ComputeTranslucencyType() { return RENDERABLE_IS_TRANSLUCENT; }

	void Render();

private:

	// Creates a single particle
	CPrecipitationParticle* CreateParticle();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void Think();

	void Simulate( float dt );

	// Renders the particle
	void RenderParticle( CPrecipitationParticle* pParticle, CMeshBuilder &mb ) const;

	void CreateWaterSplashes();

	// Emits the actual particles
	void EmitParticles( float fTimeDelta );
	
	// Computes where we're gonna emit
	bool ComputeEmissionArea( Vector& origin, Vector2D& size, C_BaseCombatCharacter *pCharacter ) const;

	// Gets the tracer width and speed
	float GetWidth() const;
	float GetLength() const;
	float GetSpeed() const;

	// Gets the remaining lifetime of the particle
	float GetRemainingLifetime( CPrecipitationParticle* pParticle ) const;

	// Computes the wind vector
	static void ComputeWindVector( );

	// simulation methods
	bool SimulateRain( CPrecipitationParticle* pParticle, float dt );
	bool SimulateSnow( CPrecipitationParticle* pParticle, float dt ) const;

	void CreateAshParticle( void );
	void CreateRainOrSnowParticle( const Vector &vSpawnPosition, const Vector &vEndPosition, const Vector &vVelocity );	// TERROR: adding end pos for lifetime calcs

	void CreateParticlePrecip( void );
	void InitializeParticlePrecip( void );
	void DispatchOuterParticlePrecip( C_BasePlayer *pPlayer, const Vector& vForward );
	void DispatchInnerParticlePrecip( C_BasePlayer *pPlayer, const Vector& vForward );
	void DestroyOuterParticlePrecip();
	void DestroyInnerParticlePrecip();

	void UpdateParticlePrecip( C_BasePlayer *pPlayer );
	float GetDensity() const { return m_flDensity; }

	// Information helpful in creating and rendering particles
	IMaterial		*m_MatHandle;	// material used 

	float			m_Color[4];		// precip color
	float			m_Lifetime;		// Precip lifetime
	float			m_InitialRamp;	// Initial ramp value
	float			m_Speed;		// Precip speed
	float			m_Width;		// Tracer width
	float			m_Remainder;	// particles we should render next time
	PrecipitationType_t	m_nPrecipType;			// Precip type
	float			m_flHalfScreenWidth;	// Precalculated each frame.

	float			m_flDensity;

	int m_nSnowDustAmount;

	// Some state used in rendering and simulation
	// Used to modify the rain density and wind from the console
	static ConVar s_raindensity;
	static ConVar s_rainwidth;
	static ConVar s_rainlength;
	static ConVar s_rainspeed;

	static Vector s_WindVector;			// Stores the wind speed vector
	
	CUtlLinkedList<CPrecipitationParticle> m_Particles;
	CUtlVector<Vector> m_Splashes;

	CSmartPtr<AshDebrisEffect>		m_pAshEmitter;
	TimedEvent						m_tAshParticleTimer;
	TimedEvent						m_tAshParticleTraceTimer;
	bool							m_bActiveAshEmitter;
	Vector							m_vAshSpawnOrigin;

	int								m_iAshCount;

	float							m_flParticleInnerDist;	//The distance at which to start drawing the inner system
	const char						*m_pParticleInnerNearDef; //Name of the first inner system
	const char						*m_pParticleInnerFarDef;  //Name of the second inner system
	const char						*m_pParticleOuterDef;     //Name of the outer system
	HPARTICLEFFECT					m_pParticlePrecipInnerNear;
	HPARTICLEFFECT					m_pParticlePrecipInnerFar;
	HPARTICLEFFECT					m_pParticlePrecipOuter;
	TimedEvent						m_tParticlePrecipTraceTimer;
	bool							m_bActiveParticlePrecipEmitter;
	bool							m_bParticlePrecipInitialized;

private:
	CClient_Precipitation( const CClient_Precipitation & ); // not defined, not accessible
};

extern CUtlVector<CClient_Precipitation*> g_Precipitations;

//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class C_EnvWind : public C_BaseEntity
{
public:
	C_EnvWind();

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EnvWind, C_BaseEntity );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return false; }

	virtual void	WindThink( );

	const CEnvWindShared& WindShared() const { return m_EnvWindShared; }

private:
	C_EnvWind( const C_EnvWind & );

	CEnvWindShared m_EnvWindShared;
};

// Draw rain effects.
void DrawPrecipitation();


#endif // C_EFFECTS_H
