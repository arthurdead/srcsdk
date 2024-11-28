#include "gamepadui/igamepadui.h"
#include "tier1/interface.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGamepadUIRedirect : public IGamepadUI
{
#ifdef __MINGW32__
private:
	void __DTOR__()
	{
		this->~CGamepadUIRedirect();
	}
#endif

public:
	void Initialize( CreateInterfaceFn factory ) override
	{ m_pTarget->Initialize( factory ); }
	void Shutdown() override
	{ m_pTarget->Shutdown(); }

	void OnUpdate( float flFrametime ) override
	{ m_pTarget->OnUpdate( flFrametime ); }
	void OnLevelInitializePreEntity() override
	{ m_pTarget->OnLevelInitializePreEntity(); }
	void OnLevelInitializePostEntity() override
	{ m_pTarget->OnLevelInitializePostEntity(); }
	void OnLevelShutdown() override
	{ m_pTarget->OnLevelShutdown(); }

	void VidInit() override
	{ m_pTarget->VidInit(); }

	bool GetRedirectTarget();

 private:
	IGamepadUI *m_pTarget;
};

bool CGamepadUIRedirect::GetRedirectTarget()
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
	m_pTarget = (IGamepadUI *)pFactory(GAMEPADUI_INTERFACE_VERSION, &status);
	if(!m_pTarget || status != IFACE_OK) {
		return false;
	}

	return m_pTarget != NULL;
}

void *CreateGamepadUI()
{
	static CGamepadUIRedirect *pGamepadUI = NULL;
	if(!pGamepadUI) {
		pGamepadUI = new CGamepadUIRedirect;
		if(!pGamepadUI->GetRedirectTarget()) {
			delete pGamepadUI;
			return NULL;
		}
	}
	return pGamepadUI;
}

EXPOSE_INTERFACE_FN(CreateGamepadUI, IGamepadUI, GAMEPADUI_INTERFACE_VERSION);
