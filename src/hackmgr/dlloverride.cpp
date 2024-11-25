#define IAPPSYSTEMGROUP_H

#include "dlloverride_internal.h"
#include "hackmgr/hackmgr.h"
#include "hackmgr_internal.h"
#include "commandline.h"
#ifndef SWDS
#include "game_loopback/igameloopback.h"
#endif

#ifdef __linux__
#include <dlfcn.h>
#endif

#include "filesystem.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"
#include "createinterface.h"
#include "vphysics_interface.h"
#ifndef SWDS
#include "video/ivideoservices.h"
#endif
#include "engine_hlds_api.h"
#include "matchmaking/imatchframework.h"
#ifndef SWDS
#include "IHammer.h"
#endif

#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "appframework/IAppSystem.h"
#include "tier3/tier3.h"

#undef IAPPSYSTEMGROUP_H

#define private public
#define protected public
#include "appframework/IAppSystemGroup.h"
#undef private
#undef protected

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef SWDS
abstract_class IEngineAPI : public IAppSystem
{
};

static constexpr int CEngineAPI_m_StartupInfo_offset = (
	sizeof(CTier3AppSystem<IEngineAPI>) +
	sizeof(void *) +
	sizeof(bool) +
	sizeof(bool) +
	2
);

static constexpr int StartupInfo_t_m_pParentAppSystemGroup_offset = (
	sizeof(void *) +
	sizeof(const char *) +
	sizeof(const char *) +
	sizeof(const char *)
);
#endif

static constexpr int CDedicatedServerAPI_m_pDedicatedServer_offset = (
	sizeof(CTier3AppSystem<IDedicatedServerAPI>)
);

static CAppSystemGroup *s_pCurrentAppSystemHack = NULL;
static CAppSystemGroup *s_pLauncherAppSystem = NULL;

static void *CAppSystemGroup_FindSystem( CAppSystemGroup *pthis, const char *pSystemName )
{
	unsigned short i = pthis->m_SystemDict.Find( pSystemName );
	if (i != pthis->m_SystemDict.InvalidIndex())
		return pthis->m_Systems[pthis->m_SystemDict[i]];

	for ( i = 0; i < pthis->m_Systems.Count(); ++i )
	{
		void *pInterface = pthis->m_Systems[i]->QueryInterface( pSystemName );
		if (pInterface)
			return pInterface;
	}

	if ( pthis->m_pParentAppSystem )
	{
		void* pInterface = CAppSystemGroup_FindSystem( pthis->m_pParentAppSystem, pSystemName );
		if ( pInterface )
			return pInterface;
	}

	return NULL;
}

static void CAppSystemGroup_AddSystem( CAppSystemGroup *pthis, IAppSystem *pAppSystem, const char *pInterfaceName )
{
	if ( !pAppSystem )
		return;

	int sysIndex = pthis->m_Systems.AddToTail( pAppSystem );

	MEM_ALLOC_CREDIT();
	pthis->m_SystemDict.Insert( pInterfaceName, sysIndex );
}

void *Launcher_AppSystemCreateInterface(const char *pName, int *pReturnCode)
{
	void *pInterface = s_pLauncherAppSystem ? CAppSystemGroup_FindSystem( s_pLauncherAppSystem, pName ) : NULL;
	if ( pReturnCode )
	{
		*pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;
	}
	return pInterface;
}

static void *AppSystemCreateInterfaceFnHack(const char *pName, int *pReturnCode)
{
	void *pInterface = s_pCurrentAppSystemHack ? CAppSystemGroup_FindSystem( s_pCurrentAppSystemHack, pName ) : NULL;
	if ( pReturnCode )
	{
		*pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;
	}
	return pInterface;
}

#ifndef SWDS
class CVideoServicesRedirect : public IVideoServices
{
public:
	static inline IVideoServices *source = NULL;
	static inline IVideoServices *target = NULL;

	virtual bool Connect( CreateInterfaceFn factory )
	{ return target->Connect(factory); }
	virtual void Disconnect()
	{ target->Disconnect(); }
	virtual void *QueryInterface( const char *pInterfaceName )
	{ return target->QueryInterface(pInterfaceName); }
	virtual InitReturnVal_t Init()
	{ return target->Init(); }
	virtual void Shutdown()
	{ target->Shutdown(); }

	virtual int GetAvailableVideoSystemCount()
	{ return target->GetAvailableVideoSystemCount(); }
	virtual VideoSystem_t GetAvailableVideoSystem( int n )
	{ return target->GetAvailableVideoSystem(n); }
	virtual bool IsVideoSystemAvailable( VideoSystem_t videoSystem )
	{ return target->IsVideoSystemAvailable(videoSystem); }
	virtual VideoSystemStatus_t GetVideoSystemStatus( VideoSystem_t videoSystem )
	{ return target->GetVideoSystemStatus(videoSystem); }
	virtual VideoSystemFeature_t GetVideoSystemFeatures( VideoSystem_t videoSystem )
	{ return target->GetVideoSystemFeatures(videoSystem); }
	virtual const char *GetVideoSystemName( VideoSystem_t videoSystem )
	{ return target->GetVideoSystemName(videoSystem); }
	virtual VideoSystem_t FindNextSystemWithFeature( VideoSystemFeature_t features, VideoSystem_t startAfter )
	{ return target->FindNextSystemWithFeature(features, startAfter); }
	virtual VideoResult_t GetLastResult()
	{ return target->GetLastResult(); }
	virtual int GetSupportedFileExtensionCount( VideoSystem_t videoSystem )
	{ return target->GetSupportedFileExtensionCount(videoSystem); }
	virtual const char *GetSupportedFileExtension( VideoSystem_t videoSystem, int extNum )
	{ return target->GetSupportedFileExtension(videoSystem, extNum); }
	virtual VideoSystemFeature_t GetSupportedFileExtensionFeatures( VideoSystem_t videoSystem, int extNum )
	{ return target->GetSupportedFileExtensionFeatures(videoSystem, extNum); }
	virtual	VideoSystem_t LocateVideoSystemForPlayingFile( const char *pFileName, VideoSystemFeature_t playMode )
	{ return target->LocateVideoSystemForPlayingFile(pFileName, playMode); }
	virtual VideoResult_t LocatePlayableVideoFile( const char *pSearchFileName, const char *pPathID, VideoSystem_t *pPlaybackSystem, char *pPlaybackFileName, int fileNameMaxLen, VideoSystemFeature_t playMode )
	{ return target->LocatePlayableVideoFile(pSearchFileName, pPathID, pPlaybackSystem, pPlaybackFileName, fileNameMaxLen, playMode); }
	virtual IVideoMaterial *CreateVideoMaterial( const char *pMaterialName, const char *pVideoFileName, const char *pPathID, VideoPlaybackFlags_t playbackFlags,  VideoSystem_t videoSystem, bool PlayAlternateIfNotAvailable )
	{ return target->CreateVideoMaterial(pMaterialName, pVideoFileName, pPathID, playbackFlags, videoSystem, PlayAlternateIfNotAvailable); }
	virtual VideoResult_t DestroyVideoMaterial( IVideoMaterial* pVideoMaterial )
	{ return target->DestroyVideoMaterial(pVideoMaterial); }
	virtual int GetUniqueMaterialID()
	{ return target->GetUniqueMaterialID(); }
	virtual VideoResult_t IsRecordCodecAvailable( VideoSystem_t videoSystem, VideoEncodeCodec_t codec )
	{ return target->IsRecordCodecAvailable(videoSystem, codec); }
	virtual IVideoRecorder *CreateVideoRecorder( VideoSystem_t videoSystem )
	{ return target->CreateVideoRecorder(videoSystem); }
	virtual VideoResult_t DestroyVideoRecorder( IVideoRecorder *pVideoRecorder )
	{ return target->DestroyVideoRecorder(pVideoRecorder); }
	virtual VideoResult_t PlayVideoFileFullScreen( const char *pFileName, const char *pPathID, void *mainWindow, int windowWidth, int windowHeight, int desktopWidth, int desktopHeight, bool windowed, float forcedMinTime,  VideoPlaybackFlags_t playbackFlags,  VideoSystem_t videoSystem, bool PlayAlternateIfNotAvailable )
	{ return target->PlayVideoFileFullScreen(pFileName, pPathID, mainWindow, windowWidth, windowHeight, desktopWidth, desktopHeight, windowed, forcedMinTime, playbackFlags, videoSystem, PlayAlternateIfNotAvailable); }
	virtual VideoResult_t SoundDeviceCommand( VideoSoundDeviceOperation_t operation, void *pDevice, void *pData, VideoSystem_t videoSystem )
	{ return target->SoundDeviceCommand(operation, pDevice, pData, videoSystem); }
	virtual const wchar_t *GetCodecName( VideoEncodeCodec_t nCodec )
	{ return target->GetCodecName(nCodec); }

	static inline int vtable_size = -1;
};
static CVideoServicesRedirect s_VideoRedirect;
#endif

abstract_class CPhysicsRedirect : public IPhysics
{
public:
	static inline IPhysics *source = NULL;
	static inline IPhysics *target = NULL;

	virtual bool Connect( CreateInterfaceFn factory )
	{ return target->Connect(factory); }
	virtual void Disconnect()
	{ target->Disconnect(); }
	virtual void *QueryInterface( const char *pInterfaceName )
	{ return target->QueryInterface(pInterfaceName); }
	virtual InitReturnVal_t Init()
	{ return target->Init(); }
	virtual void Shutdown()
	{ target->Shutdown(); }

	virtual	IPhysicsEnvironment *CreateEnvironment()
	{ return target->CreateEnvironment(); }
	virtual void DestroyEnvironment(IPhysicsEnvironment *pEnv)
	{ return target->DestroyEnvironment(pEnv); }
	virtual IPhysicsEnvironment *GetActiveEnvironmentByIndex(int index)
	{ return target->GetActiveEnvironmentByIndex(index); }

	virtual IPhysicsObjectPairHash *CreateObjectPairHash()
	{ return target->CreateObjectPairHash(); }
	virtual void DestroyObjectPairHash( IPhysicsObjectPairHash *pHash )
	{ return target->DestroyObjectPairHash(pHash); }

	virtual IPhysicsCollisionSet *FindOrCreateCollisionSet( unsigned int id, int maxElementCount )
	{ return target->FindOrCreateCollisionSet(id, maxElementCount); }
	virtual IPhysicsCollisionSet *FindCollisionSet( unsigned int id )
	{ return target->FindCollisionSet(id); }
	virtual void DestroyAllCollisionSets()
	{ return target->DestroyAllCollisionSets(); }

	static inline int vtable_size = -1;
};
static CPhysicsRedirect s_PhysicsRedirect;

#ifndef SWDS
HACKMGR_API void HackMgr_SetEngineVideoServicesPtr(IVideoServices *pOldInter, IVideoServices *pNewInter)
{
	if(!pOldInter || !pNewInter || (pOldInter == pNewInter))
		return;

	CVideoServicesRedirect::source = pOldInter;
	CVideoServicesRedirect::target = pNewInter;

	int vtable_size = CVideoServicesRedirect::vtable_size;
	int vtable_mem_size = (sizeof(generic_plain_mfp_t) * CVideoServicesRedirect::vtable_size);

	generic_vtable_t source_vtable = NULL;
	generic_vtable_t target_vtable = NULL;

#if defined __GNUC__ && defined __linux__
	source_vtable = vtable_from_object(CVideoServicesRedirect::source);

	page_info page_access = page_info(source_vtable, vtable_mem_size);
	page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

	target_vtable = vtable_from_object(&s_VideoRedirect);

	page_access = page_info(target_vtable, vtable_mem_size);
	page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
#else
	Assert(0);
	DebuggerBreak();
#endif

	for(int i = 0; i < vtable_size; ++i) {
		source_vtable[i] = target_vtable[i];
	}
}

HACKMGR_API bool HackMgr_IsSafeToSwapVideoServices()
{
	return false;
}
#endif

HACKMGR_API void HackMgr_SetEnginePhysicsPtr(IPhysics *pOldInter, IPhysics *pNewInter)
{
	if(!pOldInter || !pNewInter || (pOldInter == pNewInter))
		return;

	CPhysicsRedirect::source = pOldInter;
	CPhysicsRedirect::target = pNewInter;

	int vtable_size = CPhysicsRedirect::vtable_size;
	int vtable_mem_size = (sizeof(generic_plain_mfp_t) * CPhysicsRedirect::vtable_size);

	generic_vtable_t source_vtable = NULL;
	generic_vtable_t target_vtable = NULL;

#if defined __GNUC__ && defined __linux__
	source_vtable = vtable_from_object(CPhysicsRedirect::source);

	page_info page_access = page_info(source_vtable, vtable_mem_size);
	page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

	target_vtable = vtable_from_object(&s_PhysicsRedirect);

	page_access = page_info(target_vtable, vtable_mem_size);
	page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
#else
	Assert(0);
	DebuggerBreak();
#endif

	for(int i = 0; i < vtable_size; ++i) {
		source_vtable[i] = target_vtable[i];
	}
}

HACKMGR_API bool HackMgr_IsSafeToSwapPhysics()
{
	return false;
}

struct app_sys_pair_t
{
	IAppSystem *pOldInter;
	IAppSystem *pNewInter;
};

static app_sys_pair_t reconnect_interface(CAppSystemGroup *ParentAppSystemGroup, CreateInterfaceFn pOldFactory, CreateInterfaceFn pNewFactory, const char *name)
{
	app_sys_pair_t ret;

	int status = IFACE_OK;
	IAppSystem *pOldInter = pOldFactory ? (IAppSystem *)pOldFactory(name, &status) : NULL;
	if(pOldInter && status == IFACE_OK) {
		pOldInter->Shutdown();
		pOldInter->Disconnect();
	}

	status = IFACE_OK;
	IAppSystem *pNewInter = pNewFactory ? (IAppSystem *)pNewFactory(name, &status) : NULL;
	if(status != IFACE_OK) {
		pNewInter = NULL;
	}

	int sys_idx = pOldInter ? ParentAppSystemGroup->m_Systems.Find(pOldInter) : ParentAppSystemGroup->m_Systems.InvalidIndex();
	if(sys_idx != ParentAppSystemGroup->m_Systems.InvalidIndex()) {
		if(pNewInter) {
			ParentAppSystemGroup->m_Systems[sys_idx] = pNewInter;
		} else {
			ParentAppSystemGroup->m_Systems.Remove(sys_idx);
		}
	} else {
		sys_idx = ParentAppSystemGroup->m_Systems.AddToTail(pNewInter);
	}

	int map_idx = ParentAppSystemGroup->m_SystemDict.Find(name);
	if(map_idx != ParentAppSystemGroup->m_SystemDict.InvalidIndex()) {
		if(sys_idx != ParentAppSystemGroup->m_Systems.InvalidIndex()) {
			ParentAppSystemGroup->m_SystemDict[map_idx] = sys_idx;
		} else {
			ParentAppSystemGroup->m_SystemDict.RemoveAt(map_idx);
		}
	} else {
		if(sys_idx != ParentAppSystemGroup->m_Systems.InvalidIndex()) {
			ParentAppSystemGroup->m_SystemDict.Insert(name, sys_idx);
		}
	}

	ret.pOldInter = pOldInter;
	ret.pNewInter = pNewInter;

	return ret;
}

CAppSystemGroup *GetLauncherAppSystem()
{
	if( s_pLauncherAppSystem )
		return s_pLauncherAppSystem;

	int status = IFACE_OK;
#ifndef SWDS
	if(!IsDedicatedServer()) {
		void *pEngineAPI = GetEngineInterfaceFactory()("VENGINE_LAUNCHER_API_VERSION004", &status);
		if(!pEngineAPI || status != IFACE_OK)
			return NULL;

		void *StartupInfo = ((unsigned char *)pEngineAPI + CEngineAPI_m_StartupInfo_offset);

		s_pLauncherAppSystem = *(CAppSystemGroup **)((unsigned char *)StartupInfo + StartupInfo_t_m_pParentAppSystemGroup_offset);
	} else 
#endif
	{
		void *pDedicatedAPI = GetEngineInterfaceFactory()(VENGINE_HLDS_API_VERSION, &status);
		if(!pDedicatedAPI || status != IFACE_OK)
			return NULL;

		s_pLauncherAppSystem = *(CAppSystemGroup **)((unsigned char *)pDedicatedAPI + CDedicatedServerAPI_m_pDedicatedServer_offset);
	}

	return s_pLauncherAppSystem;
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(1)

HackMgr_InitCommandLine();

#if defined __GNUC__ && defined __linux__
#ifndef SWDS
CVideoServicesRedirect::vtable_size = vfunc_index(&IVideoServices::GetCodecName) + 1;
#endif
CPhysicsRedirect::vtable_size = vfunc_index(&IPhysics::DestroyAllCollisionSets) + 1;
#endif

if(!GetFilesystemInterfaceFactory()) {
	return;
}

int status = IFACE_OK;
IFileSystem *pFileSystem = (IFileSystem *)GetFilesystemInterfaceFactory()(FILESYSTEM_INTERFACE_VERSION, &status);
if(!pFileSystem || status != IFACE_OK) {
	return;
}

char gamebin_path[MAX_PATH];
pFileSystem->GetSearchPath("GAMEBIN", false, gamebin_path, ARRAYSIZE(gamebin_path));
char *comma = V_strstr(gamebin_path,";");
if(comma) {
	*comma = '\0';
}
V_StripTrailingSlash(gamebin_path);
int gamebin_len = V_strlen(gamebin_path);

#ifndef SWDS
if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated") && !CommandLine()->HasParm("-textmode")) {
	V_strncat(gamebin_path, CORRECT_PATH_SEPARATOR_S, sizeof(gamebin_path));
	++gamebin_len;

	V_strncat(gamebin_path, "tools", sizeof(gamebin_path));

	pFileSystem->AddSearchPath(gamebin_path, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);
	pFileSystem->AddSearchPath(gamebin_path, "GAMEBIN", PATH_ADD_TO_TAIL);

	gamebin_path[gamebin_len-1] = '\0';

	pFileSystem->AddSearchPath(gamebin_path, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);

	gamebin_path[gamebin_len-1] = CORRECT_PATH_SEPARATOR;
	gamebin_path[gamebin_len] = '\0';
} else
#endif
{
	pFileSystem->AddSearchPath(gamebin_path, "EXECUTABLE_PATH", PATH_ADD_TO_HEAD);

	V_strncat(gamebin_path, CORRECT_PATH_SEPARATOR_S, sizeof(gamebin_path));
	++gamebin_len;
}

CAppSystemGroup *ParentAppSystemGroup = GetLauncherAppSystem();
if( !ParentAppSystemGroup )
	return;

s_pCurrentAppSystemHack = ParentAppSystemGroup;

struct connect_info_t
{
	connect_info_t(connect_info_t &&) = default;
	connect_info_t &operator=(connect_info_t &&) = default;
	connect_info_t(const connect_info_t &) = delete;
	connect_info_t &operator=(const connect_info_t &) = delete;

	connect_info_t() = default;

	connect_info_t(CAppSystemGroup *group_)
		: group(group_)
	{
	}

	CAppSystemGroup *group;
	CUtlVector<IAppSystem *> apps;
};

CUtlVector<connect_info_t> connect_later;

int connect_idx = connect_later.AddToTail();
connect_later[connect_idx].group = ParentAppSystemGroup;

const char *matchmaking_dll_name = NULL;
#ifndef SWDS
if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated")) {
	matchmaking_dll_name = "matchmaking" DLL_EXT_STRING;
} else 
#endif
{
	matchmaking_dll_name = "matchmaking_ds" DLL_EXT_STRING;
}

V_strncat(gamebin_path, matchmaking_dll_name, sizeof(gamebin_path));

CSysModule *pTmpMod = Sys_LoadModule(gamebin_path);
if(pTmpMod) {
	CreateInterfaceFn pTmpFac = Sys_GetFactory(pTmpMod);
	if(pTmpFac) {
		status = IFACE_OK;
		IMatchFramework *pTmpInter = (IMatchFramework *)pTmpFac(IMATCHFRAMEWORK_VERSION_STRING, &status);
		if(status != IFACE_OK)
			pTmpInter = NULL;

		if(pTmpInter) {
			CAppSystemGroup_AddSystem(ParentAppSystemGroup, pTmpInter, IMATCHFRAMEWORK_VERSION_STRING);
			connect_later[connect_idx].apps.AddToTail(pTmpInter);
		}
	}
}

gamebin_path[gamebin_len] = '\0';

#ifndef SWDS
if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated")) {
	V_strncat(gamebin_path, "game_loopback" DLL_EXT_STRING, sizeof(gamebin_path));

	pTmpMod = Sys_LoadModule(gamebin_path);
	if(pTmpMod) {
		CreateInterfaceFn pTmpFac = Sys_GetFactory(pTmpMod);
		if(pTmpFac) {
			status = IFACE_OK;
			IGameLoopback *pTmpInter = (IGameLoopback *)pTmpFac(GAMELOOPBACK_INTERFACE_VERSION, &status);
			if(status != IFACE_OK)
				pTmpInter = NULL;

			if(pTmpInter) {
				CAppSystemGroup_AddSystem(ParentAppSystemGroup, pTmpInter, GAMELOOPBACK_INTERFACE_VERSION);
				connect_later[connect_idx].apps.AddToTail(pTmpInter);
			}
		}
	}

	gamebin_path[gamebin_len] = '\0';
}
#endif

#ifndef SWDS
if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated") && !CommandLine()->HasParm("-textmode")) {
	if(CommandLine()->HasParm("-edit") ||
	#ifdef __linux__
		CommandLine()->HasParm("-edit_linux") ||
	#endif
		CommandLine()->HasParm("-foundrymode")) {
		V_strncat(gamebin_path, "hammer_dll" DLL_EXT_STRING, sizeof(gamebin_path));

		CSysModule *pHammerMod = Sys_LoadModule(gamebin_path);
		if(pHammerMod) {
			CreateInterfaceFn pHammerFac = Sys_GetFactory(pHammerMod);
			if(pHammerFac) {
				status = IFACE_OK;
				IHammer *pHammerInter = (IHammer *)pHammerFac(INTERFACEVERSION_HAMMER, &status);
				if(status != IFACE_OK)
					pHammerInter = NULL;

				if(pHammerInter) {
					CAppSystemGroup_AddSystem(ParentAppSystemGroup, pHammerInter, INTERFACEVERSION_HAMMER);
					connect_later[connect_idx].apps.AddToTail(pHammerInter);
				}
			}
		}

		gamebin_path[gamebin_len] = '\0';
	}
}
#endif

const char *vphysics_dll_name = NULL;
if(!IsDedicatedServer()) {
	vphysics_dll_name = "vphysics" DLL_EXT_STRING;
} else {
	vphysics_dll_name = "vphysics_srv" DLL_EXT_STRING;
}

char *szModuleName=new char[MAX_PATH];

do {
	for(int i = 0; i < ParentAppSystemGroup->m_Modules.Count(); ++i) {
		CAppSystemGroup::Module_t &old_mod = ParentAppSystemGroup->m_Modules[i];
		if(!old_mod.m_pModuleName)
			continue;

		const char *pFileName = V_GetFileName(old_mod.m_pModuleName);
		if(!pFileName)
			continue;

		V_strcpy(szModuleName, pFileName);
		const char *pExt = V_GetFileExtension(pFileName);
		if(!pExt || *pExt == '\0')
			V_strcat(szModuleName, DLL_EXT_STRING, MAX_PATH);

		if(pFileSystem->FileExists(szModuleName, "GAMEBIN")) {

			CAppSystemGroup::Module_t new_mod;

			new_mod.m_pModuleName = szModuleName;
			szModuleName = new char[MAX_PATH];

			new_mod.m_pModule = pFileSystem->LoadModule(new_mod.m_pModuleName, "GAMEBIN", false);
			if(new_mod.m_pModule)
				new_mod.m_Factory = Sys_GetFactory(new_mod.m_pModule);
			else
				new_mod.m_Factory = NULL;

			CreateInterfaceFn pOldFactory = old_mod.m_Factory;
			if(!pOldFactory && old_mod.m_pModule)
				pOldFactory = Sys_GetFactory(old_mod.m_pModule);

			if(V_stricmp(new_mod.m_pModuleName, vphysics_dll_name) == 0) {
				app_sys_pair_t pair;
				pair = reconnect_interface(ParentAppSystemGroup, pOldFactory, new_mod.m_Factory, VPHYSICS_INTERFACE_VERSION);
				HackMgr_SetEnginePhysicsPtr((IPhysics *)pair.pOldInter, (IPhysics *)pair.pNewInter);
				if(pair.pNewInter)
					connect_later[connect_idx].apps.AddToTail(pair.pNewInter);
			}
		#ifndef SWDS
			else if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated") && !CommandLine()->HasParm("-textmode")) {
				if(V_stricmp(new_mod.m_pModuleName, "video_services" DLL_EXT_STRING) == 0) {
					app_sys_pair_t pair;
					pair = reconnect_interface(ParentAppSystemGroup, pOldFactory, new_mod.m_Factory, VIDEO_SERVICES_INTERFACE_VERSION);
					HackMgr_SetEngineVideoServicesPtr((IVideoServices *)pair.pOldInter, (IVideoServices *)pair.pNewInter);
					if(pair.pNewInter)
						connect_later[connect_idx].apps.AddToTail(pair.pNewInter);
				}
				else if(V_stricmp(new_mod.m_pModuleName, "hammer_dll" DLL_EXT_STRING) == 0) {
					app_sys_pair_t pair;
					pair = reconnect_interface(ParentAppSystemGroup, pOldFactory, new_mod.m_Factory, INTERFACEVERSION_HAMMER);
					if(pair.pNewInter)
						connect_later[connect_idx].apps.AddToTail(pair.pNewInter);
				}
			}
		#endif

		#ifdef __linux__
			Dl_info mod_dl_info;
			if(dladdr((const void *)pOldFactory, &mod_dl_info) != 0) {
				Dl_info sys_dl_info;
				for(int i = 0; i < ParentAppSystemGroup->m_Systems.Count();) {
					if(dladdr((const void *)ParentAppSystemGroup->m_Systems[i], &sys_dl_info) == 0) {
						++i;
						continue;
					}

					if(mod_dl_info.dli_fbase == sys_dl_info.dli_fbase) {
						ParentAppSystemGroup->m_Systems[i]->Shutdown();
						ParentAppSystemGroup->m_Systems[i]->Disconnect();

						bool remove = true;
						bool added = false;

						for(int j = 0; j < ParentAppSystemGroup->m_SystemDict.Count();) {
							if(ParentAppSystemGroup->m_SystemDict[j] == i) {
								if(!added && new_mod.m_Factory) {
									status = IFACE_OK;
									IAppSystem *pNewInter = (IAppSystem *)new_mod.m_Factory(ParentAppSystemGroup->m_SystemDict.GetElementName(j), &status);
									if(!pNewInter || status != IFACE_OK) {
										ParentAppSystemGroup->m_SystemDict.RemoveAt(j);
										continue;
									} else {
										ParentAppSystemGroup->m_Systems[i] = pNewInter;
										connect_later[connect_idx].apps.AddToTail(pNewInter);
										++i;
										++j;
										remove = false;
										added = true;
									}
								} else {
									ParentAppSystemGroup->m_SystemDict.RemoveAt(j);
									continue;
								}
							} else {
								++j;
							}
						}

						if(remove) {
							ParentAppSystemGroup->m_Systems.Remove(i);
						}
					} else {
						++i;
					}
				}
			}
		#endif

			if(old_mod.m_pModule)
				Sys_UnloadModule(old_mod.m_pModule);

			old_mod = new_mod;
		}
	}

	ParentAppSystemGroup = ParentAppSystemGroup->m_pParentAppSystem;
	s_pCurrentAppSystemHack = ParentAppSystemGroup;

	if(ParentAppSystemGroup) {
		connect_idx = connect_later.AddToTail();
		connect_later[connect_idx].group = ParentAppSystemGroup;
	}

} while(ParentAppSystemGroup);

delete[] szModuleName;

//TODO!!!!! are the appsystems actually already initalized at this point?
FOR_EACH_VEC(connect_later, i)
{
	s_pCurrentAppSystemHack = connect_later[i].group;

	FOR_EACH_VEC(connect_later[i].apps, j)
	{
		if(connect_later[i].apps[j]->Connect(AppSystemCreateInterfaceFnHack)) {
			if(connect_later[i].apps[j]->Init() != INIT_OK) {
				connect_later[i].apps[j]->Shutdown();
			}
		} else {
			connect_later[i].apps[j]->Disconnect();
		}
	}
}

HACKMGR_EXECUTE_ON_LOAD_END
