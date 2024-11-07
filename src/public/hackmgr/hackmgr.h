#ifndef HACKMGR_H
#define HACKMGR_H

#pragma once

#include "tier0/platform.h"
#include "engine_target.h"
#include "tier1/interface.h"
#include "globalvars_base.h"

#ifdef BUILDING_HACKMGR
	#define HACKMGR_API DLL_EXPORT
	#define HACKMGR_CLASS_API DLL_CLASS_EXPORT
	#define HACKMGR_OVERLOAD_API DLL_GLOBAL_EXPORT
#else
	#define HACKMGR_API DLL_IMPORT
	#define HACKMGR_CLASS_API DLL_CLASS_IMPORT
	#define HACKMGR_OVERLOAD_API DLL_GLOBAL_IMPORT
#endif

#define HACKMGR_OPEN_PARENTHESIS (
#define HACKMGR_CLOSE_PARENTHESIS )

#define HACKMGR_CONCAT5_IMPL(a1,a2,a3,a4,a5) a1##a2##a3##a4##a5
#define HACKMGR_CONCAT5(a1,a2,a3,a4,a5) HACKMGR_CONCAT5_IMPL(a1,a2,a3,a4,a5)

#ifdef __GNUC__
#define HACKMGR_INIT_PRIO(...) __attribute__((__init_priority__((__VA_ARGS__))))
#else
#define HACKMGR_INIT_PRIO(...) 
#endif

#ifdef __GNUC__
#define HACKMGR_EXECUTE_ON_LOAD_BEGIN(...) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wprio-ctor-dtor\"") \
	[[using __gnu__: __constructor__ __VA_OPT__(HACKMGR_OPEN_PARENTHESIS HACKMGR_OPEN_PARENTHESIS __VA_ARGS__ HACKMGR_CLOSE_PARENTHESIS HACKMGR_CLOSE_PARENTHESIS)]] static void HACKMGR_CONCAT5(_, __LINE__, _, __COUNTER__, _)() {
#define HACKMGR_EXECUTE_ON_LOAD_END \
	} \
	_Pragma("GCC diagnostic pop")

#define HACKMGR_EXECUTE_ON_UNLOAD_BEGIN(...) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wprio-ctor-dtor\"") \
	[[using __gnu__: __deconstructor__ __VA_OPT__(HACKMGR_OPEN_PARENTHESIS HACKMGR_OPEN_PARENTHESIS __VA_ARGS__ HACKMGR_CLOSE_PARENTHESIS HACKMGR_CLOSE_PARENTHESIS)]] static void HACKMGR_CONCAT5(_, __LINE__, _, __COUNTER__, _)() {
#define HACKMGR_EXECUTE_ON_UNLOAD_END \
	} \
	_Pragma("GCC diagnostic pop")
#else
#define HACKMGR_EXECUTE_ON_LOAD_BEGIN(...) \
namespace HACKMGR_CONCAT5(_, __LINE__, _, __COUNTER__, _) { \
	__VA_OPT__(HACKMGR_INIT_PRIO(__VA_ARGS__)) static struct _ { \
	public: \
		_() {
#define HACKMGR_EXECUTE_ON_LOAD_END \
		} \
	} _; \
}

#define HACKMGR_EXECUTE_ON_UNLOAD_BEGIN(...) \
namespace HACKMGR_CONCAT5(_, __LINE__, _, __COUNTER__, _) { \
	static struct _ { \
	public: \
		~_() {
#define HACKMGR_EXECUTE_ON_UNLOAD_END \
		} \
	} _; \
}
#endif

HACKMGR_API void HackMgr_DependantModuleLoaded(const char *name);

#ifndef SWDS
class IBaseClientDLL;
#endif
class IServerGameDLL;
struct CGlobalVars;

#ifndef SWDS
HACKMGR_API bool HackMgr_Client_PreInit(IBaseClientDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals);
#endif
HACKMGR_API bool HackMgr_Server_PreInit(IServerGameDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals, bool bDedicated);

HACKMGR_API bool HackMgr_IsGamePaused();
HACKMGR_API void HackMgr_SetGamePaused(bool value);

#endif
