//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ==================
//
// Purpose: Shared global string library.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPBASE_GLOBAL_STRINGS_H
#define MAPBASE_GLOBAL_STRINGS_H
#pragma once

#ifndef CBASE_H
#include "cbase.h"
#endif

// -------------------------------------------------------------
// 
// Valve uses global pooled strings in various parts of the code, particularly Episodic code,
// so they could get away with integer/pointer comparisons instead of string comparisons.
// 
// This system was developed early in Mapbase's development as an attempt to make this technique more widely used.
// For the most part, this mainly just serves to apply micro-optimize parts of the code.
// 
// -------------------------------------------------------------

// -------------------------------------------------------------
// 
// Classnames
// 
// -------------------------------------------------------------

extern string_t gm_isz_class_Bullseye;

extern string_t gm_isz_class_PropPhysics;
extern string_t gm_isz_class_PropPhysicsOverride;
extern string_t gm_isz_class_FuncPhysbox;
extern string_t gm_isz_class_EnvFire;

// -------------------------------------------------------------

extern string_t gm_isz_name_player;
extern string_t gm_isz_name_activator;

// -------------------------------------------------------------

// Does the classname of this entity match the string_t?
// 
// This function is for comparing global strings and allows us to change how we compare them quickly.
inline bool EntIsClass( CBaseEntity *ent, string_t str2 )
{
	//return ent->ClassMatches(str2);

	// Since classnames are pooled, the global string and the entity's classname should point to the same string in memory.
	// As long as this rule is preserved, we only need a pointer comparison. A string comparison isn't necessary.
	return ent->m_iClassname == str2;
}

// -------------------------------------------------------------

void InitGlobalStrings();

// -------------------------------------------------------------

#endif
