//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ==================
//
// Purpose: See GlobalStrings.h for more information.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "GlobalStrings.h"
#include "gamestringpool.h"

#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global strings must be initially declared here.
// Be sure to sync them with the externs in GlobalStrings.h.

// -------------------------------------------------------------
// 
// Classnames
// 
// -------------------------------------------------------------

string_t g_iszFuncBrushClassname = NULL_STRING;
string_t g_iszPhysicsPropClassname = NULL_STRING;

string_t gm_isz_class_Bullseye = NULL_STRING;

string_t gm_isz_class_PropPhysicsOverride = NULL_STRING;
string_t gm_isz_class_FuncPhysbox = NULL_STRING;
string_t gm_isz_class_EnvFire = NULL_STRING;
string_t gm_isz_class_PropPhysics = NULL_STRING;

// -------------------------------------------------------------

string_t gm_isz_name_player = NULL_STRING;
string_t gm_isz_name_activator = NULL_STRING;

// -------------------------------------------------------------

bool EntIsClass( CBaseEntity *ent, string_t str2 )
{
#ifdef NO_STRING_T
	return ent->ClassMatches(str2);
#else
	// Since classnames are pooled, the global string and the entity's classname should point to the same string in memory.
	// As long as this rule is preserved, we only need a pointer comparison. A string comparison isn't necessary.
	return ent->GetClassnameStr() == str2;
#endif
}

// -------------------------------------------------------------

// We know it hasn't been allocated yet
#define INITIALIZE_GLOBAL_STRING(string, text) \
	string = AllocPooledString(text)

void InitGlobalStrings()
{
	INITIALIZE_GLOBAL_STRING(g_iszFuncBrushClassname, "func_brush");
	INITIALIZE_GLOBAL_STRING(g_iszPhysicsPropClassname, "prop_physics");

	INITIALIZE_GLOBAL_STRING(gm_isz_class_Bullseye, "npc_bullseye");

	INITIALIZE_GLOBAL_STRING(gm_isz_class_PropPhysics, "prop_physics");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_PropPhysicsOverride, "prop_physics_override");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_FuncPhysbox, "func_physbox"); 
	INITIALIZE_GLOBAL_STRING(gm_isz_class_EnvFire, "env_fire");

	INITIALIZE_GLOBAL_STRING(gm_isz_name_player, "!player");
	INITIALIZE_GLOBAL_STRING(gm_isz_name_activator, "!activator");
}

// -------------------------------------------------------------



