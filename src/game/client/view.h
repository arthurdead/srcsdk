//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#if !defined( VIEW_H )
#define VIEW_H
#pragma once

#include "vstdlib/cvar.h"
#include "tier1/convar.h"

#if _DEBUG
extern bool g_bRenderingCameraView;		// For debugging (frustum fix for cameras)...
#endif

class VMatrix;
class Vector;
class QAngle;
class VPlane;


// near and far Z it uses to render the world.
#define VIEW_NEARZ	7
//#define VIEW_FARZ	28400


//-----------------------------------------------------------------------------
// There's a difference between the 'current view' and the 'main view'
// The 'main view' is where the player is sitting. Current view is just
// what's currently being rendered, which, owing to monitors or water,
// could be just about anywhere.
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin();
const QAngle &MainViewAngles();
const Vector &PrevMainViewOrigin();
const QAngle &PrevMainViewAngles();
const VMatrix &MainWorldToViewMatrix();
const Vector &MainViewForward();
const Vector &MainViewRight();
const Vector &MainViewUp();

const Vector &CurrentViewOrigin();
const QAngle &CurrentViewAngles();
const VMatrix &CurrentWorldToViewMatrix();
const Vector &CurrentViewForward();
const Vector &CurrentViewRight();
const Vector &CurrentViewUp();

// This identifies the view for certain systems that are unique per view (e.g. pixel visibility)
// NOTE: This is identifying which logical part of the scene an entity is being redered in
// This is not identifying a particular render target necessarily.  This is mostly needed for entities that
// can be rendered more than once per frame (pixel vis queries need to be identified per-render call)
enum view_id_t
{
	VIEW_ILLEGAL = -2,
	VIEW_NONE = -1,
	VIEW_MAIN = 0,
	VIEW_3DSKY = 1,
	VIEW_MONITOR = 2,
	VIEW_REFLECTION = 3,
	VIEW_REFRACTION = 4,
	VIEW_INTRO_PLAYER = 5,
	VIEW_INTRO_CAMERA = 6,
	VIEW_SHADOW_DEPTH_TEXTURE = 7,
	VIEW_SSAO = 8,
	VIEW_ID_COUNT
};
view_id_t CurrentViewID();

void AllowCurrentViewAccess( bool allow );
bool IsCurrentViewAccessAllowed();

// Returns true of the sphere is outside the frustum defined by pPlanes.
// (planes point inwards).
bool R_CullSphere( const VPlane *pPlanes, int nPlanes, const Vector *pCenter, float radius );
float ScaleFOVByWidthRatio( float fovDegrees, float ratio );

extern ConVar mat_wireframe;

extern const ConVar *sv_cheats;


static inline int WireFrameMode( void )
{
	if ( !sv_cheats )
	{
		sv_cheats = g_pCVar->FindVar( "sv_cheats" );
	}

	if ( sv_cheats && sv_cheats->GetBool() )
		return mat_wireframe.GetInt();
	else
		return 0;
}

static inline bool ShouldDrawInWireFrameMode( void )
{
	if ( !sv_cheats )
	{
		sv_cheats = g_pCVar->FindVar( "sv_cheats" );
	}

	if ( sv_cheats && sv_cheats->GetBool() )
		return ( mat_wireframe.GetInt() != 0 );
	else
		return false;
}

void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse );

#endif // VIEW_H
