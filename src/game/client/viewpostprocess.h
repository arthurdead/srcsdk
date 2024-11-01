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

enum HistogramEntryState_t
{
	HESTATE_INITIAL=0,
	HESTATE_FIRST_QUERY_IN_FLIGHT,
	HESTATE_QUERY_IN_FLIGHT,
	HESTATE_QUERY_DONE,
};

class CHistogramBucket
{
public:
	HistogramEntryState_t m_state;
	OcclusionQueryObjectHandle_t m_hOcclusionQueryHandle;
	int m_nFrameQueued;									// when this query was last queued
	int m_nPixels;										// # of pixels this histogram represents
	int m_nPixelsInRange;
	float m_flMinLuminance, m_flMaxLuminance;			// the luminance range this entry was queried with
	float m_flScreenMinX, m_flScreenMinY, m_flScreenMaxX, m_flScreenMaxY; // range is 0..1 in fractions of the screen

	bool ContainsValidData( void )
	{
		return ( m_state == HESTATE_QUERY_DONE ) || ( m_state == HESTATE_QUERY_IN_FLIGHT );
	}

	void IssueQuery( int nFrameNum );
};

#define NUM_HISTOGRAM_BUCKETS 31
#define NUM_HISTOGRAM_BUCKETS_NEW 17
#define MAX_QUERIES_PER_FRAME 1

class CTonemapSystem
{
	CHistogramBucket m_histogramBucketArray[NUM_HISTOGRAM_BUCKETS];
	int m_nCurrentQueryFrame;
	int m_nCurrentAlgorithm;

	float m_flTargetTonemapScale;
	float m_flCurrentTonemapScale;

	int m_nNumMovingAverageValid;
	float m_movingAverageTonemapScale[10];

	bool m_bOverrideTonemapScaleEnabled;
	float m_flOverrideTonemapScale;

public:
	void IssueAndReceiveBucketQueries();
	void UpdateBucketRanges();
	float FindLocationOfPercentBrightPixels( float flPercentBrightPixels, float flPercentTarget );
	float ComputeTargetTonemapScalar( bool bGetIdealTargetForDebugMode );

	void UpdateMaterialSystemTonemapScalar();
	void SetTargetTonemappingScale( float flTonemapScale );
	void ResetTonemappingScale( float flTonemapScale );
	void SetTonemapScale( IMatRenderContext *pRenderContext, float newvalue, float minvalue, float maxvalue );

	float GetTargetTonemappingScale() { return m_flTargetTonemapScale; }
	float GetCurrentTonemappingScale() { return m_flCurrentTonemapScale; }

	void SetOverrideTonemapScale( bool bEnableOverride, float flTonemapScale );

	// Dev functions
	void DisplayHistogram();

	// Constructor
	CTonemapSystem();
};

extern CTonemapSystem * GetCurrentTonemappingSystem();

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
