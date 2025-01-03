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

struct map_datamap_t;

abstract_class IEntityFactory
{
public:
	virtual C_BaseEntity *Create( const char *pClassName ) = 0;
	virtual void Destroy( C_BaseEntity *pNetworkable ) = 0;
	virtual size_t GetEntitySize() = 0;
	virtual const char *MapClassname() const = 0;
	virtual const char *DllClassname() const  = 0;
	virtual map_datamap_t *GetMapDataDesc() const = 0;
};

abstract_class IClassMap
{
public:
	virtual					~IClassMap() {}

	virtual void			Add( const char *mapname, IEntityFactory *factory ) = 0;
	virtual C_BaseEntity	*CreateEntity( const char *mapname ) = 0;
	virtual int				GetClassSize( const char *mapname ) = 0;
	virtual IEntityFactory *FindFactory( const char *pClassName ) = 0;
	virtual int GetFactoryCount() const = 0;
	virtual IEntityFactory *GetFactory( int idx ) = 0;

	virtual void			AddMapping( const char *mapname, const char *classname ) = 0;
	virtual char const		*LookupMapping( const char *classname ) = 0;
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
		: m_pClassname( pClassName )
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

	size_t GetEntitySize()
	{
		return sizeof(T);
	}

	const char *MapClassname() const
	{
		return m_pClassname;
	}

	map_datamap_t *GetMapDataDesc() const
	{
		return &T::m_MapDataDesc;
	}

private:
	const char *m_pClassname;
};

#endif // ICLASSMAP_H
