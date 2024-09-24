//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERUNKNOWN_H
#define ISERVERUNKNOWN_H

#pragma once


#include "ihandleentity.h"

class ICollideable;
class IServerNetworkable;
#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CServerBaseEntity;
#else
class CServerBaseEntity;
#endif


// This is the server's version of IUnknown. We may want to use a QueryInterface-like
// mechanism if this gets big.
class IServerUnknown : public IHandleEntity
{
public:
	// Gets the interface to the collideable + networkable representation of the entity
	virtual ICollideable*		GetCollideable() = 0;
	virtual IServerNetworkable*	GetNetworkable() = 0;
	virtual CServerBaseEntity*		GetBaseEntity() = 0;
};

class IServerUnknownMod
{
public:
	
};

class IServerUnknownEx : public IServerUnknown, public IServerUnknownMod
{
public:
	
};


#endif // ISERVERUNKNOWN_H
