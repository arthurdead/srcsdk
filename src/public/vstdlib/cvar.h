//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined( CVAR_H )
#define CVAR_H
#pragma once

#include "vstdlib/vstdlib.h"
#include "icvar.h"


//-----------------------------------------------------------------------------
// Returns a CVar dictionary for tool usage
//-----------------------------------------------------------------------------
VSTDLIB_INTERFACE CreateInterfaceFn VStdLib_GetICVarFactory();


#endif // CVAR_H
