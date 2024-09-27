//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLASSMAP_H
#define ICLASSMAP_H
#pragma once

#include "tier0/platform.h"
#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL( LOG_ENTITYFACTORY );

class C_BaseEntity;
typedef C_BaseEntity* (*DISPATCHFUNCTION)( const char * );

abstract_class IClassMap
{
public:
	virtual					~IClassMap() {}

	virtual void			Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory = 0 ) = 0;
	virtual char const		*ClassnameToMapName( const char *classname ) = 0;
	virtual char const		*MapNameToClassname( const char *mapname ) = 0;
	virtual C_BaseEntity	*CreateEntity( const char *mapname ) = 0;
	virtual int				GetClassSize( const char *classname ) = 0;
};

extern IClassMap& GetClassMap();


#endif // ICLASSMAP_H
