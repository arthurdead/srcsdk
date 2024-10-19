#include "IHammer.h"
#include "tier1/interface.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHammerRedirect : public IHammer
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName );

	// Init, shutdown
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

	virtual bool HammerPreTranslateMessage( MSG * pMsg );
	virtual bool HammerIsIdleMessage( MSG * pMsg );
	virtual bool HammerOnIdle( long count );

	virtual void RunFrame();

	// Returns the mod and the game to initially start up
	virtual const char *GetDefaultMod();
	virtual const char *GetDefaultGame();

	virtual bool InitSessionGameConfig( const char *szGameDir );

	// Request a new config from hammer's config system
	virtual RequestRetval_t RequestNewConfig();

	// Returns the full path to the mod and the game to initially start up
	virtual const char *GetDefaultModFullPath();

	virtual int MainLoop();

	bool GetRedirectTarget();

private:
#ifdef __linux__
	char m_szMod[MAX_PATH];
	char m_szModPath[MAX_PATH];
	char m_szGame[MAX_PATH];
#else
	IHammer *m_pTarget;
#endif
};

bool CHammerRedirect::Connect( CreateInterfaceFn factory )
{
#ifdef __linux__
	return true;
#else
	return m_pTarget->Connect(factory);
#endif
}

void CHammerRedirect::Disconnect()
{
#ifdef __linux__

#else
	m_pTarget->Disconnect();
#endif
}

void *CHammerRedirect::QueryInterface( const char *pInterfaceName )
{
#ifdef __linux__
	if (!Q_strncmp(	pInterfaceName, INTERFACEVERSION_HAMMER, Q_strlen(INTERFACEVERSION_HAMMER) + 1))
		return (IHammer*)this;

	return NULL;
#else
	return m_pTarget->QueryInterface(pInterfaceName);
#endif
}

InitReturnVal_t CHammerRedirect::Init()
{
#ifdef __linux__
	return INIT_OK;
#else
	return m_pTarget->Init();
#endif
}

void CHammerRedirect::Shutdown()
{
#ifdef __linux__

#else
	m_pTarget->Shutdown();
#endif
}

bool CHammerRedirect::HammerPreTranslateMessage( MSG * pMsg )
{
#ifdef __linux__
	return true;
#else
	return m_pTarget->HammerPreTranslateMessage(pMsg);
#endif
}

bool CHammerRedirect::HammerIsIdleMessage( MSG * pMsg )
{
#ifdef __linux__
	return true;
#else
	return m_pTarget->HammerIsIdleMessage(pMsg);
#endif
}

bool CHammerRedirect::HammerOnIdle( long count )
{
#ifdef __linux__
	return true;
#else
	return m_pTarget->HammerOnIdle(count);
#endif
}

void CHammerRedirect::RunFrame()
{
#ifdef __linux__
	
#else
	m_pTarget->RunFrame();
#endif
}

const char *CHammerRedirect::GetDefaultMod()
{
#ifdef __linux__
	return m_szMod;
#else
	return m_pTarget->GetDefaultMod();
#endif
}

const char *CHammerRedirect::GetDefaultGame()
{
#ifdef __linux__
	return m_szGame;
#else
	return m_pTarget->GetDefaultGame();
#endif
}

bool CHammerRedirect::InitSessionGameConfig( const char *szGameDir )
{
#ifdef __linux__
	return true;
#else
	return m_pTarget->InitSessionGameConfig(szGameDir);
#endif
}

RequestRetval_t CHammerRedirect::RequestNewConfig()
{
#ifdef __linux__
	return REQUEST_OK;
#else
	return m_pTarget->RequestNewConfig();
#endif
}

const char *CHammerRedirect::GetDefaultModFullPath()
{
#ifdef __linux__
	return m_szModPath;
#else
	return m_pTarget->GetDefaultModFullPath();
#endif
}

int CHammerRedirect::MainLoop()
{
#ifdef __linux__
	return 0;
#else
	return m_pTarget->MainLoop();
#endif
}

bool CHammerRedirect::GetRedirectTarget()
{
#ifndef __linux__
	#error
#else
	const char *pDefaultGameDir = CommandLine()->ParmValue( "-defaultgamedir", "hl2" );
	const char *pGameDir = CommandLine()->ParmValue( "-game", pDefaultGameDir );

	V_FileBase( pGameDir, m_szMod, sizeof(m_szMod) );
	V_FileBase( pDefaultGameDir, m_szGame, sizeof(m_szGame) );

	V_MakeAbsolutePath( m_szModPath, sizeof(m_szModPath), pGameDir, NULL );

	return true;
#endif
}

void *CreateHammer()
{
	static CHammerRedirect *pHammer = NULL;
	if(!pHammer) {
		pHammer = new CHammerRedirect;
		if(!pHammer->GetRedirectTarget()) {
			delete pHammer;
			return NULL;
		}
	}
	return pHammer;
}

EXPOSE_INTERFACE_FN(CreateHammer, IHammer, INTERFACEVERSION_HAMMER);
