//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#include "cbase.h"
#include "decals.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "fx.h"
#include "fx_impact.h"
#include "view.h"
#include "datacache/imdlcache.h"
#include "debugoverlay_shared.h"
#include "cdll_util.h"
#include "engine/IStaticPropMgr.h"
#include "c_impact_effects.h"
#include "tier0/vprof.h"
#include "ragdoll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar  r_drawflecks( "r_drawflecks", "1" );
static ConVar  r_impacts_alt_orientation ( "r_impacts_alt_orientation", "1" );
extern ConVar r_drawmodeldecals;

ConVar g_ragdoll_steal_impacts_client( "g_ragdoll_steal_impacts_client", "1", FCVAR_NONE, "Allows clientside death ragdolls to \"steal\" impacts from their source entities. This fixes issues with NPCs dying before decals are applied." );
ConVar g_ragdoll_steal_impacts_server( "g_ragdoll_steal_impacts_server", "1", FCVAR_NONE, "Allows serverside death ragdolls to \"steal\" impacts from their source entities. This fixes issues with NPCs dying before decals are applied." );

ConVar g_ragdoll_client_impact_decals( "g_ragdoll_client_impact_decals", "1", FCVAR_NONE, "Applies decals to clientside ragdolls when they are hit." );

ImpactSoundRouteFn g_pImpactSoundRouteFn = NULL;

//==========================================================================================================================
// RAGDOLL ENUMERATOR
//==========================================================================================================================
CRagdollEnumerator::CRagdollEnumerator( Ray_t& shot, int iDamageType )
{
	m_rayShot = shot;
	m_iDamageType = iDamageType;
	m_pHitEnt = NULL;
}

IterationRetval_t CRagdollEnumerator::EnumElement( IHandleEntity *pHandleEntity )
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
	if ( pEnt == NULL )
		return ITERATION_CONTINUE;

	C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

	// If the ragdoll was created on this tick, then the forces were already applied on the server
	if ( pModel == NULL || WasRagdollCreatedOnCurrentTick( pEnt ) )
		return ITERATION_CONTINUE;

	IPhysicsObject *pPhysicsObject = pModel->VPhysicsGetObject();
	if ( pPhysicsObject == NULL )
		return ITERATION_CONTINUE;

	trace_t tr;
	enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

	if ( tr.fraction < 1.0 )
	{
		pModel->ImpactTrace( &tr, m_iDamageType, NULL );
		m_pHitEnt = pModel;

		//FIXME: Yes?  No?
		return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FX_AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType )
{
	// don't do this when lots of ragdolls are simulating
	if ( s_RagdollLRU.CountRagdolls(true) > 1 )
		return false;
	Ray_t shotRay;
	shotRay.Init( vecStart, vecOrigin );

	CRagdollEnumerator ragdollEnum( shotRay, iDamageType );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );

	return ragdollEnum.Hit();
}

C_BaseAnimating *FX_AffectRagdolls_GetHit( Vector vecOrigin, Vector vecStart, int iDamageType )
{
	// don't do this when lots of ragdolls are simulating
	if ( s_RagdollLRU.CountRagdolls(true) > 1 )
		return NULL;
	Ray_t shotRay;
	shotRay.Init( vecStart, vecOrigin );

	CRagdollEnumerator ragdollEnum( shotRay, iDamageType );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );

	return ragdollEnum.GetHit();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void RagdollImpactCallback( const CEffectData &data )
{
	FX_AffectRagdolls( data.m_vOrigin, data.m_vStart, data.m_nDamageType );
}

DECLARE_CLIENT_EFFECT( RagdollImpact, RagdollImpactCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Impact( const Vector &vecOrigin, const Vector &vecStart, int iMaterial, int iDamageType, int iHitbox, C_BaseEntity *pEntity, trace_t &tr, int nFlags, int maxLODToDecal )
{
	VPROF( "Impact" );

	Assert ( pEntity );

	MDLCACHE_CRITICAL_SECTION();

	// If the entity already has a ragdoll that was created on the current tick, use that ragdoll instead.
	// This allows the killing damage's decals to show up on the ragdoll.
	if (C_BaseAnimating *pAnimating = pEntity->GetBaseAnimating())
	{
		if (pAnimating->m_pClientsideRagdoll && WasRagdollCreatedOnCurrentTick( pAnimating->m_pClientsideRagdoll ) && g_ragdoll_steal_impacts_client.GetBool())
		{
			pEntity = pAnimating->m_pClientsideRagdoll;
		}
		else if (pAnimating->m_pServerRagdoll && WasRagdollCreatedOnCurrentTick( pAnimating->m_pServerRagdoll ) && g_ragdoll_steal_impacts_server.GetBool())
		{
			pEntity = pAnimating->m_pServerRagdoll;
		}
	}

	// Clear out the trace
	memset( (void *)&tr, 0, sizeof(trace_t));
	tr.fraction = 1.0f;

	// Setup our shot information
	Vector shotDir = vecOrigin - vecStart;
	float flLength = VectorNormalize( shotDir );
	Vector traceExt;
	VectorMA( vecStart, flLength + 8.0f, shotDir, traceExt );

	// Attempt to hit ragdolls
	
	bool bHitRagdoll = false;
	
	if ( !pEntity->IsClientCreated() )
	{
		C_BaseAnimating *pRagdoll = FX_AffectRagdolls_GetHit( vecOrigin, vecStart, iDamageType );
		if (pRagdoll)
		{
			bHitRagdoll = true;

			if (g_ragdoll_client_impact_decals.GetBool() && pRagdoll->IsRagdoll())
			{
				pEntity = pRagdoll;

				// HACKHACK: Get the ragdoll's nearest bone for its material
				int iNearestMaterial = 0;
				float flNearestDistSqr = FLT_MAX;

				IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
				int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
				for ( int i = 0; i < count; i++ )
				{
					Vector vecPosition;
					QAngle angAngles;
					pList[i]->GetPosition( &vecPosition, &angAngles );
					float flDistSqr = (vecStart - vecPosition).LengthSqr();
					if (flDistSqr < flNearestDistSqr)
					{
						iNearestMaterial = pList[i]->GetMaterialIndex();
						flNearestDistSqr = flDistSqr;
					}
				}

				// Get the material from the surfaceprop
				surfacedata_t *psurfaceData = physprops->GetSurfaceData( iNearestMaterial );
				iMaterial = psurfaceData->game.material;
			}
		}
	}

	if ( (nFlags & IMPACT_NODECAL) == 0 )
	{
		const char *pchDecalName = GetImpactDecal( pEntity, iMaterial, iDamageType );
		int decalNumber = decalsystem->GetDecalIndexForName( pchDecalName );
		if ( decalNumber == -1 )
			return false;

		bool bSkipDecal = false;

		// Don't show blood decals if we're filtering them out (Pyro Goggles)
		if ( IsLocalPlayerUsingVisionFilterFlags( VISION_FILTER_LOW_VIOLENCE ) )
		{
			if ( V_strstr( pchDecalName, "Flesh" ) )
			{
				bSkipDecal = true;
			}
		}

		if ( !bSkipDecal )
		{
			if ( (pEntity->IsWorld()) && (iHitbox != 0) )
			{
				staticpropmgr->AddDecalToStaticProp( vecStart, traceExt, iHitbox - 1, decalNumber, true, tr );
			}
			else if ( pEntity )
			{
				// Here we deal with decals on entities.
				pEntity->AddDecal( vecStart, traceExt, vecOrigin, iHitbox, decalNumber, true, tr, maxLODToDecal );
			}
		}
	}
	else
	{
		// Perform the trace ourselves
		Ray_t ray;
		ray.Init( vecStart, traceExt );

		if ( (pEntity->IsWorld()) && (iHitbox != 0) )
		{
			// Special case for world entity with hitbox (that's a static prop)
			ICollideable *pCollideable = staticpropmgr->GetStaticPropByIndex( iHitbox - 1 ); 
			enginetrace->ClipRayToCollideable( ray, MASK_SHOT, pCollideable, &tr );
		}
		else
		{
			if ( !pEntity )
				return false;

			enginetrace->ClipRayToEntity( ray, MASK_SHOT, pEntity, &tr );
		}
	}

	// If we found the surface, emit debris flecks
	bool bReportRagdollImpacts = (nFlags & IMPACT_REPORT_RAGDOLL_IMPACTS) != 0;
	if ( ( tr.fraction == 1.0f ) || ( bHitRagdoll && !bReportRagdollImpacts ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, int iDamageType )
{
	char const *decalName;
	if ( !pEntity )
	{
		decalName = "Impact.Concrete";
	}
	else
	{
		decalName = pEntity->DamageDecal( iDamageType, iMaterial );
	}

	// See if we need to offset the decal for material type
	return decalsystem->TranslateDecalForGameMaterial( decalName, iMaterial );
}

//-----------------------------------------------------------------------------
// Purpose: Perform custom effects based on the Decal index
//-----------------------------------------------------------------------------
static ConVar cl_new_impact_effects( "cl_new_impact_effects", "1" );

struct ImpactEffect_t
{
	ImpactEffect_t(int iMaterial, const char *pName, bool bTryCheap)
		: m_iMaterial(iMaterial), m_pName(pName)
	{
		if(bTryCheap) {
			V_strcpy(m_szNameNoFlecks, pName);
			V_strcat(m_szNameNoFlecks, "_noflecks", ARRAYSIZE(m_szNameNoFlecks));

			V_strcpy(m_szNameCheap, pName);
			V_strcat(m_szNameCheap, "_cheap", ARRAYSIZE(m_szNameCheap));
		} else {
			m_szNameNoFlecks[0] = '\0';
			m_szNameCheap[0] = '\0';
		}

		m_bInitalized = false;

		m_bName = false;
		m_bNameNoFlecks = false;
		m_bNameCheap = false;
	}

	const char *m_pMaterialName;
	int m_iMaterial;
	const char *m_pName;
	bool m_bName;
	bool m_bInitalized;

	char m_szNameNoFlecks[64];
	bool m_bNameNoFlecks;
	char m_szNameCheap[64];
	bool m_bNameCheap;
};

static ImpactEffect_t s_pImpactEffect[26+11] = 
{
	{ CHAR_TEX_ALIENINSECT, "impact_antlion",					false },				// CHAR_TEX_ANTLION
	{ CHAR_TEX_BLOODYFLESH, NULL,					false },							// CHAR_TEX_BLOODYFLESH	
	{ CHAR_TEX_CONCRETE, "impact_concrete",	true },		// CHAR_TEX_CONCRETE		
	{ CHAR_TEX_DIRT, "impact_dirt",		true },			// CHAR_TEX_DIRT			
	{ -1, NULL,					false },							// CHAR_TEX_EGGSHELL		
	{ CHAR_TEX_FLESH, NULL,					false },							// CHAR_TEX_FLESH			
	{ CHAR_TEX_GRATE, "impact_metal",		true },			// CHAR_TEX_GRATE			
	{ CHAR_TEX_ALIENFLESH, NULL,					false },							// CHAR_TEX_ALIENFLESH		
	{ CHAR_TEX_CLIP, NULL,					false },							// CHAR_TEX_CLIP			
	{ -1, "impact_grass",		true },			// CHAR_TEX_GRASS		
	{ -1, "impact_mud",			true },			// CHAR_TEX_MUD		
	{ CHAR_TEX_PLASTIC, "impact_plastic",		true },		// CHAR_TEX_PLASTIC		
	{ CHAR_TEX_METAL, "impact_metal",		true },			// CHAR_TEX_METAL			
	{ CHAR_TEX_SAND, "impact_sand",		true },			// CHAR_TEX_SAND			
	{ -1, "impact_leaves",		true },		// CHAR_TEX_LEAVES		
	{ CHAR_TEX_COMPUTER, "impact_computer",	true },		// CHAR_TEX_COMPUTER		
	{ -1, "impact_asphalt",		true },		// CHAR_TEX_ASPHALT		
	{ -1, "impact_brick",		true },			// CHAR_TEX_BRICK		
	{ CHAR_TEX_SLOSH, "impact_wet",			true },			// CHAR_TEX_SLOSH			
	{ CHAR_TEX_TILE, "impact_tile",		true },			// CHAR_TEX_TILE			
	{ -1, "impact_cardboard",	true },		// CHAR_TEX_CARDBOARD		
	{ CHAR_TEX_VENT, "impact_metal",		true },			// CHAR_TEX_VENT			
	{ CHAR_TEX_WOOD, "impact_wood",		true },			// CHAR_TEX_WOOD			
	{ -1, NULL,					false },							// CHAR_TEX_FAKE		
	{ CHAR_TEX_GLASS, "impact_glass",		true },			// CHAR_TEX_GLASS			
	{ -1, "warp_shield_impact",					false },			// CHAR_TEX_WARPSHIELD	

	{ -1, "impact_clay",		true },			// CHAR_TEX_CLAY
	{ -1, "impact_plaster",		true },		// CHAR_TEX_PLASTER	
	{ -1, "impact_rock",		true },			// CHAR_TEX_ROCK		
	{ -1, "impact_rubber",		true },		// CHAR_TEX_RUBBER			
	{ -1, "impact_sheetrock",	true },		// CHAR_TEX_SHEETROCK		
	{ -1, "impact_cloth",		true },			// CHAR_TEX_CLOTH			
	{ -1, "impact_carpet",		true },		// CHAR_TEX_CARPET			
	{ -1, "impact_paper",		true },			// CHAR_TEX_PAPER		
	{ -1, "impact_upholstery",	true },	// CHAR_TEX_UPHOLSTERY				
	{ -1, "impact_puddle",		true },		// CHAR_TEX_PUDDLE
	{ -1, "impact_metal",		true },			// CHAR_TEX_STEAM_PIPE
};

static void SetImpactControlPoint( CNewParticleEffect *pEffect, int nPoint, const Vector &vecImpactPoint, const Vector &vecForward, C_BaseEntity *pEntity )
{
	Vector vecImpactY, vecImpactZ;
	VectorVectors( vecForward, vecImpactY, vecImpactZ ); 
	vecImpactY *= -1.0f;

	pEffect->SetControlPoint( nPoint, vecImpactPoint );
	if ( r_impacts_alt_orientation.GetBool() )
		pEffect->SetControlPointOrientation( nPoint, vecImpactZ, vecImpactY, vecForward );
	else
		pEffect->SetControlPointOrientation( nPoint, vecForward, vecImpactY, vecImpactZ );
	pEffect->SetControlPointEntity( nPoint, pEntity );
}

static bool PerformNewCustomEffects( const Vector &vecOrigin, trace_t &tr, const Vector &shotDir, int iMaterial, int iScale, int nFlags )
{
	bool bNoFlecks = !r_drawflecks.GetBool();
	if ( !bNoFlecks )
	{
		bNoFlecks = ( ( nFlags & FLAGS_CUSTIOM_EFFECTS_NOFLECKS ) != 0  );
	}

	int nOffset;
	if ( iMaterial >= 1 && iMaterial <= 11 )
	{
		nOffset = 'Z' - 'A';
	}
	else
	{
		nOffset = 'A';
	}

	ImpactEffect_t &effect = s_pImpactEffect[ iMaterial - nOffset ];
	if(!effect.m_bInitalized) {
		if(effect.m_pName && effect.m_pName[0] != '\0')
			effect.m_bName = g_pParticleSystemMgr->IsParticleSystemDefined(effect.m_pName);
		if(effect.m_szNameNoFlecks[0] != '\0')
			effect.m_bNameNoFlecks = g_pParticleSystemMgr->IsParticleSystemDefined(effect.m_szNameNoFlecks);
		if(effect.m_szNameCheap[0] != '\0')
			effect.m_bNameCheap = g_pParticleSystemMgr->IsParticleSystemDefined(effect.m_szNameCheap);
		effect.m_bInitalized = true;
	}

	const char *pImpactName = effect.m_pName;
	if ( bNoFlecks )
	{
		if(effect.m_bNameNoFlecks)
			pImpactName = effect.m_szNameNoFlecks;
		else if(effect.m_bNameCheap)
			pImpactName = effect.m_szNameCheap;
		else {
			return false;
		}
	} else if(!effect.m_bName) {
		return false;
	}

	Vector	vecReflect;
	float	flDot = DotProduct( shotDir, tr.plane.normal );
	VectorMA( shotDir, -2.0f * flDot, tr.plane.normal, vecReflect );

	Vector vecShotBackward;
	VectorMultiply( shotDir, -1.0f, vecShotBackward );

	Vector vecImpactPoint = ( tr.fraction != 1.0f ) ? tr.endpos : vecOrigin;
	AssertMsg( VectorsAreEqual( vecOrigin, tr.endpos, 1e-1 ), "Impact decal drawn too far from the surface impacted." );

	if ( !pImpactName || pImpactName[0] == '\0' )
		return false;

	CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pImpactName );
	if ( !pEffect->IsValid() )
		return false;

	SetImpactControlPoint( pEffect.GetObject(), 0, vecImpactPoint, tr.plane.normal, tr.m_pEnt ); 
	SetImpactControlPoint( pEffect.GetObject(), 1, vecImpactPoint, vecReflect,		tr.m_pEnt ); 
	SetImpactControlPoint( pEffect.GetObject(), 2, vecImpactPoint, vecShotBackward,	tr.m_pEnt ); 
	pEffect->SetControlPoint( 3, Vector( iScale, iScale, iScale ) );
	if ( pEffect->m_pDef->ReadsControlPoint( 4 ) )
	{
		Vector vecColor;
		GetColorForSurface( &tr, &vecColor );
		pEffect->SetControlPoint( 4, vecColor );
	}

	return true;
}

void PerformCustomEffects( const Vector &vecOrigin, trace_t &tr, const Vector &shotDir, int iMaterial, int iScale, int nFlags )
{
	// Throw out the effect if any of these are true
	const int noEffectsFlags = (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP);
	if ( tr.surface.flags & noEffectsFlags )
		return;

	if ( cl_new_impact_effects.GetBool() && PerformNewCustomEffects( vecOrigin, tr, shotDir, iMaterial, iScale, nFlags ) )
	{
		return;
	}

	bool bNoFlecks = !r_drawflecks.GetBool();
	if ( !bNoFlecks )
	{
		bNoFlecks = ( ( nFlags & FLAGS_CUSTIOM_EFFECTS_NOFLECKS ) != 0  );
	}

	// Cement and wood have dust and flecks
	if ( ( iMaterial == CHAR_TEX_CONCRETE ) || ( iMaterial == CHAR_TEX_TILE ) )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale, bNoFlecks );
	}
	else if ( iMaterial == CHAR_TEX_WOOD )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale, bNoFlecks );
	}
	else if ( ( iMaterial == CHAR_TEX_DIRT ) || ( iMaterial == CHAR_TEX_SAND ) )
	{
		FX_DustImpact( vecOrigin, &tr, iScale );
	}
	/*else if ( iMaterial == CHAR_TEX_ALIENINSECT )
	{
		FX_AlienInsectImpact( vecOrigin, &tr );
	}
	*/else if ( ( iMaterial == CHAR_TEX_METAL ) || ( iMaterial == CHAR_TEX_VENT ) )
	{
		Vector	reflect;
		float	dot = shotDir.Dot( tr.plane.normal );
		reflect = shotDir + ( tr.plane.normal * ( dot*-2.0f ) );

		reflect[0] += random_valve->RandomFloat( -0.2f, 0.2f );
		reflect[1] += random_valve->RandomFloat( -0.2f, 0.2f );
		reflect[2] += random_valve->RandomFloat( -0.2f, 0.2f );

		FX_MetalSpark( vecOrigin, reflect, tr.plane.normal, iScale );
	}
	else if ( iMaterial == CHAR_TEX_COMPUTER )
	{
		Vector	offset = vecOrigin + ( tr.plane.normal * 1.0f );

		g_pEffects->Sparks( offset );
	}
	/*else if ( iMaterial == CHAR_TEX_WARPSHIELD )
	{
		QAngle vecAngles;
		VectorAngles( -shotDir, vecAngles );
		DispatchParticleEffect( "alien_shield_impact", vecOrigin, vecAngles );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Play a sound for an impact. If tr contains a valid hit, use that. 
//			If not, use the passed in origin & surface.
//-----------------------------------------------------------------------------
void PlayImpactSound( C_BaseEntity *pEntity, trace_t &tr, const Vector &vecServerOrigin, int nServerSurfaceProp )
{
	VPROF( "PlayImpactSound" );
	surfacedata_t *pdata;
	Vector vecOrigin;

	// If the client-side trace hit a different entity than the server, or
	// the server didn't specify a surfaceprop, then use the client-side trace 
	// material if it's valid.
	if ( tr.DidHit() && (pEntity != tr.m_pEnt || nServerSurfaceProp == 0) )
	{
		nServerSurfaceProp = tr.surface.surfaceProps;
	}
	pdata = physprops->GetSurfaceData( nServerSurfaceProp );
	if ( tr.fraction < 1.0 )
	{
		vecOrigin = tr.endpos;
	}
	else
	{
		vecOrigin = vecServerOrigin;
	}

	// Now play the esound
	if ( pdata->sounds.bulletImpact )
	{
		const char *pbulletImpactSoundName = physprops->GetString( pdata->sounds.bulletImpact );
		
		if ( g_pImpactSoundRouteFn )
		{
			g_pImpactSoundRouteFn( pbulletImpactSoundName, vecOrigin );
		}
		else
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, 0, pbulletImpactSoundName, /*pdata->soundhandles.bulletImpact,*/ &vecOrigin );
		}

		return;
	}

#ifdef _DEBUG
	Msg("***ERROR: PlayImpactSound() on a surface with 0 bulletImpactCount!\n");
#endif //_DEBUG
}


void SetImpactSoundRoute( ImpactSoundRouteFn fn )
{
	g_pImpactSoundRouteFn = fn;
}


//-----------------------------------------------------------------------------
// Purpose: Pull the impact data out
// Input  : &data - 
//			*vecOrigin - 
//			*vecAngles - 
//			*iMaterial - 
//			*iDamageType - 
//			*iHitbox - 
//			*iEntIndex - 
//-----------------------------------------------------------------------------
C_BaseEntity *ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, 
	Vector *vecShotDir, short &nSurfaceProp, int &iMaterial, int &iDamageType, int &iHitbox )
{
	C_BaseEntity *pEntity = data.GetEntity( );
	*vecOrigin = data.m_vOrigin;
	*vecStart = data.m_vStart;
	nSurfaceProp = data.m_nSurfaceProp;
	iDamageType = data.m_nDamageType;
	iHitbox = data.m_nHitBox;

	*vecShotDir = (*vecOrigin - *vecStart);
	VectorNormalize( *vecShotDir );

	// Get the material from the surfaceprop
	surfacedata_t *psurfaceData = physprops->GetSurfaceData( data.m_nSurfaceProp );
	iMaterial = psurfaceData->game.material;

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
void ImpactCallback( const CEffectData &data )
{
	VPROF_BUDGET( "ImpactCallback", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox;
	short nSurfaceProp;
	C_BaseEntity *pEntity = ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, nSurfaceProp, iMaterial, iDamageType, iHitbox );

	if ( !pEntity )
	{
		// This happens for impacts that occur on an object that's then destroyed.
		// Clear out the fraction so it uses the server's data
		tr.fraction = 1.0;
		PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
		return;
	}

	// If we hit, perform our custom effects and play the sound
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, pEntity, tr ) )
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1.0 );
	}

	PlayImpactSound( pEntity, tr, vecOrigin, nSurfaceProp );
}

DECLARE_CLIENT_EFFECT( Impact, ImpactCallback );
