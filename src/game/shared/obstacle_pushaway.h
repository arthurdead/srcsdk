//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OBSTACLE_PUSHAWAY_H
#define OBSTACLE_PUSHAWAY_H
#pragma once

#include "props_shared.h"
#ifndef CLIENT_DLL
#include "func_breakablesurf.h"
#include "BasePropDoor.h"
#include "doors.h"
#endif // CLIENT_DLL
#include "ispatialpartition.h"

#ifdef GAME_DLL
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
#else
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
#endif

//--------------------------------------------------------------------------------------------------------------
bool IsPushAwayEntity( CSharedBaseEntity *pEnt );
bool IsPushableEntity( CSharedBaseEntity *pEnt );

//--------------------------------------------------------------------------------------------------------------
#ifndef CLIENT_DLL
bool IsBreakableEntity( CBaseEntity *pEnt );
#endif // !CLIENT_DLL

//--------------------------------------------------------------------------------------------------------------
class CPushAwayEnumerator : public IPartitionEnumerator
{
public:
	// Forced constructor
	CPushAwayEnumerator(CSharedBaseEntity **ents, int nMaxEnts)
	{
		m_nAlreadyHit = 0;
		m_AlreadyHit = ents;
		m_nMaxHits = nMaxEnts;
	}
	
	// Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

public:

	CSharedBaseEntity **m_AlreadyHit;
	int m_nAlreadyHit;
	int m_nMaxHits;
};


#ifndef CLIENT_DLL
//--------------------------------------------------------------------------------------------------------------
/**
 * This class will collect breakable objects in a volume.  Physics props that can be damaged, func_breakable*, etc
 * are all collected by this class.
 */
class CBotBreakableEnumerator : public CPushAwayEnumerator
{
public:
	CBotBreakableEnumerator(CBaseEntity **ents, int nMaxEnts) : CPushAwayEnumerator(ents, nMaxEnts)
	{
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
};


//--------------------------------------------------------------------------------------------------------------
/**
 * This class will collect door objects in a volume.
 */
class CBotDoorEnumerator : public CPushAwayEnumerator
{
public:
	CBotDoorEnumerator(CBaseEntity **ents, int nMaxEnts) : CPushAwayEnumerator(ents, nMaxEnts)
	{
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
};


//--------------------------------------------------------------------------------------------------------------
/**
 *  Returns an entity that matches the filter that is along the line segment
 */
CBaseEntity * CheckForEntitiesAlongSegment( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, CPushAwayEnumerator *enumerator );
#endif // CLIENT_DLL


//--------------------------------------------------------------------------------------------------------------
// Retrieves physics objects near pPushingEntity
void AvoidPushawayProps(  CSharedBaseCombatCharacter *pPlayer, CUserCmd *pCmd );
int GetPushawayEnts( CSharedBaseCombatCharacter *pPushingEntity, CSharedBaseEntity **ents, int nMaxEnts, float flPlayerExpand, int PartitionMask, CPushAwayEnumerator *enumerator = NULL );

//--------------------------------------------------------------------------------------------------------------
// Pushes physics objects away from the entity
void PerformObstaclePushaway( CSharedBaseCombatCharacter *pPushingEntity );


#endif // OBSTACLE_PUSHAWAY_H
