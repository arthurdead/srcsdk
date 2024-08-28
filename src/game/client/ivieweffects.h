//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( IVIEWEFFECTS_H )
#define IVIEWEFFECTS_H
#pragma once

#include "tier0/platform.h"
#include "shake.h"

class Vector;
class QAngle;
class bf_read;

//-----------------------------------------------------------------------------
// Purpose: Apply effects to view origin/angles, etc.  Screen fade and shake
//-----------------------------------------------------------------------------
abstract_class IViewEffects
{
public:
	// Initialize subsystem
	virtual void	Init( void ) = 0;
	// Initialize after each level change
	virtual void	LevelInit( void ) = 0;
	// Called each frame to determine the current view fade parameters ( color and alpha )
	virtual void	GetFadeParams( unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a, bool *blend ) = 0;
	// Apply directscreen shake
	virtual void	Shake( const ScreenShake_t &data ) = 0;
	// Apply direct screen fade
	virtual void	Fade( const ScreenFade_t &data ) = 0;
	virtual void	Tilt( const ScreenTilt_t &data ) = 0;
	// Clear all permanent fades in our fade list
	virtual void	ClearPermanentFades( void ) = 0;
	// Clear all fades in our fade list
	virtual void	ClearAllFades( void ) = 0;
	// Compute screen shake values for this frame
	virtual void	CalcShake( void ) = 0;
	// Apply those values to the passed in vector(s).
	virtual void	ApplyShake(  Vector& origin,  QAngle& angles, float factor ) = 0;
	// Compute screen tilt values for this frame
	virtual void	CalcTilt( void ) = 0;
	// Apply those values to the passed in vector(s).
	virtual void	ApplyTilt(  QAngle& angles, float factor ) = 0;
	virtual void	ClearAllShakes( void ) = 0;
};

extern IViewEffects *GetViewEffects();

#endif // IVIEWEFFECTS_H