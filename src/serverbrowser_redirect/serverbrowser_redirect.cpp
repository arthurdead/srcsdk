#include "ServerBrowser/IServerBrowser.h"
#include "tier1/interface.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "IVGuiModule.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CServerBrowserRedirect : public IServerBrowser
{
public:
	// activates the server browser window, brings it to the foreground
	virtual bool Activate()
	{ return m_pTarget->Activate(); }

	// joins a game directly
	virtual bool JoinGame( uint32 unGameIP, uint16 usGamePort )
	{ return m_pTarget->JoinGame(unGameIP, usGamePort); }

	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	virtual bool JoinGame( uint64 ulSteamIDFriend )
	{ return m_pTarget->JoinGame(ulSteamIDFriend); }

	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	virtual bool OpenGameInfoDialog( uint64 ulSteamIDFriend )
	{ return m_pTarget->OpenGameInfoDialog(ulSteamIDFriend); }

	// forces the game info dialog closed
	virtual void CloseGameInfoDialog( uint64 ulSteamIDFriend )
	{ m_pTarget->CloseGameInfoDialog(ulSteamIDFriend); }

	// closes all the game info dialogs
	virtual void CloseAllGameInfoDialogs()
	{ m_pTarget->CloseAllGameInfoDialogs(); }

	bool GetRedirectTarget();

 private:
	IServerBrowser *m_pTarget;
};

class CServerBrowserVGUIModuleRedirect : public IVGuiModule
{
#ifdef __MINGW32__
private:
	void __DTOR__()
	{
		this->~CServerBrowserVGUIModuleRedirect();
	}
#endif

public:
	// called first to setup the module with the vgui
	// returns true on success, false on failure
	virtual bool Initialize(CreateInterfaceFn *vguiFactories, int factoryCount)
	{ return m_pTarget->Initialize(vguiFactories, factoryCount); }

	// called after all the modules have been initialized
	// modules should use this time to link to all the other module interfaces
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount)
	{ return m_pTarget->PostInitialize(modules, factoryCount); }

	// called when the module is selected from the menu or otherwise activated
	virtual bool Activate()
	{ return m_pTarget->Activate(); }

	// returns true if the module is successfully initialized and available
	virtual bool IsValid()
	{ return m_pTarget->IsValid(); }

	// requests that the UI is temporarily disabled and all data files saved
	virtual void Deactivate()
	{ m_pTarget->Deactivate(); }

	// restart from a Deactivate()
	virtual void Reactivate()
	{ m_pTarget->Reactivate(); }

	// called when the module is about to be shutdown
	virtual void Shutdown()
	{ m_pTarget->Shutdown(); }

	// returns a handle to the main module panel
	virtual vgui::VPANEL GetPanel()
	{ return m_pTarget->GetPanel(); }

	// sets the parent of the main module panel
	virtual void SetParent(vgui::VPANEL parent)
	{ m_pTarget->SetParent(parent); }

	bool GetRedirectTarget();

 private:
	IVGuiModule *m_pTarget;
};

bool CServerBrowserRedirect::GetRedirectTarget()
{
	CSysModule *pTargetMod = NULL;

	const char *pGameDir = CommandLine()->ParmValue( "-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );

	char szTargetPath[MAX_PATH];
	V_strncpy(szTargetPath, pGameDir, sizeof(szTargetPath));
	V_AppendSlash(szTargetPath, sizeof(szTargetPath));
	V_strcat(szTargetPath, "bin" CORRECT_PATH_SEPARATOR_S "GameUI" DLL_EXT_STRING, sizeof(szTargetPath));

	pTargetMod = Sys_LoadModule(szTargetPath);
	if(!pTargetMod) {
		return false;
	}

	CreateInterfaceFn pFactory = Sys_GetFactory(pTargetMod);
	if(!pFactory) {
		return false;
	}

	int status = IFACE_OK;
	m_pTarget = (IServerBrowser *)pFactory(SERVERBROWSER_INTERFACE_VERSION, &status);
	if(!m_pTarget || status != IFACE_OK) {
		return false;
	}

	return m_pTarget != NULL;
}

bool CServerBrowserVGUIModuleRedirect::GetRedirectTarget()
{
	CSysModule *pTargetMod = NULL;

	const char *pGameDir = CommandLine()->ParmValue( "-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );

	char szTargetPath[MAX_PATH];
	V_strncpy(szTargetPath, pGameDir, sizeof(szTargetPath));
	V_AppendSlash(szTargetPath, sizeof(szTargetPath));
	V_strcat(szTargetPath, "bin" CORRECT_PATH_SEPARATOR_S "GameUI" DLL_EXT_STRING, sizeof(szTargetPath));

	pTargetMod = Sys_LoadModule(szTargetPath);
	if(!pTargetMod) {
		return false;
	}

	CreateInterfaceFn pFactory = Sys_GetFactory(pTargetMod);
	if(!pFactory) {
		return false;
	}

	int status = IFACE_OK;
	m_pTarget = (IVGuiModule *)pFactory("VGuiModuleServerBrowser001", &status);
	if(!m_pTarget || status != IFACE_OK) {
		return false;
	}

	return m_pTarget != NULL;
}

void *CreateServerBrowser()
{
	static CServerBrowserRedirect *pServerBrowser = NULL;
	if(!pServerBrowser) {
		pServerBrowser = new CServerBrowserRedirect;
		if(!pServerBrowser->GetRedirectTarget()) {
			delete pServerBrowser;
			return NULL;
		}
	}
	return pServerBrowser;
}

void *CreateServerBrowserVGUIModule()
{
	static CServerBrowserVGUIModuleRedirect *pServerBrowserVGUIModule = NULL;
	if(!pServerBrowserVGUIModule) {
		pServerBrowserVGUIModule = new CServerBrowserVGUIModuleRedirect;
		if(!pServerBrowserVGUIModule->GetRedirectTarget()) {
			delete pServerBrowserVGUIModule;
			return NULL;
		}
	}
	return pServerBrowserVGUIModule;
}

EXPOSE_INTERFACE_FN(CreateServerBrowser, IServerBrowser, SERVERBROWSER_INTERFACE_VERSION);
EXPOSE_INTERFACE_FN(CreateServerBrowserVGUIModule, IVGuiModule, "VGuiModuleServerBrowser001");
