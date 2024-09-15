//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//===========================================================================//

#if !defined( CLIENTLEAFSYSTEM_H )
#define CLIENTLEAFSYSTEM_H
#pragma once

#include "igamesystem.h"
#include "engine/IClientLeafSystem.h"
#include "cdll_int.h"
#include "ivrenderview.h"
#include "tier1/mempool.h"
#include "tier1/refcount.h"
#include "iclientrenderable.h"
#include "engine/ivmodelinfo.h"

//-----------------------------------------------------------------------------
// Render groups
//-----------------------------------------------------------------------------
enum ClientRenderGroup_t
{
	CLIENT_RENDER_GROUP_OPAQUE = 0,
	CLIENT_RENDER_GROUP_TRANSLUCENT,
	CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ,
	CLIENT_RENDER_GROUP_COUNT, // Indicates the groups above are real and used for bucketing a scene
};

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct WorldListInfo_t;
class IClientRenderable;
class Vector;
class CGameTrace;
typedef CGameTrace trace_t;
struct Ray_t;
class Vector2D;
class CStaticProp;


//-----------------------------------------------------------------------------
// Handle to an renderables in the client leaf system
//-----------------------------------------------------------------------------
inline const ClientRenderHandle_t DETAIL_PROP_RENDER_HANDLE = (ClientRenderHandle_t)0xfffe;

//-----------------------------------------------------------------------------
// Distance fade information
//-----------------------------------------------------------------------------
struct DistanceFadeInfo_t
{
	float m_flMaxDistSqr;		// distance at which everything is faded out
	float m_flMinDistSqr;		// distance at which everything is unfaded
	float m_flFalloffFactor;	// 1.0f / ( maxDistSqr - MinDistSqr )
								// opacity = ( maxDist - distSqr ) * falloffFactor 
};

class CClientRenderablesList : public CRefCounted<>
{
	DECLARE_FIXEDSIZE_ALLOCATOR( CClientRenderablesList );

public:
	enum
	{
		MAX_GROUP_ENTITIES = 16834,
		MAX_BONE_SETUP_DEPENDENCY = 64,
	};

	enum
	{
		GROUP_OPAQUE_STATIC_BUCKET_ROOT = ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1 ) * 2,
		GROUP_OPAQUE_ENTITY_BUCKET_ROOT = ( GROUP_OPAQUE_STATIC_BUCKET_ROOT + 1),

		GROUP_OPAQUE_BEGIN = 0,
		GROUP_OPAQUE_END = ( GROUP_OPAQUE_ENTITY_BUCKET_ROOT+1 ),

		GROUP_OPAQUE_STATIC_HUGE = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_STATIC_BUCKET_ROOT, RENDER_GROUP_BUCKET_HUGE ),
		GROUP_OPAQUE_STATIC_MEDIUM = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_STATIC_BUCKET_ROOT, RENDER_GROUP_BUCKET_MEDIUM ),
		GROUP_OPAQUE_STATIC_SMALL = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_STATIC_BUCKET_ROOT, RENDER_GROUP_BUCKET_SMALL ),
		GROUP_OPAQUE_STATIC_TINY = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_STATIC_BUCKET_ROOT, RENDER_GROUP_BUCKET_TINY ),

		GROUP_OPAQUE_ENTITY_HUGE = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_ENTITY_BUCKET_ROOT, RENDER_GROUP_BUCKET_HUGE ),
		GROUP_OPAQUE_ENTITY_MEDIUM = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_ENTITY_BUCKET_ROOT, RENDER_GROUP_BUCKET_MEDIUM ),
		GROUP_OPAQUE_ENTITY_SMALL = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_ENTITY_BUCKET_ROOT, RENDER_GROUP_BUCKET_SMALL ),
		GROUP_OPAQUE_ENTITY_TINY = RENDER_GROUP_OPAQUE_BUCKETED( GROUP_OPAQUE_ENTITY_BUCKET_ROOT, RENDER_GROUP_BUCKET_TINY ),

		GROUP_OPAQUE_STATIC = GROUP_OPAQUE_STATIC_BUCKET_ROOT,
		GROUP_OPAQUE_ENTITY = GROUP_OPAQUE_ENTITY_BUCKET_ROOT,

		GROUP_OPAQUE = GROUP_OPAQUE_ENTITY,

		GROUP_TRANSLUCENT_ENTITY = GROUP_OPAQUE_END,

		GROUP_OPAQUE_BRUSH,					// Brushes

		// This one's always gotta be last
		GROUP_COUNT
	};

	struct CEntry
	{
		IClientRenderable	*m_pRenderable;
		IClientRenderableMod	*m_pRenderableMod;
		unsigned short		m_iWorldListInfoLeaf; // NOTE: this indexes WorldListInfo_t's leaf list.
		RenderableInstance_t m_InstanceData;
		uint8		m_TwoPass : 1;
		uint8				m_nModelType : 7;		// See RenderableModelType_t
		ClientRenderHandle_t m_RenderHandle;
	};

	// The leaves for the entries are in the order of the leaves you call CollateRenderablesInLeaf in.
	DistanceFadeInfo_t	m_DetailFade;
	CEntry		m_RenderGroups[GROUP_COUNT][MAX_GROUP_ENTITIES];
	int			m_RenderGroupCounts[GROUP_COUNT];
	int					m_nBoneSetupDependencyCount;
	IClientRenderable	*m_pBoneSetupDependency[MAX_BONE_SETUP_DEPENDENCY];
};

//-----------------------------------------------------------------------------
// Render list for viewmodels
//-----------------------------------------------------------------------------
class CViewModelRenderablesList
{
public:
	enum
	{
		GROUP_OPAQUE = 0,
		GROUP_TRANSLUCENT,
		GROUP_COUNT,
	};

	struct CEntry
	{
		IClientRenderable	*m_pRenderable;
		IClientRenderableMod	*m_pRenderableMod;
		RenderableInstance_t m_InstanceData;
	};

	typedef CUtlVectorFixedGrowable< CEntry, 32 > RenderGroups_t;

	// The leaves for the entries are in the order of the leaves you call CollateRenderablesInLeaf in.
	RenderGroups_t	m_RenderGroups[GROUP_COUNT];
};

//-----------------------------------------------------------------------------
// Used by CollateRenderablesInLeaf
//-----------------------------------------------------------------------------
struct SetupRenderInfo_t
{
	WorldListInfo_t *m_pWorldListInfo;
	CClientRenderablesList *m_pRenderList;
	Vector m_vecRenderOrigin;
	Vector m_vecRenderForward;
	int m_nRenderFrame;
	int m_nDetailBuildFrame;	// The "render frame" for detail objects
	float m_flRenderDistSq;
	int m_nViewID;
	bool m_bDrawDetailObjects : 1;
	bool m_bDrawTranslucentObjects : 1;
	bool m_bFastEntityRendering : 1;
	bool m_bDrawDepthViewNonCachedObjectsOnly : 1;

	SetupRenderInfo_t()
	{
		m_bDrawDetailObjects = true;
		m_bDrawTranslucentObjects = true;
		m_bFastEntityRendering = false;
		m_bDrawDepthViewNonCachedObjectsOnly = false;
	}
};

//-----------------------------------------------------------------------------
// Used to do batched screen size computations
//-----------------------------------------------------------------------------
struct ScreenSizeComputeInfo_t
{
	VMatrix m_matViewProj;
	Vector m_vecViewUp;
	int m_nViewportHeight;
};

void ComputeScreenSizeInfo( ScreenSizeComputeInfo_t *pInfo );
float ComputeScreenSize( const Vector &vecOrigin, float flRadius, const ScreenSizeComputeInfo_t& info );

//-----------------------------------------------------------------------------
// A handle associated with shadows managed by the client leaf system
//-----------------------------------------------------------------------------
enum class ClientLeafShadowHandle_t : unsigned short
{
};
inline const ClientLeafShadowHandle_t CLIENT_LEAF_SHADOW_INVALID_HANDLE = (ClientLeafShadowHandle_t)~0;


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafShadowEnum
{
public:
	// The user ID is the id passed into CreateShadow
	virtual void EnumShadow( ClientShadowHandle_t userId ) = 0;
};


// subclassed by things which wish to add per-leaf data managed by the client leafsystem
class CClientLeafSubSystemData
{
public:
	virtual ~CClientLeafSubSystemData( void )
	{
	}
};


// defines for subsystem ids. each subsystem id uses up one pointer in each leaf
#define CLSUBSYSTEM_DETAILOBJECTS 0
#define N_CLSUBSYSTEMS 1



//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafSystem : public IClientLeafSystemEngineEx, public IGameSystemPerFrame
{
public:
	// Adds and removes renderables from the leaf lists
	virtual void AddRenderable( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType ) = 0;

	// This tells if the renderable is in the current PVS. It assumes you've updated the renderable
	// with RenderableChanged() calls
	virtual bool IsRenderableInPVS( IClientRenderable *pRenderable ) = 0;

	virtual void SetSubSystemDataInLeaf( int leaf, int nSubSystemIdx, CClientLeafSubSystemData *pData ) =0;
	virtual CClientLeafSubSystemData *GetSubSystemDataInLeaf( int leaf, int nSubSystemIdx ) =0;

	virtual void SetDetailObjectsInLeaf( int leaf, int firstDetailObject, int detailObjectCount ) = 0;
	virtual void GetDetailObjectsInLeaf( int leaf, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Indicates which leaves detail objects should be rendered from, returns the detais objects in the leaf
	virtual void DrawDetailObjectsInLeaf( int leaf, int frameNumber, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Should we draw detail objects (sprites or models) in this leaf (because it's close enough to the view)
	// *and* are there any objects in the leaf?
	virtual bool ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber ) = 0;

	// Call this when a renderable origin/angles/bbox parameters has changed
	virtual void RenderableChanged( ClientRenderHandle_t handle ) = 0;

	// Computes which leaf translucent objects should be rendered in
	virtual void ComputeTranslucentRenderLeaf( int count, const LeafIndex_t *pLeafList, const LeafFogVolume_t *pLeafFogVolumeList, int frameNumber, int viewID ) = 0;

	// Put renderables into their appropriate lists.
	virtual void BuildRenderablesList( const SetupRenderInfo_t &info ) = 0;

	// Put renderables in the leaf into their appropriate lists.
	virtual void CollateViewModelRenderables( CViewModelRenderablesList *pList ) = 0;

	// Call this to deactivate static prop rendering..
	virtual void DrawStaticProps( bool enable ) = 0;

	// Call this to deactivate small object rendering
	virtual void DrawSmallEntities( bool enable ) = 0;

	// The following methods are related to shadows...
	virtual ClientLeafShadowHandle_t AddShadow( ClientShadowHandle_t userId, unsigned short flags ) = 0;
	virtual void RemoveShadow( ClientLeafShadowHandle_t h ) = 0;

	// Project a shadow
	virtual void ProjectShadow( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList ) = 0;

	// Project a projected texture spotlight
	virtual void ProjectFlashlight( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList ) = 0;

	// Find all shadow casters in a set of leaves
	virtual void EnumerateShadowsInLeaves( int leafCount, LeafIndex_t* pLeaves, IClientLeafShadowEnum* pEnum ) = 0;

	// Fill in a list of the leaves this renderable is in.
	// Returns -1 if the handle is invalid.
	virtual int GetRenderableLeaves( ClientRenderHandle_t handle, int leaves[128] ) = 0;

	// Get leaves this renderable is in
	virtual bool GetRenderableLeaf ( ClientRenderHandle_t handle, int* pOutLeaf, const int* pInIterator = 0, int* pOutIterator = 0 ) = 0;

	// Use alternate translucent sorting algorithm (draw translucent objects in the furthest leaf they lie in)
	virtual void EnableAlternateSorting( ClientRenderHandle_t handle, bool bEnable ) = 0;

	// Mark this as rendering with viewmodels
	virtual void RenderWithViewModels( ClientRenderHandle_t handle, bool bEnable ) = 0;
	virtual bool IsRenderingWithViewModels( ClientRenderHandle_t handle ) const = 0;

	// Call this if the model changes
	virtual void SetTranslucencyType( ClientRenderHandle_t handle, RenderableTranslucencyType_t nType ) = 0;
	virtual RenderableTranslucencyType_t GetTranslucencyType( ClientRenderHandle_t handle ) const = 0;
	virtual void SetModelType( ClientRenderHandle_t handle, RenderableModelType_t nType = RENDERABLE_MODEL_UNKNOWN_TYPE ) = 0;

	// Suppress rendering of this renderable
	virtual void EnableRendering( ClientRenderHandle_t handle, bool bEnable ) = 0;

	// Indicates this renderable should bloat its client leaf bounds over time
	// used for renderables with oscillating bounds to reduce the cost of
	// them reinserting themselves into the tree over and over.
	virtual void EnableBloatedBounds( ClientRenderHandle_t handle, bool bEnable ) = 0;

	// Indicates this renderable should always recompute its bounds accurately
	virtual void DisableCachedRenderBounds( ClientRenderHandle_t handle, bool bDisable ) = 0;

	// Recomputes which leaves renderables are in
	virtual void RecomputeRenderableLeaves() = 0;

	// Warns about leaf reinsertion
	virtual void DisableLeafReinsertion( bool bDisable ) = 0;

	//Assuming the renderable would be in a properly built render list, generate a render list entry
	virtual EngineRenderGroup_t GenerateRenderListEntry( IClientRenderable *pRenderable, CClientRenderablesList::CEntry &entryOut ) = 0; 

	// Enable/disable rendering into the shadow depth buffer
	virtual void DisableShadowDepthRendering(ClientRenderHandle_t handle, bool bDisable) = 0;

	// Enable/disable caching in the shadow depth buffer
	virtual void DisableShadowDepthCaching(ClientRenderHandle_t handle, bool bDisable) = 0;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
IClientLeafSystem* ClientLeafSystem();


#endif	// CLIENTLEAFSYSTEM_H


