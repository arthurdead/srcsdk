#include "engine/IStaticPropMgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef SWDS
HACKMGR_CLASS_API void IStaticPropMgrClient::GetLightingOrigins( Vector *pLightingOrigins, int nOriginStride, int nCount, IClientRenderable **ppRenderable, int nRenderableStride )
{
	Assert(0);
}

HACKMGR_CLASS_API void IStaticPropMgrClient::DrawStaticProps( IClientRenderable **pProps, const RenderableInstance_t *pInstances, int count, bool bShadowDepth, bool drawVCollideWireframe )
{
	Assert(0);
}
#endif
