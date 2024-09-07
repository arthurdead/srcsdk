#include "hackmgr/hackmgr.h"
#include "engine/ishadowmgr.h"
#include "materialsystem/imaterialsystem.h"
#include "engine_interface.h"
#include "hackmgr_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static constexpr int CShadowMgr_Shadow_t_m_Flags_offset = (
	sizeof(ShadowInfo_t) +
	sizeof(Vector) +
	sizeof(IMaterial*) +
	sizeof(IMaterial*) +
	sizeof(void*)
);

static constexpr int CShadowMgr_SHADOW_DISABLED_value = (SHADOW_LAST_FLAG << 1);

static int IShadowMgr_CreateShadow_index = -1;
static int IShadowMgr_CreateShadowEx_index = -1;
static int IShadowMgr_DestroyShadow_index = -1;

static decltype(&IShadowMgr::CreateShadow) IShadowMgr_CreateShadow_ptr = NULL;
static decltype(&IShadowMgr::CreateShadowEx) IShadowMgr_CreateShadowEx_ptr = NULL;
static decltype(&IShadowMgr::DestroyShadow) IShadowMgr_DestroyShadow_ptr = NULL;

HACKMGR_CLASS_API void IShadowMgr::RemoveAllDecalsFromShadow( ShadowHandle_t handle )
{
	const unsigned char *pInfo = (const unsigned char *)&GetInfo( handle );

	const unsigned short flags = *(const unsigned short *)(pInfo + CShadowMgr_Shadow_t_m_Flags_offset);

	bool wasEnabled = ((flags & CShadowMgr_SHADOW_DISABLED_value) == 0);
	EnableShadow( handle, false );
	if(wasEnabled)
		EnableShadow( handle, true );
}

HACKMGR_CLASS_API void IShadowMgr::UpdateFlashlightState( ShadowHandle_t shadowHandle, const FlashlightStateEx_t &lightState )
{
	UpdateFlashlightState( shadowHandle, static_cast<const FlashlightState_t &>(lightState) );
}

HACKMGR_CLASS_API const FlashlightStateMod_t &IShadowMgr::GetFlashlightStateMod( ShadowHandle_t handle )
{
	static FlashlightStateMod_t tmp;
	return tmp;
}

static ShadowHandle_t HACKMGR_THISCALL IShadowMgr_CreateShadow_hook( IShadowMgr *pthis, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, int creationFlags )
{
	ShadowHandle_t handle = (pthis->*IShadowMgr_CreateShadow_ptr)(pMaterial, pModelMaterial, pBindProxy, creationFlags);
	if(handle != SHADOW_HANDLE_INVALID) {

	}
	return handle;
}

static ShadowHandle_t HACKMGR_THISCALL IShadowMgr_CreateShadowEx_hook( IShadowMgr *pthis, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, int creationFlags )
{
	ShadowHandle_t handle = (pthis->*IShadowMgr_CreateShadowEx_ptr)(pMaterial, pModelMaterial, pBindProxy, creationFlags);
	if(handle != SHADOW_HANDLE_INVALID) {
		
	}
	return handle;
}

static void HACKMGR_THISCALL IShadowMgr_DestroyShadow_hook( IShadowMgr *pthis, ShadowHandle_t handle )
{
	(pthis->*IShadowMgr_DestroyShadow_ptr)(handle);
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(101)

IShadowMgr_CreateShadow_index = vfunc_index(&IShadowMgr::CreateShadow);
IShadowMgr_CreateShadowEx_index = vfunc_index(&IShadowMgr::CreateShadowEx);
IShadowMgr_DestroyShadow_index = vfunc_index(&IShadowMgr::DestroyShadow);

int status = IFACE_OK;
IShadowMgr *mgr = (IShadowMgr *)GetEngineInterface()(ENGINE_SHADOWMGR_INTERFACE_VERSION, &status);
if(!mgr || status != IFACE_OK)
	return;

#ifdef __linux__
generic_vtable_t vtable = vtable_from_object(mgr);

page_info page_access(vtable, (sizeof(generic_plain_mfp_t) * IShadowMgr_CreateShadowEx_index));
page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

IShadowMgr_CreateShadow_ptr = func_from_vtable<decltype(IShadowMgr_CreateShadow_ptr)>(vtable[IShadowMgr_CreateShadow_index]);
IShadowMgr_CreateShadowEx_ptr = func_from_vtable<decltype(IShadowMgr_CreateShadowEx_ptr)>(vtable[IShadowMgr_CreateShadowEx_index]);
IShadowMgr_DestroyShadow_ptr = func_from_vtable<decltype(IShadowMgr_DestroyShadow_ptr)>(vtable[IShadowMgr_DestroyShadow_index]);

//vtable[IShadowMgr_CreateShadow_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_CreateShadow_hook);
//vtable[IShadowMgr_CreateShadowEx_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_CreateShadowEx_hook);
//vtable[IShadowMgr_DestroyShadow_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_DestroyShadow_hook);
#endif

HACKMGR_EXECUTE_ON_LOAD_END