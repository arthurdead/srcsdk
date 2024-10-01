#ifndef GAMEUISYSTEMMGR_H
#define GAMEUISYSTEMMGR_H

#include "game_controls/igameuisystemmgr.h"

class CGameUISystemMgr : public CBaseAppSystem<IGameUISystemMgr>
{
public:
	virtual void *QueryInterface( const char *pInterfaceName );

	virtual void SetGameUIVisible( bool bVisible );
	virtual bool GetGameUIVisible();

	// Load the game UI menu screen
	// key values are owned and released by caller
	virtual IGameUISystem * LoadGameUIScreen( KeyValues *kvScreenLoadSettings );
	virtual void ReleaseAllGameUIScreens();

	virtual void SetSoundPlayback( IGameUISoundPlayback *pPlayback );
	virtual void UseGameInputSystemEventQueue( bool bEnable );
	virtual void SetInputContext( InputContextHandle_t hInputContext );
	virtual void RegisterInputEvent( const InputEvent_t &iEvent );

	virtual void RunFrame();
	virtual void Render( const Rect_t &viewport, DmeTime_t flCurrentTime );
	virtual void Render( IRenderContext *pRenderContext, PlatWindow_t hWnd, const Rect_t &viewport, DmeTime_t flCurrentTime );

	virtual void RegisterScreenControllerFactory( char const *szControllerName, IGameUIScreenControllerFactory *pFactory );
	virtual IGameUIScreenControllerFactory * GetScreenControllerFactory( char const *szControllerName );

	virtual void SendEventToAllScreens( KeyValues *kvGlobalEvent );

	virtual IGameUISystemSurface * GetSurface();
	virtual IGameUISchemeMgr * GetSchemeMgr();
	virtual IGameUIMiscUtils * GetMiscUtils();

	// Init any render targets needed by the UI.
	virtual void InitRenderTargets();
	virtual IMaterialProxy *CreateProxy( const char *proxyName );
};

extern CGameUISystemMgr g_GameUISystemMgr;

#endif
