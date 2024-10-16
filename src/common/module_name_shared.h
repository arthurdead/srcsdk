#ifndef MODULE_NAME_SHARED_H
#define MODULE_NAME_SHARED_H

#pragma once

#if defined MEMOVERRIDE_MODULE && !defined DLLNAME
	#define DLLNAME MEMOVERRIDE_MODULE
#endif

#ifndef DLLNAME
	#if defined GAME_DLL || defined SERVER_DLL
		#if defined SWDS || defined DEDICATED
			#define DLLNAME server_srv
		#else
			#define DLLNAME server
		#endif
	#elif defined CLIENT_DLL
		#define DLLNAME client
	#elif defined GAMEUI_EXPORTS
		#define DLLNAME GameUI
	#elif defined GAMEPADUI_DLL
		#define DLLNAME gamepadui
	#elif defined GAME_SHADER_DLL
		#ifdef STDSHADER_DX9_DLL_EXPORT
			#define DLLNAME game_shader_dx9
		#else
			#define DLLNAME game_shader_generic
		#endif
	#endif
#endif

#if !defined MEMOVERRIDE_MODULE && defined DLLNAME
	#define MEMOVERRIDE_MODULE DLLNAME
#endif

#ifndef LIBNAME
	#ifdef TIER1_STATIC_LIB
		#define LIBNAME tier1
	#elif defined MATHLIB_LIB
		#define LIBNAME mathlib
	#endif
#endif

#endif
