//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef POSITIONWATCHER_H
#define POSITIONWATCHER_H
#pragma once

#include "ehandle.h"

class IPhysicsObject;

// inherit from this interface to be able to call WatchPositionChanges
abstract_class IWatcherCallback
{
public:
	virtual ~IWatcherCallback() {}
};

abstract_class IPositionWatcher : public IWatcherCallback
{
public:
	virtual void NotifyPositionChanged( CSharedBaseEntity *pEntity ) = 0;
};

// NOTE: The table of watchers is NOT saved/loaded!  Recreate these links on restore
void ReportPositionChanged( CSharedBaseEntity *pMovedEntity );
void WatchPositionChanges( CSharedBaseEntity *pWatcher, CSharedBaseEntity *pMovingEntity );
void RemovePositionWatcher( CSharedBaseEntity *pWatcher, CSharedBaseEntity *pMovingEntity );


// inherit from this interface to be able to call WatchPositionChanges
abstract_class IVPhysicsWatcher : public IWatcherCallback
{
public:
	virtual void NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CSharedBaseEntity *pEntity, bool bAwake ) = 0;
};

// NOTE: The table of watchers is NOT saved/loaded!  Recreate these links on restore
void ReportVPhysicsStateChanged( IPhysicsObject *pPhysics, CSharedBaseEntity *pEntity, bool bAwake );
void WatchVPhysicsStateChanges( CSharedBaseEntity *pWatcher, CSharedBaseEntity *pPhysicsEntity );
void RemoveVPhysicsStateWatcher( CSharedBaseEntity *pWatcher, CSharedBaseEntity *pPhysicsEntity );


#endif // POSITIONWATCHER_H
