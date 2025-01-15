//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IVMODELINFO_H
#define IVMODELINFO_H

#pragma once

#include "platform.h"
#include "dbg.h"
#include "hackmgr/hackmgr.h"

DECLARE_LOGGING_CHANNEL( LOG_MODEL );

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class KeyValues;
struct vcollide_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
struct cplane_t;
typedef CGameTrace trace_t;
struct studiohdr_t;
struct virtualmodel_t;
typedef unsigned char byte;
struct virtualterrainparams_t;
class CPhysCollide;
enum MDLHandle_t : unsigned short;
class CUtlBuffer;
class IClientRenderable;
enum sequence_t : unsigned short;

//-----------------------------------------------------------------------------
// Indicates the type of translucency of an unmodulated renderable
//-----------------------------------------------------------------------------
enum RenderableTranslucencyType_t : unsigned int
{
	RENDERABLE_IS_OPAQUE = 0,
	RENDERABLE_IS_TRANSLUCENT,
	RENDERABLE_IS_TWO_PASS,	// has both translucent and opaque sub-partsa
};

//-----------------------------------------------------------------------------
// Purpose: a callback class that is notified when a model has finished loading
//-----------------------------------------------------------------------------
abstract_class IModelLoadCallback
{
public:
	virtual void OnModelLoadComplete( const model_t* pModel ) = 0;

protected:
	// Protected destructor so that nobody tries to delete via this interface.
	// Automatically unregisters if the callback is destroyed while still pending.
	~IModelLoadCallback();
};

enum modelindex_t : int;
inline const modelindex_t INVALID_MODEL_INDEX = (modelindex_t)-1;

UNORDEREDENUM_OPERATORS( modelindex_t, int )

// MODEL INDEX RULES
// If index >= 0, then index references the precached model string table
// If index == -1, then the model is invalid
// If index < -1, then the model is DYNAMIC and has a DYNAMIC INDEX of (-2 - index)
// - if the dynamic index is ODD, then the model is CLIENT ONLY
//   and has a m_LocalDynamicModels lookup index of (dynamic index)>>1
// - if the dynamic index is EVEN, then the model is NETWORKED
//   and has a dynamic model string table index of (dynamic index)>>1

inline bool IsDynamicModelIndex( modelindex_t modelindex ) { return (int)modelindex < -1; }
inline bool IsClientOnlyModelIndex( modelindex_t modelindex ) { return (int)modelindex < -1 && ((int)modelindex & 1); }
inline bool IsNetworkedModelIndex( modelindex_t modelindex ) { return (int)modelindex == -1 || (int)modelindex > 0 || ((int)modelindex < -1 && !((int)modelindex & 1)); }
inline bool IsValidModelIndex( modelindex_t modelindex ) { return (int)modelindex < -1 || (int)modelindex > 0; }
inline bool IsPrecachedModelIndex( modelindex_t modelindex ) { return (int)modelindex > 0; }

//-----------------------------------------------------------------------------
// Purpose: Automate refcount tracking on a model index
//-----------------------------------------------------------------------------
class CRefCountedModelIndex
{
private:
	modelindex_t m_nIndex;
public:
	CRefCountedModelIndex() : m_nIndex( INVALID_MODEL_INDEX ) { }
	~CRefCountedModelIndex() { Set( INVALID_MODEL_INDEX ); }

	CRefCountedModelIndex( const CRefCountedModelIndex& src ) : m_nIndex( INVALID_MODEL_INDEX ) { Set( src.m_nIndex ); }
	CRefCountedModelIndex& operator=( const CRefCountedModelIndex& src ) { Set( src.m_nIndex ); return *this; }

	explicit CRefCountedModelIndex( modelindex_t i ) : m_nIndex( INVALID_MODEL_INDEX ) { Set( i ); }
	CRefCountedModelIndex& operator=( modelindex_t i ) { Set( i ); return *this; }

	modelindex_t Get() const { return m_nIndex; }
	void Set( modelindex_t i );
	void Clear() { Set( INVALID_MODEL_INDEX ); }

	bool IsValid() const
	{ return IsValidModelIndex( m_nIndex ); }

	operator modelindex_t () const { return m_nIndex; }
};


//-----------------------------------------------------------------------------
// Model info interface
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VMODELINFO_CLIENT_INTERFACE_VERSION		"VModelInfoClient006"
#define VMODELINFO_SERVER_INTERFACE_VERSION		"VModelInfoServer004"

abstract_class IVModelInfo
{
public:
	virtual							~IVModelInfo( void ) { }

	// Returns model_t* pointer for a model given a precached or dynamic model index.
	virtual const model_t			*GetModel( modelindex_t modelindex ) = 0;

	// Returns index of model by name for precached or known dynamic models.
	// Does not adjust reference count for dynamic models.
	virtual modelindex_t						GetModelIndex( const char *name ) const = 0;

	// Returns name of model
	virtual const char				*GetModelName( const model_t *model ) const = 0;
	virtual const vcollide_t				*GetVCollide( const model_t *model ) = 0;
	virtual const vcollide_t				*GetVCollide( modelindex_t modelindex ) = 0;
	virtual void					GetModelBounds( const model_t *model, Vector& mins, Vector& maxs ) const = 0;
	virtual	void					GetModelRenderBounds( const model_t *model, Vector& mins, Vector& maxs ) const = 0;
	virtual int						GetModelFrameCount( const model_t *model ) const = 0;
	virtual int						GetModelType( const model_t *model ) const = 0;
	virtual void					*GetModelExtraData( const model_t *model ) = 0;
	virtual bool					ModelHasMaterialProxy( const model_t *model ) const = 0;
	virtual bool					IsTranslucent( model_t const* model ) const = 0;
	virtual bool					IsTranslucentTwoPass( const model_t *model ) const = 0;
	virtual void					RecomputeTranslucency( const model_t *model, int nSkin, int nBody, IClientRenderable *pClientRenderable, float fInstanceAlphaModulate=1.0f) = 0;
	HACKMGR_CLASS_API RenderableTranslucencyType_t ComputeTranslucencyType( const model_t *model, int nSkin, int nBody );
	virtual int						GetModelMaterialCount( const model_t* model ) const = 0;
	virtual void					GetModelMaterials( const model_t *model, int count, IMaterial** ppMaterial ) = 0;
	virtual bool					IsModelVertexLit( const model_t *model ) const = 0;
	HACKMGR_CLASS_API bool UsesEnvCubemap( const model_t *model ) const;
	HACKMGR_CLASS_API bool UsesStaticLighting( const model_t *model ) const;
	virtual const char				*GetModelKeyValueText( const model_t *model ) = 0;
	virtual bool					GetModelKeyValue( const model_t *model, CUtlBuffer &buf ) = 0; // supports keyvalue blocks in submodels
	virtual float					GetModelRadius( const model_t *model ) = 0;

	virtual const studiohdr_t		*FindModel( const studiohdr_t *pStudioHdr, void **cache, const char *modelname ) const = 0;
	virtual const studiohdr_t		*FindModel( void *cache ) const = 0;
	virtual	const virtualmodel_t			*GetVirtualModel( const studiohdr_t *pStudioHdr ) const = 0;
	virtual const byte					*GetAnimBlock( const studiohdr_t *pStudioHdr, int iBlock ) const = 0;

	// Available on client only!!!
	virtual void					GetModelMaterialColorAndLighting( const model_t *model, Vector const& origin,
										QAngle const& angles, trace_t* pTrace, 
										Vector& lighting, Vector& matColor ) = 0;
	virtual void					GetIlluminationPoint( const model_t *model, IClientRenderable *pRenderable, Vector const& origin, 
										QAngle const& angles, Vector* pLightingCenter ) = 0;

	virtual int						GetModelContents( modelindex_t modelIndex ) = 0;
	virtual studiohdr_t				*GetStudiomodel( const model_t *mod ) = 0;
	virtual int						GetModelSpriteWidth( const model_t *model ) const = 0;
	virtual int						GetModelSpriteHeight( const model_t *model ) const = 0;

	// Sets/gets a map-specified fade range (client only)
	virtual void					SetLevelScreenFadeRange( float flMinSize, float flMaxSize ) = 0;
	virtual void					GetLevelScreenFadeRange( float *pMinArea, float *pMaxArea ) const = 0;

	// Sets/gets a map-specified per-view fade range (client only)
	virtual void					SetViewScreenFadeRange( float flMinSize, float flMaxSize ) = 0;

	// Computes fade alpha based on distance fade + screen fade (client only)
	virtual unsigned char			ComputeLevelScreenFade( const Vector &vecAbsOrigin, float flRadius, float flFadeScale ) const = 0;
	virtual unsigned char			ComputeViewScreenFade( const Vector &vecAbsOrigin, float flRadius, float flFadeScale ) const = 0;

	// both client and server
	virtual int						GetAutoplayList( const studiohdr_t *pStudioHdr, sequence_t **pAutoplayList ) const = 0;

	// Gets a virtual terrain collision model (creates if necessary)
	// NOTE: This may return NULL if the terrain model cannot be virtualized
	virtual CPhysCollide			*GetCollideForVirtualTerrain( int index ) = 0;

	virtual bool					IsUsingFBTexture( const model_t *model, int nSkin, int nBody, void /*IClientRenderable*/ *pClientRenderable ) const = 0;

	// Obsolete methods. These are left in to maintain binary compatibility with clients using the IVModelInfo old version.
	virtual const model_t			*DO_NOT_USE_FindOrLoadModel( const char *name ) { Assert(0); return NULL; }
	virtual void					DO_NOT_USE_InitDynamicModels( ) { Assert(0); }
	virtual void					DO_NOT_USE_ShutdownDynamicModels( ) { Assert(0); }
	virtual void					DO_NOT_USE_AddDynamicModel( const char *name, modelindex_t nModelIndex = INVALID_MODEL_INDEX ) { Assert(0); }
	virtual void					DO_NOT_USE_ReferenceModel( modelindex_t modelindex ) { Assert(0); }
	virtual void					DO_NOT_USE_UnreferenceModel( modelindex_t modelindex ) { Assert(0); }
	virtual void					DO_NOT_USE_CleanupDynamicModels( bool bForce = false ) { Assert(0); }

	virtual MDLHandle_t				GetCacheHandle( const model_t *model ) const = 0;

	// Returns planes of non-nodraw brush model surfaces
	virtual int						GetBrushModelPlaneCount( const model_t *model ) const = 0;
	virtual void					GetBrushModelPlane( const model_t *model, int nIndex, cplane_t &plane, Vector *pOrigin ) const = 0;
	virtual int						GetSurfacepropsForVirtualTerrain( int index ) = 0;

	// Poked by engine host system
	virtual void					OnLevelChange() = 0;

	virtual modelindex_t						GetModelClientSideIndex( const char *name ) const = 0;

	// Returns index of model by name, dynamically registered if not already known.
	virtual modelindex_t						RegisterDynamicModel( const char *name, bool bClientSide ) = 0;

	virtual bool					IsDynamicModelLoading( modelindex_t modelIndex ) = 0;

	virtual void					AddRefDynamicModel( modelindex_t modelIndex ) = 0;
	virtual void					ReleaseDynamicModel( modelindex_t modelIndex ) = 0;

	// Registers callback for when dynamic model has finished loading.
	// Automatically adds reference, pair with ReleaseDynamicModel.
	virtual bool					RegisterModelLoadCallback( modelindex_t modelindex, IModelLoadCallback* pCallback, bool bCallImmediatelyIfLoaded = true ) = 0;
	virtual void					UnregisterModelLoadCallback( modelindex_t modelindex, IModelLoadCallback* pCallback ) = 0;
};

abstract_class IVModelInfoClient : public IVModelInfo
{
public:
	virtual void OnDynamicModelsStringTableChange( int nStringIndex, const char *pString, const void *pData ) = 0;

	// For tools only!
	virtual const model_t *DO_NOT_USE_FindOrLoadModel( const char *name ) = 0;
};

struct virtualterrainparams_t
{
	// UNDONE: Add grouping here, specified in BSP file? (test grouping to see if this is necessary)
	int index;
};

#if (defined CLIENT_DLL || defined TOOL_DLL) && !defined GAME_DLL
extern IVModelInfoClient *modelinfo;
#elif defined GAME_DLL
extern IVModelInfo *modelinfo;
#endif

#if defined CLIENT_DLL || defined TOOL_DLL || defined GAME_DLL
inline bool IsModelIndexLoaded( modelindex_t modelindex )
{
	if( IsPrecachedModelIndex( modelindex ) )
		return true;

	if( IsDynamicModelIndex( modelindex ) )
		return !modelinfo->IsDynamicModelLoading( modelindex );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Force removal from callback list on destruction to avoid crashes.
//-----------------------------------------------------------------------------
inline IModelLoadCallback::~IModelLoadCallback()
{
	if ( modelinfo )
	{
		modelinfo->UnregisterModelLoadCallback( INVALID_MODEL_INDEX, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Automate refcount tracking on a model index
//-----------------------------------------------------------------------------
inline void CRefCountedModelIndex::Set( modelindex_t i )
{
	if ( i == m_nIndex )
		return;
	modelinfo->AddRefDynamicModel( i );
	modelinfo->ReleaseDynamicModel( m_nIndex );
	m_nIndex = i;
}
#endif


#endif // IVMODELINFO_H
