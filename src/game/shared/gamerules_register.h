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
typedef CGameRules CSharedGameRules;
#else
class C_GameRules;
typedef C_GameRules CSharedGameRules;
#endif

// Each game rules class must register using this in it's .cpp file.
#if defined(_STATIC_LINKED)
	#error
#endif

#define REGISTER_GAMERULES_CLASS_INTERNAL( className, name ) \
	CSharedGameRules * V_CONCAT2(__CreateGameRules_, className)() { \
		CSharedGameRules *pRules = new className; \
		if(!pRules->PostConstructor( V_STRINGIFY(className) )) { \
			UTIL_Remove(pRules); \
			return NULL; \
		} \
		return pRules; \
	} \
	static CGameRulesRegister V_CONCAT2(__g_GameRulesRegister_, className)( name, V_CONCAT2(__CreateGameRules_, className) );

#ifdef GAME_DLL
#define REGISTER_GAMERULES_CLASS_ALIASED( className ) REGISTER_GAMERULES_CLASS_INTERNAL( C##className, #className )
#else
#define REGISTER_GAMERULES_CLASS_ALIASED( className ) REGISTER_GAMERULES_CLASS_INTERNAL( C_##className, #className )
#endif

class CGameRulesRegister
{
public:
	typedef CSharedGameRules * (*CreateGameRulesFn)();

	CGameRulesRegister( const char *pClassName, CreateGameRulesFn fn );

	// Allocates the gamerules object associated with this class.
	CSharedGameRules *CreateGameRules();

	static CGameRulesRegister* FindByName( const char *pName );

private:
	const char *m_pClassName;
	CreateGameRulesFn m_pFn;
	CGameRulesRegister *m_pNext;	// Links it into the global list.
	
	static CGameRulesRegister *s_pHead;

};

void UTIL_Remove( CSharedGameRules *pGameRules );

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
