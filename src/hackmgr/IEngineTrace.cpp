#include "engine/IEngineTrace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API ContentsFlags_t IEngineTrace::GetPointContents( const Vector &vecAbsPosition, ContentsFlags_t contentsMask, IHandleEntity** ppEntity )
{
	return DO_NOT_USE_GetPointContents( vecAbsPosition, ppEntity );
}

HACKMGR_CLASS_API ContentsFlags_t IEngineTrace::GetPointContents_WorldOnly( const Vector &vecAbsPosition, ContentsFlags_t contentsMask )
{
	return DO_NOT_USE_GetPointContents( vecAbsPosition, NULL );
}
