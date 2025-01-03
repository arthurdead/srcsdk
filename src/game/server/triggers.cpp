//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawn and use functions for editor-placed triggers.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "globalstate.h"
#include "filters.h"
#include "vstdlib/random.h"
#include "triggers.h"
#include "hierarchy.h"
#include "bspfile.h"
#include "te_effect_dispatch.h"
#include "ammodef.h"
#include "iservervehicle.h"
#include "movevars_shared.h"
#include "physics_prop_ragdoll.h"
#include "props.h"
#include "RagdollBoogie.h"
#include "EntityParticleTrail.h"
#include "in_buttons.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_lead.h"
#include "gameinterface.h"
#include "ilagcompensationmanager.h"
#include "collisionproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEBUG_TRANSITIONS_VERBOSE	2
ConVar g_debug_transitions( "g_debug_transitions", "0", FCVAR_NONE, "Set to 1 and restart the map to be warned if the map has no trigger_transition volumes. Set to 2 to see a dump of all entities & associated results during a transition." );

ConVar mp_transition_players_percent("mp_transition_players_percent",
	"66", FCVAR_NOTIFY,
	"How many players in percent are needed for a level transition?");

// Global list of triggers that care about weapon fire
// Doesn't need saving, the triggers re-add themselves on restore.
CUtlVector< CHandle<CTriggerMultiple> >	g_hWeaponFireTriggers;

extern CServerGameDLL	g_ServerGameDLL;
extern bool				g_fGameOver;
extern bool Transitioned;

ConVar showtriggers( "showtriggers", "0", FCVAR_CHEAT, "Shows trigger brushes" );

bool IsTriggerClass( CBaseEntity *pEntity );

// Command to dynamically toggle trigger visibility
void Cmd_ShowtriggersToggle_f( const CCommand &args )
{
	// Loop through the entities in the game and make visible anything derived from CBaseTrigger
	CBaseEntity *pEntity = gEntList.FirstEnt();
	while ( pEntity )
	{
		if ( IsTriggerClass(pEntity) )
		{
			// If a classname is specified, only show triggles of that type
			if ( args.ArgC() > 1 )
			{
				const char *sClassname = args[1];
				if ( sClassname && sClassname[0] )
				{
					if ( !FClassnameIs( pEntity, sClassname ) )
					{
						pEntity = gEntList.NextEnt( pEntity );
						continue;
					}
				}
			}

			if ( pEntity->IsEffectActive( EF_NODRAW ) )
			{
				pEntity->RemoveEffects( EF_NODRAW );
			}
			else
			{
				pEntity->AddEffects( EF_NODRAW );
			}
		}

		pEntity = gEntList.NextEnt( pEntity );
	}
}

static ConCommand showtriggers_toggle( "showtriggers_toggle", Cmd_ShowtriggersToggle_f, "Toggle show triggers", FCVAR_CHEAT );

// Global Savedata for base trigger
BEGIN_MAPENTITY( CBaseTrigger )

	// Keyfields
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),

	DEFINE_KEYFIELD( m_bDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_KEYFIELD( m_flWait, FIELD_FLOAT, "wait" ),
	DEFINE_KEYFIELD( m_sMaster, FIELD_STRING, "master" ),

	// Inputs	
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TouchTest", InputTouchTest ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartTouch", InputStartTouch ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EndTouch", InputEndTouch ),

	// Outputs
	DEFINE_OUTPUT( m_OnStartTouch, "OnStartTouch"),
	DEFINE_OUTPUT( m_OnStartTouchAll, "OnStartTouchAll"),
	DEFINE_OUTPUT( m_OnEndTouch, "OnEndTouch"),
	DEFINE_OUTPUT( m_OnEndTouchAll, "OnEndTouchAll"),
	DEFINE_OUTPUT( m_OnTouching, "OnTouching" ),
	DEFINE_OUTPUT( m_OnNotTouching, "OnNotTouching" ),

END_MAPENTITY()


LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );

IMPLEMENT_SERVERCLASS_ST( CBaseTrigger, DT_BaseTrigger )
	SendPropBool( SENDINFO( m_bClientSidePredicted ) ),
	SendPropInt( SENDINFO(m_spawnflags), -1, SPROP_NOSCALE )
END_SEND_TABLE()

CBaseTrigger::CBaseTrigger()
{
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
	m_bClientSidePredicted = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputEnable( inputdata_t &inputdata )
{ 
	Enable();
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputDisable( inputdata_t &inputdata )
{ 
	Disable();
}

void CBaseTrigger::InputTouchTest( inputdata_t &inputdata )
{
	TouchTest();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CBaseTrigger::Spawn()
{
	if ( HasSpawnFlags( SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS ) || HasSpawnFlags( SF_TRIGGER_ONLY_NPCS_IN_VEHICLES ) )
	{
		// Automatically set this trigger to work with NPC's.
		AddSpawnFlags( SF_TRIGGER_ALLOW_NPCS );
	}

	if ( HasSpawnFlags( SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES ) )
	{
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	}

	if ( HasSpawnFlags( SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES ) )
	{
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
	}

	BaseClass::Spawn();
}


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CBaseTrigger::UpdateOnRemove( void )
{
	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->RemoveTrigger();
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose: Turns on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Enable( void )
{
	m_bDisabled = false;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( true );
	}

	if (!IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		AddSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CBaseTrigger::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose: Called after player becomes active in the game
//-----------------------------------------------------------------------------
void CBaseTrigger::PostClientActive( void )
{
	BaseClass::PostClientActive();

	if ( !m_bDisabled )
	{
		PhysicsTouchTriggers();
	}
}

//------------------------------------------------------------------------------
// Purpose: Turns off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Disable( void )
{ 
	m_bDisabled = true;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( false );
	}

	if (IsSolidFlagSet(FSOLID_TRIGGER))
	{
		RemoveSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}
//------------------------------------------------------------------------------
// Purpose: Tests to see if anything is touching this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::TouchTest( void )
{
	// If the trigger is disabled don't test to see if anything is touching it.
	if ( !m_bDisabled )
	{
		if ( m_hTouchingEntities.Count() !=0 )
		{
			
			m_OnTouching.FireOutput( m_hTouchingEntities[0], this );
		}
		else
		{
			m_OnNotTouching.FireOutput( this, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CBaseTrigger::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (IsSolidFlagSet(FSOLID_TRIGGER)) 
		{
			Q_strncpy(tempstr,"State: Enabled",sizeof(tempstr));
		}
		else
		{
			Q_strncpy(tempstr,"State: Disabled",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CBaseTrigger::PointIsWithin( const Vector &vecPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( vecPoint, vecPoint );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTrigger::InitTrigger( )
{
	SetSolid( GetParent() ? SOLID_VPHYSICS : SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );
	if (m_bDisabled)
	{
		RemoveSolidFlags( FSOLID_TRIGGER );	
	}
	else
	{
		AddSolidFlags( FSOLID_TRIGGER );	
	}

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		AddEffects( EF_NODRAW );
	}

	m_hTouchingEntities.Purge();

	if ( HasSpawnFlags( SF_TRIG_TOUCH_DEBRIS ) )
	{
		CollisionProp()->AddSolidFlags( FSOLID_TRIGGER_TOUCH_DEBRIS );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CBaseTrigger::PassesTriggerFilters(CBaseEntity *pOther)
{
	// First test spawn flag filters
	if ( HasSpawnFlags(SF_TRIGGER_ALLOW_ALL) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS) 
		||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_ITEMS) && pOther->GetMoveType() == MOVETYPE_FLYGRAVITY)
		||
		(	HasSpawnFlags(SF_TRIG_TOUCH_DEBRIS) && 
			(pOther->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
			pOther->GetCollisionGroup() == COLLISION_GROUP_DEBRIS_TRIGGER || 
			pOther->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS)
		)
		)
	{
		if ( pOther->GetFlags() & FL_NPC )
		{
			CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

			if( pNPC )
			{
				if ( HasSpawnFlags( SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS ) )
				{
					if ( !pNPC->IsPlayerAlly() )
					{
						return false;
					}
				}

				if ( HasSpawnFlags( SF_TRIGGER_ONLY_NPCS_IN_VEHICLES ) )
				{
					if ( !pNPC->IsInAVehicle() )
						return false;
				}
			}
		}

		bool bOtherIsPlayer = pOther->IsPlayer();

		if ( bOtherIsPlayer )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)pOther;
			if ( !pPlayer->IsAlive() )
				return false;

			if ( HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES) )
			{
				if ( !pPlayer->IsInAVehicle() )
					return false;

				// Make sure we're also not exiting the vehicle at the moment
				IServerVehicle *pVehicleServer = pPlayer->GetVehicle();
				if ( pVehicleServer == NULL )
					return false;

				if ( pVehicleServer->IsPassengerExiting() )
					return false;
			}

			if ( HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES) )
			{
				if ( pPlayer->IsInAVehicle() )
					return false;
			}

			if ( HasSpawnFlags( SF_TRIGGER_DISALLOW_BOTS ) )
			{
				if ( pPlayer->IsFakeClient() )
					return false;
			}
		}

		CBaseFilter *pFilter = m_hFilter.Get();
		return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called to simulate what happens when an entity touches the trigger.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputStartTouch( inputdata_t &inputdata )
{
	//Pretend we just touched the trigger.
	StartTouch( inputdata.pCaller );
}
//-----------------------------------------------------------------------------
// Purpose: Called to simulate what happens when an entity leaves the trigger.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputEndTouch( inputdata_t &inputdata )
{
	//And... pretend we left the trigger.
	EndTouch( inputdata.pCaller );	
}

//-----------------------------------------------------------------------------
// Purpose: Called when the first entity that passes filters starts touching us.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::OnStartTouchAll(CBaseEntity *pOther)
{
	m_OnStartTouchAll.FireOutput( pOther, this );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the last entity stops touching us
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::OnEndTouchAll(CBaseEntity *pOther)
{
	m_OnEndTouchAll.FireOutput(pOther, this);
}

//-----------------------------------------------------------------------------
// Purpose: Called when an entity starts touching us.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::StartTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther) )
	{
		EHANDLE hOther;
		hOther = pOther;
		
		bool bAdded = false;
		if ( m_hTouchingEntities.Find( hOther ) == m_hTouchingEntities.InvalidIndex() )
		{
			m_hTouchingEntities.AddToTail( hOther );
			bAdded = true;
		}

		m_OnStartTouch.FireOutput(pOther, this);

		if ( bAdded && ( m_hTouchingEntities.Count() == 1 ) )
		{
			// First entity to touch us that passes our filters
			OnStartTouchAll( pOther );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when an entity stops touching us.
// Input  : pOther - The entity that was touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::EndTouch(CBaseEntity *pOther)
{
	if ( IsTouching( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;
		m_hTouchingEntities.FindAndRemove( hOther );
		
		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		//if ( !m_bDisabled )
		//{
			m_OnEndTouch.FireOutput(pOther, this);
		//}

		// If there are no more entities touching this trigger, fire the lost all touches
		// Loop through the touching entities backwards. Clean out old ones, and look for existing
		bool bFoundOtherTouchee = false;
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther;
			hOther = m_hTouchingEntities[i];

			if ( !hOther )
			{
				m_hTouchingEntities.Remove( i );
			}
			else if ( hOther->IsPlayer() && !hOther->IsAlive() )
			{
#ifdef STAGING_ONLY
				if ( !HushAsserts() )
				{
					AssertMsg( false, "Dead player [%s] is still touching this trigger at [%f %f %f]", hOther->GetEntityName().ToCStr(), XYZ( hOther->GetAbsOrigin() ) );
				}
				Warning( "Dead player [%s] is still touching this trigger at [%f %f %f]", hOther->GetEntityName().ToCStr(), XYZ( hOther->GetAbsOrigin() ) );
#endif
				m_hTouchingEntities.Remove( i );
			}
			else
			{
				bFoundOtherTouchee = true;
			}
		}

		//FIXME: Without this, triggers fire their EndTouch outputs when they are disabled!
		// Didn't find one?
		if ( !bFoundOtherTouchee /*&& !m_bDisabled*/ )
		{
			OnEndTouchAll( pOther );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching us
//-----------------------------------------------------------------------------
bool CBaseTrigger::IsTouching( CBaseEntity *pOther )
{
	EHANDLE hOther;
	hOther = pOther;
	return ( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the first entity of the specified type being touched by this trigger
//-----------------------------------------------------------------------------
CBaseEntity *CBaseTrigger::GetTouchedEntityOfType( const char *sClassName )
{
	int iCount = m_hTouchingEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CBaseEntity *pEntity = m_hTouchingEntities[i];
		if ( FClassnameIs( pEntity, sClassName ) )
			return pEntity;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Toggles this trigger between enabled and disabled.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputToggle( inputdata_t &inputdata )
{
	if (IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		RemoveSolidFlags(FSOLID_TRIGGER);
	}
	else
	{
		AddSolidFlags(FSOLID_TRIGGER);
	}

	PhysicsTouchTriggers();
}


//-----------------------------------------------------------------------------
// Purpose: Removes anything that touches it. If the trigger has a targetname,
//			firing it will toggle state.
//-----------------------------------------------------------------------------
class CTriggerRemove : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerRemove, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	
	DECLARE_MAPENTITY();
	
	// Outputs
	COutputEvent m_OnRemove;
};

BEGIN_MAPENTITY( CTriggerRemove )

	// Outputs
	DEFINE_OUTPUT( m_OnRemove, "OnRemove" ),

END_MAPENTITY()


LINK_ENTITY_TO_CLASS( trigger_remove, CTriggerRemove );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerRemove::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CTriggerRemove::Touch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	UTIL_Remove( pOther );
}


BEGIN_MAPENTITY( CTriggerHurt )

	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flDamageCap, FIELD_FLOAT, "damagecap" ),
	DEFINE_KEYFIELD( m_bitsDamageInflict, FIELD_INTEGER, "damagetype" ),
	DEFINE_KEYFIELD( m_damageModel, FIELD_INTEGER, "damagemodel" ),
	DEFINE_KEYFIELD( m_bNoDmgForce, FIELD_BOOLEAN, "nodmgforce" ),

	DEFINE_KEYFIELD( m_flHurtRate, FIELD_FLOAT, "hurtrate" ),

	// Inputs
	DEFINE_INPUT( m_flDamage, FIELD_FLOAT, "SetDamage" ),

	// Outputs
	DEFINE_OUTPUT( m_OnHurt, "OnHurt" ),
	DEFINE_OUTPUT( m_OnHurtPlayer, "OnHurtPlayer" ),

END_MAPENTITY()


LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

IMPLEMENT_AUTO_LIST( ITriggerHurtAutoList );

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerHurt::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	m_flOriginalDamage = m_flDamage;

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( &CTriggerHurtShim::RadiationThinkShim );
		SetNextThink( gpGlobals->curtime + random_valve->RandomFloat(0.0, 0.5) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CTriggerHurt::RadiationThink( void )
{
	// check to see if a player is in pvs
	// if not, continue	
	Vector vecSurroundMins, vecSurroundMaxs;
	CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
	CBasePlayer *pPlayer = static_cast<CBasePlayer *>(UTIL_FindClientInPVS( vecSurroundMins, vecSurroundMaxs ));

	if (pPlayer)
	{
		// get range to player;
		float flRange = CollisionProp()->CalcDistanceFromPoint( pPlayer->WorldSpaceCenter() );
		flRange *= 3.0f;
		pPlayer->NotifyNearbyRadiationSource(flRange);
	}

	float dt = gpGlobals->curtime - m_flLastDmgTime;
	if ( dt >= m_flHurtRate )
	{
		HurtAllTouchers( dt );
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
}


//-----------------------------------------------------------------------------
// Purpose: When touched, a hurt trigger does m_flDamage points of damage each half-second.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
bool CTriggerHurt::HurtEntity( CBaseEntity *pOther, float damage )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther) )
		return false;

	// If player is disconnected, we're probably in this routine via the
	//  PhysicsRemoveTouchedList() function to make sure all Untouch()'s are called for the
	//  player. Calling TakeDamage() in this case can get into the speaking criteria, which
	//  will then loop through the control points and the touched list again. We shouldn't
	//  need to hurt players that are disconnected, so skip all of this...
	bool bPlayerDisconnected = pOther->IsPlayer() && ( ((CBasePlayer *)pOther)->IsConnected() == false );
	if ( bPlayerDisconnected )
		return false;

	if ( damage < 0 )
	{
		pOther->TakeHealth( -damage, m_bitsDamageInflict );
	}
	else
	{
		// The damage position is the nearest point on the damaged entity
		// to the trigger's center. Not perfect, but better than nothing.
		Vector vecCenter = CollisionProp()->WorldSpaceCenter();

		Vector vecDamagePos;
		pOther->CollisionProp()->CalcNearestPoint( vecCenter, &vecDamagePos );

		CTakeDamageInfo info( this, this, damage, m_bitsDamageInflict );
		info.SetDamagePosition( vecDamagePos );
		if ( !m_bNoDmgForce )
		{
			GuessDamageForce( &info, ( vecDamagePos - vecCenter ), vecDamagePos );
		}
		else
		{
			info.SetDamageForce( vec3_origin );
		}
		
		pOther->TakeDamage( info );
	}

	if (pOther->IsPlayer())
	{
		m_OnHurtPlayer.FireOutput(pOther, this);
	}
	else
	{
		m_OnHurt.FireOutput(pOther, this);
	}
	m_hurtEntities.AddToTail( EHANDLE(pOther) );
	//NDebugOverlay::Box( pOther->GetAbsOrigin(), pOther->WorldAlignMins(), pOther->WorldAlignMaxs(), 255,0,0,0,0.5 );
	return true;
}

void CTriggerHurt::HurtThink()
{
	// if I hurt anyone, think again
	if ( HurtAllTouchers( 0.5 ) <= 0 )
	{
		SetThink(NULL);
	}
	else
	{
		SetNextThink( gpGlobals->curtime + m_flHurtRate );
	}
}

void CTriggerHurt::EndTouch( CBaseEntity *pOther )
{
	if (PassesTriggerFilters(pOther))
	{
		EHANDLE hOther;
		hOther = pOther;

		// if this guy has never taken damage, hurt him now
		if ( !m_hurtEntities.HasElement( hOther ) )
		{
			HurtEntity( pOther, m_flDamage * 0.5 );
		}
	}
	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: called from RadiationThink() as well as HurtThink()
//			This function applies damage to any entities currently touching the
//			trigger
// Input  : dt - time since last call
// Output : int - number of entities actually hurt
//-----------------------------------------------------------------------------
#define TRIGGER_HURT_FORGIVE_TIME	3.0f	// time in seconds
int CTriggerHurt::HurtAllTouchers( float dt )
{
	int hurtCount = 0;
	// half second worth of damage
	float fldmg = m_flDamage * dt;
	m_flLastDmgTime = gpGlobals->curtime;

	m_hurtEntities.RemoveAll();

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch )
			{
				if ( HurtEntity( pTouch, fldmg ) )
				{
					hurtCount++;
				}
			}
		}
	}

	if( m_damageModel == DAMAGEMODEL_DOUBLE_FORGIVENESS )
	{
		if( hurtCount == 0 )
		{
			if( gpGlobals->curtime > m_flDmgResetTime  )
			{
				// Didn't hurt anyone. Reset the damage if it's time. (hence, the forgiveness)
				m_flDamage = m_flOriginalDamage;
			}
		}
		else
		{
			// Hurt someone! double the damage
			m_flDamage *= 2.0f;

			if( m_flDamage > m_flDamageCap )
			{
				// Clamp
				m_flDamage = m_flDamageCap;
			}

			// Now, put the damage reset time into the future. The forgive time is how long the trigger
			// must go without harming anyone in order that its accumulated damage be reset to the amount
			// set by the level designer. This is a stop-gap for an exploit where players could hop through
			// slime and barely take any damage because the trigger would reset damage anytime there was no
			// one in the trigger when this function was called. (sjb)
			m_flDmgResetTime = gpGlobals->curtime + TRIGGER_HURT_FORGIVE_TIME;
		}
	}

	return hurtCount;
}

void CTriggerHurt::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CTriggerHurtShim::HurtThinkShim );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerHurt::KeyValue( const char *szKeyName, const char *szValue )
{
	// Additional OR flags
	if (FStrEq( szKeyName, "damageor" ) || FStrEq( szKeyName, "damagepresets" ))
	{
		m_bitsDamageInflict |= atoi(szValue);
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if this point is in any trigger_hurt zones with positive damage
//-----------------------------------------------------------------------------
bool IsTakingTriggerHurtDamageAtPoint( const Vector &vecPoint )
{
	for ( int i = 0; i < ITriggerHurtAutoList::AutoList().Count(); i++ )
	{
		// Some maps use trigger_hurt with negative values as healing triggers; don't consider those
		CTriggerHurt *pTrigger = static_cast<CTriggerHurt*>( ITriggerHurtAutoList::AutoList()[i] );
		if ( !pTrigger->m_bDisabled && pTrigger->PointIsWithin( vecPoint ) && pTrigger->m_flDamage > 0.f )
		{
			return true;
		}
	}

	return false;
}


// ##################################################################################
//	>> TriggerMultiple
// ##################################################################################
LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


BEGIN_MAPENTITY( CTriggerMultiple )

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger")

END_MAPENTITY()



//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerMultiple::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if (m_flWait == 0)
	{
		m_flWait = 0.2;
	}

	ASSERTSZ(GetHealth() == 0, "trigger_multiple with health");
	SetTouch( &CTriggerMultiple::MultiTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Touch function. Activates the trigger.
// Input  : pOther - The thing that touched us.
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther))
	{
		ActivateMultiTrigger( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pActivator - 
//-----------------------------------------------------------------------------
void CTriggerMultiple::ActivateMultiTrigger(CBaseEntity *pActivator)
{
	if (GetNextThink() > gpGlobals->curtime)
		return;         // still waiting for reset time

	m_hActivator = pActivator;

	m_OnTrigger.FireOutput(m_hActivator, this);

	if (m_flWait > 0)
	{
		SetThink( &CTriggerMultiple::MultiWaitOver );
		SetNextThink( gpGlobals->curtime + m_flWait );
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink(  &CTriggerMultiple::SUB_Remove );
	}
}


//-----------------------------------------------------------------------------
// Purpose: The wait time has passed, so set back up for another activation
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiWaitOver( void )
{
	SetThink( NULL );
}

// ##################################################################################
//	>> TriggerOnce
// ##################################################################################
class CTriggerOnce : public CTriggerMultiple
{
public:
	DECLARE_CLASS( CTriggerOnce, CTriggerMultiple );

	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );

void CTriggerOnce::Spawn( void )
{
	BaseClass::Spawn();

	m_flWait = -1;
}


// ##################################################################################
//	>> TriggerLook
//
//  Triggers once when player is looking at m_target
//
// ##################################################################################
#define SF_TRIGGERLOOK_FIREONCE		128
#define SF_TRIGGERLOOK_USEVELOCITY	256

class CTriggerLook : public CTriggerOnce
{
public:
	DECLARE_CLASS( CTriggerLook, CTriggerOnce );

	CUtlVector<EHANDLE> m_hLookTargets;
	float m_flFieldOfView;
	float m_flLookTime;			// How long must I look for
	float m_flLookTimeTotal;	// How long have I looked
	float m_flLookTimeLast;		// When did I last look
	float m_flTimeoutDuration;	// Number of seconds after start touch to fire anyway
	bool m_bTimeoutFired;		// True if the OnTimeout output fired since the last StartTouch.
	EHANDLE m_hActivator;		// The entity that triggered us.
	bool m_bUseLOS;				// Makes lookers use LOS calculations in addition to viewcone calculations
	bool m_bUseLookEntityAsCaller;	// Fires OnTrigger with the seen entity


	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void StartTouch(CBaseEntity *pOther);
	void EndTouch( CBaseEntity *pOther );
	int	 DrawDebugTextOverlays(void);

	DECLARE_MAPENTITY();

private:

	void Trigger(CBaseEntity *pActivator, bool bTimeout, CBaseEntity *pCaller = NULL);
	void TimeoutThink();

	COutputEvent m_OnTimeout;
};

LINK_ENTITY_TO_CLASS( trigger_look, CTriggerLook );
BEGIN_MAPENTITY( CTriggerLook )

	DEFINE_KEYFIELD( m_flTimeoutDuration, FIELD_FLOAT, "timeout" ),

	DEFINE_OUTPUT( m_OnTimeout, "OnTimeout" ),

	// Inputs
	DEFINE_INPUT( m_flFieldOfView,		FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( m_flLookTime,			FIELD_FLOAT,	"LookTime" ),

	DEFINE_KEYFIELD( m_bUseLOS, FIELD_BOOLEAN, "UseLOS" ),
	DEFINE_KEYFIELD( m_bUseLookEntityAsCaller, FIELD_BOOLEAN, "LookEntityCaller" ),

END_MAPENTITY()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::Spawn( void )
{
	m_flLookTimeTotal = -1;
	m_bTimeoutFired = false;

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerLook::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if (pOther->IsPlayer() && m_flTimeoutDuration)
	{
		m_bTimeoutFired = false;
		m_hActivator = pOther;
		SetThink(&CTriggerLook::TimeoutThink);
		SetNextThink(gpGlobals->curtime + m_flTimeoutDuration);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerLook::TimeoutThink(void)
{
	Trigger(m_hActivator, true);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch(pOther);

	if (pOther->IsPlayer())
	{
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );

		m_flLookTimeTotal = -1;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerLook::Touch(CBaseEntity *pOther)
{
	// Don't fire the OnTrigger if we've already fired the OnTimeout. This will be
	// reset in OnEndTouch.
	if (m_bTimeoutFired)
		return;

	// --------------------------------
	// Make sure we have a look target
	// --------------------------------
	if (m_hLookTargets.Count() <= 0)
	{
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_target, this, m_hActivator, this );
		while (pEntity)
		{
			m_hLookTargets.AddToTail(pEntity);
			pEntity = gEntList.FindEntityByName( pEntity, m_target, this, m_hActivator, this );
		}
	}

	// This is designed for single player only
	// so we'll always have the same player
	if (pOther->IsPlayer())
	{
		// ----------------------------------------
		// Check that toucher is facing the target
		// ----------------------------------------
		Vector vLookDir;
		if ( HasSpawnFlags( SF_TRIGGERLOOK_USEVELOCITY ) )
		{
			vLookDir = pOther->GetAbsVelocity();
			if ( vLookDir == vec3_origin )
			{
				// See if they're in a vehicle
				CBasePlayer *pPlayer = (CBasePlayer *)pOther;
				if ( pPlayer->IsInAVehicle() )
				{
					vLookDir = pPlayer->GetVehicle()->GetVehicleEnt()->GetSmoothedVelocity();
				}
			}
			VectorNormalize( vLookDir );
		}
		else
		{
			vLookDir = ((CBaseCombatCharacter*)pOther)->EyeDirection3D( );
		}

		// Check if the player is looking at any of the entities, even if they turn to look at another entity candidate.
		// This is how we're doing support for multiple entities without redesigning trigger_look.
		EHANDLE hLookingAtEntity = NULL;
		for (int i = 0; i < m_hLookTargets.Count(); i++)
		{
			if (!m_hLookTargets[i])
				continue;

			Vector vTargetDir = m_hLookTargets[i]->GetAbsOrigin() - pOther->EyePosition();
			VectorNormalize(vTargetDir);

			float fDotPr = DotProduct(vLookDir,vTargetDir);
			if (fDotPr > m_flFieldOfView && (!m_bUseLOS || pOther->FVisible(m_hLookTargets[i])))
			{
				hLookingAtEntity = m_hLookTargets[i];
				break;
			}
		}

		if (hLookingAtEntity != NULL)
		{
			// Is it the first time I'm looking?
			if (m_flLookTimeTotal == -1)
			{
				m_flLookTimeLast	= gpGlobals->curtime;
				m_flLookTimeTotal	= 0;
			}
			else
			{
				m_flLookTimeTotal	+= gpGlobals->curtime - m_flLookTimeLast;
				m_flLookTimeLast	=  gpGlobals->curtime;
			}

			if (m_flLookTimeTotal >= m_flLookTime)
			{
				Trigger(pOther, false, hLookingAtEntity);
			}
		}
		else
		{
			m_flLookTimeTotal	= -1;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the trigger is fired by look logic or timeout.
//-----------------------------------------------------------------------------
void CTriggerLook::Trigger(CBaseEntity *pActivator, bool bTimeout, CBaseEntity *pCaller)
{
	if (bTimeout)
	{
		// Fired due to timeout (player never looked at the target).
		m_OnTimeout.FireOutput(pActivator, this);

		// Don't fire the OnTrigger for this toucher.
		m_bTimeoutFired = true;
	}
	else
	{
		// Fire because the player looked at the target.
		m_OnTrigger.FireOutput(pActivator, m_bUseLookEntityAsCaller ? pCaller : this);
		m_flLookTimeTotal = -1;

		// Cancel the timeout think.
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );
	}

	if (HasSpawnFlags(SF_TRIGGERLOOK_FIREONCE))
	{
		SetThink(&CTriggerLook::SUB_Remove);
		SetNextThink(gpGlobals->curtime);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerLook::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// ----------------
		// Print Look time
		// ----------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Time:   %3.2f",m_flLookTime - MAX(0,m_flLookTimeTotal));
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


// ##################################################################################
//	>> TriggerVolume
// ##################################################################################
class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	DECLARE_CLASS( CTriggerVolume, CPointEntity );

	virtual void	Spawn( void );
	virtual void	Activate( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume::Spawn( void )
{
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		AddEffects( EF_NODRAW );
	}
}

// Make a special naming exception for Portal
void CTriggerVolume::Activate( void )
{
	BaseClass::Activate();


}

#define SF_CHANGELEVEL_NOTOUCH		0x0002
#define SF_CHANGELEVEL_CHAPTER		0x0004

#define cchMapNameMost 32

enum
{
	TRANSITION_VOLUME_SCREENED_OUT = 0,
	TRANSITION_VOLUME_NOT_FOUND = 1,
	TRANSITION_VOLUME_PASSED = 2,
};


//------------------------------------------------------------------------------
// Reesponsible for changing levels when the player touches it
//------------------------------------------------------------------------------
class CChangeLevel : public CBaseTrigger
{
	DECLARE_MAPENTITY();

public:
	DECLARE_CLASS( CChangeLevel, CBaseTrigger );

	void Spawn( void );
	void Activate( void );
	bool KeyValue( const char *szKeyName, const char *szValue );

	static int ChangeList( levellist_t *pLevelList, int maxList );

private:
	void TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	void InputChangeLevel( inputdata_t &inputdata );

	bool IsEntityInTransition( CBaseEntity *pEntity );
	void NotifyEntitiesOutOfTransition();

	void WarnAboutActiveLead( void );

	static CBaseEntity *FindLandmark( const char *pLandmarkName );
	static int AddTransitionToList( levellist_t *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, const char *pVolumeName );

	// Builds the list of entities to save when moving across a transition
	static int BuildChangeLevelList( levellist_t *pLevelList, int maxList );

	// Builds the list of entities to bring across a particular transition
	static int BuildEntityTransitionList( CBaseEntity *pLandmarkEntity, const char *pLandmarkName, CBaseEntity **ppEntList, int *pEntityFlags, int nMaxList );

	// Adds a single entity to the transition list, if appropriate. Returns the new count
	static int AddEntityToTransitionList( CBaseEntity *pEntity, int flags, int nCount, CBaseEntity **ppEntList, int *pEntityFlags );

	// Adds in all entities depended on by entities near the transition
	static int AddDependentEntities( int nCount, CBaseEntity **ppEntList, int *pEntityFlags, int nMaxList );

private:
	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	bool m_bTouched;

	// Outputs
	COutputEvent m_OnChangeLevel;
};


LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
BEGIN_MAPENTITY( CChangeLevel )

	DEFINE_INPUTFUNC( FIELD_VOID, "ChangeLevel", InputChangeLevel ),

	// Outputs
	DEFINE_OUTPUT( m_OnChangeLevel, "OnChangeLevel"),

END_MAPENTITY()


//
// Cache user-entity-field values until spawn is called.
//

bool CChangeLevel::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "map"))
	{
		if (strlen(szValue) >= cchMapNameMost)
		{
			Warning( "Map name '%s' too long (32 chars)\n", szValue );
			Assert(0);
		}
		Q_strncpy(m_szMapName, szValue, sizeof(m_szMapName));
	}
	else if (FStrEq(szKeyName, "landmark"))
	{
		if (strlen(szValue) >= cchMapNameMost)
		{
			Warning( "Landmark name '%s' too long (32 chars)\n", szValue );
			Assert(0);
		}
		
		Q_strncpy(m_szLandmarkName, szValue, sizeof( m_szLandmarkName ));
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}



void CChangeLevel::Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
	{
		Msg( "a trigger_changelevel doesn't have a map" );
	}

	if ( FStrEq( m_szLandmarkName, "" ) )
	{
		Msg( "trigger_changelevel to %s doesn't have a landmark", m_szMapName );
	}

	InitTrigger();
	
	if ( !HasSpawnFlags(SF_CHANGELEVEL_NOTOUCH) )
	{
		SetTouch( &CChangeLevel::TouchChangeLevel );
	}

//	Msg( "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}

void CChangeLevel::Activate( void )
{
	BaseClass::Activate();

	if ( gpGlobals->eLoadType == MapLoad_NewGame )
	{
		if ( HasSpawnFlags( SF_CHANGELEVEL_CHAPTER ) )
		{
			VPhysicsInitStatic();
			RemoveSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
			SetTouch( NULL );
			return;
		}
	}

	// Level transitions will bust if they are in solid
	CBaseEntity *pLandmark = FindLandmark( m_szLandmarkName );
	if ( pLandmark )
	{
		int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
		if ( clusterIndex < 0 )
		{
			Warning( "trigger_changelevel to map %s has a landmark embedded in solid!\n"
					"This will break level transitions!\n", m_szMapName ); 
		}

		if ( g_debug_transitions.GetInt() )
		{
			if ( !gEntList.FindEntityByClassname( NULL, "trigger_transition" ) )
			{
				Warning( "Map has no trigger_transition volumes for landmark %s\n", m_szLandmarkName );
			}
		}
	}

	m_bTouched = false;
}


static char st_szNextMap[cchMapNameMost];
static char st_szNextSpot[cchMapNameMost];

// Used to show debug for only the transition volume we're currently in
static int g_iDebuggingTransition = 0;

CBaseEntity *CChangeLevel::FindLandmark( const char *pLandmarkName )
{
	CBaseEntity *pentLandmark;

	pentLandmark = gEntList.FindEntityByName( NULL, pLandmarkName );
	while ( pentLandmark )
	{
		// Found the landmark
		if ( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = gEntList.FindEntityByName( pentLandmark, pLandmarkName );
	}
	Warning( "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Allows level transitions to be triggered by buttons, etc.
//-----------------------------------------------------------------------------
void CChangeLevel::InputChangeLevel( inputdata_t &inputdata )
{
	ChangeLevelNow( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Performs the level change and fires targets.
// Input  : pActivator - 
//-----------------------------------------------------------------------------
bool CChangeLevel::IsEntityInTransition( CBaseEntity *pEntity )
{
	int transitionState = InTransitionVolume(pEntity, m_szLandmarkName);
	if ( transitionState == TRANSITION_VOLUME_SCREENED_OUT )
	{
		return false;
	}

	// look for a landmark entity		
	CBaseEntity	*pLandmark = FindLandmark( m_szLandmarkName );

	if ( !pLandmark )
		return false;

	// Check to make sure it's also in the PVS of landmark
	byte pvs[MAX_MAP_CLUSTERS/8];
	int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
	engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
	Vector vecSurroundMins, vecSurroundMaxs;
	pEntity->CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );

	return engine->CheckBoxInPVS( vecSurroundMins, vecSurroundMaxs, pvs, sizeof( pvs ) );
}

void CChangeLevel::NotifyEntitiesOutOfTransition()
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		// Found the landmark
		if ( pEnt->ObjectCaps() & FCAP_NOTIFY_ON_TRANSITION )
		{
			variant_t emptyVariant;
			if ( !(pEnt->ObjectCaps() & (FCAP_ACROSS_TRANSITION|FCAP_FORCE_TRANSITION)) || !IsEntityInTransition( pEnt ) )
			{
				pEnt->AcceptInput( "OutsideTransition", this, this, emptyVariant, 0 );
			}
			else
			{
				pEnt->AcceptInput( "InsideTransition", this, this, emptyVariant, 0 );
			}
		}
		pEnt = gEntList.NextEnt( pEnt );
	}
}

//------------------------------------------------------------------------------
// Purpose : Checks all spawned AIs and prints a warning if any are actively leading
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CChangeLevel::WarnAboutActiveLead( void )
{
	int					i;
	CAI_BaseNPC *		ai;
	CAI_BehaviorBase *	behavior;

	for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		ai = g_AI_Manager.AccessAIs()[i];
		behavior = ai->GetPrimaryBehavior();
		if ( behavior )
		{
			if ( dynamic_cast<CAI_LeadBehavior *>( behavior ) )
			{
				Warning( "Entity '%s' is still actively leading\n", STRING( ai->GetEntityName() ) );
			} 
		}
	}
}

void CChangeLevel::ChangeLevelNow( CBaseEntity *pActivator )
{
	CBaseEntity	*pLandmark=NULL;

	Assert(!FStrEq(m_szMapName, ""));

	if ( mp_transition_players_percent.GetInt() > 0 )
	{
		int totalPlayers = 0;
		int transitionPlayers = 0;
		for( int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->IsAlive() )
			{
				totalPlayers++;

				int transitionState = InTransitionVolume(pPlayer, m_szLandmarkName);
				if ( transitionState == TRANSITION_VOLUME_SCREENED_OUT )
				{
					continue;
				}

				// no transition volumes, check PVS of landmark
				if ( transitionState == TRANSITION_VOLUME_NOT_FOUND )
				{
					byte pvs[MAX_MAP_CLUSTERS/8];
					int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
					engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
					if ( pPlayer )
					{
						Vector vecSurroundMins, vecSurroundMaxs;
						pPlayer->CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
						bool playerInPVS = engine->CheckBoxInPVS( vecSurroundMins, vecSurroundMaxs, pvs, sizeof( pvs ) );

						//Assert( playerInPVS );
						if ( !playerInPVS )
						{
							continue;
						}
					}
				}

				transitionPlayers++;
			}
		}

		if ( ( (int) (transitionPlayers / totalPlayers * 100) ) < mp_transition_players_percent.GetInt() )
		{
			Msg("Transitions: Not enough players to trigger level change\n");
			return;
		}
	}
	else if(pActivator && pActivator->IsPlayer())
	{
		CBasePlayer* pPlayer = (CBasePlayer *)pActivator;

		int transitionState = InTransitionVolume(pPlayer, m_szLandmarkName);
		if ( transitionState == TRANSITION_VOLUME_SCREENED_OUT )
		{
			return;
		}

		// no transition volumes, check PVS of landmark
		if ( transitionState == TRANSITION_VOLUME_NOT_FOUND )
		{
			byte pvs[MAX_MAP_CLUSTERS/8];
			int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
			engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
			if ( pPlayer )
			{
				Vector vecSurroundMins, vecSurroundMaxs;
				pPlayer->CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
				bool playerInPVS = engine->CheckBoxInPVS( vecSurroundMins, vecSurroundMaxs, pvs, sizeof( pvs ) );

				//Assert( playerInPVS );
				if ( !playerInPVS )
				{
					return;
				}
			}
		}
	}

	// Some people are firing these multiple times in a frame, disable
	if ( m_bTouched )
		return;

	m_bTouched = true;

	// look for a landmark entity		
	pLandmark = FindLandmark( m_szLandmarkName );

	if ( !pLandmark )
		return;

	WarnAboutActiveLead();

	g_iDebuggingTransition = 0;
	st_szNextSpot[0] = 0;	// Init landmark to NULL
	Q_strncpy(st_szNextSpot, m_szLandmarkName,sizeof(st_szNextSpot));
	// This object will get removed in the call to engine->ChangeLevel, copy the params into "safe" memory
	Q_strncpy(st_szNextMap, m_szMapName, sizeof(st_szNextMap));

	m_hActivator = pActivator;

	m_OnChangeLevel.FireOutput(pActivator, this);

	NotifyEntitiesOutOfTransition();


////	Msg( "Level touches %d levels\n", ChangeList( levels, 16 ) );
	if ( g_debug_transitions.GetInt() )
	{
		Msg( "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	}

	for( int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pOther = UTIL_PlayerByIndex( i );
		if ( pOther )
		{
			pOther->m_bTransition = true;
		}
	}

	Transitioned = true;

	// If we're debugging, don't actually change level
	if ( g_debug_transitions.GetInt() == 0 )
	{
		engine->ChangeLevel( st_szNextMap, st_szNextSpot );
	}
	else
	{
		SetTouch( NULL );
	}
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel::TouchChangeLevel( CBaseEntity *pOther )
{
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsInAVehicle() && pPlayer->GetMoveType() == MOVETYPE_NOCLIP )
	{
		DevMsg("In level transition: %s %s\n", st_szNextMap, st_szNextSpot );
		return;
	}

	ChangeLevelNow( pOther );
}


// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( levellist_t *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	// Ignore changelevels to the level we're ready in. Mapmakers love to do this!
	if ( stricmp( pMapName, STRING(gpGlobals->mapname) ) == 0 )
		return 0;

	return 0;
}

int BuildChangeList( levellist_t *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}

struct collidelist_t
{
	const CPhysCollide	*pCollide;
	Vector			origin;
	QAngle			angles;
};


// NOTE: This routine is relatively slow.  If you need to use it for per-frame work, consider that fact.
// UNDONE: Expand this to the full matrix of solid types on each side and move into enginetrace
bool TestEntityTriggerIntersection_Accurate( CBaseEntity *pTrigger, CBaseEntity *pEntity )
{
	Assert( pTrigger->GetSolid() == SOLID_BSP );

	if ( pTrigger->Intersects( pEntity ) )	// It touches one, it's in the volume
	{
		switch ( pEntity->GetSolid() )
		{
		case SOLID_BBOX:
			{
				ICollideable *pCollide = pTrigger->CollisionProp();
				Ray_t ray;
				trace_t tr;
				ray.Init( pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin(), pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs() );
				enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );

				if ( tr.startsolid )
					return true;
			}
			break;
		case SOLID_BSP:
		case SOLID_VPHYSICS:
			{
				CPhysCollide *pTriggerCollide = modelinfo->GetVCollide( pTrigger->GetModelIndex() )->solids[0];
				Assert( pTriggerCollide );

				CUtlVector<collidelist_t> collideList;
				IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
				int physicsCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
				if ( physicsCount )
				{
					for ( int i = 0; i < physicsCount; i++ )
					{
						const CPhysCollide *pCollide = pList[i]->GetCollide();
						if ( pCollide )
						{
							collidelist_t element;
							element.pCollide = pCollide;
							pList[i]->GetPosition( &element.origin, &element.angles );
							collideList.AddToTail( element );
						}
					}
				}
				else
				{
					vcollide_t *pVCollide = modelinfo->GetVCollide( pEntity->GetModelIndex() );
					if ( pVCollide && pVCollide->solidCount )
					{
						collidelist_t element;
						element.pCollide = pVCollide->solids[0];
						element.origin = pEntity->GetAbsOrigin();
						element.angles = pEntity->GetAbsAngles();
						collideList.AddToTail( element );
					}
				}
				for ( int i = collideList.Count()-1; i >= 0; --i )
				{
					const collidelist_t &element = collideList[i];
					trace_t tr;
					physcollision->TraceCollide( element.origin, element.origin, element.pCollide, element.angles, pTriggerCollide, pTrigger->GetAbsOrigin(), pTrigger->GetAbsAngles(), &tr );
					if ( tr.startsolid )
						return true;
				}
			}
			break;

		default:
			return true;
		}
	}
	return false;
}

int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, const char *pVolumeName )
{
	CBaseEntity *pVolume;

	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return TRANSITION_VOLUME_PASSED;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	pEntity = pEntity->GetRootMoveParent();

	int inVolume = TRANSITION_VOLUME_NOT_FOUND;	// Unless we find a trigger_transition, everything is in the volume

	pVolume = gEntList.FindEntityByName( NULL, pVolumeName );
	while ( pVolume )
	{
		if ( pVolume && FClassnameIs( pVolume, "trigger_transition" ) )
		{
			if ( TestEntityTriggerIntersection_Accurate(pVolume,  pEntity ) )	// It touches one, it's in the volume
				return TRANSITION_VOLUME_PASSED;

			inVolume = TRANSITION_VOLUME_SCREENED_OUT;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pVolume = gEntList.FindEntityByName( pVolume, pVolumeName );
	}
	return inVolume;
}


//------------------------------------------------------------------------------
// Builds the list of entities to save when moving across a transition
//------------------------------------------------------------------------------
int CChangeLevel::BuildChangeLevelList( levellist_t *pLevelList, int maxList )
{
	int nCount = 0;

	CBaseEntity *pentChangelevel = gEntList.FindEntityByClassname( NULL, "trigger_changelevel" );
	while ( pentChangelevel )
	{
		CChangeLevel *pTrigger = dynamic_cast<CChangeLevel *>(pentChangelevel);
		if ( pTrigger )
		{
			// Find the corresponding landmark
			CBaseEntity *pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, nCount, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark->edict() ) )
				{
					++nCount;
					if ( nCount >= maxList )		// FULL!!
						break;
				}
			}
		}
		pentChangelevel = gEntList.FindEntityByClassname( pentChangelevel, "trigger_changelevel" );
	}

	return nCount;
}


//------------------------------------------------------------------------------
// Adds a single entity to the transition list, if appropriate. Returns the new count
//------------------------------------------------------------------------------
inline int CChangeLevel::AddEntityToTransitionList( CBaseEntity *pEntity, int flags, int nCount, CBaseEntity **ppEntList, int *pEntityFlags )
{
	ppEntList[ nCount ] = pEntity;
	pEntityFlags[ nCount ] = flags;
	++nCount;

	// If we're debugging, make it visible
	if ( g_iDebuggingTransition )
	{
		if ( g_iDebuggingTransition == DEBUG_TRANSITIONS_VERBOSE )
		{
			// In verbose mode we've already printed out what the entity is
			Msg("ADDED.\n");
		}
		else
		{
			// In non-verbose mode, we just print this line
			Msg( "ADDED %s (%s) to transition.\n", pEntity->GetClassname(), pEntity->GetDebugName() );
		}

		pEntity->m_debugOverlays |= (OVERLAY_BBOX_BIT | OVERLAY_NAME_BIT);
	}

	return nCount;
}


//------------------------------------------------------------------------------
// Builds the list of entities to bring across a particular transition
//------------------------------------------------------------------------------
int CChangeLevel::BuildEntityTransitionList( CBaseEntity *pLandmarkEntity, const char *pLandmarkName,
	CBaseEntity **ppEntList, int *pEntityFlags, int nMaxList )
{
	int iEntity = 0;

	// Only show debug for the transition to the level we're going to
	if ( g_debug_transitions.GetInt() && pLandmarkEntity->NameMatches(st_szNextSpot) )
	{
		g_iDebuggingTransition = g_debug_transitions.GetInt();

		// Show us where the landmark entity is
		pLandmarkEntity->m_debugOverlays |= (OVERLAY_PIVOT_BIT | OVERLAY_BBOX_BIT | OVERLAY_NAME_BIT);
	}
	else
	{
		g_iDebuggingTransition = 0;
	}

	return 0;
}


//------------------------------------------------------------------------------
// Tests bits in a bitfield
//------------------------------------------------------------------------------
static inline bool IsBitSet( char *pBuf, int nBit )
{
	return (pBuf[ nBit >> 3 ] & ( 1 << (nBit & 0x7) )) != 0;
}

static inline void Set( char *pBuf, int nBit )
{
	pBuf[ nBit >> 3 ] |= 1 << (nBit & 0x7);
}


//------------------------------------------------------------------------------
// Adds in all entities depended on by entities near the transition
//------------------------------------------------------------------------------
#define MAX_ENTITY_BYTE_COUNT	(GAME_NUM_ENT_ENTRIES >> 3)
int CChangeLevel::AddDependentEntities( int nCount, CBaseEntity **ppEntList, int *pEntityFlags, int nMaxList )
{
	char pEntitiesSaved[MAX_ENTITY_BYTE_COUNT];
	memset( pEntitiesSaved, 0, MAX_ENTITY_BYTE_COUNT * sizeof(char) );
			  
	// Populate the initial bitfield
	int i;
	for ( i = 0; i < nCount; ++i )
	{
		// NOTE: Must use GetEntryIndex because we're saving non-networked entities
		int nEntIndex = ppEntList[i]->GetRefEHandle().GetEntryIndex();

		// We shouldn't already have this entity in the list!
		Assert( !IsBitSet( pEntitiesSaved, nEntIndex ) );

		// Mark the entity as being in the list
		Set( pEntitiesSaved, nEntIndex );
	}

	return 0;
}


//------------------------------------------------------------------------------
// This builds the list of all transitions on this level and which entities 
// are in their PVS's and can / should be moved across.
//------------------------------------------------------------------------------

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// FIXME: This has grown into a complicated beast. Can we make this more elegant?
int CChangeLevel::ChangeList( levellist_t *pLevelList, int maxList )
{
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: A trigger that pushes the player, NPCs, or objects.
//-----------------------------------------------------------------------------
class CTriggerPush : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerPush, CBaseTrigger );

	void Spawn( void );
	void Activate( void );
	void Touch( CBaseEntity *pOther );
	void Untouch( CBaseEntity *pOther );
	void DrawDebugGeometryOverlays();

	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetPushDir( inputdata_t &inputdata );

	void InputSetPushDirection( inputdata_t &inputdata );

	Vector m_vecPushDir;

	DECLARE_MAPENTITY();
	
	float m_flAlternateTicksFix; // Scale factor to apply to the push speed when running with alternate ticks
	float m_flPushSpeed;
};

BEGIN_MAPENTITY( CTriggerPush )
	DEFINE_KEYFIELD( m_vecPushDir, FIELD_VECTOR, "pushdir" ),
	DEFINE_KEYFIELD( m_flAlternateTicksFix, FIELD_FLOAT, "alternateticksfix" ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetPushDirection", InputSetPushDirection ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetPushDir", InputSetPushDir ),
END_MAPENTITY()

LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPush::Spawn()
{
	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle(m_vecPushDir.x, m_vecPushDir.y, m_vecPushDir.z);
	AngleVectors(angPushDir, &vecAbsDir);

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vecPushDir );

	BaseClass::Spawn();

	InitTrigger();

	if (m_flSpeed == 0)
	{
		m_flSpeed = 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerPush::InputSetSpeed( inputdata_t &inputdata )
{
	m_flSpeed = inputdata.value.Float();

	// Need to update push speed/alternative ticks
	Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerPush::InputSetPushDir( inputdata_t &inputdata )
{
	inputdata.value.Vector3D( m_vecPushDir );

	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle( m_vecPushDir.x, m_vecPushDir.y, m_vecPushDir.z );
	AngleVectors( angPushDir, &vecAbsDir );

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vecPushDir );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTriggerPush::Activate()
{
	// Fix problems with triggers pushing too hard under sv_alternateticks.
	// This is somewhat hacky, but it's simple and we're really close to shipping.
	if ( ( m_flAlternateTicksFix != 0 ) && IsSimulatingOnAlternateTicks() )
	{
		m_flPushSpeed = m_flSpeed * m_flAlternateTicksFix;
	}
	else
	{
		m_flPushSpeed = m_flSpeed;
	}
	
	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPush::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || (pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE ) )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	// FIXME: If something is hierarchically attached, should we try to push the parent?
	if (pOther->GetMoveParent())
		return;

	// Transform the push dir into global space
	Vector vecAbsDir;
	VectorRotate( m_vecPushDir, EntityToWorldTransform(), vecAbsDir );

	// Instant trigger, just transfer velocity and remove
	if (HasSpawnFlags(SF_TRIG_PUSH_ONCE))
	{
		pOther->ApplyAbsVelocityImpulse( m_flPushSpeed * vecAbsDir );

		if ( vecAbsDir.z > 0 )
		{
			pOther->SetGroundEntity( NULL );
		}
		UTIL_Remove( this );
		return;
	}

	switch( pOther->GetMoveType() )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
		break;

	case MOVETYPE_VPHYSICS:
		{
			const float DEFAULT_MASS = 100.0f;
			if ( HasSpawnFlags( SF_TRIGGER_PUSH_USE_MASS ) )
			{
				// New-style code, affects all physobjs and accounts for mass
				IPhysicsObject *ppPhysObjs[ VPHYSICS_MAX_OBJECT_LIST_COUNT ];
				int nNumPhysObjs = pOther->VPhysicsGetObjectList( ppPhysObjs, VPHYSICS_MAX_OBJECT_LIST_COUNT );
				for ( int i = 0; i < nNumPhysObjs; i++ )
				{
					Vector force = m_flPushSpeed * vecAbsDir * DEFAULT_MASS * gpGlobals->frametime;
					force *= ppPhysObjs[ i ]->GetMass() / DEFAULT_MASS;
					ppPhysObjs[ i ]->ApplyForceCenter( force );
				}
			}
			else
			{
				IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
				if ( pPhys )
				{
					// UNDONE: Assume the velocity is for a 100kg object, scale with mass
					pPhys->ApplyForceCenter( m_flPushSpeed * vecAbsDir * DEFAULT_MASS * gpGlobals->frametime );
					return;
				}
			}
		}
		break;

	default:
		{
			// HACK HACK  HL2 players on ladders will only be disengaged if the sf is set, otherwise no push occurs.
			if ( pOther->IsPlayer() && 
				 pOther->GetMoveType() == MOVETYPE_LADDER )
			{
				if ( !HasSpawnFlags(SF_TRIG_PUSH_AFFECT_PLAYER_ON_LADDER) )
				{
					// Ignore the push
					return;
				}
			}

			Vector vecPush = (m_flPushSpeed * vecAbsDir);
			if ( ( pOther->GetFlags() & FL_BASEVELOCITY ) && !lagcompensation->IsCurrentlyDoingLagCompensation() )
			{
				vecPush = vecPush + pOther->GetBaseVelocity();
			}
			if ( vecPush.z > 0 && (pOther->GetFlags() & FL_ONGROUND) )
			{
				pOther->SetGroundEntity( NULL );
				Vector origin = pOther->GetAbsOrigin();
				origin.z += 1.0f;
				pOther->SetAbsOrigin( origin );
			}

#ifdef HL1_DLL
			// Apply the z velocity as a force so it counteracts gravity properly
			Vector vecImpulse( 0, 0, vecPush.z * 0.025 );//magic hack number

			pOther->ApplyAbsVelocityImpulse( vecImpulse );

			// apply x, y as a base velocity so we travel at constant speed on conveyors
			vecPush.z = 0;
#endif			

			pOther->SetBaseVelocity( vecPush );
			pOther->AddFlag( FL_BASEVELOCITY );
		}
		break;
	}
}

void CTriggerPush::InputSetPushDirection( inputdata_t &inputdata )
{
	inputdata.value.Vector3D( m_vecPushDir );

	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle(m_vecPushDir.x, m_vecPushDir.y, m_vecPushDir.z);
	AngleVectors(angPushDir, &vecAbsDir);

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vecPushDir );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTriggerPush::DrawDebugGeometryOverlays()
{
	BaseClass::DrawDebugGeometryOverlays();
	
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{
		// Transform the push dir into global space
		Vector vecAbsDir;
		VectorRotate( m_vecPushDir, EntityToWorldTransform(), vecAbsDir );
		NDebugOverlay::VertArrow( WorldSpaceCenter(), WorldSpaceCenter() + vecAbsDir * 100, 10, 255, 0, 255, 0, true, 0.1 );
	}	
}

//-----------------------------------------------------------------------------
// Teleport trigger
//-----------------------------------------------------------------------------
const int SF_TELEPORT_PRESERVE_ANGLES = 0x20;	// Preserve angles even when a local landmark is not specified

class CTriggerTeleport : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerTeleport, CBaseTrigger );

	virtual void Spawn( void ) OVERRIDE;
	virtual void Touch( CBaseEntity *pOther ) OVERRIDE;

	string_t m_iLandmark;

	DECLARE_MAPENTITY();

	bool							m_bUseLandmarkAngles;
};

LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

BEGIN_MAPENTITY( CTriggerTeleport )

	DEFINE_KEYFIELD( m_iLandmark, FIELD_STRING, "landmark" ),
	DEFINE_KEYFIELD( m_bUseLandmarkAngles, FIELD_BOOLEAN, "UseLandmarkAngles" ),

END_MAPENTITY()

void CTriggerTeleport::Spawn( void )
{
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Teleports the entity that touched us to the location of our target,
//			setting the toucher's angles to our target's angles if they are a
//			player.
//
//			If a landmark was specified, the toucher is offset from the target
//			by their initial offset from the landmark and their angles are
//			left alone.
//
// Input  : pOther - The entity that touched us.
//-----------------------------------------------------------------------------
void CTriggerTeleport::Touch( CBaseEntity *pOther )
{
	CBaseEntity	*pentTarget = NULL;

	if (!PassesTriggerFilters(pOther))
	{
		return;
	}

	// The activator and caller are the same
	pentTarget = gEntList.FindEntityByName( pentTarget, m_target, NULL, pOther, pOther );
	if (!pentTarget)
	{
	    Warning("Teleport trigger '%s' cannot find destination named '%s'!\n", STRING(this->GetEntityName()), STRING(m_target) );
	   return;
	}
	
	//
	// If a landmark was specified, offset the player relative to the landmark.
	//
	CBaseEntity	*pentLandmark = NULL;
	Vector vecLandmarkOffset(0, 0, 0);
	if (m_iLandmark != NULL_STRING)
	{
		// The activator and caller are the same
		pentLandmark = gEntList.FindEntityByName(pentLandmark, m_iLandmark, NULL, pOther, pOther );
		if (pentLandmark)
		{
			vecLandmarkOffset = pOther->GetAbsOrigin() - pentLandmark->GetAbsOrigin();
		}
	}

	pOther->SetGroundEntity( NULL );

	// collect the teleporting object's angles, origin, and velocity
	QAngle qActivatorEyeAngles = pOther->GetAbsAngles();

	if ( pOther->IsPlayer() )
	{
		qActivatorEyeAngles = pOther->EyeAngles();
	}

	QAngle qNewActivatorEyeAngles = qActivatorEyeAngles;

	Vector vecActivatorOrigin = pOther->GetAbsOrigin();
	Vector vecNewActivatorOrigin = vecActivatorOrigin;
	
	Vector vecActivatorVelocity;
	pOther->GetVelocity( &vecActivatorVelocity );
	Vector vecNewActivatorVelocity = vecActivatorVelocity;

	Vector vecPentTargetOrigin = pentTarget->GetAbsOrigin();

	if ( pentLandmark )
	{
		// transform the activator origin, angles, and velocity into the world space of the destination landmark
		matrix3x4_t pTransformMatrix;
		matrix3x4_t pLocalLandmarkMatrix;

		matrix3x4_t pRemoteLandmarkMatrix = pentTarget->EntityToWorldTransform();

		MatrixInvert( pentLandmark->EntityToWorldTransform(), pLocalLandmarkMatrix );

		ConcatTransforms( pRemoteLandmarkMatrix, pLocalLandmarkMatrix, pTransformMatrix );

		if(!HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES))
			qNewActivatorEyeAngles = TransformAnglesToWorldSpace( qActivatorEyeAngles, pTransformMatrix );

		VectorTransform( vecActivatorOrigin, pTransformMatrix, vecNewActivatorOrigin );

		VectorRotate( vecActivatorVelocity, pTransformMatrix, vecNewActivatorVelocity );
	}
	else
	{
		// there isn't a local landmark specified so just set the origin to the origin of the destination landmark
		if ( pOther->IsPlayer())
		{
			// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
			vecPentTargetOrigin.z -= pOther->WorldAlignMins().z;
		}
		// apply the offset
		vecNewActivatorOrigin = vecPentTargetOrigin + vecLandmarkOffset;
	}


	// force the object to use the angles of the teleport destination entity
	if ( (m_bUseLandmarkAngles && !HasSpawnFlags(SF_TELEPORT_PRESERVE_ANGLES)) )
	{
		qNewActivatorEyeAngles = pentTarget->GetAbsAngles();
	}

	pOther->Teleport( &vecNewActivatorOrigin, &qNewActivatorEyeAngles, &vecNewActivatorVelocity );
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );


//-----------------------------------------------------------------------------
// Teleport Relative trigger
//-----------------------------------------------------------------------------
class CTriggerTeleportRelative : public CBaseTrigger
{
public:
	DECLARE_CLASS(CTriggerTeleportRelative, CBaseTrigger);

	virtual void Spawn( void ) OVERRIDE;
	virtual void Touch( CBaseEntity *pOther ) OVERRIDE;

	Vector m_TeleportOffset;

	DECLARE_MAPENTITY();
};

LINK_ENTITY_TO_CLASS( trigger_teleport_relative, CTriggerTeleportRelative );
BEGIN_MAPENTITY( CTriggerTeleportRelative )
	DEFINE_KEYFIELD( m_TeleportOffset, FIELD_VECTOR, "teleportoffset" )
END_MAPENTITY()


void CTriggerTeleportRelative::Spawn( void )
{
	InitTrigger();
}

void CTriggerTeleportRelative::Touch( CBaseEntity *pOther )
{
	if ( !PassesTriggerFilters(pOther) )
	{
		return;
	}

	const Vector finalPos = m_TeleportOffset + WorldSpaceCenter();
	const Vector *momentum = &vec3_origin;

	pOther->Teleport( &finalPos, NULL, momentum );
}


class CTriggerGravity : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerGravity, CBaseTrigger );

	void Spawn( void );
	void GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

void CTriggerGravity::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->SetGravity( GetGravity() );
}


// this is a really bad idea.
class CAI_ChangeTarget : public CBaseEntity
{
public:
	DECLARE_CLASS( CAI_ChangeTarget, CBaseEntity );

	// Input handlers.
	void InputActivate( inputdata_t &inputdata );

	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_MAPENTITY();

private:
	string_t	m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS( ai_changetarget, CAI_ChangeTarget );

BEGIN_MAPENTITY( CAI_ChangeTarget )

	DEFINE_KEYFIELD( m_iszNewTarget, FIELD_STRING, "m_iszNewTarget" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

END_MAPENTITY()


void CAI_ChangeTarget::InputActivate( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = NULL;

	while ((pTarget = gEntList.FindEntityByName( pTarget, m_target, NULL, inputdata.pActivator, inputdata.pCaller )) != NULL)
	{
		pTarget->m_target = m_iszNewTarget;
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer( );
		if (pNPC)
		{
			pNPC->SetGoalEnt( NULL );
		}
	}
}

const float CTriggerCamera::kflPosInterpTime = 2.0f;

LINK_ENTITY_TO_CLASS( point_viewcontrol, CTriggerCamera );

BEGIN_MAPENTITY( CTriggerCamera )

	DEFINE_KEYFIELD( m_iszTargetAttachment, FIELD_STRING, "targetattachment" ),

	DEFINE_KEYFIELD( m_bInterpolatePosition, FIELD_BOOLEAN, "interpolatepositiontoplayer" ),

	DEFINE_KEYFIELD( m_fov, FIELD_FLOAT, "fov" ),
	DEFINE_KEYFIELD( m_fovSpeed, FIELD_FLOAT, "fov_rate" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget", InputSetTarget),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetAttachment", InputSetTargetAttachment ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ReturnToEyes", InputReturnToEyes ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TeleportToView", InputTeleportToView ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTrackSpeed", InputSetTrackSpeed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPath", InputSetPath ),

	// Function Pointers
	DEFINE_OUTPUT( m_OnEndFollow, "OnEndFollow" ),
	DEFINE_OUTPUT( m_OnStartFollow, "OnStartFollow" ),
	DEFINE_KEYFIELD( m_flTrackSpeed, FIELD_FLOAT, "trackspeed" ),

	DEFINE_KEYFIELD( m_bDontSetPlayerView, FIELD_BOOLEAN, "DontSetPlayerView" ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFOV", InputSetFOV ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFOVRate", InputSetFOVRate ),


END_MAPENTITY()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerCamera::CTriggerCamera()
{
	m_fov = 90;
	m_fovSpeed = 1;
	m_flTrackSpeed = 40.0f;
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CTriggerCamera::UpdateOnRemove()
{
	if (m_state == USE_ON && HasSpawnFlags(SF_CAMERA_PLAYER_NEW_BEHAVIOR))
	{
		Disable();
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Spawn( void )
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NOCLIP );
	SetSolid( SOLID_NONE );								// Remove model & collisions
	SetRenderAlpha( 0 );								// The engine won't draw this model if this is set to 0 and blending is on
	SetRenderMode( kRenderTransTexture );

	m_state = USE_OFF;
	
	m_initialSpeed = m_flSpeed;

	if ( m_acceleration == 0 )
		m_acceleration = 500;

	if ( m_deceleration == 0 )
		m_deceleration = 500;

	DispatchUpdateTransmitState();
}

int CTriggerCamera::UpdateTransmitState()
{
	// always tranmit if currently used by a monitor
	if ( m_state == USE_ON )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}
}

void CTriggerCamera::StartCameraShot( const char *pszShotType, CBaseEntity *pSceneEntity, CBaseEntity *pActor1, CBaseEntity *pActor2, float duration )
{
	// called from SceneEntity in response to a CChoreoEvent::CAMERA sent from a VCD.
}

//------------------------------------------------------------------------------
// Purpose: Input handler to set FOV.
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetFOV( inputdata_t &inputdata )
{
	m_fov = inputdata.value.Float();

	if ( m_state == USE_ON && m_hPlayer )
	{
		((CBasePlayer*)m_hPlayer.Get())->SetFOV( this, m_fov, m_fovSpeed );
	}
}

//------------------------------------------------------------------------------
// Purpose: Input handler to set FOV rate.
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetFOVRate( inputdata_t &inputdata )
{ 
	m_fovSpeed = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerCamera::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "wait"))
	{
		m_flWait = atof(szValue);
	}
	else if (FStrEq(szKeyName, "moveto"))
	{
		m_sPath = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "acceleration"))
	{
		m_acceleration = atof( szValue );
	}
	else if (FStrEq(szKeyName, "deceleration"))
	{
		m_deceleration = atof( szValue );
	}
	else if (FStrEq(szKeyName, "trackspeed"))
	{
		m_targetSpeed = atof(szValue);
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetTargetAttachment( inputdata_t &inputdata )
{
	m_iszTargetAttachment = inputdata.value.StringID();
	m_iAttachmentIndex = 0;

	if (m_hTarget)
	{
		if ( m_iszTargetAttachment != NULL_STRING )
		{
			if ( !m_hTarget->GetBaseAnimating() )
			{
				Warning("%s tried to target an attachment (%s) on target %s, which has no model.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
			}
			else
			{
				m_iAttachmentIndex = m_hTarget->GetBaseAnimating()->LookupAttachment( STRING(m_iszTargetAttachment) );
				if ( m_iAttachmentIndex <= 0 )
				{
					Warning("%s could not find attachment %s on target %s.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetTrackSpeed( inputdata_t &inputdata )
{
	m_flTrackSpeed = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CTriggerCamera::InputEnable( inputdata_t &inputdata )
{ 
	m_hPlayer = inputdata.pActivator;
	Enable();
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CTriggerCamera::InputDisable( inputdata_t &inputdata )
{ 
	Disable();
}

//------------------------------------------------------------------------------
// Purpose: Set a new target for the camera to point at.
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetTarget(inputdata_t &inputdata)
{
	BaseClass::InputSetTarget( inputdata );

	if ( FStrEq(STRING(m_target), "!player") )
	{
		AddSpawnFlags( SF_CAMERA_PLAYER_TARGET );
		m_hTarget = m_hPlayer;
	}
	else
	{
		RemoveSpawnFlags( SF_CAMERA_PLAYER_TARGET );
		m_hTarget = GetNextTarget();
	}
}

//------------------------------------------------------------------------------
// Purpose: Return the camera view to the player's eyes.
//------------------------------------------------------------------------------
void CTriggerCamera::InputReturnToEyes(inputdata_t &inputdata)
{
	if (m_hPlayer)
	{
		CBasePlayer *pPlayer = (CBasePlayer*)m_hPlayer.Get();
		if (pPlayer)
		{
			SetAbsOrigin(m_hPlayer->EyePosition());
			SetAbsAngles(m_hPlayer->EyeAngles());
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Teleport the player to the current position of the camera.
//------------------------------------------------------------------------------
void CTriggerCamera::InputTeleportToView(inputdata_t &inputdata)
{
	if (m_hPlayer)
	{
		CBasePlayer *pPlayer = (CBasePlayer*)m_hPlayer.Get();
		CBaseEntity *pViewControl = pPlayer->GetViewEntity();

		if (pViewControl && pViewControl != pPlayer)
		{
			CTriggerCamera *pOtherCamera = dynamic_cast<CTriggerCamera *>(pViewControl);
			if (pOtherCamera)
			{
				static Vector AbsOrigin = pOtherCamera->GetAbsOrigin();
				if (AbsOrigin != vec3_invalid)
				{
					pPlayer->SetAbsOrigin(AbsOrigin);
				}
			}
		}
	}
	Disable();
}

//------------------------------------------------------------------------------
// Purpose: Have the camera start following a new path.
//------------------------------------------------------------------------------
void CTriggerCamera::InputSetPath(inputdata_t &inputdata)
{
	m_sPath = inputdata.value.StringID();
	m_pPath = gEntList.FindEntityGeneric(NULL, STRING(m_sPath), this, inputdata.pActivator);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Enable( void )
{
	m_state = USE_ON;

	if ( !m_hPlayer || !m_hPlayer->IsPlayer() )
	{
		m_hPlayer = UTIL_GetNearestPlayer( GetAbsOrigin() );
	}

	if ( !m_hPlayer )
	{
		DispatchUpdateTransmitState();
		return;
	}

	Assert( m_hPlayer->IsPlayer() );
	CBasePlayer *pPlayer = NULL;

	if ( m_hPlayer->IsPlayer() )
	{
		pPlayer = ((CBasePlayer*)m_hPlayer.Get());
	}
	else
	{
		Warning("CTriggerCamera could not find a player!\n");
		return;
	}

	// if the player was already under control of a similar trigger, disable the previous trigger.
	{
		CBaseEntity *pPrevViewControl = pPlayer->GetViewEntity();
		if (pPrevViewControl && pPrevViewControl != pPlayer)
		{
			CTriggerCamera *pOtherCamera = dynamic_cast<CTriggerCamera *>(pPrevViewControl);
			if ( pOtherCamera )
			{
				if ( pOtherCamera == this )
				{
					// what the hell do you think you are doing?
					Warning("Viewcontrol %s was enabled twice in a row!\n", GetDebugName());
					return;
				}
				else
				{
					pOtherCamera->Disable();
				}
			}
		}
	}


	m_nPlayerButtons = pPlayer->m_nButtons;

	
	// Make the player invulnerable while under control of the camera.  This will prevent situations where the player dies while under camera control but cannot restart their game due to disabled player inputs.
	m_nOldTakeDamage = m_hPlayer->m_takedamage;
	m_hPlayer->m_takedamage = DAMAGE_NO;
	
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_NOT_SOLID ) )
	{
		m_hPlayer->AddSolidFlags( FSOLID_NOT_SOLID );
	}
	
	m_flReturnTime = gpGlobals->curtime + m_flWait;
	m_flSpeed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	// this pertains to view angles, not translation.
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_SNAP_TO ) )
	{
		m_bSnapToGoal = true;
	}

	if ( HasSpawnFlags( SF_CAMERA_PLAYER_SETFOV ) )
	{
		if ( pPlayer )
		{
			if ( pPlayer->GetFOVOwner() && (FClassnameIs( pPlayer->GetFOVOwner(), "point_viewcontrol_multiplayer" ) || FClassnameIs( pPlayer->GetFOVOwner(), "point_viewcontrol" )) )
			{
				pPlayer->ClearZoomOwner();
			}
			pPlayer->SetFOV( this, m_fov, m_fovSpeed );
		}
	}

	if ( HasSpawnFlags(SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// If we don't have a target, ignore the attachment / etc
	if ( m_hTarget )
	{
		m_iAttachmentIndex = 0;
		if ( m_iszTargetAttachment != NULL_STRING )
		{
			if ( !m_hTarget->GetBaseAnimating() )
			{
				Warning("%s tried to target an attachment (%s) on target %s, which has no model.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
			}
			else
			{
				m_iAttachmentIndex = m_hTarget->GetBaseAnimating()->LookupAttachment( STRING(m_iszTargetAttachment) );
				if ( !m_iAttachmentIndex )
				{
					Warning("%s could not find attachment %s on target %s.\n", GetClassname(), STRING(m_iszTargetAttachment), STRING(m_hTarget->GetEntityName()) );
				}
			}
		}
	}

	if (HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		((CBasePlayer*)m_hPlayer.Get())->EnableControl(FALSE);
	}

	if ( m_sPath != NULL_STRING )
	{
		m_pPath = gEntList.FindEntityByName( NULL, m_sPath, NULL, m_hPlayer );
	}
	else
	{
		m_pPath = NULL;
	}

	m_flStopTime = gpGlobals->curtime;
	if ( m_pPath )
	{
		if ( m_pPath->m_flSpeed != 0 )
			m_targetSpeed = m_pPath->m_flSpeed;
		
		m_flStopTime += m_pPath->GetDelay();
		m_vecMoveDir = m_pPath->GetLocalOrigin() - GetLocalOrigin();
		m_moveDistance = VectorNormalize( m_vecMoveDir );
		m_flStopTime = gpGlobals->curtime + m_pPath->GetDelay();
	}
	else
	{
		m_moveDistance = 0;
	}


	// copy over player information. If we're interpolating from
	// the player position, do something more elaborate.
	if (m_bInterpolatePosition)
	{
		// initialize the values we'll spline between
		m_vStartPos = m_hPlayer->EyePosition();
		m_vEndPos = GetAbsOrigin();
		m_flInterpStartTime = gpGlobals->curtime;
		UTIL_SetOrigin( this, m_hPlayer->EyePosition() );
		SetLocalAngles( QAngle( m_hPlayer->GetLocalAngles().x, m_hPlayer->GetLocalAngles().y, 0 ) );

		SetAbsVelocity( vec3_origin );
	}
	else if (HasSpawnFlags(SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( this, m_hPlayer->EyePosition() );
		SetLocalAngles( QAngle( m_hPlayer->GetLocalAngles().x, m_hPlayer->GetLocalAngles().y, 0 ) );
		SetAbsVelocity( m_hPlayer->GetAbsVelocity() );
	}
	else
	{
		SetAbsVelocity( vec3_origin );
	}

	if (!m_bDontSetPlayerView)
	{
		pPlayer->SetViewEntity( this );

		// Hide the player's viewmodel
		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->AddEffects( EF_NODRAW );
		}
	}

	// Only track if we have a target
	if ( m_hTarget )
	{
		// follow the player down
		SetThink( &CTriggerCamera::FollowTarget );
		SetNextThink( gpGlobals->curtime );
	}
	else if (m_pPath && HasSpawnFlags(SF_CAMERA_PLAYER_NEW_BEHAVIOR))
	{
		// Move if we have a path
		SetThink( &CTriggerCamera::MoveThink );
		SetNextThink( gpGlobals->curtime );
	}

	m_OnStartFollow.FireOutput( pPlayer, this );

	m_vecLastPos = GetAbsOrigin();
	Move();

	DispatchUpdateTransmitState();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Disable( void )
{
	if ( m_hPlayer )
	{
		CBasePlayer *pBasePlayer = (CBasePlayer*)m_hPlayer.Get();

		if ( pBasePlayer->IsAlive() )
		{
			if ( HasSpawnFlags( SF_CAMERA_PLAYER_NOT_SOLID ) )
			{
				pBasePlayer->RemoveSolidFlags( FSOLID_NOT_SOLID );
			}

			if ( HasSpawnFlags( SF_CAMERA_PLAYER_TAKECONTROL ) )
			{
				pBasePlayer->EnableControl( TRUE );
			}

			if (!m_bDontSetPlayerView)
			{
				pBasePlayer->SetViewEntity( NULL );
				pBasePlayer->m_Local.m_bDrawViewmodel = true;
			}
		}

		if ( HasSpawnFlags( SF_CAMERA_PLAYER_SETFOV ) )
		{
			pBasePlayer->SetFOV( this, 0, m_fovSpeed );
		}

		//return the player to previous takedamage state
		m_hPlayer->m_takedamage = m_nOldTakeDamage;
	}

	m_state = USE_OFF;
	m_flReturnTime = gpGlobals->curtime;
	SetThink( NULL );

	m_OnEndFollow.FireOutput(this, this); // dvsents2: what is the best name for this output?
	SetLocalAngularVelocity( vec3_angle );

	DispatchUpdateTransmitState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	if ( m_state != USE_OFF )
	{
		Disable();
	}
	else
	{
		m_hPlayer = pActivator;
		Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if ( m_hTarget == NULL )
	{
		Disable();
		return;
	}

	if ( !HasSpawnFlags(SF_CAMERA_PLAYER_INFINITE_WAIT) && (!m_hTarget || m_flReturnTime < gpGlobals->curtime) )
	{
		Disable();
		return;
	}

	QAngle vecGoal;
	if ( m_iAttachmentIndex )
	{
		Vector vecOrigin;
		m_hTarget->GetBaseAnimating()->GetAttachment( m_iAttachmentIndex, vecOrigin );
		VectorAngles( vecOrigin - GetAbsOrigin(), vecGoal );
	}
	else
	{
		if ( m_hTarget )
		{
			VectorAngles( m_hTarget->GetAbsOrigin() - GetAbsOrigin(), vecGoal );
		}
		else
		{
			// Use the viewcontroller's angles
			vecGoal = GetAbsAngles();
		}
	}

	// Should we just snap to the goal angles?
	if ( m_bSnapToGoal ) 
	{
		SetAbsAngles( vecGoal );
		m_bSnapToGoal = false;
	}
	else
	{
		// UNDONE: Can't we just use UTIL_AngleDiff here?
		QAngle angles = GetLocalAngles();

		if (angles.y > 360)
			angles.y -= 360;

		if (angles.y < 0)
			angles.y += 360;

		SetLocalAngles( angles );

		float dx = vecGoal.x - GetLocalAngles().x;
		float dy = vecGoal.y - GetLocalAngles().y;

		if (dx < -180) 
			dx += 360;
		if (dx > 180) 
			dx = dx - 360;
		
		if (dy < -180) 
			dy += 360;
		if (dy > 180) 
			dy = dy - 360;

		QAngle vecAngVel;
		vecAngVel.Init( dx * m_flTrackSpeed * gpGlobals->frametime, dy * m_flTrackSpeed * gpGlobals->frametime, GetLocalAngularVelocity().z );
		SetLocalAngularVelocity(vecAngVel);
	}

	if (!HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL))	
	{
		SetAbsVelocity( GetAbsVelocity() * 0.8 );
		if (GetAbsVelocity().Length( ) < 10.0)
		{
			SetAbsVelocity( vec3_origin );
		}
	}

	SetNextThink( gpGlobals->curtime );

	Move();
}

void CTriggerCamera::MoveThink()
{
	Move();
	SetNextThink( gpGlobals->curtime );
}

void CTriggerCamera::Move()
{
	if ( HasSpawnFlags( SF_CAMERA_PLAYER_INTERRUPT ) )
	{
		if ( m_hPlayer )
		{
			CBasePlayer *pPlayer = ToBasePlayer( m_hPlayer );

			if ( pPlayer  )
			{
				uint64 buttonsChanged = m_nPlayerButtons ^ pPlayer->m_nButtons;

				if ( buttonsChanged && pPlayer->m_nButtons )
				{
					 Disable();
					 return;
				}

				m_nPlayerButtons = pPlayer->m_nButtons;
			}
		}
	}

	// In vanilla HL2, the camera is either on a path, or doesn't move. In episodic
	// we add the capacity for interpolation to the start point. 
	if (m_pPath)
	{
		// Subtract movement from the previous frame
		if (m_pPath->GetSpawnFlags() & SF_PATHCORNER_TELEPORT)
		{
			SetAbsOrigin(m_pPath->GetAbsOrigin());
			m_moveDistance = -1;  //Make sure we enter the conditional below and advance to the next corner.
		}
		else
		{	
			// Subtract movement from the previous frame
			Vector movDist(GetAbsOrigin() - m_vecLastPos);
			m_moveDistance -= VectorNormalize(movDist);
		}

		// Have we moved enough to reach the target?
		if ( m_moveDistance <= 0 )
		{
			variant_t emptyVariant;
			m_pPath->AcceptInput( "InPass", this, this, emptyVariant, 0 );
			// Time to go to the next target
			m_pPath = m_pPath->GetNextTarget();

			// Set up next corner
			if ( !m_pPath )
			{
				SetAbsVelocity( vec3_origin );
			}
			else 
			{
				if ( m_pPath->m_flSpeed != 0 )
					m_targetSpeed = m_pPath->m_flSpeed;

				m_vecMoveDir = m_pPath->GetLocalOrigin() - GetLocalOrigin();
				m_moveDistance = VectorNormalize( m_vecMoveDir );
				m_flStopTime = gpGlobals->curtime + m_pPath->GetDelay();
			}
		}

		if ( m_flStopTime > gpGlobals->curtime )
			m_flSpeed = UTIL_Approach( 0, m_flSpeed, m_deceleration * gpGlobals->frametime );
		else
			m_flSpeed = UTIL_Approach( m_targetSpeed, m_flSpeed, m_acceleration * gpGlobals->frametime );

		float fraction = 2 * gpGlobals->frametime;
		SetAbsVelocity( ((m_vecMoveDir * m_flSpeed) * fraction) + (GetAbsVelocity() * (1-fraction)) );
	}
	else if (m_bInterpolatePosition)
	{
		// get the interpolation parameter [0..1]
		float tt = (gpGlobals->curtime - m_flInterpStartTime) / kflPosInterpTime;
		if (tt >= 1.0f)
		{
			// we're there, we're done
			UTIL_SetOrigin( this, m_vEndPos );
			SetAbsVelocity( vec3_origin );

			m_bInterpolatePosition = false;
		}
		else
		{
			Assert(tt >= 0);

			Vector nextPos = ( (m_vEndPos - m_vStartPos) * SimpleSpline(tt) ) + m_vStartPos;
			// rather than stomping origin, set the velocity so that we get there in the proper time
			Vector desiredVel = (nextPos - GetAbsOrigin()) * (1.0f / gpGlobals->frametime);
			SetAbsVelocity( desiredVel );
		}
	}

	m_vecLastPos = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Measures the proximity to a specified entity of any entities within
//			the trigger, provided they are within a given radius of the specified
//			entity. The nearest entity distance is output as a number from [0 - 1].
//-----------------------------------------------------------------------------
class CTriggerProximity : public CBaseTrigger
{
public:

	DECLARE_CLASS( CTriggerProximity, CBaseTrigger );

	virtual void Spawn(void);
	virtual void Activate(void);
	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	void MeasureThink(void);

protected:

	EHANDLE m_hMeasureTarget;
	string_t m_iszMeasureTarget;		// The entity from which we measure proximities.
	float m_fRadius;			// The radius around the measure target that we measure within.
	int m_nTouchers;			// Number of entities touching us.

	// Outputs
	COutputFloat m_NearestEntityDistance;

	DECLARE_MAPENTITY();
};


BEGIN_MAPENTITY( CTriggerProximity )

	// Keys
	DEFINE_KEYFIELD(m_iszMeasureTarget, FIELD_STRING, "measuretarget"),

	DEFINE_KEYFIELD(m_fRadius, FIELD_FLOAT, "radius"),

	// Outputs
	DEFINE_OUTPUT(m_NearestEntityDistance, "NearestEntityDistance"),

END_MAPENTITY()



LINK_ENTITY_TO_CLASS(trigger_proximity, CTriggerProximity);
LINK_ENTITY_TO_CLASS(logic_proximity, CPointEntity);


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerProximity::Spawn(void)
{
	// Avoid divide by zero in MeasureThink!
	if (m_fRadius == 0)
	{
		m_fRadius = 32;
	}

	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//			Finds the reference point from which to measure.
//-----------------------------------------------------------------------------
void CTriggerProximity::Activate(void)
{
	BaseClass::Activate();
	m_hMeasureTarget = gEntList.FindEntityByName(NULL, m_iszMeasureTarget );

	//
	// Disable our Touch function if we were given a bad measure target.
	//
	if ((m_hMeasureTarget == NULL) || (m_hMeasureTarget->edict() == NULL))
	{
		Warning( "TriggerProximity - Missing measure target or measure target with no origin!\n");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers++;	

		SetThink( &CTriggerProximity::MeasureThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers--;

		if ( m_nTouchers == 0 )
		{
			SetThink( NULL );
			SetNextThink( TICK_NEVER_THINK );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function called every frame as long as we have entities touching
//			us that we care about. Finds the closest entity to the measure
//			target and outputs the distance as a normalized value from [0..1].
//-----------------------------------------------------------------------------
void CTriggerProximity::MeasureThink( void )
{
	if ( ( m_hMeasureTarget == NULL ) || ( m_hMeasureTarget->edict() == NULL ) )
	{
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );
		return;
	}

	//
	// Traverse our list of touchers and find the entity that is closest to the
	// measure target.
	//
	float fMinDistance = m_fRadius + 100;
	CBaseEntity *pNearestEntity = NULL;

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		touchlink_t *pLink = root->nextLink;
		while ( pLink != root )
		{
			CBaseEntity *pEntity = pLink->entityTouched;

			// If this is an entity that we care about, check its distance.
			if ( ( pEntity != NULL ) && PassesTriggerFilters( pEntity ) )
			{
				float flDistance = (pEntity->GetLocalOrigin() - m_hMeasureTarget->GetLocalOrigin()).Length();
				if (flDistance < fMinDistance)
				{
					fMinDistance = flDistance;
					pNearestEntity = pEntity;
				}
			}

			pLink = pLink->nextLink;
		}
	}

	// Update our output with the nearest entity distance, normalized to [0..1].
	if ( fMinDistance <= m_fRadius )
	{
		fMinDistance /= m_fRadius;
		if ( fMinDistance != m_NearestEntityDistance.Get() )
		{
			m_NearestEntityDistance.Set( fMinDistance, pNearestEntity, this );
		}
	}

	SetNextThink( gpGlobals->curtime );
}


// ##################################################################################
//	>> TriggerWind
//
//  Blows physics objects in the trigger
//
// ##################################################################################

#define MAX_WIND_CHANGE		5.0f

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
class CPhysicsWind : public IMotionEvent
{
public:
	simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
	{
		// If we have no windspeed, we're not doing anything
		if ( !m_flWindSpeed )
			return IMotionEvent::SIM_NOTHING;

		// Get a cosine modulated noise between 5 and 20 that is object specific
		int nNoiseMod = 5+(int)pObject%15; // 

		// Turn wind yaw direction into a vector and add noise
		QAngle vWindAngle = vec3_angle;	
		vWindAngle[1] = m_nWindYaw+(30*cos(nNoiseMod * gpGlobals->curtime + nNoiseMod));
		Vector vWind;
		AngleVectors(vWindAngle,&vWind);

		// Add lift with noise
		vWind.z = 1.1 + (1.0 * sin(nNoiseMod * gpGlobals->curtime + nNoiseMod));

		linear = 3*vWind*m_flWindSpeed;
		angular = vec3_origin;
		return IMotionEvent::SIM_GLOBAL_FORCE;	
	}

	int		m_nWindYaw;
	float	m_flWindSpeed;
};


extern modelindex_t g_sModelIndexSmoke;
extern float	GetFloorZ(const Vector &origin);
#define WIND_THINK_CONTEXT		"WindThinkContext"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerWind : public CBaseVPhysicsTrigger
{
	DECLARE_CLASS( CTriggerWind, CBaseVPhysicsTrigger );
public:
	DECLARE_MAPENTITY();

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	UpdateOnRemove();
	bool	CreateVPhysics();
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );
	void	WindThink( void );
	int		DrawDebugTextOverlays( void );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputSetSpeed( inputdata_t &inputdata );

private:
	int 	m_nSpeedBase;	// base line for how hard the wind blows
	int		m_nSpeedNoise;	// noise added to wind speed +/-
	int		m_nSpeedCurrent;// current wind speed
	int		m_nSpeedTarget;	// wind speed I'm approaching

	int		m_nDirBase;		// base line for direction the wind blows (yaw)
	int		m_nDirNoise;	// noise added to wind direction
	int		m_nDirCurrent;	// the current wind direction
	int		m_nDirTarget;	// wind direction I'm approaching

	int		m_nHoldBase;	// base line for how long to wait before changing wind
	int		m_nHoldNoise;	// noise added to how long to wait before changing wind

	bool	m_bSwitch;		// when does wind change

	IPhysicsMotionController*	m_pWindController;
	CPhysicsWind				m_WindCallback;

};

LINK_ENTITY_TO_CLASS( trigger_wind, CTriggerWind );

BEGIN_MAPENTITY( CTriggerWind )

	DEFINE_KEYFIELD( m_nSpeedNoise,	FIELD_INTEGER, "SpeedNoise"),
	DEFINE_KEYFIELD( m_nDirNoise,	FIELD_INTEGER, "DirectionNoise"),
	DEFINE_KEYFIELD( m_nHoldBase,	FIELD_INTEGER, "HoldTime"),
	DEFINE_KEYFIELD( m_nHoldNoise,	FIELD_INTEGER, "HoldNoise"),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSpeed", InputSetSpeed ),

END_MAPENTITY()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::Spawn( void )
{
	m_bSwitch = true;
	m_nDirBase = GetLocalAngles().y;

	BaseClass::Spawn();

	m_nSpeedCurrent = m_nSpeedBase;
	m_nDirCurrent = m_nDirBase;

	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime, WIND_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerWind::KeyValue( const char *szKeyName, const char *szValue )
{
	// Done here to avoid collision with CBaseEntity's speed key
	if ( FStrEq(szKeyName, "Speed") )
	{
		m_nSpeedBase = atoi( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CTriggerWind::CreateVPhysics()
{
	BaseClass::CreateVPhysics();

	m_pWindController = physenv->CreateMotionController( &m_WindCallback );
	return true;
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CTriggerWind::UpdateOnRemove()
{
	if ( m_pWindController )
	{
		physenv->DestroyMotionController( m_pWindController );
		m_pWindController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::StartTouch(CBaseEntity *pOther)
{
	if ( !PassesTriggerFilters(pOther) )
		return;
	if ( pOther->IsPlayer() )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys)
	{
		m_pWindController->AttachObject( pPhys, false );
		pPhys->Wake();
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::EndTouch(CBaseEntity *pOther)
{
	if ( !PassesTriggerFilters(pOther) )
		return;
	if ( pOther->IsPlayer() )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys && m_pWindController )
	{
		m_pWindController->DetachObject( pPhys );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputEnable( inputdata_t &inputdata )
{
	BaseClass::InputEnable( inputdata );
	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime + 0.1f, WIND_THINK_CONTEXT );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::WindThink( void )
{
	// By default...
	SetContextThink( &CTriggerWind::WindThink, gpGlobals->curtime + 0.1, WIND_THINK_CONTEXT );

	// Is it time to change the wind?
	if (m_bSwitch)
	{
		m_bSwitch = false;

		// Set new target direction and speed
		m_nSpeedTarget = m_nSpeedBase + random_valve->RandomInt( -m_nSpeedNoise, m_nSpeedNoise );
		m_nDirTarget = UTIL_AngleMod( m_nDirBase + random_valve->RandomInt(-m_nDirNoise, m_nDirNoise) );
	}
	else
	{
		bool bDone = true;
		// either ramp up, or sleep till change
		if (abs(m_nSpeedTarget - m_nSpeedCurrent) > MAX_WIND_CHANGE)
		{
			m_nSpeedCurrent += (m_nSpeedTarget > m_nSpeedCurrent) ? MAX_WIND_CHANGE : -MAX_WIND_CHANGE;
			bDone = false;
		}

		if (abs(m_nDirTarget - m_nDirCurrent) > MAX_WIND_CHANGE)
		{

			m_nDirCurrent = UTIL_ApproachAngle( m_nDirTarget, m_nDirCurrent, MAX_WIND_CHANGE );
			bDone = false;
		}
		
		if (bDone)
		{
			m_nSpeedCurrent = m_nSpeedTarget;
			SetContextThink( &CTriggerWind::WindThink, m_nHoldBase + random_valve->RandomFloat(-m_nHoldNoise,m_nHoldNoise), WIND_THINK_CONTEXT );
			m_bSwitch = true;
		}
	}

	// If we're starting to blow, where we weren't before, wake up all our objects
	if ( m_nSpeedCurrent )
	{
		m_pWindController->WakeObjects();
	}

	// store the wind data in the controller callback
	m_WindCallback.m_nWindYaw = m_nDirCurrent;
	if ( m_bDisabled )
	{
		m_WindCallback.m_flWindSpeed = 0;
	}
	else
	{
		m_WindCallback.m_flWindSpeed = m_nSpeedCurrent;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputSetSpeed( inputdata_t &inputdata )
{
	// Set new speed and mark to switch
	m_nSpeedBase = inputdata.value.Int();
	m_bSwitch = true;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerWind::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Dir: %i (%i)",m_nDirCurrent,m_nDirTarget);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Speed: %i (%i)",m_nSpeedCurrent,m_nSpeedTarget);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

// ##################################################################################
//	>> TriggerHierarchy 
//
//  Triggers when touching entities children that match a given filter
//
// ##################################################################################
class CTriggerHierarchy : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerHierarchy, CTriggerMultiple );
public:
	DECLARE_MAPENTITY();

	string_t					m_iChildFilterName;
	CHandle<class CBaseFilter>	m_hChildFilter;

	virtual void Activate( void );
	virtual bool PassesTriggerFilters( CBaseEntity *pOther );

	bool HasChildThatPassesChildFilter( CBaseEntity *pEnt );
};


LINK_ENTITY_TO_CLASS( trigger_hierarchy, CTriggerHierarchy );

BEGIN_MAPENTITY( CTriggerHierarchy )
	DEFINE_KEYFIELD( m_iChildFilterName,	FIELD_STRING,	"childfiltername" ),
END_MAPENTITY()


void CTriggerHierarchy::Activate( void )
{	
	BaseClass::Activate();

	if ( m_iChildFilterName != NULL_STRING )
	{
		m_hChildFilter = dynamic_cast<CBaseFilter *>( gEntList.FindEntityByName( NULL, m_iChildFilterName ) );
	}
}

bool CTriggerHierarchy::PassesTriggerFilters( CBaseEntity *pOther )
{
	bool bPass = BaseClass::PassesTriggerFilters( pOther );

	if ( !bPass )
		return false;

	bPass = HasChildThatPassesChildFilter( pOther );

	return bPass;
}

bool CTriggerHierarchy::HasChildThatPassesChildFilter( CBaseEntity *pEnt )
{
	CBaseFilter *pChildFilter = m_hChildFilter.Get();

	// Loop through all children of this ent and check them for passing
	for ( CBaseEntity *pChild = pEnt->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		if ( !pChildFilter || pChildFilter->PassesFilter( this, pChild ) )
		{
			// Found one!
			return true;
		}

		// This didn't pass, maybe its children will... recurse!
		if ( HasChildThatPassesChildFilter( pChild ) )
		{
			return true;
		}
	}

	// None of these children or their children pass
	return false;
}


// ##################################################################################
//	>> TriggerImpact
//
//  Blows physics objects in the trigger
//
// ##################################################################################
#define TRIGGERIMPACT_VIEWKICK_SCALE 0.1

class CTriggerImpact : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerImpact, CTriggerMultiple );
public:
	DECLARE_MAPENTITY();

	float	m_flMagnitude;
	float	m_flNoise;
	float	m_flViewkick;

	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );

	// Inputs
	void InputSetMagnitude( inputdata_t &inputdata );
	void InputImpact( inputdata_t &inputdata );

	// Outputs
	COutputVector	m_pOutputForce;		// Output force in case anyone else wants to use it

	// Debug
	int		DrawDebugTextOverlays(void);
};

LINK_ENTITY_TO_CLASS( trigger_impact, CTriggerImpact );

BEGIN_MAPENTITY( CTriggerImpact )

	DEFINE_KEYFIELD( m_flMagnitude,	FIELD_FLOAT, "Magnitude"),
	DEFINE_KEYFIELD( m_flNoise,		FIELD_FLOAT, "Noise"),
	DEFINE_KEYFIELD( m_flViewkick,	FIELD_FLOAT, "Viewkick"),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,  "Impact", InputImpact ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMagnitude", InputSetMagnitude ),

	// Outputs
	DEFINE_OUTPUT(m_pOutputForce, "ImpactForce"),

END_MAPENTITY()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::Spawn( void )
{	
	// Clamp date in case user made an error
	m_flNoise = clamp(m_flNoise,0.f,1.f);
	m_flViewkick = clamp(m_flViewkick,0.f,1.f);

	// Always start disabled
	m_bDisabled = true;
	BaseClass::Spawn();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputImpact( inputdata_t &inputdata )
{
	// Output the force vector in case anyone else wants to use it
	Vector vDir;
	AngleVectors( GetLocalAngles(),&vDir );
	m_pOutputForce.Set( m_flMagnitude * vDir, inputdata.pActivator, inputdata.pCaller);

	// Enable long enough to throw objects inside me
	Enable();
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink(&CTriggerImpact::Disable);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::StartTouch(CBaseEntity *pOther)
{
	//If the entity is valid and has physics, hit it
	if ( ( pOther != NULL  ) && ( pOther->VPhysicsGetObject() != NULL ) )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );
		vDir += RandomVector(-m_flNoise,m_flNoise);
		pOther->VPhysicsGetObject()->ApplyForceCenter( m_flMagnitude * vDir );
	}

	// If the player, so a view kick
	if (pOther->IsPlayer() && fabs(m_flMagnitude)>0 )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );

		float flPunch = -m_flViewkick*m_flMagnitude*TRIGGERIMPACT_VIEWKICK_SCALE;
		pOther->ViewPunch( QAngle( vDir.y * flPunch, 0, vDir.x * flPunch ) );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputSetMagnitude( inputdata_t &inputdata )
{
	m_flMagnitude = inputdata.value.Float();
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerImpact::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Magnitude: %3.2f",m_flMagnitude);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Disables auto movement on players that touch it
//-----------------------------------------------------------------------------

class CTriggerPlayerMovement : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerPlayerMovement, CBaseTrigger );
	DECLARE_SERVERCLASS();
public:

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );

};

IMPLEMENT_SERVERCLASS_ST( CTriggerPlayerMovement, DT_TriggerPlayerMovement )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( trigger_playermovement, CTriggerPlayerMovement );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPlayerMovement::Spawn( void )
{
	if( HasSpawnFlags( SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS ) )
	{
		// @Note (toml 01-07-04): fix up spawn flag collision coding error. Remove at some point once all maps fixed up please!
		DevMsg("*** trigger_playermovement using obsolete spawnflag. Remove and reset with new value for \"Disable auto player movement\"\n" );
		RemoveSpawnFlags(SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS);
		AddSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE);
	}
	BaseClass::Spawn();

	InitTrigger();

	// Bugbait 19571:  Make CTriggerPlayerMovement for AutoDuck available on client for client side prediction
	if ( HasSpawnFlags( SF_TRIGGER_AUTO_DUCK ) )
	{
		m_bClientSidePredicted = true;
		SetTransmitState( FL_EDICT_PVSCHECK );
	}
}


// UNDONE: This will not support a player touching more than one of these
// UNDONE: Do we care?  If so, ref count automovement in the player?
void CTriggerPlayerMovement::StartTouch( CBaseEntity *pOther )
{	
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_DUCK ) )
	{
		pPlayer->ForceButtons( IN_DUCK );
	}

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_WALK ) )
	{
		pPlayer->ForceButtons( IN_WALK );
	}

	if ( HasSpawnFlags( SF_TRIGGER_DISABLE_JUMP ) )
	{
		pPlayer->DisableButtons( IN_JUMP );
	}

	// UNDONE: Currently this is the only operation this trigger can do
	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = false;
	}
}

void CTriggerPlayerMovement::EndTouch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_DUCK ) )
	{
		pPlayer->UnforceButtons( IN_DUCK );
	}

	if ( HasSpawnFlags( SF_TRIGGER_AUTO_WALK ) )
	{
		pPlayer->UnforceButtons( IN_WALK );
	}

	if ( HasSpawnFlags( SF_TRIGGER_DISABLE_JUMP ) )
	{
		pPlayer->EnableButtons( IN_JUMP );
	}

	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = true;
	}
}

//------------------------------------------------------------------------------
// Base VPhysics trigger implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Save/load
//------------------------------------------------------------------------------
BEGIN_MAPENTITY( CBaseVPhysicsTrigger )
	DEFINE_KEYFIELD( m_bDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
END_MAPENTITY()

//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::Spawn()
{
	Precache();

	SetSolid( SOLID_VPHYSICS );	
	AddSolidFlags( FSOLID_NOT_SOLID );

	// NOTE: Don't make yourself FSOLID_TRIGGER here or you'll get game 
	// collisions AND vphysics collisions.  You don't want any game collisions
	// so just use FSOLID_NOT_SOLID

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		AddEffects( EF_NODRAW );
	}

	CreateVPhysics();
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CBaseVPhysicsTrigger::CreateVPhysics()
{
	IPhysicsObject *pPhysics;
	if ( !HasSpawnFlags( SF_VPHYSICS_MOTION_MOVEABLE ) )
	{
		pPhysics = VPhysicsInitStatic();
	}
	else
	{
		pPhysics = VPhysicsInitShadow( false, false );
	}

	pPhysics->BecomeTrigger();
	return true;
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::UpdateOnRemove()
{
	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->RemoveTrigger();
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Activate
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}

//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		m_bDisabled = false;
		if ( VPhysicsGetObject())
		{
			VPhysicsGetObject()->EnableCollisions( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		m_bDisabled = true;
		if ( VPhysicsGetObject())
		{
			VPhysicsGetObject()->EnableCollisions( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::StartTouch( CBaseEntity *pOther )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseVPhysicsTrigger::EndTouch( CBaseEntity *pOther )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseVPhysicsTrigger::PassesTriggerFilters( CBaseEntity *pOther )
{
	if ( !pOther->VPhysicsGetObject() )
		return false;

	// First test spawn flag filters
	// Copied and pasted code from CBaseTrigger::PassesTriggerFilters().
	if ( HasSpawnFlags(SF_TRIGGER_ALLOW_ALL) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_ITEMS) && pOther->GetMoveType() == MOVETYPE_FLYGRAVITY)
		||
		(	HasSpawnFlags(SF_TRIG_TOUCH_DEBRIS) && 
			(pOther->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
			pOther->GetCollisionGroup() == COLLISION_GROUP_DEBRIS_TRIGGER || 
			pOther->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS)
		)
		)
	{
		if ( pOther->GetFlags() & FL_NPC )
		{
			CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

			if ( HasSpawnFlags( SF_TRIGGER_ONLY_PLAYER_ALLY_NPCS ) )
			{
				if ( !pNPC || !pNPC->IsPlayerAlly() )
				{
					return false;
				}
			}

			if ( HasSpawnFlags( SF_TRIGGER_ONLY_NPCS_IN_VEHICLES ) )
			{
				if ( !pNPC || !pNPC->IsInAVehicle() )
					return false;
			}
		}

		bool bOtherIsPlayer = pOther->IsPlayer();

		if ( bOtherIsPlayer )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)pOther;
			if ( !pPlayer->IsAlive() )
				return false;

			if ( HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_IN_VEHICLES) )
			{
				if ( !pPlayer->IsInAVehicle() )
					return false;

				// Make sure we're also not exiting the vehicle at the moment
				IServerVehicle *pVehicleServer = pPlayer->GetVehicle();
				if ( pVehicleServer == NULL )
					return false;

				if ( pVehicleServer->IsPassengerExiting() )
					return false;
			}

			if ( HasSpawnFlags(SF_TRIGGER_ONLY_CLIENTS_OUT_OF_VEHICLES) )
			{
				if ( pPlayer->IsInAVehicle() )
					return false;
			}

			if ( HasSpawnFlags( SF_TRIGGER_DISALLOW_BOTS ) )
			{
				if ( pPlayer->IsFakeClient() )
					return false;
			}
		}

		CBaseFilter *pFilter = m_hFilter.Get();
		return (!pFilter) ? true : pFilter->PassesFilter( this, pOther );
	}
	return false;
}

//=====================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: VPhysics trigger that changes the motion of vphysics objects that touch it
//-----------------------------------------------------------------------------
class CTriggerVPhysicsMotion : public CBaseVPhysicsTrigger, public IMotionEvent
{
	DECLARE_CLASS( CTriggerVPhysicsMotion, CBaseVPhysicsTrigger );

public:
	void Spawn();
	void Precache();
	virtual void UpdateOnRemove();
	bool CreateVPhysics();

	// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
	// BUGBUG: If a player touches two of these, his movement will screw up.
	// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );

	void InputSetVelocityLimitTime( inputdata_t &inputdata );

	float LinearLimit();

	inline bool HasGravityScale() { return m_gravityScale != 1.0 ? true : false; }
	inline bool HasAirDensity() { return m_addAirDensity != 0 ? true : false; }
	inline bool HasLinearLimit() { return LinearLimit() != 0.0f; }
	inline bool HasLinearScale() { return m_linearScale != 1.0 ? true : false; }
	inline bool HasAngularLimit() { return m_angularLimit != 0 ? true : false; }
	inline bool HasAngularScale() { return m_angularScale != 1.0 ? true : false; }
	inline bool HasLinearForce() { return m_linearForce != 0.0 ? true : false; }

	DECLARE_MAPENTITY();

	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	IPhysicsMotionController	*m_pController;

	EntityParticleTrailInfo_t	m_ParticleTrail;

	float						m_gravityScale;
	float						m_addAirDensity;
	float						m_linearLimit;
	float						m_linearLimitDelta;
	float						m_linearLimitTime;
	float						m_linearLimitStart;
	float						m_linearLimitStartTime;
	float						m_linearScale;
	float						m_angularLimit;
	float						m_angularScale;
	float						m_linearForce;
	QAngle						m_linearForceAngles;
};


//------------------------------------------------------------------------------
// Save/load
//------------------------------------------------------------------------------
BEGIN_MAPENTITY( CTriggerVPhysicsMotion )

	DEFINE_INPUT( m_gravityScale, FIELD_FLOAT, "SetGravityScale" ),
	DEFINE_INPUT( m_addAirDensity, FIELD_FLOAT, "SetAdditionalAirDensity" ),
	DEFINE_INPUT( m_linearLimit, FIELD_FLOAT, "SetVelocityLimit" ),
	DEFINE_INPUT( m_linearLimitDelta, FIELD_FLOAT, "SetVelocityLimitDelta" ),

	DEFINE_INPUT( m_linearScale, FIELD_FLOAT, "SetVelocityScale" ),
	DEFINE_INPUT( m_angularLimit, FIELD_FLOAT, "SetAngVelocityLimit" ),
	DEFINE_INPUT( m_angularScale, FIELD_FLOAT, "SetAngVelocityScale" ),
	DEFINE_INPUT( m_linearForce, FIELD_FLOAT, "SetLinearForce" ),
	DEFINE_INPUT( m_linearForceAngles, FIELD_VECTOR, "SetLinearForceAngles" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetVelocityLimitTime", InputSetVelocityLimitTime ),
END_MAPENTITY()

LINK_ENTITY_TO_CLASS( trigger_vphysics_motion, CTriggerVPhysicsMotion );


//------------------------------------------------------------------------------
// Spawn
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::Spawn()
{
	Precache();

	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Precache
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::Precache()
{
	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		PrecacheMaterial( STRING(m_ParticleTrail.m_strMaterialName) ); 
	}
}

//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
float CTriggerVPhysicsMotion::LinearLimit()
{
	if ( m_linearLimitTime == 0.0f )
		return m_linearLimit;

	float dt = gpGlobals->curtime - m_linearLimitStartTime;
	if ( dt >= m_linearLimitTime )
	{
		m_linearLimitTime = 0.0;
		return m_linearLimit;
	}

	dt /= m_linearLimitTime;
	float flLimit = RemapVal( dt, 0.0f, 1.0f, m_linearLimitStart, m_linearLimit );
	return flLimit;
}

	
//------------------------------------------------------------------------------
// Create VPhysics
//------------------------------------------------------------------------------
bool CTriggerVPhysicsMotion::CreateVPhysics()
{
	m_pController = physenv->CreateMotionController( this );
	BaseClass::CreateVPhysics();

	return true;
}


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::UpdateOnRemove()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Start/End Touch
//------------------------------------------------------------------------------
// UNDONE: Pass trigger event in or change Start/EndTouch.  Add ITriggerVPhysics perhaps?
// BUGBUG: If a player touches two of these, his movement will screw up.
// BUGBUG: If a player uses crouch/uncrouch it will generate touch events and clear the motioncontroller flag
void CTriggerVPhysicsMotion::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( !PassesTriggerFilters(pOther) )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( pPlayer )
	{
		pPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, true );
		pPlayer->m_Local.m_bSlowMovement = true;
	}

	triggerevent_t event;
	PhysGetTriggerEvent( &event, this );
	if ( event.pObject )
	{
		// these all get done again on save/load, so check
		m_pController->AttachObject( event.pObject, true );
	}

	// Don't show these particles on the XBox

	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		CEntityParticleTrail::Create( pOther, m_ParticleTrail, this ); 
	}

	if ( pOther->GetBaseAnimating() && pOther->GetBaseAnimating()->IsRagdoll() )
	{
		CRagdollBoogie::IncrementSuppressionCount( pOther );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerVPhysicsMotion::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( !PassesTriggerFilters(pOther) )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );
	if ( pPlayer )
	{
		pPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, false );
		pPlayer->m_Local.m_bSlowMovement = false;
	}
	triggerevent_t event;
	PhysGetTriggerEvent( &event, this );
	if ( event.pObject && m_pController )
	{
		m_pController->DetachObject( event.pObject );
	}

	if ( m_ParticleTrail.m_strMaterialName != NULL_STRING )
	{
		CEntityParticleTrail::Destroy( pOther, m_ParticleTrail ); 
	}

	if ( pOther->GetBaseAnimating() && pOther->GetBaseAnimating()->IsRagdoll() )
	{
		CRagdollBoogie::DecrementSuppressionCount( pOther );
	}
}


//------------------------------------------------------------------------------
// Inputs
//------------------------------------------------------------------------------
void CTriggerVPhysicsMotion::InputSetVelocityLimitTime( inputdata_t &inputdata )
{
	m_linearLimitStart = LinearLimit();
	m_linearLimitStartTime = gpGlobals->curtime;

	float args[2];
	UTIL_StringToFloatArray( args, 2, inputdata.value.String() );
	m_linearLimit = args[0];
	m_linearLimitTime = args[1];
}

//------------------------------------------------------------------------------
// Apply the forces to the entity
//------------------------------------------------------------------------------
IMotionEvent::simresult_e CTriggerVPhysicsMotion::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( m_bDisabled )
		return SIM_NOTHING;

	linear.Init();
	angular.Init();

	if ( HasGravityScale() )
	{
		// assume object already has 1.0 gravities applied to it, so apply the additional amount
		linear.z -= (m_gravityScale-1) * GetCurrentGravity();
	}

	if ( HasLinearForce() )
	{
		Vector vecForceDir;
		AngleVectors( m_linearForceAngles, &vecForceDir );
		VectorMA( linear, m_linearForce, vecForceDir, linear );
	}

	if ( HasAirDensity() || HasLinearLimit() || HasLinearScale() || HasAngularLimit() || HasAngularScale() )
	{
		Vector vel;
		AngularImpulse angVel;
		pObject->GetVelocity( &vel, &angVel );
		vel += linear * deltaTime; // account for gravity scale

		Vector unitVel = vel;
		Vector unitAngVel = angVel;

		float speed = VectorNormalize( unitVel );
		float angSpeed = VectorNormalize( unitAngVel );

		float speedScale = 0.0;
		float angSpeedScale = 0.0;

		if ( HasAirDensity() )
		{
			float linearDrag = -0.5 * m_addAirDensity * pObject->CalculateLinearDrag( unitVel ) * deltaTime;
			if ( linearDrag < -1 )
			{
				linearDrag = -1;
			}
			speedScale += linearDrag / deltaTime;
			float angDrag = -0.5 * m_addAirDensity * pObject->CalculateAngularDrag( unitAngVel ) * deltaTime;
			if ( angDrag < -1 )
			{
				angDrag = -1;
			}
			angSpeedScale += angDrag / deltaTime;
		}

		if ( HasLinearLimit() && speed > m_linearLimit )
		{
			float flDeltaVel = (LinearLimit() - speed) / deltaTime;
			if ( m_linearLimitDelta != 0.0f )
			{
				float flMaxDeltaVel = -m_linearLimitDelta / deltaTime;
				if ( flDeltaVel < flMaxDeltaVel )
				{
					flDeltaVel = flMaxDeltaVel;
				}
			}
			VectorMA( linear, flDeltaVel, unitVel, linear );
		}
		if ( HasAngularLimit() && angSpeed > m_angularLimit )
		{
			angular += ((m_angularLimit - angSpeed)/deltaTime) * unitAngVel;
		}
		if ( HasLinearScale() )
		{
			speedScale = ( (speedScale+1) * m_linearScale ) - 1;
		}
		if ( HasAngularScale() )
		{
			angSpeedScale = ( (angSpeedScale+1) * m_angularScale ) - 1;
		}
		linear += vel * speedScale;
		angular += angVel * angSpeedScale;
	}

	return SIM_GLOBAL_ACCELERATION;
}

class CServerRagdollTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CServerRagdollTrigger, CBaseTrigger );

public:

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
	virtual void Spawn( void );

};

LINK_ENTITY_TO_CLASS( trigger_serverragdoll, CServerRagdollTrigger );

void CServerRagdollTrigger::Spawn( void )
{
	BaseClass::Spawn();

	// This didn't use PassesTriggerFilters() before, so a trigger_serverragdoll could work regardless of flags.
	// Because of this, using trigger filters now will break existing trigger_serverragdolls that functioned without the right flags ticked.
	// NPCs are going to be using this in almost all circumstances, so SF_TRIGGER_ALLOW_NPCS is pretty much the main flag of concern.
	AddSpawnFlags(SF_TRIGGER_ALLOW_NPCS);

	InitTrigger();
}

void CServerRagdollTrigger::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( pOther->IsPlayer() )
		return;

	// This means base class didn't accept it (trigger filters)
	if (m_hTouchingEntities.Find(pOther) == m_hTouchingEntities.InvalidIndex())
		return;

	CBaseCombatCharacter *pCombatChar = pOther->MyCombatCharacterPointer();

	if ( pCombatChar )
	{
		// The mapper or some other force might've changed it themselves.
		// Pretend it never touched us...
		if (pCombatChar->m_bForceServerRagdoll == true)
		{
			BaseClass::EndTouch(pOther);
			return;
		}

		pCombatChar->m_bForceServerRagdoll = true;
	}
}

void CServerRagdollTrigger::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( pOther->IsPlayer() )
		return;

	CBaseCombatCharacter *pCombatChar = pOther->MyCombatCharacterPointer();

	if ( pCombatChar )
	{
		pCombatChar->m_bForceServerRagdoll = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: A trigger that adds impulse to touching entities
//-----------------------------------------------------------------------------
class CTriggerApplyImpulse : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerApplyImpulse, CBaseTrigger );
	DECLARE_MAPENTITY();

	CTriggerApplyImpulse();

	void Spawn( void );

	void InputApplyImpulse( inputdata_t& );

private:
	Vector m_vecImpulseDir;
	float m_flForce;
};


BEGIN_MAPENTITY( CTriggerApplyImpulse )
	DEFINE_KEYFIELD( m_vecImpulseDir, FIELD_VECTOR, "impulse_dir" ),
	DEFINE_KEYFIELD( m_flForce, FIELD_FLOAT, "force" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ApplyImpulse", InputApplyImpulse ),
END_MAPENTITY()


LINK_ENTITY_TO_CLASS( trigger_apply_impulse, CTriggerApplyImpulse );


CTriggerApplyImpulse::CTriggerApplyImpulse()
{
	m_flForce = 300.f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerApplyImpulse::Spawn()
{
	// Convert pushdir from angles to a vector
	Vector vecAbsDir;
	QAngle angPushDir = QAngle(m_vecImpulseDir.x, m_vecImpulseDir.y, m_vecImpulseDir.z);
	AngleVectors(angPushDir, &vecAbsDir);

	// Transform the vector into entity space
	VectorIRotate( vecAbsDir, EntityToWorldTransform(), m_vecImpulseDir );

	BaseClass::Spawn();

	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerApplyImpulse::InputApplyImpulse( inputdata_t& )
{
	Vector vecImpulse = m_flForce * m_vecImpulseDir;
	FOR_EACH_VEC( m_hTouchingEntities, i )
	{
		if ( m_hTouchingEntities[i] )
		{
			m_hTouchingEntities[i]->ApplyAbsVelocityImpulse( vecImpulse );
		}
	}
}

class CTriggerFall : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerFall, CBaseTrigger );

public:

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
	virtual void Spawn( void );

	bool m_bStayLethal;

	DECLARE_MAPENTITY();
};

LINK_ENTITY_TO_CLASS( trigger_fall, CTriggerFall );

BEGIN_MAPENTITY( CTriggerFall )

	DEFINE_KEYFIELD( m_bStayLethal, FIELD_BOOLEAN, "StayLethal" ),

END_MAPENTITY()

void CTriggerFall::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

void CTriggerFall::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( !pOther->IsPlayer() )
		return;

	static_cast<CBasePlayer*>(pOther)->m_bInTriggerFall = true;
}

void CTriggerFall::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( !pOther->IsPlayer() || m_bStayLethal )
		return;

	static_cast<CBasePlayer*>(pOther)->m_bInTriggerFall = false;
}



class CTriggerWorld : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerWorld, CTriggerMultiple );

public:

	virtual bool PassesTriggerFilters(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS( trigger_world, CTriggerWorld );

bool CTriggerWorld::PassesTriggerFilters( CBaseEntity *pOther )
{
	return pOther->IsWorld();
}

//----------------------------------------------------------------------------------
// func_friction
//----------------------------------------------------------------------------------
class CFrictionModifier : public CBaseTrigger
{
	DECLARE_CLASS( CFrictionModifier, CBaseTrigger );

public:
	void		Spawn( void );
	bool		KeyValue( const char *szKeyName, const char *szValue );

	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	virtual int	ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	float		m_frictionFraction;

};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

// Modify an entity's friction
void CFrictionModifier::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
bool CFrictionModifier::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "modifier"))
	{
		m_frictionFraction = atof(szValue) / 100.0;
	}
	else
	{
		BaseClass::KeyValue( szKeyName, szValue );
	}
	return true;
}

void CFrictionModifier::StartTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )		// ignore player
	{
		pOther->SetFriction( m_frictionFraction );
	}
}

void CFrictionModifier::EndTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )		// ignore player
	{
		pOther->SetFriction( 1.0f );
	}
}


bool IsTriggerClass( CBaseEntity *pEntity )
{
	if ( NULL != dynamic_cast<CBaseTrigger *>(pEntity) )
		return true;

	if ( NULL != dynamic_cast<CTriggerVPhysicsMotion *>(pEntity) )
		return true;

	if ( NULL != dynamic_cast<CTriggerVolume *>(pEntity) )
		return true;
	
	return false;
}

//--------------------------------------------------------------------------------------------------------
// Allows players touching it to auto-crouch.
class CTriggerAutoCrouch : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerAutoCrouch, CBaseTrigger );

	void Spawn( void );

	virtual bool PassesTriggerFilters( CBaseEntity *pOther )
	{
		return pOther && pOther->IsPlayer();
	}

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
};


LINK_ENTITY_TO_CLASS( trigger_auto_crouch, CTriggerAutoCrouch );


//--------------------------------------------------------------------------------------------------------
void CTriggerAutoCrouch::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


//--------------------------------------------------------------------------------------------------------
void CTriggerAutoCrouch::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( IsTouching( pOther ) )
	{
		CBasePlayer *player = ToBasePlayer( pOther );
		if ( player )
		{
			// FIXMEL4DTOMAINMERGE
//			player->SetAutoCrouchEnabled( true );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CTriggerAutoCrouch::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	CBasePlayer *player = ToBasePlayer( pOther );
	if ( player )
	{
		// FIXMEL4DTOMAINMERGE
//		player->SetAutoCrouchEnabled( false );
	}
}


//----------------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------------
void CTriggerCallback::Spawn( void )
{
	// Setup our basic attributes
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_OBB );
	SetSolidFlags( FSOLID_NOT_SOLID|FSOLID_TRIGGER );

	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS|SF_TRIGGER_ALLOW_NPCS|SF_TRIGGER_ALLOW_PHYSICS );
}

//----------------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------------
void CTriggerCallback::StartTouch( CBaseEntity *pOther )
{
	// Don't touch things under certain circumstances
	if( pOther->VPhysicsGetObject() )
	{
		if( pOther->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			return;
	}

	if ( PassesTriggerFilters( pOther ) == false )
		return;

	Assert( m_pfnCallback );

	if ( GetParent() && m_pfnCallback )
	{
		(GetParent()->*m_pfnCallback)( pOther );
	}

	BaseClass::StartTouch( pOther );
}
