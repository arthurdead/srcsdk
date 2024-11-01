//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "usercmd.h"
#include "igamesystem.h"
#include "ilagcompensationmanager.h"
#include "inetchannelinfo.h"
#include "utllinkedlist.h"
#include "BaseAnimatingOverlay.h"
#include "player_lagcompensation.h"
#include "ai_basenpc.h"
#include "collisionproperty.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LC_NONE				0
#define LC_ALIVE			(1<<0)

#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_ANGLES_CHANGED	(1<<9)
#define LC_SIZE_CHANGED		(1<<10)
#define LC_ANIMATION_CHANGED (1<<11)

static ConVar sv_lagcompensation_teleport_dist( "sv_lagcompensation_teleport_dist", "64", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "How far a player got moved by game code before we can't lag compensate their position back" );
#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
// Allow 4 units of error ( about 1 / 8 bbox width )
#define LAG_COMPENSATION_ERROR_EPS_SQR ( 4.0f * 4.0f )

ConVar sv_unlag( "sv_unlag", "1", FCVAR_DEVELOPMENTONLY, "Enables player lag compensation" );
ConVar sv_maxunlag( "sv_maxunlag", "1.0", FCVAR_DEVELOPMENTONLY, "Maximum lag compensation in seconds", true, 0.0f, true, 1.0f );
ConVar sv_lagflushbonecache( "sv_lagflushbonecache", "1", FCVAR_DEVELOPMENTONLY, "Flushes entity bone cache on lag compensation" );
ConVar sv_showlagcompensation( "sv_showlagcompensation", "0", FCVAR_CHEAT, "Show lag compensated hitboxes whenever a player is lag compensated." );
ConVar sv_lagcompensationforcerestore( "sv_lagcompensationforcerestore", "1", FCVAR_CHEAT, "Don't test validity of a lag comp restore, just do it.");

ConVar sv_unlag_fixstuck( "sv_unlag_fixstuck", "0", FCVAR_DEVELOPMENTONLY, "Disallow backtracking a player for lag compensation if it will cause them to become stuck" );

ConVar sv_lagpushticks( "sv_lagpushticks", "0", FCVAR_DEVELOPMENTONLY, "Push computed lag compensation amount by this many ticks." );

//
// Try to take the player from his current origin to vWantedPos.
// If it can't get there, leave the player where he is.
// 

ConVar sv_unlag_debug( "sv_unlag_debug", "0", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY );

float g_flFractionScale = 0.95;

static void RestoreEntityTo( CBaseEntity *pEntity, const Vector &vWantedPos )
{
	// Try to move to the wanted position from our current position.
	trace_t tr;
	VPROF_BUDGET( "RestorePlayerTo", "CLagCompensationManager" );
	if( sv_lagcompensationforcerestore.GetBool() )
	{
		// We don't have to test for validity.  Just put them back where you found them.
		UTIL_SetOrigin( pEntity, vWantedPos, true );
	}

	unsigned int mask = UTIL_MaskForEntity( pEntity );
	unsigned int collisiongroup = UTIL_CollisionGroupForEntity( pEntity );

	UTIL_TraceEntity( pEntity, vWantedPos, vWantedPos, mask, pEntity, collisiongroup, &tr );
	if ( tr.startsolid || tr.allsolid )
	{
		if ( sv_unlag_debug.GetBool() )
		{
			DevMsg( "RestorePlayerTo() could not restore position for entity \"%s\" ( %.1f %.1f %.1f )\n",
					pEntity->GetDebugName(), vWantedPos.x, vWantedPos.y, vWantedPos.z );
		}

		UTIL_TraceEntity( pEntity, pEntity->GetLocalOrigin(), vWantedPos, mask, pEntity, collisiongroup, &tr );
		if ( tr.startsolid || tr.allsolid )
		{
			// In this case, the guy got stuck back wherever we lag compensated him to. Nasty.

			if ( sv_unlag_debug.GetBool() )
				DevMsg( " restore failed entirely\n" );
		}
		else
		{
			// We can get to a valid place, but not all the way back to where we were.
			Vector vPos;
			VectorLerp( pEntity->GetLocalOrigin(), vWantedPos, tr.fraction * g_flFractionScale, vPos );
			UTIL_SetOrigin( pEntity, vPos, true );

			if ( sv_unlag_debug.GetBool() )
				DevMsg( " restore got most of the way\n" );
		}
	}
	else
	{
		// Cool, the player can go back to whence he came.
		UTIL_SetOrigin( pEntity, tr.endpos, true );
	}
}

static CLagCompensationManager g_LagCompensationManager( "CLagCompensationManager" );
ILagCompensationManager *lagcompensation = &g_LagCompensationManager;

// Mappers can flag certain additional entities to lag compensate, this handles them
void CLagCompensationManager::AddAdditionalEntity( CBaseEntity *pEntity )
{
	EHANDLE eh;
	eh = pEntity;
	if ( m_AdditionalEntities.Find( eh ) == m_AdditionalEntities.InvalidIndex() )
	{
		m_AdditionalEntities.Insert( eh );
	}
}

void CLagCompensationManager::RemoveAdditionalEntity( CBaseEntity *pEntity )
{
	EHANDLE eh;
	eh = pEntity;
	m_AdditionalEntities.Remove( eh );
}

//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all entities have had a chance to think
//-----------------------------------------------------------------------------
void CLagCompensationManager::FrameUpdatePostEntityThink()
{
	if ( (gpGlobals->maxClients <= 1) || !sv_unlag.GetBool() )
	{
		ClearHistory();
		return;
	}
	
	m_flTeleportDistanceSqr = sv_lagcompensation_teleport_dist.GetFloat() * sv_lagcompensation_teleport_dist.GetFloat();

	VPROF_BUDGET( "FrameUpdatePostEntityThink", "CLagCompensationManager" );

	CUtlRBTree< CBaseEntity * > rbEntityList( 0, 0, DefLessFunc( CBaseEntity * ) );

	// Add active players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
		{
			continue;
		}

		if ( rbEntityList.Find( pPlayer ) == rbEntityList.InvalidIndex() )
		{
			rbEntityList.Insert( pPlayer );
		}
	}

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[i];

		if ( rbEntityList.Find( pNPC ) == rbEntityList.InvalidIndex() )
		{
			rbEntityList.Insert( pNPC );
		}
	}

	// Add any additional entities
	for ( int i = m_AdditionalEntities.FirstInorder(); i != m_AdditionalEntities.InvalidIndex(); i = m_AdditionalEntities.NextInorder( i ) )
	{
		CBaseEntity *pAddEntity = m_AdditionalEntities[ i ].Get();
		if ( !pAddEntity )
			continue;

		if ( rbEntityList.Find( pAddEntity ) == rbEntityList.InvalidIndex() )
		{
			rbEntityList.Insert( pAddEntity );
		}
	}

	// Now record the actual history information
	for ( int i = rbEntityList.FirstInorder(); i != rbEntityList.InvalidIndex(); i = rbEntityList.NextInorder( i  ) )
	{
		CBaseEntity *pEntity = rbEntityList[ i ];

		EHANDLE eh;
		eh = pEntity;

		int slot = m_CompensatedEntities.Find( eh );
		if ( slot == m_CompensatedEntities.InvalidIndex() )
		{
			EntityLagData *pNewEntry = new EntityLagData();
			slot = m_CompensatedEntities.Insert( eh, pNewEntry );
		}

		EntityLagData *ld = m_CompensatedEntities[ slot ];

		RecordDataIntoTrack( pEntity, &ld->m_LagRecords, true );
	}
}

// Called during player movement to set up/restore after lag compensation
void CLagCompensationManager::StartLagCompensation( CBasePlayer *player, LagCompensationType lagCompensationType, const Vector& weaponPos, const QAngle &weaponAngles, float weaponRange )
{
	Assert(!m_isCurrentlyDoingCompensation);

	// Assume no entities need to be restored
	CUtlVector< EHANDLE > invalidList;
	FOR_EACH_MAP( m_CompensatedEntities, i )
	{
		EntityLagData *ld = m_CompensatedEntities[ i ];
		EHANDLE key;
		key = m_CompensatedEntities.Key( i );
		if ( !key.Get() )
		{
			// Note that the EHANDLE is NULL now
			invalidList.AddToTail( key );
			continue;
		}

		// Clear state
		ld->m_bRestoreEntity = false;
		ld->m_RestoreData.Clear();
		ld->m_ChangeData.Clear();
	}

	// Wipe any deleted entities from the list
	for ( int i = 0; i < invalidList.Count(); ++i )
	{
		int slot = m_CompensatedEntities.Find( invalidList[ i ] );
		Assert( slot != m_CompensatedEntities.InvalidIndex() );
		if ( slot == m_CompensatedEntities.InvalidIndex() )
			continue;

		EntityLagData *ld = m_CompensatedEntities[ slot ];
		delete ld;
		m_CompensatedEntities.RemoveAt( slot );
	}

	m_bNeedToRestore = false;

	m_pCurrentPlayer = player;
	
	if ( !player->m_bLagCompensation		// Player not wanting lag compensation
		 || (gpGlobals->maxClients <= 1)	// no lag compensation in single player
		 || !sv_unlag.GetBool()				// disabled by server admin
		 || player->IsBot() 				// not for bots
		 || player->IsObserver()			// not for spectators
		)
		return;

	const CUserCmd *cmd = player->GetCurrentUserCommand(); 
	if ( !cmd )
	{
		// This can hit if m_hActiveWeapon incorrectly gets set to a weapon not actually owned by the local player 
		// (since lag comp asks for the GetPlayerOwner() which will have m_pCurrentCommand == NULL, 
		//  not the player currently executing a CUserCmd).
		Error( "CLagCompensationManager::StartLagCompensation with NULL CUserCmd!!!\n" );
	}

	// NOTE: Put this here so that it won't show up in single player mode.
	VPROF_BUDGET( "StartLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING );

	m_isCurrentlyDoingCompensation = true;

	// correct is the amount of time we have to correct game time
	float correct = 0.0f;

	// Get true latency
	INetChannelInfo *nci = engine->GetPlayerNetInfo( player->entindex() ); 
	if ( nci )
	{
		// add network latency
		correct+= nci->GetLatency( FLOW_OUTGOING );
	}

	// NOTE:  do these computations in float time, not ticks, to avoid big roundoff error accumulations in the math
	// add view interpolation latency see C_BaseEntity::GetInterpolationAmount()
	correct += player->m_fLerpTime;
	
	// check bounds [0,sv_maxunlag]
	correct = clamp( correct, 0.0f, sv_maxunlag.GetFloat() );

	// correct tick send by player 
	float flTargetTime = TICKS_TO_TIME( cmd->tick_count ) - player->m_fLerpTime;

	// calculate difference between tick sent by player and our latency based tick
	float deltaTime =  correct - ( gpGlobals->curtime - flTargetTime );

	if ( fabs( deltaTime ) > 0.2f )
	{
		// difference between cmd time and latency is too big > 200ms, use time correction based on latency
		// DevMsg("StartLagCompensation: delta too big (%.3f)\n", deltaTime );
		flTargetTime = gpGlobals->curtime - correct;
	}

	flTargetTime += TICKS_TO_TIME( sv_lagpushticks.GetInt() );

	m_lagCompensationType = lagCompensationType;
	m_weaponPos = weaponPos;
	m_weaponAngles = weaponAngles;
	m_weaponRange = weaponRange;

	// Iterate all lag compensatable entities
	const CBitVec<MAX_EDICTS> *pEntityTransmitBits = engine->GetEntityTransmitBitsForClient( player->entindex() - 1 );

	FOR_EACH_MAP( m_CompensatedEntities, i )
	{
		EntityLagData *ld = m_CompensatedEntities[ i ];
		EHANDLE key = m_CompensatedEntities.Key( i );
		CBaseEntity *pEntity = key.Get();
		if ( !pEntity )
		{
			// Stale entity in list, fixed up on next time through loop
			continue;
		}

		// Don't lag compensate self
		if ( player == pEntity )
		{
			continue;
		}

		// Custom checks for if things should lag compensate (based on things like what team the entity is associated with).
		if ( !player->WantsLagCompensationOnEntity( pEntity, cmd, pEntityTransmitBits ) )
			continue;

		// Move entity back in time and remember that fact
		ld->m_bRestoreEntity = BacktrackEntity( pEntity, flTargetTime, &ld->m_LagRecords, &ld->m_RestoreData, &ld->m_ChangeData, true );
	}
}

bool CLagCompensationManager::BacktrackEntity( CBaseEntity *entity, float flTargetTime, LagRecordList *track, LagRecord *restore, LagRecord *change, bool wantsAnims )
{
	Vector org, minsPreScaled, maxsPreScaled;
	QAngle ang;

	VPROF_BUDGET( "BacktrackEntity", "CLagCompensationManager" );

	// check if we have at least one entry
	if ( track->Count() <= 0 )
		return false;

	int curr = track->Head();

	LagRecord *prevRecord = NULL;
	LagRecord *record = NULL;

	Vector prevOrg = entity->GetAbsOrigin();
	
	// Walk context looking for any invalidating event
	while( track->IsValidIndex(curr) )
	{
		// remember last record
		prevRecord = record;

		// get next record
		record = &track->Element( curr );

		if ( !(record->m_fFlags & LC_ALIVE) )
		{
			// entity must be alive, lost track
			return false;
		}

		Vector delta = record->m_vecOrigin - prevOrg;
		if ( delta.LengthSqr() > m_flTeleportDistanceSqr )
		{
			// lost track, too much difference
			return false;
		}

		// did we find a context smaller than target time ?
		if ( record->m_flSimulationTime <= flTargetTime )
			break; // hurra, stop

		prevOrg = record->m_vecOrigin;

		// go one step back
		curr = track->Next( curr );
	}

	Assert( record );

	if ( !record )
	{
		if ( sv_unlag_debug.GetBool() )
		{
			DevMsg( "No valid positions in history for BacktrackEntity entity ( %s )\n", entity->GetDebugName() );
		}
		return false;
	}

	float frac = 0.0f;
	if ( prevRecord && 
		 (record->m_flSimulationTime < flTargetTime) &&
		 (record->m_flSimulationTime < prevRecord->m_flSimulationTime) )
	{
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		Assert( prevRecord->m_flSimulationTime > record->m_flSimulationTime );
		Assert( flTargetTime < prevRecord->m_flSimulationTime );

		// calc fraction between both records
		frac = ( flTargetTime - record->m_flSimulationTime ) / 
			( prevRecord->m_flSimulationTime - record->m_flSimulationTime );

		Assert( frac > 0 && frac < 1 ); // should never extrapolate

		ang  = Lerp( frac, record->m_vecAngles, prevRecord->m_vecAngles );
		org  = Lerp( frac, record->m_vecOrigin, prevRecord->m_vecOrigin  );
		minsPreScaled = Lerp( frac, record->m_vecMinsPreScaled, prevRecord->m_vecMinsPreScaled  );
		maxsPreScaled = Lerp( frac, record->m_vecMaxsPreScaled, prevRecord->m_vecMaxsPreScaled );
	}
	else
	{
		// we found the exact record or no other record to interpolate with
		// just copy these values since they are the best we have
		ang  = record->m_vecAngles;
		org  = record->m_vecOrigin;
		minsPreScaled = record->m_vecMinsPreScaled;
		maxsPreScaled = record->m_vecMaxsPreScaled;
	}

	// See if this is still a valid position for us to teleport to
	if ( sv_unlag_fixstuck.GetBool() )
	{
		// Try to move to the wanted position from our current position.
		trace_t tr;
		UTIL_TraceEntity( entity, org, org, UTIL_MaskForEntity(entity), &tr );

		if ( tr.startsolid || tr.allsolid )
		{
			if ( sv_unlag_debug.GetBool() )
				DevMsg( "WARNING: BackupPlayer trying to back player into a bad position - client %d\n", entity->entindex() );

			CBaseEntity *pHitEntity = tr.m_pEnt;
			// don't lag compensate the current player
			if ( pHitEntity && ( pHitEntity != m_pCurrentPlayer ) )	
			{
				// Find it
				EHANDLE eh;
				eh = pHitEntity;

				int slot = m_CompensatedEntities.Find( eh );
				if ( slot != m_CompensatedEntities.InvalidIndex() )
				{
					EntityLagData *ld = m_CompensatedEntities[ slot ];

					// If we haven't backtracked this player, do it now
					// this deliberately ignores WantsLagCompensationOnEntity.
					if ( !ld->m_bRestoreEntity )
					{
						// Temp turn this flag on
						ld->m_bRestoreEntity = true;

						BacktrackEntity( pHitEntity, flTargetTime, &ld->m_LagRecords, &ld->m_RestoreData, &ld->m_ChangeData, true );

						// Remove the temp flag
						ld->m_bRestoreEntity = false;
					}	
				}
			}

			// now trace us back as far as we can go
			unsigned int mask = UTIL_MaskForEntity(entity);
			UTIL_TraceEntity( entity, entity->GetAbsOrigin(), org, mask, &tr );

			if ( tr.startsolid || tr.allsolid )
			{
				// Our starting position is bogus

				if ( sv_unlag_debug.GetBool() )
					DevMsg( "Backtrack failed completely, bad starting position\n" );
			}
			else
			{
				// We can get to a valid place, but not all the way to the target
				Vector vPos;
				VectorLerp( entity->GetAbsOrigin(), org, tr.fraction * g_flFractionScale, vPos );
				
				// This is as close as we're going to get
				org = vPos;

				if ( sv_unlag_debug.GetBool() )
					DevMsg( "Backtrack got most of the way\n" );
			}
		}
	}
	
	// See if this represents a change for the entity
	int flags = 0;

	QAngle angdiff = entity->GetAbsAngles() - ang;
	Vector orgdiff = entity->GetAbsOrigin() - org;

	// Always remember the pristine simulation time in case we need to restore it.
	restore->m_flSimulationTime = entity->GetSimulationTime();

	if ( angdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
	{
		flags |= LC_ANGLES_CHANGED;
		restore->m_vecAngles = entity->GetAbsAngles();
		entity->SetAbsAngles( ang );
		change->m_vecAngles = ang;
	}

	// Use absolute equality here
	if ( ( minsPreScaled != entity->CollisionProp()->OBBMinsPreScaled() ) ||
		 ( maxsPreScaled != entity->CollisionProp()->OBBMaxsPreScaled() ) )
	{
		flags |= LC_SIZE_CHANGED;

		restore->m_vecMinsPreScaled = entity->CollisionProp()->OBBMinsPreScaled() ;
		restore->m_vecMaxsPreScaled = entity->CollisionProp()->OBBMaxsPreScaled();

		entity->SetSize( minsPreScaled, maxsPreScaled );

		change->m_vecMinsPreScaled = minsPreScaled;
		change->m_vecMaxsPreScaled = maxsPreScaled;
	}

	// Note, do origin at end since it causes a relink into the k/d tree
	if ( orgdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR )
	{
		flags |= LC_ORIGIN_CHANGED;
		restore->m_vecOrigin = entity->GetAbsOrigin();
		entity->SetAbsOrigin( org );
		change->m_vecOrigin = org;
	}

	bool skipAnims = false;
	switch (m_lagCompensationType)
	{
	case LAG_COMPENSATE_BOUNDS:
		skipAnims = true;
		break;
	case LAG_COMPENSATE_HITBOXES:
		skipAnims = false;
		break;
	case LAG_COMPENSATE_HITBOXES_ALONG_RAY:
		skipAnims = false;
		{
			Vector forward;
			AngleVectors( m_weaponAngles, &forward );

			const float maxRange = 100.0f;
			float range = DistanceToRay( entity->WorldSpaceCenter(), m_weaponPos, m_weaponPos + forward * m_weaponRange );
			if ( range < 0.0f || range > maxRange )
			{
				skipAnims = true;
			}
		}
		break;
	}

	if( !skipAnims && wantsAnims && entity->GetBaseAnimating() )
	{
		CBaseAnimating *pAnimating = entity->GetBaseAnimating();

		// Sorry for the loss of the optimization for the case of people
		// standing still, but you breathe even on the server.
		// This is quicker than actually comparing all bazillion floats.
		flags |= LC_ANIMATION_CHANGED;
		restore->m_masterSequence = pAnimating->GetSequence();
		restore->m_masterCycle = pAnimating->GetCycle();

		bool interpolationAllowed = false;
		if( prevRecord && (record->m_masterSequence == prevRecord->m_masterSequence) )
		{
			// If the master state changes, all layers will be invalid too, so don't interp (ya know, interp barely ever happens anyway)
			interpolationAllowed = true;
		}
		
		////////////////////////
		// First do the master settings
		bool interpolatedMasters = false;
		if( frac > 0.0f && interpolationAllowed )
		{
			interpolatedMasters = true;
			pAnimating->SetSequence( Lerp( frac, record->m_masterSequence, prevRecord->m_masterSequence ) );
			pAnimating->SetCycle( Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle ) );

			if( record->m_masterCycle > prevRecord->m_masterCycle )
			{
				// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
				// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
				float newCycle = Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle + 1 );
				pAnimating->SetCycle(newCycle < 1 ? newCycle : newCycle - 1 );// and make sure .9 to 1.2 does not end up 1.05
			}
			else
			{
				pAnimating->SetCycle( Lerp( frac, record->m_masterCycle, prevRecord->m_masterCycle ) );
			}
		}
		if( !interpolatedMasters )
		{
			pAnimating->SetSequence(record->m_masterSequence);
			pAnimating->SetCycle(record->m_masterCycle);
		}

		////////////////////////
		// Now do all the layers
		//
		CBaseAnimatingOverlay *pAnimatingOverlay = entity->GetBaseAnimatingOverlay();
		if ( pAnimatingOverlay )
		{
			int layerCount = pAnimatingOverlay->GetNumAnimOverlays();
			for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
			{
				CAnimationLayer *currentLayer = pAnimatingOverlay->GetAnimOverlay(layerIndex);
				if( currentLayer )
				{
					restore->m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
					restore->m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
					restore->m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
					restore->m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;

					bool interpolated = false;
					if( (frac > 0.0f)  &&  interpolationAllowed )
					{
						LayerRecord &recordsLayerRecord = record->m_layerRecords[layerIndex];
						LayerRecord &prevRecordsLayerRecord = prevRecord->m_layerRecords[layerIndex];
						if( (recordsLayerRecord.m_order == prevRecordsLayerRecord.m_order)
							&& (recordsLayerRecord.m_sequence == prevRecordsLayerRecord.m_sequence)
							)
						{
							// We can't interpolate across a sequence or order change
							interpolated = true;
							if( recordsLayerRecord.m_cycle > prevRecordsLayerRecord.m_cycle )
							{
								// the older record is higher in frame than the newer, it must have wrapped around from 1 back to 0
								// add one to the newer so it is lerping from .9 to 1.1 instead of .9 to .1, for example.
								float newCycle = Lerp( frac, recordsLayerRecord.m_cycle, prevRecordsLayerRecord.m_cycle + 1 );
								currentLayer->m_flCycle = newCycle < 1 ? newCycle : newCycle - 1;// and make sure .9 to 1.2 does not end up 1.05
							}
							else
							{
								currentLayer->m_flCycle = Lerp( frac, recordsLayerRecord.m_cycle, prevRecordsLayerRecord.m_cycle  );
							}
							currentLayer->m_nOrder = recordsLayerRecord.m_order;
							currentLayer->m_nSequence = recordsLayerRecord.m_sequence;
							currentLayer->m_flWeight = Lerp( frac, recordsLayerRecord.m_weight, prevRecordsLayerRecord.m_weight  );
						}
					}
					if( !interpolated )
					{
						//Either no interp, or interp failed.  Just use record.
						currentLayer->m_flCycle = record->m_layerRecords[layerIndex].m_cycle;
						currentLayer->m_nOrder = record->m_layerRecords[layerIndex].m_order;
						currentLayer->m_nSequence = record->m_layerRecords[layerIndex].m_sequence;
						currentLayer->m_flWeight = record->m_layerRecords[layerIndex].m_weight;
					}
				}
			}
		}
	}
	
	if ( !flags )
		return false; // we didn't change anything

	if ( sv_lagflushbonecache.GetBool() && (flags & LC_ANIMATION_CHANGED) && entity->GetBaseAnimating() )
		entity->GetBaseAnimating()->InvalidateBoneCache();

	/*
	char text[256]; Q_snprintf( text, sizeof(text), "time %.2f", flTargetTime );
	pPlayer->DrawServerHitboxes( 10 );
	NDebugOverlay::Text( org, text, false, 10 );
	if ( skipAnims )
	{
		NDebugOverlay::EntityBounds( pPlayer, 0, 255, 0, 32, 10 );
	}
	else
	{
		NDebugOverlay::EntityBounds( pPlayer, 255, 0, 0, 32, 10 );
	}
	*/

	m_bNeedToRestore = true;  // we changed at least one entity
	restore->m_fFlags = flags; // we need to restore these flags
	change->m_fFlags = flags; // we have changed these flags

	if( sv_showlagcompensation.GetInt() == 1 && entity->GetBaseAnimating() )
	{
		entity->GetBaseAnimating()->DrawServerHitboxes(4, true);
	}

	return true;// Yes, this guy has been backtracked
}

void CLagCompensationManager::FinishLagCompensation( CBasePlayer *player )
{
	VPROF_BUDGET_FLAGS( "FinishLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING, BUDGETFLAG_CLIENT|BUDGETFLAG_SERVER );

	m_isCurrentlyDoingCompensation = false;

	if ( !m_bNeedToRestore )
		return; // no entity was changed at all

	// Iterate all active entities
	FOR_EACH_MAP( m_CompensatedEntities, i )
	{
		EntityLagData *ld = m_CompensatedEntities[ i ];
		// entity wasn't changed by lag compensation
		if ( !ld->m_bRestoreEntity )
			continue;

		CBaseEntity *pEntity = m_CompensatedEntities.Key( i ).Get();
		if ( !pEntity )
			continue;

		LagRecord *restore = &ld->m_RestoreData;
		LagRecord *change  = &ld->m_ChangeData;

		RestoreEntityFromRecords( pEntity, restore, change, true );
	}
}

void CLagCompensationManager::RecordDataIntoTrack( CBaseEntity *entity, LagRecordList *track, bool wantsAnims )
{
	Assert( track->Count() < 1000 ); // insanity check

	// remove all records before that time:
	int flDeadtime = gpGlobals->curtime - sv_maxunlag.GetFloat();

	// remove tail records that are too old
	int tailIndex = track->Tail();
	while ( track->IsValidIndex( tailIndex ) )
	{
		LagRecord &tail = track->Element( tailIndex );

		// if tail is within limits, stop
		if ( tail.m_flSimulationTime >= flDeadtime )
			break;

		// remove tail, get new tail
		track->Remove( tailIndex );
		tailIndex = track->Tail();
	}

	// check if head has same simulation time
	if ( track->Count() > 0 )
	{
		LagRecord &head = track->Element( track->Head() );

		// check if player changed simulation time since last time updated
		if ( head.m_flSimulationTime >= entity->GetSimulationTime() )
			return; // don't add new entry for same or older time
	}

	// add new record to entity track
	LagRecord &record = track->Element( track->AddToHead() );

	record.m_fFlags = 0;
	if ( entity->IsAlive() )
	{
		record.m_fFlags |= LC_ALIVE;
	}

	record.m_flSimulationTime	= entity->GetSimulationTime();
	record.m_vecAngles			= entity->GetAbsAngles();
	record.m_vecOrigin			= entity->GetAbsOrigin();
	record.m_vecMaxsPreScaled			= entity->CollisionProp()->OBBMaxsPreScaled();
	record.m_vecMinsPreScaled			= entity->CollisionProp()->OBBMinsPreScaled();

	CBaseAnimating *pAnimating = entity->GetBaseAnimating();

	if( wantsAnims && pAnimating )
	{
		CBaseAnimatingOverlay *pAnimatingOverlay = entity->GetBaseAnimatingOverlay();

		if ( pAnimatingOverlay )
		{
			int layerCount = pAnimatingOverlay->GetNumAnimOverlays();
			for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
			{
				CAnimationLayer *currentLayer = pAnimatingOverlay->GetAnimOverlay(layerIndex);
				if( currentLayer )
				{
					record.m_layerRecords[layerIndex].m_cycle = currentLayer->m_flCycle;
					record.m_layerRecords[layerIndex].m_order = currentLayer->m_nOrder;
					record.m_layerRecords[layerIndex].m_sequence = currentLayer->m_nSequence;
					record.m_layerRecords[layerIndex].m_weight = currentLayer->m_flWeight;
				}
			}
		}
		record.m_masterSequence = pAnimating->GetSequence();
		record.m_masterCycle = pAnimating->GetCycle();
	}
}
void CLagCompensationManager::RestoreEntityFromRecords( CBaseEntity *entity, LagRecord *restore, LagRecord *change, bool wantsAnims )
{
	bool restoreSimulationTime = false;

	if ( restore->m_fFlags & LC_SIZE_CHANGED )
	{
		restoreSimulationTime = true;

		// see if simulation made any changes, if no, then do the restore, otherwise,
		//  leave new values in
		if ( entity->CollisionProp()->OBBMinsPreScaled() == change->m_vecMinsPreScaled &&
			entity->CollisionProp()->OBBMaxsPreScaled() == change->m_vecMaxsPreScaled )
		{
			// Restore it
			entity->SetSize( restore->m_vecMinsPreScaled, restore->m_vecMaxsPreScaled );
		}
	}

	if ( restore->m_fFlags & LC_ANGLES_CHANGED )
	{		   
		restoreSimulationTime = true;

		if ( entity->GetAbsAngles() == change->m_vecAngles )
		{
			entity->SetAbsAngles( restore->m_vecAngles );
		}
	}

	if ( restore->m_fFlags & LC_ORIGIN_CHANGED )
	{
		restoreSimulationTime = true;

		// Okay, let's see if we can do something reasonable with the change
		Vector delta = entity->GetAbsOrigin() - change->m_vecOrigin;

		// If it moved really far, just leave the entity in the new spot!!!
		if ( delta.LengthSqr() < m_flTeleportDistanceSqr )
		{
			RestoreEntityTo( entity, restore->m_vecOrigin + delta );
		}
	}
			
	CBaseAnimating *pAnimating = entity->GetBaseAnimating();

	if( wantsAnims && pAnimating )
	{
		if( restore->m_fFlags & LC_ANIMATION_CHANGED )
		{
			restoreSimulationTime = true;

			pAnimating->SetSequence(restore->m_masterSequence);
			pAnimating->SetCycle(restore->m_masterCycle);

			CBaseAnimatingOverlay *pAnimatingOverlay = entity->GetBaseAnimatingOverlay();
			if ( pAnimatingOverlay )
			{
				int layerCount = pAnimatingOverlay->GetNumAnimOverlays();
				for( int layerIndex = 0; layerIndex < layerCount; ++layerIndex )
				{
					CAnimationLayer *currentLayer = pAnimatingOverlay->GetAnimOverlay(layerIndex);
					if( currentLayer )
					{
						currentLayer->m_flCycle = restore->m_layerRecords[layerIndex].m_cycle;
						currentLayer->m_nOrder = restore->m_layerRecords[layerIndex].m_order;
						currentLayer->m_nSequence = restore->m_layerRecords[layerIndex].m_sequence;
						currentLayer->m_flWeight = restore->m_layerRecords[layerIndex].m_weight;
					}
				}
			}
		}
	}

	if ( restoreSimulationTime )
	{
		entity->SetSimulationTime( restore->m_flSimulationTime );
	}
}
