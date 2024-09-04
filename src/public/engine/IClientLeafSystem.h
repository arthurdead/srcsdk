//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================//

#if !defined( ICLIENTLEAFSYSTEM_H )
#define ICLIENTLEAFSYSTEM_H
#pragma once

#include "tier0/platform.h"
#include "client_render_handle.h"
#include "ivmodelinfo.h"
#include "iclientrenderable.h"

//-----------------------------------------------------------------------------
// Render groups
//-----------------------------------------------------------------------------
enum RenderGroup_Config_t
{
	// Number of buckets that are used to hold opaque entities
	// and opaque static props by size. The bucketing should be used to reduce overdraw.
	RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS	= 4,
};

enum RenderableModelType_t
{
	RENDERABLE_MODEL_UNKNOWN_TYPE = -1,
	RENDERABLE_MODEL_ENTITY = 0,
	RENDERABLE_MODEL_STUDIOMDL,
	RENDERABLE_MODEL_STATIC_PROP,
	RENDERABLE_MODEL_BRUSH,
};

COMPILE_TIME_ASSERT(RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS == 4);

enum RenderGroupBucket_t
{
	RENDER_GROUP_BUCKET_HUGE = 0,
	RENDER_GROUP_BUCKET_MEDIUM = 1,
	RENDER_GROUP_BUCKET_SMALL = 2,
	RENDER_GROUP_BUCKET_TINY = 3,
};

#define RENDER_GROUP_OPAQUE_BUCKETED( root, bucket ) \
	( root - ( ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1 ) - bucket ) * 2 )
#define RENDER_GROUP_OPAQUE_BUCKETED_STATIC( bucket ) \
	RENDER_GROUP_OPAQUE_BUCKETED( ENGINE_RENDER_GROUP_OPAQUE_STATIC_BUCKET_ROOT, bucket )
#define RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( bucket ) \
	RENDER_GROUP_OPAQUE_BUCKETED( ENGINE_RENDER_GROUP_OPAQUE_ENTITY_BUCKET_ROOT, bucket )

enum EngineRenderGroup_t
{
	ENGINE_RENDER_GROUP_OPAQUE_STATIC_BUCKET_ROOT = ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1 ) * 2,
	ENGINE_RENDER_GROUP_OPAQUE_ENTITY_BUCKET_ROOT = ( ENGINE_RENDER_GROUP_OPAQUE_STATIC_BUCKET_ROOT + 1),

	ENGINE_RENDER_GROUP_OPAQUE_BEGIN = 0,
	ENGINE_RENDER_GROUP_OPAQUE_END = ( ENGINE_RENDER_GROUP_OPAQUE_ENTITY_BUCKET_ROOT+1 ),

	ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE = RENDER_GROUP_OPAQUE_BUCKETED_STATIC( RENDER_GROUP_BUCKET_HUGE ),
	ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM = RENDER_GROUP_OPAQUE_BUCKETED_STATIC( RENDER_GROUP_BUCKET_MEDIUM ),
	ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL = RENDER_GROUP_OPAQUE_BUCKETED_STATIC( RENDER_GROUP_BUCKET_SMALL ),
	ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY = RENDER_GROUP_OPAQUE_BUCKETED_STATIC( RENDER_GROUP_BUCKET_TINY ),

	ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE = RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( RENDER_GROUP_BUCKET_HUGE ),
	ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM = RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( RENDER_GROUP_BUCKET_MEDIUM ),
	ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL = RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( RENDER_GROUP_BUCKET_SMALL ),
	ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY = RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( RENDER_GROUP_BUCKET_TINY ),

	ENGINE_RENDER_GROUP_OPAQUE_STATIC = ENGINE_RENDER_GROUP_OPAQUE_STATIC_BUCKET_ROOT,
	ENGINE_RENDER_GROUP_OPAQUE_ENTITY = ENGINE_RENDER_GROUP_OPAQUE_ENTITY_BUCKET_ROOT,

	ENGINE_RENDER_GROUP_OPAQUE = ENGINE_RENDER_GROUP_OPAQUE_ENTITY,

	ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY = ENGINE_RENDER_GROUP_OPAQUE_END,
	ENGINE_RENDER_GROUP_TWOPASS,						// Implied opaque and translucent in two passes
	ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE,				// Solid weapon view models
	ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT,		// Transparent overlays etc

	ENGINE_RENDER_GROUP_OPAQUE_BRUSH,					// Brushes

	ENGINE_RENDER_GROUP_OTHER,							// Unclassfied. Won't get drawn.

	// This one's always gotta be last
	ENGINE_RENDER_GROUP_COUNT
};

inline bool IsEngineRenderGroupOpaqueEntity(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupEntity(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
	case ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupOpaqueStatic(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupStatic(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupOpaque(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
	case ENGINE_RENDER_GROUP_OPAQUE_BRUSH:
	case ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupViewModel(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT:
	case ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE:
		return true;
	default:
		return false;
	}
}

inline bool IsEngineRenderGroupTranslucent(EngineRenderGroup_t group)
{
	switch(group) {
	case ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY:
	case ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT:
		return true;
	default:
		return false;
	}
}

#define CLIENTLEAFSYSTEM_INTERFACE_VERSION_1 "ClientLeafSystem001"
#define CLIENTLEAFSYSTEM_INTERFACE_VERSION	"ClientLeafSystem002"


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafSystemEngine
{
private:
	// Adds and removes renderables from the leaf lists
	// CreateRenderableHandle stores the handle inside pRenderable.
	virtual void DO_NOT_USE_CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp ) = 0;
public:
	virtual void RemoveRenderable( ClientRenderHandle_t handle ) = 0;
	virtual void AddRenderableToLeaves( ClientRenderHandle_t renderable, int nLeafCount, unsigned short *pLeaves ) = 0;
private:
	virtual void DO_NOT_USE_ChangeRenderableRenderGroup( ClientRenderHandle_t handle, EngineRenderGroup_t group ) = 0;
};

abstract_class IClientLeafSystemEngineEx : public IClientLeafSystemEngine
{
private:
	virtual void DO_NOT_USE_CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp ) override final
	{ CreateRenderableHandle(pRenderable, false, RENDERABLE_IS_OPAQUE, bIsStaticProp ? RENDERABLE_MODEL_STATIC_PROP : RENDERABLE_MODEL_UNKNOWN_TYPE); }
public:
	virtual void CreateRenderableHandle( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType ) = 0;
	virtual void SetTranslucencyType( ClientRenderHandle_t handle, RenderableTranslucencyType_t nType ) = 0;
};


#endif	// ICLIENTLEAFSYSTEM_H


