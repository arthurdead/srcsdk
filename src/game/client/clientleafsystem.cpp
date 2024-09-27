//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//===========================================================================//

#include "cbase.h"
#include "clientleafsystem.h"
#include "utlbidirectionalset.h"
#include "model_types.h"
#include "ivrenderview.h"
#include "tier0/vprof.h"
#include "bsptreedata.h"
#include "detailobjectsystem.h"
#include "engine/IStaticPropMgr.h"
#include "engine/ivdebugoverlay.h"
#include "vstdlib/jobthread.h"
#include "tier1/utllinkedlist.h"
#include "datacache/imdlcache.h"
#include "view.h"
#include "iviewrender.h"
#include "viewrender.h"
#include "clientalphaproperty.h"
#include "con_nprint.h"
//#include "tier0/miniprofiler.h" 

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class VMatrix;  // forward decl

static ConVar cl_drawleaf("cl_drawleaf", "-1", FCVAR_CHEAT );
static ConVar r_PortalTestEnts( "r_PortalTestEnts", "1", FCVAR_CHEAT, "Clip entities against portal frustums." );
static ConVar r_portalsopenall( "r_portalsopenall", "0", FCVAR_CHEAT, "Open all portals" );
static ConVar cl_threaded_client_leaf_system("cl_threaded_client_leaf_system", "1"  );
static ConVar r_shadows_on_renderables_enable( "r_shadows_on_renderables_enable", "0", 0, "Support casting RTT shadows onto other renderables" );

static ConVar r_drawallrenderables( "r_drawallrenderables", "0", FCVAR_CHEAT, "Draw all renderables, even ones inside solid leaves." );
static ConVar cl_leafsystemvis( "cl_leafsystemvis", "0", FCVAR_CHEAT );

extern ConVar r_DrawDetailProps;

extern ConVar cl_leveloverview;
#ifdef _DEBUG
extern ConVar r_FadeProps;
#endif

//extern LinkedMiniProfiler *g_pMiniProfilers;
//LinkedMiniProfiler g_mpRecomputeLeaves("CClientLeafSystem::RecomputeRenderableLeaves", &g_pMiniProfilers);
//LinkedMiniProfiler g_mpComputeBounds("CClientLeafSystem::ComputeBounds", &g_pMiniProfilers);

DEFINE_FIXEDSIZE_ALLOCATOR( CClientRenderablesList, 1, CUtlMemoryPool::GROW_SLOW );

//-----------------------------------------------------------------------------
// Threading helpers
//-----------------------------------------------------------------------------

static void FrameLock()
{
	g_pMDLCache->BeginLock();
}

static void FrameUnlock()
{
	g_pMDLCache->EndLock();
}

void CallComputeFXBlend( IClientRenderable *&pRenderable )
{
	pRenderable->DO_NOT_USE_ComputeFxBlend();
}

//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
class CClientLeafSystem : public IClientLeafSystem, public ISpatialLeafEnumerator, public IClientAlphaPropertyMgr
{
public:
	virtual char const *Name() { return "CClientLeafSystem"; }

	// constructor, destructor
	CClientLeafSystem();
	virtual ~CClientLeafSystem();

	// Methods of IClientSystem
	bool Init() { return true; }
	void PostInit() {}
	void Shutdown() {}

	virtual bool IsPerFrame() { return true; }

	void PreRender();
	void PostRender() { }
	void Update( float frametime ) { m_nDebugIndex = 0; }

	void LevelInitPreEntity();
	void LevelInitPostEntity() {}
	void LevelShutdownPreEntity();
	void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

// Methods of IClientAlphaPropertyMgr
public:
	virtual IClientAlphaProperty *CreateClientAlphaProperty( IClientUnknown *pUnknown );
	virtual void DestroyClientAlphaProperty( IClientAlphaProperty *pAlphaProperty );

// Methods of IClientLeafSystem
public:
	void NewRenderable( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType );
	virtual void AddRenderable( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType );
	virtual bool IsRenderableInPVS( IClientRenderable *pRenderable );
	virtual void DO_NOT_USE_CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp );
	virtual void CreateRenderableHandle( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType );
	virtual void RemoveRenderable( ClientRenderHandle_t handle );


	virtual void SetSubSystemDataInLeaf( int leaf, int nSubSystemIdx, CClientLeafSubSystemData *pData );
	virtual CClientLeafSubSystemData *GetSubSystemDataInLeaf( int leaf, int nSubSystemIdx );

	// FIXME: There's an incestuous relationship between DetailObjectSystem
	// and the ClientLeafSystem. Maybe they should be the same system?
	virtual void GetDetailObjectsInLeaf( int leaf, int& firstDetailObject, int& detailObjectCount );
 	virtual void SetDetailObjectsInLeaf( int leaf, int firstDetailObject, int detailObjectCount );
	virtual void DrawDetailObjectsInLeaf( int leaf, int frameNumber, int& nFirstDetailObject, int& nDetailObjectCount );
	virtual bool ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber );
	virtual void RenderableChanged( ClientRenderHandle_t handle );
	virtual void ComputeTranslucentRenderLeaf( int count, const LeafIndex_t *pLeafList, const LeafFogVolume_t *pLeafFogVolumeList, int frameNumber, int viewID );
	virtual void CollateViewModelRenderables( CViewModelRenderablesList *pList );
	virtual void BuildRenderablesList( const SetupRenderInfo_t &info );
	virtual void DrawStaticProps( bool enable );
	virtual void DrawSmallEntities( bool enable );
	virtual void EnableAlternateSorting( ClientRenderHandle_t handle, bool bEnable );
	virtual void RenderWithViewModels( ClientRenderHandle_t handle, bool bEnable );
	virtual bool IsRenderingWithViewModels( ClientRenderHandle_t handle ) const;
	virtual void SetTranslucencyType( ClientRenderHandle_t handle, RenderableTranslucencyType_t nType );
	virtual RenderableTranslucencyType_t GetTranslucencyType( ClientRenderHandle_t handle ) const;
	virtual void SetModelType( ClientRenderHandle_t handle, RenderableModelType_t nType );
	virtual void EnableRendering( ClientRenderHandle_t handle, bool bEnable );
	virtual void EnableBloatedBounds( ClientRenderHandle_t handle, bool bEnable );
	virtual void DisableCachedRenderBounds( ClientRenderHandle_t handle, bool bDisable );
	virtual void DisableShadowDepthRendering( ClientRenderHandle_t handle, bool bDisable );
	virtual void DisableShadowDepthCaching( ClientRenderHandle_t handle, bool bDisable );

	// Adds a renderable to a set of leaves
	virtual void AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves );
	void AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves, bool bReceiveShadows );

	// The following methods are related to shadows...
	virtual ClientLeafShadowHandle_t AddShadow( ClientShadowHandle_t userId, unsigned short flags );
	virtual void RemoveShadow( ClientLeafShadowHandle_t h );

	virtual void ProjectShadow( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList );
	virtual void ProjectFlashlight( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList );

	// Find all shadow casters in a set of leaves
	virtual void EnumerateShadowsInLeaves( int leafCount, LeafIndex_t* pLeaves, IClientLeafShadowEnum* pEnum );
	virtual void RecomputeRenderableLeaves();
	virtual void DisableLeafReinsertion( bool bDisable );

	//Assuming the renderable would be in a properly built render list, generate a render list entry
	virtual EngineRenderGroup_t GenerateRenderListEntry( IClientRenderable *pRenderable, CClientRenderablesList::CEntry &entryOut );

	// methods of ISpatialLeafEnumerator
public:

	bool EnumerateLeaf( int leaf, int context );

	// Adds a shadow to a leaf
	void AddShadowToLeaf( int leaf, ClientLeafShadowHandle_t handle, bool bFlashlight );

	// Fill in a list of the leaves this renderable is in.
	// Returns -1 if the handle is invalid.
	int GetRenderableLeaves( ClientRenderHandle_t handle, int leaves[128] );

	// Get leaves this renderable is in
	virtual bool GetRenderableLeaf ( ClientRenderHandle_t handle, int* pOutLeaf, const int* pInIterator = 0, int* pOutIterator = 0 );

	// Singleton instance...
	static CClientLeafSystem s_ClientLeafSystem;

public:
	enum
	{
		RENDER_FLAGS_DISABLE_RENDERING		=  0x01,
		RENDER_FLAGS_HASCHANGED				=  0x02,
		RENDER_FLAGS_ALTERNATE_SORTING		=  0x04,
		RENDER_FLAGS_RENDER_WITH_VIEWMODELS	=  0x08,
		RENDER_FLAGS_BLOAT_BOUNDS			=  0x10,
		RENDER_FLAGS_BOUNDS_VALID			=  0x20,
		RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE = 0x40,
		RENDER_FLAGS_TWOPASS				=  0x80,
		RENDER_FLAGS_STATIC_PROP			=  0x100,
		RENDER_FLAGS_BRUSH_MODEL			=  0x200,
		RENDER_FLAGS_STUDIO_MODEL			=  0x400,
	};

	// All the information associated with a particular handle
	struct RenderableInfo_t
	{
		IClientRenderable*	m_pRenderable;
		IClientRenderableMod*	m_pRenderableMod;
		CClientAlphaProperty *m_pAlphaProperty;
		int					m_nRenderFrame;	// which frame did I render it in?
		int					m_EnumCount;	// Have I been added to a particular shadow yet?
		int					m_TranslucencyCalculated;
		unsigned int		m_LeafList;		// What leafs is it in?
		unsigned int		m_RenderLeaf;	// What leaf do I render in?
		uint16				m_Flags : 11;				// rendering flags
		bool				m_bDisableShadowDepthRendering : 1;	// Should we not render into the shadow depth map?
		bool				m_bDisableShadowDepthCaching : 1;	// Should we not be cached in the shadow depth map?
		EngineRenderGroup_t		m_EngineRenderGroup : 4;	// RenderGroup_t type
		EngineRenderGroup_t		m_EngineRenderGroupOpaque : 4;	// RenderGroup_t type
		ClientRenderGroup_t		m_ClientRenderGroup : 3;	// RenderGroup_t type
		unsigned short		m_FirstShadow;	// The first shadow caster that cast on it
		short m_Area;	// -1 if the renderable spans multiple areas.
		signed char			m_TranslucencyCalculatedView;
		RenderableTranslucencyType_t				m_nTranslucencyType : 2;	// RenderableTranslucencyType_t
		RenderableModelType_t				m_nModelType;			// RenderableModelType_t
		Vector				m_vecBloatedAbsMins;		// Use this for tree insertion
		Vector				m_vecBloatedAbsMaxs;
		Vector				m_vecAbsMins;			// NOTE: These members are not threadsafe!!
		Vector				m_vecAbsMaxs;			// They can be updated from any viewpoint (based on RENDER_FLAGS_BOUNDS_VALID)

		bool IsTwoPass() const;

		bool RendersWithViewmodels() const;

		ClientRenderGroup_t GetClientRenderGroup() const;

		bool IsOpaque() const;

		bool IsTranslucent() const;

		RenderableTranslucencyType_t GetTranslucencyType() const;

		RenderableModelType_t GetModelType() const;

		bool IsBrushModel() const;

		bool IsStudioModel() const;

		bool IsStaticProp() const;

		bool IsEntity() const;

		bool IsDisabled() const;
	};

	struct RenderableInfoOrLeaf_t
	{
		RenderableInfoOrLeaf_t()
			: info(NULL)
		{}

		RenderableInfoOrLeaf_t(RenderableInfo_t *info_)
			: info(info_), is_leaf(false)
		{}

		RenderableInfoOrLeaf_t(int leaf_)
			: leaf(leaf_), is_leaf(true)
		{}

		RenderableInfoOrLeaf_t(const RenderableInfoOrLeaf_t &) = default;
		RenderableInfoOrLeaf_t &operator=(const RenderableInfoOrLeaf_t &) = default;

		RenderableInfoOrLeaf_t(RenderableInfoOrLeaf_t &&) = default;
		RenderableInfoOrLeaf_t &operator=(RenderableInfoOrLeaf_t &&) = default;

		union {
			RenderableInfo_t *info;
			int leaf;
		};
		bool is_leaf;
	};

	struct RenderableInfoAndHandle_t
	{
		RenderableInfoAndHandle_t()
			: infoorleaf(NULL), handle(INVALID_CLIENT_RENDER_HANDLE)
		{}

		RenderableInfoAndHandle_t(RenderableInfo_t *info_, ClientRenderHandle_t handle_)
			: infoorleaf(info_), handle(handle_)
		{}

		RenderableInfoAndHandle_t(int leaf_)
			: infoorleaf(leaf_), handle(INVALID_CLIENT_RENDER_HANDLE)
		{}

		RenderableInfoAndHandle_t(const RenderableInfoAndHandle_t &) = default;
		RenderableInfoAndHandle_t &operator=(const RenderableInfoAndHandle_t &) = default;

		RenderableInfoAndHandle_t(RenderableInfoAndHandle_t &&) = default;
		RenderableInfoAndHandle_t &operator=(RenderableInfoAndHandle_t &&) = default;

		RenderableInfoOrLeaf_t infoorleaf;
		ClientRenderHandle_t handle;
	};

private:
	// The leaf contains an index into a list of renderables
	struct ClientLeaf_t
	{
		unsigned int	m_FirstElement;
		unsigned short	m_FirstShadow;

		unsigned short	m_FirstDetailProp;
		unsigned short	m_DetailPropCount;
		int				m_DetailPropRenderFrame;
		CClientLeafSubSystemData *m_pSubSystemData[N_CLSUBSYSTEMS];

	};

	// Shadow information
	struct ShadowInfo_t
	{
		unsigned short	m_FirstLeaf;
		unsigned short	m_FirstRenderable;
		int				m_EnumCount;
		ClientShadowHandle_t	m_Shadow;
		unsigned short	m_Flags;
	};

	struct EnumResult_t
	{
		int leaf;
		EnumResult_t *pNext;
	};

	struct EnumResultList_t
	{
		EnumResult_t *pHead;
		ClientRenderHandle_t handle;
		bool bReceiveShadows;
	};

	struct BuildRenderListInfo_t
	{
		Vector	m_vecMins;
		Vector	m_vecMaxs;
		short	m_nArea;
		uint8	m_nAlpha;
		uint8	m_nShadowAlpha;
		bool	m_bPerformOcclusionTest : 1;
		bool	m_bIgnoreZBuffer : 1;
	};

	struct AlphaInfo_t
	{
		CClientAlphaProperty *m_pAlphaProperty;
		IClientRenderableMod *m_pRenderableMod;
		Vector m_vecCenter;
		float m_flRadius;
		float m_flFadeFactor;
	};

	void AddRenderableToLeaf( int leaf, ClientRenderHandle_t handle, bool bReceiveShadows );

	void SortEntities(  const Vector &vecRenderOrigin, const Vector &vecRenderForward, CClientRenderablesList::CEntry *pEntities, int nEntities );

	// Returns -1 if the renderable spans more than one area. If it's totally in one area, then this returns the leaf.
	short GetRenderableArea( ClientRenderHandle_t handle );

	// remove renderables from leaves
	void InsertIntoTree( ClientRenderHandle_t handle, const Vector &absMins, const Vector &absMaxs );
	void RemoveFromTree( ClientRenderHandle_t handle );

	struct InsertIntoTreeArgs_t
	{
		ClientRenderHandle_t handle;
		Vector absMins;
		Vector absMaxs;
	};

	void InsertIntoTreeParallel( InsertIntoTreeArgs_t &args )
	{ InsertIntoTree(args.handle, args.absMins, args.absMaxs); }

	// Adds, removes renderables from view model list
	void AddToViewModelList( ClientRenderHandle_t handle );
	void RemoveFromViewModelList( ClientRenderHandle_t handle );

	// Insert translucent renderables into list of translucent objects
	void InsertTranslucentRenderable( IClientRenderable* pRenderable,
		int& count, IClientRenderable** pList, float* pDist );

	// Used to change renderables from translucent to opaque
	// Only really used by the static prop fading...
	void DO_NOT_USE_ChangeRenderableRenderGroup( ClientRenderHandle_t handle, EngineRenderGroup_t group ) override;

	// Adds a shadow to a leaf/removes shadow from renderable
	void AddShadowToRenderable( ClientRenderHandle_t renderHandle, ClientLeafShadowHandle_t shadowHandle );
	void RemoveShadowFromRenderables( ClientLeafShadowHandle_t handle );

	// Adds a shadow to a leaf/removes shadow from renderable
	bool ShouldRenderableReceiveShadow( ClientRenderHandle_t renderHandle, int nShadowFlags );

	// Adds a shadow to a leaf/removes shadow from leaf
	void RemoveShadowFromLeaves( ClientLeafShadowHandle_t handle );

	// Methods related to renderable list building
	int ExtractStaticProps( int nCount, RenderableInfoAndHandle_t *ppRenderables );
	int ExtractTranslucentRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables );
	int ExtractDuplicates( int nFrameNumber, int nCount, RenderableInfoAndHandle_t *ppRenderables );
	int ExtractDisableShadowDepthRenderables(int nCount, RenderableInfoAndHandle_t *ppRenderables);
	int ExtractDisableShadowDepthCacheRenderables(int nCount, RenderableInfoAndHandle_t *ppRenderables);
	int ExtractFadedRenderables( int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo );
	int ExtractNotInScreen( int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables );
	void ComputeBounds( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo );
	int ExtractCulledRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo );
	int ExtractOccludedRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo );
	void AddRenderablesToRenderLists( const SetupRenderInfo_t &info, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo, int nDetailCount, DetailRenderableInfo_t *pDetailInfo );
	void AddDependentRenderables( const SetupRenderInfo_t &info );

	int ComputeTranslucency( int nFrameNumber, int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo );
	void ComputeDistanceFade( int nCount, AlphaInfo_t *pAlphaInfo, BuildRenderListInfo_t *pRLInfo );
	void ComputeScreenFade( const ScreenSizeComputeInfo_t &info, float flMinScreenWidth, float flMaxScreenWidth, int nCount, AlphaInfo_t *pAlphaInfo );

	void CalcRenderableWorldSpaceAABB_Bloated( const RenderableInfo_t &info, Vector &absMin, Vector &absMax );

	// Methods associated with the various bi-directional sets
	static unsigned int& FirstRenderableInLeaf( int leaf ) 
	{ 
		return s_ClientLeafSystem.m_Leaf[leaf].m_FirstElement;
	}

	static unsigned int& FirstLeafInRenderable( ClientRenderHandle_t renderable ) 
	{ 
		return s_ClientLeafSystem.m_Renderables[(uint)renderable].m_LeafList;
	}

	static unsigned short& FirstShadowInLeaf( int leaf ) 
	{ 
		return s_ClientLeafSystem.m_Leaf[leaf].m_FirstShadow;
	}

	static unsigned short& FirstLeafInShadow( ClientLeafShadowHandle_t shadow ) 
	{ 
		return s_ClientLeafSystem.m_Shadows[(uint)shadow].m_FirstLeaf;
	}

	static unsigned short& FirstShadowOnRenderable( ClientRenderHandle_t renderable ) 
	{ 
		return s_ClientLeafSystem.m_Renderables[(uint)renderable].m_FirstShadow;
	}

	static unsigned short& FirstRenderableInShadow( ClientLeafShadowHandle_t shadow ) 
	{ 
		return s_ClientLeafSystem.m_Shadows[(uint)shadow].m_FirstRenderable;
	}

	void FrameLock()
	{
		g_pMDLCache->BeginLock();
	}

	void FrameUnlock()
	{
		g_pMDLCache->EndLock();
	}

public:
	const RenderableInfo_t &GetRenderableInfo( ClientRenderHandle_t handle ) const
	{ 
		return m_Renderables[(uint)handle];
	}

private:
	// Stores data associated with each leaf.
	CUtlVector< ClientLeaf_t >	m_Leaf;

	// Stores all unique non-detail renderables
	CUtlLinkedList< RenderableInfo_t, ClientRenderHandle_t, false, unsigned int >	m_Renderables;

	// Information associated with shadows registered with the client leaf system
	CUtlLinkedList< ShadowInfo_t, ClientLeafShadowHandle_t, false, unsigned int >	m_Shadows;

	// Maintains the list of all renderables in a particular leaf
	CBidirectionalSet< int, ClientRenderHandle_t, unsigned int, unsigned int >	m_RenderablesInLeaf;

	// Maintains a list of all shadows in a particular leaf 
	CBidirectionalSet< int, ClientLeafShadowHandle_t, unsigned short, unsigned int >	m_ShadowsInLeaf;

	// Maintains a list of all shadows cast on a particular renderable
	CBidirectionalSet< ClientRenderHandle_t, ClientLeafShadowHandle_t, unsigned short, unsigned int >	m_ShadowsOnRenderable;

	// Dirty list of renderables
	CUtlVector< ClientRenderHandle_t >	m_DirtyRenderables;

	// List of renderables in view model render groups
	CUtlVector< ClientRenderHandle_t >	m_ViewModels;

	// Should I draw static props?
	bool m_DrawStaticProps;
	bool m_DrawSmallObjects;
	bool m_bDisableLeafReinsertion;

	// A little enumerator to help us when adding shadows to renderables
	int	m_ShadowEnum;

	// Does anything disable shadow depth
	int m_nDisableShadowDepthCount;

	// Does anything disable shadow depth cache
	int m_nDisableShadowDepthCacheCount;

	CTSList<EnumResultList_t> m_DeferredInserts;

	// Does anything use alternate sorting?
	int m_nAlternateSortCount;

	// Number of alpha properties out there
	int m_nAlphaPropertyCount;

	CUtlMemoryPool m_AlphaPropertyPool;

	int m_nDebugIndex;
};


//-----------------------------------------------------------------------------
// Expose IClientLeafSystem to the client dll.
//-----------------------------------------------------------------------------
CClientLeafSystem CClientLeafSystem::s_ClientLeafSystem;
IClientAlphaPropertyMgr *g_pClientAlphaPropertyMgr = &CClientLeafSystem::s_ClientLeafSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientLeafSystem, IClientLeafSystem, CLIENTLEAFSYSTEM_INTERFACE_VERSION, CClientLeafSystem::s_ClientLeafSystem );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientLeafSystem, IClientAlphaPropertyMgr, CLIENT_ALPHA_PROPERTY_MGR_INTERFACE_VERSION, CClientLeafSystem::s_ClientLeafSystem );

IClientLeafSystem* ClientLeafSystem()
{
	return &CClientLeafSystem::s_ClientLeafSystem;
}

//-----------------------------------------------------------------------------
// Methods of IClientAlphaPropertyMgr
//-----------------------------------------------------------------------------
IClientAlphaProperty *CClientLeafSystem::CreateClientAlphaProperty( IClientUnknown *pUnk )
{
	++m_nAlphaPropertyCount;
	CClientAlphaProperty *pProperty = (CClientAlphaProperty*)m_AlphaPropertyPool.Alloc( sizeof(CClientAlphaProperty) );
	Construct( pProperty );
	pProperty->Init( pUnk );
	return pProperty;
}

void CClientLeafSystem::DestroyClientAlphaProperty( IClientAlphaProperty *pAlphaProperty )
{
	if ( !pAlphaProperty )
		return;

	Destruct( static_cast<CClientAlphaProperty*>( pAlphaProperty ) );
	m_AlphaPropertyPool.Free( pAlphaProperty );
	Assert( m_nAlphaPropertyCount > 0 );
	if ( --m_nAlphaPropertyCount == 0 )
	{
		m_AlphaPropertyPool.Clear();
	}
}

bool CClientLeafSystem::RenderableInfo_t::IsTwoPass() const
{
	return (
		(m_Flags & RENDER_FLAGS_TWOPASS) ||
		(m_nTranslucencyType == RENDERABLE_IS_TWO_PASS)
	);
}

bool CClientLeafSystem::RenderableInfo_t::RendersWithViewmodels() const
{
	return (
		((m_Flags & RENDER_FLAGS_RENDER_WITH_VIEWMODELS) != 0)
	);
}

ClientRenderGroup_t CClientLeafSystem::RenderableInfo_t::GetClientRenderGroup() const
{
	return m_ClientRenderGroup;
}

bool CClientLeafSystem::RenderableInfo_t::IsOpaque() const
{
	return (m_nTranslucencyType == RENDERABLE_IS_OPAQUE);
}

bool CClientLeafSystem::RenderableInfo_t::IsTranslucent() const
{
	return (m_nTranslucencyType == RENDERABLE_IS_TRANSLUCENT);
}

RenderableTranslucencyType_t CClientLeafSystem::RenderableInfo_t::GetTranslucencyType() const
{
	return m_nTranslucencyType;
}

RenderableModelType_t CClientLeafSystem::RenderableInfo_t::GetModelType() const
{
	return m_nModelType;
}

bool CClientLeafSystem::RenderableInfo_t::IsBrushModel() const
{
	return (
		((m_Flags & RENDER_FLAGS_BRUSH_MODEL) != 0) ||
		((m_nModelType == RENDERABLE_MODEL_BRUSH))
	);
}

bool CClientLeafSystem::RenderableInfo_t::IsStudioModel() const
{
	return (
		((m_Flags & RENDER_FLAGS_STUDIO_MODEL) != 0) ||
		((m_nModelType == RENDERABLE_MODEL_STUDIOMDL))
	);
}

bool CClientLeafSystem::RenderableInfo_t::IsStaticProp() const
{
	return (
		((m_Flags & RENDER_FLAGS_STATIC_PROP) != 0) ||
		((m_nModelType == RENDERABLE_MODEL_STATIC_PROP))
	);
}

bool CClientLeafSystem::RenderableInfo_t::IsEntity() const
{
	return (
		((m_nModelType == RENDERABLE_MODEL_ENTITY))
	);
}

bool CClientLeafSystem::RenderableInfo_t::IsDisabled() const
{
	return (
		((m_Flags & RENDER_FLAGS_DISABLE_RENDERING) != 0)
	);
}

void CalcRenderableWorldSpaceAABB_Fast( IClientRenderable *pRenderable, Vector &absMin, Vector &absMax );

//-----------------------------------------------------------------------------
// Helper functions.
//-----------------------------------------------------------------------------
void DefaultRenderBoundsWorldspace( IClientRenderable *pRenderable, Vector &absMins, Vector &absMaxs )
{
	// Tracker 37433:  This fixes a bug where if the stunstick is being wielded by a combine soldier, the fact that the stick was
	//  attached to the soldier's hand would move it such that it would get frustum culled near the edge of the screen.
	IClientUnknown *pUnk = pRenderable->GetIClientUnknown();
	C_BaseEntity *pEnt = pUnk->GetBaseEntity();
	if ( pEnt && ( pEnt->IsFollowingEntity() || ( pEnt->GetParentAttachment() > 0 ) ) )
	{
		C_BaseEntity *pParent = pEnt->GetMoveParent();
		if ( pParent )
		{
			// Get the parent's abs space world bounds.
			CalcRenderableWorldSpaceAABB_Fast( pParent, absMins, absMaxs );

			// Add the maximum of our local render bounds. This is making the assumption that we can be at any
			// point and at any angle within the parent's world space bounds.
			Vector vAddMins, vAddMaxs;
			pEnt->GetRenderBounds( vAddMins, vAddMaxs );
			// if our origin is actually farther away than that, expand again
			float radius = pEnt->GetLocalOrigin().Length();

			float flBloatSize = MAX( vAddMins.Length(), vAddMaxs.Length() );
			flBloatSize = MAX(flBloatSize, radius);
			absMins -= Vector( flBloatSize, flBloatSize, flBloatSize );
			absMaxs += Vector( flBloatSize, flBloatSize, flBloatSize );
			return;
		}
	}

	Vector mins, maxs;
	pRenderable->GetRenderBounds( mins, maxs );

	// FIXME: Should I just use a sphere here?
	// Another option is to pass the OBB down the tree; makes for a better fit
	// Generate a world-aligned AABB
	const QAngle& angles = pRenderable->GetRenderAngles();
	if (angles == vec3_angle)
	{
		const Vector& origin = pRenderable->GetRenderOrigin();
		VectorAdd( mins, origin, absMins );
		VectorAdd( maxs, origin, absMaxs );
	}
	else
	{
		TransformAABB( pRenderable->RenderableToWorldTransform(), mins, maxs, absMins, absMaxs );
	}
	Assert( absMins.IsValid() && absMaxs.IsValid() );
}

// Figure out a world space bounding box that encloses the entity's local render bounds in world space.
inline void CalcRenderableWorldSpaceAABB( 
	IClientRenderable *pRenderable, 
	Vector &absMins,
	Vector &absMaxs )
{
	pRenderable->GetRenderBoundsWorldspace( absMins, absMaxs );
}


// This gets an AABB for the renderable, but it doesn't cause a parent's bones to be setup.
// This is used for placement in the leaves, but the more expensive version is used for culling.
void CalcRenderableWorldSpaceAABB_Fast( IClientRenderable *pRenderable, Vector &absMin, Vector &absMax )
{
	C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
	if ( pEnt && ( pEnt->IsFollowingEntity() || ( pEnt->GetMoveParent() && ( pEnt->GetParentAttachment() > 0 ) ) ) )
	{
		C_BaseEntity *pParent = pEnt->GetMoveParent();
		Assert( pParent );

		// Get the parent's abs space world bounds.
		CalcRenderableWorldSpaceAABB_Fast( pParent, absMin, absMax );

		// Add the maximum of our local render bounds. This is making the assumption that we can be at any
		// point and at any angle within the parent's world space bounds.
		Vector vAddMins, vAddMaxs;
		pEnt->GetRenderBounds( vAddMins, vAddMaxs );
		// if our origin is actually farther away than that, expand again
		float radius = pEnt->GetLocalOrigin().Length();

		float flBloatSize = MAX( vAddMins.Length(), vAddMaxs.Length() );
		flBloatSize = MAX(flBloatSize, radius);
		absMin -= Vector( flBloatSize, flBloatSize, flBloatSize );
		absMax += Vector( flBloatSize, flBloatSize, flBloatSize );
	}
	else
	{
		// Start out with our own render bounds. Since we don't have a parent, this won't incur any nasty 
		CalcRenderableWorldSpaceAABB( pRenderable, absMin, absMax );
	}
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CClientLeafSystem::CClientLeafSystem() : m_DrawStaticProps(true), m_DrawSmallObjects(true), 
	m_AlphaPropertyPool( sizeof( CClientAlphaProperty ), 1024, CUtlMemoryPool::GROW_SLOW, "CClientAlphaProperty" ) 
{
	// Set up the bi-directional lists...
	m_RenderablesInLeaf.Init( FirstRenderableInLeaf, FirstLeafInRenderable );
	m_ShadowsInLeaf.Init( FirstShadowInLeaf, FirstLeafInShadow ); 
	m_ShadowsOnRenderable.Init( FirstShadowOnRenderable, FirstRenderableInShadow );
	m_nAlternateSortCount = 0;
	m_nDisableShadowDepthCount = 0;
	m_nDisableShadowDepthCacheCount = 0;
	m_bDisableLeafReinsertion = false;
}

CClientLeafSystem::~CClientLeafSystem()
{
}

//-----------------------------------------------------------------------------
// Activate, deactivate static props
//-----------------------------------------------------------------------------
void CClientLeafSystem::DrawStaticProps( bool enable )
{
	m_DrawStaticProps = enable;
}

void CClientLeafSystem::DrawSmallEntities( bool enable )
{
	m_DrawSmallObjects = enable;
}

void CClientLeafSystem::DisableLeafReinsertion( bool bDisable )
{
	m_bDisableLeafReinsertion = bDisable;
}

//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------
void CClientLeafSystem::LevelInitPreEntity()
{
	MEM_ALLOC_CREDIT();

	m_Renderables.EnsureCapacity( 1024 );
	m_RenderablesInLeaf.EnsureCapacity( 1024 );
	m_ShadowsInLeaf.EnsureCapacity( 256 );
	m_ShadowsOnRenderable.EnsureCapacity( 256 );
	m_DirtyRenderables.EnsureCapacity( 256 );

	// Add all the leaves we'll need
	int leafCount = engine->LevelLeafCount();
	m_Leaf.EnsureCapacity( leafCount );

	ClientLeaf_t newLeaf;
	newLeaf.m_FirstElement = m_RenderablesInLeaf.InvalidIndex();
	newLeaf.m_FirstShadow = m_ShadowsInLeaf.InvalidIndex();
	memset( newLeaf.m_pSubSystemData, 0, sizeof( newLeaf.m_pSubSystemData ) );
	newLeaf.m_FirstDetailProp = 0;
	newLeaf.m_DetailPropCount = 0;
	newLeaf.m_DetailPropRenderFrame = -1;
	while ( --leafCount >= 0 )
	{
		m_Leaf.AddToTail( newLeaf );
	}
}

void CClientLeafSystem::LevelShutdownPreEntity()
{
}

void CClientLeafSystem::LevelShutdownPostEntity()
{
	m_nAlternateSortCount = 0;
	m_nDisableShadowDepthCount = 0;
	m_nDisableShadowDepthCacheCount = 0;
	m_ViewModels.Purge();
	m_Renderables.Purge();
	m_RenderablesInLeaf.Purge();
	m_Shadows.Purge();

	// delete subsystem data
	for( int i = 0; i < m_Leaf.Count() ; i++ )
	{
		for( int j = 0 ; j < ARRAYSIZE( m_Leaf[i].m_pSubSystemData ) ; j++ )
		{
			if ( m_Leaf[i].m_pSubSystemData[j] )
			{
				delete m_Leaf[i].m_pSubSystemData[j];
				m_Leaf[i].m_pSubSystemData[j] = NULL;
			}
		}
	}
	m_Leaf.Purge();
	m_ShadowsInLeaf.Purge();
	m_ShadowsOnRenderable.Purge();
	m_DirtyRenderables.Purge();
}

//-----------------------------------------------------------------------------
// Computes a bloated bounding box to reduce insertions into the tree
//-----------------------------------------------------------------------------
#define BBOX_GRANULARITY 32.0f
#define MIN_SHRINK_VOLUME ( 32.0f * 32.0f * 32.0f )

void CClientLeafSystem::CalcRenderableWorldSpaceAABB_Bloated( const RenderableInfo_t &info, Vector &absMin, Vector &absMax )
{
	CalcRenderableWorldSpaceAABB_Fast( info.m_pRenderable, absMin, absMax );

	// Bloat bounds to avoid reinsertion into tree
	absMin.x = floor( absMin.x / BBOX_GRANULARITY ) * BBOX_GRANULARITY;
	absMin.y = floor( absMin.y / BBOX_GRANULARITY ) * BBOX_GRANULARITY;
	absMin.z = floor( absMin.z / BBOX_GRANULARITY ) * BBOX_GRANULARITY;

	absMax.x = ceil( absMax.x / BBOX_GRANULARITY ) * BBOX_GRANULARITY;
	absMax.y = ceil( absMax.y / BBOX_GRANULARITY ) * BBOX_GRANULARITY;
	absMax.z = ceil( absMax.z / BBOX_GRANULARITY ) * BBOX_GRANULARITY;

	// Optimization to make particle systems not re-insert themselves
	if ( info.m_Flags & RENDER_FLAGS_BLOAT_BOUNDS )
	{
		Vector vecTempMin, vecTempMax;
		VectorMin( info.m_vecBloatedAbsMins, absMin, vecTempMin );
		VectorMax( info.m_vecBloatedAbsMaxs, absMax, vecTempMax );
		float flTempVolume = ComputeVolume( vecTempMin, vecTempMax );
		float flCurrVolume = ComputeVolume( absMin, absMax );

		if ( ( flTempVolume <= MIN_SHRINK_VOLUME ) || ( flCurrVolume * 2.0f >= flTempVolume ) )
		{
			absMin = vecTempMin;
			absMax = vecTempMax;
		}
	}
}


//-----------------------------------------------------------------------------
// This is what happens before rendering a particular view
//-----------------------------------------------------------------------------
void CClientLeafSystem::PreRender()
{
	VPROF_BUDGET( "CClientLeafSystem::PreRender", "PreRender" );

	//	Assert( m_DirtyRenderables.Count() == 0 );

	// FIXME: This should never need to happen here!
	// At the moment, it's necessary because of the horrid viewmodel/combatweapon
	// confusion in the code where a combat weapon changes its rendering model
	// per view.
	RecomputeRenderableLeaves();
}

// Use this to make sure we're not adding the same renderables to the list while we're going through and re-inserting them into the clientleafsystem
static bool s_bIsInRecomputeRenderableLeaves = false;

void CClientLeafSystem::RecomputeRenderableLeaves()
{
	int i;
	int nIterations = 0;

	bool bDebugLeafSystem = cl_leafsystemvis.GetBool();

	Vector absMins, absMaxs;
	while ( m_DirtyRenderables.Count() )
	{
		if ( ++nIterations > 10 )
		{
			Warning( "Too many dirty renderables!\n" );
			break;
		}

		s_bIsInRecomputeRenderableLeaves = true;

		int nDirty = m_DirtyRenderables.Count();
		for ( i = nDirty; --i >= 0; )
		{
			ClientRenderHandle_t handle = m_DirtyRenderables[i];
			Assert( m_Renderables[ (uint)handle ].m_Flags & RENDER_FLAGS_HASCHANGED );
			RenderableInfo_t &info = m_Renderables[ (uint)handle ];

			Assert( info.m_Flags & RENDER_FLAGS_HASCHANGED );

			// See note below
			info.m_Flags &= ~RENDER_FLAGS_HASCHANGED;

			CalcRenderableWorldSpaceAABB_Bloated( info, absMins, absMaxs );
			if ( absMins != info.m_vecBloatedAbsMins || absMaxs != info.m_vecBloatedAbsMaxs )
			{
				// Update position in leaf system
				RemoveFromTree( handle );
				InsertIntoTree( m_DirtyRenderables[i], absMins, absMaxs );
				if ( bDebugLeafSystem )
				{
					debugoverlay->AddBoxOverlay( vec3_origin, absMins, absMaxs, QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
				}
			}
		}

		s_bIsInRecomputeRenderableLeaves = false;

	#if 0
		bool bThreaded = ( nDirty > 5 && cl_threaded_client_leaf_system.GetBool() && g_pThreadPool->NumThreads() );

		if ( !bThreaded )
	#endif
		{
			for ( i = nDirty; --i >= 0; )
			{
				InsertIntoTree( m_DirtyRenderables[i], absMins, absMaxs );
			}
		}
	#if 0
		else
		{
			// InsertIntoTree can result in new renderables being added, so copy:
			InsertIntoTreeArgs_t *pDirtyRenderables = (InsertIntoTreeArgs_t *)alloca( sizeof(InsertIntoTreeArgs_t) * nDirty );
			for ( i = nDirty; --i >= 0; ) {
				pDirtyRenderables[i].handle = m_DirtyRenderables[i];
				pDirtyRenderables[i].absMins = absMins;
				pDirtyRenderables[i].absMaxs = absMaxs;
			}
			ParallelProcess( "CClientLeafSystem::PreRender", pDirtyRenderables, nDirty, this, &CClientLeafSystem::InsertIntoTreeParallel, &CClientLeafSystem::FrameLock, &CClientLeafSystem::FrameUnlock );
		}
	#endif

		if ( m_DeferredInserts.Count() )
		{
			EnumResultList_t enumResultList;
			while ( m_DeferredInserts.PopItem( &enumResultList ) )
			{
				m_ShadowEnum++;
				while ( enumResultList.pHead )
				{
					EnumResult_t *p = enumResultList.pHead;
					enumResultList.pHead = p->pNext;
					AddRenderableToLeaf( p->leaf, enumResultList.handle, enumResultList.bReceiveShadows );
					delete p;
				}
			}
		}

		for ( i = nDirty; --i >= 0; )
		{
			// Cache off the area it's sitting in.
			ClientRenderHandle_t handle = m_DirtyRenderables[i];
			RenderableInfo_t& renderable = m_Renderables[ (uint)handle ];

			renderable.m_Flags &= ~RENDER_FLAGS_HASCHANGED;
			m_Renderables[(uint)handle].m_Area = GetRenderableArea( handle );
		}

		m_DirtyRenderables.RemoveMultiple( 0, nDirty );
	}
}

#ifdef STAGING_ONLY
static const char *ent_model_typestr[]{
	"mod_bad",
	"mod_brush",
	"mod_sprite",
	"mod_studio",
};

static const char *model_typestr[]{
	"RENDERABLE_MODEL_UNKNOWN_TYPE",
	"RENDERABLE_MODEL_ENTITY",
	"RENDERABLE_MODEL_STUDIOMDL",
	"RENDERABLE_MODEL_STATIC_PROP",
	"RENDERABLE_MODEL_BRUSH",
};

static const char *translucent_typestr[]{
	"RENDERABLE_IS_OPAQUE",
	"RENDERABLE_IS_TRANSLUCENT",
	"RENDERABLE_IS_TWO_PASS",
};

static const char *clrendergroup_typestr[]{
	"CLIENT_RENDER_GROUP_OPAQUE",
	"CLIENT_RENDER_GROUP_TRANSLUCENT",
	"CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ",
	"CLIENT_RENDER_GROUP_COUNT",
};

static const char *engrndergroup_typestr[]{
	"ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE",
	"ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE",
	"ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM",
	"ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM",
	"ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL",
	"ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL",
	"ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY",
	"ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY",
	"ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY",
	"ENGINE_RENDER_GROUP_TWOPASS",
	"ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE",
	"ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT",
	"ENGINE_RENDER_GROUP_OPAQUE_BRUSH",
	"ENGINE_RENDER_GROUP_OTHER",
	"ENGINE_RENDER_GROUP_COUNT",
};

static const char *rndmodestr[]{
	"kRenderNormal",
	"kRenderTransColor",
	"kRenderTransTexture",
	"kRenderGlow",
	"kRenderTransAlpha",
	"kRenderTransAdd",
	"kRenderEnvironmental",
	"kRenderTransAddFrameBlend",
	"kRenderTransAlphaAdd",
	"kRenderWorldGlow",
	"kRenderNone",
	"kRenderModeCount",
};

static const char *rndfxstr[]{
	"kRenderFxNone",
	"kRenderFxPulseSlow",
	"kRenderFxPulseFast",
	"kRenderFxPulseSlowWide",
	"kRenderFxPulseFastWide",
	"kRenderFxFadeSlow",
	"kRenderFxFadeFast",
	"kRenderFxSolidSlow",
	"kRenderFxSolidFast",
	"kRenderFxStrobeSlow",
	"kRenderFxStrobeFast",
	"kRenderFxStrobeFaster",
	"kRenderFxFlickerSlow",
	"kRenderFxFlickerFast",
	"kRenderFxNoDissipation",
	"kRenderFxDistort",
	"kRenderFxHologram",
	"kRenderFxExplode",
	"kRenderFxGlowShell",
	"kRenderFxClampMinScale",
	"kRenderFxEnvRain",
	"kRenderFxEnvSnow",
	"kRenderFxSpotlight",
	"kRenderFxPulseFastWider",
	"kRenderFxFadeOut",
	"kRenderFxFadeIn",
	"kRenderFxMax",
};

static void dump_render_info(const CClientLeafSystem::RenderableInfo_t &info)
{
	DevMsg("  info engine render group = %s\n", engrndergroup_typestr[info.m_EngineRenderGroup]);
	DevMsg("  info engine render group opaque = %s\n", engrndergroup_typestr[info.m_EngineRenderGroupOpaque]);
	DevMsg("  info client render group = %s\n", clrendergroup_typestr[info.m_ClientRenderGroup]);
	DevMsg("  info translucency type = %s\n", translucent_typestr[info.m_nTranslucencyType]);
	DevMsg("  info model type = %s\n", model_typestr[info.m_nModelType+1]);
	DevMsg("  info flags = \n");
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_DISABLE_RENDERING) != 0) {
		DevMsg("    RENDER_FLAGS_DISABLE_RENDERING\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_HASCHANGED) != 0) {
		DevMsg("    RENDER_FLAGS_HASCHANGED\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_ALTERNATE_SORTING) != 0) {
		DevMsg("    RENDER_FLAGS_ALTERNATE_SORTING\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_RENDER_WITH_VIEWMODELS) != 0) {
		DevMsg("    RENDER_FLAGS_RENDER_WITH_VIEWMODELS\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_BLOAT_BOUNDS) != 0) {
		DevMsg("    RENDER_FLAGS_BLOAT_BOUNDS\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_BOUNDS_VALID) != 0) {
		DevMsg("    RENDER_FLAGS_BOUNDS_VALID\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE) != 0) {
		DevMsg("    RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_TWOPASS) != 0) {
		DevMsg("    RENDER_FLAGS_TWOPASS\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_STATIC_PROP) != 0) {
		DevMsg("    RENDER_FLAGS_STATIC_PROP\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_BRUSH_MODEL) != 0) {
		DevMsg("    RENDER_FLAGS_BRUSH_MODEL\n");
	}
	if((info.m_Flags & CClientLeafSystem::RENDER_FLAGS_STUDIO_MODEL) != 0) {
		DevMsg("    RENDER_FLAGS_STUDIO_MODEL\n");
	}
}

CON_COMMAND_F(cl_ent_render_debug, "", FCVAR_DEVELOPMENTONLY|FCVAR_CHEAT)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer) {
		DevMsg("no client\n");
		return;
	}

	Vector forward;
	pPlayer->EyeVectors(&forward);

	trace_t tr;
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);
	if( !tr.m_pEnt ) {
		DevMsg("trace fail\n");
		return;
	}

	C_BaseEntity *pTarget = tr.m_pEnt;

	DevMsg("cl_ent_render_debug %p\n", pTarget->GetClientRenderable());
	DevMsg("  ent translucency type = %s\n", translucent_typestr[pTarget->ComputeTranslucencyType()]);

	if(pTarget->GetModel()) {
		DevMsg("  model = %s\n", modelinfo->GetModelName(pTarget->GetModel()));
		DevMsg("  ent model type = %s\n", ent_model_typestr[modelinfo->GetModelType(pTarget->GetModel())]);
		DevMsg("  is two pass = %i\n", modelinfo->IsTranslucentTwoPass(pTarget->GetModel()));
		DevMsg("  is translucent = %i\n", modelinfo->IsTranslucent(pTarget->GetModel()));
	} else {
		DevMsg("no model\n");
	}

	if(pTarget->AlphaProp()) {
		DevMsg("  alpha blend = %i\n", pTarget->AlphaProp()->GetAlphaBlend());
		DevMsg("  alpha modulation = %i\n", pTarget->AlphaProp()->GetAlphaModulation());
		DevMsg("  render mode = %s\n", rndmodestr[pTarget->AlphaProp()->GetRenderMode()]);
		DevMsg("  render fx = %s\n", rndfxstr[pTarget->AlphaProp()->GetRenderFX()]);
		DevMsg("  ignorez = %i\n", pTarget->AlphaProp()->IgnoresZBuffer());
		DevMsg("  fade min = %f\n", pTarget->AlphaProp()->GetMinFadeDist());
		DevMsg("  fade max = %f\n", pTarget->AlphaProp()->GetMaxFadeDist());
		DevMsg("  fade scale = %f\n", pTarget->AlphaProp()->GetGlobalFadeScale());
	} else {
		DevMsg("no alphaprop\n");
	}

	ClientRenderHandle_t handle = pTarget->RenderHandle();
	if(handle == INVALID_CLIENT_RENDER_HANDLE) {
		DevMsg("no render handle\n");
		return;
	}

	const CClientLeafSystem::RenderableInfo_t &info = CClientLeafSystem::s_ClientLeafSystem.GetRenderableInfo(handle);

	dump_render_info(info);
}
#endif

//-----------------------------------------------------------------------------
// Creates a new renderable
//-----------------------------------------------------------------------------
void CClientLeafSystem::NewRenderable( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType )
{
	Assert( pRenderable );
	Assert( pRenderable->RenderHandle() == INVALID_CLIENT_RENDER_HANDLE );

	ClientRenderHandle_t handle = (ClientRenderHandle_t)m_Renderables.AddToTail();
	RenderableInfo_t &info = m_Renderables[(uint)handle];

	int modelType = modelinfo->GetModelType( pRenderable->GetModel() );

	if(nModelType == RENDERABLE_MODEL_UNKNOWN_TYPE) {
		switch(modelType) {
		case mod_sprite:
			nModelType = RENDERABLE_MODEL_ENTITY;
			break;
		case mod_brush:
			nModelType = RENDERABLE_MODEL_BRUSH;
			break;
		case mod_studio:
			nModelType = RENDERABLE_MODEL_STUDIOMDL;
			break;
		default:
			Assert(0);
			break;
		}
	}

	IClientUnknownMod *pUnkMod = dynamic_cast<IClientUnknownMod *>( pRenderable->GetIClientUnknown() );
	CClientAlphaProperty *pAlphaProp = static_cast< CClientAlphaProperty* >( pUnkMod ? pUnkMod->GetClientAlphaProperty() : NULL );

	info.m_Flags = 0;

	switch(nType) {
	case RENDERABLE_IS_OPAQUE:
		info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
		break;
	case RENDERABLE_IS_TWO_PASS:
		info.m_Flags |= RENDER_FLAGS_TWOPASS;
		[[fallthrough]];
	case RENDERABLE_IS_TRANSLUCENT:
		if(pAlphaProp && pAlphaProp->IgnoresZBuffer())
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ;
		else
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT;
		break;
	default:
		Assert(0);
		break;
	}

	switch(nModelType) {
	case RENDERABLE_MODEL_STUDIOMDL:
		Assert(modelType == mod_studio);
		info.m_Flags |= RENDER_FLAGS_STUDIO_MODEL;
		[[fallthrough]];
	case RENDERABLE_MODEL_ENTITY:
		switch(nType) {
		case RENDERABLE_IS_OPAQUE:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			break;
		case RENDERABLE_IS_TWO_PASS:
		case RENDERABLE_IS_TRANSLUCENT:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		default:
			Assert(0);
			break;
		}
		info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
		break;
	case RENDERABLE_MODEL_STATIC_PROP:
		Assert(modelType == mod_studio);
		info.m_Flags |= RENDER_FLAGS_STATIC_PROP;
		switch(nType) {
		case RENDERABLE_IS_OPAQUE:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_STATIC;
			break;
		case RENDERABLE_IS_TWO_PASS:
		case RENDERABLE_IS_TRANSLUCENT:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		default:
			Assert(0);
			break;
		}
		info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_STATIC;
		break;
	case RENDERABLE_MODEL_BRUSH:
		Assert(modelType == mod_brush);
		info.m_Flags |= RENDER_FLAGS_BRUSH_MODEL;
		switch(nType) {
		case RENDERABLE_IS_OPAQUE:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_BRUSH;
			break;
		case RENDERABLE_IS_TWO_PASS:
		case RENDERABLE_IS_TRANSLUCENT:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		default:
			Assert(0);
			break;
		}
		info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_BRUSH;
		break;
	default:
		info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OTHER;
		info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OTHER;
		break;
	}

	info.m_pRenderable = pRenderable;
	info.m_pRenderableMod = dynamic_cast<IClientRenderableMod *>( pRenderable );
	info.m_pAlphaProperty = pAlphaProp;
	info.m_nRenderFrame = -1;
	info.m_TranslucencyCalculated = -1;
	info.m_TranslucencyCalculatedView = VIEW_ILLEGAL;
	info.m_FirstShadow = m_ShadowsOnRenderable.InvalidIndex();
	info.m_LeafList = m_RenderablesInLeaf.InvalidIndex();
	info.m_bDisableShadowDepthRendering = false;
	info.m_bDisableShadowDepthCaching = false;
	info.m_EnumCount = 0;
	info.m_RenderLeaf = m_RenderablesInLeaf.InvalidIndex();
	info.m_nTranslucencyType = nType;
	info.m_nModelType = nModelType;
	info.m_vecBloatedAbsMins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	info.m_vecBloatedAbsMaxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	info.m_vecAbsMins.Init();
	info.m_vecAbsMaxs.Init();

#if defined STAGING_ONLY && 0
	DevMsg("CClientLeafSystem::NewRenderable %p\n", info.m_pRenderable);
	dump_render_info(info);
#endif

	pRenderable->RenderHandle() = handle;

	RenderWithViewModels( handle, bRenderWithViewModels );
}

void CClientLeafSystem::DO_NOT_USE_CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp )
{
	RenderableModelType_t nModelType = RENDERABLE_MODEL_UNKNOWN_TYPE;
	if(bIsStaticProp) {
		nModelType = RENDERABLE_MODEL_STATIC_PROP;
	} else {
		// We need to know if it's a brush model for shadows
		int modelType = modelinfo->GetModelType( pRenderable->GetModel() );
		switch(modelType) {
		case mod_sprite:
			nModelType = RENDERABLE_MODEL_ENTITY;
			break;
		case mod_brush:
			nModelType = RENDERABLE_MODEL_BRUSH;
			break;
		case mod_studio:
			nModelType = RENDERABLE_MODEL_STUDIOMDL;
			break;
		default:
			Assert(0);
			break;
		}
	}

	RenderableTranslucencyType_t nType = RENDERABLE_IS_OPAQUE;

	IClientRenderableMod *pRenderMod = dynamic_cast<IClientRenderableMod *>( pRenderable );
	if(pRenderMod) {
		nType = pRenderMod->ComputeTranslucencyType();
	} else {
		if(pRenderable->DO_NOT_USE_IsTransparent()) {
			if(pRenderable->DO_NOT_USE_IsTwoPass()) {
				nType = RENDERABLE_IS_TWO_PASS;
			} else {
				nType = RENDERABLE_IS_TRANSLUCENT;
			}
		}
	}

	NewRenderable( pRenderable, false, nType, nModelType );
}

void CClientLeafSystem::CreateRenderableHandle( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType )
{
	NewRenderable( pRenderable, bRenderWithViewModels, nType, nModelType );
}

void CClientLeafSystem::AddRenderable( IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType )
{
	// force a relink we we try to draw it for the first time
	NewRenderable( pRenderable, bRenderWithViewModels, nType, nModelType );
	ClientRenderHandle_t handle = pRenderable->RenderHandle();
	RenderableChanged( handle );
}

//-----------------------------------------------------------------------------
// Call this if the model changes
//-----------------------------------------------------------------------------
void CClientLeafSystem::SetTranslucencyType( ClientRenderHandle_t handle, RenderableTranslucencyType_t nType )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];

	if(info.m_nTranslucencyType != nType)
	{
		info.m_nTranslucencyType = nType;

		if(nType == RENDERABLE_IS_TWO_PASS) {
			info.m_Flags |= RENDER_FLAGS_TWOPASS;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_TWOPASS;
		}

		switch(nType) {
		case RENDERABLE_IS_OPAQUE:
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
			break;
		case RENDERABLE_IS_TWO_PASS:
		case RENDERABLE_IS_TRANSLUCENT:
			if(info.m_pAlphaProperty && info.m_pAlphaProperty->IgnoresZBuffer())
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ;
			else
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT;
			break;
		default:
			Assert(0);
			break;
		}

		if(info.IsStaticProp()) {
			if(!IsEngineRenderGroupOpaqueStatic(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_STATIC;
		} else if(info.IsBrushModel()) {
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_BRUSH;
		} else if(info.IsEntity() || info.IsStudioModel()) {
			if(!IsEngineRenderGroupOpaqueEntity(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
		} else {
			Assert(0);
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OTHER;
		}

		switch(nType) {
		case RENDERABLE_IS_OPAQUE:
			info.m_EngineRenderGroup = info.m_EngineRenderGroupOpaque;
			break;
		case RENDERABLE_IS_TWO_PASS:
		case RENDERABLE_IS_TRANSLUCENT:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		default:
			Assert(0);
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OTHER;
			break;
		}
	}
}

void CClientLeafSystem::DisableShadowDepthRendering(ClientRenderHandle_t handle, bool bDisable)
{
	if (handle == INVALID_CLIENT_RENDER_HANDLE)
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if (bDisable != info.m_bDisableShadowDepthRendering)
	{
		info.m_bDisableShadowDepthRendering = bDisable;
		m_nDisableShadowDepthCount += bDisable ? 1 : -1;
		Assert(m_nDisableShadowDepthCount >= 0);
	}
}

void CClientLeafSystem::DisableShadowDepthCaching(ClientRenderHandle_t handle, bool bDisable)
{
	if (handle == INVALID_CLIENT_RENDER_HANDLE)
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if (bDisable != info.m_bDisableShadowDepthCaching)
	{
		info.m_bDisableShadowDepthCaching = bDisable;
		m_nDisableShadowDepthCacheCount += bDisable ? 1 : -1;
		Assert(m_nDisableShadowDepthCacheCount >= 0);
	}
}

RenderableTranslucencyType_t CClientLeafSystem::GetTranslucencyType( ClientRenderHandle_t handle ) const
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return RENDERABLE_IS_OPAQUE;

	const RenderableInfo_t &info = m_Renderables[(uint)handle];
	return (RenderableTranslucencyType_t)info.m_nTranslucencyType;
}

//-----------------------------------------------------------------------------
// Used to change renderables from translucent to opaque
//-----------------------------------------------------------------------------
void CClientLeafSystem::DO_NOT_USE_ChangeRenderableRenderGroup( ClientRenderHandle_t handle, EngineRenderGroup_t group )
{
	RenderableInfo_t &info = m_Renderables[(uint)handle];

	if(info.m_EngineRenderGroup != group)
	{
		if(IsEngineRenderGroupStatic(group)) {
			info.m_Flags |= RENDER_FLAGS_STATIC_PROP;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_STATIC_PROP;
		}

		if(group == ENGINE_RENDER_GROUP_TWOPASS) {
			info.m_Flags |= RENDER_FLAGS_TWOPASS;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_TWOPASS;
		}

		if(IsEngineRenderGroupViewModel(group)) {
			RenderWithViewModels(handle, true);
		} else {
			RenderWithViewModels(handle, false);
		}

		switch(group) {
		case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
		case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
		case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
		case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
			info.m_EngineRenderGroup = group;
			info.m_EngineRenderGroupOpaque = group;
			info.m_nTranslucencyType = RENDERABLE_IS_OPAQUE;
			info.m_nModelType = RENDERABLE_MODEL_STATIC_PROP;
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
			break;
		case ENGINE_RENDER_GROUP_TWOPASS:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			if(!IsEngineRenderGroupOpaqueEntity(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			info.m_nTranslucencyType = RENDERABLE_IS_TWO_PASS;
			info.m_nModelType = RENDERABLE_MODEL_ENTITY;
			if(info.m_pAlphaProperty && info.m_pAlphaProperty->IgnoresZBuffer())
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ;
			else
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT;
			break;
		case ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY:
			info.m_EngineRenderGroup = group;
			if(!IsEngineRenderGroupOpaqueEntity(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			info.m_nTranslucencyType = RENDERABLE_IS_TRANSLUCENT;
			info.m_nModelType = RENDERABLE_MODEL_ENTITY;
			if(info.m_pAlphaProperty && info.m_pAlphaProperty->IgnoresZBuffer())
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ;
			else
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT;
			break;
		case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
		case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
		case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
		case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
			info.m_EngineRenderGroup = group;
			info.m_EngineRenderGroupOpaque = group;
			info.m_nTranslucencyType = RENDERABLE_IS_OPAQUE;
			info.m_nModelType = RENDERABLE_MODEL_ENTITY;
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
			break;
		case ENGINE_RENDER_GROUP_OPAQUE_BRUSH:
			info.m_EngineRenderGroup = group;
			info.m_EngineRenderGroupOpaque = group;
			info.m_nTranslucencyType = RENDERABLE_IS_OPAQUE;
			info.m_nModelType = RENDERABLE_MODEL_BRUSH;
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
			break;
		case ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			info.m_nTranslucencyType = RENDERABLE_IS_TRANSLUCENT;
			info.m_nModelType = RENDERABLE_MODEL_ENTITY;
			if(info.m_pAlphaProperty && info.m_pAlphaProperty->IgnoresZBuffer())
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT_IGNOREZ;
			else
				info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_TRANSLUCENT;
			break;
		case ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE:
			if(!IsEngineRenderGroupOpaqueEntity(info.m_EngineRenderGroup))
				info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			if(!IsEngineRenderGroupOpaqueEntity(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			info.m_nTranslucencyType = RENDERABLE_IS_OPAQUE;
			info.m_nModelType = RENDERABLE_MODEL_ENTITY;
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_OPAQUE;
			break;
		case ENGINE_RENDER_GROUP_OTHER:
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OTHER;
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OTHER;
			info.m_nTranslucencyType = RENDERABLE_IS_TRANSLUCENT;
			info.m_nModelType = RENDERABLE_MODEL_UNKNOWN_TYPE;
			info.m_ClientRenderGroup = CLIENT_RENDER_GROUP_COUNT;
			break;
		}
	}
}

void CClientLeafSystem::SetModelType( ClientRenderHandle_t handle, RenderableModelType_t nModelType )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( nModelType == RENDERABLE_MODEL_UNKNOWN_TYPE )
	{
		int nType = modelinfo->GetModelType( info.m_pRenderable->GetModel() );
		switch( nType )
		{
		case mod_sprite:
			nModelType = RENDERABLE_MODEL_ENTITY;
			break;
		case mod_brush:
			nModelType = RENDERABLE_MODEL_BRUSH;
			break;
		case mod_studio:
			nModelType = RENDERABLE_MODEL_STUDIOMDL;
			break;
		default:
			Assert(0);
			break;
		}
	}

	if ( info.m_nModelType != nModelType )
	{
		info.m_nModelType = nModelType;

		if(nModelType == RENDERABLE_MODEL_STUDIOMDL) {
			info.m_Flags |= RENDER_FLAGS_STUDIO_MODEL;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_STUDIO_MODEL;
		}

		if(nModelType == RENDERABLE_MODEL_STATIC_PROP) {
			info.m_Flags |= RENDER_FLAGS_STATIC_PROP;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_STATIC_PROP;
		}

		if(nModelType == RENDERABLE_MODEL_BRUSH) {
			info.m_Flags |= RENDER_FLAGS_BRUSH_MODEL;
		} else {
			info.m_Flags &= ~RENDER_FLAGS_BRUSH_MODEL;
		}

		switch(nModelType) {
		case RENDERABLE_MODEL_STUDIOMDL:
		case RENDERABLE_MODEL_ENTITY:
			if(!IsEngineRenderGroupOpaqueStatic(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			if(info.IsOpaque()) {
				if(!IsEngineRenderGroupOpaqueStatic(info.m_EngineRenderGroup))
					info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_ENTITY;
			} else
				info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		case RENDERABLE_MODEL_STATIC_PROP:
			if(!IsEngineRenderGroupOpaqueStatic(info.m_EngineRenderGroupOpaque))
				info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_STATIC;
			if(info.IsOpaque()) {
				if(!IsEngineRenderGroupOpaqueStatic(info.m_EngineRenderGroup))
					info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_STATIC;
			} else
				info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		case RENDERABLE_MODEL_BRUSH:
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OPAQUE_BRUSH;
			if(info.IsOpaque())
				info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OPAQUE_BRUSH;
			else
				info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY;
			break;
		default:
			info.m_EngineRenderGroupOpaque = ENGINE_RENDER_GROUP_OTHER;
			info.m_EngineRenderGroup = ENGINE_RENDER_GROUP_OTHER;
			break;
		}

	#if defined STAGING_ONLY && 0
		DevMsg("CClientLeafSystem::SetModelType %p\n", info.m_pRenderable);
		dump_render_info(info);
	#endif

		RenderableChanged( handle );
	}
}

void CClientLeafSystem::EnableRendering( ClientRenderHandle_t handle, bool bEnable )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( bEnable )
	{
		if((info.m_Flags & RENDER_FLAGS_DISABLE_RENDERING) != 0) {
			info.m_Flags &= ~RENDER_FLAGS_DISABLE_RENDERING;
		}
	}
	else
	{
		if((info.m_Flags & RENDER_FLAGS_DISABLE_RENDERING) == 0) {
			info.m_Flags |= RENDER_FLAGS_DISABLE_RENDERING;
		}
	}
}

void CClientLeafSystem::EnableBloatedBounds( ClientRenderHandle_t handle, bool bEnable )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( bEnable )
	{
		if((info.m_Flags & RENDER_FLAGS_BLOAT_BOUNDS) == 0) {
			info.m_Flags |= RENDER_FLAGS_BLOAT_BOUNDS;
		}
	}
	else
	{
		if ( (info.m_Flags & RENDER_FLAGS_BLOAT_BOUNDS) != 0 )
		{
			info.m_Flags &= ~RENDER_FLAGS_BLOAT_BOUNDS;

			// Necessary to generate unbloated bounds later
			RenderableChanged( handle );
		}
	}
}

void CClientLeafSystem::DisableCachedRenderBounds( ClientRenderHandle_t handle, bool bDisable )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( bDisable )
	{
		if((info.m_Flags & RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE) == 0) {
			info.m_Flags |= RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE;
		}
	}
	else
	{
		if ( (info.m_Flags & RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE) != 0 )
		{
			info.m_Flags &= ~RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE;
		}
	}
}

//-----------------------------------------------------------------------------
// Use alternate translucent sorting algorithm (draw translucent objects in the furthest leaf they lie in)
//-----------------------------------------------------------------------------
void CClientLeafSystem::EnableAlternateSorting( ClientRenderHandle_t handle, bool bEnable )
{
	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( bEnable )
	{
		if ( ( info.m_Flags & RENDER_FLAGS_ALTERNATE_SORTING ) == 0 )
		{
			++m_nAlternateSortCount;
			info.m_Flags |= RENDER_FLAGS_ALTERNATE_SORTING;
		}
	}
	else
	{
		if ( ( info.m_Flags & RENDER_FLAGS_ALTERNATE_SORTING ) != 0 )
		{
			--m_nAlternateSortCount;
			info.m_Flags &= ~RENDER_FLAGS_ALTERNATE_SORTING;
		}
	}
}

//-----------------------------------------------------------------------------
// Should this render with viewmodels?
//-----------------------------------------------------------------------------
void CClientLeafSystem::RenderWithViewModels( ClientRenderHandle_t handle, bool bEnable )
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return;

	RenderableInfo_t &info = m_Renderables[(uint)handle];
	if ( bEnable )
	{
		if((info.m_Flags & RENDER_FLAGS_RENDER_WITH_VIEWMODELS) == 0)
		{
			info.m_Flags |= RENDER_FLAGS_RENDER_WITH_VIEWMODELS;

			AddToViewModelList( handle );
			RemoveFromTree( handle );
		}
	}
	else
	{
		if((info.m_Flags & RENDER_FLAGS_RENDER_WITH_VIEWMODELS ) != 0)
		{
			info.m_Flags &= ~RENDER_FLAGS_RENDER_WITH_VIEWMODELS;

			RemoveFromViewModelList( handle );
			RenderableChanged( handle );
		}
	}
}

bool CClientLeafSystem::IsRenderingWithViewModels( ClientRenderHandle_t handle ) const
{
	if ( handle == INVALID_CLIENT_RENDER_HANDLE )
		return false;

	const RenderableInfo_t &info = m_Renderables[(uint)handle];
	return info.RendersWithViewmodels();
}

//-----------------------------------------------------------------------------
// Add/remove renderable
//-----------------------------------------------------------------------------
void CClientLeafSystem::RemoveRenderable( ClientRenderHandle_t handle )
{
	// This can happen upon level shutdown
	if (!m_Renderables.IsValidIndex((uint)handle))
		return;

	// Reset the render handle in the entity.
	IClientRenderable *pRenderable = m_Renderables[(uint)handle].m_pRenderable;
	Assert( handle == pRenderable->RenderHandle() );
	pRenderable->RenderHandle() = INVALID_CLIENT_RENDER_HANDLE;

	int nFlags = m_Renderables[(uint)handle].m_Flags;
	if ( nFlags & RENDER_FLAGS_ALTERNATE_SORTING )
	{
		--m_nAlternateSortCount;
	}

	if (m_Renderables[(uint)handle].m_bDisableShadowDepthRendering)
	{
		--m_nDisableShadowDepthCount;
	}

	if (m_Renderables[(uint)handle].m_bDisableShadowDepthCaching)
	{
		--m_nDisableShadowDepthCacheCount;
	}

	// Reemove the renderable from the dirty list
	if ( nFlags & RENDER_FLAGS_HASCHANGED )
	{
		// NOTE: This isn't particularly fast (linear search),
		// but I'm assuming it's an unusual case where we remove 
		// renderables that are changing or that m_DirtyRenderables usually
		// only has a couple entries
		int i = m_DirtyRenderables.Find( handle );
		Assert( i != m_DirtyRenderables.InvalidIndex() );
		m_DirtyRenderables.FastRemove( i ); 
	}

	if ( IsRenderingWithViewModels( handle ) )
	{
		RemoveFromViewModelList( handle );
	}

	RemoveFromTree( handle );
	m_Renderables.Remove( (uint)handle );
}


int CClientLeafSystem::GetRenderableLeaves( ClientRenderHandle_t handle, int leaves[128] )
{
	if ( !m_Renderables.IsValidIndex( (uint)handle ) )
		return -1;

	RenderableInfo_t *pRenderable = &m_Renderables[(uint)handle];
	if ( pRenderable->m_LeafList == m_RenderablesInLeaf.InvalidIndex() )
		return -1;

	int nLeaves = 0;
	for ( int i=m_RenderablesInLeaf.FirstBucket( handle ); i != m_RenderablesInLeaf.InvalidIndex(); i = m_RenderablesInLeaf.NextBucket( i ) )
	{
		leaves[nLeaves++] = m_RenderablesInLeaf.Bucket( i );
		if ( nLeaves >= 128 )
			break;
	}
	return nLeaves;
}


//-----------------------------------------------------------------------------
// Retrieve leaf handles to leaves a renderable is in
// the pOutLeaf parameter is filled with the leaf the renderable is in.
// If pInIterator is not specified, pOutLeaf is the first leaf in the list.
// if pInIterator is specified, that iterator is used to return the next leaf
// in the list in pOutLeaf.
// the pOutIterator parameter is filled with the iterater which index to the pOutLeaf returned.
//
// Returns false on failure cases where pOutLeaf will be invalid. CHECK THE RETURN!
//-----------------------------------------------------------------------------
bool CClientLeafSystem::GetRenderableLeaf(ClientRenderHandle_t handle, int* pOutLeaf, const int* pInIterator /* = 0 */, int* pOutIterator /* = 0  */)
{
	// bail on invalid handle
	if ( !m_Renderables.IsValidIndex( (uint)handle ) )
		return false;

	// bail on no output value pointer
	if ( !pOutLeaf )
		return false;

	// an iterator was specified
	if ( pInIterator )
	{
		int iter = *pInIterator;

		// test for invalid iterator
		if ( iter == m_RenderablesInLeaf.InvalidIndex() )
			return false;

		int iterNext =  m_RenderablesInLeaf.NextBucket( iter );

		// test for end of list
		if ( iterNext == m_RenderablesInLeaf.InvalidIndex() )
			return false;

		// Give the caller the iterator used
		if ( pOutIterator )
		{
			*pOutIterator = iterNext;
		}
		
		// set output value to the next leaf
		*pOutLeaf = m_RenderablesInLeaf.Bucket( iterNext );

	}
	else // no iter param, give them the first bucket in the renderable's list
	{
		int iter = m_RenderablesInLeaf.FirstBucket( handle );

		if ( iter == m_RenderablesInLeaf.InvalidIndex() )
			return false;

		// Set output value to this leaf
		*pOutLeaf = m_RenderablesInLeaf.Bucket( iter );

		// give this iterator to caller
		if ( pOutIterator )
		{
			*pOutIterator = iter;
		}
		
	}
	
	return true;
}

bool CClientLeafSystem::IsRenderableInPVS( IClientRenderable *pRenderable )
{
	ClientRenderHandle_t handle = pRenderable->RenderHandle();
	int leaves[128];
	int nLeaves = GetRenderableLeaves( handle, leaves );
	if ( nLeaves == -1 )
		return false;

	// Ask the engine if this guy is visible.
	return render->AreAnyLeavesVisible( leaves, nLeaves );
}

short CClientLeafSystem::GetRenderableArea( ClientRenderHandle_t handle )
{
	int leaves[128];
	int nLeaves = GetRenderableLeaves( handle, leaves );
	if ( nLeaves == -1 )
		return 0;

	// Now ask the 
	return engine->GetLeavesArea( leaves, nLeaves );
}


void CClientLeafSystem::SetSubSystemDataInLeaf( int leaf, int nSubSystemIdx, CClientLeafSubSystemData *pData )
{
	assert( nSubSystemIdx < N_CLSUBSYSTEMS );
	if ( m_Leaf[leaf].m_pSubSystemData[nSubSystemIdx] )
		delete m_Leaf[leaf].m_pSubSystemData[nSubSystemIdx];
	m_Leaf[leaf].m_pSubSystemData[nSubSystemIdx] = pData;
}

CClientLeafSubSystemData *CClientLeafSystem::GetSubSystemDataInLeaf( int leaf, int nSubSystemIdx )
{
	assert( nSubSystemIdx < N_CLSUBSYSTEMS );
	return m_Leaf[leaf].m_pSubSystemData[nSubSystemIdx];
}

//-----------------------------------------------------------------------------
// Indicates which leaves detail objects are in
//-----------------------------------------------------------------------------
void CClientLeafSystem::SetDetailObjectsInLeaf( int leaf, int firstDetailObject,
											    int detailObjectCount )
{
	m_Leaf[leaf].m_FirstDetailProp = firstDetailObject;
	m_Leaf[leaf].m_DetailPropCount = detailObjectCount;
}

//-----------------------------------------------------------------------------
// Returns the detail objects in a leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::GetDetailObjectsInLeaf( int leaf, int& firstDetailObject,
											    int& detailObjectCount )
{
	firstDetailObject = m_Leaf[leaf].m_FirstDetailProp;
	detailObjectCount = m_Leaf[leaf].m_DetailPropCount;
}


//-----------------------------------------------------------------------------
// Create/destroy shadows...
//-----------------------------------------------------------------------------
ClientLeafShadowHandle_t CClientLeafSystem::AddShadow( ClientShadowHandle_t userId, unsigned short flags )
{
	ClientLeafShadowHandle_t idx = (ClientLeafShadowHandle_t)m_Shadows.AddToTail();
	m_Shadows[(uint)idx].m_Shadow = userId;
	m_Shadows[(uint)idx].m_FirstLeaf = m_ShadowsInLeaf.InvalidIndex();
	m_Shadows[(uint)idx].m_FirstRenderable = m_ShadowsOnRenderable.InvalidIndex();
	m_Shadows[(uint)idx].m_EnumCount = 0;
	m_Shadows[(uint)idx].m_Flags = flags;
	return idx;
}

void CClientLeafSystem::RemoveShadow( ClientLeafShadowHandle_t handle )
{
	if(handle == CLIENT_LEAF_SHADOW_INVALID_HANDLE)
		return;

	// Remove the shadow from all leaves + renderables...
	RemoveShadowFromLeaves( handle );
	RemoveShadowFromRenderables( handle );

	// Blow away the handle
	m_Shadows.Remove( (uint)handle );
}


//-----------------------------------------------------------------------------
// Adds a shadow to a leaf/removes shadow from renderable
//-----------------------------------------------------------------------------
inline bool CClientLeafSystem::ShouldRenderableReceiveShadow( ClientRenderHandle_t renderHandle, int nShadowFlags )
{
	RenderableInfo_t &renderable = m_Renderables[(uint)renderHandle];
	if( !renderable.IsBrushModel() && !renderable.IsStaticProp() && !renderable.IsStudioModel() )
		return false;

	return renderable.m_pRenderable->ShouldReceiveProjectedTextures( nShadowFlags );
}


//-----------------------------------------------------------------------------
// Adds a shadow to a leaf/removes shadow from renderable
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddShadowToRenderable( ClientRenderHandle_t renderHandle, 
										ClientLeafShadowHandle_t shadowHandle )
{
	// Check if this renderable receives the type of projected texture that shadowHandle refers to.
	int nShadowFlags = m_Shadows[(uint)shadowHandle].m_Flags;
	if ( !ShouldRenderableReceiveShadow( renderHandle, nShadowFlags ) )
		return;

	m_ShadowsOnRenderable.AddElementToBucket( renderHandle, shadowHandle );

	// Also, do some stuff specific to the particular types of renderables
	// Do AddShadowToReceiver to avoid branching
	static const byte arrRecvType[0x4] = {
		0,
		SHADOW_RECEIVER_STUDIO_MODEL,
		SHADOW_RECEIVER_STATIC_PROP,
		SHADOW_RECEIVER_BRUSH_MODEL
	};
	COMPILE_TIME_ASSERT( RENDERABLE_MODEL_STUDIOMDL == 1 );
	COMPILE_TIME_ASSERT( RENDERABLE_MODEL_STATIC_PROP == 2 );
	COMPILE_TIME_ASSERT( RENDERABLE_MODEL_BRUSH == 3 );

	RenderableInfo_t const &ri = m_Renderables[(uint)renderHandle];
	if ( ri.GetModelType() < ARRAYSIZE( arrRecvType ) )
	{
		g_pClientShadowMgr->AddShadowToReceiver(
			m_Shadows[(uint)shadowHandle].m_Shadow,
			ri.m_pRenderable,
			( ShadowReceiver_t ) arrRecvType[ ri.GetModelType() ] );
	}
}

void CClientLeafSystem::RemoveShadowFromRenderables( ClientLeafShadowHandle_t handle )
{
	m_ShadowsOnRenderable.RemoveElement( handle );
}


//-----------------------------------------------------------------------------
// Adds a shadow to a leaf/removes shadow from leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddShadowToLeaf( int leaf, ClientLeafShadowHandle_t shadow, bool bFlashlight )
{
	m_ShadowsInLeaf.AddElementToBucket( leaf, shadow ); 

	if ( !( bFlashlight || r_shadows_on_renderables_enable.GetBool() ) )
	{
		return;
	}

	// Add the shadow exactly once to all renderables in the leaf
	unsigned int i = m_RenderablesInLeaf.FirstElement( leaf );
	while ( i != m_RenderablesInLeaf.InvalidIndex() )
	{
		ClientRenderHandle_t renderable = m_RenderablesInLeaf.Element(i);
		RenderableInfo_t& info = m_Renderables[(uint)renderable];

		// Add each shadow exactly once to each renderable
		if (info.m_EnumCount != m_ShadowEnum)
		{
			AddShadowToRenderable( renderable, shadow );
			info.m_EnumCount = m_ShadowEnum;
		}

		//Assert( m_ShadowsInLeaf.NumAllocated() < 2000 );

		i = m_RenderablesInLeaf.NextElement(i);
	}
}

void CClientLeafSystem::RemoveShadowFromLeaves( ClientLeafShadowHandle_t handle )
{
	m_ShadowsInLeaf.RemoveElement( handle );
}


//-----------------------------------------------------------------------------
// Adds a shadow to all leaves listed
//-----------------------------------------------------------------------------
void CClientLeafSystem::ProjectShadow( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList )
{
	// Remove the shadow from any leaves it current exists in
	RemoveShadowFromLeaves( handle );
	RemoveShadowFromRenderables( handle );

	Assert( ( m_Shadows[(uint)handle].m_Flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) == SHADOW_FLAGS_SHADOW );

	// This will help us to avoid adding the shadow multiple times to a renderable
	++m_ShadowEnum;

	for ( int i = 0; i < nLeafCount; ++i )
	{
		AddShadowToLeaf( pLeafList[i], handle, false );
	}
}

void CClientLeafSystem::ProjectFlashlight( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList )
{
	VPROF_BUDGET( "CClientLeafSystem::ProjectFlashlight", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// Remove the shadow from any leaves it current exists in
	RemoveShadowFromLeaves( handle );
	RemoveShadowFromRenderables( handle );

	Assert( ( m_Shadows[(uint)handle].m_Flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) == SHADOW_FLAGS_FLASHLIGHT );
	
	// This will help us to avoid adding the shadow multiple times to a renderable
	++m_ShadowEnum;

	for ( int i = 0; i < nLeafCount; ++i )
	{
		AddShadowToLeaf( pLeafList[i], handle, true );
	}
}


//-----------------------------------------------------------------------------
// Find all shadow casters in a set of leaves
//-----------------------------------------------------------------------------
void CClientLeafSystem::EnumerateShadowsInLeaves( int leafCount, LeafIndex_t* pLeaves, IClientLeafShadowEnum* pEnum )
{
	if (leafCount == 0)
		return;

	// This will help us to avoid enumerating the shadow multiple times
	++m_ShadowEnum;

	for (int i = 0; i < leafCount; ++i)
	{
		int leaf = pLeaves[i];

		unsigned short j = m_ShadowsInLeaf.FirstElement( leaf );
		while ( j != m_ShadowsInLeaf.InvalidIndex() )
		{
			ClientLeafShadowHandle_t shadow = m_ShadowsInLeaf.Element(j);
			ShadowInfo_t& info = m_Shadows[(uint)shadow];

			if (info.m_EnumCount != m_ShadowEnum)
			{
				pEnum->EnumShadow(info.m_Shadow);
				info.m_EnumCount = m_ShadowEnum;
			}

			j = m_ShadowsInLeaf.NextElement(j);
		}
	}
}


//-----------------------------------------------------------------------------
// Adds a renderable to a leaf
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddRenderableToLeaf( int leaf, ClientRenderHandle_t renderable, bool bReceiveShadows )
{
#ifdef VALIDATE_CLIENT_LEAF_SYSTEM
	m_RenderablesInLeaf.ValidateAddElementToBucket( leaf, renderable );
#endif

#ifdef DUMP_RENDERABLE_LEAFS
	static uint32 count = 0;
	if (count < m_RenderablesInLeaf.NumAllocated())
	{
		count = m_RenderablesInLeaf.NumAllocated();
		Msg("********** frame: %d count:%u ***************\n", gpGlobals->framecount, count );

		if (count >= 20000)
		{
			for (int j = 0; j < m_RenderablesInLeaf.NumAllocated(); j++)
			{
				const ClientRenderHandle_t& renderable = m_RenderablesInLeaf.Element(j);
				RenderableInfo_t& info = m_Renderables[renderable];

				char pTemp[256];
				const char *pClassName = "<unknown renderable>";
				C_BaseEntity *pEnt = info.m_pRenderable->GetIClientUnknown()->GetBaseEntity();
				if ( pEnt )
				{
					pClassName = pEnt->GetClassname();
				}
				else
				{
					CNewParticleEffect *pEffect = dynamic_cast< CNewParticleEffect*>( info.m_pRenderable );
					if ( pEffect )
					{
						Vector mins, maxs;
						pEffect->GetRenderBounds(mins, maxs);
						Q_snprintf( pTemp, sizeof(pTemp), "ps: %s %.2f,%.2f", pEffect->GetEffectName(), maxs.x - mins.x, maxs.y - mins.y );
						pClassName = pTemp;
					}
					else if ( dynamic_cast< CParticleEffectBinding* >( info.m_pRenderable ) )
					{
						pClassName = "<old particle system>";
					}
				}

				Msg(" %d: %p group:%d %s %d %d TransCalc:%d renderframe:%d\n", j, info.m_pRenderable, info.m_RenderGroup, pClassName,
					info.m_LeafList, info.m_RenderLeaf, info.m_TranslucencyCalculated, info.m_RenderFrame);
			}

			DebuggerBreak();
		}
	}
#endif // DUMP_RENDERABLE_LEAFS

	m_RenderablesInLeaf.AddElementToBucket(leaf, renderable);

	bool bShadowsOnRenderables = r_shadows_on_renderables_enable.GetBool();

	if ( !bReceiveShadows )
	{
		return;
	}

	if ( bShadowsOnRenderables )
	{
		// skipping this code entirely is only safe with single-pass flashlight (i.e. on the 360)

		// Add all shadows in the leaf to the renderable...
		unsigned short i = m_ShadowsInLeaf.FirstElement( leaf );
		while ( i != m_ShadowsInLeaf.InvalidIndex() )
		{
			ClientLeafShadowHandle_t shadow = m_ShadowsInLeaf.Element(i);
			ShadowInfo_t& info = m_Shadows[(uint)shadow];

			// Add each shadow exactly once to each renderable
			if ( info.m_EnumCount != m_ShadowEnum )
			{
				AddShadowToRenderable( renderable, shadow );
				info.m_EnumCount = m_ShadowEnum;
			}

			i = m_ShadowsInLeaf.NextElement(i);
		}
	}
	else /* if ( !bShadowsOnRenderables ) */
	{
		// for non-singlepass flashlight (i.e. PC) we need to still add all flashlights to the renderable
		
		// Add all flashlights in the leaf to the renderable...
		unsigned short i = m_ShadowsInLeaf.FirstElement( leaf );
		while ( i != m_ShadowsInLeaf.InvalidIndex() )
		{
			ClientLeafShadowHandle_t shadow = m_ShadowsInLeaf.Element(i);
			ShadowInfo_t& info = m_Shadows[(uint)shadow];

			// Add each flashlight exactly once to each renderable
			if ( ( info.m_Flags & ( SHADOW_FLAGS_FLASHLIGHT | SHADOW_FLAGS_SIMPLE_PROJECTION ) ) && ( info.m_EnumCount != m_ShadowEnum ) )
			{
				AddShadowToRenderable( renderable, shadow );
				info.m_EnumCount = m_ShadowEnum;
			}

			i = m_ShadowsInLeaf.NextElement(i);
		}
	}
}


//-----------------------------------------------------------------------------
// Adds a renderable to a set of leaves
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves, bool bReceiveShadows )
{ 
	for (int j = 0; j < nLeafCount; ++j)
	{
		AddRenderableToLeaf( pLeaves[j], handle, bReceiveShadows ); 
	}
	m_Renderables[(uint)handle].m_Area = GetRenderableArea( handle );
}

void CClientLeafSystem::AddRenderableToLeaves( ClientRenderHandle_t handle, int nLeafCount, unsigned short *pLeaves )
{ 
	bool bReceiveShadows = ShouldRenderableReceiveShadow( handle, SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );
	AddRenderableToLeaves( handle, nLeafCount, pLeaves, bReceiveShadows );
}

//-----------------------------------------------------------------------------
// Inserts an element into the tree
//-----------------------------------------------------------------------------
bool CClientLeafSystem::EnumerateLeaf( int leaf, int context )
{
	EnumResultList_t *pList = (EnumResultList_t *)context;
	if ( ThreadInMainThread() )
	{
		AddRenderableToLeaf( leaf, pList->handle, pList->bReceiveShadows );
	}
	else
	{
		EnumResult_t *p = new EnumResult_t;
		p->leaf = leaf;
		p->pNext = pList->pHead;
		pList->pHead = p;
	}
	return true;
}

void CClientLeafSystem::InsertIntoTree( ClientRenderHandle_t handle, const Vector &absMins, const Vector &absMaxs )
{
	if ( ThreadInMainThread() )
	{
		// When we insert into the tree, increase the shadow enumerator
		// to make sure each shadow is added exactly once to each renderable
		m_ShadowEnum++;
	}

	EnumResultList_t list = { NULL, handle, ShouldRenderableReceiveShadow( handle, SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) };

	// NOTE: The render bounds here are relative to the renderable's coordinate system
	IClientRenderable* pRenderable = m_Renderables[(uint)handle].m_pRenderable;

	Assert( absMins.IsValid() && absMaxs.IsValid() );

	m_Renderables[(uint)handle].m_vecBloatedAbsMins = absMins;
	m_Renderables[(uint)handle].m_vecBloatedAbsMaxs = absMaxs;

	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInBox( absMins, absMaxs, this, (int)&list );

	if ( list.pHead )
	{
		m_DeferredInserts.PushItem( list );
	}
}

//-----------------------------------------------------------------------------
// Removes an element from the tree
//-----------------------------------------------------------------------------
void CClientLeafSystem::RemoveFromTree( ClientRenderHandle_t handle )
{
	m_RenderablesInLeaf.RemoveElement( handle );

	// Remove all shadows cast onto the object
	m_ShadowsOnRenderable.RemoveBucket( handle );

	// If the renderable is a brush model, then remove all shadows from it
	switch( m_Renderables[(uint)handle].GetModelType() )
	{
	case RENDERABLE_MODEL_BRUSH:
		g_pClientShadowMgr->RemoveAllShadowsFromReceiver( 
			m_Renderables[(uint)handle].m_pRenderable, SHADOW_RECEIVER_BRUSH_MODEL );
		break;

	case RENDERABLE_MODEL_STATIC_PROP:
		g_pClientShadowMgr->RemoveAllShadowsFromReceiver( 
			m_Renderables[(uint)handle].m_pRenderable, SHADOW_RECEIVER_STATIC_PROP );
		break;

	case RENDERABLE_MODEL_STUDIOMDL:
		g_pClientShadowMgr->RemoveAllShadowsFromReceiver( 
			m_Renderables[(uint)handle].m_pRenderable, SHADOW_RECEIVER_STUDIO_MODEL );
		break;
	}
}


//-----------------------------------------------------------------------------
// Call this when the renderable moves
//-----------------------------------------------------------------------------
void CClientLeafSystem::RenderableChanged( ClientRenderHandle_t handle )
{
	Assert ( handle != INVALID_CLIENT_RENDER_HANDLE );
	Assert( m_Renderables.IsValidIndex( (uint)handle ) );
	if ( !m_Renderables.IsValidIndex( (uint)handle ) )
		return;

	m_Renderables[(uint)handle].m_Flags &= ~RENDER_FLAGS_BOUNDS_VALID;

	if ( (m_Renderables[(uint)handle].m_Flags & RENDER_FLAGS_HASCHANGED ) == 0 )
	{
		m_Renderables[(uint)handle].m_Flags |= RENDER_FLAGS_HASCHANGED;
		m_DirtyRenderables.AddToTail( handle );
	}
#if _DEBUG
	else
	{
		if ( s_bIsInRecomputeRenderableLeaves )
		{
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "Re-entrancy found in CClientLeafSystem::RenderableChanged\n" );
			Warning( "Contact Shanon or Brian\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
			Warning( "------------------------------------------------------------\n" );
		}
		// It had better be in the list
		Assert( m_DirtyRenderables.Find( handle ) != m_DirtyRenderables.InvalidIndex() );
	}
#endif
}


//-----------------------------------------------------------------------------
// Adds, removes renderables from view model list
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddToViewModelList( ClientRenderHandle_t handle )
{
	MEM_ALLOC_CREDIT();
	Assert( m_ViewModels.Find( handle ) == m_ViewModels.InvalidIndex() );
	m_ViewModels.AddToTail( handle );
}

void CClientLeafSystem::RemoveFromViewModelList( ClientRenderHandle_t handle )
{
	int i = m_ViewModels.Find( handle );
	Assert( i != m_ViewModels.InvalidIndex() );
	m_ViewModels.FastRemove( i );
}


//-----------------------------------------------------------------------------
// Detail system marks 
//-----------------------------------------------------------------------------
void CClientLeafSystem::DrawDetailObjectsInLeaf( int leaf, int nFrameNumber, int& nFirstDetailObject, int& nDetailObjectCount )
{
	ClientLeaf_t &leafInfo = m_Leaf[leaf];
	leafInfo.m_DetailPropRenderFrame = nFrameNumber;
	nFirstDetailObject = leafInfo.m_FirstDetailProp;
	nDetailObjectCount = leafInfo.m_DetailPropCount;
}


//-----------------------------------------------------------------------------
// Are we close enough to this leaf to draw detail props *and* are there any props in the leaf?
//-----------------------------------------------------------------------------
bool CClientLeafSystem::ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber )
{
	if(r_DrawDetailProps.GetInt() == 0)
		return false;

	ClientLeaf_t &leafInfo = m_Leaf[leaf];
	return ( (leafInfo.m_DetailPropRenderFrame == frameNumber ) &&
			 ( ( leafInfo.m_DetailPropCount != 0 ) || ( leafInfo.m_pSubSystemData[CLSUBSYSTEM_DETAILOBJECTS] ) ) );
}


//-----------------------------------------------------------------------------
// Compute which leaf the translucent renderables should render in
//-----------------------------------------------------------------------------
void CClientLeafSystem::ComputeTranslucentRenderLeaf( int count, const LeafIndex_t *pLeafList, const LeafFogVolume_t *pLeafFogVolumeList, int frameNumber, int viewID )
{
	ASSERT_NO_REENTRY();
	VPROF_BUDGET( "CClientLeafSystem::ComputeTranslucentRenderLeaf", "ComputeTranslucentRenderLeaf"  );

	// For better sorting, we're gonna choose the leaf that is closest to the camera.
	// The leaf list passed in here is sorted front to back
	bool bThreaded = ( cl_threaded_client_leaf_system.GetBool() && g_pThreadPool->NumThreads() );
	int globalFrameCount = gpGlobals->framecount;
	int i;

	static CUtlVector<RenderableInfoOrLeaf_t> orderedList; // @MULTICORE (toml 8/30/2006): will need to make non-static if thread this function
	static CUtlVector<IClientRenderable *> renderablesToUpdate;
	int leaf = 0;
	for ( i = 0; i < count; ++i )
	{
		leaf = pLeafList[i];
		orderedList.AddToTail( RenderableInfoOrLeaf_t( leaf ) );

		// iterate over all elements in this leaf
		unsigned int idx = m_RenderablesInLeaf.FirstElement(leaf);
		while (idx != m_RenderablesInLeaf.InvalidIndex())
		{
			RenderableInfo_t& info = m_Renderables[(uint)m_RenderablesInLeaf.Element(idx)];
			if ( info.m_TranslucencyCalculated != globalFrameCount || info.m_TranslucencyCalculatedView != viewID )
			{ 
				// Compute translucency
				if ( bThreaded )
				{
					renderablesToUpdate.AddToTail( info.m_pRenderable );
				}
				else
				{
					if( info.m_pAlphaProperty )
						info.m_pAlphaProperty->ComputeAlphaBlend();
					else
						info.m_pRenderable->DO_NOT_USE_ComputeFxBlend();
				}
				info.m_TranslucencyCalculated = globalFrameCount;
				info.m_TranslucencyCalculatedView = viewID;
			}
			orderedList.AddToTail( &info );
			idx = m_RenderablesInLeaf.NextElement(idx); 
		}
	}

	if ( bThreaded )
	{
		ParallelProcess( "CClientLeafSystem::ComputeTranslucentRenderLeaf", renderablesToUpdate.Base(), renderablesToUpdate.Count(), &CallComputeFXBlend, &::FrameLock, &::FrameUnlock );
		renderablesToUpdate.RemoveAll();
	}

	for ( i = 0; i != orderedList.Count(); i++ )
	{
		RenderableInfoOrLeaf_t infoorleaf = orderedList[i];
		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			if( pInfo->m_nRenderFrame != frameNumber )
			{   
				if( pInfo->IsTranslucent() )
				{
					pInfo->m_RenderLeaf = leaf;
				}
				pInfo->m_nRenderFrame = frameNumber;
			}
			else if ( pInfo->m_Flags & RENDER_FLAGS_ALTERNATE_SORTING )
			{
				if( pInfo->IsTranslucent() )
				{
					pInfo->m_RenderLeaf = leaf;
				}
			}

		}
		else
		{
			leaf = infoorleaf.leaf;
		}
	}

	orderedList.RemoveAll();
}


//-----------------------------------------------------------------------------
// Adds a renderable to the list of renderables to render this frame
//-----------------------------------------------------------------------------
inline void AddRenderableToRenderList( CClientRenderablesList &renderList, const DetailRenderableInfo_t &info, 
	int iLeaf, uint8 nAlphaModulation, ClientRenderHandle_t renderHandle )
{
#ifdef _DEBUG
	if (cl_drawleaf.GetInt() >= 0)
	{
		if (iLeaf != cl_drawleaf.GetInt())
			return;
	}
#endif

	int group_index = CClientRenderablesList::GROUP_COUNT;

	switch(info.m_nEngineRenderGroup) {
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY:
		group_index = CClientRenderablesList::GROUP_OPAQUE_ENTITY;
		break;
	case ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY:
		group_index = CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY;
		break;
	default:
		Assert(0);
		return;
	}

	Assert( group_index >= 0 && group_index < CClientRenderablesList::GROUP_COUNT );

	int &curCount = renderList.m_RenderGroupCounts[group_index];
	if ( curCount < CClientRenderablesList::MAX_GROUP_ENTITIES )
	{
		Assert( (iLeaf >= 0) && (iLeaf <= 65535) );

		CClientRenderablesList::CEntry *pEntry = &renderList.m_RenderGroups[group_index][curCount];
		pEntry->m_pRenderable = info.m_pRenderable;
		pEntry->m_pRenderableMod = info.m_pRenderableMod;
		pEntry->m_iWorldListInfoLeaf = iLeaf;
		pEntry->m_TwoPass = false;
		pEntry->m_RenderHandle = renderHandle;
		pEntry->m_nModelType = RENDERABLE_MODEL_ENTITY;
		pEntry->m_InstanceData.m_nAlpha = nAlphaModulation;
		curCount++;
	}
#ifdef _DEBUG
	else
	{
		engine->Con_NPrintf( 10, "Warning: overflowed CClientRenderablesList group %d", group_index );
	}
#endif
}

enum
{
	ADD_RENDERABLE_TRANSLUCENT,
	ADD_RENDERABLE_OPAQUE,
};

inline void AddRenderableToRenderList( CClientRenderablesList &renderList, const CClientLeafSystem::RenderableInfo_t &info, 
	int iLeaf, uint8 nAlphaModulation, ClientRenderHandle_t renderHandle, bool bTwoPass, int method )
{
#ifdef _DEBUG
	if (cl_drawleaf.GetInt() >= 0)
	{
		if (iLeaf != cl_drawleaf.GetInt())
			return;
	}
#endif

	int group_index = CClientRenderablesList::GROUP_COUNT;

	if(info.IsBrushModel()) {
		if(method == ADD_RENDERABLE_TRANSLUCENT) {
			group_index = CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY;
		} else if(method == ADD_RENDERABLE_OPAQUE) {
			group_index = CClientRenderablesList::GROUP_OPAQUE_BRUSH;
		} else {
			Assert(0);
			return;
		}
	} else if(info.IsStaticProp()) {
		if(method == ADD_RENDERABLE_TRANSLUCENT) {
			group_index = CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY;
		} else if(method == ADD_RENDERABLE_OPAQUE) {
			switch(info.m_EngineRenderGroupOpaque) {
			case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
				group_index = CClientRenderablesList::GROUP_OPAQUE_STATIC_HUGE;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
				group_index = CClientRenderablesList::GROUP_OPAQUE_STATIC_MEDIUM;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
				group_index = CClientRenderablesList::GROUP_OPAQUE_STATIC_SMALL;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
				group_index = CClientRenderablesList::GROUP_OPAQUE_STATIC_TINY;
				break;
			default:
				Assert(0);
				return;
			}
		} else {
			Assert(0);
			return;
		}
	} else if(info.IsEntity() || info.IsStudioModel()) {
		if(method == ADD_RENDERABLE_TRANSLUCENT) {
			group_index = CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY;
		} else if(method == ADD_RENDERABLE_OPAQUE) {
			switch(info.m_EngineRenderGroupOpaque) {
			case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
				group_index = CClientRenderablesList::GROUP_OPAQUE_ENTITY_HUGE;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
				group_index = CClientRenderablesList::GROUP_OPAQUE_ENTITY_MEDIUM;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
				group_index = CClientRenderablesList::GROUP_OPAQUE_ENTITY_SMALL;
				break;
			case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
				group_index = CClientRenderablesList::GROUP_OPAQUE_ENTITY_TINY;
				break;
			default:
				Assert(0);
				return;
			}
		} else {
			Assert(0);
			return;
		}
	} else {
		Assert(0);
		return;
	}

	Assert( group_index >= 0 && group_index < CClientRenderablesList::GROUP_COUNT );
	
	int &curCount = renderList.m_RenderGroupCounts[group_index];
	if ( curCount < CClientRenderablesList::MAX_GROUP_ENTITIES )
	{
		Assert( (iLeaf >= 0) && (iLeaf <= 65535) );

		CClientRenderablesList::CEntry *pEntry = &renderList.m_RenderGroups[group_index][curCount];
		pEntry->m_pRenderable = info.m_pRenderable;
		pEntry->m_pRenderableMod = info.m_pRenderableMod;
		pEntry->m_iWorldListInfoLeaf = iLeaf;
		pEntry->m_TwoPass = bTwoPass;
		pEntry->m_RenderHandle = renderHandle;
		pEntry->m_nModelType = info.GetModelType();
		pEntry->m_InstanceData.m_nAlpha = nAlphaModulation;
		curCount++;
	}
#ifdef _DEBUG
	else
	{
		engine->Con_NPrintf( 10, "Warning: overflowed CClientRenderablesList group %d", group_index );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : renderList - 
//			renderGroup - 
//-----------------------------------------------------------------------------
void CClientLeafSystem::CollateViewModelRenderables( CViewModelRenderablesList *pList )
{
	CViewModelRenderablesList::RenderGroups_t &opaqueList = pList->m_RenderGroups[ CViewModelRenderablesList::GROUP_OPAQUE ];
	CViewModelRenderablesList::RenderGroups_t &translucentList = pList->m_RenderGroups[ CViewModelRenderablesList::GROUP_TRANSLUCENT ];

	for ( int i = m_ViewModels.Count()-1; i >= 0; --i )
	{
		ClientRenderHandle_t handle = m_ViewModels[i];
		RenderableInfo_t& renderable = m_Renderables[(uint)handle];

		int nAlpha = renderable.m_pAlphaProperty ? renderable.m_pAlphaProperty->ComputeRenderAlpha( false ) : 255;
		bool bIsTransparent = ( nAlpha != 255 ) || ( !renderable.IsOpaque() );

		// That's why we need to test RENDER_GROUP_OPAQUE_ENTITY - it may have changed in ComputeFXBlend()
		if ( !bIsTransparent )
		{
			int i = opaqueList.AddToTail();
			CViewModelRenderablesList::CEntry *pEntry = &opaqueList[i];
			pEntry->m_pRenderable = renderable.m_pRenderable;
			pEntry->m_pRenderableMod = renderable.m_pRenderableMod;
			pEntry->m_InstanceData.m_nAlpha = 255;
		}
		else
		{
			int i = translucentList.AddToTail();
			CViewModelRenderablesList::CEntry *pEntry = &translucentList[i];
			pEntry->m_pRenderable = renderable.m_pRenderable;
			pEntry->m_pRenderableMod = renderable.m_pRenderableMod;
			pEntry->m_InstanceData.m_nAlpha = nAlpha;

			if ( renderable.IsTwoPass() )
			{
				int i = opaqueList.AddToTail();
				CViewModelRenderablesList::CEntry *pEntry = &opaqueList[i];
				pEntry->m_pRenderable = renderable.m_pRenderable;
				pEntry->m_pRenderableMod = renderable.m_pRenderableMod;
				pEntry->m_InstanceData.m_nAlpha = 255;
			}
		}
	}
}

static void DetectBucketedRenderGroup( CClientLeafSystem::RenderableInfo_t &info, float fDimension )
{
	COMPILE_TIME_ASSERT(RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS == 4);

	float const arrThresholds[ 3 ] = {
		200.f,	// tree size
		80.f,	// player size
		30.f,	// crate size
	};
	Assert( ARRAYSIZE( arrThresholds ) + 1 == RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS );

	int bucketedGroupIndex;
	if ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS <= 2 ||
		fDimension >= arrThresholds[1] )
	{
		if ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS <= 1 ||
			fDimension >= arrThresholds[0] )
			bucketedGroupIndex = RENDER_GROUP_BUCKET_HUGE;
		else
			bucketedGroupIndex = RENDER_GROUP_BUCKET_MEDIUM;
	}
	else
	{
		if ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS <= 3 ||
			fDimension >= arrThresholds[2] )
			bucketedGroupIndex = RENDER_GROUP_BUCKET_SMALL;
		else
			bucketedGroupIndex = RENDER_GROUP_BUCKET_TINY;
	}

	EngineRenderGroup_t bucketedGroup = ENGINE_RENDER_GROUP_OTHER;

	if(info.IsStaticProp()) {
		bucketedGroup = (EngineRenderGroup_t)ENGINE_RENDER_GROUP_OPAQUE_BUCKETED_STATIC( bucketedGroupIndex );

		Assert( bucketedGroup >= ENGINE_RENDER_GROUP_OPAQUE_BEGIN && bucketedGroup < ENGINE_RENDER_GROUP_OPAQUE_END );
	} else if(info.IsEntity() || info.IsStudioModel()) {
		bucketedGroup = (EngineRenderGroup_t)ENGINE_RENDER_GROUP_OPAQUE_BUCKETED_ENTITY( bucketedGroupIndex );

		Assert( bucketedGroup >= ENGINE_RENDER_GROUP_OPAQUE_BEGIN && bucketedGroup < ENGINE_RENDER_GROUP_OPAQUE_END );
	}

	if(bucketedGroup != ENGINE_RENDER_GROUP_OTHER) {
		info.m_EngineRenderGroupOpaque = bucketedGroup;
	}

	if(info.IsOpaque()) {
		info.m_EngineRenderGroup = bucketedGroup;
	}
}

//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void CClientLeafSystem::SortEntities( const Vector &vecRenderOrigin, const Vector &vecRenderForward, CClientRenderablesList::CEntry *pEntities, int nEntities )
{
	// Don't sort if we only have 1 entity
	if ( nEntities <= 1 )
		return;

	float dists[CClientRenderablesList::MAX_GROUP_ENTITIES];

	// First get a distance for each entity.
	int i;
	for( i=0; i < nEntities; i++ )
	{
		IClientRenderable *pRenderable = pEntities[i].m_pRenderable;

		// Compute the center of the object (needed for translucent brush models)
		Vector boxcenter;
		Vector mins,maxs;
		pRenderable->GetRenderBounds( mins, maxs );
		VectorAdd( mins, maxs, boxcenter );
		VectorMA( pRenderable->GetRenderOrigin(), 0.5f, boxcenter, boxcenter );

		// Compute distance...
		Vector delta;
		VectorSubtract( boxcenter, vecRenderOrigin, delta );
		dists[i] = DotProduct( delta, vecRenderForward );
	}

	// H-sort.
	int stepSize = 4;
	while( stepSize )
	{
		int end = nEntities - stepSize;
		for( i=0; i < end; i += stepSize )
		{
			if( dists[i] > dists[i+stepSize] )
			{
				::V_swap( pEntities[i], pEntities[i+stepSize] );
				::V_swap( dists[i], dists[i+stepSize] );

				if( i == 0 )
				{
					i = -stepSize;
				}
				else
				{
					i -= stepSize << 1;
				}
			}
		}

		stepSize >>= 1;
	}
}

int CClientLeafSystem::ExtractStaticProps( int nCount, RenderableInfoAndHandle_t *ppRenderables )
{
	if ( m_DrawStaticProps )
		return nCount;

	int nUniqueCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			// Early out on static props if we don't want to render them
			if ( pInfo->IsStaticProp() )
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}
		ppRenderables[nUniqueCount++] = ppRenderables[i];
	}
	return nUniqueCount;
}

//-----------------------------------------------------------------------------
// Extracts models which are *not* marked for "fast reflections"
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractDisableShadowDepthRenderables(int nCount, RenderableInfoAndHandle_t *ppRenderables)
{
	if (m_nDisableShadowDepthCount == 0)
		return nCount;

	int nNewCount = 0;
	for (int i = 0; i < nCount; ++i)
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if (!infoorleaf.is_leaf)
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			if (pInfo->m_bDisableShadowDepthRendering)
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}

		ppRenderables[nNewCount++] = ppRenderables[i];
	}

	return nNewCount;
}

//-----------------------------------------------------------------------------
// Extracts models which are cacheable or not depending on what we render now
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractDisableShadowDepthCacheRenderables(int nCount, RenderableInfoAndHandle_t *ppRenderables)
{
	int nNewCount = 0;
	for (int i = 0; i < nCount; ++i)
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if (!infoorleaf.is_leaf)
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			if (!pInfo->m_bDisableShadowDepthCaching)	// this means renderable is in depth cache and shouldn't be rendered again
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}

		ppRenderables[nNewCount++] = ppRenderables[i];
	}

	return nNewCount;
}

int CClientLeafSystem::ExtractDuplicates( int nFrameNumber, int nCount, RenderableInfoAndHandle_t *ppRenderables )
{
	// NOTE: We don't know whether these renderables are translucent or not
	// but we do know if they participate in alternate sorting, which is all we need.
	int nUniqueCount = 0;
	int nLeaf = 0;

	// For better sorting, we're gonna choose the leaf that is closest to the camera.
	// The leaf list passed in here is sorted front to back

	// FIXME: This algorithm won't work in a threaded context since it stores state in renderableinfo_t
	if ( m_nAlternateSortCount == 0 )
	{
		// I expect this is the typical case; nothing needs alternate sorting
		for ( int i = 0; i < nCount; ++i )
		{
			RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

			if ( !infoorleaf.is_leaf )
			{
				RenderableInfo_t *pInfo = infoorleaf.info;

				// Skip these bad boys altogether
				if ( pInfo->RendersWithViewmodels() || pInfo->IsDisabled() )
					continue;

				// If we've seen this already, then we don't need to add it 
				if ( pInfo->m_nRenderFrame == nFrameNumber )
					continue;

				pInfo->m_nRenderFrame = nFrameNumber;
			}
			ppRenderables[nUniqueCount++] = ppRenderables[i];
		}
		return nUniqueCount;
	}

	// Here, we have to worry about alternate sorting. I'm not sure if I
	// can do better than 2n unless I cache off counts of each renderable 
	// in the first loop in BuildRenderablesListV2. I'm doing it this way
	// because I don't believe we'll ever use this path.
	int nAlternateSortCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;
		
		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			// If we've seen this already, then we don't need to add it 
			if ( ( pInfo->m_Flags & RENDER_FLAGS_ALTERNATE_SORTING ) == 0 )
			{
				if( pInfo->m_nRenderFrame == nFrameNumber )
					continue;
				pInfo->m_nRenderFrame = nFrameNumber;
			}
			else
			{
				// A little convoluted, but I don't want to store any unnecessary state
				// Basically, the render frame will == frame number + duplication count by the end
				// NOTE: This will produce a problem for a few frames every 4 billion frames when wraparound happens
				// tough noogies
				++nAlternateSortCount;
				if( pInfo->m_nRenderFrame < nFrameNumber )
					pInfo->m_nRenderFrame = nFrameNumber + 1;
				else
					++pInfo->m_nRenderFrame;
			}
		}
		ppRenderables[nUniqueCount++] = ppRenderables[i];
	}

	if ( nAlternateSortCount )
	{
		// Extract out the renderables which use alternate sorting
		nCount = nUniqueCount;
		nUniqueCount = 0;
		nLeaf = 0;
		for ( int i = 0; i < nCount; ++i )
		{
			RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;
			
			if ( !infoorleaf.is_leaf )
			{
				RenderableInfo_t *pInfo = infoorleaf.info;

				if ( pInfo->m_Flags & RENDER_FLAGS_ALTERNATE_SORTING )
				{
					// Add in the last one we encountered
					if( --pInfo->m_nRenderFrame != nFrameNumber )
						continue;
				}
			}

			ppRenderables[nUniqueCount++] = ppRenderables[i];
		}
	}

	return nUniqueCount;
}

int CClientLeafSystem::ExtractTranslucentRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables )
{
	int nUniqueCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			if ( pInfo->IsTranslucent() )
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}
		ppRenderables[nUniqueCount++] = ppRenderables[i];
	}
	return nUniqueCount;
}

void CClientLeafSystem::ComputeDistanceFade( int nCount, AlphaInfo_t *pAlphaInfo, BuildRenderListInfo_t *pRLInfo )
{
	// Distance fade computations
	float flDistFactorSq = 1.0f;
	Vector vecViewOrigin = CurrentViewOrigin();
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( pLocal )
	{
		flDistFactorSq = pLocal->GetFOVDistanceAdjustFactor();
		flDistFactorSq *= flDistFactorSq;
	}

	for ( int i = 0; i < nCount; ++i )
	{
		CClientAlphaProperty *pAlphaProp = pAlphaInfo[i].m_pAlphaProperty;
		if ( !pAlphaProp )
			continue;

		// Distance fade is inactive in this case
		if ( pAlphaProp->m_nDistFadeEnd == 0 )
			continue;

		float flCurrentDistanceSq;
		if ( pAlphaProp->m_nDistanceFadeMode == CLIENT_ALPHA_DISTANCE_FADE_USE_CENTER )
		{
			flCurrentDistanceSq = flDistFactorSq * vecViewOrigin.DistToSqr( pAlphaInfo[i].m_vecCenter );
		}
		else
		{
			flCurrentDistanceSq = flDistFactorSq * CalcSqrDistanceToAABB( pRLInfo[i].m_vecMins, pRLInfo[i].m_vecMaxs, vecViewOrigin );
		}

		float flDistFadeStartSq = pAlphaProp->m_nDistFadeStart;
		flDistFadeStartSq *= flDistFadeStartSq;
		if ( flCurrentDistanceSq <= flDistFadeStartSq )
			continue;

		float flDistFadeEndSq = pAlphaProp->m_nDistFadeEnd;
		flDistFadeEndSq *= flDistFadeEndSq;
		if ( flCurrentDistanceSq >= flDistFadeEndSq )
		{
			pAlphaInfo[i].m_flFadeFactor = 0.0f;
			continue;
		}

		// NOTE: Because of the if-checks above, flDistFadeEndSq != flDistFadeStartSq here
		pAlphaInfo[i].m_flFadeFactor = ( flDistFadeEndSq - flCurrentDistanceSq ) / ( flDistFadeEndSq - flDistFadeStartSq );
	}
}

float ComputeScreenSize( const Vector &vecOrigin, float flRadius, const ScreenSizeComputeInfo_t& info )
{
	// This is sort of faked, but it's faster that way
	// FIXME: Also, there's a much faster way to do this with similar triangles
	// but I want to make sure it exactly matches the current matrices, so
	// for now, I do it this conservative way
	/*
	Vector4D testPoint1, testPoint2;
	VectorMA( vecOrigin, flRadius, info.m_vecViewUp, testPoint1.AsVector3D() );
	VectorMA( vecOrigin, -flRadius, info.m_vecViewUp, testPoint2.AsVector3D() );
	testPoint1.w = testPoint2.w = 1.0f;

	Vector4D clipPos1, clipPos2;
	Vector4DMultiply( info.m_matViewProj, testPoint1, clipPos1 );
	Vector4DMultiply( info.m_matViewProj, testPoint2, clipPos2 );
	if (clipPos1.w >= 0.001f)
	{
		clipPos1.y /= clipPos1.w;
	}
	else
	{
		clipPos1.y *= 1000;
	}
	if (clipPos2.w >= 0.001f)
	{
		clipPos2.y /= clipPos2.w;
	}
	else
	{
		clipPos2.y *= 1000;
	}

	// The divide-by-two here is because y goes from -1 to 1 in projection space
	return info.m_nViewportHeight * fabs( clipPos2.y - clipPos1.y ) * 0.5f;
	*/

	// NOTE: Optimized version of the above algorithm, which only uses y and w components of clip
	// Can also optimize based on clipPos = a +/- b * r
	const float *pViewProjY	= info.m_matViewProj[1];
	const float *pViewProjW	= info.m_matViewProj[3];
	float flODotY		= pViewProjY[0] * vecOrigin.x			+ pViewProjY[1] * vecOrigin.y			+ pViewProjY[2] * vecOrigin.z			+ pViewProjY[3];
	float flViewDotY	= pViewProjY[0] * info.m_vecViewUp.x	+ pViewProjY[1] * info.m_vecViewUp.y	+ pViewProjY[2] * info.m_vecViewUp.z;  
	flViewDotY			*= flRadius;
	float flODotW		= pViewProjW[0] * vecOrigin.x			+ pViewProjW[1] * vecOrigin.y			+ pViewProjW[2] * vecOrigin.z			+ pViewProjW[3];
	float flViewDotW	= pViewProjW[0] * info.m_vecViewUp.x	+ pViewProjW[1] * info.m_vecViewUp.y	+ pViewProjW[2] * info.m_vecViewUp.z;
	flViewDotW			*= flRadius;
	float y0			= flODotY + flViewDotY;
	float w0			= flODotW + flViewDotW;
	y0					*= ( w0 >= 0.001f ) ? ( 1.0f / w0 ) : 1000.0f;
	float y1			= flODotY - flViewDotY;
	float w1			= flODotW - flViewDotW;
	y1					*= ( w1 >= 0.001f ) ? ( 1.0f / w1 ) : 1000.0f;

	// The divide-by-two here is because y goes from -1 to 1 in projection space
	return info.m_nViewportHeight * fabs( y1 - y0 ) * 0.5f;
}

void ComputeScreenSizeInfo( ScreenSizeComputeInfo_t *pInfo )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	VMatrix viewMatrix, projectionMatrix;
	pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	pRenderContext->GetMatrix( MATERIAL_PROJECTION, &projectionMatrix );
	MatrixMultiply( projectionMatrix, viewMatrix, pInfo->m_matViewProj );

	int x, y, w, h;
	pRenderContext->GetViewport( x, y, w, h );
	pInfo->m_nViewportHeight = h;

	pRenderContext->GetWorldSpaceCameraVectors( NULL, NULL, &pInfo->m_vecViewUp );
}

void CClientLeafSystem::ComputeScreenFade( const ScreenSizeComputeInfo_t &info, float flMinScreenWidth, float flMaxScreenWidth, int nCount, AlphaInfo_t *pAlphaInfo )
{	
	if ( flMaxScreenWidth <= flMinScreenWidth )
	{
		flMaxScreenWidth = flMinScreenWidth;
	}
	if ( flMinScreenWidth <= 0 ) 
		return;

	float flFalloffFactor;
	if ( flMaxScreenWidth != flMinScreenWidth )
	{
		flFalloffFactor = 1.0f / ( flMaxScreenWidth - flMinScreenWidth );
	}
	else
	{
		flFalloffFactor = 1.0f;
	}
									    
	for ( int i = 0; i < nCount; ++i )
	{
		CClientAlphaProperty *pAlphaProp = pAlphaInfo[i].m_pAlphaProperty;
		if ( !pAlphaProp )
			continue;

		// Fade is inactive in this case
		if ( pAlphaProp->m_flFadeScale <= 0.0f )
			continue;

		float flPixelWidth = ComputeScreenSize( pAlphaInfo[i].m_vecCenter, pAlphaInfo[i].m_flRadius, info ) / pAlphaProp->m_flFadeScale;

		// NOTE: This is to account for an error in the original screen computations years ago
		flPixelWidth *= 2.0f;

		float flAlpha = 0.0f;
		if ( flPixelWidth > flMinScreenWidth )
		{
			if ( ( flMaxScreenWidth >= 0) && ( flPixelWidth < flMaxScreenWidth ) )
			{
				flAlpha = flFalloffFactor * (flPixelWidth - flMinScreenWidth );
			}
			else
			{
				flAlpha = 1.0f;
			}
		}

		pAlphaInfo[i].m_flFadeFactor = MIN( pAlphaInfo[i].m_flFadeFactor, flAlpha );
	}
}

int CClientLeafSystem::ComputeTranslucency( int nFrameNumber, int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo )
{
	AlphaInfo_t *pAlphaInfo = (AlphaInfo_t*)stackalloc( nCount * sizeof(AlphaInfo_t) );
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( infoorleaf.is_leaf )
		{
			pAlphaInfo[i].m_pAlphaProperty = NULL;
			pAlphaInfo[i].m_pRenderableMod = NULL;
			continue;
		}

		RenderableInfo_t *pInfo = infoorleaf.info;

		Vector vecCenter;
		VectorAdd( pRLInfo[i].m_vecMaxs, pRLInfo[i].m_vecMins, vecCenter );
		vecCenter *= 0.5f;
		pAlphaInfo[i].m_vecCenter = vecCenter;
		pAlphaInfo[i].m_flRadius = vecCenter.DistTo( pRLInfo[i].m_vecMaxs );
		pAlphaInfo[i].m_pAlphaProperty = pInfo->m_pAlphaProperty;
		pAlphaInfo[i].m_pRenderableMod = pInfo->m_pRenderableMod;
		pAlphaInfo[i].m_flFadeFactor = 1.0f;
	}

	for ( int i = 0; i < nCount; ++i )
	{
		// FIXME: Computing the base alpha could potentially be sorted by renderfx type
		CClientAlphaProperty *pAlphaProp = pAlphaInfo[i].m_pAlphaProperty;
		if ( pAlphaProp )
		{
			pRLInfo[i].m_nAlpha = pAlphaProp->ComputeRenderAlpha( false );
			pRLInfo[i].m_bIgnoreZBuffer = pAlphaProp->IgnoresZBuffer();
		}
		else
		{
			pRLInfo[i].m_nAlpha = 255;
			pRLInfo[i].m_bIgnoreZBuffer = false;
		}
	}

	// If we're taking devshots, don't fade props at all
	bool bFadeProps = true;
#ifdef _DEBUG
	bFadeProps = r_FadeProps.GetBool();
#endif

	if ( nViewID == VIEW_3DSKY )
	{
		bFadeProps = false;
	}

	if ( !g_MakingDevShots && !cl_leveloverview.GetInt() && bFadeProps )
	{
		ComputeDistanceFade( nCount, pAlphaInfo, pRLInfo );

		ScreenSizeComputeInfo_t info;

		CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
		VMatrix viewMatrix, projectionMatrix;
		pRenderContext->GetMatrix( MATERIAL_VIEW, &viewMatrix );
		pRenderContext->GetMatrix( MATERIAL_PROJECTION, &projectionMatrix );
		MatrixMultiply( projectionMatrix, viewMatrix, info.m_matViewProj );

		int x, y, w, h;
		pRenderContext->GetViewport( x, y, w, h );
		info.m_nViewportHeight = h;

		pRenderContext->GetWorldSpaceCameraVectors( NULL, NULL, &info.m_vecViewUp );

		if ( GetViewRenderInstance()->AllowScreenspaceFade() )
		{
			float flMinLevelFadeArea, flMaxLevelFadeArea;
			modelinfo->GetLevelScreenFadeRange( &flMinLevelFadeArea, &flMaxLevelFadeArea );
			ComputeScreenFade( info, flMinLevelFadeArea, flMaxLevelFadeArea, nCount, pAlphaInfo );

			float flMinViewFadeArea, flMaxViewFadeArea;
			GetViewRenderInstance()->GetScreenFadeDistances( &flMinViewFadeArea, &flMaxViewFadeArea );
			ComputeScreenFade( info, flMinViewFadeArea, flMaxViewFadeArea, nCount, pAlphaInfo );
		}

		for ( int i = 0; i < nCount; ++i )
		{
			if ( !pAlphaInfo[i].m_pAlphaProperty )
				continue;

			float flAlpha = pRLInfo[i].m_nAlpha * pAlphaInfo[i].m_flFadeFactor;
			int nAlpha = (int)flAlpha;
			pRLInfo[i].m_nAlpha = clamp( nAlpha, 0, 255 );
		}
	}

	// Update shadows
	for ( int i = 0; i < nCount; ++i )
	{
		CClientAlphaProperty *pAlphaProp = pAlphaInfo[i].m_pAlphaProperty;
		IClientRenderableMod *pRenderableMod = pAlphaInfo[i].m_pRenderableMod;
		if ( !pAlphaProp || ( pAlphaProp->m_hShadowHandle == CLIENTSHADOW_INVALID_HANDLE ) )
			continue;

		pRLInfo[i].m_nShadowAlpha = pAlphaProp->ComputeRenderAlpha( true );
		g_pClientShadowMgr->SetFalloffBias( pAlphaInfo[i].m_pAlphaProperty->m_hShadowHandle, (255 - pRLInfo[i].m_nShadowAlpha) );
	}

	// Strip invisible ones out
	int nUniqueCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( !infoorleaf.is_leaf && ( !pRLInfo[i].m_nAlpha ) )
		{
			// Necessary for dependent models to be grabbed
			infoorleaf.info->m_nRenderFrame--;
			continue;
		}

		ppRenderables[nUniqueCount] = ppRenderables[i];
		pRLInfo[nUniqueCount] = pRLInfo[i];
		++nUniqueCount;
	}
	return nUniqueCount;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractFadedRenderables( int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo )
{
	// Distance fade computations
	float flDistFactorSq = 1.0f;
	Vector vecViewOrigin = (nViewID == VIEW_SHADOW_DEPTH_TEXTURE ? MainViewOrigin() : CurrentViewOrigin());
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	if ( pLocal )
	{
		flDistFactorSq = pLocal->GetFOVDistanceAdjustFactor();
		flDistFactorSq *= flDistFactorSq;
	}

	// Strip faded renderables
	int nUniqueCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		BuildRenderListInfo_t &rlInfo = pRLInfo[i];
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if( !infoorleaf.is_leaf )
		{ 
			RenderableInfo_t *pInfo = infoorleaf.info;

			if(pInfo->m_pAlphaProperty) {
				float fDistanceFade = pInfo->m_pAlphaProperty->m_nDistFadeEnd;
				fDistanceFade *= fDistanceFade;

				Vector vecCenter;
				VectorAdd( pRLInfo[i].m_vecMaxs, pRLInfo[i].m_vecMins, vecCenter );
				vecCenter *= 0.5f;

				float flCurrentDistanceSq = flDistFactorSq * vecViewOrigin.DistToSqr( vecCenter );

				if( fDistanceFade != 0 && fDistanceFade < flCurrentDistanceSq )
				{
					// Necessary for dependent models to be grabbed
					pInfo->m_nRenderFrame--;
					continue;
				}
			}
		}

		ppRenderables[nUniqueCount] = ppRenderables[i];
		pRLInfo[nUniqueCount] = rlInfo;
		++nUniqueCount;
	}
	return nUniqueCount;
}

// Test code:
static int ScreenTransform2( const Vector& point, Vector& screen, const VMatrix &worldToScreen )
{
	// UNDONE: Clean this up some, handle off-screen vertices
	float w;

	screen.x = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen.y = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
	//	z		 = worldToScreen[2][0] * point[0] + worldToScreen[2][1] * point[1] + worldToScreen[2][2] * point[2] + worldToScreen[2][3];
	w		 = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];

	// Just so we have something valid here
	screen.z = 0.0f;

	bool behind;
	if( w < 0.001f )
	{
		behind = true;
		screen.x *= 100000;
		screen.y *= 100000;
	}
	else
	{
		behind = false;
		float invw = 1.0f / w;
		screen.x *= invw;
		screen.y *= invw;
	}

	return behind;
}

#include "vgui_int.h"
bool GetVectorInScreenSpace2( Vector pos, int& iX, int& iY, Vector *vecOffset, const VMatrix &worldToScreen )
{
	Vector screen;

	// Apply the offset, if one was specified
	if ( vecOffset != NULL )
		pos += *vecOffset;

	// Transform to screen space
	int x, y, screenWidth, screenHeight;
	int insetX, insetY;
	VGui_GetEngineRenderBounds( x, y, screenWidth, screenHeight, insetX, insetY );

	// Transform to screen space
	int iFacing = ScreenTransform2( pos, screen, worldToScreen );

	iX = 0.5 * screen[0] * screenWidth;
	iY = -0.5 * screen[1] * screenHeight;
	iX += 0.5 * screenWidth;
	iY += 0.5 * screenHeight;	

	iX += insetX;
	iY += insetY;

	// Make sure the player's facing it
	if ( iFacing )
	{
		// We're actually facing away from the Target. Stomp the screen position.
		iX = -640;
		iY = -640;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractNotInScreen( int nViewID, int nCount, RenderableInfoAndHandle_t *ppRenderables )
{
	// Strip renderables not in screen of local player
	int nUniqueCount = 0;
	int x, y;
	bool onscreen;

	static VMatrix worldToScreen;
	if( nViewID == 0 )
	{
		worldToScreen = engine->WorldToScreenMatrix();
	}

	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			onscreen = GetVectorInScreenSpace2( pInfo->m_pRenderable->GetRenderOrigin(), x, y, NULL, worldToScreen );
			//NDebugOverlay::Text( pInfo->m_pRenderable->GetRenderOrigin(), VarArgs("%d,%d,%d", x, y, onscreen), false, gpGlobals->frametime );
			if( !onscreen || x < 0 || y < 0 || x > ScreenWidth() || y > ScreenHeight() )
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}

		ppRenderables[nUniqueCount] = ppRenderables[i];
		++nUniqueCount;
	}

	return nUniqueCount;
}

//-----------------------------------------------------------------------------
// Computes bounds for all renderables
//-----------------------------------------------------------------------------
void CClientLeafSystem::ComputeBounds( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo )
{
//	MiniProfilerGuard mpGuard(&g_mpComputeBounds);

	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( infoorleaf.is_leaf )
			continue;

		RenderableInfo_t *pInfo = infoorleaf.info;

		// UNDONE: Investigate speed tradeoffs of occlusion culling brush models too?
		pRLInfo[i].m_bPerformOcclusionTest = ( pInfo->IsStaticProp() || pInfo->IsStudioModel() ); 
		pRLInfo[i].m_nArea = pInfo->m_Area;
		pRLInfo[i].m_nAlpha = 255;	// necessary to set for shadow depth rendering

		// NOTE: This is inherently not threadsafe!!
		if ( ( pInfo->m_Flags & RENDER_FLAGS_BOUNDS_VALID ) == 0 )
		{
			CalcRenderableWorldSpaceAABB( pInfo->m_pRenderable, pInfo->m_vecAbsMins, pInfo->m_vecAbsMaxs );

			// Determine object group offset
			if ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS > 1 )
			{
				Vector dims;
				VectorSubtract( pInfo->m_vecAbsMaxs, pInfo->m_vecAbsMins, dims );

				float const fDimension = MAX( MAX( fabs(dims.x), fabs(dims.y) ), fabs(dims.z) );
				DetectBucketedRenderGroup( *pInfo, fDimension );
			}

			if ( ( pInfo->m_Flags & RENDER_FLAGS_BOUNDS_ALWAYS_RECOMPUTE ) == 0 )
			{
				C_BaseEntity *pEnt = pInfo->m_pRenderable->GetIClientUnknown()->GetBaseEntity();
				if ( !pEnt || !pEnt->GetMoveParent() )
				{
					pInfo->m_Flags |= RENDER_FLAGS_BOUNDS_VALID;
				}
			}
		}
#ifdef _DEBUG
		else
		{
			// If these assertions trigger, it means there's some state that GetRenderBounds
			// depends on which, on change, doesn't call ClientLeafSystem::RenderableChanged().
			Vector vecTestMins, vecTestMaxs;
			CalcRenderableWorldSpaceAABB( pInfo->m_pRenderable, vecTestMins, vecTestMaxs );
			Assert( VectorsAreEqual( vecTestMins, pInfo->m_vecAbsMins, 1e-3 ) );
			Assert( VectorsAreEqual( vecTestMaxs, pInfo->m_vecAbsMaxs, 1e-3 ) );
		}
#endif

		pRLInfo[i].m_vecMins = pInfo->m_vecAbsMins;
		pRLInfo[i].m_vecMaxs = pInfo->m_vecAbsMaxs;
	}
}


//-----------------------------------------------------------------------------
// Culls renderables based on view frustum + areaportals
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractCulledRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo )
{
	bool bPortalTestEnts = r_PortalTestEnts.GetBool() && !r_portalsopenall.GetBool();

	// FIXME: sort by area and inline cull. Should make it a bunch faster
	int nUniqueCount = 0;
	if ( bPortalTestEnts )
	{
		for ( int i = 0; i < nCount; ++i )
		{
			BuildRenderListInfo_t &rlInfo = pRLInfo[i];
			RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;
			
			if ( !infoorleaf.is_leaf )
			{
				RenderableInfo_t *pInfo = infoorleaf.info;

				if ( !engine->DoesBoxTouchAreaFrustum( rlInfo.m_vecMins, rlInfo.m_vecMaxs, rlInfo.m_nArea ) )
				{
					// Necessary for dependent models to be grabbed
					pInfo->m_nRenderFrame--;
					continue;
				}
			}
			pRLInfo[nUniqueCount] = rlInfo;
			ppRenderables[nUniqueCount] = ppRenderables[i];
			++nUniqueCount;
		}
		return nUniqueCount;
	}

	// Debug mode, doesn't need to be fast
	for ( int i = 0; i < nCount; ++i )
	{
		BuildRenderListInfo_t &rlInfo = pRLInfo[i];
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			// cull with main frustum
			if ( engine->CullBox( rlInfo.m_vecMins, rlInfo.m_vecMaxs ) )
			{
				// Necessary for dependent models to be grabbed
				pInfo->m_nRenderFrame--;
				continue;
			}
		}
		pRLInfo[nUniqueCount] = rlInfo;
		ppRenderables[nUniqueCount] = ppRenderables[i];
		++nUniqueCount;
	}
	return nUniqueCount;
}

//-----------------------------------------------------------------------------
// Culls renderables based on occlusion
//-----------------------------------------------------------------------------
int CClientLeafSystem::ExtractOccludedRenderables( int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo )
{
	static ConVarRef r_occlusion("r_occlusion");

	// occlusion is off, just return
	if ( !r_occlusion.GetBool() )
		return nCount;

	int nUniqueCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		BuildRenderListInfo_t &rlInfo = pRLInfo[i];
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( !infoorleaf.is_leaf )
		{
			RenderableInfo_t *pInfo = infoorleaf.info;

			if ( rlInfo.m_bPerformOcclusionTest )
			{
				// test to see if this renderable is occluded by the engine's occlusion system
				if ( engine->IsOccluded( rlInfo.m_vecMins, rlInfo.m_vecMaxs ) )
				{
					// Necessary for dependent models to be grabbed
					pInfo->m_nRenderFrame--;
					continue;
				}
			}
		}
		pRLInfo[nUniqueCount] = rlInfo;
		ppRenderables[nUniqueCount] = ppRenderables[i];
		++nUniqueCount;
	}

	return nUniqueCount;
}

//-----------------------------------------------------------------------------
// Adds renderables into their final lists
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddDependentRenderables( const SetupRenderInfo_t &info )
{
	// NOTE: This turns out to have non-zero cost.
	// Remove early out if we actually end up needing to use this
#if 0
	CClientRenderablesList *pRenderList = info.m_pRenderList;
	pRenderList->m_nBoneSetupDependencyCount = 0;
	for ( int i = 0; i < ENGINE_RENDER_GROUP_COUNT; ++i )
	{
		int nCount = pRenderList->m_RenderGroupCounts[i];
		for ( int j = 0; j < nCount; ++j )
		{
			IClientRenderable *pRenderable = pRenderList->m_RenderGroups[i][j].m_pRenderable;
			C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
			if ( !pEnt )
				continue;

			while ( pEnt->IsFollowingEntity() || ( pEnt->GetMoveParent() && pEnt->GetParentAttachment() > 0 ) )
			{
				pEnt = pEnt->GetMoveParent();
				ClientRenderHandle_t hParent = pEnt->GetRenderHandle();
				Assert( hParent != INVALID_CLIENT_RENDER_HANDLE );
				if ( hParent == INVALID_CLIENT_RENDER_HANDLE )
					continue;
				RenderableInfo_t &parentInfo = m_Renderables[(uint)hParent];
				if ( parentInfo.m_nRenderFrame != info.m_nRenderFrame )
				{
					parentInfo.m_nRenderFrame = info.m_nRenderFrame;
					pRenderList->m_pBoneSetupDependency[ pRenderList->m_nBoneSetupDependencyCount++ ] = pEnt->GetClientRenderable();
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Adds renderables into their final lists
//-----------------------------------------------------------------------------
void CClientLeafSystem::AddRenderablesToRenderLists( const SetupRenderInfo_t &info, int nCount, RenderableInfoAndHandle_t *ppRenderables, BuildRenderListInfo_t *pRLInfo, int nDetailCount, DetailRenderableInfo_t *pDetailInfo )
{
	CClientRenderablesList::CEntry *pTranslucentEntries = info.m_pRenderList->m_RenderGroups[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
	const int &nTranslucentEntries = info.m_pRenderList->m_RenderGroupCounts[CClientRenderablesList::GROUP_TRANSLUCENT_ENTITY];
	int nTranslucent = 0;
	int nCurDetail = 0;
	int nWorldListLeafIndex = -1;
	for ( int i = 0; i < nCount; ++i )
	{
		RenderableInfoOrLeaf_t infoorleaf = ppRenderables[i].infoorleaf;

		if ( infoorleaf.is_leaf )
		{
			// Add detail props for this leaf
			for( ; nCurDetail < nDetailCount; ++nCurDetail )
			{
				DetailRenderableInfo_t &detailInfo = pDetailInfo[nCurDetail];
				if ( detailInfo.m_nLeafIndex > nWorldListLeafIndex )
					break;
				Assert( detailInfo.m_nLeafIndex == nWorldListLeafIndex );
				AddRenderableToRenderList( *info.m_pRenderList, detailInfo, 
					nWorldListLeafIndex, detailInfo.m_InstanceData.m_nAlpha, detailInfo.m_hHandle );
			}

			int nNewTranslucent = nTranslucentEntries - nTranslucent;
			if( ( nNewTranslucent != 0 ) && info.m_bDrawTranslucentObjects )
			{
				// Sort the new translucent entities.
				SortEntities( info.m_vecRenderOrigin, info.m_vecRenderForward, &pTranslucentEntries[nTranslucent], nNewTranslucent );
			}
			nTranslucent = nTranslucentEntries;
			nWorldListLeafIndex++;
			continue;
		}

		RenderableInfo_t *pInfo = infoorleaf.info;

		bool bIsTranslucent = ( pRLInfo[i].m_nAlpha != 255 ) || ( !pInfo->IsOpaque() ); 
		if ( !bIsTranslucent )
		{
			AddRenderableToRenderList( *info.m_pRenderList, *pInfo, 
				nWorldListLeafIndex, pRLInfo[i].m_nAlpha, ppRenderables[i].handle, false, ADD_RENDERABLE_OPAQUE );
			continue;
		}

		// FIXME: Remove call to GetFXBlend
		bool bIsTwoPass = ( pInfo->IsTwoPass() ) && ( pRLInfo[i].m_nAlpha == 255 );	// Two pass?
		if(pInfo->IsBrushModel()) {
			bIsTwoPass = false;
		}

		// Add to appropriate list if drawing translucent objects (shadow depth mapping will skip this)
		if ( info.m_bDrawTranslucentObjects ) 
		{
			AddRenderableToRenderList( *info.m_pRenderList, *pInfo, 
				nWorldListLeafIndex, pRLInfo[i].m_nAlpha, ppRenderables[i].handle, bIsTwoPass, ADD_RENDERABLE_TRANSLUCENT );
		}

		if ( bIsTwoPass )	// Also add to opaque list if it's a two-pass model... 
		{
			AddRenderableToRenderList( *info.m_pRenderList, *pInfo, 
				nWorldListLeafIndex, 255, ppRenderables[i].handle, bIsTwoPass, ADD_RENDERABLE_OPAQUE );
		}
	}

	// Add detail props for this leaf
	for( ; nCurDetail < nDetailCount; ++nCurDetail )
	{
		DetailRenderableInfo_t &detailInfo = pDetailInfo[nCurDetail];
		if ( detailInfo.m_nLeafIndex > nWorldListLeafIndex )
			break;
		Assert( detailInfo.m_nLeafIndex == nWorldListLeafIndex );
		AddRenderableToRenderList( *info.m_pRenderList, detailInfo, 
			nWorldListLeafIndex, detailInfo.m_InstanceData.m_nAlpha, detailInfo.m_hHandle );
	}

	int nNewTranslucent = nTranslucentEntries - nTranslucent;
	if( ( nNewTranslucent != 0 ) && info.m_bDrawTranslucentObjects )
	{
		// Sort the new translucent entities.
		SortEntities( info.m_vecRenderOrigin, info.m_vecRenderForward, &pTranslucentEntries[nTranslucent], nNewTranslucent );
	}

	AddDependentRenderables( info );
}

ConVar debug_buildrenderables_snapshot("debug_buildrenderables_snapshot", "0", FCVAR_CHEAT);
void CClientLeafSystem::BuildRenderablesList( const SetupRenderInfo_t &info )
{
	VPROF_BUDGET( "BuildRenderablesList", "BuildRenderablesList" );

	ASSERT_NO_REENTRY();

	// Deal with detail objects
	CUtlVectorFixedGrowable< DetailRenderableInfo_t, 2048 > detailRenderables( 2048 );

	// Get the fade information for detail objects
	float flDetailDist = DetailObjectSystem()->ComputeDetailFadeInfo( &info.m_pRenderList->m_DetailFade );
	DetailObjectSystem()->BuildRenderingData( detailRenderables, info, flDetailDist, info.m_pRenderList->m_DetailFade );

	// First build a non-unique list of renderables, separated by special leaf markers
	CUtlVectorFixedGrowable< RenderableInfoAndHandle_t , 65536 > orderedList( 65536 );

	if ( info.m_nViewID != VIEW_3DSKY && r_drawallrenderables.GetBool() )
	{
		// HACK - treat all renderables as being in first visible leaf
		int leaf = info.m_pWorldListInfo->m_pLeafList[ 0 ];
		orderedList.AddToTail( RenderableInfoAndHandle_t( leaf ) );

		for ( uint i = m_Renderables.Head(); i != (uint)m_Renderables.InvalidIndex(); i = m_Renderables.Next( i ) )
		{
			ClientRenderHandle_t handle = (ClientRenderHandle_t)i;
			RenderableInfo_t& renderable = m_Renderables[(uint)handle];

			orderedList.AddToTail( RenderableInfoAndHandle_t(&renderable, handle) );
		}
	}
	else
	{
		int leaf = 0;
		for ( int i = 0; i < info.m_pWorldListInfo->m_LeafCount; ++i )
		{
			leaf = info.m_pWorldListInfo->m_pLeafList[i];
			orderedList.AddToTail( RenderableInfoAndHandle_t( leaf ) );

			// iterate over all elements in this leaf
			unsigned int idx = m_RenderablesInLeaf.FirstElement(leaf);
			for ( ; idx != m_RenderablesInLeaf.InvalidIndex(); idx = m_RenderablesInLeaf.NextElement( idx ) )
			{
				ClientRenderHandle_t handle = m_RenderablesInLeaf.Element(idx);
				RenderableInfo_t& renderable = m_Renderables[(uint)handle];

				orderedList.AddToTail( RenderableInfoAndHandle_t(&renderable, handle) );
			}
		}
	}

	// Debugging
	int nCount = orderedList.Count();
	RenderableInfoAndHandle_t *ppRenderables = orderedList.Base();
	nCount = ExtractDuplicates( info.m_nRenderFrame, nCount, ppRenderables );
	nCount = ExtractStaticProps( nCount, ppRenderables );

	if (info.m_nViewID == VIEW_SHADOW_DEPTH_TEXTURE)
	{
		nCount = ExtractDisableShadowDepthRenderables(nCount, ppRenderables);
		if (info.m_bDrawDepthViewNonCachedObjectsOnly)
		{
			nCount = ExtractDisableShadowDepthCacheRenderables(nCount, ppRenderables);
		}
	}

	if ( !info.m_bDrawTranslucentObjects )
	{
		nCount = ExtractTranslucentRenderables( nCount, ppRenderables );
	}

	BuildRenderListInfo_t* pRLInfo = (BuildRenderListInfo_t*)stackalloc( nCount * sizeof(BuildRenderListInfo_t) );
	ComputeBounds( nCount, ppRenderables, pRLInfo );

	nCount = ExtractCulledRenderables( nCount, ppRenderables, pRLInfo );

	if ( info.m_bDrawTranslucentObjects )
	{
		nCount = ComputeTranslucency( gpGlobals->framecount /*info.m_nRenderFrame*/, info.m_nViewID, nCount, ppRenderables, pRLInfo );
	}
	else
	{
		nCount = ExtractFadedRenderables( info.m_nViewID, nCount, ppRenderables, pRLInfo );
	}

	nCount = ExtractOccludedRenderables( nCount, ppRenderables, pRLInfo );

	// Debug code
	static bool bSnapshotting = false;
	static int iSnaphotframecount = -1;
	static int iOffset = 3;
	if( debug_buildrenderables_snapshot.GetBool() )
	{
		debug_buildrenderables_snapshot.SetValue( false );
		bSnapshotting = true;
		iSnaphotframecount = gpGlobals->framecount;
	}

	if( bSnapshotting )
	{
		if( iSnaphotframecount != gpGlobals->framecount )
		{
			bSnapshotting = false;
			iOffset = 3;
		}
		else
		{
			Vector vecViewOrigin = CurrentViewOrigin();
			engine->Con_NPrintf( iOffset, "View: %d; Real View: %d; Draw TransLuc: %d; vecViewOrigin: %.2f %.2f %.2f. Leaf count: %d. Renderables count: %d\n", 
				info.m_nViewID, CurrentViewID(), info.m_bDrawTranslucentObjects, vecViewOrigin.x, vecViewOrigin.y, vecViewOrigin.z, info.m_pWorldListInfo->m_LeafCount, nCount );
			iOffset += 1;
		}
	}

	AddRenderablesToRenderLists( info, nCount, ppRenderables, pRLInfo, detailRenderables.Count(), detailRenderables.Base() );
	stackfree( pRLInfo );
}

EngineRenderGroup_t CClientLeafSystem::GenerateRenderListEntry( IClientRenderable *pRenderable, CClientRenderablesList::CEntry &entryOut )
{
	ClientRenderHandle_t iter = (ClientRenderHandle_t)m_Renderables.Head();
	while( m_Renderables.IsValidIndex( (uint)iter ) )
	{
		RenderableInfo_t &info = m_Renderables.Element( (uint)iter );
		if( info.m_pRenderable == pRenderable )
		{			
			int nAlpha = info.m_pAlphaProperty ? info.m_pAlphaProperty->ComputeRenderAlpha( false ) : 255;
			bool bIsTranslucent = ( nAlpha != 255 ) || ( !info.IsOpaque() ); 

			entryOut.m_pRenderable = pRenderable;
			entryOut.m_pRenderableMod = NULL;
			entryOut.m_iWorldListInfoLeaf = 0; //info.m_RenderLeaf;			
			entryOut.m_TwoPass = ( info.m_nTranslucencyType == RENDERABLE_IS_TWO_PASS );
			entryOut.m_nModelType = info.m_nModelType;
			entryOut.m_InstanceData.m_nAlpha = nAlpha;
			if ( !bIsTranslucent )
				return info.m_EngineRenderGroupOpaque;
			return info.m_EngineRenderGroup;
		}
		iter = (ClientRenderHandle_t)m_Renderables.Next( (uint)iter );
	}

	entryOut.m_pRenderable = NULL;
	entryOut.m_pRenderableMod = NULL;
	entryOut.m_iWorldListInfoLeaf = 0;
	entryOut.m_TwoPass = false;
	entryOut.m_nModelType = RENDERABLE_MODEL_ENTITY;
	entryOut.m_InstanceData.m_nAlpha = 255;

	return ENGINE_RENDER_GROUP_OTHER;
}

