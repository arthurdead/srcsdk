//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( MODEL_TYPES_H )
#define MODEL_TYPES_H
#pragma once

#include "tier0/platform.h"

enum StudioDrawModelFlags_t : unsigned int;

enum DrawModelFlags_t : unsigned int
{
	STUDIO_NONE =						0x00000000,
	STUDIO_RENDER =					0x00000001,
	STUDIO_VIEWXFORMATTACHMENTS =		0x00000002,
	STUDIO_DRAWTRANSLUCENTSUBMODELS = 0x00000004,
	STUDIO_TWOPASS =					0x00000008,
	STUDIO_STATIC_LIGHTING =			0x00000010,
	STUDIO_WIREFRAME =				0x00000020,
	STUDIO_ITEM_BLINK =				0x00000040,
	STUDIO_NOSHADOWS =				0x00000080,
	STUDIO_WIREFRAME_VCOLLIDE =		0x00000100,
	STUDIO_NOLIGHTING_OR_CUBEMAP =	0x00000200,
	STUDIO_SKIP_FLEXES =				0x00000400,
	STUDIO_DONOTMODIFYSTENCILSTATE =	0x00000800,	// TERROR

	// Not a studio flag, but used to flag when we want studio stats
	STUDIO_GENERATE_STATS =			0x01000000,

	// Not a studio flag, but used to flag model as using shadow depth material override
	STUDIO_SSAODEPTHTEXTURE =			0x08000000,

	// Not a studio flag, but used to flag model as using shadow depth material override
	STUDIO_SHADOWDEPTHTEXTURE =		0x40000000,

	// Not a studio flag, but used to flag model as a non-sorting brush model
	STUDIO_TRANSPARENCY =				0x80000000,

	// Not a studio flag, but used to flag model as doing custom rendering into shadow texture
	STUDIO_SHADOWTEXTURE =			0x20000000,

	STUDIO_SKIP_DECALS =				0x10000000,
};

FLAGENUM_OPERATORS( DrawModelFlags_t, unsigned int )

enum modtype_t : unsigned int
{
	mod_bad = 0, 
	mod_brush, 
	mod_sprite, 
	mod_studio
};

#endif // MODEL_TYPES_H
