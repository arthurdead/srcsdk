//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "physpropclientside.h"
#include "vcollide_parse.h"
#include "mapentities_shared.h"
#include "gamestringpool.h"
#include "props_shared.h"
#include "c_te_effect_dispatch.h"
#include "datacache/imdlcache.h"
#include "view.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FADEOUT_TIME	1.0f

ConVar	cl_phys_props_max( "cl_phys_props_max", "300", 0, "Maximum clientside physic props" );
ConVar	r_propsmaxdist( "r_propsmaxdist", "1200", 0, "Maximum visible distance" );

ConVar	cl_phys_props_enable( "cl_phys_props_enable", "1", 0, "Disable clientside physics props (must be set before loading a level)." );
ConVar	cl_phys_props_respawndist( "cl_phys_props_respawndist", "1500", 0, "Minimum distance from the player that a clientside prop must be before it's allowed to respawn." );
ConVar	cl_phys_props_respawnrate( "cl_phys_props_respawnrate", "60", 0, "Time, in seconds, between clientside prop respawns." );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static int PropBreakablePrecacheAll( int modelIndex )
{
	CUtlVector<breakmodel_t> list;

	BreakModelList( list, modelIndex, COLLISION_GROUP_NONE, 0 );
	return list.Count();
}

static CUtlVector<C_PhysPropClientside*> s_PhysPropList;
static CUtlVector<C_FuncPhysicsRespawnZone*> s_RespawnZoneList;

LINK_ENTITY_TO_CLASS(prop_physics_clientside, C_PhysPropClientside);

C_PhysPropClientside *C_PhysPropClientside::CreateNew( bool bForce )
{
	if ( (s_PhysPropList.Count() >= cl_phys_props_max.GetInt()) && !bForce )
	{
		DevMsg("Warning! Client physic props overflow *max %i).\n", cl_phys_props_max.GetInt() );
		return NULL;
	}

	return CREATE_ENTITY(C_PhysPropClientside, "prop_physics_clientside");
}

C_PhysPropClientside::C_PhysPropClientside()
	: C_BreakableProp()
{
	AddEFlags( EFL_NOT_NETWORKED );

	m_fDeathTime = -1;
	m_impactEnergyScale = 1.0f;
	m_iHealth = 0;
	m_iPhysicsMode = PHYSICS_AUTODETECT;
	m_flTouchDelta = 0;
	m_pRespawnZone = NULL;

	s_PhysPropList.AddToTail( this );
}

C_PhysPropClientside::~C_PhysPropClientside()
{
	if ( m_pRespawnZone )
	{
		m_pRespawnZone->PropDestroyed( this );
	}

	PhysCleanupFrictionSounds( this );
	VPhysicsDestroyObject();
	s_PhysPropList.FindAndRemove( this );
}

void C_PhysPropClientside::SetPhysicsMode(int iMode)
{
	if ( m_iPhysicsMode == PHYSICS_AUTODETECT )
		m_iPhysicsMode = iMode;
}

//-----------------------------------------------------------------------------
// Should we collide?
//-----------------------------------------------------------------------------

bool C_PhysPropClientside::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "physdamagescale"))
	{
		m_impactEnergyScale = atof(szValue);
	}
	else if ( FStrEq(szKeyName, "health") )
	{
		m_iHealth = Q_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "spawnflags"))
	{
		m_spawnflags = Q_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "model"))
	{
		SetModelName( AllocPooledString( szValue ) );
	}
	else if (FStrEq(szKeyName, "fademaxdist"))
	{
		SetDistanceFade( GetMinFadeDist(), Q_atof(szValue) );
	}
	else if (FStrEq(szKeyName, "fademindist"))
	{
		SetDistanceFade( Q_atof(szValue), GetMaxFadeDist() );
	}
	else if (FStrEq(szKeyName, "fadescale"))
	{
		SetGlobalFadeScale( Q_atof(szValue) );
	}
	else if (FStrEq(szKeyName, "inertiaScale"))
	{
		m_inertiaScale = Q_atof(szValue);
	}
	else if (FStrEq(szKeyName, "skin"))
	{
		SetSkin( Q_atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "physicsmode"))
	{
		m_iPhysicsMode = Q_atoi(szValue);
	}

	else
	{
		if ( !BaseClass::KeyValue( szKeyName, szValue ) )
		{
			// key hasn't been handled
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void C_PhysPropClientside::StartTouch( C_BaseEntity *pOther )
{
	// Limit the amount of times we can bounce
	if ( m_flTouchDelta < gpGlobals->curtime )
	{
		HitSurface( pOther );
		m_flTouchDelta = gpGlobals->curtime + 0.1f;
	}

	BaseClass::StartTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void C_PhysPropClientside::HitSurface( C_BaseEntity *pOther )
{
	if ( HasInteraction( PROPINTER_WORLD_BLOODSPLAT ) )
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
		if ( tr.m_pEnt )
		{
			UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		}
	}
}

void C_PhysPropClientside::RecreateAll()
{
	DestroyAll();
	if ( cl_phys_props_enable.GetInt() )
	{
		ParseAllEntities( engine->GetMapEntitiesString() );
		InitializePropRespawnZones();
	}
}

void C_PhysPropClientside::DestroyAll()
{
	while (s_PhysPropList.Count() > 0 )
	{
		C_PhysPropClientside *p = s_PhysPropList[0];
		UTIL_Remove( p );
	}
	while (s_RespawnZoneList.Count() > 0)
	{
		C_FuncPhysicsRespawnZone *p = s_RespawnZoneList[0];
		UTIL_Remove( p );
	}
}

void C_PhysPropClientside::SetRespawnZone( C_FuncPhysicsRespawnZone *pZone ) 
{ 
	m_pRespawnZone = pZone; 
}

//-----------------------------------------------------------------------------
// Purpose: Parse this prop's data from the model, if it has a keyvalues section.
//			Returns true only if this prop is using a model that has a prop_data section that's invalid.
//-----------------------------------------------------------------------------
int C_PhysPropClientside::ParsePropData( void )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( !modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	// Do we have a props section?
	KeyValues *pkvPropData = modelKeyValues->FindKey("prop_data");
	if ( !pkvPropData )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	int iResult = g_PropDataSystem.ParsePropFromKV( this, this, pkvPropData, modelKeyValues );
	modelKeyValues->deleteThis();
	return iResult;
}

bool C_PhysPropClientside::Initialize()
{
	SetModel( STRING(GetModelName()) );

	const model_t *mod = GetModel();
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		SetCollisionBounds( mins, maxs );
	}

	solid_t tmpSolid;

	// Create the object in the physics system

	if ( !PhysModelParseSolid( tmpSolid, this, GetModelIndex() ) )
	{
		DevMsg("C_PhysPropClientside::Initialize: PhysModelParseSolid failed for entity %i.\n", GetModelIndex() );
		return false;
	}
	else
	{
		m_pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, m_spawnflags & SF_PHYSPROP_START_ASLEEP, &tmpSolid );
	
		if ( !m_pPhysicsObject )
		{
			// failed to create a physics object
		DevMsg(" C_PhysPropClientside::Initialize: VPhysicsInitNormal() failed for %s.\n", STRING(GetModelName()) );
			return false;
		}
	}

	// We want touch calls when we hit the world
	unsigned int flags = VPhysicsGetObject()->GetCallbackFlags();
	VPhysicsGetObject()->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );

	if ( m_spawnflags & SF_PHYSPROP_MOTIONDISABLED )
	{
		m_pPhysicsObject->EnableMotion( false );
	}

	if(DispatchSpawn(this) < 0) // loads breakable & prop data
		return false;

	if ( m_iPhysicsMode == PHYSICS_AUTODETECT )
	{
		m_iPhysicsMode = GetAutoPhysicsMode( 
			CollisionProp()->OBBSize(), m_pPhysicsObject->GetMass() );
	}

	if 	( m_spawnflags & SF_PHYSPROP_FORCE_SERVER_SIDE )
	{
		// forced to be server-side by map maker
		return false;
	}
		

	
	if ( m_iPhysicsMode != PHYSICS_CLIENTSIDE )
	{
		// spawn only clientside entities
		return false;
	}
	else 
	{
		if ( engine->IsInEditMode() )
		{
			// don't spawn in map edit mode
			return false;
		}
	}

	if ( GetMinFadeDist() < 0 )
	{
		// start fading out at 75% of r_propsmaxdist
		SetFadeMinMax( r_propsmaxdist.GetFloat(), r_propsmaxdist.GetFloat() * 0.75f );
	}

	// player can push it away
	SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

	UpdatePartitionListEntry();

	CollisionProp()->UpdatePartition();

	SetBlocksLOS( false ); // this should be a small object

	// Set up shadows; do it here so that objects can change shadowcasting state
	CreateShadow();

	UpdateVisibility();

	SetNextThink( TICK_NEVER_THINK );

	return true;
}

void C_PhysPropClientside::Spawn()
{
	// Initialize damage modifiers. Must be done before baseclass spawn.
	m_flDmgModBullet = 1.0;
	m_flDmgModClub = 1.0;
	m_flDmgModExplosive = 1.0;
	m_flDmgModFire = 1.0;

	BaseClass::Spawn();

	// we don't really precache models here, just checking how many we have:
	m_iNumBreakableChunks = PropBreakablePrecacheAll( GetModelIndex() );

	ParsePropData();

	// If we have no custom breakable chunks, see if we're breaking into generic ones
	if ( !m_iNumBreakableChunks )
	{
		if ( GetBreakableModel() != NULL_STRING && GetBreakableCount() )
		{
			m_iNumBreakableChunks = GetBreakableCount();
		}
	}

	// Setup takedamage based upon the health we parsed earlier
	if ( m_iHealth == 0 )
	{
		m_takedamage = DAMAGE_NO;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}
}

void C_PhysPropClientside::OnTakeDamage( int iDamage ) // very simple version
{
	if ( m_takedamage == DAMAGE_NO )
		return;

	m_iHealth -= iDamage;

	if (m_iHealth <= 0)
	{
		Break();
	}
}

float C_PhysPropClientside::GetMass()
{
	if ( VPhysicsGetObject() )
	{
		return VPhysicsGetObject()->GetMass();
	}

	return 0.0f;
}

bool C_PhysPropClientside::IsAsleep()
{
	if ( VPhysicsGetObject() )
	{
		return VPhysicsGetObject()->IsAsleep();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PhysPropClientside::FadeThink( void )
{
	if ( m_fDeathTime < 0 )
	{
		SetNextThink( TICK_NEVER_THINK, "FadeThink" );
		return;
	}

	if ( m_fDeathTime <= gpGlobals->curtime )
	{
		UTIL_Remove( this );
		return;
	}

	// fade out 

	float alpha = (m_fDeathTime - gpGlobals->curtime)/FADEOUT_TIME;

	SetRenderMode( kRenderTransTexture );

	SetRenderAlpha( alpha * 255 );

	SetNextThink( TICK_ALWAYS_THINK, "FadeThink" );
}

void C_PhysPropClientside::StartFadeOut( float fDelay )
{
	m_fDeathTime = gpGlobals->curtime + fDelay + FADEOUT_TIME;

	SetContextThink( &C_PhysPropClientside::FadeThink, gpGlobals->curtime + fDelay, "FadeThink" );
}


void C_PhysPropClientside::Break()
{
	m_takedamage = DAMAGE_NO;
	
	IPhysicsObject *pPhysics = VPhysicsGetObject();

	Vector velocity;
	AngularImpulse angVelocity;
	Vector origin;
	QAngle angles;
	AddSolidFlags( FSOLID_NOT_SOLID );

	if ( pPhysics )
	{
		pPhysics->GetVelocity( &velocity, &angVelocity );
		pPhysics->GetPosition( &origin, &angles );
		pPhysics->RecheckCollisionFilter();
	}
	else
	{
		velocity = GetAbsVelocity();
		QAngleToAngularImpulse( GetLocalAngularVelocity(), angVelocity );
		origin = GetAbsOrigin();
		angles = GetAbsAngles();
	}

	breakablepropparams_t params( origin, angles, velocity, angVelocity );
	params.impactEnergyScale = m_impactEnergyScale;
	params.defCollisionGroup = GetCollisionGroup();
	if ( params.defCollisionGroup == COLLISION_GROUP_NONE )
	{
		// don't automatically make anything COLLISION_GROUP_NONE or it will
		// collide with debris being ejected by breaking
		params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;
	}

	// no damage/damage force? set a burst of 100 for some movement
	params.defBurstScale = 100;

	// spwan break chunks
	PropBreakableCreateAll( GetModelIndex(), pPhysics, params, this, -1, false );

	UTIL_Remove( this );
}

void C_PhysPropClientside::Clone( Vector &velocity )
{
	C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew();

	if ( !pEntity )
		return;

	pEntity->m_spawnflags = m_spawnflags;

	// We never want to be motion disabled
	pEntity->m_spawnflags &= ~SF_PHYSPROP_MOTIONDISABLED;
		
	pEntity->SetDmgModBullet( GetDmgModBullet() );
	pEntity->SetDmgModClub( GetDmgModClub() );
	pEntity->SetDmgModExplosive( GetDmgModExplosive() );
	
	pEntity->SetModelName( GetModelName() );
	pEntity->SetLocalOrigin( GetLocalOrigin() );
	pEntity->SetLocalAngles( GetLocalAngles() );
	pEntity->SetOwnerEntity( this );
	pEntity->SetPhysicsMode( PHYSICS_CLIENTSIDE );

	if ( !pEntity->Initialize() )
	{
		UTIL_Remove( pEntity );
		return;
	}

	pEntity->SetSkin( GetSkin() );
	pEntity->m_iHealth = m_iHealth;

	if ( pEntity->m_iHealth == 0 )
	{
		// if  no health, don't collide with player anymore, don't take damage
		pEntity->m_takedamage = DAMAGE_NO;
		pEntity->SetCollisionGroup( COLLISION_GROUP_NONE );
	}
	
	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();

	if( pPhysicsObject )
	{
		// randomize velocity by 5%
		float rndf = RandomFloat( -0.025, 0.025 );
		Vector rndVel = velocity + rndf*velocity;

		pPhysicsObject->AddVelocity( &rndVel, NULL );
	}
	else
	{
		// failed to create a physics object
		UTIL_Remove( pEntity );
	}
}

void C_PhysPropClientside::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_PhysPropClientside::ImpactTrace" );
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;
	int iDamage = 0;

	if ( iDamageType & DMG_BLAST )
	{
		iDamage = VectorLength( dir );
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		// guess avg damage
		if ( iDamageType == DMG_BULLET )
		{
			iDamage = 30;
		}
		else
		{
			iDamage = 50;
		}
		 
		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		// Build the impact data
		CEffectData data;
		data.m_vOrigin = pTrace->endpos;
		data.m_vStart = pTrace->startpos;
		data.m_nSurfaceProp = pTrace->surface.surfaceProps;
		data.m_nDamageType = iDamageType;
		data.m_nHitBox = pTrace->hitbox;
		data.m_hEntity = GetRefEHandle();

		// Send it on its way
		if ( !pCustomImpactName )
		{
			DispatchEffect( "Impact", data );
		}
		else
		{
			DispatchEffect( pCustomImpactName, data );
		}
	}

	// Clone( dir ); // debug code

	OnTakeDamage( iDamage );
}

const char *C_PhysPropClientside::ParseEntity( const char *pEntData )
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	MDLCACHE_CRITICAL_SECTION();

	if (!entData.ExtractValue("classname", className))
	{
		Error( "classname missing from entity!\n" );
	}

	if ( !Q_strcmp( className, "prop_physics" ) ||
		!Q_strcmp( className, "prop_physics_override" ) ||
		!Q_strcmp( className, "prop_physics_multiplayer" ) )
	{
		// always force clientside entitis placed in maps
		C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew( true ); 

		if ( pEntity )
		{	// Set up keyvalues.
			pEntity->ParseMapData(&entData);
			
			if ( !pEntity->Initialize() )
				UTIL_Remove( pEntity );
		
			return entData.CurrentBufferPosition();
		}
	}

	if ( !Q_strcmp( className, "func_proprrespawnzone" ) )
	{
		C_FuncPhysicsRespawnZone *pEntity = CREATE_ENTITY(C_FuncPhysicsRespawnZone, "func_proprrespawnzone_clientside");

		if ( pEntity )
		{
			// Set up keyvalues.
			pEntity->ParseMapData(&entData);

			if ( !pEntity->Initialize() )
				UTIL_Remove( pEntity );

			return entData.CurrentBufferPosition();
		}
	}
	
	// Just skip past all the keys.
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];
	if ( entData.GetFirstKey(keyName, value) )
	{
		do 
		{
		} 
		while ( entData.GetNextKey(keyName, value) );
	}

	//
	// Return the current parser position in the data block
	//
	return entData.CurrentBufferPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Only called on BSP load. Parses and spawns all the entities in the BSP.
// Input  : pMapData - Pointer to the entity data block to parse.
//-----------------------------------------------------------------------------
void C_PhysPropClientside::ParseAllEntities(const char *pMapData)
{
	int nEntities = 0;

	char szTokenBuffer[MAPKEY_MAXLENGTH];

	//
	//  Loop through all entities in the map data, creating each.
	//
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Error( "MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		//
		// Parse the entity and add it to the spawn list.
		//

		pMapData = ParseEntity( pMapData );

		nEntities++;
	}
}

CBaseAnimating *BreakModelCreate_Ragdoll( CBaseEntity *pOwnerEnt, breakmodel_t *pModel, const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity )
{
	C_BaseAnimating *pOwner = dynamic_cast<C_BaseAnimating *>( pOwnerEnt );
	if ( !pOwner )
		return NULL;

	C_ClientRagdoll *pRagdoll = CREATE_ENTITY(C_ClientRagdoll, "client_ragdoll");
	if ( pRagdoll == NULL )
		return NULL;

	const char *pModelName = pModel->modelName;

	pRagdoll->SetModel( pModelName );

	pRagdoll->SetAbsOrigin( position );
	pRagdoll->SetAbsAngles( angles );

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.1f;

	pRagdoll->SetParent( pOwner );
	pRagdoll->ForceSetupBonesAtTime( boneDelta0, gpGlobals->curtime - boneDt );
	pRagdoll->ForceSetupBonesAtTime( boneDelta1, gpGlobals->curtime );
	pRagdoll->ForceSetupBonesAtTime( currentBones, gpGlobals->curtime );
	pRagdoll->SetParent( NULL );

	// We need to take these from the entity
	//pRagdoll->SetAbsOrigin( position );
	//pRagdoll->SetAbsAngles( angles );

	pRagdoll->IgniteRagdoll( pOwner );
	pRagdoll->TransferDissolveFrom( pOwner );
	pRagdoll->InitModelEffects();

	if ( pOwner->IsEffectActive( EF_NOSHADOW ) )
	{
		pRagdoll->AddEffects( EF_NOSHADOW );
	}

	pRagdoll->SetClientSideRagdoll();
	pRagdoll->SetRenderMode( pOwner->GetRenderMode() );
	pRagdoll->SetRenderColor( pOwner->GetRenderColor().r, pOwner->GetRenderColor().g, pOwner->GetRenderColor().b );
	pRagdoll->SetRenderAlpha( pOwner->GetRenderAlpha() );
	pRagdoll->SetGlobalFadeScale( pOwner->GetGlobalFadeScale() );

	pRagdoll->SetSkin( pOwner->GetSkin() );
	//pRagdoll->m_vecForce = pOwner->m_vecForce;
	//pRagdoll->m_nForceBone = 0; //pOwner->m_nForceBone;
	pRagdoll->SetNextThink( TICK_ALWAYS_THINK );

	pRagdoll->SetModelName( AllocPooledString( pModelName ) );
	pRagdoll->ResetSequence( 0 );
	pRagdoll->SetModelScale( pOwner->GetModelScale() );
	pRagdoll->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	//pRagdoll->m_builtRagdoll = true;

	CStudioHdr *hdr = pRagdoll->GetModelPtr();
	if ( !hdr )
	{
		UTIL_Remove( pRagdoll );
		return NULL;
	}

	pRagdoll->m_pRagdoll = CreateRagdoll( 
		pRagdoll, 
		hdr, 
		vec3_origin, 
		0, 
		boneDelta0, 
		boneDelta1, 
		currentBones,
 		boneDt );

	IPhysicsObject *pPhysicsObject = pRagdoll->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		// randomize velocity by 5%
		float rndf = RandomFloat( -0.025, 0.025 );
		Vector rndVel = velocity + rndf*velocity;

		pPhysicsObject->AddVelocity( &rndVel, &angVelocity );
	}
	pRagdoll->ApplyLocalAngularVelocityImpulse( angVelocity );

	if ( pRagdoll->m_pRagdoll )
	{
		pRagdoll->m_bImportant = false;
		s_RagdollLRU.MoveToTopOfLRU( pRagdoll, pRagdoll->m_bImportant, pModel->fadeTime > 0.0f ? gpGlobals->curtime + pModel->fadeTime : 0.0f );
		pRagdoll->m_bFadeOut = true;
	}

	// Cause the entity to recompute its shadow	type and make a
	// version which only updates when physics state changes
	// NOTE: We have to do this after m_pRagdoll is assigned above
	// because that's what ShadowCastType uses to figure out which type of shadow to use.
	pRagdoll->DestroyShadow();
	pRagdoll->CreateShadow();

	pRagdoll->SetAbsOrigin( position );
	pRagdoll->SetAbsAngles( angles );

	pRagdoll->SetPlaybackRate( 0 );
	pRagdoll->UpdatePartitionListEntry();
	pRagdoll->MarkRenderHandleDirty();

	//pRagdoll->InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );

	if(DispatchSpawn(pRagdoll) < 0) {
		UTIL_Remove(pRagdoll);
		return NULL;
	}

	return pRagdoll;
}

CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
	const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params )
{
	if ( pModel->isRagdoll )
	{
		CBaseEntity *pEntity = BreakModelCreate_Ragdoll( pOwner, pModel, position, angles, velocity, angVelocity );
		return pEntity;
	}
	
	C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew();

	if ( !pEntity )
		return NULL;

	// UNDONE: Allow .qc to override spawnflags for child pieces
	C_PhysPropClientside *pBreakableOwner = dynamic_cast<C_PhysPropClientside *>(pOwner);

	// Inherit the base object's damage modifiers
	if ( pBreakableOwner )
	{
		pEntity->SetEffects( pBreakableOwner->GetEffects() );

		pEntity->m_spawnflags = pBreakableOwner->m_spawnflags;

		// We never want to be motion disabled
		pEntity->m_spawnflags &= ~SF_PHYSPROP_MOTIONDISABLED;
		
		pEntity->SetDmgModBullet( pBreakableOwner->GetDmgModBullet() );
		pEntity->SetDmgModClub( pBreakableOwner->GetDmgModClub() );
		pEntity->SetDmgModExplosive( pBreakableOwner->GetDmgModExplosive() );

		// FIXME: If this was created from a client-side entity which was in the
		// middle of ramping the fade scale, we're screwed.
		pEntity->CopyFadeFrom( pBreakableOwner );
	}
	
	pEntity->SetModelName( AllocPooledString( pModel->modelName ) );
	pEntity->SetLocalOrigin( position );
	pEntity->SetLocalAngles( angles );
	pEntity->SetOwnerEntity( pOwner );
	pEntity->SetPhysicsMode( PHYSICS_CLIENTSIDE );

	if ( !pEntity->Initialize() )
	{
		UTIL_Remove( pEntity );
		return NULL;
	}

	pEntity->SetSkin( nSkin );
	pEntity->m_iHealth = pModel->health;

	pEntity->SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	if ( pModel->health == 0 )
	{
		// if  no health, don't collide with player anymore, don't take damage
		pEntity->m_takedamage = DAMAGE_NO;

		if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_PUSHAWAY )
		{
			pEntity->SetCollisionGroup( COLLISION_GROUP_NONE );
		}
	}
	
	if ( pModel->fadeTime > 0 )
	{
		pEntity->StartFadeOut( pModel->fadeTime );
	}

	if ( pModel->fadeMinDist > 0 && pModel->fadeMaxDist >= pModel->fadeMinDist )
	{
		pEntity->SetFadeMinMax( pModel->fadeMinDist, pModel->fadeMaxDist );
	}

	if ( pModel->isRagdoll )
	{
		DevMsg( "BreakModelCreateSingle: clientside doesn't support ragdoll breakmodels.\n" );
	}


	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();

	if( pPhysicsObject )
	{
		// randomize velocity by 5%
		float rndf = RandomFloat( -0.025, 0.025 );
		Vector rndVel = velocity + rndf*velocity;

		pPhysicsObject->AddVelocity( &rndVel, &angVelocity );
	}
	else
	{
		// failed to create a physics object
		UTIL_Remove( pEntity );
		return NULL;
	}

	return pEntity;
}



//======================================================================================================================
// PROP RESPAWN ZONES
//======================================================================================================================
LINK_ENTITY_TO_CLASS(func_proprrespawnzone_clientside, C_FuncPhysicsRespawnZone);

C_FuncPhysicsRespawnZone::C_FuncPhysicsRespawnZone( void )
{
	AddEFlags( EFL_NOT_NETWORKED );

	s_RespawnZoneList.AddToTail( this );
}

C_FuncPhysicsRespawnZone::~C_FuncPhysicsRespawnZone( void )
{
	s_RespawnZoneList.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_FuncPhysicsRespawnZone::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "model"))
	{
		SetModelName( AllocPooledString( szValue ) );
	}
	else
	{
		if ( !BaseClass::KeyValue( szKeyName, szValue ) )
		{
			// key hasn't been handled
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_FuncPhysicsRespawnZone::Initialize( void )
{
	SetModel(STRING(GetModelName()));

	SetSolid( SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_TRIGGER );	
	SetMoveType( MOVETYPE_NONE );

	const model_t *mod = GetModel();
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		SetCollisionBounds( mins, maxs );
	}

	if(DispatchSpawn(this) < 0)
		return false;

	AddEffects( EF_NODRAW );

	UpdatePartitionListEntry();

	CollisionProp()->UpdatePartition();

	UpdateVisibility();

	SetContextThink( &C_FuncPhysicsRespawnZone::RespawnThink, gpGlobals->curtime + (cl_phys_props_respawnrate.GetFloat() * RandomFloat(1.0,1.1)), "RespawnThink" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Iterate over all prop respawn zones and find the props inside them
//-----------------------------------------------------------------------------
void C_PhysPropClientside::InitializePropRespawnZones(void)
{
	for ( int i = 0; i < s_RespawnZoneList.Count(); i++ )
	{
		C_FuncPhysicsRespawnZone *pZone = s_RespawnZoneList[i];
		pZone->InitializePropsWithin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FuncPhysicsRespawnZone::InitializePropsWithin( void )
{
	// Find the props inside this zone
	for ( int i = 0; i < s_PhysPropList.Count(); i++ )
	{
		C_PhysPropClientside *pProp = s_PhysPropList[i];
		if ( CollisionProp()->IsPointInBounds( pProp->WorldSpaceCenter() ) )
		{
			pProp->SetRespawnZone( this );

			// This is a crappy way to do this
			int index = m_PropList.AddToTail();
			m_PropList[index].iszModelName = pProp->GetModelName();
			m_PropList[index].vecOrigin = pProp->GetAbsOrigin();
			m_PropList[index].vecAngles = pProp->GetAbsAngles();
			m_PropList[index].iSkin = pProp->GetSkin();
			m_PropList[index].iHealth = pProp->GetHealth();
			m_PropList[index].iSpawnFlags = pProp->m_spawnflags;
			m_PropList[index].hClientEntity = pProp->GetClientHandle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FuncPhysicsRespawnZone::PropDestroyed( C_PhysPropClientside *pProp )
{
	for ( int i = 0; i < m_PropList.Count(); i++ )
	{
		if ( pProp->GetClientHandle() == m_PropList[i].hClientEntity )
		{
			m_PropList[i].hClientEntity = INVALID_CLIENTENTITY_HANDLE;
			return;
		}
	}

	// We've got a clientside prop that thinks it belongs to a zone that doesn't recognise it. Shouldn't happen.
	Assert(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_FuncPhysicsRespawnZone::CanMovePropAt( Vector vecOrigin, const Vector &vecMins, const Vector &vecMaxs )
{
	float flDist = cl_phys_props_respawndist.GetFloat();

	// Do a distance check first. We don't want to move props when the player is near 'em.
	if ( (MainViewOrigin() - vecOrigin).LengthSqr() < (flDist*flDist) )
		return false;

	// Now make sure it's not in view
	if( engine->IsBoxInViewCluster( vecMins + vecOrigin, vecMaxs + vecOrigin) )
		return false;

	if( !engine->CullBox( vecMins + vecOrigin, vecMaxs + vecOrigin ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FuncPhysicsRespawnZone::RespawnProps( void )
{
	for ( int i = 0; i < m_PropList.Count(); i++ )
	{
		if ( m_PropList[i].hClientEntity == INVALID_CLIENTENTITY_HANDLE )
		{
			if ( !CanMovePropAt( m_PropList[i].vecOrigin, -Vector(32,32,32), Vector(32,32,32) ) )
				continue;

			// This is a crappy way to do this
			C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew();
			if ( pEntity )
			{
				pEntity->m_spawnflags = m_PropList[i].iSpawnFlags;
				pEntity->SetModelName( m_PropList[i].iszModelName );
				pEntity->SetAbsOrigin( m_PropList[i].vecOrigin );
				pEntity->SetAbsAngles( m_PropList[i].vecAngles );
				pEntity->SetPhysicsMode( PHYSICS_CLIENTSIDE );
				pEntity->SetSkin( m_PropList[i].iSkin );
				pEntity->m_iHealth = m_PropList[i].iHealth;
				if ( pEntity->m_iHealth == 0 )
				{
					pEntity->m_takedamage = DAMAGE_NO;
				}

				if ( !pEntity->Initialize() )
				{
					UTIL_Remove( pEntity );
				}
				else
				{
					pEntity->SetRespawnZone( this );
					m_PropList[i].hClientEntity = pEntity->GetClientHandle();
				}
			}
		}
		else
		{
			// If the prop has moved, bring it back
			C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( m_PropList[i].hClientEntity );
			if ( pEntity )
			{
				if ( !CollisionProp()->IsPointInBounds( pEntity->WorldSpaceCenter() ) )
				{
					Vector vecMins, vecMaxs;
					pEntity->CollisionProp()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
					if ( !CanMovePropAt( m_PropList[i].vecOrigin, vecMins, vecMaxs ) || 
						 !CanMovePropAt( pEntity->GetAbsOrigin(), vecMins, vecMaxs ) )
						continue;

					pEntity->SetAbsOrigin( m_PropList[i].vecOrigin );
					pEntity->SetAbsAngles( m_PropList[i].vecAngles );

					IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
					if ( pPhys )
					{
						pPhys->SetPosition( pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), true );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_FuncPhysicsRespawnZone::RespawnThink( void )
{
	RespawnProps();

	SetNextThink( gpGlobals->curtime + (cl_phys_props_respawnrate.GetFloat() * RandomFloat(1.0,1.1)), "RespawnThink" );
}