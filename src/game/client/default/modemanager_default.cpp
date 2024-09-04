#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode.h"
#include "panelmetaclassmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( int mode, bool force );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CModeManager::Init()
{
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
}

void CModeManager::SwitchMode( int mode, bool force )
{
	GetClientMode()->Enable();
}

void CModeManager::LevelInit( const char *newmap )
{
	GetClientMode()->LevelInit( newmap );
}

void CModeManager::LevelShutdown( void )
{
	GetClientMode()->LevelShutdown();
}
