//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef TOOLFRAMEWORK_SERVER_H
#define TOOLFRAMEWORK_SERVER_H

#pragma once


//-----------------------------------------------------------------------------
// Are tools enabled? 
//-----------------------------------------------------------------------------
#ifndef NO_TOOLFRAMEWORK
bool ToolsEnabled();
#else
#define ToolsEnabled() 0
#endif


#endif // TOOLFRAMEWORK_SERVER_H 
