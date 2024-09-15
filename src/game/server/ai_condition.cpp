//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "stringregistry.h"
#include "ai_basenpc.h"
#include "ai_condition.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Given and condition name, return the condition ID
//-----------------------------------------------------------------------------
AiCondGlobalId_t CAI_BaseNPC::GetConditionID(const char* condName)
{
	return GetSchedulingSymbols()->ConditionSymbolToId(condName);
}

//-----------------------------------------------------------------------------
// Purpose: Register the default conditions
// Input  :
// Output :
//-----------------------------------------------------------------------------

#define ADD_CONDITION_TO_SR( _n ) idSpace.AddCondition( #_n, _n, "CAI_BaseNPC" )

void	CAI_BaseNPC::InitDefaultConditionSR(void)
{
	CAI_ClassScheduleIdSpace &idSpace = CAI_BaseNPC::AccessClassScheduleIdSpaceDirect();

	#define AI_COND_ENUM(name, ...) \
		ADD_CONDITION_TO_SR( name );

	#include "ai_default_cond_enum.inc"
}
