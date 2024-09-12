//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef VIEWPOSTPROCESS_H
#define VIEWPOSTPROCESS_H

#pragma once

#include "view_shared.h"
#include "postprocess_shared.h"
#include "iclientrenderable.h"

struct RenderableInstance_t;

void DoEnginePostProcessing( int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui = false );
void DoImageSpaceMotionBlur( const CViewSetupEx &view, int x, int y, int w, int h );
void DumpTGAofRenderTarget( const int width, const int height, const char *pFilename );

bool IsDepthOfFieldEnabled();
void DoDepthOfField( const CViewSetupEx &view );
void BlurEntity( IClientRenderable *pRenderable, IClientRenderableMod *pRenderableMod, bool bPreDraw, int drawFlags, const RenderableInstance_t &instance, const CViewSetupEx &view, int x, int y, int w, int h );

void SetRenderTargetAndViewPort( ITexture *rt );

void UpdateMaterialSystemTonemapScalar();

float GetCurrentTonemapScale();

void SetOverrideTonemapScale( bool bEnableOverride, float flTonemapScale );

void SetOverridePostProcessingDisable( bool bForceOff );

void DoBlurFade( float flStrength, float flDesaturate, int x, int y, int w, int h );

void SetPostProcessParams( const PostProcessParameters_t *pPostProcessParameters );
void SetPostProcessParams( const PostProcessParameters_t* pPostProcessParameters, bool override );

void SetViewFadeParams( byte r, byte g, byte b, byte a, bool bModulate );

#endif // VIEWPOSTPROCESS_H
