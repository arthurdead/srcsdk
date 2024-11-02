//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTABLE_ENTITY_H
#define PREDICTABLE_ENTITY_H
#pragma once

// For introspection
#include "tier0/platform.h"
#include "predictioncopy.h"
#include "shared_classnames.h"

// CLIENT DLL includes
#if defined( CLIENT_DLL )

#include "recvproxy.h"

// Game DLL includes
#else

#include "sendproxy.h"

#endif  // !CLIENT_DLL

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

#if defined( CLIENT_DLL )

#define DECLARE_NETWORKCLASS()											\
		DECLARE_CLIENTCLASS()

#define DECLARE_NETWORKCLASS_NOBASE()									\
		DECLARE_CLIENTCLASS_NOBASE()							

#else

#define DECLARE_NETWORKCLASS()											\
		DECLARE_SERVERCLASS()

#define DECLARE_NETWORKCLASS_NOBASE()									\
		DECLARE_SERVERCLASS_NOBASE()	

#endif

#if defined( CLIENT_DLL )

#define DECLARE_PREDICTABLE()											\
	public:																\
		static typedescription_t m_PredDesc[];							\
		static pred_datamap_t m_PredMap;										\
		virtual datamap_t *GetPredDescMap( void );						\
		template <typename T> friend datamap_t *PredMapInit(T *)

#define BEGIN_PREDICTION_DATA( className ) \
	pred_datamap_t className::m_PredMap( V_STRINGIFY(className), &BaseClass::m_PredMap ); \
	datamap_t *className::GetPredDescMap( void ) { return &m_PredMap; } \
	BEGIN_PREDICTION_DATA_GUTS( className )

#define BEGIN_PREDICTION_DATA_NO_BASE( className ) \
	pred_datamap_t className::m_PredMap( V_STRINGIFY(className), NULL ); \
	datamap_t *className::GetPredDescMap( void ) { return &m_PredMap; } \
	BEGIN_PREDICTION_DATA_GUTS( className )

#define BEGIN_PREDICTION_DATA_GUTS( className ) \
	template <typename T> datamap_t *PredMapInit(T *); \
	template <> datamap_t *PredMapInit<className>( className * ); \
	namespace V_CONCAT2(className, _PredDataDescInit) \
	{ \
		datamap_t *g_PredMapHolder = PredMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *PredMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static typedescription_t predDesc[] = \
		{ \
		{ FIELD_VOID,0, {0,0},0,0,0,0,0,0}, /* so you can define "empty" tables */

#define END_PREDICTION_DATA() \
		}; \
		\
		if ( sizeof( predDesc ) > sizeof( predDesc[0] ) ) \
		{ \
			classNameTypedef::m_PredMap.dataNumFields = ARRAYSIZE( predDesc ) - 1; \
			classNameTypedef::m_PredMap.dataDesc 	  = &predDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_PredMap.dataNumFields = 1; \
			classNameTypedef::m_PredMap.dataDesc 	  = predDesc; \
		} \
		return &classNameTypedef::m_PredMap; \
	}

#else

	// nothing, only client has a prediction system
	#define DECLARE_PREDICTABLE()	
	#define BEGIN_PREDICTION_DATA( className ) 
	#define END_PREDICTION_DATA() 

#endif

#if defined( CLIENT_DLL )

// On the client .dll this creates a mapping between a classname and
//  a client side class.  Probably could be templatized at some point.

#define BEGIN_NETWORK_TABLE( className, tableName ) BEGIN_RECV_TABLE( className, tableName )
#define BEGIN_NETWORK_TABLE_NOBASE( className, tableName ) BEGIN_RECV_TABLE_NOBASE( className, tableName )

#define END_NETWORK_TABLE	END_RECV_TABLE

#define LINK_ENTITY_TO_SERVERCLASS( localName, className )						\
	class localName##Mapping													\
	{																		\
	public:																	\
		localName##Mapping( void )											\
		{																	\
			GetClassMap().AddMapping( V_STRINGIFY(localName), V_STRINGIFY(className) );			\
		}																	\
	};																		\
	static localName##Mapping g_##localName##Mapping;

#define LINK_ENTITY_TO_CLASS_ALIASED( localName, className ) \
	LINK_ENTITY_TO_SERVERCLASS( localName, C##className ) \
	LINK_ENTITY_TO_CLASS( localName##_clientside, C_##className )

#define IMPLEMENT_NETWORKCLASS_ALIASED(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS( C_##className, dataTable, C##className )
#define IMPLEMENT_NETWORKCLASS(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS(className, dataTable, className)
#define IMPLEMENT_NETWORKCLASS_DT(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS_DT(className, dataTable, className)

#else

#define BEGIN_NETWORK_TABLE( className, tableName ) BEGIN_SEND_TABLE( className, tableName )
#define BEGIN_NETWORK_TABLE_NOBASE( className, tableName ) BEGIN_SEND_TABLE_NOBASE( className, tableName )

#define END_NETWORK_TABLE	END_SEND_TABLE

#define LINK_ENTITY_TO_CLASS_ALIASED( localName, className ) LINK_ENTITY_TO_CLASS( localName, C##className )
#define LINK_ENTITY_TO_SERVERCLASS( localName, className ) LINK_ENTITY_TO_CLASS( localName, className )

#define IMPLEMENT_NETWORKCLASS_ALIASED(className, dataTable)			\
	IMPLEMENT_SERVERCLASS( C##className, dataTable )
#define IMPLEMENT_NETWORKCLASS(className, dataTable)			\
	IMPLEMENT_SERVERCLASS(className, dataTable)
#define IMPLEMENT_NETWORKCLASS_DT(className, dataTable)			\
	IMPLEMENT_SERVERCLASS_ST(className, dataTable)

#endif																	

// Interface used by client and server to track predictable entities
abstract_class IPredictableList
{
public:
	// Get predictables by index
	virtual CSharedBaseEntity		*GetPredictable( int slot ) = 0;
	// Get count of predictables
	virtual int				GetPredictableCount( void ) const = 0;
};

// Expose interface to rest of .dll
extern IPredictableList *predictables;

#endif // PREDICTABLE_ENTITY_H
