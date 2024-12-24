//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ISHADOWMGR_H
#define ISHADOWMGR_H

#pragma once

#include "interface.h"
#include "mathlib/vmatrix.h"
#include "hackmgr/hackmgr.h"
#include "engine/ivmodelrender.h"
#include "shadowflags.h"

DECLARE_LOGGING_CHANNEL( LOG_SHADOWS );

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class IMaterial;
class Vector;
class Vector2D;
struct model_t;
class IClientRenderable;
class ITexture;
struct FlashlightInstance_t;
struct FlashlightState_t;
enum ShadowFlags_t : unsigned short;

// change this when the new version is incompatable with the old
#define ENGINE_SHADOWMGR_INTERFACE_VERSION	"VEngineShadowMgr002"

//-----------------------------------------------------------------------------
//
// Shadow-related functionality exported by the engine
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// This is a handle	to shadows, clients can create as many as they want
//-----------------------------------------------------------------------------
enum ShadowHandle_t : unsigned short;
inline const ShadowHandle_t SHADOW_HANDLE_INVALID = (ShadowHandle_t)~0;

UNORDEREDENUM_OPERATORS( ShadowHandle_t, unsigned short )

//-----------------------------------------------------------------------------
// Used for the creation Flags field of CreateShadow
//-----------------------------------------------------------------------------
enum ShadowCreateFlags_t : unsigned int
{
	SHADOW_NO_CREATION_FLAGS = 0,

	SHADOW_CREATE_CACHE_VERTS =  ( 1 << 0 ),
	SHADOW_CREATE_FLASHLIGHT =   ( 1 << 1 ),
	SHADOW_LAST_CREATION_FLAG = SHADOW_CREATE_FLASHLIGHT,

	//(SHADOW_LAST_CREATION_FLAG << 1) is used by the engine
	SHADOW_CREATE_SIMPLE_PROJECTION	= ( SHADOW_LAST_CREATION_FLAG << 2 ),
};

FLAGENUM_OPERATORS( ShadowCreateFlags_t, unsigned int )

//-----------------------------------------------------------------------------
// Information about a particular shadow
//-----------------------------------------------------------------------------
struct ShadowInfo_t
{
	// Transforms from world space into texture space of the shadow
	VMatrix		m_WorldToShadow;

	// The shadow should no longer be drawn once it's further than MaxDist
	// along z in shadow texture coordinates.
	float			m_FalloffOffset;
	float			m_MaxDist;
	float			m_FalloffAmount;	// how much to lighten the shadow maximally
	Vector2D		m_TexOrigin;
	Vector2D		m_TexSize;
	unsigned char	m_FalloffBias;
};

typedef void (*ShadowDrawCallbackFn_t)( void * );

struct FlashlightState_t;
struct FlashlightStateMod_t;
struct FlashlightStateEx_t;

//-----------------------------------------------------------------------------
// The engine's interface to the shadow manager
//-----------------------------------------------------------------------------
abstract_class IShadowMgr
{
public:
	// Create, destroy shadows (see ShadowCreateFlags_t for creationFlags)
	virtual ShadowHandle_t CreateShadow( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, ShadowCreateFlags_t creationFlags ) = 0;
	virtual void DestroyShadow( ShadowHandle_t handle ) = 0;

	// Resets the shadow material (useful for shadow LOD.. doing blobby at distance) 
	virtual void SetShadowMaterial( ShadowHandle_t handle, IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy ) = 0;

	// Shadow opacity
//	virtual void SetShadowOpacity( ShadowHandle_t handle, float alpha ) = 0;
//	virtual float GetShadowOpacity( ShadowHandle_t handle ) const = 0;

	// Project a shadow into the world
	// The two points specify the upper left coordinate and the lower-right
	// coordinate of the shadow specified in a shadow "viewplane". The
	// projection matrix is a shadow viewplane->world transformation,
	// and can be orthographic orperspective.

	// I expect that the client DLL will call this method any time the shadow
	// changes because the light changes, or because the entity casting the
	// shadow moves

	// Note that we can't really control the shadows from the engine because
	// the engine only knows about pevs, which don't exist on the client

	// The shadow matrix specifies a world-space transform for the shadow
	// the shadow is projected down the z direction, and the origin of the
	// shadow matrix is the origin of the projection ray. The size indicates
	// the shadow size measured in the space of the shadow matrix; the
	// shadow goes from +/- size.x/2 along the x axis of the shadow matrix
	// and +/- size.y/2 along the y axis of the shadow matrix.
	virtual void ProjectShadow( ShadowHandle_t handle, const Vector &origin, 
		const Vector& projectionDir, const VMatrix& worldToShadow, const Vector2D& size,
		int nLeafCount, const int *pLeafList,
		float maxHeight, float falloffOffset, float falloffAmount, const Vector &vecCasterOrigin ) = 0;

	virtual void ProjectFlashlight( ShadowHandle_t handle, const VMatrix &worldToShadow, int nLeafCount, const int *pLeafList ) = 0;

	// Gets at information about a particular shadow
	virtual const ShadowInfo_t &GetInfo( ShadowHandle_t handle ) = 0;

	virtual const Frustum_t &GetFlashlightFrustum( ShadowHandle_t handle ) = 0;

	// Methods related to shadows on brush models
	virtual void AddShadowToBrushModel( ShadowHandle_t handle, 
		model_t* pModel, const Vector& origin, const QAngle& angles ) = 0;

	// Removes all shadows from a brush model
	virtual void RemoveAllShadowsFromBrushModel( model_t* pModel ) = 0;

	// Sets the texture coordinate range for a shadow...
	virtual void SetShadowTexCoord( ShadowHandle_t handle, float x, float y, float w, float h ) = 0;

	// Methods related to shadows on studio models
	virtual void AddShadowToModel( ShadowHandle_t shadow, ModelInstanceHandle_t instance ) = 0;
	virtual void RemoveAllShadowsFromModel( ModelInstanceHandle_t instance ) = 0;

	// Set extra clip planes related to shadows...
	// These are used to prevent pokethru and back-casting
	virtual void ClearExtraClipPlanes( ShadowHandle_t shadow ) = 0;
	virtual void AddExtraClipPlane( ShadowHandle_t shadow, const Vector& normal, float dist ) = 0;

	// Allows us to disable particular shadows
	virtual void EnableShadow( ShadowHandle_t shadow, bool bEnable ) = 0;

	// Set the darkness falloff bias
	virtual void SetFalloffBias( ShadowHandle_t shadow, unsigned char ucBias ) = 0;

	// Update the state for a flashlight.
	virtual void UpdateFlashlightState( ShadowHandle_t shadowHandle, const FlashlightState_t &lightState ) = 0;
	HACKMGR_CLASS_API void UpdateFlashlightState( ShadowHandle_t shadowHandle, const FlashlightStateEx_t &lightState );

	virtual void DrawFlashlightDepthTexture( ) = 0;

	virtual void AddFlashlightRenderable( ShadowHandle_t shadow, IClientRenderable *pRenderable ) = 0;
	virtual ShadowHandle_t CreateShadowEx( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, ShadowCreateFlags_t creationFlags ) = 0;
	HACKMGR_CLASS_API ShadowHandle_t CreateShadowEx( IMaterial* pMaterial, IMaterial* pModelMaterial, void* pBindProxy, ShadowCreateFlags_t creationFlags, int nEntIndex );

	virtual void SetFlashlightDepthTexture( ShadowHandle_t shadowHandle, ITexture *pFlashlightDepthTexture, unsigned char ucShadowStencilBit ) = 0;

	virtual const FlashlightState_t &GetFlashlightState( ShadowHandle_t handle ) = 0;
	HACKMGR_CLASS_API const FlashlightStateMod_t &GetFlashlightStateMod( ShadowHandle_t handle );

	virtual void SetFlashlightRenderState( ShadowHandle_t handle ) = 0;

	HACKMGR_CLASS_API void RemoveAllDecalsFromShadow( ShadowHandle_t handle );

	HACKMGR_CLASS_API void EndFlashlightRenderState( ShadowHandle_t handle );

	HACKMGR_CLASS_API void DrawVolumetrics();

	HACKMGR_CLASS_API int GetNumShadowsOnModel( ModelInstanceHandle_t instance );
	HACKMGR_CLASS_API int GetShadowsOnModel( ModelInstanceHandle_t instance, ShadowHandle_t* pShadowArray, bool bNormalShadows, bool bFlashlightShadows );

	HACKMGR_CLASS_API void FlashlightDrawCallback( ShadowDrawCallbackFn_t pCallback, void *pData ); //used to draw each additive flashlight pass. The callback is called once per flashlight state for an additive pass.

	//Way for the client to determine which flashlight to use in single-pass modes. Does not actually enable the flashlight in any way.
	HACKMGR_CLASS_API void SetSinglePassFlashlightRenderState( ShadowHandle_t handle );

	//Enable/Disable the flashlight state set with SetSinglePassFlashlightRenderState.
	HACKMGR_CLASS_API void PushSinglePassFlashlightStateEnabled( bool bEnable );
	HACKMGR_CLASS_API void PopSinglePassFlashlightStateEnabled( void );

	HACKMGR_CLASS_API bool SinglePassFlashlightModeEnabled( void );

	// Determine a unique list of flashlights which hit at least one of the specified models
	// Accepts an instance count and an array of ModelInstanceHandle_ts.
	// Returns the number of FlashlightInstance_ts it's found that affect the models.
	// Also fills in a mask of which flashlights affect each ModelInstanceHandle_t
	// There can be at most MAX_FLASHLIGHTS_PER_INSTANCE_DRAW_CALL pFlashlights,
	// and the size of the pModelUsageMask array must be nInstanceCount.
	HACKMGR_CLASS_API int SetupFlashlightRenderInstanceInfo( ShadowHandle_t *pUniqueFlashlights, uint32 *pModelUsageMask, int nUsageStride, int nInstanceCount, const ModelInstanceHandle_t *pInstance );

	// Returns the flashlight state for multiple flashlights
	HACKMGR_CLASS_API void GetFlashlightRenderInfo( FlashlightInstance_t *pFlashlightState, int nCount, const ShadowHandle_t *pHandles );

	HACKMGR_CLASS_API void SkipShadowForEntity( int nEntIndex );
};


#endif
