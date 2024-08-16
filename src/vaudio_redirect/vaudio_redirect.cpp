#include "tier0/dbg.h"
#include "tier1/interface.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"
#include "vaudio/ivaudio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef LIBNAME
	#error
#endif

class CVAudioRedirect : public IVAudio
{
public:
	CVAudioRedirect();
	~CVAudioRedirect();

	bool Init();

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

bool CVAudioRedirect::Init()
{
	CSysModule *pTargetMod = NULL;

	const char *pGameDir = CommandLine()->ParmValue("-game", "hl2");

	char szTargetPath[MAX_PATH];
	V_strncpy(szTargetPath, pGameDir, sizeof(szTargetPath));
	int len = V_strlen(szTargetPath);
	if(szTargetPath[len] != CORRECT_PATH_SEPARATOR &&
		szTargetPath[len] != INCORRECT_PATH_SEPARATOR) {
		V_strcat(szTargetPath, CORRECT_PATH_SEPARATOR_S "bin" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
	} else {
		V_strcat(szTargetPath, "bin" CORRECT_PATH_SEPARATOR_S, sizeof(szTargetPath));
	}

	if(strcmp(V_STRINGIFY(LIBNAME), "vaudio_miles") == 0) {
		V_strcat(szTargetPath, "vaudio_minimp3", sizeof(szTargetPath));
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

	int status;
	m_pTarget = (IVAudio *)pFactory(VAUDIO_INTERFACE_VERSION, &status);
	if(status != IFACE_OK) {
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
		if(!pVAudio->Init()) {
			delete pVAudio;
			return NULL;
		}
	}
	return pVAudio;
}

EXPOSE_INTERFACE_FN(CreateAudioRedirect, IVAudio, VAUDIO_INTERFACE_VERSION);
