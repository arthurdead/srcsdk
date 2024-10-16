//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "particlemgr.h"
#include "particles_new.h"
#include "iclientmode.h"
#include "engine/ivdebugoverlay.h"
#include "particle_property.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "tier1/KeyValues.h"
#include "model_types.h"
#include "vprof.h"
#include "input.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ConVar cl_particleeffect_aabb_buffer;

//extern ConVar cl_particle_show_bbox;
//extern ConVar cl_particle_show_bbox_cost;
extern bool g_cl_particle_show_bbox;
extern int g_cl_particle_show_bbox_cost;


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CNewParticleEffect::CNewParticleEffect( C_BaseEntity *pOwner, CParticleSystemDefinition *pEffect )
{
	m_hOwner = pOwner;
	Init( pEffect );
	Construct();
}

CNewParticleEffect::CNewParticleEffect( C_BaseEntity *pOwner, const char* pEffectName )
{
	m_hOwner = pOwner;
	Init( pEffectName );
	Construct();
}

static ConVar cl_aggregate_particles( "cl_aggregate_particles", "1" );

void CNewParticleEffect::Construct()
{
	m_vSortOrigin.Init();

	m_bDontRemove = false;
	m_bRemove = false;
	m_bDrawn = false;
	m_bNeedsBBoxUpdate = false;
	m_bIsFirstFrame = true;
	m_bAutoUpdateBBox = false;
	m_bAllocated = true;
	m_bSimulate = true;
	m_bRecord = false;
	m_bShouldPerformCullCheck = false;
	m_bDisableAggregation = true;							// will be reset when someone creates it via CreateOrAggregate

	m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID;
	m_RefCount = 0;
	ParticleMgr()->AddEffect( this );
	m_LastMax = Vector( -1.0e6, -1.0e6, -1.0e6 );
	m_LastMin = Vector( 1.0e6, 1.0e6, 1.0e6 );
	m_MinBounds = Vector( 1.0e6, 1.0e6, 1.0e6 );
	m_MaxBounds = Vector( -1.0e6, -1.0e6, -1.0e6 );
	m_pDebugName = NULL;

	m_bViewModelEffect = m_pDef ? m_pDef->IsViewModelEffect() : false;

	RecordCreation();
}

CNewParticleEffect::~CNewParticleEffect(void)
{
	if ( m_bRecord && m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemDestroyedState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;

		KeyValues *msg = new KeyValues( "ParticleSystem_Destroy" );
		msg->SetPtr( "state", &state );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		m_nToolParticleEffectId = TOOLPARTICLESYSTEMID_INVALID; 
	}

	m_bAllocated = false;
	if ( m_hOwner )
	{
		// NOTE: This can provoke another NotifyRemove call which is why flags is set to 0
		m_hOwner->ParticleProp()->OnParticleSystemDeleted( this );
	}
}


//-----------------------------------------------------------------------------
// Refcounting
//-----------------------------------------------------------------------------
void CNewParticleEffect::AddRef()
{
	++m_RefCount;
}

void CNewParticleEffect::Release()
{
	Assert( m_RefCount > 0 );
	--m_RefCount;

	// If all the particles are already gone, delete ourselves now.
	// If there are still particles, wait for the last NotifyDestroyParticle.
	if ( m_RefCount == 0 )
	{
		if ( m_bAllocated )
		{
			if ( IsFinished() )
			{
				SetRemoveFlag();
			}
		}
	}
}

void CNewParticleEffect::NotifyRemove()
{
	if ( m_bAllocated )
	{
		delete this;
	}
}

int CNewParticleEffect::IsReleased()
{
	return m_RefCount == 0;
}


//-----------------------------------------------------------------------------
// Refraction and soft particle support
//-----------------------------------------------------------------------------
RenderableTranslucencyType_t CNewParticleEffect::ComputeTranslucencyType( void )
{
	if( CParticleCollection::IsTwoPass() )
		return RENDERABLE_IS_TWO_PASS;

	if( CParticleCollection::IsTranslucent() )
		return RENDERABLE_IS_TRANSLUCENT;

	return RENDERABLE_IS_OPAQUE;
}

int CNewParticleEffect::GetRenderFlags( void )
{
	int nFlags = 0;

	if( CParticleCollection::UsesPowerOfTwoFrameBufferTexture( true ) )
		nFlags |= ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB;

	if( CParticleCollection::UsesFullFrameBufferTexture( true ) )
		nFlags |= ERENDERFLAGS_NEEDS_FULL_FB;

	return nFlags;
}

//-----------------------------------------------------------------------------
// Overrides for recording
//-----------------------------------------------------------------------------
void CNewParticleEffect::StopEmission( bool bInfiniteOnly, bool bRemoveAllParticles, bool bWakeOnStop, bool bPlayEndCap )
{
	if ( m_bRecord && m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "ParticleSystem_StopEmission" );

		static ParticleSystemStopEmissionState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_bInfiniteOnly = bInfiniteOnly;

		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	CParticleCollection::StopEmission( bInfiniteOnly, bRemoveAllParticles, bWakeOnStop );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNewParticleEffect::SetDormant( bool bDormant )
{
	CParticleCollection::SetDormant( bDormant );
}

void CNewParticleEffect::SetControlPointEntity( int nWhichPoint, C_BaseEntity *pEntity )
{
	if ( m_bRecord && m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemSetControlPointObjectState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		state.m_nObject = pEntity ? pEntity->entindex() : -1;

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointObject" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	if ( pEntity )
	{
		CParticleCollection::SetControlPointObject( nWhichPoint, &m_hControlPointOwners[ nWhichPoint ] );
		m_hControlPointOwners[ nWhichPoint ] = pEntity;
	}
	else
		CParticleCollection::SetControlPointObject( nWhichPoint, NULL );
}


void CNewParticleEffect::SetControlPoint( int nWhichPoint, const Vector &v )
{
	if ( m_bRecord && m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		static ParticleSystemSetControlPointPositionState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		state.m_vecPosition = v;

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointPosition" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}

	CParticleCollection::SetControlPoint( nWhichPoint, v );
}


void CNewParticleEffect::RecordControlPointOrientation( int nWhichPoint )
{
	if ( m_bRecord && m_nToolParticleEffectId != TOOLPARTICLESYSTEMID_INVALID && clienttools->IsInRecordingMode() )
	{
		// FIXME: Make a more direct way of getting 
		QAngle angles;
		VectorAngles( m_ControlPoints[nWhichPoint].m_ForwardVector, m_ControlPoints[nWhichPoint].m_UpVector, angles );

		static ParticleSystemSetControlPointOrientationState_t state;
		state.m_nParticleSystemId = GetToolParticleEffectId();
		state.m_flTime = gpGlobals->curtime;
		state.m_nControlPoint = nWhichPoint;
		AngleQuaternion( angles, state.m_qOrientation );

		KeyValues *msg = new KeyValues( "ParticleSystem_SetControlPointOrientation" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}
}

void CNewParticleEffect::SetToolRecording( bool bRecord )
{
	if ( bRecord == m_bRecord )
		return;

	m_bRecord = bRecord;

	if ( m_bRecord )
	{
		RecordCreation();
	}
}

void CNewParticleEffect::RecordCreation()
{
	if ( IsValid() && clienttools->IsInRecordingMode() )
	{
		m_bRecord = true;

		int nId = AllocateToolParticleEffectId();	

		static ParticleSystemCreatedState_t state;
		state.m_nParticleSystemId = nId;
		state.m_flTime = gpGlobals->curtime;
		state.m_pName = m_pDef->GetName();
		state.m_nOwner = m_hOwner ? m_hOwner->entindex() : -1;

		KeyValues *msg = new KeyValues( "ParticleSystem_Create" );
		msg->SetPtr( "state", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
	}
}

void CNewParticleEffect::SetControlPointOrientation( int nWhichPoint, 
	const Vector &forward, const Vector &right, const Vector &up )
{
	CParticleCollection::SetControlPointOrientation( nWhichPoint, forward, right, up );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointOrientation( int nWhichPoint, const Quaternion &q )
{
	CParticleCollection::SetControlPointOrientation( nWhichPoint, q );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointForwardVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointForwardVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointUpVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointUpVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}

void CNewParticleEffect::SetControlPointRightVector( int nWhichPoint, const Vector &v )
{
	CParticleCollection::SetControlPointRightVector( nWhichPoint, v );
	RecordControlPointOrientation( nWhichPoint );
}


//-----------------------------------------------------------------------------
// Called when the particle effect is about to update
//-----------------------------------------------------------------------------
void CNewParticleEffect::Update( float flTimeDelta )
{
	if ( m_hOwner )
	{
		m_hOwner->ParticleProp()->OnParticleSystemUpdated( this, flTimeDelta );
	}
}


//-----------------------------------------------------------------------------
// Bounding box
//-----------------------------------------------------------------------------
CNewParticleEffect* CNewParticleEffect::ReplaceWith( const char *pParticleSystemName )
{
	StopEmission( false, true, true );
	if ( !pParticleSystemName || !pParticleSystemName[0] )
		return NULL;

	CSmartPtr< CNewParticleEffect > pNewEffect = CNewParticleEffect::Create( GetOwner(), pParticleSystemName, pParticleSystemName );
	if ( !pNewEffect->IsValid() )
		return NULL;

	// Copy over the control point data
	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !ReadsControlPoint( i ) )
			continue;

		Vector vecForward, vecRight, vecUp;
		pNewEffect->SetControlPoint( i, GetControlPointAtCurrentTime( i ) );
		GetControlPointOrientationAtCurrentTime( i, &vecForward, &vecRight, &vecUp );
		pNewEffect->SetControlPointOrientation( i, vecForward, vecRight, vecUp );
		pNewEffect->SetControlPointParent( i, GetControlPointParent( i ) );
	}

	if ( m_hOwner )
	{
		m_hOwner->ParticleProp()->ReplaceParticleEffect( this, pNewEffect.GetObject() );
	}

	// fixup any other references to the old system, to point to the new system
	while( m_References.m_pHead )
	{
		// this will remove the reference from m_References
		m_References.m_pHead->Set( pNewEffect.GetObject() );
	}

	// At this point any references should have been redirected,
	// but we may still be running with some stray particles, so we
	// might not be flagged for removal - force the issue!
	Assert( m_RefCount == 0 );
	SetRemoveFlag();

	return pNewEffect.GetObject();
}


//-----------------------------------------------------------------------------
// Bounding box
//-----------------------------------------------------------------------------
void CNewParticleEffect::SetParticleCullRadius( float radius )
{
}

bool CNewParticleEffect::RecalculateBoundingBox()
{
	BloatBoundsUsingControlPoint();
	if ( !m_bBoundsValid )
	{
		m_MaxBounds = m_MinBounds = GetRenderOrigin();
		return false;
	}

	return true;
}


void CNewParticleEffect::GetRenderBounds( Vector& mins, Vector& maxs )
{
	if ( !m_bBoundsValid )
	{
		mins = vec3_origin;
		maxs = mins;
		return;
	}
	VectorSubtract( m_MinBounds, GetRenderOrigin(), mins );
	VectorSubtract( m_MaxBounds, GetRenderOrigin(), maxs );
}

void CNewParticleEffect::DetectChanges()
{
	// if we have no render handle, return
	if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	// Turn off rendering if the bounds aren't valid
	ClientLeafSystem()->EnableRendering( m_hRenderHandle, m_bBoundsValid );

	if ( !m_bBoundsValid )
	{
		m_LastMin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		m_LastMin.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		return;
	}

	float flBuffer = cl_particleeffect_aabb_buffer.GetFloat();
	float flExtraBuffer = flBuffer * 1.3f;

	// if nothing changed, return
	if ( m_MinBounds.x < m_LastMin.x || 
		 m_MinBounds.y < m_LastMin.y || 
		 m_MinBounds.z < m_LastMin.z || 

		 m_MinBounds.x > (m_LastMin.x + flExtraBuffer) ||
		 m_MinBounds.y > (m_LastMin.y + flExtraBuffer) ||
		 m_MinBounds.z > (m_LastMin.z + flExtraBuffer) ||

		 m_MaxBounds.x > m_LastMax.x || 
		 m_MaxBounds.y > m_LastMax.y || 
		 m_MaxBounds.z > m_LastMax.z || 

		 m_MaxBounds.x < (m_LastMax.x - flExtraBuffer) ||
		 m_MaxBounds.y < (m_LastMax.y - flExtraBuffer) ||
		 m_MaxBounds.z < (m_LastMax.z - flExtraBuffer)
		 )
	{
		// call leafsystem to updated this guy
		ClientLeafSystem()->RenderableChanged( m_hRenderHandle );

		// remember last parameters
		// Add some padding in here so we don't reinsert it into the leaf system if it just changes a tiny amount.
		m_LastMin = m_MinBounds - Vector( flBuffer, flBuffer, flBuffer );
		m_LastMax = m_MaxBounds + Vector( flBuffer, flBuffer, flBuffer );
	}
}

extern ConVar r_DrawParticles;


void CNewParticleEffect::DebugDrawBbox ( bool bCulled )
{
	int nParticlesShowBboxCost = g_cl_particle_show_bbox_cost;
	bool bShowCheapSystems = false;
	if ( nParticlesShowBboxCost < 0 )
	{
		nParticlesShowBboxCost = -nParticlesShowBboxCost;
		bShowCheapSystems = true;
	}

	Vector center = GetRenderOrigin();
	Vector mins   = m_MinBounds - center;
	Vector maxs   = m_MaxBounds - center;
	
	int r, g, b;
	bool bDraw = true;
	if ( bCulled )
	{
		r = 64;
		g = 64;
		b = 64;
	}
	else if ( nParticlesShowBboxCost > 0 )
	{
		float fAmount = (float)m_nActiveParticles / (float)nParticlesShowBboxCost;
		if ( fAmount < 0.5f )
		{
			if ( bShowCheapSystems )
			{
				r = 0;
				g = 255;
				b = 0;
			}
			else
			{
				// Prevent the screen getting spammed with low-count particles which aren't that expensive.
				bDraw = false;
				r = 0;
				g = 0;
				b = 0;
			}
		}
		else if ( fAmount < 1.0f )
		{
			// green 0.5-1.0 blue
			int nBlend = (int)( 512.0f * ( fAmount - 0.5f ) );
			nBlend = MIN ( 255, MAX ( 0, nBlend ) );
			r = 0;
			g = 255 - nBlend;
			b = nBlend;
		}
		else if ( fAmount < 2.0f )
		{
			// blue 1.0-2.0 red
			int nBlend = (int)( 256.0f * ( fAmount - 1.0f ) );
			nBlend = MIN ( 255, MAX ( 0, nBlend ) );
			r = nBlend;
			g = 0;
			b = 255 - nBlend;
		}
		else
		{
			r = 255;
			g = 0;
			b = 0;
		}
	}
	else
	{
		if ( GetAutoUpdateBBox() )
		{
			// red is bad, the bbox update is costly
			r = 255;
			g = 0;
			b = 0;
		}
		else
		{
			// green, this effect presents less cpu load 
			r = 0;
			g = 255;
			b = 0;
		}
	}
		 
	if ( bDraw )
	{
		debugoverlay->AddBoxOverlay( center, mins, maxs, QAngle( 0, 0, 0 ), r, g, b, 16, 0 );
		debugoverlay->AddTextOverlayRGB( center, 0, 0, r, g, b, 64, "%s:(%d)", GetEffectName(), m_nActiveParticles );
	}
}


//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------
int CNewParticleEffect::DrawModel( int flags, const RenderableInstance_t &instance )
{
	VPROF_BUDGET( "CNewParticleEffect::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	if ( r_DrawParticles.GetBool() == false )
		return 0;

	if ( !GetClientMode()->ShouldDrawParticles() || !ParticleMgr()->ShouldRenderParticleSystems() )
		return 0;
	
	if ( ( flags & ( STUDIO_SHADOWDEPTHTEXTURE | STUDIO_SSAODEPTHTEXTURE ) ) != 0 )
	{
		return 0;
	}

	if ( m_hOwner && m_hOwner->IsDormant() )
		return 0;
	
	// do distance cull check here. We do it here instead of in particles so we can easily only do
	// it for root objects, not bothering to cull children individually
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if ( !m_pDef->IsViewModelEffect() )
	{
		Vector vecCamera;
		pRenderContext->GetWorldSpaceCameraPosition( &vecCamera );
		if ( CalcSqrDistanceToAABB( m_MinBounds, m_MaxBounds, vecCamera ) > ( m_pDef->m_flMaxDrawDistance * m_pDef->m_flMaxDrawDistance ) )
		{
		#ifndef _RETAIL
			if ( g_cl_particle_show_bbox || ( g_cl_particle_show_bbox_cost != 0 ) )
			{
				DebugDrawBbox ( true );
			}
		#endif

			// Still need to make sure we set this or they won't follow their attachemnt points.
			m_flNextSleepTime = Max ( m_flNextSleepTime, ( g_pParticleSystemMgr->GetLastSimulationTime() + m_pDef->m_flNoDrawTimeToGoToSleep ));

			return 0;
		}
	}

	if ( ( flags & STUDIO_TRANSPARENCY ) || !IsBatchable() )
	{
		int viewentity = render->GetViewEntity();
		C_BaseEntity *pCameraObject = cl_entitylist->GetEnt( viewentity );
		// apply logic that lets you skip rendering a system if the camera is attached to its entity
		if ( pCameraObject &&
			 ( m_pDef->m_nSkipRenderControlPoint != -1 ) &&
			 ( m_pDef->m_nSkipRenderControlPoint <= m_nHighestCP ) )
		{
			C_BaseEntity *pEntity = (EHANDLE)GetControlPointEntity( m_pDef->m_nSkipRenderControlPoint );
			if ( pEntity )
			{
				// If we're in thirdperson, we still see it
				if ( !input->CAM_IsThirdPerson() )
				{
					if ( pEntity == pCameraObject )
						return 0;
					C_BaseEntity *pRootMove = pEntity->GetRootMoveParent();
					if ( pRootMove == pCameraObject )
						return 0;

					// If we're spectating in-eyes of the camera object, we don't see it
					C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
					if ( pPlayer == pCameraObject )
					{
						C_BaseEntity *pObTarget = pPlayer->GetObserverTarget();
						if ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE && (pObTarget == pEntity || pRootMove == pObTarget ) )
							return 0;
					}
				}
			}
		}

		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PushMatrix();
		pRenderContext->LoadIdentity();
		Render( pRenderContext, ( flags & STUDIO_TRANSPARENCY ) ? CParticleCollection::IsTwoPass() : false, pCameraObject );
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PopMatrix();
	}
	else
	{
		g_pParticleSystemMgr->AddToRenderCache( this );
	}

#ifndef _RETAIL
	CParticleMgr *pMgr = ParticleMgr();
	if ( pMgr->m_bStatsRunning )
	{
		pMgr->StatsNewParticleEffectDrawn ( this );
	}

	if ( g_cl_particle_show_bbox || ( g_cl_particle_show_bbox_cost != 0 ) )
	{
		DebugDrawBbox ( false );
	}
#endif

	return 1;
}

bool CNewParticleEffect::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	matrix3x4_t mat;
	mat.Init( Vector( 0, -1, 0), Vector( 1, 0, 0), Vector( 0, 0, 1 ), vec3_origin );
	MatrixMultiply( m_DrawModelMatrix, mat, pBoneToWorldOut[0] );
	return true;
}

static void DumpParticleStats_f( void )
{
	g_pParticleSystemMgr->DumpProfileInformation();
}

static ConCommand cl_dump_particle_stats( "cl_dump_particle_stats", DumpParticleStats_f, "dump particle profiling info to particle_profile.csv") ;

