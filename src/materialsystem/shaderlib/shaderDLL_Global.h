//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef SHADERDLL_GLOBAL_H
#define SHADERDLL_GLOBAL_H

#pragma once


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IShaderSystem;


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
inline IShaderSystem *GetShaderSystem()
{
	extern IShaderSystem* g_pSLShaderSystem;
	return g_pSLShaderSystem;
}


#endif	// SHADERDLL_GLOBAL_H
