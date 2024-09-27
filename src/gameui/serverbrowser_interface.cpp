#include "serverbrowser_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CServerBrowser s_ServerBrowser;
static CServerBrowserVGUIModule s_ServerBrowserVGUIModule;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowser, IServerBrowser, SERVERBROWSER_INTERFACE_VERSION, s_ServerBrowser);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowserVGUIModule, IVGuiModule, "VGuiModuleServerBrowser001", s_ServerBrowserVGUIModule);

bool CServerBrowser::Activate()
{
	return true;
}

bool CServerBrowser::JoinGame( uint32 unGameIP, uint16 usGamePort )
{
	return false;
}

bool CServerBrowser::JoinGame( uint64 ulSteamIDFriend )
{
	return false;
}

bool CServerBrowser::OpenGameInfoDialog( uint64 ulSteamIDFriend )
{
	return false;
}

void CServerBrowser::CloseGameInfoDialog( uint64 ulSteamIDFriend )
{
}

void CServerBrowser::CloseAllGameInfoDialogs()
{
}

bool CServerBrowserVGUIModule::Initialize(CreateInterfaceFn *vguiFactories, int factoryCount)
{
	return true;
}

bool CServerBrowserVGUIModule::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	return true;
}

bool CServerBrowserVGUIModule::Activate()
{
	return true;
}

bool CServerBrowserVGUIModule::IsValid()
{
	return true;
}

void CServerBrowserVGUIModule::Deactivate()
{
}

void CServerBrowserVGUIModule::Reactivate()
{
}

void CServerBrowserVGUIModule::Shutdown()
{
}

vgui::VPANEL CServerBrowserVGUIModule::GetPanel()
{
	return vgui::INVALID_VPANEL;
}

void CServerBrowserVGUIModule::SetParent(vgui::VPANEL parent)
{
}
