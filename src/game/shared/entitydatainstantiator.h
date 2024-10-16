//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYDATAINSTANTIATOR_H
#define ENTITYDATAINSTANTIATOR_H
#pragma once

#include "utlhash.h"

// This is the hash key type, but it could just as easily be and int or void *
#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
abstract_class IEntityDataInstantiator
{
public:
	virtual ~IEntityDataInstantiator() {};

	virtual void *GetDataObject( const CSharedBaseEntity *instance ) = 0;
	virtual void *CreateDataObject( const CSharedBaseEntity *instance ) = 0;
	virtual void DestroyDataObject( const CSharedBaseEntity *instance ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template <class T>
class CEntityDataInstantiator : public IEntityDataInstantiator
{
public:
	CEntityDataInstantiator() : 
		m_HashTable( 64, 0, 0, CompareFunc, KeyFunc )
	{
	}

	virtual void *GetDataObject( const CSharedBaseEntity *instance );

	virtual void *CreateDataObject( const CSharedBaseEntity *instance );

	virtual void DestroyDataObject( const CSharedBaseEntity *instance );

private:

	struct HashEntry
	{
		HashEntry()
		{
			key = NULL;
			data = NULL;
		}

		const CSharedBaseEntity *key;
		T				*data;
	};

	static bool CompareFunc( const HashEntry &src1, const HashEntry &src2 )
	{
		return ( src1.key == src2.key );
	}


	static unsigned int KeyFunc( const HashEntry &src )
	{
		// Shift right to get rid of alignment bits and border the struct on a 16 byte boundary
		return (unsigned int)src.key;
	}

	CUtlHash< HashEntry >	m_HashTable;
};

#include "tier0/memdbgon.h"

template <class T>
void *CEntityDataInstantiator<T>::GetDataObject( const CSharedBaseEntity *instance )
{
	UtlHashHandle_t handle; 
	HashEntry entry;
	entry.key = instance;
	handle = m_HashTable.Find( entry );

	if ( handle != m_HashTable.InvalidHandle() )
	{
		return (void *)m_HashTable[ handle ].data;
	}

	return NULL;
}

template <class T>
void *CEntityDataInstantiator<T>::CreateDataObject( const CSharedBaseEntity *instance )
{
	UtlHashHandle_t handle; 
	HashEntry entry;
	entry.key = instance;
	handle = m_HashTable.Find( entry );

	// Create it if not already present
	if ( handle == m_HashTable.InvalidHandle() )
	{
		handle = m_HashTable.Insert( entry );
		Assert( handle != m_HashTable.InvalidHandle() );
		m_HashTable[ handle ].data = new T;

		// FIXME: We'll have to remove this if any objects we instance have vtables!!!
		Q_memset( m_HashTable[ handle ].data, 0, sizeof( T ) );
	}

	return (void *)m_HashTable[ handle ].data;
}

template <class T>
void CEntityDataInstantiator<T>::DestroyDataObject( const CSharedBaseEntity *instance )
{
	UtlHashHandle_t handle; 
	HashEntry entry;
	entry.key = instance;
	handle = m_HashTable.Find( entry );

	if ( handle != m_HashTable.InvalidHandle() )
	{
		delete m_HashTable[ handle ].data;
		m_HashTable.Remove( handle );
	}
}

#include "tier0/memdbgoff.h"

#endif // ENTITYDATAINSTANTIATOR_H
