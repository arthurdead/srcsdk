#include "hackmgr/hackmgr.h"
#include "engine/ishadowmgr.h"
#include "materialsystem/imaterialsystem.h"
#include "createinterface.h"
#include "hackmgr_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const unsigned char INVALID_FLASHLIGHT_MOD_IDX = (unsigned char)-1;

HACKMGR_INIT_PRIO(101) static CUtlLinkedList< FlashlightStateMod_t, unsigned char > s_FlashlightMods;

static constexpr int CShadowMgr_Shadow_t_m_Flags_offset = (
	sizeof(ShadowInfo_t) +
	sizeof(Vector) +
	sizeof(IMaterial*) +
	sizeof(IMaterial*) +
	sizeof(void*)
);

static constexpr int CShadowMgr_SHADOW_DISABLED_value = (SHADOW_LAST_CREATION_FLAG << 1);

static int IShadowMgr_CreateShadow_index = -1;
static int IShadowMgr_CreateShadowEx_index = -1;
static int IShadowMgr_DestroyShadow_index = -1;

using IShadowMgr_CreateShadowEx_t = ShadowHandle_t(IShadowMgr::*)(IMaterial*, IMaterial*, void*, ShadowCreateFlags_t);

static decltype(&IShadowMgr::CreateShadow) IShadowMgr_CreateShadow_ptr = NULL;
static IShadowMgr_CreateShadowEx_t IShadowMgr_CreateShadowEx_ptr = NULL;
static decltype(&IShadowMgr::DestroyShadow) IShadowMgr_DestroyShadow_ptr = NULL;

HACKMGR_CLASS_API int IShadowMgr::SetupFlashlightRenderInstanceInfo( ShadowHandle_t *pUniqueFlashlights, uint32 *pModelUsageMask, int nUsageStride, int nInstanceCount, const ModelInstanceHandle_t *pInstance )
{
	Assert(0);
	return 0;
}

HACKMGR_CLASS_API void IShadowMgr::GetFlashlightRenderInfo( FlashlightInstance_t *pFlashlightState, int nCount, const ShadowHandle_t *pHandles )
{
	Assert(0);
}

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

	if(shadowHandle != SHADOW_HANDLE_INVALID) {
		FlashlightState_t &state = const_cast<FlashlightState_t &>(GetFlashlightState( shadowHandle ));

		if(state.m_Reserved == INVALID_FLASHLIGHT_MOD_IDX || !s_FlashlightMods.IsValidIndex( state.m_Reserved )) {
			state.m_Reserved = s_FlashlightMods.AddToTail();
		}

		s_FlashlightMods[ state.m_Reserved ] = static_cast<const FlashlightStateMod_t &>(lightState);
	}
}

HACKMGR_CLASS_API const FlashlightStateMod_t &IShadowMgr::GetFlashlightStateMod( ShadowHandle_t handle )
{
	if(handle != SHADOW_HANDLE_INVALID) {
		FlashlightState_t &state = const_cast<FlashlightState_t &>(GetFlashlightState(handle));

		if(state.m_Reserved == INVALID_FLASHLIGHT_MOD_IDX || !s_FlashlightMods.IsValidIndex( state.m_Reserved )) {
			state.m_Reserved = s_FlashlightMods.AddToTail();
		}

		return s_FlashlightMods[ state.m_Reserved ];
	}

	Assert(0);
	static FlashlightStateMod_t tmp;
	return tmp;
}

static ShadowHandle_t IShadowMgr_CreateShadow_hook( IShadowMgr *pthis, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, ShadowCreateFlags_t creationFlags )
{
	ShadowHandle_t handle = (pthis->*IShadowMgr_CreateShadow_ptr)(pMaterial, pModelMaterial, pBindProxy, creationFlags);
	if(handle != SHADOW_HANDLE_INVALID) {
		const unsigned char *pInfo = (const unsigned char *)&pthis->GetInfo( handle );
		const unsigned short flags = *(const unsigned short *)(pInfo + CShadowMgr_Shadow_t_m_Flags_offset);

		if(((ShadowCreateFlags_t)flags & SHADOW_CREATE_FLASHLIGHT) != SHADOW_NO_CREATION_FLAGS) {
			FlashlightState_t &state = const_cast<FlashlightState_t &>(pthis->GetFlashlightState(handle));
			if(state.m_Reserved == INVALID_FLASHLIGHT_MOD_IDX || !s_FlashlightMods.IsValidIndex( state.m_Reserved )) {
				state.m_Reserved = s_FlashlightMods.AddToTail();
			}
		}
	}
	return handle;
}

static ShadowHandle_t IShadowMgr_CreateShadowEx_hook( IShadowMgr *pthis, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, ShadowCreateFlags_t creationFlags )
{
	ShadowHandle_t handle = (pthis->*IShadowMgr_CreateShadowEx_ptr)(pMaterial, pModelMaterial, pBindProxy, creationFlags);
	if(handle != SHADOW_HANDLE_INVALID) {
		const unsigned char *pInfo = (const unsigned char *)&pthis->GetInfo( handle );
		const unsigned short flags = *(const unsigned short *)(pInfo + CShadowMgr_Shadow_t_m_Flags_offset);

		if(((ShadowCreateFlags_t)flags & SHADOW_CREATE_FLASHLIGHT) != SHADOW_NO_CREATION_FLAGS) {
			FlashlightState_t &state = const_cast<FlashlightState_t &>(pthis->GetFlashlightState(handle));
			if(state.m_Reserved == INVALID_FLASHLIGHT_MOD_IDX || !s_FlashlightMods.IsValidIndex( state.m_Reserved )) {
				state.m_Reserved = s_FlashlightMods.AddToTail();
			}
		}
	}
	return handle;
}

static void IShadowMgr_DestroyShadow_hook( IShadowMgr *pthis, ShadowHandle_t handle )
{
	if(handle != SHADOW_HANDLE_INVALID) {
		const unsigned char *pInfo = (const unsigned char *)&pthis->GetInfo( handle );
		const unsigned short flags = *(const unsigned short *)(pInfo + CShadowMgr_Shadow_t_m_Flags_offset);

		if(((ShadowCreateFlags_t)flags & SHADOW_CREATE_FLASHLIGHT) != SHADOW_NO_CREATION_FLAGS) {
			FlashlightState_t &state = const_cast<FlashlightState_t &>(pthis->GetFlashlightState(handle));
			if(state.m_Reserved != INVALID_FLASHLIGHT_MOD_IDX && s_FlashlightMods.IsValidIndex( state.m_Reserved )) {
				s_FlashlightMods.Remove( state.m_Reserved );
			}

			state.m_Reserved = INVALID_FLASHLIGHT_MOD_IDX;
		}
	}

	(pthis->*IShadowMgr_DestroyShadow_ptr)(handle);
}

HACKMGR_API FlashlightStateMod_t &HackMgr_GetFlashlightMod(FlashlightState_t &flashlightState)
{
	if(flashlightState.m_Reserved == INVALID_FLASHLIGHT_MOD_IDX || !s_FlashlightMods.IsValidIndex( flashlightState.m_Reserved )) {
		flashlightState.m_Reserved = s_FlashlightMods.AddToTail();
	}

	return s_FlashlightMods[ flashlightState.m_Reserved ];
}

HACKMGR_API void HackMgr_RemoveFlashlightMod(FlashlightState_t &flashlightState)
{
	if(flashlightState.m_Reserved != INVALID_FLASHLIGHT_MOD_IDX && s_FlashlightMods.IsValidIndex( flashlightState.m_Reserved )) {
		s_FlashlightMods.Remove( flashlightState.m_Reserved );
	}

	flashlightState.m_Reserved = INVALID_FLASHLIGHT_MOD_IDX;
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(101)

IShadowMgr_CreateShadow_index = hackmgr::compiler_native::vfunc_index_single(&IShadowMgr::CreateShadow);
IShadowMgr_CreateShadowEx_index = hackmgr::compiler_native::vfunc_index_single(static_cast<IShadowMgr_CreateShadowEx_t>(&IShadowMgr::CreateShadowEx));
IShadowMgr_DestroyShadow_index = hackmgr::compiler_native::vfunc_index_single(&IShadowMgr::DestroyShadow);

if(!GetEngineInterfaceFactory())
	return;

int status = IFACE_FAILED;
IShadowMgr *mgr = (IShadowMgr *)GetEngineInterfaceFactory()(ENGINE_SHADOWMGR_INTERFACE_VERSION, &status);
if(!mgr || status != IFACE_OK)
	return;

generic_vtable_t vtable = hackmgr::target_runtime::vtable_from_object(mgr);

page_info page_access(vtable, (sizeof(generic_plain_mfp_t) * IShadowMgr_CreateShadowEx_index));
#ifdef __linux__
page_access.protect(PROT_READ|PROT_WRITE|PROT_EXEC);
#else
page_access.protect(PAGE_EXECUTE_READWRITE);
#endif

IShadowMgr_CreateShadow_ptr = hackmgr::target_runtime::func_from_single_vtable<decltype(IShadowMgr_CreateShadow_ptr)>(vtable[IShadowMgr_CreateShadow_index]);
IShadowMgr_CreateShadowEx_ptr = hackmgr::target_runtime::func_from_single_vtable<decltype(IShadowMgr_CreateShadowEx_ptr)>(vtable[IShadowMgr_CreateShadowEx_index]);
IShadowMgr_DestroyShadow_ptr = hackmgr::target_runtime::func_from_single_vtable<decltype(IShadowMgr_DestroyShadow_ptr)>(vtable[IShadowMgr_DestroyShadow_index]);

vtable[IShadowMgr_CreateShadow_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_CreateShadow_hook);
vtable[IShadowMgr_CreateShadowEx_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_CreateShadowEx_hook);
vtable[IShadowMgr_DestroyShadow_index] = reinterpret_cast<generic_plain_mfp_t>(IShadowMgr_DestroyShadow_hook);

HACKMGR_EXECUTE_ON_LOAD_END