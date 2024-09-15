//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		The default shared conditions
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	CONDITION_H
#define	CONDITION_H

// NOTE: Changing this constant will break save files!!! (changes type of CAI_ScheduleBits)
#ifndef MAX_CONDITIONS
#define	MAX_CONDITIONS 32*8
#endif

//=========================================================
// These are the default shared conditions
//=========================================================
enum SCOND_t 
{
	#define AI_COND_ENUM(name, ...) \
		name __VA_OPT__( = __VA_ARGS__),

	#include "ai_default_cond_enum.inc"

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_CONDITION	
};

typedef int AiCondGlobalId_t;
typedef int AiCondLocalId_t;

//-----------------

#define DECLARE_CONDITION( id ) \
	conditionIds.PushBack( #id, id );

#define ADD_CUSTOM_CONDITION_NAMED(derivedClass,condName,condEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddCondition( condName, condEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_CONDITION(derivedClass,condEN) ADD_CUSTOM_CONDITION_NAMED(derivedClass,#condEN,condEN)


#endif	//CONDITION_H
