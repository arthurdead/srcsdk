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

extern const char *COM_GetModDirectory();

const char	*VarArgs( PRINTF_FORMAT_STRING const char *format, ... );
#define UTIL_VarArgs VarArgs

// ScreenHeight returns the height of the screen, in pixels
int		ScreenHeight( void );
// ScreenWidth returns the width of the screen, in pixels
int		ScreenWidth( void );

#define XRES(x)	( x  * ( ( float )ScreenWidth() / 640.0 ) )
#define YRES(y)	( y  * ( ( float )ScreenHeight() / 480.0 ) )

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
