#ifndef MAP_ENTITY_H
#define MAP_ENTITY_H

#pragma once

#include "datamap.h"
#include "string_t.h"
#include "tier1/utldict.h"

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

#define DECLARE_MAPENTITY()											\
	public:																\
		static typedescription_t m_MapTypeDesc[];							\
		static map_datamap_t m_MapDataDesc;										\
		virtual datamap_t *GetMapDataDesc( void );						\
		template <typename T> friend datamap_t *MapDataDescInit(T *)

#define DECLARE_SIMPLE_MAPEMBEDDED()											\
	public:																\
		static typedescription_t m_MapTypeDesc[];							\
		static map_datamap_t m_MapDataDesc;										\
		template <typename T> friend datamap_t *MapDataDescInit(T *)

#define BEGIN_MAPENTITY( className ) \
	map_datamap_t className::m_MapDataDesc( V_STRINGIFY(className), &BaseClass::m_MapDataDesc ); \
	datamap_t *className::GetMapDataDesc( void ) { return &m_MapDataDesc; } \
	BEGIN_MAPENTITY_GUTS( className )

#define BEGIN_MAPENTITY_NO_BASE( className ) \
	map_datamap_t className::m_MapDataDesc( V_STRINGIFY(className), NULL ); \
	datamap_t *className::GetMapDataDesc( void ) { return &m_MapDataDesc; } \
	BEGIN_MAPENTITY_GUTS( className )

#define BEGIN_SIMPLE_MAPEMBEDDED( className ) \
	map_datamap_t className::m_MapDataDesc( V_STRINGIFY(className), NULL ); \
	BEGIN_MAPENTITY_GUTS( className )

#ifdef GAME_DLL
#define BEGIN_MAPENTITY_ALIASED( className ) BEGIN_MAPENTITY( C##className )

#define BEGIN_MAPENTITY_ALIASED_NO_BASE( className ) BEGIN_MAPENTITY_NO_BASE( C##className )

#define BEGIN_SIMPLE_MAPEMBEDDED_ALIASED( className ) BEGIN_SIMPLE_MAPEMBEDDED( C##className )
#else
#define BEGIN_MAPENTITY_ALIASED( className ) BEGIN_MAPENTITY( C_##className )

#define BEGIN_MAPENTITY_ALIASED_NO_BASE( className ) BEGIN_MAPENTITY_NO_BASE( C_##className )

#define BEGIN_SIMPLE_MAPEMBEDDED_ALIASED( className ) BEGIN_SIMPLE_MAPEMBEDDED( C_##className )
#endif

#define BEGIN_MAPENTITY_GUTS( className ) \
	template <typename T> datamap_t *MapDataDescInit(T *); \
	template <> datamap_t *MapDataDescInit<className>( className * ); \
	namespace V_CONCAT2(className, _MapDataDescInit) \
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

#define DECLARE_MAPKVPARSER() DECLARE_MAPENTITY()
#define BEGIN_MAPKVPARSER_NO_BASE BEGIN_MAPENTITY_NO_BASE
#define END_MAPKVPARSER END_MAPENTITY

class CEntityMapData;

class CMapKVParser
{
public:
	DECLARE_MAPKVPARSER();

	CMapKVParser( const char *classname );

	virtual void ParseMapData( CEntityMapData *mapData );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void OnParseMapDataFinished();

	static CMapKVParser *Find( const char *classname );

private:
	string_t m_iClassname;

	static CUtlDict<CMapKVParser *, unsigned short> s_Parsers;
};

#define LINK_ENTITY_TO_KVPARSER(mapClassName,DLLClassName) \
	class C##mapClassName##Factory : public DLLClassName \
	{ \
		typedef DLLClassName BaseClass; \
	public: \
		C##mapClassName##Factory( const char *pClassName ) \
			: BaseClass( pClassName ) \
		{ \
		} \
	}; \
	INIT_PRIORITY(65535) C##mapClassName##Factory g_##mapClassName##Factory( #mapClassName );

#endif
