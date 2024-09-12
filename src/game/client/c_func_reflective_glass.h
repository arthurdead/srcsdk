//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef C_FUNC_REFLECTIVE_GLASS
#define C_FUNC_REFLECTIVE_GLASS

#pragma once

struct cplane_t;
class CViewSetup;
class C_BaseEntity;
class ITexture;
struct Frustum_t;
class CViewSetupEx;

//-----------------------------------------------------------------------------
// Do we have reflective glass in view? If so, what's the reflection plane?
//-----------------------------------------------------------------------------
bool IsReflectiveGlassInView( const CViewSetupEx& view, cplane_t &plane );

C_BaseEntity *NextReflectiveGlass( C_BaseEntity *pStart, const CViewSetupEx& view, cplane_t &plane,
	const Frustum_t &frustum, ITexture **pRenderTargets = NULL );

#endif // C_FUNC_REFLECTIVE_GLASS


