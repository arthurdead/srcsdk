//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef GAMEUI_UTIL_H
#define GAMEUI_UTIL_H
#pragma once

#include "tier0/platform.h"
#include "tier1/convar.h"

const char	*VarArgs( PRINTF_FORMAT_STRING const char *format, ... );
#define UTIL_VarArgs VarArgs

void GameUI_MakeSafeName( const char *oldName, char *newName, int newNameBufSize );

//-----------------------------------------------------------------------------
// Useful for game ui since game ui has a single active "splitscreen" owner and since
//  it can gracefully handle non-FCVAR_SS vars without code changes required.
//-----------------------------------------------------------------------------
typedef ConVarRef CGameUIConVarRef;

#ifdef _DEBUG
// In GAMUI we should never use the regular ConVarRef
#define ConVarRef ****!!!USE_CGameUIConVarRef!!!!***
#endif

#endif // GAMEUI_UTIL_H
