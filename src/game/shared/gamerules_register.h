//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMERULES_REGISTER_H
#define GAMERULES_REGISTER_H
#pragma once

#ifdef GAME_DLL
class CGameRules;
#else
#define CGameRules C_GameRules
class C_GameRules;
#endif

// Each game rules class must register using this in it's .cpp file.
#if !defined(_STATIC_LINKED)
#define REGISTER_GAMERULES_CLASS( className ) \
	CGameRules *__CreateGameRules_##className() { \
		CGameRules *pRules = new className; \
		if(!pRules->PostConstructor( #className )) { \
			UTIL_Remove(pRules); \
			return NULL; \
		} \
		return pRules; \
	} \
	static CGameRulesRegister __g_GameRulesRegister_##className( #className, __CreateGameRules_##className );
#else
#define REGISTER_GAMERULES_CLASS( className ) \
	CGameRules * MAKE_NAME_UNIQUE(__CreateGameRules_)##className() { \
		CGameRules *pRules = new className; \
		if(!pRules->PostConstructor( #className )) { \
			UTIL_Remove(pRules); \
			return NULL; \
		} \
		return pRules; \
	} \
	static CGameRulesRegister __g_GameRulesRegister_##className( #className, MAKE_NAME_UNIQUE(__CreateGameRules_)##className );
#endif

class CGameRulesRegister
{
public:
	typedef CGameRules * (*CreateGameRulesFn)();

	CGameRulesRegister( const char *pClassName, CreateGameRulesFn fn );

	// Allocates the gamerules object associated with this class.
	CGameRules *CreateGameRules();

	static CGameRulesRegister* FindByName( const char *pName );

private:
	const char *m_pClassName;
	CreateGameRulesFn m_pFn;
	CGameRulesRegister *m_pNext;	// Links it into the global list.
	
	static CGameRulesRegister *s_pHead;

};

void UTIL_Remove( CGameRules *pGameRules );

#ifdef CLIENT_DLL

	// The client forwards this call so the game rules manager can create the appropriate
	// game rules class.
	void InstallStringTableCallback_GameRules();

#else

	// Server calls this at startup.
	void CreateNetworkStringTables_GameRules();
	
	// Server calls this to install a specific game rules object. The class should have been registered
	// with REGISTER_GAMERULES_CLASS.
	void CreateGameRulesObject( const char *pClassName );

#endif


#endif // GAMERULES_REGISTER_H
