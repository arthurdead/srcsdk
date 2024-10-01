#include "serverbrowser.h"
#include "ServerBrowser/IServerBrowser.h"
#include "IVGuiModule.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CServerBrowser g_ServerBrowser;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowser, IServerBrowser, SERVERBROWSER_INTERFACE_VERSION, g_ServerBrowser);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerBrowser, IVGuiModule, "VGuiModuleServerBrowser001", g_ServerBrowser);

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

bool CServerBrowser::Initialize(CreateInterfaceFn *vguiFactories, int factoryCount)
{
	return true;
}

bool CServerBrowser::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	return true;
}

bool CServerBrowser::IsValid()
{
	return true;
}

void CServerBrowser::Deactivate()
{
}

void CServerBrowser::Reactivate()
{
}

void CServerBrowser::Shutdown()
{
}

vgui::VPANEL CServerBrowser::GetPanel()
{
	return vgui::INVALID_VPANEL;
}

void CServerBrowser::SetParent(vgui::VPANEL parent)
{
}
