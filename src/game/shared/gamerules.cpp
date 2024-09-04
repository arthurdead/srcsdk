//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "ammodef.h"
#include "tier0/vprof.h"
#include "KeyValues.h"
#include "iachievementmgr.h"
#include "tier0/icommandline.h"
#include "mp_shareddefs.h"

#ifdef CLIENT_DLL

	#include "usermessages.h"

#else

	#include "player.h"
	#include "game.h"
	#include "entitylist.h"
	#include "basecombatweapon.h"
	#include "voice_gamemgr.h"
	#include "globalstate.h"
	#include "player_resource.h"
	#include "tactical_mission.h"
	#include "gamestats.h"
	#include "hltvdirector.h"
	#include "viewport_panel_names.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGameRules*	g_pGameRules = NULL;

ConVar g_Language( "g_Language", "0", FCVAR_REPLICATED );
ConVar sk_autoaim_mode( "sk_autoaim_mode", "1", FCVAR_ARCHIVE | FCVAR_REPLICATED );

ConVar mp_chattime(
		"mp_chattime", 
		"10", 
		FCVAR_REPLICATED,
		"amount of time players can chat after the game is over",
		true, 1,
		true, 120 );

#ifdef GAME_DLL
void MPTimeLimitCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( mp_timelimit.GetInt() < 0 )
	{
		mp_timelimit.SetValue( 0 );
	}

	if ( GameRules() )
	{
		GameRules()->HandleTimeLimitChange();
	}
}
#endif 

ConVar mp_timelimit( "mp_timelimit", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "game time per map in minutes"
#ifdef GAME_DLL
					, MPTimeLimitCallback 
#endif
					);

ConVar fraglimit( "mp_fraglimit","0", FCVAR_NOTIFY|FCVAR_REPLICATED, "The number of kills at which the map ends");

ConVar mp_show_voice_icons( "mp_show_voice_icons", "1", FCVAR_REPLICATED, "Show overhead player voice icons when players are speaking.\n" );

#ifdef GAME_DLL

ConVar tv_delaymapchange( "tv_delaymapchange", "0", FCVAR_NONE, "Delays map change until broadcast is complete" );
ConVar tv_delaymapchange_protect( "tv_delaymapchange_protect", "1", FCVAR_NONE, "Protect against doing a manual map change if HLTV is broadcasting and has not caught up with a major game event such as round_end" );

ConVar mp_restartgame( "mp_restartgame", "0", FCVAR_GAMEDLL, "If non-zero, game will restart in the specified number of seconds" );
ConVar mp_restartgame_immediate( "mp_restartgame_immediate", "0", FCVAR_GAMEDLL, "If non-zero, game will restart immediately" );

ConVar mp_mapcycle_empty_timeout_seconds( "mp_mapcycle_empty_timeout_seconds", "0", FCVAR_REPLICATED, "If nonzero, server will cycle to the next map if it has been empty on the current map for N seconds");

void cc_SkipNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( GameRules() )
	{
		GameRules()->SkipNextMapInCycle();
	}
}

void cc_GotoNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( GameRules() )
	{
		GameRules()->ChangeLevel();
	}
}

ConCommand skip_next_map( "skip_next_map", cc_SkipNextMapInCycle, "Skips the next map in the map rotation for the server." );
ConCommand changelevel_next( "changelevel_next", cc_GotoNextMapInCycle, "Immediately changes to the next map in the map rotation for the server." );

ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "0", FCVAR_GAMEDLL, "WaitingForPlayers time length in seconds" );

ConVar mp_waitingforplayers_restart( "mp_waitingforplayers_restart", "0", FCVAR_GAMEDLL, "Set to 1 to start or restart the WaitingForPlayers period." );
ConVar mp_waitingforplayers_cancel( "mp_waitingforplayers_cancel", "0", FCVAR_GAMEDLL, "Set to 1 to end the WaitingForPlayers period." );
ConVar mp_clan_readyrestart( "mp_clan_readyrestart", "0", FCVAR_GAMEDLL, "If non-zero, game will restart once someone from each team gives the ready signal" );
ConVar mp_clan_ready_signal( "mp_clan_ready_signal", "ready", FCVAR_GAMEDLL, "Text that team leader from each team must speak for the match to begin" );

ConVar nextlevel( "nextlevel", 
				  "", 
				  FCVAR_GAMEDLL | FCVAR_NOTIFY,
				  "If set to a valid map name, will trigger a changelevel to the specified map at the end of the round" );

// Hook into the convar from the engine
ConVar skill( "skill", "1" );

#endif

#ifdef GAME_DLL
#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;
extern bool	g_fGameOver;
#endif

#ifndef CLIENT_DLL
int CGameRules::m_nMapCycleTimeStamp = 0;
int CGameRules::m_nMapCycleindex = 0;
CUtlVector<char*> CGameRules::m_MapList;
#endif

#ifndef CLIENT_DLL
ConVar log_verbose_enable( "log_verbose_enable", "0", FCVAR_GAMEDLL, "Set to 1 to enable verbose server log on the server." );
ConVar log_verbose_interval( "log_verbose_interval", "3.0", FCVAR_GAMEDLL, "Determines the interval (in seconds) for the verbose server log." );
#endif // CLIENT_DLL

static CViewVectors g_DefaultViewVectors(
	Vector( 0, 0, 64 ),			//VEC_VIEW (m_vView)
								
	Vector(-16, -16, 0 ),		//VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),		//VEC_HULL_MAX (m_vHullMax)
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),		//VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),			//VEC_DUCK_VIEW		(m_vDuckView)
													
	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX	(m_vObsHullMax)
													
	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)
);													
													

// ------------------------------------------------------------------------------------ //
// CGameRulesProxy implementation.
// ------------------------------------------------------------------------------------ //

CGameRulesProxy *CGameRulesProxy::s_pGameRulesProxy = NULL;

IMPLEMENT_NETWORKCLASS_ALIASED( GameRulesProxy, DT_GameRulesProxy )

// Don't send any of the CBaseEntity stuff..
BEGIN_NETWORK_TABLE_NOBASE( CGameRulesProxy, DT_GameRulesProxy )
END_NETWORK_TABLE()


CGameRulesProxy::CGameRulesProxy()
{
	// allow map placed proxy entities to overwrite the static one
	if ( s_pGameRulesProxy )
	{
		UTIL_Remove( s_pGameRulesProxy );
		s_pGameRulesProxy = NULL;
	}
	s_pGameRulesProxy = this;
}

CGameRulesProxy::~CGameRulesProxy()
{
	if ( s_pGameRulesProxy == this )
	{
		s_pGameRulesProxy = NULL;
	}
}

int CGameRulesProxy::UpdateTransmitState()
{
#ifndef CLIENT_DLL
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
#else
	return 0;
#endif

}

void CGameRulesProxy::NotifyNetworkStateChanged()
{
	if ( s_pGameRulesProxy )
		s_pGameRulesProxy->NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CGameRules::Damage_GetShouldGibCorpse( void )
{
	int iDamage = ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CGameRules::Damage_GetNoPhysicsForce( void )
{
	int iTimeBasedDamage = Damage_GetTimeBased();
	int iDamage = ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CGameRules::Damage_GetShouldNotBleed( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::Damage_ShouldGibCorpse( int iDmgType )
{
	// Damage types that gib the corpse.
	return ( ( iDmgType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::Damage_NoPhysicsForce( int iDmgType )
{
	// Damage types that don't have to supply a physics force & position.
	int iTimeBasedDamage = Damage_GetTimeBased();
	return ( ( iDmgType & ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Damage types that don't make the player bleed.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID ) ) != 0 );
}

bool CGameRules::Init()
{
#ifdef GAME_DLL
	// Initialize the custom response rule dictionaries.
	InitCustomResponseRulesDicts();

	RefreshSkillData( true );

	LoadMapCycleFile();
#endif

	return BaseClass::Init();
}

CGameRules::CGameRules() : CAutoGameSystemPerFrame( "CGameRules" )
{
	Assert( !g_pGameRules );
	g_pGameRules = this;

#ifdef GAME_DLL
	GetVoiceGameMgr()->Init( g_pVoiceGameMgrHelper, gpGlobals->maxClients );
	ClearMultiDamage();

	m_flNextVerboseLogOutput = 0.0f;

	m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( engine->IsDedicatedServer() )
	{
		// dedicated server
		const char *cfgfile = servercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log( "Executing dedicated server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}
	else
	{
		// listen server
		const char *cfgfile = lservercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log( "Executing listen server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}

	nextlevel.SetValue( "" );
#endif

	LoadVoiceCommandScript();
}

#ifdef CLIENT_DLL

const char *CGameRules::GetVoiceCommandSubtitle( int iMenu, int iItem )
{
	Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return "";

	Assert( iItem >= 0 && iItem < m_VoiceCommandMenus.Element( iMenu ).Count() );
	if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
		return "";

	VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

	Assert( pItem );

	return pItem->m_szSubtitle;
}

// Returns false if no such menu is declared or if it's an empty menu
bool CGameRules::GetVoiceMenuLabels( int iMenu, KeyValues *pKV )
{
	Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return false;

	int iNumItems = m_VoiceCommandMenus.Element( iMenu ).Count();

	for ( int i=0; i<iNumItems; i++ )
	{
		VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( i );

		KeyValues *pLabelKV = new KeyValues( pItem->m_szMenuLabel );

		pKV->AddSubKey( pLabelKV );
	}

	return iNumItems > 0;
}

bool CGameRules::IsBonusChallengeTimeBased( void )
{
	return true;
}

bool CGameRules::IsLocalPlayer( int nEntIndex )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	return ( pLocalPlayer && pLocalPlayer == ClientEntityList().GetEnt( nEntIndex ) );
}

#else

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		// Get the max carrying capacity for this ammo
		int iMaxCarry = GetAmmoDef()->MaxCarry( iAmmoIndex, pPlayer );

		// Does the player have room for more of this type of ammo?
		if ( pPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName )
{
	return CanHaveAmmo( pPlayer, GetAmmoDef()->Index(szName) );
}

//=========================================================
//=========================================================
CBaseEntity *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
	Assert( pSpawnSpot );

	pPlayer->SetLocalOrigin( pSpawnSpot->GetAbsOrigin() + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

// checks if the spot is clear of players
bool CGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer  )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return false;
	}

	for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && ent != pPlayer )
			return false;
	}

	return true;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	if ( weaponstay.GetInt() > 0 )
	{
		if ( pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD )
			return BaseClass::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
		{
			if ( pPlayer->GetWeapon(i) == pWeapon )
			{
				return false;
			}
		}
	}

	if ( pWeapon->UsesPrimaryAmmo() )
	{
		if ( !CanHaveAmmo( pPlayer, pWeapon->m_iPrimaryAmmoType ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if ( pPlayer->Weapon_OwnsThisType( pWeapon->GetClassname() ) )
			{
				return FALSE;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if ( pPlayer->Weapon_OwnsThisType( pWeapon->GetClassname() ) )
		{
			return FALSE;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return TRUE;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ( bool forceUpdate )
{
	if ( !forceUpdate )
	{
		if ( GlobalEntity_IsInTable( "skill.cfg" ) )
			return;
	}
	GlobalEntity_Add( "skill.cfg", STRING(gpGlobals->mapname), GLOBAL_ON );

	char	szExec[256];

	ConVarRef skill( "skill" );

	ConVarRef suitcharger( "sk_suitcharger" );
	suitcharger.SetValue( 30 );

	SetSkillLevel( skill.IsValid() ? skill.GetInt() : 1 );

	Q_snprintf( szExec,sizeof(szExec), "exec skill%d.cfg\n", GetSkillLevel() );

	engine->ServerCommand( szExec );
	engine->ServerExecute();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsExplosionTraceBlocked( trace_t *ptr )
{
	if( ptr->DidHitWorld() )
		return true;

	if( ptr->m_pEnt == NULL )
		return false;

	if( ptr->m_pEnt->GetMoveType() == MOVETYPE_PUSH )
	{
		// All doors are push, but not all things that push are doors. This 
		// narrows the search before we start to do classname compares.
		if( FClassnameIs(ptr->m_pEnt, "prop_door_rotating") ||
        FClassnameIs(ptr->m_pEnt, "func_door") ||
        FClassnameIs(ptr->m_pEnt, "func_door_rotating") )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Default implementation of radius damage
//-----------------------------------------------------------------------------
#define ROBUST_RADIUS_PROBE_DIST 16.0f // If a solid surface blocks the explosion, this is how far to creep along the surface looking for another way to the target
void CGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;

	if( bInWater )
	{
		// Only muffle the explosion if deeper than 2 feet in water.
		if( !(UTIL_PointContents(vecSrc + Vector(0, 0, 24)) & MASK_WATER) )
		{
			bInWater = false;
		}
	}
	
	vecSrc.z += 1;// in case grenade is lying on the ground

	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// blast's don't tavel into or out of water
		if (bInWater && pEntity->GetWaterLevel() == 0)
			continue;

		if (!bInWater && pEntity->GetWaterLevel() == 3)
			continue;

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			if ( IsExplosionTraceBlocked(&tr) )
			{
				if( ShouldUseRobustRadiusDamage( pEntity ) )
				{
					if( vecSpot.DistToSqr( vecSrc ) > flHalfRadiusSqr )
					{
						// Only use robust model on a target within one-half of the explosion's radius.
						continue;
					}

					Vector vecToTarget = vecSpot - tr.endpos;
					VectorNormalize( vecToTarget );

					// We're going to deflect the blast along the surface that 
					// interrupted a trace from explosion to this target.
					Vector vecUp, vecDeflect;
					CrossProduct( vecToTarget, tr.plane.normal, vecUp );
					CrossProduct( tr.plane.normal, vecUp, vecDeflect );
					VectorNormalize( vecDeflect );

					// Trace along the surface that intercepted the blast...
					UTIL_TraceLine( tr.endpos, tr.endpos + vecDeflect * ROBUST_RADIUS_PROBE_DIST, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 255, 0, false, 10 );

					// ...to see if there's a nearby edge that the explosion would 'spill over' if the blast were fully simulated.
					UTIL_TraceLine( tr.endpos, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, false, 10 );

					if( tr.fraction != 1.0 && tr.DidHitWorld() )
					{
						// Still can't reach the target.
						continue;
					}
					// else fall through
				}
				else
				{
					continue;
				}
			}

			// UNDONE: Probably shouldn't let children block parents either?  Or maybe those guys should set their owner if they want this behavior?
			// HL2 - Dissolve damage is not reduced by interposing non-world objects
			if( tr.m_pEnt && tr.m_pEnt != pEntity && tr.m_pEnt->GetOwnerEntity() != pEntity )
			{
				// Some entity was hit by the trace, meaning the explosion does not have clear
				// line of sight to the entity that it's trying to hurt. If the world is also
				// blocking, we do no damage.
				CBaseEntity *pBlockingEntity = tr.m_pEnt;
				//Msg( "%s may be blocked by %s...", pEntity->GetClassname(), pBlockingEntity->GetClassname() );

				UTIL_TraceLine( vecSrc, vecSpot, CONTENTS_SOLID, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

				if( tr.fraction != 1.0 )
				{
					continue;
				}
				
				// Now, if the interposing object is physics, block some explosion force based on its mass.
				if( pBlockingEntity->VPhysicsGetObject() )
				{
					const float MASS_ABSORB_ALL_DAMAGE = 350.0f;
					float flMass = pBlockingEntity->VPhysicsGetObject()->GetMass();
					float scale = flMass / MASS_ABSORB_ALL_DAMAGE;

					// Absorbed all the damage.
					if( scale >= 1.0f )
					{
						continue;
					}

					ASSERT( scale > 0.0f );
					flBlockedDamagePercent = scale;
					//Msg("  Object (%s) weighing %fkg blocked %f percent of explosion damage\n", pBlockingEntity->GetClassname(), flMass, scale * 100.0f);
				}
				else
				{
					// Some object that's not the world and not physics. Generically block 25% damage
					flBlockedDamagePercent = 0.25f;
				}
			}
		}

		// decrease damage for an ent that's farther from the bomb.
		flAdjustedDamage = ( vecSrc - tr.endpos ).Length() * falloff;
		flAdjustedDamage = info.GetDamage() - flAdjustedDamage;

		if ( flAdjustedDamage <= 0 )
		{
			continue;
		}

		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

		// Now make a consideration for skill level!
		if( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
		{
			// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
			adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize( dir );

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
		{
			if ( !( adjustedInfo.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) )
			{
				CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
			}
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
			adjustedInfo.SetDamageForce( dir * flForce );
			adjustedInfo.SetDamagePosition( vecSrc );
		}

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );

		if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && ToBaseCombatCharacter( tr.m_pEnt ) )
		{
			// This is a total hack!!!
			bool bIsPrimary = true;
			CBasePlayer *player = ToBasePlayer( info.GetAttacker() );
			CBaseCombatWeapon *pWeapon = player->GetActiveWeapon();

			gamestats->Event_WeaponHit( player, bIsPrimary, (pWeapon != NULL) ? pWeapon->GetClassname() : "NULL", info );
		}
	}
}

bool CGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CBaseExpresserPlayer *pPlayer = ToBaseExpresserPlayer( pEdict );

	const char *pcmd = args[0];
	if ( FStrEq( pcmd, "voicemenu" ) )
	{
		if ( args.ArgC() < 3 )
			return true;

		int iMenu = atoi( args[1] );
		int iItem = atoi( args[2] );

		if(pPlayer)
			VoiceCommand( pPlayer, iMenu, iItem );

		return true;
	}

	if( pPlayer )
	{
		if( GetVoiceGameMgr()->ClientCommand( pPlayer, args ) )
			return true;
	}

	if(pPlayer->ClientCommand(args)) {
		return true;
	}

	return false;
}


void CGameRules::FrameUpdatePostEntityThink()
{
	VPROF( "CGameRules::FrameUpdatePostEntityThink" );
	Think();

	float flNow = Plat_FloatTime();

	// Update time when client was last connected
	if ( m_flTimeLastMapChangeOrPlayerWasConnected <= 0.0f )
	{
		m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
	}
	else
	{
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			player_info_t pi;
			if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
				continue;
#if defined( REPLAY_ENABLED )
			if ( pi.ishltv || pi.isreplay || pi.fakeplayer )
#else
			if ( pi.ishltv || pi.fakeplayer )
#endif
				continue;

			m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
			break;
		}
	}

	// Check if we should cycle the map because we've been empty
	// for long enough
	if ( mp_mapcycle_empty_timeout_seconds.GetInt() > 0 )
	{
		int iIdleSeconds = (int)( flNow - m_flTimeLastMapChangeOrPlayerWasConnected );
		if ( iIdleSeconds >= mp_mapcycle_empty_timeout_seconds.GetInt() )
		{

			Log( "Server has been empty for %d seconds on this map, cycling map as per mp_mapcycle_empty_timeout_seconds\n", iIdleSeconds );
			ChangeLevel();
		}
	}
}

void CGameRules::Think()
{
	GetVoiceGameMgr()->Update( gpGlobals->frametime );
	SetSkillLevel( skill.GetInt() );

	if ( log_verbose_enable.GetBool() )
	{
		if ( m_flNextVerboseLogOutput < gpGlobals->curtime )
		{
			ProcessVerboseLogOutput();
			m_flNextVerboseLogOutput = gpGlobals->curtime + log_verbose_interval.GetFloat();
		}
	}

	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		ChangeLevel(); // intermission is over
		return;
	}

	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	float flFragLimit = fraglimit.GetFloat();
	
	if ( flTimeLimit != 0 && gpGlobals->curtime >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		// check if any player is over the frag limit
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
	}
}

//=========================================================
//=========================================================
bool CGameRules::IsDeathmatch( void )
{
	return gpGlobals->deathmatch;
}

//=========================================================
//=========================================================
bool CGameRules::IsCoOp( void )
{
	return gpGlobals->coop;
}

//-----------------------------------------------------------------------------
// Purpose: Called at the end of GameFrame (i.e. after all game logic has run this frame)
//-----------------------------------------------------------------------------
void CGameRules::EndGameFrame( void )
{
	// If you hit this assert, it means something called AddMultiDamage() and didn't ApplyMultiDamage().
	// The g_MultiDamage.m_hAttacker & g_MultiDamage.m_hInflictor should give help you figure out the culprit.
	Assert( g_MultiDamage.IsClear() );
	if ( !g_MultiDamage.IsClear() )
	{
		Warning("Unapplied multidamage left in the system:\nTarget: %s\nInflictor: %s\nAttacker: %s\nDamage: %.2f\n", 
			g_MultiDamage.GetTarget()->GetDebugName(),
			g_MultiDamage.GetInflictor()->GetDebugName(),
			g_MultiDamage.GetAttacker()->GetDebugName(),
			g_MultiDamage.GetDamage() );
		ApplyMultiDamage();
	}
}

//-----------------------------------------------------------------------------
// trace line rules
//-----------------------------------------------------------------------------
float CGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
					 unsigned int mask, trace_t *ptr )
{
	UTIL_TraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
	return 1.0f;
}


void CGameRules::CreateStandardEntities()
{
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );
	g_pPlayerResource->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
}

//-----------------------------------------------------------------------------
// Purpose: Inform client(s) they can mark the indicated achievement as completed (SERVER VERSION)
// Input  : filter - which client(s) to send this to
//			iAchievementID - The enumeration value of the achievement to mark (see TODO:Kerry, what file will have the mod's achievement enum?) 
//-----------------------------------------------------------------------------
void CGameRules::MarkAchievement( IRecipientFilter& filter, char const *pchAchievementName )
{
	gamestats->Event_IncrementCountedStatistic( vec3_origin, pchAchievementName, 1.0f );

	IAchievementMgr *pAchievementMgr = engine->GetAchievementMgr();
	if ( !pAchievementMgr )
		return;
	pAchievementMgr->OnMapEvent( pchAchievementName );
}

bool CGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	if(pPlayer->GetActiveWeapon() && pPlayer->IsNetClient()) {
		const char *cl_autowepswitch = engine->GetClientConVarValue(engine->IndexOfEdict(pPlayer->edict()), "cl_autowepswitch");
		if(cl_autowepswitch && atoi(cl_autowepswitch) <= 0) {
			return false;
		}
	}

	if ( !pPlayer->Weapon_CanSwitchTo( pWeapon ) )
	{
		// Can't switch weapons for some reason.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon() )
	{
		// Player doesn't have an active item, might as well switch.
		return true;
	}

	if ( !pWeapon->AllowsAutoSwitchTo() )
	{
		// The given weapon should not be auto switched to from another weapon.
		return false;
	}

	if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
	{
		// The active weapon does not allow autoswitching away from it.
		return false;
	}

	if ( pWeapon->GetWeight() > pPlayer->GetActiveWeapon()->GetWeight() )
	{
		return true;
	}

	return false;
}

void CGameRules::ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			pPlayer->RemoveAllItems( true );// destroy all of the players weapons and items

			// Kill off view model entities
			pPlayer->DestroyViewModels();

			pPlayer->SetConnected( PlayerDisconnected );
		}
	}
}

float CGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.GetFloat();

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
}

bool CGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	GetVoiceGameMgr()->ClientConnected( pEntity );
	return true;
}

const char *CGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( pPlayer && pPlayer->IsAlive() == false )
	{
		if ( bTeamOnly )
			return "*DEAD*(TEAM)";
		else
			return "*DEAD*";
	}
	
	return "";
}

void CGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		char text[256];
		Q_snprintf( text,sizeof(text), "%s changed name to %s\n", pszOldName, pszName );

		UTIL_ClientPrintAll( HUD_PRINTTALK, text );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	if ( pszFov )
	{
		int iFov = atoi(pszFov);
		iFov = clamp( iFov, 75, 90 );
		pPlayer->SetDefaultFOV( iFov );
	}
}

CTacticalMissionManager *CGameRules::TacticalMissionManagerFactory( void )
{
	return new CTacticalMissionManager;
}

void CGameRules::SkipNextMapInCycle()
{
	char szSkippedMap[MAX_MAP_NAME];
	char szNextMap[MAX_MAP_NAME];

	GetNextLevelName( szSkippedMap, sizeof( szSkippedMap ) );
	IncrementMapCycleIndex();
	GetNextLevelName( szNextMap, sizeof( szNextMap ) );

	Msg( "Skipping: %s\tNext map: %s\n", szSkippedMap, szNextMap );

	if ( nextlevel.GetString() && *nextlevel.GetString() )
	{
		Msg( "Warning! \"nextlevel\" is set to \"%s\" and will override the next map to be played.\n", nextlevel.GetString() );
	}
}

void CGameRules::IncrementMapCycleIndex()
{
	// Reset index if we've passed the end of the map list
	if ( ++m_nMapCycleindex >= m_MapList.Count() )
	{
		m_nMapCycleindex = 0;
	}
}

void CGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	CBasePlayer *pPlayer = ToBasePlayer( CBaseEntity::Instance( pEntity ) );

	if ( !pPlayer )
		return;

	char const *pszCommand = pKeyValues->GetName();
	if ( pszCommand && pszCommand[0] )
	{
		if ( FStrEq( pszCommand, "AchievementEarned" ) )
		{
			if ( pPlayer->ShouldAnnounceAchievement() )
			{
				int nAchievementID = pKeyValues->GetInt( "achievementID" );

				IGameEvent * event = gameeventmanager->CreateEvent( "achievement_earned" );
				if ( event )
				{
					event->SetInt( "player", pPlayer->entindex() );
					event->SetInt( "achievement", nAchievementID );
					gameeventmanager->FireEvent( event );
				}

				pPlayer->OnAchievementEarned( nAchievementID );
			}
		}
	}
}

VoiceCommandMenuItem_t *CGameRules::VoiceCommand( CBaseExpresserPlayer *pPlayer, int iMenu, int iItem )
{
	// have the player speak the concept that is in a particular menu slot
	if ( !pPlayer )
		return NULL;

	if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
		return NULL;

	if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
		return NULL;

	VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

	Assert( pItem );

	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if ( pPlayer->CanSpeakVoiceCommand() )
	{
		CAI_Expresser *pExpresser = pPlayer->GetExpresser();
		Assert( pExpresser );
		pExpresser->AllowMultipleScenes();

		if ( pPlayer->SpeakConceptIfAllowed( pItem->m_iConcept, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
		{
			// show a subtitle if we need to
			if ( pItem->m_bShowSubtitle )
			{
				CRecipientFilter filter;

				if ( pItem->m_bDistanceBasedSubtitle )
				{
					filter.AddRecipientsByPAS( pPlayer->WorldSpaceCenter() );

					// further reduce the range to a certain radius
					int i;
					for ( i = filter.GetRecipientCount()-1; i >= 0; i-- )
					{
						int index = filter.GetRecipientIndex(i);

						CBasePlayer *pListener = UTIL_PlayerByIndex( index );

						if ( pListener && pListener != pPlayer )
						{
							float flDist = ( pListener->WorldSpaceCenter() - pPlayer->WorldSpaceCenter() ).Length2D();

							if ( flDist > VOICE_COMMAND_MAX_SUBTITLE_DIST )
								filter.RemoveRecipientByPlayerIndex( index );
						}
					}
				}
				else
				{
					filter.AddAllPlayers();
				}

				// if we aren't a disguised spy
				if ( !pPlayer->ShouldShowVoiceSubtitleToEnemy() )
				{
					// remove players on other teams
					filter.RemoveRecipientsNotOnTeam( pPlayer->GetTeam() );
				}

				// Register this event in the mod-specific usermessages .cpp file if you hit this assert
				Assert( usermessages->LookupUserMessage( "VoiceSubtitle" ) != -1 );

				// Send a subtitle to anyone in the PAS
				UserMessageBegin( filter, "VoiceSubtitle" );
					WRITE_BYTE( pPlayer->entindex() );
					WRITE_BYTE( iMenu );
					WRITE_BYTE( iItem );
				MessageEnd();
			}

			pPlayer->NoteSpokeVoiceCommand( szResponse );
		}
		else
		{
			pItem = NULL;
		}

		pExpresser->DisallowMultipleScenes();
		return pItem;
	}

	return NULL;
}

bool CGameRules::IsLoadingBugBaitReport()
{
	return ( !engine->IsDedicatedServer()&& CommandLine()->CheckParm( "-bugbait" ) && sv_cheats->GetBool() );
}

void CGameRules::HaveAllPlayersSpeakConceptIfAllowed( int iConcept, int iTeam /* = TEAM_UNASSIGNED */, const char *modifiers /* = NULL */ )
{
	CBaseExpresserPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBaseExpresserPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( iTeam != TEAM_UNASSIGNED )
		{
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		pPlayer->SpeakConceptIfAllowed( iConcept, modifiers );
	}
}

void CGameRules::RandomPlayersSpeakConceptIfAllowed( int iConcept, int iNumRandomPlayer /*= 1*/, int iTeam /*= TEAM_UNASSIGNED*/, const char *modifiers /*= NULL*/ )
{
	CUtlVector< CBaseExpresserPlayer* > speakCandidates;

	CBaseExpresserPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBaseExpresserPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( iTeam != TEAM_UNASSIGNED )
		{
			if ( pPlayer->GetTeamNumber() != iTeam )
				continue;
		}

		speakCandidates.AddToTail( pPlayer );
	}

	int iSpeaker = iNumRandomPlayer;
	while ( iSpeaker > 0 && speakCandidates.Count() > 0 )
	{
		int iRandomSpeaker = RandomInt( 0, speakCandidates.Count() - 1 );
		speakCandidates[ iRandomSpeaker ]->SpeakConceptIfAllowed( iConcept, modifiers );
		speakCandidates.FastRemove( iRandomSpeaker );
		iSpeaker--;
	}
}

void CGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	// sv_gravity
	KeyValues *pGravity = new KeyValues( "sv_gravity" );
	pGravity->SetString( "convar", "sv_gravity" );
	pGravity->SetString( "tag", "gravity" );

	pCvarTagList->AddSubKey( pGravity );

	// sv_alltalk
	KeyValues *pAllTalk = new KeyValues( "sv_alltalk" );
	pAllTalk->SetString( "convar", "sv_alltalk" );
	pAllTalk->SetString( "tag", "alltalk" );

	pCvarTagList->AddSubKey( pAllTalk );
}

void CGameRules::PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->m_nButtons = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

void CGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	bool		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	addDefault = true;

	while ( (pWeaponEntity = gEntList.FindEntityByClassname( pWeaponEntity, "game_player_equip" )) != NULL)
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = false;
	}
}

CBasePlayer *CGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor )
{
	if ( pKiller)
	{
		if ( pKiller->IsPlayer() )
			return (CBasePlayer*)pKiller;

		// Killing entity might be specifying a scorer player
		IScorer *pScorerInterface = dynamic_cast<IScorer*>( pKiller );
		if ( pScorerInterface )
		{
			CBasePlayer *pPlayer = pScorerInterface->GetScorer();
			if ( pPlayer )
				return pPlayer;
		}

		// Inflicting entity might be specifying a scoring player
		pScorerInterface = dynamic_cast<IScorer*>( pInflictor );
		if ( pScorerInterface )
		{
			CBasePlayer *pPlayer = pScorerInterface->GetScorer();
			if ( pPlayer )
				return pPlayer;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns player who should receive credit for kill
//-----------------------------------------------------------------------------
CBasePlayer *CGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	// if this method not overridden by subclass, just call our default implementation
	return GetDeathScorer( pKiller, pInflictor );
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	DeathNotice( pVictim, info );

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
	
	pVictim->IncrementDeathCount( 1 );

	// dvsents2: uncomment when removing all FireTargets
	// variant_t value;
	// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	// Did the player kill himself?
	if ( pVictim == pScorer )  
	{			
		if ( UseSuicidePenalty() )
		{
			// Players lose a frag for killing themselves
			pVictim->IncrementFragCount( -1 );
		}			
	}
	else if ( pScorer )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pScorer->IncrementFragCount( IPointsForKill( pScorer, pVictim ) );
		
		// Allow the scorer to immediately paint a decal
		pScorer->AllowImmediateDecalPainting();

		// dvsents2: uncomment when removing all FireTargets
		//variant_t value;
		//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
		FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
	}
	else
	{  
		if ( UseSuicidePenalty() )
		{
			// Players lose a frag for letting the world kill them			
			pVictim->IncrementFragCount( -1 );
		}					
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );

	// Custom damage type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetDeathNoticeName();
					}
				}
				else
				{
					killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = STRING( pInflictor->m_iClassname );
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( V_strnicmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( V_strnicmp( killer_weapon_name, "NPC_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( V_strnicmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
	if ( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetInt("customkill", info.GetDamageCustom() );
		event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
		event->SetString("weapon", killer_weapon_name );
		gameeventmanager->FireEvent( event );
	}

}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CGameRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->curtime + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->curtime + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CGameRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CGameRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
	return pWeapon->GetAbsOrigin();
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CGameRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

bool CGameRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
	return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
}

int CGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

bool CGameRules::PlayFootstepSounds( CBasePlayer *pl )
{
	if ( footsteps.GetInt() == 0 )
		return false;

	if ( pl->IsOnLadder() || pl->GetAbsVelocity().Length2D() > 220 )
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CGameRules::FAllowFlashlight( void ) 
{ 
	return flashlight.GetInt() != 0; 
}

//=========================================================
//=========================================================
bool CGameRules::FAllowNPCs( void )
{
	return ( allowNPCs.GetInt() != 0 );
}

//=========================================================
//======== CMultiplayRules private functions ===========

void CGameRules::GoToIntermission( void )
{
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	float flWaitTime = mp_chattime.GetInt();

	if ( tv_delaymapchange.GetBool() )
	{
		if ( HLTVDirector()->IsActive() )	
			flWaitTime = MAX( flWaitTime, HLTVDirector()->GetDelay() );
	}
			
	m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
	}
}

// Strip ' ' and '\n' characters from string.
static void StripWhitespaceChars( char *szBuffer )
{
	char *szOut = szBuffer;

	for ( char *szIn = szOut; *szIn; szIn++ )
	{
		if ( *szIn != ' ' && *szIn != '\r' )
			*szOut++ = *szIn;
	}
	*szOut = '\0';
}

void CGameRules::GetNextLevelName( char *pszNextMap, int bufsize, bool bRandom /* = false */ )
{
	char mapcfile[MAX_PATH];
	DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

	// Check the time of the mapcycle file and re-populate the list of level names if the file has been modified
	const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );

	if ( 0 == nMapCycleTimeStamp )
	{
		// Map cycle file does not exist, make a list containing only the current map
		char *szCurrentMapName = new char[MAX_MAP_NAME];
		Q_strncpy( szCurrentMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
		m_MapList.AddToTail( szCurrentMapName );
	}
	else
	{
		// If map cycle file has changed or this is the first time through ...
		if ( m_nMapCycleTimeStamp != nMapCycleTimeStamp )
		{
			// Reload
			LoadMapCycleFile();
		}
	}

	// If somehow we have no maps in the list then add the current one
	if ( 0 == m_MapList.Count() )
	{
		char *szDefaultMapName = new char[MAX_MAP_NAME];
		Q_strncpy( szDefaultMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
		m_MapList.AddToTail( szDefaultMapName );
	}

	if ( bRandom )
	{
		m_nMapCycleindex = RandomInt( 0, m_MapList.Count() - 1 );
	}

	// Here's the return value
	Q_strncpy( pszNextMap, m_MapList[m_nMapCycleindex], bufsize);
}

void CGameRules::DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew )
{
	static char szLastResult[ MAX_PATH ];

	const char *pszVar = mapcyclefile.GetString();
	if ( *pszVar == '\0' )
	{
		if ( bForceSpew || V_stricmp( szLastResult, "__novar") )
		{
			Msg( "mapcyclefile convar not set.\n" );
			V_strcpy_safe( szLastResult, "__novar" );
		}
		*pszResult = '\0';
		return;
	}

	char szRecommendedName[ MAX_PATH ];
	V_sprintf_safe( szRecommendedName, "cfg/%s", pszVar );

	// First, look for a mapcycle file in the cfg directory, which is preferred
	V_strncpy( pszResult, szRecommendedName, nSizeResult );
	if ( filesystem->FileExists( pszResult, "GAME" ) )
	{
		if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
		{
			Msg( "Using map cycle file '%s'.\n", pszResult );
			V_strcpy_safe( szLastResult, pszResult );
		}
		return;
	}

	// Nope?  Try the root.
	V_strncpy( pszResult, pszVar, nSizeResult );
	if ( filesystem->FileExists( pszResult, "GAME" ) )
	{
		if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
		{
			Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
			V_strcpy_safe( szLastResult, pszResult );
		}
		return;
	}

	// Nope?  Use the default.
	if ( !V_stricmp( pszVar, "mapcycle.txt" ) )
	{
		V_strncpy( pszResult, "cfg/mapcycle_default.txt", nSizeResult );
		if ( filesystem->FileExists( pszResult, "GAME" ) )
		{
			if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
			{
				Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
				V_strcpy_safe( szLastResult, pszResult );
			}
			return;
		}
	}

	// Failed
	*pszResult = '\0';
	if ( bForceSpew || V_stricmp( szLastResult, "__notfound") )
	{
		Msg( "Map cycle file '%s' was not found.\n", szRecommendedName );
		V_strcpy_safe( szLastResult, "__notfound" );
	}
}

void CGameRules::LoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
{
	CGameRules::RawLoadMapCycleFileIntoVector( pszMapCycleFile, mapList );
}

void CGameRules::RawLoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
{
	CUtlBuffer buf;
	if ( !filesystem->ReadFile( pszMapCycleFile, "GAME", buf ) )
		return;
	buf.PutChar( 0 );
	V_SplitString( (char*)buf.Base(), "\n", mapList );

	for ( int i = 0; i < mapList.Count(); i++ )
	{
		bool bIgnore = false;

		// Strip out ' ' and '\r' chars.
		StripWhitespaceChars( mapList[i] );

		if ( !Q_strncmp( mapList[i], "//", 2 ) || mapList[i][0] == '\0' )
		{
			bIgnore = true;
		}

		if ( bIgnore )
		{
			delete [] mapList[i];
			mapList.Remove( i );
			--i;
		}
	}
}

void CGameRules::FreeMapCycleFileVector( CUtlVector<char *> &mapList )
{
	// Clear out existing map list. Not using Purge() or PurgeAndDeleteAll() because they won't delete [] each element.
	for ( int i = 0; i < mapList.Count(); i++ )
	{
		delete [] mapList[i];
	}

	mapList.RemoveAll();
}

bool CGameRules::IsManualMapChangeOkay( const char **pszReason )
{
	if ( HLTVDirector()->IsActive() && ( HLTVDirector()->GetDelay() >= HLTV_MIN_DIRECTOR_DELAY ) )
	{
		if ( tv_delaymapchange.GetBool() && tv_delaymapchange_protect.GetBool() )
		{
			float flLastEvent = GetLastMajorEventTime();
			if ( flLastEvent > -1 )
			{
				if ( flLastEvent > ( gpGlobals->curtime - ( HLTVDirector()->GetDelay() + 3 ) ) ) // +3 second delay to prevent instant change after a major event
				{
					*pszReason = "\n***WARNING*** Map change blocked. HLTV is broadcasting and has not caught up to the last major game event yet.\nYou can disable this check by setting the value of the server convar \"tv_delaymapchange_protect\" to 0.\n";
					return false;
				}
			}
		}
	}

	return true;
}

bool CGameRules::IsMapInMapCycle( const char *pszName )
{
	for ( int i = 0; i < m_MapList.Count(); i++ )
	{
		if ( V_stricmp( pszName, m_MapList[i] ) == 0 )
		{
			return true;
		}
	}	

	return false;
}

void CGameRules::ChangeLevel( void )
{
	char szNextMap[MAX_MAP_NAME];

	if ( nextlevel.GetString() && *nextlevel.GetString() )
	{
		Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
	}
	else
	{
		GetNextLevelName( szNextMap, sizeof(szNextMap) );
		IncrementMapCycleIndex();
	}

	ChangeLevelToMap( szNextMap );
}

void CGameRules::LoadMapCycleFile( void )
{
	int nOldCycleIndex = m_nMapCycleindex;
	m_nMapCycleindex = 0;

	char mapcfile[MAX_PATH];
	DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

	FreeMapCycleFileVector( m_MapList );

	const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );
	m_nMapCycleTimeStamp = nMapCycleTimeStamp;

	// Repopulate map list from mapcycle file
	LoadMapCycleFileIntoVector( mapcfile, m_MapList );

	// Load server's mapcycle into network string table for client-side voting
	if ( g_pStringTableServerMapCycle )
	{
		CUtlString sFileList;
		for ( int i = 0; i < m_MapList.Count(); i++ )
		{
			sFileList += m_MapList[i];
			sFileList += '\n';
		}

		g_pStringTableServerMapCycle->AddString( true, "ServerMapCycle", sFileList.Length() + 1, sFileList.String() );
	}

	// If the current map is in the same location in the new map cycle, keep that index. This gives better behavior
	// when reloading a map cycle that has the current map in it multiple times.
	int nOldPreviousMap = ( nOldCycleIndex == 0 ) ? ( m_MapList.Count() - 1 ) : ( nOldCycleIndex - 1 );
	if ( nOldCycleIndex >= 0 && nOldCycleIndex < m_MapList.Count() &&
	     nOldPreviousMap >= 0 && nOldPreviousMap < m_MapList.Count() &&
	     V_strcmp( STRING( gpGlobals->mapname ), m_MapList[ nOldPreviousMap ] ) == 0 )
	{
		// The old index is still valid, and falls after our current map in the new cycle, use it
		m_nMapCycleindex = nOldCycleIndex;
	}
	else
	{
		// Otherwise, if the current map selection is in the list, set m_nMapCycleindex to the map that follows it.
		for ( int i = 0; i < m_MapList.Count(); i++ )
		{
			if ( V_strcmp( STRING( gpGlobals->mapname ), m_MapList[i] ) == 0 )
			{
				m_nMapCycleindex = i;
				IncrementMapCycleIndex();
				break;
			}
		}
	}
}

void CGameRules::ChangeLevelToMap( const char *pszMap )
{
	g_fGameOver = true;
	m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;
	Msg( "CHANGE LEVEL: %s\n", pszMap );
	engine->ChangeLevel( pszMap, NULL );
}

#endif // !CLIENT_DLL


void CGameRules::LoadVoiceCommandScript( void )
{
	KeyValues *pKV = new KeyValues( "VoiceCommands" );

	if ( pKV->LoadFromFile( filesystem, "scripts/voicecommands.txt", "GAME" ) )
	{
		for ( KeyValues *menu = pKV->GetFirstSubKey(); menu != NULL; menu = menu->GetNextKey() )
		{
			int iMenuIndex = m_VoiceCommandMenus.AddToTail();

			int iNumItems = 0;

			// for each subkey of this menu, add a menu item
			for ( KeyValues *menuitem = menu->GetFirstSubKey(); menuitem != NULL; menuitem = menuitem->GetNextKey() )
			{
				iNumItems++;

				if ( iNumItems > 9 )
				{
					Warning( "Trying to load more than 9 menu items in voicecommands.txt, extras ignored" );
					continue;
				}

				VoiceCommandMenuItem_t item;

#ifndef CLIENT_DLL
				int iConcept = GetMPConceptIndexFromString( menuitem->GetString( "concept", "" ) );
				if ( iConcept == MP_CONCEPT_NONE )
				{
					Warning( "Voicecommand script attempting to use unknown concept. Need to define new concepts in code. ( %s )\n", menuitem->GetString( "concept", "" ) );
				}
				item.m_iConcept = iConcept;

				item.m_bShowSubtitle = ( menuitem->GetInt( "show_subtitle", 0 ) > 0 );
				item.m_bDistanceBasedSubtitle = ( menuitem->GetInt( "distance_check_subtitle", 0 ) > 0 );

				Q_strncpy( item.m_szGestureActivity, menuitem->GetString( "activity", "" ), sizeof( item.m_szGestureActivity ) ); 
#else
				Q_strncpy( item.m_szSubtitle, menuitem->GetString( "subtitle", "" ), MAX_VOICE_COMMAND_SUBTITLE );
				Q_strncpy( item.m_szMenuLabel, menuitem->GetString( "menu_label", "" ), MAX_VOICE_COMMAND_SUBTITLE );

#endif
				m_VoiceCommandMenus.Element( iMenuIndex ).AddToTail( item );
			}
		}
	}

	pKV->deleteThis();
}

// ----------------------------------------------------------------------------- //
// Shared CGameRules implementation.
// ----------------------------------------------------------------------------- //

CGameRules::~CGameRules()
{
	Assert( g_pGameRules == this );
	g_pGameRules = NULL;
}

bool CGameRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	CBaseCombatWeapon *pWeapon = GetNextBestWeapon( pPlayer, pCurrentWeapon );

	if ( pWeapon != NULL )
		return pPlayer->Weapon_Switch( pWeapon );
	
	return false;
}

CBaseCombatWeapon *CGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	CBaseCombatWeapon *pCheck;
	CBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

	int iCurrentWeight = -1;
	int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	// If I have a weapon, make sure I'm allowed to holster it
	if ( pCurrentWeapon )
	{
		if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
		{
			// Either this weapon doesn't allow autoswitching away from it or I
			// can't put this weapon away right now, so I can't switch.
			return NULL;
		}

		iCurrentWeight = pCurrentWeapon->GetWeight();
	}

	for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
	{
		pCheck = pPlayer->GetWeapon( i );
		if ( !pCheck )
			continue;

		// If we have an active weapon and this weapon doesn't allow autoswitching away
		// from another weapon, skip it.
		if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
			continue;

		if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
		{
			// this weapon is from the same category. 
			if ( pCheck->HasAnyAmmo() )
			{
				if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
				{
					return pCheck;
				}
			}
		}
		else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
		{
			//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
			// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
			// that the player was using. This will end up leaving the player with his heaviest-weighted 
			// weapon. 
			if ( pCheck->HasAnyAmmo() )
			{
				// if this weapon is useable, flag it as the best
				iBestWeight = pCheck->GetWeight();
				pBest = pCheck;
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	return pBest;
}

bool CGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		::V_swap(collisionGroup0,collisionGroup1);
	}

	if(collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) {
		collisionGroup0 = COLLISION_GROUP_PLAYER;
	}

	if(collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT) {
		collisionGroup1 = COLLISION_GROUP_PLAYER;
	}

	if(collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER) {
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	if((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) && collisionGroup1 == COLLISION_GROUP_WEAPON) {
		return false;
	}

	if(collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER) {
		return false;
	}

	if(collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED) {
		return false;
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		// let debris and multiplayer objects collide
		return true;
	}
	
	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------

	// Don't bother if either is in a vehicle...
	if (( collisionGroup0 == COLLISION_GROUP_IN_VEHICLE ) || ( collisionGroup1 == COLLISION_GROUP_IN_VEHICLE ))
		return false;

	if ( ( collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER ) && ( collisionGroup0 != COLLISION_GROUP_NPC ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && ( collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR ) )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER )
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}

	// Dissolving guys only collide with COLLISION_GROUP_NONE
	if ( (collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING) )
	{
		if ( collisionGroup0 != COLLISION_GROUP_NONE )
			return false;
	}

	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	// This change was breaking HL2DM
	// Adrian: TEST! Interactive Debris doesn't collide with the player.
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup1 == COLLISION_GROUP_PLAYER ) )
		 return false;

	if ( collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS )
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if ( collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE )
		return false;

	// Projectiles hit everything but debris, weapons, + other projectiles
	if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE )
		{
			return false;
		}
	}

	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE || 
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	// collision with vehicle clip entity??
	if ( collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP )
	{
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE )
			return true;
		// vehicle clip against non-vehicle, no collision
		return false;
	}

	return true;
}


const CViewVectors* CGameRules::GetViewVectors() const
{
	return &g_DefaultViewVectors;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how much damage the given ammo type should do to the victim
//			when fired by the attacker.
// Input  : pAttacker - Dude what shot the gun.
//			pVictim - Dude what done got shot.
//			nAmmoType - What been shot out.
// Output : How much hurt to put on dude what done got shot (pVictim).
//-----------------------------------------------------------------------------
float CGameRules::GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType )
{
	float flDamage = 0;
	CAmmoDef *pAmmoDef = GetAmmoDef();

	if ( pAttacker && pAttacker->IsPlayer() )
	{
		flDamage = pAmmoDef->PlrDamage( nAmmoType );
	}
	else
	{
		flDamage = pAmmoDef->NPCDamage( nAmmoType );
	}

	return flDamage;
}
