#ifndef MAP_ENTITY_H
#define MAP_ENTITY_H

#pragma once

#include "datamap.h"

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

#define DECLARE_MAPENTITY()											\
	public:																\
		static typedescription_t m_MapTypeDesc[];							\
		static datamap_t m_MapDataDesc;										\
		virtual datamap_t *GetMapDataDesc( void );						\
		template <typename T> friend datamap_t *MapDataDescInit(T *)

#define DECLARE_SIMPLE_MAPEMBEDDED()											\
	public:																\
		static typedescription_t m_MapTypeDesc[];							\
		static datamap_t m_MapDataDesc;										\
		template <typename T> friend datamap_t *MapDataDescInit(T *)

#define BEGIN_MAPENTITY( className ) \
	datamap_t className::m_MapDataDesc = { 0, 0, #className, &BaseClass::m_MapDataDesc }; \
	datamap_t *className::GetMapDataDesc( void ) { return &m_MapDataDesc; } \
	BEGIN_MAPENTITY_GUTS( className )

#define BEGIN_MAPENTITY_NO_BASE( className ) \
	datamap_t className::m_MapDataDesc = { 0, 0, #className, NULL }; \
	datamap_t *className::GetMapDataDesc( void ) { return &m_MapDataDesc; } \
	BEGIN_MAPENTITY_GUTS( className )

#define BEGIN_SIMPLE_MAPEMBEDDED( className ) \
	datamap_t className::m_MapDataDesc = { 0, 0, #className, NULL }; \
	BEGIN_MAPENTITY_GUTS( className )

#define BEGIN_MAPENTITY_GUTS( className ) \
	template <typename T> datamap_t *MapDataDescInit(T *); \
	template <> datamap_t *MapDataDescInit<className>( className * ); \
	namespace className##_MapDataDescInit \
	{ \
		datamap_t *g_MapDataDescHolder = MapDataDescInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *MapDataDescInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static typedescription_t mapTypeDesc[] = \
		{ \
		{ FIELD_VOID,0, {0,0},0,0,0,0,0,0}, /* so you can define "empty" tables */

#define END_MAPENTITY() \
		}; \
		\
		if ( sizeof( mapTypeDesc ) > sizeof( mapTypeDesc[0] ) ) \
		{ \
			classNameTypedef::m_MapDataDesc.dataNumFields = ARRAYSIZE( mapTypeDesc ) - 1; \
			classNameTypedef::m_MapDataDesc.dataDesc 	  = &mapTypeDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_MapDataDesc.dataNumFields = 1; \
			classNameTypedef::m_MapDataDesc.dataDesc 	  = mapTypeDesc; \
		} \
		return &classNameTypedef::m_MapDataDesc; \
	}

#define DECLARE_MAPEMBEDDED() DECLARE_MAPENTITY()
#define BEGIN_MAPEMBEDDED_NO_BASE BEGIN_MAPENTITY_NO_BASE
#define END_MAPEMBEDDED END_MAPENTITY

#endif
