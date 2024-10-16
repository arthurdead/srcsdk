//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
// 
// Purpose: Accessing MapEdit
// 
// $NoKeywords: $
//=============================================================================//
#ifndef MAPEDIT_H
#define MAPEDIT_H

#pragma once

#include "tier1/convar.h"


extern ConVar mapedit_enabled;
extern ConVar mapedit_stack;
extern ConVar mapedit_debug;

void MapEdit_MapReload( void );

void MapEdit_LoadFile( const char *pFile, bool bStack = mapedit_stack.GetBool() );

#endif