#include "engine/ivmodelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API bool IVModelInfo::UsesEnvCubemap( const model_t *model ) const
{
	Assert(0);
	return false;
}

HACKMGR_CLASS_API bool IVModelInfo::UsesStaticLighting( const model_t *model ) const
{
	Assert(0);
	return false;
}
