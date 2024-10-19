//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( ICLIENTENTITYLIST_H )
#define ICLIENTENTITYLIST_H

#pragma once

#include "interface.h"

class IClientEntity;
class ClientClass;
class IClientNetworkable;
class CBaseHandle;
class IClientUnknown;

// Cached info for networked entities.
// NOTE: Changing this changes the interface between engine & client
struct ClientEntityCacheInfo_t
{
	// Cached off because GetClientNetworkable is called a *lot*
	IClientNetworkable *m_pNetworkable;
	unsigned short m_BaseEntitiesIndex;	// Index into m_BaseEntities (or m_BaseEntities.InvalidIndex() if none).
	unsigned short m_bDormant;	// cached dormant state - this is only a bit
};

#ifdef CLIENT_DLL
typedef ClientEntityCacheInfo_t EntityCacheInfo_t;
#endif

//-----------------------------------------------------------------------------
// Purpose: Exposes IClientEntity's to engine
//-----------------------------------------------------------------------------
abstract_class IClientEntityList
{
public:
	// Get IClientNetworkable interface for specified entity
	virtual IClientNetworkable*	GetClientNetworkable( int entnum ) = 0;
	virtual IClientNetworkable*	GetClientNetworkableFromHandle( CBaseHandle hEnt ) = 0;
	virtual IClientUnknown*		GetClientUnknownFromHandle( CBaseHandle hEnt ) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIClientEntity().
	virtual IClientEntity*		GetClientEntity( int entnum ) = 0;
	virtual IClientEntity*		GetClientEntityFromHandle( CBaseHandle hEnt ) = 0;

	// Returns number of entities currently in use
	virtual int					NumberOfEntities( bool bIncludeNonNetworkable ) = 0;

	// Returns highest index actually used
	virtual int					GetHighestEntityIndex( void ) = 0;

	// Sizes entity list to specified size
	virtual void				SetMaxEntities( int maxents ) = 0;
	virtual int					GetMaxEntities( ) = 0;
};

abstract_class IClientEntityListEx : public IClientEntityList
{
public:
	virtual ClientEntityCacheInfo_t	*GetClientNetworkableArray() = 0;
};

#ifdef GAME_DLL
extern IClientEntityList *cl_entitylist;
extern IClientEntityListEx *cl_entitylist_ex;
#else
extern IClientEntityList *entitylist;
extern IClientEntityListEx *entitylist_ex;
#endif

#define VCLIENTENTITYLIST_INTERFACE_VERSION	"VClientEntityList003"
#define VCLIENTENTITYLIST_EX_INTERFACE_VERSION	"VClientEntityExList001"

#endif // ICLIENTENTITYLIST_H
