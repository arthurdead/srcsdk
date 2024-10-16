#ifndef SERVERBROWSER_H
#define SERVERBROWSER_H

#pragma once

#include "ServerBrowser/IServerBrowser.h"
#include "IVGuiModule.h"

class CServerBrowser : public IServerBrowser, public IVGuiModule
{
public:
	// activates the server browser window, brings it to the foreground
	virtual bool Activate();

	// joins a game directly
	virtual bool JoinGame( uint32 unGameIP, uint16 usGamePort );

	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	virtual bool JoinGame( uint64 ulSteamIDFriend );

	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	virtual bool OpenGameInfoDialog( uint64 ulSteamIDFriend );

	// forces the game info dialog closed
	virtual void CloseGameInfoDialog( uint64 ulSteamIDFriend );

	// closes all the game info dialogs
	virtual void CloseAllGameInfoDialogs();

	// called first to setup the module with the vgui
	// returns true on success, false on failure
	virtual bool Initialize(CreateInterfaceFn *vguiFactories, int factoryCount);

	// called after all the modules have been initialized
	// modules should use this time to link to all the other module interfaces
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount);

	// returns true if the module is successfully initialized and available
	virtual bool IsValid();

	// requests that the UI is temporarily disabled and all data files saved
	virtual void Deactivate();

	// restart from a Deactivate()
	virtual void Reactivate();

	// called when the module is about to be shutdown
	virtual void Shutdown();

	// returns a handle to the main module panel
	virtual vgui::VPANEL GetPanel();

	// sets the parent of the main module panel
	virtual void SetParent(vgui::VPANEL parent);
};

extern CServerBrowser g_ServerBrowser;

#endif
