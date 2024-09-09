#include "engine/ivmodelrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API void IVModelRender::ComputeLightingState( int nCount, const LightingQuery_t *pQuery, MaterialLightingState_t *pState, ITexture **ppEnvCubemapTexture )
{
	Assert(0);
}

HACKMGR_CLASS_API void IVModelRender::GetModelDecalHandles( StudioDecalHandle_t *pDecals, int nDecalStride, int nCount, const ModelInstanceHandle_t *pHandles )
{
	Assert(0);
}

HACKMGR_CLASS_API void IVModelRender::CleanupStaticLightingState( int nCount, DataCacheHandle_t *pColorMeshHandles )
{
	Assert(0);
}

HACKMGR_CLASS_API void IVModelRender::ComputeStaticLightingState( int nCount, const StaticLightingQuery_t *pQuery, MaterialLightingState_t *pState, MaterialLightingState_t *pDecalState, ColorMeshInfo_t **ppStaticLighting, ITexture **ppEnvCubemapTexture, DataCacheHandle_t *pColorMeshHandles )
{
	Assert(0);
}