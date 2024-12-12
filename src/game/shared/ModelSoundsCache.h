//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef MODELSOUNDSCACHE_H
#define MODELSOUNDSCACHE_H
#pragma once

#include "UtlCachedFileData.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#define MODELSOUNDSCACHE_VERSION		5

class CStudioHdr;

#pragma pack(1)
class CModelSoundsCache : public IBaseCacheInfo
{
public:
	CUtlVector< unsigned short > sounds;

	CModelSoundsCache();
	CModelSoundsCache( const CModelSoundsCache& src );

	void PrecacheSoundList();

	virtual void Save( CUtlBuffer& buf  );
	virtual void Restore( CUtlBuffer& buf  );
	virtual void Rebuild( char const *filename );

	static void FindOrAddScriptSound( CUtlVector< unsigned short >& sounds, char const *soundname );
	static void BuildAnimationEventSoundList( CStudioHdr *hdr, CUtlVector< unsigned short >& sounds );
private:
	char const *GetSoundName( int index );
};
#pragma pack()

extern bool ModelSoundsCache_EntryExists( const char *name );
extern CModelSoundsCache *ModelSoundsCache_Get( const char *name );

extern CStudioHdr *ModelSoundsCache_LoadModel( char const *filename );
extern void ModelSoundsCache_PrecacheScriptSound( const char *soundname );
extern void ModelSoundsCache_FinishModel( CStudioHdr *hdr );

extern bool ModelSoundsCacheInit();
extern void ModelSoundsCacheShutdown();

#endif // MODELSOUNDSCACHE_H
