//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SERVER_H
#define VSCRIPT_SERVER_H

#include "vscript/ivscript.h"
#include "vscript_shared.h"

#pragma once

extern IScriptVM * g_pScriptVM;

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

#endif // VSCRIPT_SERVER_H
