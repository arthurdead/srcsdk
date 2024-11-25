#if !defined( ISERVERENTITYLIST_H )
#define ISERVERENTITYLIST_H

#pragma once

#include "interface.h"

class IServerEntity;
class ServerClass;
class IServerNetworkable;
class CBaseHandle;
class IServerUnknown;

//-----------------------------------------------------------------------------
// Purpose: Exposes IServerEntity's to engine
//-----------------------------------------------------------------------------
abstract_class IServerEntityList
{
public:
	// Get IServerNetworkable interface for specified entity
	virtual IServerNetworkable*	GetServerNetworkable( int entnum ) = 0;
	virtual IServerNetworkable*	GetServerNetworkableFromHandle( CBaseHandle hEnt ) = 0;
	virtual IServerUnknown*		GetServerUnknownFromHandle( CBaseHandle hEnt ) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIServerEntity().
	virtual IServerEntity*		GetServerEntity( int entnum ) = 0;
	virtual IServerEntity*		GetServerEntityFromHandle( CBaseHandle hEnt ) = 0;

	// Returns number of entities currently in use
	virtual int					NumberOfEntities( bool bIncludeNonNetworkable ) = 0;

	// Returns highest index actually used
	virtual int					GetHighestEntityIndex( void ) = 0;
};

#ifdef GAME_DLL
extern IServerEntityList *entitylist;
#else
extern IServerEntityList *sv_entitylist;
#endif

#define VSERVERENTITYLIST_INTERFACE_VERSION	"VServerEntityList001"

#endif // ICLIENTENTITYLIST_H
