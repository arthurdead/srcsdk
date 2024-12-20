//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Precaches and defs for entities and other data that must always be available.
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "EnvMessage.h"
#include "player.h"
#include "gamerules.h"
#include "physics.h"
#include "activitylist.h"
#include "eventlist.h"
#include "eventqueue.h"
#include "ai_schedule.h"
#include "ai_utils.h"
#include "basetempentity.h"
#include "world.h"
#include "mempool.h"
#include "igamesystem.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "engine/IStaticPropMgr.h"
#include "particle_parse.h"
#include "globalstate.h"
#include "model_types.h"
#include "cvisibilitymonitor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CBaseEntity				*g_pLastSpawn;
void InitBodyQue(void);
extern void W_Precache(void);
extern void ActivityList_Free( void );
extern CUtlMemoryPool g_EntityListPool;

#if !defined( CLIENT_DLL )
#define SF_GAME_EVENT_PROXY_AUTO_VISIBILITY		1

//=========================================================
// Allows level designers to generate certain game events 
// from entity i/o.
//=========================================================
class CInfoGameEventProxy : public CPointEntity
{
private:
	string_t	m_iszEventName;
	float		m_flRange;

public:
	DECLARE_CLASS( CInfoGameEventProxy, CPointEntity );

	void Spawn();
	int UpdateTransmitState();
	void InputGenerateGameEvent( inputdata_t &inputdata );

	static bool GameEventProxyCallback( CBaseEntity *pProxy, CBasePlayer *pViewingPlayer );

	DECLARE_MAPENTITY();
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CInfoGameEventProxy::Spawn()
{
	BaseClass::Spawn();

	m_flRange *= 12.0f; // Convert feet to inches

	if( GetSpawnFlags() & SF_GAME_EVENT_PROXY_AUTO_VISIBILITY )
	{
		VisibilityMonitor_AddEntity( this, m_flRange, &CInfoGameEventProxy::GameEventProxyCallback, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CInfoGameEventProxy::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CInfoGameEventProxy::InputGenerateGameEvent( inputdata_t &inputdata )
{
	CBasePlayer *pActivator = ToBasePlayer( inputdata.pActivator );

	IGameEvent *event = gameeventmanager->CreateEvent( m_iszEventName.ToCStr() );
	if ( event )
	{
		if ( pActivator )
		{
			event->SetInt( "userid", pActivator->GetUserID() );
		}
		event->SetInt( "subject", entindex() );
		gameeventmanager->FireEvent( event );
	}
}

//---------------------------------------------------------
// Callback for the visibility monitor.
//---------------------------------------------------------
bool CInfoGameEventProxy::GameEventProxyCallback( CBaseEntity *pProxy, CBasePlayer *pViewingPlayer )
{
	CInfoGameEventProxy *pProxyPtr = dynamic_cast <CInfoGameEventProxy *>(pProxy);

	if( !pProxyPtr )
		return true;

	IGameEvent * event = gameeventmanager->CreateEvent( pProxyPtr->m_iszEventName.ToCStr() );
	if ( event )
	{
		event->SetInt( "userid", pViewingPlayer->GetUserID() );
		event->SetInt( "subject", pProxyPtr->entindex() );
		gameeventmanager->FireEvent( event );
	}

	return false;
}


LINK_ENTITY_TO_CLASS( info_game_event_proxy, CInfoGameEventProxy );

BEGIN_MAPENTITY( CInfoGameEventProxy )
	DEFINE_KEYFIELD( m_iszEventName, FIELD_STRING, "event_name" ),
	DEFINE_KEYFIELD( m_flRange, FIELD_FLOAT, "range" ),
	DEFINE_INPUTFUNC( FIELD_STRING, "GenerateGameEvent", InputGenerateGameEvent ),
END_MAPENTITY()
#endif

#define SF_DECAL_NOTINDEATHMATCH		2048

class CDecal : public CPointEntity
{
public:
	DECLARE_CLASS( CDecal, CPointEntity );

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	// Need to apply static decals here to get them into the signon buffer for the server appropriately
	virtual void Activate();

	void	TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );

	CBaseEntity *GetDecalEntityAndPosition( Vector *pPosition, bool bStatic );


	DECLARE_MAPENTITY();

public:
	int		m_nTexture;
	bool	m_bLowPriority;
	string_t m_entityName;

private:

	void	StaticDecal( void );
};

BEGIN_MAPENTITY( CDecal )

	DEFINE_KEYFIELD( m_bLowPriority, FIELD_BOOLEAN, "LowPriority" ), // Don't mark as FDECAL_PERMANENT so not save/restored and will be reused on the client preferentially
	DEFINE_KEYFIELD( m_entityName, FIELD_STRING, "ApplyEntity" ), // Force apply to this entity instead of tracing

	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

END_MAPENTITY()

LINK_ENTITY_TO_CLASS( infodecal, CDecal );

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal::Spawn( void )
{
	if ( m_nTexture < 0 || 
		(GameRules()->IsDeathmatch() && HasSpawnFlags( SF_DECAL_NOTINDEATHMATCH )) )
	{
		UTIL_Remove( this );
		return;
	} 
}

void CDecal::Activate()
{
	BaseClass::Activate();

	if ( !GetEntityName() )
	{
		StaticDecal();
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink ( &CDecal::SUB_DoNothing );
		SetUse(&CDecal::TriggerDecal);
	}
}

class CTraceFilterValidForDecal : public CTraceFilterSimple
{
public:
	CTraceFilterValidForDecal(const IHandleEntity *passentity, int collisionGroup )
	 :	CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		static const char *ppszIgnoredClasses[] = 
		{
			"weapon_*",
			"item_*",
			"prop_ragdoll",
			"prop_dynamic",
			"prop_static",
			"prop_physics",
			"npc_bullseye",  // Tracker 15335
		};

		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		// Tracker 15335:  Never impact decals against entities which are not rendering, either.
		if ( pEntity->IsEffectActive( EF_NODRAW ) )
			return false;

		for ( int i = 0; i < ARRAYSIZE(ppszIgnoredClasses); i++ )
		{
			if ( pEntity->ClassMatches( ppszIgnoredClasses[i] ) )
				return false;
		}

		if ( modelinfo->GetModelType( pEntity->GetModel() ) != mod_brush )
			return false;

		return CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

void CDecal::TriggerDecal ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// this is set up as a USE function for info_decals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	trace_t		trace;
	int			entityIndex;

	Vector position;
	CBaseEntity *pEntity = GetDecalEntityAndPosition(&position, false);
	entityIndex = pEntity ? pEntity->entindex() : 0;

	CBroadcastRecipientFilter filter;

	te->BSPDecal( filter, 0.0, &position, entityIndex, m_nTexture );

	SetThink( &CDecal::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CDecal::InputActivate( inputdata_t &inputdata )
{
	TriggerDecal( inputdata.pActivator, inputdata.pCaller, USE_ON, 0 );
}


void CDecal::StaticDecal( void )
{
	Vector position;
	CBaseEntity *pEntity = GetDecalEntityAndPosition(&position, true);
	int entityIndex = 0;
	modelindex_t modelIndex = INVALID_MODEL_INDEX;

	if ( pEntity )
	{
		entityIndex = pEntity->entindex();
		modelIndex = pEntity->GetModelIndex();
		Vector worldspace = position;
		VectorITransform( worldspace, pEntity->EntityToWorldTransform(), position );
	}
	else
	{
		position = GetAbsOrigin();
	}

	engine->StaticDecal( position, m_nTexture, entityIndex, modelIndex, m_bLowPriority );

	SUB_Remove();
}

CBaseEntity *CDecal::GetDecalEntityAndPosition( Vector *pPosition, bool bStatic )
{
	CBaseEntity *pEntity = NULL;
	if ( !m_entityName )
	{
		trace_t trace;
		Vector start = GetAbsOrigin();
		Vector direction(1,1,1);
		if ( GetAbsAngles() == vec3_angle )
		{
			start -= direction * 5;
		}
		else
		{
			GetVectors( &direction, NULL, NULL );
		}
		Vector end = start + direction * 10;
		if ( bStatic )
		{
			CTraceFilterValidForDecal traceFilter( this, COLLISION_GROUP_NONE );
			UTIL_TraceLine( start, end, MASK_SOLID, &traceFilter, &trace );
		}
		else
		{
			UTIL_TraceLine( start, end, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
		}
		if ( trace.DidHitNonWorldEntity() )
		{
			*pPosition = trace.endpos;
			return trace.m_pEnt;
		}
	}
	else
	{
		pEntity = gEntList.FindEntityByName( NULL, m_entityName );
	}

	*pPosition = GetAbsOrigin();
	return pEntity;
}


bool CDecal::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "texture"))
	{
		// FIXME:  should decals all be preloaded?
		m_nTexture = UTIL_PrecacheDecal( szValue, true );
		
		// Found
		if (m_nTexture >= 0 )
			return true;
		Warning( "Can't find decal %s\n", szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Projects a decal against a prop
//-----------------------------------------------------------------------------
class CProjectedDecal : public CPointEntity
{
public:
	DECLARE_CLASS( CProjectedDecal, CPointEntity );

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	// Need to apply static decals here to get them into the signon buffer for the server appropriately
	virtual void Activate();

	void	TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );

	DECLARE_MAPENTITY();

public:
	int		m_nTexture;
	float	m_flDistance;

private:
	void	ProjectDecal( CRecipientFilter& filter );

	void	StaticDecal( void );
};

BEGIN_MAPENTITY( CProjectedDecal )

	DEFINE_KEYFIELD( m_flDistance, FIELD_FLOAT, "Distance" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

END_MAPENTITY()

LINK_ENTITY_TO_CLASS( info_projecteddecal, CProjectedDecal );

// UNDONE:  These won't get sent to joining players in multi-player
void CProjectedDecal::Spawn( void )
{
	if ( m_nTexture < 0 || 
		(GameRules()->IsDeathmatch() && HasSpawnFlags( SF_DECAL_NOTINDEATHMATCH )) )
	{
		UTIL_Remove( this );
		return;
	} 
}

void CProjectedDecal::Activate()
{
	BaseClass::Activate();

	if ( !GetEntityName() )
	{
		StaticDecal();
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink ( &CProjectedDecal::SUB_DoNothing );
		SetUse(&CProjectedDecal::TriggerDecal);
	}
}

void CProjectedDecal::InputActivate( inputdata_t &inputdata )
{
	TriggerDecal( inputdata.pActivator, inputdata.pCaller, USE_ON, 0 );
}

void CProjectedDecal::ProjectDecal( CRecipientFilter& filter )
{
	te->ProjectDecal( filter, 0.0, 
		&GetAbsOrigin(), &GetAbsAngles(), m_flDistance, m_nTexture );
}

void CProjectedDecal::TriggerDecal ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBroadcastRecipientFilter filter;

	ProjectDecal( filter );

	SetThink( &CProjectedDecal::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CProjectedDecal::StaticDecal( void )
{
	CBroadcastRecipientFilter initFilter;
	initFilter.MakeInitMessage();

	ProjectDecal( initFilter );

	SUB_Remove();
}


bool CProjectedDecal::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "texture"))
	{
		// FIXME:  should decals all be preloaded?
		m_nTexture = UTIL_PrecacheDecal( szValue, true );
		
		// Found
		if (m_nTexture >= 0 )
			return true;
		Warning( "Can't find decal %s\n", szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================
LINK_ENTITY_TO_CLASS( worldspawn, CWorld );

modelindex_t g_sModelIndexWorld = INVALID_MODEL_INDEX;

BEGIN_MAPENTITY( CWorld, MAPENT_POINTCLASS )

	// keyvalues are parsed from map, but not saved/loaded
	DEFINE_KEYFIELD( m_iszChapterTitle, FIELD_STRING, "chaptertitle" ),
	DEFINE_KEYFIELD( m_bStartDark,		FIELD_BOOLEAN, "startdark" ),
	DEFINE_KEYFIELD( m_bDisplayTitle,	FIELD_BOOLEAN, "gametitle" ),

	DEFINE_KEYFIELD( m_flMaxOccludeeArea, FIELD_FLOAT, "maxoccludeearea" ),
	DEFINE_KEYFIELD( m_flMinOccluderArea, FIELD_FLOAT, "minoccluderarea" ),

	DEFINE_KEYFIELD( m_flMaxPropScreenSpaceWidth, FIELD_FLOAT, "maxpropscreenwidth" ),
	DEFINE_KEYFIELD( m_flMinPropScreenSpaceWidth, FIELD_FLOAT, "minpropscreenwidth" ),
	DEFINE_KEYFIELD( m_iszDetailSpriteMaterial, FIELD_STRING, "detailmaterial" ),
	DEFINE_KEYFIELD( m_bColdWorld,		FIELD_BOOLEAN, "coldworld" ),

	DEFINE_KEYFIELD( m_bChapterTitleNoMessage, FIELD_BOOLEAN, "chaptertitlenomessage" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetChapterTitle", InputSetChapterTitle ),

END_MAPENTITY()


// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CWorld, DT_WORLD)
	SendPropFloat	(SENDINFO(m_flWaveHeight), 8, SPROP_ROUNDUP,	0.0f,	8.0f),
	SendPropVector	(SENDINFO(m_WorldMins),	-1,	SPROP_COORD),
	SendPropVector	(SENDINFO(m_WorldMaxs),	-1,	SPROP_COORD),
	SendPropInt		(SENDINFO(m_bStartDark), 1, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flMaxOccludeeArea), 0, SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_flMinOccluderArea), 0, SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_flMaxPropScreenSpaceWidth), 0, SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_flMinPropScreenSpaceWidth), 0, SPROP_NOSCALE ),
	SendPropStringT (SENDINFO(m_iszDetailSpriteMaterial) ),
	SendPropInt		(SENDINFO(m_bColdWorld), 1, SPROP_UNSIGNED ),
	SendPropStringT (SENDINFO(m_iszChapterTitle) ),
END_SEND_TABLE()

extern ConVar sv_skyname;

//
// Just to ignore the "wad" field.
//
bool CWorld::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "skyname") )
	{
		// Sent over net now.
		sv_skyname.SetValue( szValue );
	}
	else if ( FStrEq(szKeyName, "newunit") )
	{
		// Single player only.  Clear save directory if set
		if ( atoi(szValue) )
		{
			extern void Game_SetOneWayTransition();
			Game_SetOneWayTransition();
		}
	}
	else if ( FStrEq(szKeyName, "world_mins") )
	{
		Vector vec;
		sscanf(	szValue, "%f %f %f", &vec.x, &vec.y, &vec.z );
		m_WorldMins = vec;
	}
	else if ( FStrEq(szKeyName, "world_maxs") )
	{
		Vector vec;
		sscanf(	szValue, "%f %f %f", &vec.x, &vec.y, &vec.z ); 
		m_WorldMaxs = vec;
	}
	else if ( FStrEq(szKeyName, "timeofday" ) )
	{
		SetTimeOfDay( atoi( szValue ) );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


extern bool		g_fGameOver;
CWorld *g_WorldEntity = NULL;

CWorld* GetWorldEntity()
{
	return g_WorldEntity;
}

//TODO!!! Arthurdead: make the world not networked

void CWorld::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
}

void CWorld::PostConstructor( const char *szClassname )
{
	BaseClass::PostConstructor( szClassname );
}

CWorld::CWorld( )
{
	if(!g_WorldEntity) {
		g_WorldEntity = this;
		AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_NONE );

	m_bColdWorld = false;

	// Set this in the constructor for legacy maps (sjb)
	m_iTimeOfDay = TIME_MIDNIGHT;
}

CWorld::~CWorld( )
{
	if(g_WorldEntity == this) {
		g_WorldEntity = NULL;
	}
}


//------------------------------------------------------------------------------
// Purpose : Add a decal to the world
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWorld::DecalTrace( trace_t *pTrace, char const *decalName)
{
	int index = decalsystem->GetDecalIndexForName( decalName );
	if ( index < 0 )
		return;

	CBroadcastRecipientFilter filter;
	if ( pTrace->hitbox != 0 )
	{
		te->Decal( filter, 0.0f, &pTrace->endpos, &pTrace->startpos, 0, pTrace->hitbox, index );
	}
	else
	{
		te->WorldDecal( filter, 0.0, &pTrace->endpos, index );
	}
}

void CWorld::Spawn( void )
{
	if(g_WorldEntity && g_WorldEntity != this) {
		UTIL_Remove(this);
		return;
	}

	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
	// NOTE:  SHOULD NEVER BE ANYTHING OTHER THAN 1!!!
	SetModelIndex( g_sModelIndexWorld );
	// world model
	SetModelName( AllocPooledString( modelinfo->GetModelName( GetModel() ) ) );
	AddFlag( FL_WORLDBRUSH );

	Precache( );
}

void CWorld::Precache( void )
{
	if(g_WorldEntity && g_WorldEntity != this) {
		return;
	}

	COM_TimestampedLog( "CWorld::Precache - Start" );

	if ( m_iszChapterTitle.Get() != NULL_STRING && !m_bChapterTitleNoMessage )
	{
		DevMsg( 2, "Chapter title: %s\n", STRING(m_iszChapterTitle.Get()) );
		CMessage *pMessage = (CMessage *)CBaseEntity::Create( "env_message", vec3_origin, vec3_angle, NULL );
		if ( pMessage )
		{
			pMessage->SetMessage( m_iszChapterTitle.Get() );
			m_iszChapterTitle.Set( NULL_STRING );

			// send the message entity a play message command, delayed by 1 second
			pMessage->AddSpawnFlags( SF_MESSAGE_ONCE );
			pMessage->SetThink( &CMessage::SUB_CallUseToggle );
			pMessage->SetNextThink( gpGlobals->curtime + 1.0f );
		}
	}

	if ( m_iszDetailSpriteMaterial.Get() != NULL_STRING )
	{
		PrecacheMaterial( STRING( m_iszDetailSpriteMaterial.Get() ) );
	}

	COM_TimestampedLog( "CWorld::Precache - Finish" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CWorld::ShouldDisplayTitle() const
{
	return m_bDisplayTitle;
}

bool CWorld::GetStartDark() const
{
	return m_bStartDark;
}

void CWorld::SetStartDark( bool startdark )
{
	m_bStartDark = startdark;
}

bool CWorld::IsColdWorld( void )
{
	return m_bColdWorld;
}

int CWorld::GetTimeOfDay() const
{
	return m_iTimeOfDay;
}

void CWorld::SetTimeOfDay( int iTimeOfDay )
{
	m_iTimeOfDay = iTimeOfDay;
}

void CWorld::InputSetChapterTitle( inputdata_t &inputdata )
{
	m_iszChapterTitle.Set( inputdata.value.StringID() );
}