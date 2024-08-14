#include "cbase.h"
#include "clientmode_heist.h"
#include "ivmodemanager.h"
#include "panelmetaclassmgr.h"
#include "vgui/heistviewport.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SCREEN_FILE "scripts/vgui_screens.txt"

ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );

IClientMode *g_pClientMode = NULL;

class CHeistModeManager : public IVModeManager
{
public:
	void Init() override;

	void SwitchMode(bool commander, bool force) override
	{
	}

	void LevelInit(const char *newmap) override;
	void LevelShutdown() override;

	void ActivateMouse(bool isactive)
	{
	}
};

static CHeistModeManager g_ModeManager;
IVModeManager *modemanager = (IVModeManager *)&g_ModeManager;

ClientModeHeistNormal g_ClientModeNormal;

void CHeistModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();

	PanelMetaClassMgr()->LoadMetaClassDefinitionFile(SCREEN_FILE);
}

void CHeistModeManager::LevelInit(const char *newmap)
{
	g_pClientMode->LevelInit(newmap);
}

void CHeistModeManager::LevelShutdown()
{
	g_pClientMode->LevelShutdown();
}

ClientModeHeistNormal::ClientModeHeistNormal()
{
}

ClientModeHeistNormal::~ClientModeHeistNormal()
{
}

void ClientModeHeistNormal::InitViewport()
{
	m_pViewport = new HeistViewport();
	m_pViewport->Start(gameuifuncs, gameeventmanager);
}

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}

ClientModeHeistNormal* GetClientModeSDKNormal()
{
	Assert(dynamic_cast< ClientModeHeistNormal *>(GetClientModeNormal()));
	return static_cast< ClientModeHeistNormal *>(GetClientModeNormal());
}

float ClientModeHeistNormal::GetViewModelFOV()
{
	return 74.0f;
}

int ClientModeHeistNormal::GetDeathMessageStartHeight()
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeHeistNormal::PostRenderVGui()
{
}
