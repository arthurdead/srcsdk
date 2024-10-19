#include "tier0/dbg.h"
#include "tier1/interface.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"
#include "vaudio/ivaudio.h"
#include "module_name.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef DLLNAME
	#error
#endif

class CVAudioRedirect : public IVAudio
{
public:
	CVAudioRedirect();
	~CVAudioRedirect();

	bool GetRedirectTarget();

	IAudioStream *CreateMP3StreamDecoder(IAudioStreamEvent *pEventHandler) override;
	void DestroyMP3StreamDecoder(IAudioStream *pDecoder) override;

private:
	IVAudio *m_pTarget;
};

CVAudioRedirect::CVAudioRedirect()
	: m_pTarget(NULL)
{
}

CVAudioRedirect::~CVAudioRedirect()
{
	if(m_pTarget) {
		delete m_pTarget;
	}
}

bool CVAudioRedirect::GetRedirectTarget()
{
	CSysModule *pTargetMod = NULL;

	const char *pGameDir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );

	char szTargetPath[MAX_PATH];
	V_strncpy(szTargetPath, pGameDir, sizeof(szTargetPath));
	V_AppendSlash(szTargetPath, sizeof(szTargetPath));
	V_strcat(szTargetPath, "bin" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));

	if(strcmp(V_STRINGIFY(DLLNAME), "vaudio_miles") == 0) {
		V_strcat(szTargetPath, "vaudio_minimp3" DLL_EXT_STRING, sizeof(szTargetPath));
	} else {
		return false;
	}

	pTargetMod = Sys_LoadModule(szTargetPath);
	if(!pTargetMod) {
		return false;
	}

	CreateInterfaceFn pFactory = Sys_GetFactory(pTargetMod);
	if(!pFactory) {
		return false;
	}

	int status = IFACE_OK;
	m_pTarget = (IVAudio *)pFactory(VAUDIO_INTERFACE_VERSION, &status);
	if(!m_pTarget || status != IFACE_OK) {
		return false;
	}

	return m_pTarget != NULL;
}

IAudioStream *CVAudioRedirect::CreateMP3StreamDecoder(IAudioStreamEvent *pEventHandler)
{
	return m_pTarget->CreateMP3StreamDecoder(pEventHandler);
}

void CVAudioRedirect::DestroyMP3StreamDecoder(IAudioStream *pDecoder)
{
	m_pTarget->DestroyMP3StreamDecoder(pDecoder);
}

void *CreateAudioRedirect()
{
	static CVAudioRedirect *pVAudio = NULL;
	if(!pVAudio) {
		pVAudio = new CVAudioRedirect;
		if(!pVAudio->GetRedirectTarget()) {
			delete pVAudio;
			return NULL;
		}
	}
	return pVAudio;
}

EXPOSE_INTERFACE_FN(CreateAudioRedirect, IVAudio, VAUDIO_INTERFACE_VERSION);
