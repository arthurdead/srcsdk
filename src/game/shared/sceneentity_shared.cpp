//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "sceneentity_shared.h"
#include "choreoscene.h"
#ifdef GAME_DLL
#include "env_debughistory.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined CLIENT_DLL || (defined GAME_DLL && defined DISABLE_DEBUG_HISTORY)
#define LocalScene_Printf Scene_Printf
#else
extern void LocalScene_Printf( const char *pFormat, ... );
#endif

static ConVar scene_print( "scene_print", "0", FCVAR_REPLICATED, "When playing back a scene, print timing and event info to console." );
ConVar scene_clientflex( "scene_clientflex", "1", FCVAR_REPLICATED, "Do client side flex animation." );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFormat - 
//			... - 
// Output : static void
//-----------------------------------------------------------------------------
void Scene_Printf( const char *pFormat, ... )
{
	int val = scene_print.GetInt();
	if ( !val )
		return;

	if ( val >= 2 )
	{
	#ifdef GAME_DLL
		if ( val != 2 )
		{
			return;
		}
	#else
		if ( val != 3 )
		{
			return;
		}
	#endif
	}

	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof(msg), pFormat, marker);
	va_end(marker);	

#ifdef GAME_DLL
	Msg( "%8.3f[%d] sv:  %s", gpGlobals->curtime, gpGlobals->tickcount, msg );
#else
	Msg( "%8.3f[%d] cl:  %s", gpGlobals->curtime, gpGlobals->tickcount, msg );
#endif
}

//-----------------------------------------------------------------------------
// I copied CSceneEntity's PrecacheScene to a unique static function so PrecacheInstancedScene()
// can precache loose scene files without having to use a CSceneEntity.
//-----------------------------------------------------------------------------
void PrecacheChoreoScene( CChoreoScene *scene )
{
	Assert( scene );

	// Iterate events and precache necessary resources
	for ( int i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SPEAK:
			{
				// Defined in SoundEmitterSystem.cpp
				// NOTE:  The script entries associated with .vcds are forced to preload to avoid
				//  loading hitches during triggering
				CBaseEntity::PrecacheScriptSound( event->GetParameters() );

				if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER && 
					 event->GetNumSlaves() > 0 )
				{
					char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
					if ( event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) ) )
					{
						CBaseEntity::PrecacheScriptSound( tok );
					}
				}
			}
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( !subscene )
					{
						subscene = ChoreoLoadScene( event->GetParameters(), NULL, &g_TokenProcessor, LocalScene_Printf );
						subscene->SetSubScene( true );
						event->SetSubScene( subscene );

						// Now precache it's resources, if any
						PrecacheChoreoScene( subscene );
					}
				}
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FreeSceneFileMemory( void *buffer )
{
	delete[] (byte*) buffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSceneTokenProcessor::CurrentToken( void )
{
	return m_szToken;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : crossline - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::GetToken( bool crossline )
{
	// NOTE: crossline is ignored here, may need to implement if needed
	m_pBuffer = engine->ParseFile( m_pBuffer, m_szToken, sizeof( m_szToken ) );
	if ( m_szToken[0] )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneTokenProcessor::TokenAvailable( void )
{
	char const *search_p = m_pBuffer;

	while ( *search_p <= 32)
	{
		if (*search_p == '\n')
			return false;
		search_p++;
		if ( !*search_p )
			return false;

	}

	if (*search_p == ';' || *search_p == '#' ||		 // semicolon and # is comment field
		(*search_p == '/' && *((search_p)+1) == '/')) // also make // a comment field
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::Error( const char *fmt, ... )
{
	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof(string), fmt, argptr );
	va_end( argptr );

	Warning( "%s", string );
	Assert(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
//-----------------------------------------------------------------------------
void CSceneTokenProcessor::SetBuffer( char *buffer )
{
	m_pBuffer = buffer;
}

CSceneTokenProcessor g_TokenProcessor;