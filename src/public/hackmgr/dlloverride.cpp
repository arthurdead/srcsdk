#include "dlloverride.h"
#include "filesystem.h"
#include "tier1/strtools.h"
#if defined CLIENT_DLL || defined GAME_DLL
#include "vphysics_interface.h"
#endif
#if defined CLIENT_DLL || defined GAMEUI_EXPORTS || defined GAMEPADUI_DLL
#include "video/ivideoservices.h"
#include "tier3/tier3.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined CLIENT_DLL || defined GAME_DLL
void HackMgr_SwapVphysics( CreateInterfaceFn &physicsFactory, CreateInterfaceFn appFactory, CSysModule *&vphysicsDLL )
{
	char gamebin_path[MAX_PATH];
	g_pFullFileSystem->GetSearchPath("GAMEBIN", false, gamebin_path, ARRAYSIZE(gamebin_path));
	char *comma = V_strstr(gamebin_path,";");
	if(comma) {
		*comma = '\0';
	}
	V_AppendSlash(gamebin_path, ARRAYSIZE(gamebin_path));
	int gamebin_length = V_strlen(gamebin_path);

	if(HackMgr_IsSafeToSwapPhysics()) {
		int status = IFACE_OK;
		IPhysics *pOldPhysics = (IPhysics *)physicsFactory( VPHYSICS_INTERFACE_VERSION, &status );
		if(status != IFACE_OK) {
			pOldPhysics = NULL;
		}

		V_strcat_safe(gamebin_path, "vphysics" DLL_EXT_STRING);
		vphysicsDLL = Sys_LoadModule( gamebin_path );
		gamebin_path[gamebin_length] = '\0';

		if( vphysicsDLL != NULL )
		{
			CreateInterfaceFn physicsFactory2 = Sys_GetFactory( vphysicsDLL );
			if( physicsFactory2 != NULL )
			{
				status = IFACE_OK;
				IPhysics *pNewPhysics = (IPhysics *)physicsFactory2( VPHYSICS_INTERFACE_VERSION, &status );
				if(status != IFACE_OK) {
					pNewPhysics = NULL;
				}
				if(pOldPhysics && pNewPhysics && (pNewPhysics != pOldPhysics)) {
					pOldPhysics->Shutdown();
					pOldPhysics->Disconnect();

					bool success = false;

					if(pNewPhysics->Connect(appFactory)) {
						if(pNewPhysics->Init() == INIT_OK) {
							HackMgr_SetEnginePhysicsPtr(pOldPhysics, pNewPhysics);
							success = true;
							physicsFactory = physicsFactory2;
						} else {
							pNewPhysics->Shutdown();
							pNewPhysics->Disconnect();
						}
					} else {
						pNewPhysics->Disconnect();
					}

					if(!success) {
						pOldPhysics->Connect(appFactory);
						pOldPhysics->Init();
					}
				}
			}
		}
	}
}
#endif

#if defined CLIENT_DLL || defined GAMEUI_EXPORTS || defined GAMEPADUI_DLL
void HackMgr_SwapVideoServices( CreateInterfaceFn appFactory, CSysModule *&videoServicesDLL )
{
	char gamebin_path[MAX_PATH];
	g_pFullFileSystem->GetSearchPath("GAMEBIN", false, gamebin_path, ARRAYSIZE(gamebin_path));
	char *comma = V_strstr(gamebin_path,";");
	if(comma) {
		*comma = '\0';
	}
	V_AppendSlash(gamebin_path, ARRAYSIZE(gamebin_path));
	int gamebin_length = V_strlen(gamebin_path);

	if(HackMgr_IsSafeToSwapVideoServices()) {
		int status = IFACE_OK;
		IVideoServices *pOldVideo = (IVideoServices *)appFactory( VIDEO_SERVICES_INTERFACE_VERSION, &status );
		if(status != IFACE_OK) {
			pOldVideo = NULL;
		}

		V_strcat_safe(gamebin_path, "video_services" DLL_EXT_STRING);
		videoServicesDLL = Sys_LoadModule( gamebin_path );
		gamebin_path[gamebin_length] = '\0';

		if ( videoServicesDLL != NULL )
		{
			CreateInterfaceFn VideoServicesFactory = Sys_GetFactory( videoServicesDLL );
			if ( VideoServicesFactory != NULL )
			{
				status = IFACE_OK;
				IVideoServices *pNewVideo = (IVideoServices *)VideoServicesFactory( VIDEO_SERVICES_INTERFACE_VERSION, &status );
				if(status != IFACE_OK) {
					pNewVideo = NULL;
				}
				if ( pNewVideo && pOldVideo && (pOldVideo != pNewVideo) )
				{
					bool success = false;

					pOldVideo->Shutdown();
					pOldVideo->Disconnect();

					if( pNewVideo->Connect( appFactory ) ) {
						if( pNewVideo->Init() == INIT_OK ) {
							HackMgr_SetEngineVideoServicesPtr(pOldVideo, pNewVideo);
							g_pVideo = pNewVideo;
							success = true;
						} else {
							pNewVideo->Shutdown();
							pNewVideo->Disconnect();
						}
					} else {
						pNewVideo->Disconnect();
					}

					if(!success) {
						pOldVideo->Connect(appFactory);
						pOldVideo->Init();
					}
				}
			}
		}
	}
}
#endif
