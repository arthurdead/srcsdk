//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======
//
// Purpose: Win32 Console API helpers
//
//=============================================================================
#ifndef CONSOLE_IO_H
#define CONSOLE_IO_H

#pragma once

#include "tier0/platform.h"

// Function to attach a console for I/O to a Win32 GUI application in a reasonably smart fashion.
PLATFORM_INTERFACE bool SetupConsoleIO();

// Win32 Console Color API Helpers, originally from cmdlib.

struct ConsoleColorContext_t
{
	int  m_InitialColor;
	uint16 m_LastColor;
	uint16 m_BadColor;
	uint16 m_BackgroundFlags;
};

PLATFORM_INTERFACE void InitConsoleColorContext( ConsoleColorContext_t *pContext );

PLATFORM_INTERFACE uint16 SetConsoleColor( ConsoleColorContext_t *pContext, int nRed, int nGreen, int nBlue, int nIntensity );

PLATFORM_INTERFACE void RestoreConsoleColor( ConsoleColorContext_t *pContext, uint16 prevColor );

#endif 
