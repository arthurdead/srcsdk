#ifndef SHADOWFLAGS_H
#define SHADOWFLAGS_H

#pragma once

#include "tier0/platform.h"

//-----------------------------------------------------------------------------
// Flags for the creation method
//-----------------------------------------------------------------------------
enum ShadowFlags_t : unsigned short
{
	//used by engine?
	SHADOW_FLAGS_FLASHLIGHT            = (1 << 0),
	SHADOW_FLAGS_SHADOW                = (1 << 1),
	SHADOW_FLAGS_LAST_FLAG = SHADOW_FLAGS_SHADOW,

	//used by tools
	SHADOW_FLAGS_USE_RENDER_TO_TEXTURE = (1 << 2),
	SHADOW_FLAGS_ANIMATING_SOURCE      = (1 << 3),
	SHADOW_FLAGS_USE_DEPTH_TEXTURE     = (1 << 4),
	SHADOW_FLAGS_CUSTOM_DRAW           = (1 << 5),
	CLIENT_SHADOW_FLAGS_LAST_FLAG = SHADOW_FLAGS_CUSTOM_DRAW,

	//used by client
	SHADOW_FLAGS_TEXTURE_DIRTY         = (1 << 6),
	SHADOW_FLAGS_BRUSH_MODEL           = (1 << 7), 
	SHADOW_FLAGS_USING_LOD_SHADOW      = (1 << 8),
	SHADOW_FLAGS_LIGHT_WORLD           = (1 << 9),

	//newly added
	SHADOW_FLAGS_SIMPLE_PROJECTION     = (1 << 10),
	SHADOW_FLAGS_PLAYER_FLASHLIGHT     = (1 << 11),
	SHADOW_FLAGS_ACTUAL_LAST_FLAG = SHADOW_FLAGS_PLAYER_FLASHLIGHT,

	SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK = ( SHADOW_FLAGS_FLASHLIGHT | SHADOW_FLAGS_SHADOW | SHADOW_FLAGS_SIMPLE_PROJECTION ),
};

FLAGENUM_OPERATORS( ShadowFlags_t, unsigned int )

typedef ShadowFlags_t ClientShadowFlags_t;
typedef ClientShadowFlags_t ToolShadowFlags_t;

#endif
