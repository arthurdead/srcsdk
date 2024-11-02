//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "decals.h"
#include "effect_dispatch_data.h"
#include "model_types.h"
#include "gamestringpool.h"
#include "ammodef.h"
#include "takedamageinfo.h"
#include "shot_manipulator.h"
#include "ai_debug_shared.h"
#include "mapentities_shared.h"
#include "debugoverlay_shared.h"
#include "coordsize.h"
#include "vphysics/performance.h"
#include "ai_criteria.h"
#include "variant_t.h"
#include "collisionproperty.h"

#ifdef CLIENT_DLL
	#include "c_te_effect_dispatch.h"
#else
	#include "te_effect_dispatch.h"
	#include "soundent.h"
	#include "iservervehicle.h"
	#include "player_pickup.h"
	#include "waterbullet.h"
	#include "func_break.h"
	#include "world.h"
	#include "globalstate.h"

	#include "gamestats.h"

#endif

#ifdef PORTAL
	#include "prop_portal_shared.h"
#endif

#include "rumble_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue );
extern bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen );

#ifdef GAME_DLL
ConVar ent_debugkeys( "ent_debugkeys", "" );
#endif

bool CSharedBaseEntity::m_bAllowPrecache = false;
bool CSharedBaseEntity::sm_bAccurateTriggerBboxChecks = true;	// set to false for legacy behavior in ep1

// Set default max values for entities based on the existing constants from elsewhere
float k_flMaxEntityPosCoord = MAX_COORD_FLOAT;
float k_flMaxEntityEulerAngle = 360.0 * 1000.0f; // really should be restricted to +/-180, but some code doesn't adhere to this.  let's just trap NANs, etc
// Sometimes the resulting computed speeds are legitimately above the original
// constants; use bumped up versions for the downstream validation logic to
// account for this.
float k_flMaxEntitySpeed = k_flMaxVelocity * 2.0f;
float k_flMaxEntitySpinRate = k_flMaxAngularVelocity * 10.0f;

ConVar	ai_shot_bias_min( "ai_shot_bias_min", "-1.0", FCVAR_REPLICATED );
ConVar	ai_shot_bias_max( "ai_shot_bias_max", "1.0", FCVAR_REPLICATED );
ConVar	ai_debug_shoot_positions( "ai_debug_shoot_positions", "0", FCVAR_REPLICATED | FCVAR_CHEAT );

#if defined(GAME_DLL)
ConVar	ai_shot_notify_targets( "ai_shot_notify_targets", "0", FCVAR_NONE, "Allows fired bullets to notify the NPCs and players they are targeting, regardless of whether they hit them or not. Can be used for custom AI and speech." );
#endif

// Utility func to throttle rate at which the "reasonable position" spew goes out
static double s_LastEntityReasonableEmitTime;
bool CheckEmitReasonablePhysicsSpew()
{

	// Reported recently?
	double now = Plat_FloatTime();
	if ( now >= s_LastEntityReasonableEmitTime && now < s_LastEntityReasonableEmitTime + 5.0 )
	{
		// Already reported recently
		return false;
	}

	// Not reported recently.  Report it now
	s_LastEntityReasonableEmitTime = now;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Spawn some blood particles
//-----------------------------------------------------------------------------
void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, vecDir, bloodColor, (int)flDamage );
}

//-----------------------------------------------------------------------------
// The player drives simulation of this entity
//-----------------------------------------------------------------------------
void CSharedBaseEntity::SetPlayerSimulated( CSharedBasePlayer *pOwner )
{
	m_bIsPlayerSimulated = true;
	pOwner->AddToPlayerSimulationList( this );
	m_hPlayerSimulationOwner = pOwner;
}

void CSharedBaseEntity::UnsetPlayerSimulated( void )
{
	if ( m_hPlayerSimulationOwner != NULL )
	{
		m_hPlayerSimulationOwner->RemoveFromPlayerSimulationList( this );
	}
	m_hPlayerSimulationOwner = NULL;
	m_bIsPlayerSimulated = false;
}

// position of eyes
Vector CSharedBaseEntity::EyePosition( void )
{ 
	return GetAbsOrigin() + GetViewOffset(); 
}

const QAngle &CSharedBaseEntity::EyeAngles( void )
{
	return GetAbsAngles();
}

const QAngle &CSharedBaseEntity::LocalEyeAngles( void )
{
	return GetLocalAngles();
}

// position of ears
Vector CSharedBaseEntity::EarPosition( void )
{ 
	return EyePosition(); 
}

void CSharedBaseEntity::SetViewOffset( const Vector& v ) 
{ 
	m_vecViewOffset = v; 
}

const Vector& CSharedBaseEntity::GetViewOffset() const 
{ 
	return m_vecViewOffset; 
}


//-----------------------------------------------------------------------------
// center point of entity
//-----------------------------------------------------------------------------
const Vector &CSharedBaseEntity::WorldSpaceCenter( ) const 
{
	return CollisionProp()->WorldSpaceCenter();
}

#if !defined( CLIENT_DLL )
#define CHANGE_FLAGS(flags,newFlags) { unsigned int old = flags; flags = (newFlags); gEntList.ReportEntityFlagsChanged( this, old, flags ); }
#else
#define CHANGE_FLAGS(flags,newFlags) (flags = (newFlags))
#endif

void CSharedBaseEntity::AddFlag( int flags )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags | flags );
}

void CSharedBaseEntity::RemoveFlag( int flagsToRemove )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags & ~flagsToRemove );
}

void CSharedBaseEntity::ClearFlags( void )
{
	CHANGE_FLAGS( m_fFlags, 0 );
}

void CSharedBaseEntity::ToggleFlag( int flagToToggle )
{
	CHANGE_FLAGS( m_fFlags, m_fFlags ^ flagToToggle );
}

void CSharedBaseEntity::SetEffects( int nEffects )
{
	if ( nEffects != m_fEffects )
	{
#if !defined( CLIENT_DLL ) && 0
		// Hack for now, to avoid player emitting radius with his flashlight
		if ( !IsPlayer() )
		{
			if ( (nEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) && !(m_fEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) )
			{
				AddEntityToDarknessCheck( this );
			}
			else if ( !(nEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) && (m_fEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) )
			{
				RemoveEntityFromDarknessCheck( this );
			}
		}
#endif // !CLIENT_DLL

		m_fEffects = nEffects;

#ifndef CLIENT_DLL
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
	}
}

void CSharedBaseEntity::AddEffects( int nEffects ) 
{ 
#if !defined( CLIENT_DLL ) && 0
	if ( (nEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) && !(m_fEffects & (EF_BRIGHTLIGHT|EF_DIMLIGHT)) )
	{
		// Hack for now, to avoid player emitting radius with his flashlight
		if ( !IsPlayer() )
		{
			AddEntityToDarknessCheck( this );
		}
	}
#endif // !CLIENT_DLL

	m_fEffects |= nEffects; 

#ifdef CLIENT_DLL
	if ( m_fEffects & (EF_DIMLIGHT|EF_DIMLIGHT) )
	{
		AddToEntityList(ENTITY_LIST_PRERENDER);
	}
#endif

	if ( nEffects & EF_NODRAW)
	{
#ifndef CLIENT_DLL
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
	}
}

void CSharedBaseEntity::SetBlocksLOS( bool bBlocksLOS )
{
	if ( bBlocksLOS )
	{
		RemoveEFlags( EFL_DONTBLOCKLOS );
	}
	else
	{
		AddEFlags( EFL_DONTBLOCKLOS );
	}
}

bool CSharedBaseEntity::BlocksLOS( void ) 
{ 
	return !IsEFlagSet(EFL_DONTBLOCKLOS); 
}

void CSharedBaseEntity::SetAIWalkable( bool bBlocksLOS )
{
	if ( bBlocksLOS )
	{
		RemoveEFlags( EFL_DONTWALKON );
	}
	else
	{
		AddEFlags( EFL_DONTWALKON );
	}
}

bool CSharedBaseEntity::IsAIWalkable( void ) 
{ 
	return !IsEFlagSet(EFL_DONTWALKON);
}

//-----------------------------------------------------------------------------
// Purpose: Verifies that this entity's data description is valid in debug builds.
//-----------------------------------------------------------------------------
#ifdef _DEBUG
typedef CUtlVector< const char * >	KeyValueNameList_t;

static void AddDataMapFieldNamesToList( KeyValueNameList_t &list, datamap_t *pDataMap )
{
	while (pDataMap != NULL)
	{
		for (int i = 0; i < pDataMap->dataNumFields; i++)
		{
			typedescription_t *pField = &pDataMap->dataDesc[i];

			if(i == 0 &&
				pField->fieldType == FIELD_VOID &&
				pField->fieldOffset[0] == 0 &&
				pField->fieldSize == 0 &&
				pField->fieldSizeInBytes == 0 &&
				pField->flags == 0 &&
				pField->fieldName == NULL) {
				continue;
			}

			if (pField->fieldType == FIELD_EMBEDDED)
			{
				AddDataMapFieldNamesToList( list, pField->td );
				continue;
			}

			if ((pField->flags & (FTYPEDESC_KEY|FTYPEDESC_INPUT|FTYPEDESC_OUTPUT)) == 0)
			{
				AssertMsg( 0,"%s has non map data description\n", pDataMap->dataClassName);
				continue;
			}

			if ((pField->flags & FTYPEDESC_KEY) != 0)
			{
				list.AddToTail( pField->externalName );
			}
		}
	
		pDataMap = pDataMap->baseMap;
	}
}

void CSharedBaseEntity::ValidateDataDescription(void)
{
	// Multiple key fields that have the same name are not allowed - it creates an
	// ambiguity when trying to parse keyvalues and outputs.
	datamap_t *pDataMap = GetMapDataDesc();
	if ((pDataMap == NULL) || pDataMap->bValidityChecked)
		return;

	pDataMap->bValidityChecked = true;

	// Let's generate a list of all keyvalue strings in the entire hierarchy...
	KeyValueNameList_t	names(128);
	AddDataMapFieldNamesToList( names, pDataMap );

	for (int i = names.Count(); --i > 0; )
	{
		for (int j = i - 1; --j >= 0; )
		{
			if (!Q_stricmp(names[i], names[j]))
			{
				Log_Msg(LOG_MAPPARSE, "%s has multiple data description entries for \"%s\"\n", STRING(m_iClassname), names[i]);
				break;
			}
		}
	}
}
#endif // _DEBUG

//-----------------------------------------------------------------------------
// Purpose: Handles keys and outputs from the BSP.
// Input  : mapData - Text block of keys and values from the BSP.
//-----------------------------------------------------------------------------
void CSharedBaseEntity::ParseMapData( CEntityMapData *mapData )
{
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];

	#ifdef _DEBUG
	ValidateDataDescription();
	#endif // _DEBUG

	// loop through all keys in the data block and pass the info back into the object
	if ( mapData->GetFirstKey(keyName, value) )
	{
		do 
		{
			KeyValue( keyName, value );
		} 
		while ( mapData->GetNextKey(keyName, value) );
	}

	OnParseMapDataFinished();
}

bool CSharedPointEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "rendercolor" ))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "rendercolor24" ))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "rendercolor32" ))
	{
		return false;
	}
	
	if ( FStrEq( szKeyName, "renderamt" ) )
	{
		return false;
	}

	if ( FStrEq( szKeyName, "disableshadows" ))
	{
		return false;
	}

	if (FStrEq(szKeyName, "disableshadowdepth"))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "mins" ))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "maxs" ))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "disablereceiveshadows" ))
	{
		return false;
	}

	if (FStrEq(szKeyName, "disableflashlight"))
	{
		return false;
	}

	if ( FStrEq( szKeyName, "mingpulevel" ))
	{
		return true;
	}
	if ( FStrEq( szKeyName, "maxgpulevel" ))
	{
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

bool CSharedLogicalEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if( FStrEq( szKeyName, "angle" ) )
	{
		return false;
	}

	if( FStrEq( szKeyName, "angles" ) )
	{
		return false;
	}

	if( FStrEq( szKeyName, "origin" ) )
	{
		return false;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Parse data from a map file
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	//!! temp hack, until worldcraft is fixed
	// strip the # tokens from (duplicate) key names
	char *s = (char *)strchr( szKeyName, '#' );
	if ( s )
	{
		*s = '\0';
	}

	if ( FStrEq( szKeyName, "rendercolor" ))
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
		return true;
	}

	if ( FStrEq( szKeyName, "rendercolor24" ))
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
		return true;
	}

	if ( FStrEq( szKeyName, "rendercolor32" ))
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
		SetRenderAlpha( tmp.a );
		return true;
	}
	
	if ( FStrEq( szKeyName, "renderamt" ) )
	{
		SetRenderAlpha( atoi( szValue ) );
		return true;
	}

	if ( FStrEq( szKeyName, "disableshadows" ))
	{
		int val = atoi( szValue );
		if (val)
		{
			AddEffects( EF_NOSHADOW );
		}
		return true;
	}

	if (FStrEq(szKeyName, "disableshadowdepth"))
	{
		int val = atoi(szValue);
		if (val)
		{
			AddEffects( EF_NOSHADOWDEPTH );
		}
		return true;
	}

	if ( FStrEq( szKeyName, "mins" ))
	{
		Vector mins;
		UTIL_StringToVector( mins.Base(), szValue );
		CollisionProp()->SetCollisionBounds( mins, CollisionProp()->OBBMaxs() );
		return true;
	}

	if ( FStrEq( szKeyName, "maxs" ))
	{
		Vector maxs;
		UTIL_StringToVector( maxs.Base(), szValue );
		CollisionProp()->SetCollisionBounds( CollisionProp()->OBBMins(), maxs );
		return true;
	}

	if ( FStrEq( szKeyName, "disablereceiveshadows" ))
	{
		int val = atoi( szValue );
		if (val)
		{
			AddEffects( EF_NORECEIVESHADOW );
		}
		return true;
	}

	if (FStrEq(szKeyName, "disableflashlight"))
	{
		int val = atoi(szValue);
		if (val)
		{
			AddEffects( EF_NOFLASHLIGHT );
		}
		return true;
	}

	if ( FStrEq( szKeyName, "nodamageforces" ))
	{
		int val = atoi( szValue );
		if (val)
		{
			AddEFlags( EFL_NO_DAMAGE_FORCES );
		}
		return true;
	}

	// Fix up single angles
	if( FStrEq( szKeyName, "angle" ) )
	{
		static char szBuf[64];

		float y = atof( szValue );
		if (y >= 0)
		{
			Q_snprintf( szBuf,sizeof(szBuf), "%f %f %f", GetLocalAngles()[0], y, GetLocalAngles()[2] );
		}
		else if ((int)y == -1)
		{
			Q_strncpy( szBuf, "-90 0 0", sizeof(szBuf) );
		}
		else
		{
			Q_strncpy( szBuf, "90 0 0", sizeof(szBuf) );
		}

		// Do this so inherited classes looking for 'angles' don't have to bother with 'angle'
		return KeyValue( "angles", szBuf );
	}

	// NOTE: Have to do these separate because they set two values instead of one
	if( FStrEq( szKeyName, "angles" ) )
	{
		QAngle angles;
		UTIL_StringToVector( angles.Base(), szValue );

		// If you're hitting this assert, it's probably because you're
		// calling SetLocalAngles from within a KeyValues method.. use SetAbsAngles instead!
		Assert( (GetMoveParent() == NULL) && !IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ) );
		SetAbsAngles( angles );
		return true;
	}

	if( FStrEq( szKeyName, "origin" ) )
	{
		Vector vecOrigin;
		UTIL_StringToVector( vecOrigin.Base(), szValue );

		// If you're hitting this assert, it's probably because you're
		// calling SetLocalOrigin from within a KeyValues method.. use SetAbsOrigin instead!
		Assert( (GetMoveParent() == NULL) && !IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ) );
		SetAbsOrigin( vecOrigin );
		return true;
	}

	if ( FStrEq( szKeyName, "eflags" ) )
	{
		// Can't use DEFINE_KEYFIELD since eflags might be set before KV are parsed
		AddEFlags( atoi( szValue ) );
		return true;
	}

	if ( FStrEq( szKeyName, "targetname" ) )
	{
		m_iName = AllocPooledString( szValue );
		return true;
	}

	if ( FStrEq( szKeyName, "mincpulevel" ))
	{
		m_nMinCPULevel = atoi( szValue );
		return true;
	}
	if ( FStrEq( szKeyName, "maxcpulevel" ))
	{
		m_nMaxCPULevel = atoi( szValue );
		return true;
	}
	if ( FStrEq( szKeyName, "mingpulevel" ))
	{
		m_nMinGPULevel = atoi( szValue );
		return true;
	}
	if ( FStrEq( szKeyName, "maxgpulevel" ))
	{
		m_nMaxGPULevel = atoi( szValue );
		return true;
	}

	// loop through the data description, and try and place the keys in
#ifdef GAME_DLL
	if ( !*ent_debugkeys.GetString() )
#endif
	{
		for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
				return true;
		}
	}
#ifdef GAME_DLL
	else
	{
		// debug version - can be used to see what keys have been parsed in
		bool printKeyHits = false;
		const char *debugName = "";

		if ( *ent_debugkeys.GetString() && !Q_stricmp(ent_debugkeys.GetString(), STRING(m_iClassname)) )
		{
			// Msg( "-- found entity of type %s\n", STRING(m_iClassname) );
			printKeyHits = true;
			debugName = STRING(m_iClassname);
		}

		// loop through the data description, and try and place the keys in
		for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( !printKeyHits && *ent_debugkeys.GetString() && !Q_stricmp(dmap->dataClassName, ent_debugkeys.GetString()) )
			{
				// Msg( "-- found class of type %s\n", dmap->dataClassName );
				printKeyHits = true;
				debugName = dmap->dataClassName;
			}

			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
			{
				if ( printKeyHits )
					Msg( "(%s) key: %-16s value: %s\n", debugName, szKeyName, szValue );
				
				return true;
			}
		}

		if ( printKeyHits )
			Msg( "!! (%s) key not handled: \"%s\" \"%s\"\n", STRING(m_iClassname), szKeyName, szValue );
	}
#endif

	// key hasn't been handled
	return false;
}

bool CSharedBaseEntity::KeyValue( const char *szKeyName, float flValue ) 
{
	char	string[256];

	Q_snprintf(string,sizeof(string), "%f", flValue );

	return KeyValue( szKeyName, string );
}

bool CSharedBaseEntity::KeyValue( const char *szKeyName, const Vector &vecValue ) 
{
	char	string[256];

	Q_snprintf(string,sizeof(string), "%f %f %f", vecValue.x, vecValue.y, vecValue.z );

	return KeyValue( szKeyName, string );
}

bool CSharedBaseEntity::KeyValue( const char *szKeyName, int nValue ) 
{
	char	string[256];

	Q_snprintf(string,sizeof(string), "%d", nValue );

	return KeyValue( szKeyName, string );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------

bool CSharedBaseEntity::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	if ( FStrEq( szKeyName, "rendercolor" ) )
	{
		color24 tmp = GetRenderColor();
		Q_snprintf( szValue, iMaxLen, "%d %d %d", tmp.r, tmp.g, tmp.b );
		return true;
	}

	if ( FStrEq( szKeyName, "rendercolor24" ) )
	{
		color24 tmp = GetRenderColor();
		Q_snprintf( szValue, iMaxLen, "%d %d %d", tmp.r, tmp.g, tmp.b );
		return true;
	}

	if ( FStrEq( szKeyName, "rendercolor32" ))
	{
		color24 tmp = GetRenderColor();
		unsigned char a = GetRenderAlpha();
		Q_snprintf( szValue, iMaxLen, "%d %d %d %d", tmp.r, tmp.g, tmp.b, a );
		return true;
	}
	
	if ( FStrEq( szKeyName, "renderamt" ) )
	{
		unsigned char a = GetRenderAlpha();
		Q_snprintf( szValue, iMaxLen, "%d", a );
		return true;
	}

	if ( FStrEq( szKeyName, "disableshadows" ))
	{
		Q_snprintf( szValue, iMaxLen, "%d", IsEffectActive( EF_NOSHADOW ) );
		return true;
	}

	if (FStrEq(szKeyName, "disableshadowdepth"))
	{
		Q_snprintf(szValue, iMaxLen, "%d", IsEffectActive(EF_NOSHADOWDEPTH));
		return true;
	}

	if ( FStrEq( szKeyName, "mins" ))
	{
		Assert( 0 );
		return false;
	}

	if ( FStrEq( szKeyName, "maxs" ))
	{
		Assert( 0 );
		return false;
	}

	if ( FStrEq( szKeyName, "disablereceiveshadows" ))
	{
		Q_snprintf( szValue, iMaxLen, "%d", IsEffectActive( EF_NORECEIVESHADOW ) );
		return true;
	}

	if (FStrEq(szKeyName, "disableflashlight"))
	{
		Q_snprintf(szValue, iMaxLen, "%d", IsEffectActive(EF_NOFLASHLIGHT));
		return true;
	}

	if ( FStrEq( szKeyName, "nodamageforces" ))
	{
		Q_snprintf( szValue, iMaxLen, "%d", IsEffectActive( EFL_NO_DAMAGE_FORCES ) );
		return true;
	}

	// Fix up single angles
	if( FStrEq( szKeyName, "angle" ) )
	{
		return false;
	}

	// NOTE: Have to do these separate because they set two values instead of one
	if( FStrEq( szKeyName, "angles" ) )
	{
		QAngle angles = GetAbsAngles();

		Q_snprintf( szValue, iMaxLen, "%f %f %f", angles.x, angles.y, angles.z );
		return true;
	}

	if( FStrEq( szKeyName, "origin" ) )
	{
		Vector vecOrigin = GetAbsOrigin();
		Q_snprintf( szValue, iMaxLen, "%f %f %f", vecOrigin.x, vecOrigin.y, vecOrigin.z );
		return true;
	}

#ifdef GAME_DLL	
	
	if ( FStrEq( szKeyName, "targetname" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%s", STRING( GetEntityName() ) );
		return true;
	}

	if ( FStrEq( szKeyName, "classname" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%s", GetClassname() );
		return true;
	}

	for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
	{
		if ( ::ExtractKeyvalue( this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen ) )
			return true;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( m_CollisionGroup == COLLISION_GROUP_DEBRIS )
	{
		if ( ! (contentsMask & CONTENTS_DEBRIS) )
			return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : seed - 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::SetPredictionRandomSeed( const CUserCmd *cmd )
{
	if ( !cmd )
	{
		m_nPredictionRandomSeed = -1;
#ifdef GAME_DLL
		m_nPredictionRandomSeedServer = -1;
#endif

		return;
	}

	m_nPredictionRandomSeed = ( cmd->random_seed );
#ifdef GAME_DLL
	m_nPredictionRandomSeedServer = ( cmd->server_random_seed );
#endif
}


//------------------------------------------------------------------------------
// Purpose : Base implimentation for entity handling decals
//------------------------------------------------------------------------------
void CSharedBaseEntity::DecalTrace( trace_t *pTrace, char const *decalName )
{
	int index = decalsystem->GetDecalIndexForName( decalName );
	if ( index < 0 )
		return;

	Assert( pTrace->m_pEnt );

	CBroadcastRecipientFilter filter;
	te->Decal( filter, 0.0, &pTrace->endpos, &pTrace->startpos,
		pTrace->GetEntityIndex(), pTrace->hitbox, index );
}

//-----------------------------------------------------------------------------
// Purpose: Base handling for impacts against entities
//-----------------------------------------------------------------------------
void CSharedBaseEntity::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "CSharedBaseEntity::ImpactTrace" );
	Assert( pTrace->m_pEnt );

	CSharedBaseEntity *pEntity = pTrace->m_pEnt;

	// Build the impact data
	CEffectData data;
	data.m_vOrigin = pTrace->endpos;
	data.m_vStart = pTrace->startpos;
	data.m_nSurfaceProp = pTrace->surface.surfaceProps;
	if ( data.m_nSurfaceProp < 0 )
	{
		data.m_nSurfaceProp = 0;
	}
	data.m_nDamageType = iDamageType;
	data.m_nHitBox = pTrace->hitbox;
#ifdef CLIENT_DLL
	data.m_hEntity = ClientEntityList().EntIndexToHandle( pEntity->entindex() );
#else
	data.m_nEntIndex = pEntity->entindex();
#endif

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

//-----------------------------------------------------------------------------
// Purpose: returns the damage decal to use, given a damage type
// Input  : bitsDamageType - the damage type
// Output : the index of the damage decal to use
//-----------------------------------------------------------------------------
char const *CSharedBaseEntity::DamageDecal( int bitsDamageType, int gameMaterial )
{
	if ( GetRenderMode() == kRenderTransAlpha )
		return "";

	if ( GetRenderMode() != kRenderNormal && gameMaterial == 'G' )
		return "BulletProof";

	if ( bitsDamageType == DMG_SLASH )
		return "ManhackCut";

	// This will get translated at a lower layer based on game material
	return "Impact.Concrete";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSharedBaseEntity::GetIndexForThinkContext( const char *pszContext )
{
	for ( int i = 0; i < m_aThinkFunctions.Size(); i++ )
	{
		if ( !Q_strncmp( STRING( m_aThinkFunctions[i].m_iszContext ), pszContext, MAX_CONTEXT_LENGTH ) )
			return i;
	}

	return NO_THINK_CONTEXT;
}

//-----------------------------------------------------------------------------
// Purpose: Get a fresh think context for this entity
//-----------------------------------------------------------------------------
int CSharedBaseEntity::RegisterThinkContext( const char *szContext )
{
	int iIndex = GetIndexForThinkContext( szContext );
	if ( iIndex != NO_THINK_CONTEXT )
		return iIndex;

	// Make a new think func
	thinkfunc_t sNewFunc;
	Q_memset( &sNewFunc, 0, sizeof( sNewFunc ) );
	sNewFunc.m_pfnThink = NULL;
	sNewFunc.m_nNextThinkTick = TICK_NEVER_THINK;
	sNewFunc.m_iszContext = AllocPooledString(szContext);

	// Insert it into our list
	return m_aThinkFunctions.AddToTail( sNewFunc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BASEPTR	CSharedBaseEntity::ThinkSet( BASEPTR func, float thinkTime, const char *szContext )
{
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
#ifdef GNUC
	COMPILE_TIME_ASSERT( sizeof(func) == 8 );
#else
	COMPILE_TIME_ASSERT( sizeof(func) == 4 );
#endif
#endif
#endif

	// Old system?
	if ( !szContext )
	{
		m_pfnThink = func;
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
		FunctionCheck( *(reinterpret_cast<void **>(&m_pfnThink)), "BaseThinkFunc" ); 
#endif
#endif
		return m_pfnThink;
	}

	// Find the think function in our list, and if we couldn't find it, register it
	int iIndex = GetIndexForThinkContext( szContext );
	if ( iIndex == NO_THINK_CONTEXT )
	{
		iIndex = RegisterThinkContext( szContext );
	}

	m_aThinkFunctions[ iIndex ].m_pfnThink = func;
#if !defined( CLIENT_DLL )
#ifdef _DEBUG
	FunctionCheck( *(reinterpret_cast<void **>(&m_aThinkFunctions[ iIndex ].m_pfnThink)), szContext ); 
#endif
#endif

	if ( thinkTime != 0 )
	{
		int thinkTick;

		if(thinkTime == TICK_NEVER_THINK)
			thinkTick = TICK_NEVER_THINK;
		else if(thinkTime == TICK_ALWAYS_THINK)
			thinkTick = TICK_ALWAYS_THINK;
		else
			thinkTick = TIME_TO_TICKS( thinkTime );

		m_aThinkFunctions[ iIndex ].m_nNextThinkTick = thinkTick;
		CheckHasThinkFunction( thinkTick );
	}
	return func;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::SetNextThink( float thinkTime, const char *szContext )
{
	int thinkTick;

	if(thinkTime == TICK_NEVER_THINK)
		thinkTick = TICK_NEVER_THINK;
	else if(thinkTime == TICK_ALWAYS_THINK)
		thinkTick = TICK_ALWAYS_THINK;
	else
		thinkTick = TIME_TO_TICKS( thinkTime );

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Setting base think function within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif

		// Old system
		m_nNextThinkTick = thinkTick;
		CheckHasThinkFunction( thinkTick );
		return;
	}
	else
	{
		// Find the think function in our list, and if we couldn't find it, register it
		iIndex = GetIndexForThinkContext( szContext );
		if ( iIndex == NO_THINK_CONTEXT )
		{
			iIndex = RegisterThinkContext( szContext );
		}
	}

	// Old system
	m_aThinkFunctions[ iIndex ].m_nNextThinkTick = thinkTick;
	CheckHasThinkFunction( thinkTick );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CSharedBaseEntity::GetNextThink( const char *szContext )
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return TICK_NEVER_THINK;

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif

		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;
		else if ( m_nNextThinkTick == TICK_ALWAYS_THINK )
			return gpGlobals->curtime;
		else
			return m_nNextThinkTick * TICK_INTERVAL;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );
	}

	if ( iIndex == m_aThinkFunctions.InvalidIndex() )
		return TICK_NEVER_THINK;

	if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_NEVER_THINK )
		return TICK_NEVER_THINK;
	else if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_ALWAYS_THINK )
		return gpGlobals->curtime;
	else
		return m_aThinkFunctions[ iIndex ].m_nNextThinkTick * TICK_INTERVAL;
}

int	CSharedBaseEntity::GetNextThinkTick( const char *szContext /*= NULL*/ )
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return TICK_NEVER_THINK;

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif

		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;
		else if ( m_nNextThinkTick == TICK_ALWAYS_THINK )
			return gpGlobals->tickcount;
		else
			return m_nNextThinkTick;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );

		// Looking up an invalid think context!
		Assert( iIndex != -1 );
	}

	if ( iIndex == -1 )
	{
		return TICK_NEVER_THINK;
	}

	if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_NEVER_THINK )
		return TICK_NEVER_THINK;
	else if ( m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_ALWAYS_THINK )
		return gpGlobals->tickcount;
	else
		return m_aThinkFunctions[ iIndex ].m_nNextThinkTick;
}

bool	CSharedBaseEntity::AlwaysThink( const char *szContext /*= NULL*/ )
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return false;

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif

		return (m_nNextThinkTick == TICK_ALWAYS_THINK);
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );

		// Looking up an invalid think context!
		Assert( iIndex != -1 );
	}

	if ( iIndex == -1 )
	{
		return false;
	}

	return (m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_ALWAYS_THINK);
}

bool	CSharedBaseEntity::NeverThink( const char *szContext /*= NULL*/ )
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return true;

	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base nextthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif

		return (m_nNextThinkTick == TICK_NEVER_THINK);
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );

		// Looking up an invalid think context!
		Assert( iIndex != -1 );
	}

	if ( iIndex == -1 )
	{
		return true;
	}

	return (m_aThinkFunctions[ iIndex ].m_nNextThinkTick == TICK_NEVER_THINK);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CSharedBaseEntity::GetLastThink( const char *szContext )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base lastthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif
		if(m_nLastThinkTick == TICK_NEVER_THINK)
			return TICK_NEVER_THINK;
		else if(m_nLastThinkTick == TICK_ALWAYS_THINK)
			return TICK_NEVER_THINK;
		else
			return m_nLastThinkTick * TICK_INTERVAL;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );
	}

	if(m_aThinkFunctions[ iIndex ].m_nLastThinkTick == TICK_NEVER_THINK)
		return TICK_NEVER_THINK;
	else if(m_aThinkFunctions[ iIndex ].m_nLastThinkTick == TICK_ALWAYS_THINK)
		return TICK_NEVER_THINK;
	else
		return m_aThinkFunctions[ iIndex ].m_nLastThinkTick * TICK_INTERVAL;
}
	
int CSharedBaseEntity::GetLastThinkTick( const char *szContext /*= NULL*/ )
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
#ifdef _DEBUG
		if ( m_iCurrentThinkContext != NO_THINK_CONTEXT )
		{
			Msg( "Warning: Getting base lastthink time within think context %s\n", STRING(m_aThinkFunctions[m_iCurrentThinkContext].m_iszContext) );
		}
#endif
		if(m_nLastThinkTick == TICK_NEVER_THINK)
			return TICK_NEVER_THINK;
		else if(m_nLastThinkTick == TICK_ALWAYS_THINK)
			return TICK_NEVER_THINK;
		else
			return m_nLastThinkTick;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );
	}

	if(m_aThinkFunctions[ iIndex ].m_nLastThinkTick == TICK_NEVER_THINK)
		return TICK_NEVER_THINK;
	else if(m_aThinkFunctions[ iIndex ].m_nLastThinkTick == TICK_ALWAYS_THINK)
		return TICK_NEVER_THINK;
	else
		return m_aThinkFunctions[ iIndex ].m_nLastThinkTick;
}

bool CSharedBaseEntity::WillThink()
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return false;

	if ( m_nNextThinkTick > 0 || m_nNextThinkTick == TICK_ALWAYS_THINK )
		return true;

	for ( int i = 0; i < m_aThinkFunctions.Count(); i++ )
	{
		if ( m_aThinkFunctions[i].m_nNextThinkTick > 0 || m_aThinkFunctions[i].m_nNextThinkTick == TICK_ALWAYS_THINK )
			return true;
	}

	return false;
}

#if !defined( CLIENT_DLL )

// Rebase all the current ticks in the think functions as delta ticks or from delta ticks to absolute ticks
void CSharedBaseEntity::RebaseThinkTicks( bool bMakeDeltas )
{
	int nCurTick = TIME_TO_TICKS( gpGlobals->curtime );
	for ( int i = 0; i < m_aThinkFunctions.Count(); i++ )
	{
		if ( m_aThinkFunctions[i].m_nNextThinkTick > 0 )
		{
			if ( bMakeDeltas )
			{
				// Turn into a delta value
				m_aThinkFunctions[i].m_nNextThinkTick = m_aThinkFunctions[i].m_nNextThinkTick - nCurTick;
				m_aThinkFunctions[i].m_nLastThinkTick = m_aThinkFunctions[i].m_nLastThinkTick - nCurTick;
			}
			else
			{
				// Change a delta to an absolute tick value
				m_aThinkFunctions[i].m_nNextThinkTick = m_aThinkFunctions[i].m_nNextThinkTick + nCurTick;
				m_aThinkFunctions[i].m_nLastThinkTick = m_aThinkFunctions[i].m_nLastThinkTick + nCurTick;
			}
		}
	}
}

#endif // !CLIENT_DLL

// returns the first tick the entity will run any think function
// returns TICK_NEVER_THINK if no think functions are scheduled
int CSharedBaseEntity::GetFirstThinkTick()
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return TICK_NEVER_THINK;

	int minTick;
	if(m_nNextThinkTick == TICK_NEVER_THINK)
		minTick = TICK_NEVER_THINK;
	else if(m_nNextThinkTick == TICK_ALWAYS_THINK)
		return gpGlobals->tickcount;
	else
		minTick = m_nNextThinkTick;

	for ( int i = 0; i < m_aThinkFunctions.Count(); i++ )
	{
		int next = m_aThinkFunctions[i].m_nNextThinkTick;
		if(next == TICK_NEVER_THINK)
			continue;
		else if(next == TICK_ALWAYS_THINK)
			return gpGlobals->tickcount;
		else
			if ( next < minTick || minTick == TICK_NEVER_THINK )
				minTick = next;
	}
	return minTick;
}

// NOTE: pass in the isThinking hint so we have to search the think functions less
void CSharedBaseEntity::CheckHasThinkFunction( int thinkTick )
{
	bool isThinking = (thinkTick != TICK_NEVER_THINK);

	if ( IsEFlagSet( EFL_NO_THINK_FUNCTION ) && isThinking )
	{
		RemoveEFlags( EFL_NO_THINK_FUNCTION );

	#ifdef CLIENT_DLL
		Assert( GetClientHandle() != INVALID_CLIENTENTITY_HANDLE );
		if(thinkTick == TICK_NEVER_THINK)
			ClientThinkList()->RemoveThinkable( GetClientHandle() );
		else if(thinkTick == TICK_ALWAYS_THINK)
			ClientThinkList()->SetNextClientThink( GetClientHandle(), TICK_ALWAYS_THINK );
		else
			ClientThinkList()->SetNextClientThink( GetClientHandle(), thinkTick * TICK_INTERVAL );
	#endif
	}
	else if ( !isThinking && !IsEFlagSet( EFL_NO_THINK_FUNCTION ) && !WillThink() )
	{
		AddEFlags( EFL_NO_THINK_FUNCTION );

	#ifdef CLIENT_DLL
		Assert( GetClientHandle() != INVALID_CLIENTENTITY_HANDLE );
		ClientThinkList()->RemoveThinkable( GetClientHandle() );
	#endif
	}
#if !defined( CLIENT_DLL )
	SimThink_EntityChanged( this );
#endif
}

bool CSharedBaseEntity::WillSimulateGamePhysics()
{
	// players always simulate game physics
	if ( !IsPlayer() )
	{
		MoveType_t movetype = GetMoveType();
		
		if ( movetype == MOVETYPE_NONE || movetype == MOVETYPE_VPHYSICS )
			return false;

#if !defined( CLIENT_DLL )
		// MOVETYPE_PUSH not supported on the client
		if ( movetype == MOVETYPE_PUSH && GetMoveDoneTime() <= 0 )
			return false;
#endif
	}

	return true;
}

void CSharedBaseEntity::CheckHasGamePhysicsSimulation()
{
	bool isSimulating = WillSimulateGamePhysics();
	if ( isSimulating != IsEFlagSet(EFL_NO_GAME_PHYSICS_SIMULATION) )
		return;
	if ( isSimulating )
	{
		RemoveEFlags( EFL_NO_GAME_PHYSICS_SIMULATION );
	}
	else
	{
		AddEFlags( EFL_NO_GAME_PHYSICS_SIMULATION );
	}
#if !defined( CLIENT_DLL )
	SimThink_EntityChanged( this );
#endif
}

//-----------------------------------------------------------------------------
// Sets/Gets the next think based on context index
//-----------------------------------------------------------------------------
void CSharedBaseEntity::SetNextThink( int nContextIndex, float thinkTime )
{
	int thinkTick;

	if(thinkTime == TICK_NEVER_THINK)
		thinkTick = TICK_NEVER_THINK;
	else if(thinkTime == TICK_ALWAYS_THINK)
		thinkTick = TICK_ALWAYS_THINK;
	else
		thinkTick = TIME_TO_TICKS( thinkTime );

	if (nContextIndex < 0)
	{
		SetNextThink( thinkTime );
	}
	else
	{
		m_aThinkFunctions[nContextIndex].m_nNextThinkTick = thinkTick;
	}
	CheckHasThinkFunction( thinkTick );
}

void CSharedBaseEntity::SetLastThink( int nContextIndex, float thinkTime )
{
	int thinkTick;

	if(thinkTime == TICK_NEVER_THINK)
		thinkTick = TICK_NEVER_THINK;
	else if(thinkTime == TICK_ALWAYS_THINK)
		thinkTick = TICK_NEVER_THINK;
	else
		thinkTick = TIME_TO_TICKS( thinkTime );

	if (nContextIndex < 0)
	{
		m_nLastThinkTick = thinkTick;
	}
	else
	{
		m_aThinkFunctions[nContextIndex].m_nLastThinkTick = thinkTick;
	}
}

float CSharedBaseEntity::GetNextThink( int nContextIndex ) const
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return TICK_NEVER_THINK;

	if (nContextIndex < 0)
	{
		if(m_nNextThinkTick == TICK_NEVER_THINK)
			return TICK_NEVER_THINK;
		else if(m_nNextThinkTick == TICK_ALWAYS_THINK)
			return gpGlobals->curtime;
		else
			return m_nNextThinkTick * TICK_INTERVAL;
	}

	if(m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_NEVER_THINK)
		return TICK_NEVER_THINK;
	else if(m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_ALWAYS_THINK)
		return gpGlobals->curtime;
	else
		return m_aThinkFunctions[nContextIndex].m_nNextThinkTick * TICK_INTERVAL; 
}

int	CSharedBaseEntity::GetNextThinkTick( int nContextIndex ) const
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return TICK_NEVER_THINK;

	if (nContextIndex < 0)
	{
		if(m_nNextThinkTick == TICK_NEVER_THINK)
			return TICK_NEVER_THINK;
		else if(m_nNextThinkTick == TICK_ALWAYS_THINK)
			return gpGlobals->tickcount;
		else
			return m_nNextThinkTick;
	}

	if(m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_NEVER_THINK)
		return TICK_NEVER_THINK;
	else if(m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_ALWAYS_THINK)
		return gpGlobals->tickcount;
	else
		return m_aThinkFunctions[nContextIndex].m_nNextThinkTick;
}

bool	CSharedBaseEntity::AlwaysThink( int nContextIndex ) const
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return false;

	if (nContextIndex < 0)
	{
		return (m_nNextThinkTick == TICK_ALWAYS_THINK);
	}

	return (m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_NEVER_THINK);
}

bool	CSharedBaseEntity::NeverThink( int nContextIndex ) const
{
	if(IsEFlagSet( EFL_NO_THINK_FUNCTION ))
		return true;

	if (nContextIndex < 0)
	{
		return (m_nNextThinkTick == TICK_NEVER_THINK);
	}

	return (m_aThinkFunctions[nContextIndex].m_nNextThinkTick == TICK_NEVER_THINK);
}

int CheckEntityVelocity( Vector &v )
{
	float r = k_flMaxEntitySpeed;
	if (
		v.x > -r && v.x < r &&
		v.y > -r && v.y < r &&
		v.z > -r && v.z < r)
	{
		// The usual case.  It's totally reasonable
		return 1;
	}
	float speed = v.Length();
	if ( speed < k_flMaxEntitySpeed * 100.0f )
	{
		// Sort of suspicious.  Clamp it
		v *= k_flMaxEntitySpeed / speed;
		return 0;
	}

	// A terrible, horrible, no good, very bad velocity.
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: My physics object has been updated, react or extract data
//-----------------------------------------------------------------------------
void CSharedBaseEntity::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	switch( GetMoveType() )
	{
	case MOVETYPE_VPHYSICS:
		{
			if ( GetMoveParent() )
			{
				DevWarning("Updating physics on object in hierarchy %s!\n", GetClassname());
				return;
			}
			Vector origin;
			QAngle angles;

			pPhysics->GetPosition( &origin, &angles );

			if ( !IsEntityQAngleReasonable( angles ) )
			{
				if ( CheckEmitReasonablePhysicsSpew() )
				{
					Warning( "Ignoring bogus angles (%f,%f,%f) from vphysics! (entity %s)\n", angles.x, angles.y, angles.z, GetDebugName() );
				}
				angles = vec3_angle;
			}

			for ( int i = 0; i < 3; ++i )
			{
				angles[ i ] = AngleNormalize( angles[ i ] );
			}

#ifndef CLIENT_DLL 
			NetworkQuantize( origin, angles );
#endif

#ifndef CLIENT_DLL 
			Vector prevOrigin = GetAbsOrigin();
#endif

			if ( IsEntityPositionReasonable( origin ) )
			{
				SetAbsOrigin( origin );
			}
			else
			{
				if ( CheckEmitReasonablePhysicsSpew() )
				{
					Warning( "Ignoring unreasonable position (%f,%f,%f) from vphysics! (entity %s)\n", origin.x, origin.y, origin.z, GetDebugName() );
				}
			}

			SetAbsAngles( angles );

			// Interactive debris converts back to debris when it comes to rest
			if ( pPhysics->IsAsleep() && GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
			{
				SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			}

#ifndef CLIENT_DLL 
			PhysicsTouchTriggers( &prevOrigin );
			PhysicsRelinkChildren(gpGlobals->frametime);
#endif
		}
	break;

	case MOVETYPE_STEP:
		break;

	case MOVETYPE_PUSH:
#ifndef CLIENT_DLL
		VPhysicsUpdatePusher( pPhysics );
#endif
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Init this object's physics as a static
//-----------------------------------------------------------------------------
IPhysicsObject *CSharedBaseEntity::VPhysicsInitStatic( void )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

#ifndef CLIENT_DLL
	// If this entity has a move parent, it needs to be shadow, not static
	if ( GetMoveParent() )
	{
		// must be SOLID_VPHYSICS if in hierarchy to solve collisions correctly
		if ( GetSolid() == SOLID_BSP && GetRootMoveParent()->GetSolid() != SOLID_BSP )
		{
			SetSolid( SOLID_VPHYSICS );
		}

		return VPhysicsInitShadow( false, false );
	}
#endif

	// No physics
	if ( GetSolid() == SOLID_NONE )
		return NULL;

	// create a static physics objct
	IPhysicsObject *pPhysicsObject = NULL;
	if ( GetSolid() == SOLID_BBOX )
	{
		pPhysicsObject = PhysModelCreateBox( this, WorldAlignMins(), WorldAlignMaxs(), GetAbsOrigin(), true );
	}
	else
	{
		pPhysicsObject = PhysModelCreateUnmoveable( this, GetModelIndex(), GetAbsOrigin(), GetAbsAngles() );
	}
	VPhysicsSetObject( pPhysicsObject );
	return pPhysicsObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysics - 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::VPhysicsSetObject( IPhysicsObject *pPhysics )
{
	if ( m_pPhysicsObject && pPhysics )
	{
		Warning( "Overwriting physics object for %s\n", GetClassname() );
	}
	m_pPhysicsObject = pPhysics;
#ifndef CLIENT_DLL
	RemoveSolidFlags(FSOLID_NOT_MOVEABLE);
#endif
	if ( m_pPhysicsObject )
	{
#ifndef CLIENT_DLL
		if ( m_pPhysicsObject->IsStatic() )
		{
			AddSolidFlags(FSOLID_NOT_MOVEABLE);
		}
#endif
	}
	if ( pPhysics && !m_pPhysicsObject )
	{
		CollisionRulesChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::VPhysicsDestroyObject( void )
{
	if ( m_pPhysicsObject )
	{
#ifndef CLIENT_DLL
		PhysRemoveShadow( this );
#endif
		PhysDestroyObject( m_pPhysicsObject, this );
		m_pPhysicsObject = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::VPhysicsInitSetup()
{
	if ( IsMarkedForDeletion() )
		return false;

	// If this entity already has a physics object, then it should have been deleted prior to making this call.
	Assert(!m_pPhysicsObject);
	VPhysicsDestroyObject();

	// make sure absorigin / absangles are correct
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This creates a normal vphysics simulated object
//			physics alone determines where it goes (gravity, friction, etc)
//			and the entity receives updates from vphysics.  SetAbsOrigin(), etc do not affect the object!
//-----------------------------------------------------------------------------
IPhysicsObject *CSharedBaseEntity::VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

	// NOTE: This has to occur before PhysModelCreate because that call will
	// call back into ShouldCollide(), which uses solidtype for rules.
	SetSolid( solidType );
	SetSolidFlags( nSolidFlags );

	// No physics
	if ( solidType == SOLID_NONE )
	{
		return NULL;
	}

	// create a normal physics object
	IPhysicsObject *pPhysicsObject = PhysModelCreate( this, GetModelIndex(), GetAbsOrigin(), GetAbsAngles(), pSolid );
	if ( pPhysicsObject )
	{
		VPhysicsSetObject( pPhysicsObject );
		SetMoveType( MOVETYPE_VPHYSICS );

		if ( !createAsleep )
		{
			pPhysicsObject->Wake();
		}
	}

	return pPhysicsObject;
}

// This creates a vphysics object with a shadow controller that follows the AI
IPhysicsObject *CSharedBaseEntity::VPhysicsInitShadow( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid )
{
	if ( !VPhysicsInitSetup() )
		return NULL;

	// No physics
	if ( GetSolid() == SOLID_NONE )
		return NULL;

	const Vector &origin = GetAbsOrigin();
	QAngle angles = GetAbsAngles();
	IPhysicsObject *pPhysicsObject = NULL;

	if ( GetSolid() == SOLID_BBOX )
	{
		// adjust these so the game tracing epsilons match the physics minimum separation distance
		// this will shrink the vphysics version of the model by the difference in epsilons
		float radius = 0.25f - DIST_EPSILON;
		Vector mins = WorldAlignMins() + Vector(radius, radius, radius);
		Vector maxs = WorldAlignMaxs() - Vector(radius, radius, radius);
		pPhysicsObject = PhysModelCreateBox( this, mins, maxs, origin, false );
		angles = vec3_angle;
	}
	else if ( GetSolid() == SOLID_OBB )
	{
		pPhysicsObject = PhysModelCreateOBB( this, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), origin, angles, false );
	}
	else
	{
		pPhysicsObject = PhysModelCreate( this, GetModelIndex(), origin, angles, pSolid );
	}
	if ( !pPhysicsObject )
		return NULL;

	VPhysicsSetObject( pPhysicsObject );
	// UNDONE: Tune these speeds!!!
	pPhysicsObject->SetShadow( 1e4, 1e4, allowPhysicsMovement, allowPhysicsRotation );
	pPhysicsObject->UpdateShadow( origin, angles, false, 0 );
	return pPhysicsObject;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::CreateVPhysics()
{
	return false;
}

bool CSharedBaseEntity::IsStandable() const
{
	if (GetSolidFlags() & FSOLID_NOT_STANDABLE) 
		return false;

	if ( GetSolid() == SOLID_BSP || GetSolid() == SOLID_VPHYSICS || GetSolid() == SOLID_BBOX )
		return true;

	return IsBSPModel( ); 
}

bool CSharedBaseEntity::IsBSPModel() const
{
	if ( GetSolid() == SOLID_BSP )
		return true;
	
	const model_t *model = modelinfo->GetModel( GetModelIndex() );

	if ( GetSolid() == SOLID_VPHYSICS && modelinfo->GetModelType( model ) == mod_brush )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Invalidates the abs state of all children
//-----------------------------------------------------------------------------
void CSharedBaseEntity::InvalidatePhysicsRecursive( int nChangeFlags )
{
	// Main entry point for dirty flag setting for the 90% case
	// 1) If the origin changes, then we have to update abstransform, Shadow projection, PVS, KD-tree, 
	//    client-leaf system.
	// 2) If the angles change, then we have to update abstransform, Shadow projection,
	//    shadow render-to-texture, client-leaf system, and surrounding bounds. 
	//	  Children have to additionally update absvelocity, KD-tree, and PVS.
	//	  If the surrounding bounds actually update, when we also need to update the KD-tree and the PVS.
	// 3) If it's due to attachment, then all children who are attached to an attachment point
	//    are assumed to have dirty origin + angles.

	// Other stuff:
	// 1) Marking the surrounding bounds dirty will automatically mark KD tree + PVS dirty.
	
	int nDirtyFlags = 0;

	int nChildrenChangeFlags = (nChangeFlags & (POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED));

	if ( (nChangeFlags & VELOCITY_CHANGED) != 0 )
	{
		nDirtyFlags |= EFL_DIRTY_ABSVELOCITY;
	}

	if ( (nChangeFlags & POSITION_CHANGED) != 0 )
	{
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

#ifndef CLIENT_DLL
		if( NetworkProp() )
			NetworkProp()->MarkPVSInformationDirty();
#endif

		// NOTE: This will also mark shadow projection + client leaf dirty
		if ( !IsWorld() && !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
		{
			CollisionProp()->MarkPartitionHandleDirty();
		}
	}

	bool bSurroundDirty = false;

	// NOTE: This has to be done after velocity + position are changed
	// because we change the nChangeFlags for the child entities
	if ( (nChangeFlags & ANGLES_CHANGED) != 0 )
	{
		nDirtyFlags |= EFL_DIRTY_ABSTRANSFORM;

		if ( !bSurroundDirty && !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
		{
			if ( CollisionProp()->DoesRotationInvalidateSurroundingBox() )
			{
				// NOTE: This will handle the KD-tree, surrounding bounds, PVS
				// render-to-texture shadow, shadow projection, and client leaf dirty
				bSurroundDirty = true;
			}
		}

		nChildrenChangeFlags |= (POSITION_CHANGED | VELOCITY_CHANGED);
	}

	if ( (nChangeFlags & SEQUENCE_CHANGED) != 0 )
	{
		if ( !bSurroundDirty && !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
		{
			if ( CollisionProp()->DoesSequenceChangeInvalidateSurroundingBox() )
			{
				// NOTE: This will handle the KD-tree, surrounding bounds, PVS
				// render-to-texture shadow, shadow projection, and client leaf dirty
				bSurroundDirty = true;
			}
		}
	}

	if ( (nChangeFlags & ANIMATION_CHANGED) != 0 )
	{
		nChildrenChangeFlags |= (POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED);
	}

	if ( (nChangeFlags & BOUNDS_CHANGED) != 0 )
	{
		nChildrenChangeFlags |= (POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED);
	}

	if( bSurroundDirty && !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
	{
		CollisionProp()->MarkSurroundingBoundsDirty();
	}
#ifdef CLIENT_DLL
	else
	{
		if ( !IsWorld() && !IsEFlagSet( EFL_NOT_RENDERABLE ) )
		{
			if((nChangeFlags & (POSITION_CHANGED | ANGLES_CHANGED | BOUNDS_CHANGED)) != 0)
			{
				MarkRenderHandleDirty();
				g_pClientShadowMgr->AddToDirtyShadowList( this );
			}

			if((nChangeFlags & (POSITION_CHANGED | ANGLES_CHANGED | BOUNDS_CHANGED)) != 0 || (nChangeFlags & ANIMATION_CHANGED) != 0)
			{
				g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetShadowHandle() );
			}
		}
	}
#endif

	AddEFlags( nDirtyFlags );

	for (CSharedBaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
	{
		// If this is due to the parent animating, only invalidate children that are parented to an attachment
		// Entities that are following also access attachments points on parents and must be invalidated.
		if ( (nChangeFlags & ANIMATION_CHANGED) != 0 && (nChangeFlags & (POSITION_CHANGED | VELOCITY_CHANGED | ANGLES_CHANGED)) == 0 )
		{
#ifdef CLIENT_DLL
			if ( (pChild->GetParentAttachment() == 0) && !pChild->IsFollowingEntity() )
				continue;
#else
			if ( pChild->GetParentAttachment() == 0 )
				continue;
#endif
		}
		pChild->InvalidatePhysicsRecursive( nChildrenChangeFlags );
	}

	if ( (nChangeFlags & (POSITION_CHANGED | ANGLES_CHANGED | ANIMATION_CHANGED)) != 0 )
	{
		CSharedBaseAnimating *pAnim = GetBaseAnimating();
		if ( pAnim )
			pAnim->InvalidateBoneCache();
	}
}



//-----------------------------------------------------------------------------
// Returns the highest parent of an entity
//-----------------------------------------------------------------------------
CSharedBaseEntity *CSharedBaseEntity::GetRootMoveParent()
{
	CSharedBaseEntity *pEntity = this;
	CSharedBaseEntity *pParent = this->GetMoveParent();
	while ( pParent )
	{
		pEntity = pParent;
		pParent = pEntity->GetMoveParent();
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: static method
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::IsPrecacheAllowed()
{
	return m_bAllowPrecache;
}

//-----------------------------------------------------------------------------
// Purpose: static method
// Input  : allow - 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::SetAllowPrecache( bool allow )
{
	m_bAllowPrecache = allow;
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/

#if defined( GAME_DLL )
class CBulletsTraceFilter : public CTraceFilterSimpleList
{
public:
	CBulletsTraceFilter( int collisionGroup ) : CTraceFilterSimpleList( collisionGroup ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( m_PassEntities.Count() )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
			CBaseEntity *pPassEntity = EntityFromEntityHandle( m_PassEntities[0] );
			if ( pEntity && pPassEntity && pEntity->GetOwnerEntity() == pPassEntity && 
				pPassEntity->IsSolidFlagSet(FSOLID_NOT_SOLID) && pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) && 
				pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ) )
			{
				// It's a bone follower of the entity to ignore (toml 8/3/2007)
				return false;
			}
		}
		return CTraceFilterSimpleList::ShouldHitEntity( pHandleEntity, contentsMask );
	}

};
#else
typedef CTraceFilterSimpleList CBulletsTraceFilter;
#endif

void CSharedBaseEntity::FireBullets( const FireBulletsInfo_t &info )
{
	static int	tracerCount;
	trace_t		tr;
	CAmmoDef*	pAmmoDef	= GetAmmoDef();
	int			nDamageType	= pAmmoDef->DamageType(info.m_iAmmoType);
	int			nAmmoFlags	= pAmmoDef->Flags(info.m_iAmmoType);
	
	bool bDoServerEffects = true;

#if defined( GAME_DLL )
	if( IsPlayer() )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>(this);

		int rumbleEffect = pPlayer->GetActiveWeapon()->GetRumbleEffect();

		if( rumbleEffect != RUMBLE_INVALID )
		{
			if( rumbleEffect == RUMBLE_SHOTGUN_SINGLE )
			{
				if( info.m_iShots == 12 )
				{
					// Upgrade to double barrel rumble effect
					rumbleEffect = RUMBLE_SHOTGUN_DOUBLE;
				}
			}

			pPlayer->RumbleEffect( rumbleEffect, 0, RUMBLE_FLAG_RESTART );
		}
	}
#endif// GAME_DLL

	float flPlayerDamage = info.m_flPlayerDamage;
	if ( flPlayerDamage == 0.0f )
	{
		if ( nAmmoFlags & AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER )
		{
			flPlayerDamage = pAmmoDef->PlrDamage( info.m_iAmmoType );
		}
	}

	// the default attacker is ourselves
	CSharedBaseEntity *pAttacker = info.m_pAttacker ? info.m_pAttacker : this;

	// Make sure we don't have a dangling damage target from a recursive call
	if ( g_MultiDamage.GetTarget() != NULL )
	{
		ApplyMultiDamage();
	}
	  
	ClearMultiDamage();
	g_MultiDamage.SetDamageType( nDamageType | DMG_NEVERGIB );

	Vector vecDir;
	Vector vecEnd;
	
	// Skip multiple entities when tracing
	CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
	traceFilter.SetPassEntity( this ); // Standard pass entity for THIS so that it can be easily removed from the list after passing through a portal
	traceFilter.AddEntityToIgnore( info.m_pAdditionalIgnoreEnt );

#if defined( GAME_DLL )
	// FIXME: We need to emulate this same behavior on the client as well -- jdw
	// Also ignore a vehicle we're a passenger in
	if ( MyCombatCharacterPointer() != NULL && MyCombatCharacterPointer()->IsInAVehicle() )
	{
		traceFilter.AddEntityToIgnore( MyCombatCharacterPointer()->GetVehicleEntity() );
	}
#endif // SERVER_DLL

	if (info.m_pIgnoreEntList != NULL)
	{
		for (int i = 0; i < info.m_pIgnoreEntList->Count(); i++)
		{
			if (info.m_pIgnoreEntList->Element(i))
			{
				traceFilter.AddEntityToIgnore(info.m_pIgnoreEntList->Element(i));
			}
		}
	}

	bool bUnderwaterBullets = ShouldDrawUnderwaterBulletBubbles();
	bool bStartedInWater = false;
	if ( bUnderwaterBullets )
	{
		bStartedInWater = ( enginetrace->GetPointContents( info.m_vecSrc, MASK_WATER ) & (CONTENTS_WATER|CONTENTS_SLIME) ) != 0;
	}

	// Prediction is only usable on players
	int iSeed = 0;
	if ( IsPlayer() )
	{
		iSeed = CSharedBaseEntity::GetPredictionRandomSeed( info.m_bUseServerRandomSeed ) & 255;
	}

#if defined( HL2MP ) && defined( GAME_DLL )
	int iEffectSeed = iSeed;
#endif
	//-----------------------------------------------------
	// Set up our shot manipulator.
	//-----------------------------------------------------
	CShotManipulator Manipulator( info.m_vecDirShooting );

	bool bDoImpacts = false;
	bool bDoTracers = false;
	
	float flCumulativeDamage = 0.0f;

	for (int iShot = 0; iShot < info.m_iShots; iShot++)
	{
		bool bHitWater = false;
		bool bHitGlass = false;

		// Prediction is only usable on players
		if ( IsPlayer() )
		{
			RandomSeed( iSeed );	// init random system with this seed
		}

		// If we're firing multiple shots, and the first shot has to be bang on target, ignore spread
		if ( iShot == 0 && info.m_iShots > 1 && (info.m_nFlags & FIRE_BULLETS_FIRST_SHOT_ACCURATE) )
		{
			vecDir = Manipulator.GetShotDirection();
		}
		else
		{

			// Don't run the biasing code for the player at the moment.
			vecDir = Manipulator.ApplySpread( info.m_vecSpread );
		}

		vecEnd = info.m_vecSrc + vecDir * info.m_flDistance;

#ifdef PORTAL
		CProp_Portal *pShootThroughPortal = NULL;
		float fPortalFraction = 2.0f;
#endif


		if( IsPlayer() && info.m_iShots > 1 && iShot % 2 )
		{
			// Half of the shotgun pellets are hulls that make it easier to hit targets with the shotgun.
#ifdef PORTAL
			Ray_t rayBullet;
			rayBullet.Init( info.m_vecSrc, vecEnd );
			pShootThroughPortal = UTIL_Portal_FirstAlongRay( rayBullet, fPortalFraction );
			if ( !UTIL_Portal_TraceRay_Bullets( pShootThroughPortal, rayBullet, MASK_SHOT, &traceFilter, &tr ) )
			{
				pShootThroughPortal = NULL;
			}
#else
			AI_TraceHull( info.m_vecSrc, vecEnd, Vector( -3, -3, -3 ), Vector( 3, 3, 3 ), MASK_SHOT, &traceFilter, &tr );
#endif //#ifdef PORTAL
		}
		else
		{
#ifdef PORTAL
			Ray_t rayBullet;
			rayBullet.Init( info.m_vecSrc, vecEnd );
			pShootThroughPortal = UTIL_Portal_FirstAlongRay( rayBullet, fPortalFraction );
			if ( !UTIL_Portal_TraceRay_Bullets( pShootThroughPortal, rayBullet, MASK_SHOT, &traceFilter, &tr ) )
			{
				pShootThroughPortal = NULL;
			}
#elif TF_DLL
			CTraceFilterIgnoreFriendlyCombatItems traceFilterCombatItem( this, COLLISION_GROUP_NONE, GetTeamNumber() );
			if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
			{
				CTraceFilterChain traceFilterChain( &traceFilter, &traceFilterCombatItem );
				AI_TraceLine(info.m_vecSrc, vecEnd, MASK_SHOT, &traceFilterChain, &tr);
			}
			else
			{
				AI_TraceLine(info.m_vecSrc, vecEnd, MASK_SHOT, &traceFilter, &tr);
			}
#else
			AI_TraceLine(info.m_vecSrc, vecEnd, MASK_SHOT, &traceFilter, &tr);
#endif //#ifdef PORTAL
		}

		// Tracker 70354/63250:  ywb 8/2/07
		// Fixes bug where trace from turret with attachment point outside of Vcollide
		//  starts solid so doesn't hit anything else in the world and the final coord 
		//  is outside of the MAX_COORD_FLOAT range.  This cause trying to send the end pos
		//  of the tracer down to the client with an origin which is out-of-range for networking
		if ( tr.startsolid )
		{
			tr.endpos = tr.startpos;
			tr.fraction = 0.0f;
		}

	// bullet's final direction can be changed by passing through a portal
#ifdef PORTAL
		if ( !tr.startsolid )
		{
			vecDir = tr.endpos - tr.startpos;
			VectorNormalize( vecDir );
		}
#endif

#ifdef GAME_DLL
		if ( ai_debug_shoot_positions.GetBool() )
			NDebugOverlay::Line(info.m_vecSrc, vecEnd, 255, 255, 255, false, .1 );
#endif

		if ( bStartedInWater )
		{
#ifdef GAME_DLL
			Vector vBubbleStart = info.m_vecSrc;
			Vector vBubbleEnd = tr.endpos;

#ifdef PORTAL
			if ( pShootThroughPortal )
			{
				vBubbleEnd = info.m_vecSrc + ( vecEnd - info.m_vecSrc ) * fPortalFraction;
			}
#endif //#ifdef PORTAL

			CreateBubbleTrailTracer( vBubbleStart, vBubbleEnd, vecDir );
			
#ifdef PORTAL
			if ( pShootThroughPortal )
			{
				Vector vTransformedIntersection;
				UTIL_Portal_PointTransform( pShootThroughPortal->MatrixThisToLinked(), vBubbleEnd, vTransformedIntersection );

				CreateBubbleTrailTracer( vTransformedIntersection, tr.endpos, vecDir );
			}
#endif //#ifdef PORTAL

#endif //#ifdef GAME_DLL
			bHitWater = true;
		}

		// Now hit all triggers along the ray that respond to shots...
		// Clip the ray to the first collided solid returned from traceline
		CSharedTakeDamageInfo triggerInfo( pAttacker, pAttacker, info.m_flDamage, nDamageType );
		CalculateBulletDamageForce( &triggerInfo, info.m_iAmmoType, vecDir, tr.endpos );
		triggerInfo.ScaleDamageForce( info.m_flDamageForceScale );
		triggerInfo.SetAmmoType( info.m_iAmmoType );
#ifdef GAME_DLL
		TraceAttackToTriggers( triggerInfo, tr.startpos, tr.endpos, vecDir );
#endif

		// Make sure given a valid bullet type
		if (info.m_iAmmoType == -1)
		{
			DevMsg("ERROR: Undefined ammo type!\n");
			return;
		}

		Vector vecTracerDest = tr.endpos;

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
#ifdef GAME_DLL
			UpdateShotStatistics( tr );

			// For shots that don't need persistance
			int soundEntChannel = ( info.m_nFlags&FIRE_BULLETS_TEMPORARY_DANGER_SOUND ) ? SOUNDENT_CHANNEL_BULLET_IMPACT : SOUNDENT_CHANNEL_UNSPECIFIED;

			CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, tr.endpos, 200, 0.5, this, soundEntChannel );
#endif

			// See if the bullet ended up underwater + started out of the water
			if ( !bHitWater && ( enginetrace->GetPointContents( tr.endpos, MASK_WATER ) & (CONTENTS_WATER|CONTENTS_SLIME) ) )
			{
				bHitWater = HandleShotImpactingWater( info, vecEnd, &traceFilter, &vecTracerDest );
			}

			float flActualDamage = info.m_flDamage;

			// If we hit a player, and we have player damage specified, use that instead
			// Adrian: Make sure to use the currect value if we hit a vehicle the player is currently driving.
			if ( flPlayerDamage != 0.0f )
			{
				if ( tr.m_pEnt->IsPlayer() )
				{
					flActualDamage = flPlayerDamage;
				}
#ifdef GAME_DLL
				else if ( tr.m_pEnt->GetServerVehicle() )
				{
					if ( tr.m_pEnt->GetServerVehicle()->GetPassenger() && tr.m_pEnt->GetServerVehicle()->GetPassenger()->IsPlayer() )
					{
						flActualDamage = flPlayerDamage;
					}
				}
#endif
			}

			int nActualDamageType = nDamageType;
			if ( flActualDamage == 0.0 )
			{
				flActualDamage = GameRules()->GetAmmoDamage( pAttacker, tr.m_pEnt, info.m_iAmmoType );
			}
			else if ((info.m_nFlags & FIRE_BULLETS_NO_AUTO_GIB_TYPE) == 0)
			{
				nActualDamageType = nDamageType | ((flActualDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB );
			}

			if ( !bHitWater || ((info.m_nFlags & FIRE_BULLETS_DONT_HIT_UNDERWATER) == 0) )
			{
				// Damage specified by function parameter
				CSharedTakeDamageInfo dmgInfo( this, pAttacker, flActualDamage, nActualDamageType );
				ModifyFireBulletsDamage( &dmgInfo );
				CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, vecDir, tr.endpos );
				dmgInfo.ScaleDamageForce( info.m_flDamageForceScale );
				dmgInfo.SetAmmoType( info.m_iAmmoType );
				tr.m_pEnt->DispatchTraceAttack( dmgInfo, vecDir, &tr );
			
				if ( ToBaseCombatCharacter( tr.m_pEnt ) )
				{
					flCumulativeDamage += dmgInfo.GetDamage();
				}

				if ( bStartedInWater || !bHitWater || (info.m_nFlags & FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS) )
				{
					if ( bDoServerEffects == true )
					{
						DoImpactEffect( tr, nDamageType );
					}
					else
					{
						bDoImpacts = true;
					}
				}
				else
				{
					// We may not impact, but we DO need to affect ragdolls on the client
					CEffectData data;
					data.m_vStart = tr.startpos;
					data.m_vOrigin = tr.endpos;
					data.m_nDamageType = nDamageType;
					
					DispatchEffect( "RagdollImpact", data );
				}
	
#ifdef GAME_DLL
				if ( nAmmoFlags & AMMO_FORCE_DROP_IF_CARRIED )
				{
					// Make sure if the player is holding this, he drops it
					Pickup_ForcePlayerToDropThisObject( tr.m_pEnt );		
				}
#endif
			}
		}

		// See if we hit glass
		if ( tr.m_pEnt != NULL )
		{
#ifdef GAME_DLL
			surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if ( ( psurf != NULL ) && ( psurf->game.material == CHAR_TEX_GLASS ) && ( tr.m_pEnt->ClassMatches( "func_breakable" ) ) )
			{
				// Query the func_breakable for whether it wants to allow for bullet penetration
				if ( tr.m_pEnt->HasSpawnFlags( SF_BREAK_NO_BULLET_PENETRATION ) == false )
				{
					bHitGlass = true;
				}
			}
#endif
		}

		if ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 && ( bHitGlass == false ) )
		{
			if ( bDoServerEffects == true )
			{
				Vector vecTracerSrc = vec3_origin;
				ComputeTracerStartPosition( info.m_vecSrc, &vecTracerSrc );

				trace_t Tracer;
				Tracer = tr;
				Tracer.endpos = vecTracerDest;

#ifdef PORTAL
				if ( pShootThroughPortal )
				{
					Tracer.endpos = info.m_vecSrc + ( vecEnd - info.m_vecSrc ) * fPortalFraction;
				}
#endif //#ifdef PORTAL

				MakeTracer( vecTracerSrc, Tracer, pAmmoDef->TracerType(info.m_iAmmoType) );

#ifdef PORTAL
				if ( pShootThroughPortal )
				{
					Vector vTransformedIntersection;
					UTIL_Portal_PointTransform( pShootThroughPortal->MatrixThisToLinked(), Tracer.endpos, vTransformedIntersection );
					ComputeTracerStartPosition( vTransformedIntersection, &vecTracerSrc );

					Tracer.endpos = vecTracerDest;

					MakeTracer( vecTracerSrc, Tracer, pAmmoDef->TracerType(info.m_iAmmoType) );

					// Shooting through a portal, the damage direction is translated through the passed-through portal
					// so the damage indicator hud animation is correct
					Vector vDmgOriginThroughPortal;
					UTIL_Portal_PointTransform( pShootThroughPortal->MatrixThisToLinked(), info.m_vecSrc, vDmgOriginThroughPortal );
					g_MultiDamage.SetDamagePosition ( vDmgOriginThroughPortal );
				}
				else
				{
					g_MultiDamage.SetDamagePosition ( info.m_vecSrc );
				}
#endif //#ifdef PORTAL
			}
			else
			{
				bDoTracers = true;
			}
		}

		//NOTENOTE: We could expand this to a more general solution for various material penetration types (wood, thin metal, etc)

		// See if we should pass through glass
#ifdef GAME_DLL
		if ( bHitGlass )
		{
			HandleShotImpactingGlass( info, tr, vecDir, &traceFilter );
		}
#endif

		iSeed++;
	}

#if defined( HL2MP ) && defined( GAME_DLL )
	if ( bDoServerEffects == false )
	{
		TE_HL2MPFireBullets( entindex(), tr.startpos, info.m_vecDirShooting, info.m_iAmmoType, iEffectSeed, info.m_iShots, info.m_vecSpread.x, bDoTracers, bDoImpacts );
	}
#endif

#ifdef GAME_DLL
	ApplyMultiDamage();

	if ( IsPlayer() && flCumulativeDamage > 0.0f )
	{
		CBasePlayer *pPlayer = static_cast< CBasePlayer * >( this );
		CTakeDamageInfo dmgInfo( this, pAttacker, flCumulativeDamage, nDamageType );
		gamestats->Event_WeaponHit( pPlayer, info.m_bPrimaryAttack, pPlayer->GetActiveWeapon()->GetClassname(), dmgInfo );
	}

	if ( ai_shot_notify_targets.GetBool() )
	{
		if ( IsPlayer() )
		{
			// Look for probable target to notify of attack
			CBaseEntity *pAimTarget = static_cast<CBasePlayer*>(this)->GetProbableAimTarget( info.m_vecSrc, info.m_vecDirShooting );
			if ( pAimTarget && pAimTarget->IsCombatCharacter() )
			{
				pAimTarget->MyCombatCharacterPointer()->OnEnemyRangeAttackedMe( this, vecDir, vecEnd );
			}
		}
		else if ( GetEnemy() && GetEnemy()->IsCombatCharacter() )
		{
			GetEnemy()->MyCombatCharacterPointer()->OnEnemyRangeAttackedMe( this, vecDir, vecEnd );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Should we draw bubbles underwater?
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::ShouldDrawUnderwaterBulletBubbles()
{
#if defined( HL2_DLL ) && defined( GAME_DLL )
	CBaseEntity *pPlayer = UTIL_GetNearestVisiblePlayer(this); 
	return pPlayer && (pPlayer->GetWaterLevel() == 3);
#else
	return false;
#endif
}


//-----------------------------------------------------------------------------
// Handle shot entering water
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::HandleShotImpactingWater( const FireBulletsInfo_t &info, 
	const Vector &vecEnd, ITraceFilter *pTraceFilter, Vector *pVecTracerDest )
{
	trace_t	waterTrace;

	// Trace again with water enabled
	AI_TraceLine( info.m_vecSrc, vecEnd, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), pTraceFilter, &waterTrace );
	
	// See if this is the point we entered
	if ( ( enginetrace->GetPointContents( waterTrace.endpos - Vector(0,0,0.1f), MASK_WATER ) & (CONTENTS_WATER|CONTENTS_SLIME) ) == 0 )
		return false;

	if ( ShouldDrawWaterImpacts() )
	{
		int	nMinSplashSize = GetAmmoDef()->MinSplashSize(info.m_iAmmoType);
		int	nMaxSplashSize = GetAmmoDef()->MaxSplashSize(info.m_iAmmoType);

		CEffectData	data;
 		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = random_valve->RandomFloat( nMinSplashSize, nMaxSplashSize );
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}
		DispatchEffect( "gunshotsplash", data );
	}

#ifdef GAME_DLL
	if ( ShouldDrawUnderwaterBulletBubbles() )
	{
		CWaterBullet *pWaterBullet = ( CWaterBullet * )CreateEntityByName( "waterbullet" );
		if ( pWaterBullet )
		{
			pWaterBullet->Spawn( waterTrace.endpos, info.m_vecDirShooting );
					 
			CEffectData tracerData;
			tracerData.m_vStart = waterTrace.endpos;
			tracerData.m_vOrigin = waterTrace.endpos + info.m_vecDirShooting * 400.0f;
			tracerData.m_fFlags = TRACER_TYPE_WATERBULLET;
			DispatchEffect( "TracerSound", tracerData );
		}
	}
#endif

	*pVecTracerDest = waterTrace.endpos;
	return true;
}


ITraceFilter* CSharedBaseEntity::GetBeamTraceFilter( void )
{
	return NULL;
}

void CSharedBaseEntity::DispatchTraceAttack( const CSharedTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
#ifdef GAME_DLL
	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( info ))
	{
		return;
	}
#endif

	TraceAttack( info, vecDir, ptr, pAccumulator );
}

void CSharedBaseEntity::TraceAttack( const CSharedTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;

	if ( m_takedamage )
	{
#ifdef GAME_DLL
		if ( pAccumulator )
		{
			pAccumulator->AccumulateMultiDamage( info, this );
		}
		else
#endif // GAME_DLL
		{
			AddMultiDamage( info, this );
		}

		CSharedBaseEntity *pAttacker = info.GetAttacker();

		if(pAttacker) {
			if(GameRules()->IsTeamplay() && pAttacker->InSameTeam(this) == true) {
				return;
			}
		}

		int blood = BloodColor();
		
#if defined(GAME_DLL)
		if ( blood != DONT_BLEED && DamageFilterAllowsBlood( info ) )
#else
		if ( blood != DONT_BLEED )
#endif
		{
			SpawnBlood( vecOrigin, vecDir, blood, info.GetDamage() );// a little surface blood.
			TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		}
	}
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CSharedBaseEntity::DoImpactEffect( trace_t &tr, int nDamageType )
{
	// give shooter a chance to do a custom impact.
	UTIL_ImpactTrace( &tr, nDamageType );
} 


//-----------------------------------------------------------------------------
// Computes the tracer start position
//-----------------------------------------------------------------------------
void CSharedBaseEntity::ComputeTracerStartPosition( const Vector &vecShotSrc, Vector *pVecTracerStart )
{
	if ( IsPlayer() )
	{
		// adjust tracer position for player
		Vector forward, right;
		CSharedBasePlayer *pPlayer = ToBasePlayer( this );
		pPlayer->EyeVectors( &forward, &right, NULL );
		*pVecTracerStart = vecShotSrc + Vector ( 0 , 0 , -4 ) + right * 2 + forward * 16;
	}
	else
	{
		*pVecTracerStart = vecShotSrc;

		CSharedBaseCombatCharacter *pBCC = MyCombatCharacterPointer();
		if ( pBCC != NULL )
		{
			CSharedBaseCombatWeapon *pWeapon = pBCC->GetActiveWeapon();

			if ( pWeapon != NULL )
			{
				Vector vecMuzzle;
				QAngle vecMuzzleAngles;

				if ( pWeapon->GetAttachment( 1, vecMuzzle, vecMuzzleAngles ) )
				{
					*pVecTracerStart = vecMuzzle;
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Virtual function allows entities to handle tracer presentation
//			as they see fit.
//
// Input  : vecTracerSrc - the point at which to start the tracer (not always the
//			same spot as the traceline!
//
//			tr - the entire trace result for the shot.
//
// Output :
//-----------------------------------------------------------------------------
void CSharedBaseEntity::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	const char *pszTracerName = GetTracerType();

	Vector vNewSrc = vecTracerSrc;

	int iAttachment = GetTracerAttachment();

	switch ( iTracerType )
	{
	case TRACER_LINE:
		UTIL_Tracer( vNewSrc, tr.endpos, entindex(), iAttachment, 0.0f, false, pszTracerName );
		break;

	case TRACER_LINE_AND_WHIZ:
		UTIL_Tracer( vNewSrc, tr.endpos, entindex(), iAttachment, 0.0f, true, pszTracerName );
		break;
	}
}

//-----------------------------------------------------------------------------
// Default tracer attachment
//-----------------------------------------------------------------------------
int CSharedBaseEntity::GetTracerAttachment( void )
{
	int iAttachment = TRACER_DONT_USE_ATTACHMENT;

	return iAttachment;
}

float CSharedBaseEntity::HealthFraction() const
{
	if ( GetMaxHealth() == 0 )
		return 1.0f;

	float flFraction = ( float )GetHealth() / ( float )GetMaxHealth();
	flFraction = clamp( flFraction, 0.0f, 1.0f );
	return flFraction;
}

int CSharedBaseEntity::BloodColor()
{
	return DONT_BLEED; 
}


void CSharedBaseEntity::TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType )
{
	if ((BloodColor() == DONT_BLEED) || (BloodColor() == BLOOD_COLOR_MECH))
	{
		return;
	}

	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_AIRBOAT)))
		return;

	// make blood decal on the wall!
	trace_t Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

#ifdef GAME_DLL
	if ( !IsAlive() )
	{
		// dealing with a dead npc.
		if ( GetMaxHealth() <= 0 )
		{
			// no blood decal for a npc that has already decalled its limit.
			return;
		}
		else
		{
			m_iMaxHealth -= 1;
		}
	}
#endif

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	float flTraceDist = (bitsDamageType & DMG_AIRBOAT) ? 384 : 172;
	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += random_valve->RandomFloat( -flNoise, flNoise );
		vecTraceDir.y += random_valve->RandomFloat( -flNoise, flNoise );
		vecTraceDir.z += random_valve->RandomFloat( -flNoise, flNoise );

		// Don't bleed on grates.
		AI_TraceLine( ptr->endpos, ptr->endpos + vecTraceDir * -flTraceDist, MASK_SOLID_BRUSHONLY & ~CONTENTS_GRATE, this, COLLISION_GROUP_NONE, &Bloodtr);

		if ( Bloodtr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}


const char* CSharedBaseEntity::GetTracerType()
{
	return NULL;
}

void CSharedBaseEntity::ModifyEmitSoundParams( EmitSound_t &params )
{
#ifdef CLIENT_DLL
	if ( GameRules() )
	{
		params.m_pSoundName = GameRules()->TranslateEffectForVisionFilter( "sounds", params.m_pSoundName );
	}
#endif
}

#if defined(GAME_DLL)
void CSharedBaseEntity::ModifySentenceParams( int &iSentenceIndex, int &iChannel, float &flVolume, soundlevel_t &iSoundlevel, int &iFlags, int &iPitch,
	const Vector **pOrigin, const Vector **pDirection, bool &bUpdatePositions, float &soundtime, int &iSpecialDSP, int &iSpeakerIndex )
{

}
#endif

//-----------------------------------------------------------------------------
// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
//-----------------------------------------------------------------------------
void CSharedBaseEntity::FollowEntity( CSharedBaseEntity *pBaseEntity, bool bBoneMerge )
{
	if (pBaseEntity)
	{
		SetParent( pBaseEntity );
		SetMoveType( MOVETYPE_NONE );
		
		if ( bBoneMerge )
			AddEffects( EF_BONEMERGE );

		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}
	else
	{
		StopFollowingEntity();
	}
}

void CSharedBaseEntity::SetEffectEntity( CSharedBaseEntity *pEffectEnt )
{
	if ( m_hEffectEntity.Get() != pEffectEnt )
	{
		m_hEffectEntity = pEffectEnt;
	}
}

void CSharedBaseEntity::ApplyLocalVelocityImpulse( const Vector &inVecImpulse )
{
	// NOTE: Don't have to use GetVelocity here because local values
	// are always guaranteed to be correct, unlike abs values which may 
	// require recomputation
	if ( inVecImpulse != vec3_origin )
	{
		Vector vecImpulse = inVecImpulse;

		// Safety check against receive a huge impulse, which can explode physics
		switch ( CheckEntityVelocity( vecImpulse ) )
		{
			case -1:
				Warning( "Discarding ApplyLocalVelocityImpulse(%f,%f,%f) on %s\n", vecImpulse.x, vecImpulse.y, vecImpulse.z, GetDebugName() );
				Assert( false );
				return;
			case 0:
				if ( CheckEmitReasonablePhysicsSpew() )
				{
					Warning( "Clamping ApplyLocalVelocityImpulse(%f,%f,%f) on %s\n", inVecImpulse.x, inVecImpulse.y, inVecImpulse.z, GetDebugName() );
				}
				break;
		}

		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			IPhysicsObject *ppPhysObjs[ VPHYSICS_MAX_OBJECT_LIST_COUNT ];
			int nNumPhysObjs = VPhysicsGetObjectList( ppPhysObjs, VPHYSICS_MAX_OBJECT_LIST_COUNT );
			for ( int i = 0; i < nNumPhysObjs; i++ )
			{
				Vector worldVel;
				ppPhysObjs[ i ]->LocalToWorld( &worldVel, vecImpulse );
				ppPhysObjs[ i ]->AddVelocity(  &worldVel, NULL );
			}
		}
		else
		{
			InvalidatePhysicsRecursive( VELOCITY_CHANGED );
			m_vecVelocity += vecImpulse;
		}
	}
}

void CSharedBaseEntity::ApplyAbsVelocityImpulse( const Vector &inVecImpulse )
{
	if ( inVecImpulse != vec3_origin )
	{
		Vector vecImpulse = inVecImpulse;

		// Safety check against receive a huge impulse, which can explode physics
		switch ( CheckEntityVelocity( vecImpulse ) )
		{
			case -1:
				Warning( "Discarding ApplyAbsVelocityImpulse(%f,%f,%f) on %s\n", vecImpulse.x, vecImpulse.y, vecImpulse.z, GetDebugName() );
				Assert( false );
				return;
			case 0:
				if ( CheckEmitReasonablePhysicsSpew() )
				{
					Warning( "Clamping ApplyAbsVelocityImpulse(%f,%f,%f) on %s\n", inVecImpulse.x, inVecImpulse.y, inVecImpulse.z, GetDebugName() );
				}
				break;
		}

		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			IPhysicsObject *ppPhysObjs[ VPHYSICS_MAX_OBJECT_LIST_COUNT ];
			int nNumPhysObjs = VPhysicsGetObjectList( ppPhysObjs, VPHYSICS_MAX_OBJECT_LIST_COUNT );
			for ( int i = 0; i < nNumPhysObjs; i++ )
			{
				ppPhysObjs[ i ]->AddVelocity( &vecImpulse, NULL );
			}
		}
		else
		{
			// NOTE: Have to use GetAbsVelocity here to ensure it's the correct value
			Vector vecResult;
			VectorAdd( GetAbsVelocity(), vecImpulse, vecResult );
			SetAbsVelocity( vecResult );
		}
	}
}

void CSharedBaseEntity::ApplyLocalAngularVelocityImpulse( const AngularImpulse &angImpulse )
{
	if (angImpulse != vec3_origin )
	{
		// Safety check against receive a huge impulse, which can explode physics
		if ( !IsEntityAngularVelocityReasonable( angImpulse ) )
		{
			Warning( "Bad ApplyLocalAngularVelocityImpulse(%f,%f,%f) on %s\n", angImpulse.x, angImpulse.y, angImpulse.z, GetDebugName() );
			Assert( false );
			return;
		}

		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			IPhysicsObject *ppPhysObjs[ VPHYSICS_MAX_OBJECT_LIST_COUNT ];
			int nNumPhysObjs = VPhysicsGetObjectList( ppPhysObjs, VPHYSICS_MAX_OBJECT_LIST_COUNT );
			for ( int i = 0; i < nNumPhysObjs; i++ )
			{
				ppPhysObjs[ i ]->AddVelocity( NULL, &angImpulse );
			}
		}
		else
		{
			QAngle vecResult;
			AngularImpulseToQAngle( angImpulse, vecResult );
			VectorAdd( GetLocalAngularVelocity(), vecResult, vecResult );
			SetLocalAngularVelocity( vecResult );
		}
	}
}

void CSharedBaseEntity::SetCollisionGroup( int collisionGroup )
{
	if ( (int)m_CollisionGroup != collisionGroup )
	{
		m_CollisionGroup = collisionGroup;
		CollisionRulesChanged();
	}
}


void CSharedBaseEntity::CollisionRulesChanged()
{
	// ivp maintains state based on recent return values from the collision filter, so anything
	// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
	if ( VPhysicsGetObject() )
	{
		extern bool PhysIsInCallback();
		if ( PhysIsInCallback() )
		{
			Warning("Changing collision rules within a callback is likely to cause crashes!\n");
			Assert(0);
		}
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i] != NULL ) //this really shouldn't happen, but it does >_<
				pList[i]->RecheckCollisionFilter();
		}
	}
}

int CSharedBaseEntity::GetWaterType() const
{
	int out = 0;
	if ( m_nWaterType & 1 )
		out |= CONTENTS_WATER;
	if ( m_nWaterType & 2 )
		out |= CONTENTS_SLIME;
	return out;
}

void CSharedBaseEntity::SetWaterType( int nType )
{
	m_nWaterType = 0;
	if ( nType & CONTENTS_WATER )
		m_nWaterType |= 1;
	if ( nType & CONTENTS_SLIME )
		m_nWaterType |= 2;
}

extern ConVar	*sv_alternateticks;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::IsSimulatingOnAlternateTicks()
{
	return sv_alternateticks->GetBool();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::IsToolRecording() const
{
	return m_bToolRecording;
}
#endif

#if defined( CLIENT_DLL ) 
extern void TouchTriggerPlayerMovement( C_BaseEntity *pEntity );
#endif

void CSharedBaseEntity::PhysicsTouchTriggers( const Vector *pPrevAbsOrigin )
{
#if defined( CLIENT_DLL )
	Assert( !pPrevAbsOrigin );
	TouchTriggerPlayerMovement( this );
#else
	edict_t *pEntity = edict();

	if ( pEntity && !IsWorld() )
	{
		Assert(CollisionProp());
		bool isTriggerCheckSolids = IsSolidFlagSet( FSOLID_TRIGGER );
		bool isSolidCheckTriggers = IsSolid() && !isTriggerCheckSolids;		// NOTE: Moving triggers (items, ammo etc) are not 
		// checked against other triggers ot reduce the number of touchlinks created
		if ( !(isSolidCheckTriggers || isTriggerCheckSolids) )
			return;

		if ( GetSolid() == SOLID_BSP ) 
		{
			if ( !GetModel() && Q_strlen( STRING( GetModelName() ) ) == 0 ) 
			{
				Warning( "Inserted %s with no model\n", GetClassname() );
				return;
			}
		}

		SetCheckUntouch( true );
		if ( isSolidCheckTriggers )
		{
			engine->SolidMoved( pEntity, CollisionProp(), pPrevAbsOrigin, sm_bAccurateTriggerBboxChecks );
		}
		if ( isTriggerCheckSolids )
		{
			engine->TriggerMoved( pEntity, sm_bAccurateTriggerBboxChecks );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CSharedBaseEntity::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	// TODO
	// Append chapter/day?

	set.AppendCriteria( "randomnum", UTIL_VarArgs("%d", RandomInt(0,100)) );
	
	// Append our classname and game name
	set.AppendCriteria( "classname", GetClassname() );

	const char *pEntityName = "";

#ifdef CLIENT_DLL
	pEntityName = GetEntityName();
#else
	pEntityName = GetEntityNameAsCStr();
#endif

	set.AppendCriteria( "name", pEntityName );

	// Append our health
	set.AppendCriteria( "health", UTIL_VarArgs( "%i", GetHealth() ) );

	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "healthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	// Go through all the global states and append them

#ifdef GAME_DLL
	for ( int i = 0; i < GlobalEntity_GetNumGlobals(); i++ ) 
	{
		const char *szGlobalName = GlobalEntity_GetName(i);
		int iGlobalState = (int)GlobalEntity_GetStateByIndex(i);
		set.AppendCriteria( szGlobalName, UTIL_VarArgs( "%i", iGlobalState ) );
	}

	// Append map name
	set.AppendCriteria( "map", gpGlobals->mapname.ToCStr() );

	// Append anything from world I/O/keyvalues with "world" as prefix
	CWorld *world = GetWorldEntity();
	if ( world )
	{
		world->AppendContextToCriteria( set, "world" );
	}

	// Append base stuff
	set.AppendCriteria("spawnflags", UTIL_VarArgs("%i", GetSpawnFlags()));
	set.AppendCriteria("flags", UTIL_VarArgs("%i", GetFlags()));
#endif
}

#ifdef GAME_DLL
ConVar ent_messages_draw( "ent_messages_draw", "0", FCVAR_CHEAT, "Visualizes all entity input/output activity." );
#endif

//-----------------------------------------------------------------------------
// Purpose: calls the appropriate message mapped function in the entity according
//			to the fired action.
// Input  : char *szInputName - input destination
//			*pActivator - entity which initiated this sequence of actions
//			*pCaller - entity from which this event is sent
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBaseEntity::AcceptInput( const char *szInputName, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller, variant_t Value, int outputID )
{
#ifdef GAME_DLL
	if ( ent_messages_draw.GetBool() )
	{
		if ( pCaller != NULL )
		{
			NDebugOverlay::Line( pCaller->GetAbsOrigin(), GetAbsOrigin(), 255, 255, 255, false, 3 );
			NDebugOverlay::Box( pCaller->GetAbsOrigin(), Vector(-4, -4, -4), Vector(4, 4, 4), 255, 0, 0, 0, 3 );
		}

		NDebugOverlay::Text( GetAbsOrigin(), szInputName, false, 3 );	
		NDebugOverlay::Box( GetAbsOrigin(), Vector(-4, -4, -4), Vector(4, 4, 4), 0, 255, 0, 0, 3 );
	}
#endif

	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, szInputName) )
				{
					// found a match

					if(developer->GetInt() >= 2) {
						char szBuffer[256];
						// mapper debug message
						if (pCaller != NULL)
						{
							Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input %s: %s.%s(%s)\n", gpGlobals->curtime, pCaller->GetEntityNameAsCStr(), GetDebugName(), szInputName, Value.String() );
						}
						else
						{
							Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input <NULL>: %s.%s(%s)\n", gpGlobals->curtime, GetDebugName(), szInputName, Value.String() );
						}
						Log_Msg( LOG_ENTITYIO, "%s", szBuffer );
					}
				#ifdef GANE_DLL
					ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );
				#endif

				#ifdef GANE_DLL
					if (m_debugOverlays & OVERLAY_MESSAGE_BIT)
					{
						DrawInputOverlay(szInputName,pCaller,Value);
					}
				#endif

					// convert the value if necessary
					if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
					{
						if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
						{
							// Activator, etc. support for EHANDLE convert
							if ( !Value.Convert( (fieldtype_t)dmap->dataDesc[i].fieldType, this, pActivator, pCaller ) )
							{
								bool bBadConversion = true;

								// Attempt to convert to string and back.
								// Almost all field types support being converted to a string, and many support being parsed from a string too.
								fieldtype_t originalfield = Value.FieldType();
								if (Value.Convert(FIELD_STRING))
								{
									bBadConversion = !(Value.Convert((fieldtype_t)dmap->dataDesc[i].fieldType, this, pActivator, pCaller));
									if (!bBadConversion)
									{
										// Actual support should be added for each field, but if it works, it works.
										// Warning against it only matters if you're a programmer and want to add support for each field.
										// Only send a warning in dev mode.
										Log_Warning(LOG_ENTITYIO,"!! Had to convert to string and back\n"
													"!! Source Field Type: %i, Target Field Type: %i\n",
												originalfield, dmap->dataDesc[i].fieldType);
									}
								}

								if (bBadConversion)
								{
									Log_Warning(LOG_ENTITYIO, "!! ERROR: bad input/output link:\n!! Unable to convert value \"%s\" from %s (%s) to field type %i\n!! Target Entity: %s (%s), Input: %s\n", 
										Value.GetDebug(),
										( pCaller != NULL ) ? pCaller->GetClassname() : "<null>",
										( pCaller != NULL ) ? pCaller->GetEntityNameAsCStr() : "<null>",
										dmap->dataDesc[i].fieldType,
										STRING(m_iClassname), GetDebugName(), szInputName );
									return false;
								}
							}
						}
					}

					// call the input handler, or if there is none just set the value
					inputfunc_t pfnInput = dmap->dataDesc[i].inputFunc;

					if ( pfnInput )
					{ 
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator;
						data.pCaller = pCaller;
						data.value = Value;
						data.nOutputID = outputID;

						(this->*pfnInput)( data );
					}
					else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
					{
						// set the value directly
						Value.SetOther( ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
					
						// TODO: if this becomes evil and causes too many full entity updates, then we should make
						// a macro like this:
						//
						// define MAKE_INPUTVAR(x) void Note##x##Modified() { x.GetForModify(); }
						//
						// Then the datadesc points at that function and we call it here. The only pain is to add
						// that function for all the DEFINE_INPUT calls.
						NetworkStateChanged();
					}

					return true;
				}
			}
		}
	}

	Log_Warning( LOG_ENTITYIO, "unhandled input: (%s) -> (%s,%s)\n", szInputName, GetClassname(), GetDebugName()/*,", from (%s,%s)" STRING(pCaller->m_iClassname), STRING(pCaller->m_iName)*/ );
	return false;
}
