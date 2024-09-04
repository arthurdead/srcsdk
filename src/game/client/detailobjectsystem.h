//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#if !defined( DETAILOBJECTSYSTEM_H )
#define DETAILOBJECTSYSTEM_H
#pragma once

#include "igamesystem.h"
#include "icliententityinternal.h"
#include "engine/ivmodelrender.h"
#include "mathlib/vector.h"
#include "ivrenderview.h"

struct model_t;
#if 0
struct WorldListLeafData_t;	
#endif
struct DistanceFadeInfo_t;


//-----------------------------------------------------------------------------
// Info used when building lists of detail objects to render
//-----------------------------------------------------------------------------
struct DetailRenderableInfo_t
{
	IClientRenderable *m_pRenderable;
	IClientRenderableMod *m_pRenderableMod;
	int m_nLeafIndex;
	EngineRenderGroup_t m_nEngineRenderGroup;
	RenderableInstance_t m_InstanceData;
};


typedef CUtlVectorFixedGrowable< DetailRenderableInfo_t, 2048 > DetailRenderableList_t;

class IDetailModel : public IClientUnknownEx, public IClientRenderableEx
{
public:
	virtual IClientEntity*		GetIClientEntity()		{ return NULL; }
	virtual IClientEntityMod*		GetIClientEntityMod() { return NULL; }

	virtual IClientUnknown*	GetIClientUnknown() { return this; }
	virtual IClientUnknownMod*	GetIClientUnknownMod() { return this; }

	virtual IClientRenderable*	GetClientRenderable() { return this; }
	virtual IClientRenderableMod*	GetClientRenderableMod() { return this; }

	virtual IClientAlphaProperty*	GetClientAlphaProperty() { return NULL; }
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod() { return NULL; }
};

//-----------------------------------------------------------------------------
// Responsible for managing detail objects
//-----------------------------------------------------------------------------
abstract_class IDetailObjectSystem : public IGameSystem
{
public:
	// How many detail models (as opposed to sprites) are there in the level?
	virtual int GetDetailModelCount() const = 0;

    // Gets a particular detail object
	virtual IDetailModel* GetDetailModel( int idx ) = 0;

	// Computes the detail prop fade info
	virtual float ComputeDetailFadeInfo( DistanceFadeInfo_t *pInfo ) = 0;

	// Gets called each view
	virtual void BuildDetailObjectRenderLists( const Vector &vViewOrigin ) = 0;

	// Builds a list of renderable info for all detail objects to render
	virtual void BuildRenderingData( DetailRenderableList_t &list, const SetupRenderInfo_t &info, float flDetailDist, const DistanceFadeInfo_t &fadeInfo ) = 0;

	// Renders all opaque detail objects in a particular set of leaves
	virtual void RenderOpaqueDetailObjects( int nLeafCount, LeafIndex_t *pLeafList ) = 0;

	// Call this before rendering translucent detail objects
	virtual void BeginTranslucentDetailRendering( ) = 0;

	// Renders all translucent detail objects in a particular set of leaves
	virtual void RenderTranslucentDetailObjects( const DistanceFadeInfo_t &info, const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeafCount, LeafIndex_t *pLeafList ) =0;

	// Renders all translucent detail objects in a particular leaf up to a particular point
	virtual void RenderTranslucentDetailObjectsInLeaf( const DistanceFadeInfo_t &info, const Vector &viewOrigin, const Vector &viewForward, const Vector &viewRight, const Vector &viewUp, int nLeaf, const Vector *pVecClosestPoint ) = 0;
};


//-----------------------------------------------------------------------------
// System for dealing with detail objects
//-----------------------------------------------------------------------------
extern IDetailObjectSystem* DetailObjectSystem();


#endif // DETAILOBJECTSYSTEM_H

