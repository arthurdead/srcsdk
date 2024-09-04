//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERENTITY_H
#define ISERVERENTITY_H
#pragma once


#include "iserverunknown.h"
#include "string_t.h"
#if defined GAME_DLL || defined CLIENT_DLL
#include "ehandle.h"
#else
typedef CBaseHandle EHANDLE;
#endif

struct Ray_t;
class ServerClass;
class ICollideable;
class IServerNetworkable;
class Vector;
class QAngle;

// This class is how the engine talks to entities in the game DLL.
// CBaseEntity implements this interface.
class IServerEntity	: public IServerUnknown
{
public:
	virtual					~IServerEntity() {}

// Previously in pev
	virtual int				GetModelIndex( void ) const = 0;
 	virtual string_t		GetModelName( void ) const = 0;

	virtual void			SetModelIndex( int index ) = 0;
};

class IServerEntityMod
{
public:
	virtual const EHANDLE& GetRefEHandle() const = 0;
};

class IServerEntityEx	: public IServerEntity, public IServerUnknownMod, public IServerEntityMod
{
public:
	virtual const EHANDLE& GetRefEHandle() const = 0;
};


#endif // ISERVERENTITY_H
