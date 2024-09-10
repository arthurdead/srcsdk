#include "hackmgr/shadersystem.h"
#include "createinterface.h"
#include "hackmgr_internal.h"
#include "IShaderSystem.h"
#include "shaderapi/ishaderapi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static constexpr int CShaderSystem_m_ShaderDLLs_offset = (
	sizeof(generic_vtable_t)
);

static constexpr int CShaderSystem_ShaderDLLInfo_t_m_pShaderDLL_offset = (
	sizeof(char *) +
	sizeof(CSysModule *)
);

static constexpr int CShaderSystem_ShaderDLLInfo_t_m_bModShaderDLL_offset = (
	CShaderSystem_ShaderDLLInfo_t_m_pShaderDLL_offset +
	sizeof(IShaderDLLInternal *) +
	sizeof(ShaderDLL_t)
);

static constexpr int CShaderSystem_ShaderDLLInfo_t_size = (
	CShaderSystem_ShaderDLLInfo_t_m_bModShaderDLL_offset +
	sizeof(bool) +
	3 +
	sizeof(CUtlDict<IShader *, unsigned short>)
);

struct alignas(1) ShaderDLLInfo_t
{
	char pad[CShaderSystem_ShaderDLLInfo_t_size];
};

HACKMGR_API void HackMgr_ToggleShaderDLLAsMod(IShaderDLLInternal *pModDLL, bool value)
{
	if(!GetMaterialSystemInterfaceFactory())
		return;

	int status = IFACE_OK;
	IShaderSystem *pShadsys = (IShaderSystem *)GetMaterialSystemInterfaceFactory()("", &status);
	if(!pShadsys || status != IFACE_OK) {
		return;
	}

	CUtlVector<ShaderDLLInfo_t> &ShaderDLLs = *(CUtlVector<ShaderDLLInfo_t> *)(((unsigned char *)pShadsys) + CShaderSystem_m_ShaderDLLs_offset);

	unsigned char *pMem = (unsigned char *)ShaderDLLs.Base();
	for(int i = 0; i < ShaderDLLs.Count(); ++i) {
		IShaderDLLInternal *pDLL = *(IShaderDLLInternal **)(((unsigned char *)pMem) + CShaderSystem_ShaderDLLInfo_t_m_pShaderDLL_offset);

		if(pDLL == pModDLL) {
			*(bool *)(((unsigned char *)pMem) + CShaderSystem_ShaderDLLInfo_t_m_bModShaderDLL_offset) = value;;

			break;
		}

		pMem += CShaderSystem_ShaderDLLInfo_t_size;
	}
}
