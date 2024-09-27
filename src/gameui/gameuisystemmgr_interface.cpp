#include "gameuisystemmgr_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGameUISystemMgr g_GameUISystemMgr;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUISystemMgr, IGameUISystemMgr, GAMEUISYSTEMMGR_INTERFACE_VERSION, g_GameUISystemMgr);

void *CGameUISystemMgr::QueryInterface( const char *pInterfaceName )
{
	if (!Q_strcmp(	pInterfaceName, GAMEUISYSTEMMGR_INTERFACE_VERSION ))
		return (IGameUISystemMgr*)this;

	return NULL;
}

void CGameUISystemMgr::SetGameUIVisible( bool bVisible )
{
}

bool CGameUISystemMgr::GetGameUIVisible()
{
	return false;
}

IGameUISystem *CGameUISystemMgr::LoadGameUIScreen( KeyValues *kvScreenLoadSettings )
{
	return NULL;
}

void CGameUISystemMgr::ReleaseAllGameUIScreens()
{
}

void CGameUISystemMgr::SetSoundPlayback( IGameUISoundPlayback *pPlayback )
{
}

void CGameUISystemMgr::UseGameInputSystemEventQueue( bool bEnable )
{
}

void CGameUISystemMgr::SetInputContext( InputContextHandle_t hInputContext )
{
}

void CGameUISystemMgr::RegisterInputEvent( const InputEvent_t &iEvent )
{
}

void CGameUISystemMgr::RunFrame()
{
}

void CGameUISystemMgr::Render( const Rect_t &viewport, DmeTime_t flCurrentTime )
{
}

void CGameUISystemMgr::Render( IRenderContext *pRenderContext, PlatWindow_t hWnd, const Rect_t &viewport, DmeTime_t flCurrentTime )
{
}

void CGameUISystemMgr::RegisterScreenControllerFactory( char const *szControllerName, IGameUIScreenControllerFactory *pFactory )
{
}

IGameUIScreenControllerFactory *CGameUISystemMgr::GetScreenControllerFactory( char const *szControllerName )
{
	return NULL;
}

void CGameUISystemMgr::SendEventToAllScreens( KeyValues *kvGlobalEvent )
{
}

IGameUISystemSurface *CGameUISystemMgr::GetSurface()
{
	return NULL;
}

IGameUISchemeMgr *CGameUISystemMgr::GetSchemeMgr()
{
	return NULL;
}

IGameUIMiscUtils *CGameUISystemMgr::GetMiscUtils()
{
	return NULL;
}

void CGameUISystemMgr::InitRenderTargets()
{
}

IMaterialProxy *CGameUISystemMgr::CreateProxy( const char *proxyName )
{
	return NULL;
}
