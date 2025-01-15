//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ModelSoundsCache.h"
#include "studio.h"
#include "eventlist.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

extern void VerifySequenceIndex( CStudioHdr *pstudiohdr );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hdr - 
// Output : static void
//-----------------------------------------------------------------------------

CModelSoundsCache::CModelSoundsCache()
{
}

CModelSoundsCache::CModelSoundsCache( const CModelSoundsCache& src )
{
	sounds = src.sounds;
}

char const *CModelSoundsCache::GetSoundName( int index )
{
	return g_pSoundEmitterSystem->GetSoundName( sounds[ index ] );
}

void CModelSoundsCache::Save( CUtlBuffer& buf  )
{
	buf.PutShort( sounds.Count() );
	
	for ( int i = 0; i < sounds.Count(); ++i )
	{
		buf.PutString( GetSoundName( i ) );
	}
}

void CModelSoundsCache::Restore( CUtlBuffer& buf  )
{
	MEM_ALLOC_CREDIT();
	unsigned short c;

	c = (unsigned short)buf.GetShort();

	for ( int i = 0; i < c; ++i )
	{
		char soundname[ 512 ];

		buf.GetString( soundname, sizeof( soundname ) );

		int idx = g_pSoundEmitterSystem->GetSoundIndex( soundname );
		if ( idx != -1 )
		{
			Assert( idx <= 65535 );
			if ( sounds.Find( idx ) == sounds.InvalidIndex() )
			{
				sounds.AddToTail( (unsigned short)idx );
			}
		}
	}
}

void CModelSoundsCache::Rebuild( char const *filename )
{
	sounds.RemoveAll();

	CStudioHdr *hdr = ModelSoundsCache_LoadModel( filename );

	if ( hdr )
	{
		// Precache all sounds referenced in animation events
		BuildAnimationEventSoundList( hdr, sounds );
		ModelSoundsCache_FinishModel( hdr );
	}
}

void CModelSoundsCache::PrecacheSoundList()
{
	for ( int i = 0; i < sounds.Count(); ++i )
	{
		ModelSoundsCache_PrecacheScriptSound( GetSoundName( i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : sounds - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CModelSoundsCache::FindOrAddScriptSound( CUtlVector< unsigned short >& sounds, char const *soundname )
{
	int soundindex = g_pSoundEmitterSystem->GetSoundIndex( soundname );
	if ( soundindex != -1 )
	{
		// Only add it once per model...
		if ( sounds.Find( soundindex ) == sounds.InvalidIndex() )
		{
			MEM_ALLOC_CREDIT();
			sounds.AddToTail( soundindex );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : *hdr - 
//			sounds - 
//-----------------------------------------------------------------------------
void CModelSoundsCache::BuildAnimationEventSoundList( const CStudioHdr *hdr, CUtlVector< unsigned short >& sounds )
{
	Assert( hdr );
	
	// force animation event resolution!!!
	VerifySequenceIndex( (CStudioHdr *)hdr );

	// Find all animation events which fire off sound script entries...
	for ( int iSeq=0; iSeq < hdr->GetNumSeq(); iSeq++ )
	{
		const mstudioseqdesc_t *pSeq = &hdr->pSeqdesc( (sequence_t)iSeq );
		
		// Now read out all the sound events with their timing
		for ( int iEvent=0; iEvent < (int)pSeq->numevents; iEvent++ )
		{
			const mstudioevent_t *pEvent = pSeq->pEvent( iEvent );

			int nEvent = pEvent->Event();
			
			switch ( nEvent )
			{
			case AE_SV_PLAYSOUND:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;
			// Old-style client .dll animation event
			case CL_EVENT_SOUND:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;
			case CL_EVENT_FOOTSTEP_LEFT:
			case CL_EVENT_FOOTSTEP_RIGHT:
				{
					char soundname[256];
					char const *options = pEvent->pszOptions();
					if ( !options || !options[0] )
					{
						options = "NPC_CombineS";
					}

					Q_snprintf( soundname, 256, "%s.RunFootstepLeft", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.RunFootstepRight", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.FootstepLeft", options );
					FindOrAddScriptSound( sounds, soundname );
					Q_snprintf( soundname, 256, "%s.FootstepRight", options );
					FindOrAddScriptSound( sounds, soundname );
				}
				break;
			case AE_CL_PLAYSOUND:
				{
					if ( pEvent->pszOptions()[0] )
					{
						FindOrAddScriptSound( sounds, pEvent->pszOptions() );
					}
					else
					{
						Warning( "-- Error --:  empty soundname, .qc error on AE_CL_PLAYSOUND in model %s, sequence %s, animevent # %i\n", 
							hdr->pszName(), pSeq->pszLabel(), iEvent+1 );
					}
				}
				break;
			case SCRIPT_EVENT_SOUND:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;

			case SCRIPT_EVENT_SOUND_VOICE:
				{
					FindOrAddScriptSound( sounds, pEvent->pszOptions() );
				}
				break;
			}
		}
	}
}

CStudioHdr *ModelSoundsCache_LoadModel( const char *filename )
{
	// Load the file
#ifdef GAME_DLL
	modelindex_t idx = engine->PrecacheModel( filename, true );
#else
	modelindex_t idx = modelinfo->GetModelIndex( filename );
#endif
	if ( IsValidModelIndex( idx ) )
	{
		const model_t *mdl = (model_t *)modelinfo->GetModel( idx );
		if ( mdl )
		{
			CStudioHdr *studioHdr = new CStudioHdr( modelinfo->GetStudiomodel( mdl ), g_pMDLCache ); 
			if ( studioHdr->IsValid() )
			{
				return studioHdr;
			}
		}
	}
	return NULL;
}

void ModelSoundsCache_FinishModel( CStudioHdr *hdr )
{
	Assert( hdr );
	delete hdr;
}

void ModelSoundsCache_PrecacheScriptSound( const char *soundname )
{
	CSharedBaseEntity::PrecacheScriptSound( soundname );
}

CUtlCachedFileData< CModelSoundsCache > g_ModelSoundsCache( 
#ifdef GAME_DLL
	"modelsounds_server.cache"
#else
	"modelsounds_client.cache"
#endif
	, MODELSOUNDSCACHE_VERSION, 0, UTL_CACHED_FILE_USE_FILESIZE, false );

bool ModelSoundsCache_EntryExists( const char *name )
{
	return g_ModelSoundsCache.EntryExists( name );
}

CModelSoundsCache *ModelSoundsCache_Get( const char *name )
{
	return g_ModelSoundsCache.Get( name );
}

void ClearModelSoundsCache()
{
	g_ModelSoundsCache.Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ModelSoundsCacheInit()
{
	return g_ModelSoundsCache.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ModelSoundsCacheShutdown()
{
	g_ModelSoundsCache.Shutdown();
}

static CUtlSymbolTable g_ModelSoundsSymbolHelper( 0, 32, true );
class CModelSoundsCacheSaver: public CAutoGameSystem
{
public:
	CModelSoundsCacheSaver( const char *name ) : CAutoGameSystem( name )
	{
	}
	virtual void LevelInitPostEntity()
	{
		if ( g_ModelSoundsCache.IsDirty() )
		{
			g_ModelSoundsCache.Save();
		}
	}
	virtual void LevelShutdownPostEntity()
	{
		if ( g_ModelSoundsCache.IsDirty() )
		{
			g_ModelSoundsCache.Save();
		}
	}
};

static CModelSoundsCacheSaver g_ModelSoundsCacheSaver( "CModelSoundsCacheSaver" );
