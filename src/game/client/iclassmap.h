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

abstract_class IEntityFactory
{
public:
	virtual C_BaseEntity *Create( const char *pClassName ) = 0;
	virtual void Destroy( C_BaseEntity *pNetworkable ) = 0;
	virtual size_t GetEntitySize() = 0;
	virtual const char *DllClassname() const  = 0;
};

abstract_class IClassMap
{
public:
	virtual					~IClassMap() {}

	virtual void			Add( const char *mapname, IEntityFactory *factory ) = 0;
	virtual C_BaseEntity	*CreateEntity( const char *mapname ) = 0;
	virtual int				GetClassSize( const char *mapname ) = 0;
	virtual IEntityFactory *FindFactory( const char *pClassName ) = 0;
};

extern IClassMap& GetClassMap();

template< class T >
T *_CreateEntityTemplate( T *newEnt, const char *className );

extern void UTIL_Remove( C_BaseEntity *oldObj );

template <class T>
class CEntityFactory : public IEntityFactory
{
public:
	CEntityFactory( const char *pClassName )
	{
		GetClassMap().Add( pClassName, this );
	}

	C_BaseEntity *Create( const char *pClassName )
	{
		T* pEnt = _CreateEntityTemplate((T*)NULL, pClassName);
		if(!pEnt)
			return NULL;
		return pEnt;
	}

	void Destroy( C_BaseEntity *pNetworkable )
	{
		UTIL_Remove( pNetworkable );
	}

	virtual size_t GetEntitySize()
	{
		return sizeof(T);
	}
};

#endif // ICLASSMAP_H
