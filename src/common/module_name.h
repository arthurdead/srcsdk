#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#pragma once

#ifndef DLLNAME
	#if defined GAME_DLL || defined SERVER_DLL
		#if defined SWDS || defined DEDICATED
			#define DLLNAME server_srv
		#else
			#define DLLNAME server
		#endif
	#endif

	#ifdef CLIENT_DLL
		#define DLLNAME client
	#endif

	#ifdef GAMEUI_EXPORTS
		#define DLLNAME GameUI
	#endif

	#if defined GAMEPADUI_DLL && !defined GAMEUI_EXPORTS
		#define DLLNAME gamepadui
	#endif

	#ifdef GAME_SHADER_DLL
		#ifdef STDSHADER_DX9_DLL_EXPORT
			#define DLLNAME game_shader_dx9
		#else
			#define DLLNAME game_shader_generic
		#endif
	#endif
#endif

#ifndef LIBNAME
	#ifdef TIER1_STATIC_LIB
		#define LIBNAME tier1
	#endif

	#ifdef MATHLIB_LIB
		#define LIBNAME mathlib
	#endif
#endif

namespace modulename
{
	extern const char *dll;
	extern const char *lib;
}

#endif
