//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "datacache/imdlcache.h"
#include "utlvector.h"
#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Pointer to a member method of IGameSystem
typedef void (IGameSystem::*GameSystemFunc_t)();

// Pointer to a member method of IGameSystem
typedef void (IGameSystemPerFrame::*PerFrameGameSystemFunc_t)();

// Used to invoke a method of all added Game systems in order
static void InvokeMethod( GameSystemFunc_t f, char const *timed = 0 );
// Used to invoke a method of all added Game systems in order
static void InvokeMethodTickProgress( GameSystemFunc_t f, char const *timed = 0 );
// Used to invoke a method of all added Game systems in reverse order
static void InvokeMethodReverseOrder( GameSystemFunc_t f );

// Used to invoke a method of all added Game systems in order
static void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed = 0 );

static bool s_bSystemsInitted = false; 

// List of all installed Game systems
static CUtlVector<IGameSystem*> s_GameSystems( 0, 4 );
// List of all installed Game systems
static CUtlVector<IGameSystemPerFrame*> s_GameSystemsPerFrame( 0, 4 );

static CSharedBasePlayer *s_pRunCommandPlayer = NULL;
static CUserCmd *s_pRunCommandUserCmd = NULL;

//-----------------------------------------------------------------------------
// Auto-registration of game systems
//-----------------------------------------------------------------------------
static	CAutoGameSystem *s_pSystemList = NULL;

CAutoGameSystem::CAutoGameSystem( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pSystemList;
		s_pSystemList = this;
	}
}

static	CAutoGameSystemPerFrame *s_pPerFrameSystemList = NULL;

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
CAutoGameSystemPerFrame::CAutoGameSystemPerFrame( char const *name ) :
	m_pszName( name )
{
	// If s_GameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_GameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pPerFrameSystemList;
		s_pPerFrameSystemList = this;
	}
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystem::~IGameSystem()
{
	Remove( this );
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystemPerFrame::~IGameSystemPerFrame()
{
	Remove( this );
}


//-----------------------------------------------------------------------------
// Adds a system to the list of systems to run
//-----------------------------------------------------------------------------
void IGameSystem::Add( IGameSystem* pSys )
{
	s_GameSystems.AddToTail( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.AddToTail( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}


//-----------------------------------------------------------------------------
// Removes a system from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::Remove( IGameSystem* pSys )
{
	s_GameSystems.FindAndRemove( pSys );
	if ( dynamic_cast< IGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_GameSystemsPerFrame.FindAndRemove( static_cast< IGameSystemPerFrame * >( pSys ) );
	}
}

//-----------------------------------------------------------------------------
// Removes *all* systems from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::RemoveAll(  )
{
	s_GameSystems.RemoveAll();
	s_GameSystemsPerFrame.RemoveAll();
}

#ifndef CLIENT_DLL
CBasePlayer *IGameSystem::RunCommandPlayer()
{
	return s_pRunCommandPlayer;
}

CUserCmd *IGameSystem::RunCommandUserCmd()
{
	return s_pRunCommandUserCmd;
}
#endif

//-----------------------------------------------------------------------------
// Invokes methods on all installed game systems
//-----------------------------------------------------------------------------
bool IGameSystem::InitAllSystems()
{
	int i;

	{
		// first add any auto systems to the end
		CAutoGameSystem *pSystem = s_pSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}
			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}

	{
		CAutoGameSystemPerFrame *pSystem = s_pPerFrameSystemList;
		while ( pSystem )
		{
			if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}

			pSystem = pSystem->m_pNext;
		}
		s_pPerFrameSystemList = NULL;
	}
	// Now remember that we are initted so new CAutoGameSystems will add themselves automatically.
	s_bSystemsInitted = true;

	for ( i = 0; i < s_GameSystems.Count(); ++i )
	{
		MDLCACHE_CRITICAL_SECTION();

		IGameSystem *sys = s_GameSystems[i];

		bool valid = sys->Init();
		if ( !valid )
		{
			Error("%s failed to init\n", sys->Name());
			return false;
		}
	}

	return true;
}

void IGameSystem::PostInitAllSystems( void )
{
	InvokeMethod( &IGameSystem::PostInit, "PostInit" );
}

void IGameSystem::ShutdownAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::Shutdown );
}

void IGameSystem::LevelInitPreEntityAllSystems()
{
	InvokeMethodTickProgress( &IGameSystem::LevelInitPreEntity, "LevelInitPreEntity" );
}

void IGameSystem::LevelInitPostEntityAllSystems( void )
{
	InvokeMethod( &IGameSystem::LevelInitPostEntity, "LevelInitPostEntity" );
}

void IGameSystem::LevelShutdownPreClearSteamAPIContextAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreClearSteamAPIContext );
}

void IGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreEntity );
}

void IGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPostEntity );
}

void IGameSystem::OnSaveAllSystems()
{
	InvokeMethod( &IGameSystem::OnSave );
}

void IGameSystem::OnRestoreAllSystems()
{
	InvokeMethod( &IGameSystem::OnRestore );
}

void IGameSystem::SafeRemoveIfDesiredAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::SafeRemoveIfDesired );
}

#ifdef CLIENT_DLL

void IGameSystem::PreRenderAllSystems()
{
	VPROF("IGameSystem::PreRenderAllSystems");
	InvokePerFrameMethod( &IGameSystemPerFrame::PreRender );
}

void IGameSystem::UpdateAllSystems( float frametime )
{
	SafeRemoveIfDesiredAllSystems();

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c; ++i )
	{
		IGameSystemPerFrame *sys = s_GameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		sys->Update( frametime );
	}
}

void IGameSystem::PostRenderAllSystems()
{
	InvokePerFrameMethod( &IGameSystemPerFrame::PostRender );
}

#else

void IGameSystem::FrameUpdatePreEntityThinkAllSystems()
{
	VPROF("FrameUpdatePreEntityThinkAllSystems");
	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePreEntityThink );
}

void IGameSystem::FrameUpdatePostEntityThinkAllSystems()
{
	VPROF("FrameUpdatePostEntityThinkAllSystems");
	SafeRemoveIfDesiredAllSystems();

	InvokePerFrameMethod( &IGameSystemPerFrame::FrameUpdatePostEntityThink );
}

void IGameSystem::PreClientUpdateAllSystems() 
{
	VPROF("PreClientUpdateAllSystems");
	InvokePerFrameMethod( &IGameSystemPerFrame::PreClientUpdate );
}

#endif

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethodTickProgress( GameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_GameSystems[i];

		MDLCACHE_COARSE_LOCK();
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethod( GameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_GameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_GameSystemsPerFrame.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystemPerFrame *sys  = s_GameSystemsPerFrame[i];
#if (VPROF_LEVEL > 0) && defined(VPROF_ACCOUNT_GAMESYSTEMS)   // make sure each game system is individually attributed
		// because vprof nodes must really be constructed with a pointer to a static
		// string, we can't create a temporary char[] here and sprintf a distinctive
		// V_snprintf( buf, 63, "gamesys_preframe_%s", sys->Name() ). We'll have to
		// settle for just the system name, and distinguish between pre and post frame
		// in hierarchy.
		VPROF( sys->Name() );
#endif
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in reverse order
//-----------------------------------------------------------------------------
void InvokeMethodReverseOrder( GameSystemFunc_t f )
{
	int i;
	int c = s_GameSystems.Count();
	for ( i = c; --i >= 0; )
	{
		IGameSystem *sys = s_GameSystems[i];
#if (VPROF_LEVEL > 0) && defined(VPROF_ACCOUNT_GAMESYSTEMS)   // make sure each game system is individually attributed
		// because vprof nodes must really be constructed with a pointer to a static
		// string, we can't create a temporary char[] here and sprintf a distinctive
		// V_snprintf( buf, 63, "gamesys_preframe_%s", sys->Name() ). We'll have to
		// settle for just the system name, and distinguish between pre and post frame
		// in hierarchy.
		VPROF( sys->Name() );
#endif
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}


