//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ICLIENTRENDERABLE_H
#define ICLIENTRENDERABLE_H
#pragma once

#include "mathlib/mathlib.h"
#include "interface.h"
#include "client_render_handle.h"
#include "engine/ivmodelrender.h"
#include "engine/ivmodelinfo.h"
#include "iclientalphaproperty.h"
#include "basehandle.h"

struct model_t;
struct matrix3x4_t;

extern void DefaultRenderBoundsWorldspace( IClientRenderable *pRenderable, Vector &absMins, Vector &absMaxs );

class IClientAlphaProperty;

//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
typedef unsigned short ClientShadowHandle_t;

enum
{
	CLIENTSHADOW_INVALID_HANDLE = (ClientShadowHandle_t)~0
};

//-----------------------------------------------------------------------------
// What kind of shadows to render?
//-----------------------------------------------------------------------------
enum ShadowType_t
{
	SHADOWS_NONE = 0,
	SHADOWS_SIMPLE,
	SHADOWS_RENDER_TO_TEXTURE,
	SHADOWS_RENDER_TO_TEXTURE_DYNAMIC,	// the shadow is always changing state
	SHADOWS_RENDER_TO_DEPTH_TEXTURE,
	SHADOWS_RENDER_TO_TEXTURE_DYNAMIC_CUSTOM,	// changing, and entity uses custom rendering code for shadow
};

//-----------------------------------------------------------------------------
// Information needed to draw a model
//-----------------------------------------------------------------------------
struct RenderableInstance_t
{
	RenderableInstance_t()
	{
		m_nAlpha = 255;
	}

	uint8 m_nAlpha;
};


// client renderable frame buffer usage flags
#define ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB  1				// needs refract texture
#define ERENDERFLAGS_NEEDS_FULL_FB          2				// needs full framebuffer texture
#define ERENDERFLAGS_REFRACT_ONLY_ONCE_PER_FRAME 4 // even if it needs a the refract texture, don't update it >once/ frame


// This provides a way for entities to know when they've entered or left the PVS.
// Normally, server entities can use NotifyShouldTransmit to get this info, but client-only
// entities can use this. Store a CPVSNotifyInfo in your 
//
// When bInPVS=true, it's being called DURING rendering. It might be after rendering any
// number of views.
//
// If no views had the entity, then it is called with bInPVS=false after rendering.
abstract_class IPVSNotify
{
public:
	virtual void OnPVSStatusChanged( bool bInPVS ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: All client entities must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientRenderable
{
public:
	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;

	// Data accessors
	virtual Vector const&			GetRenderOrigin( void ) = 0;
	virtual QAngle const&			GetRenderAngles( void ) = 0;
	virtual bool					ShouldDraw( void ) = 0;
private:
	virtual bool					DO_NOT_USE_IsTransparent( void ) = 0;
	virtual bool					DO_NOT_USE_UsesPowerOfTwoFrameBufferTexture() = 0;
	virtual bool					DO_NOT_USE_UsesFullFrameBufferTexture() = 0;
public:
	virtual ClientShadowHandle_t	GetShadowHandle() const
	{
		return CLIENTSHADOW_INVALID_HANDLE;
	}


	// Used by the leaf system to store its render handle.
	virtual ClientRenderHandle_t&	RenderHandle() = 0;

	// Render baby!
	virtual const model_t*			GetModel( ) const = 0;
private:
	virtual int						DO_NOT_USE_DrawModel( int flags ) = 0;
public:
	// Get the body parameter
	virtual int		GetBody() = 0;

	// Determine alpha and blend amount for transparent objects based on render state info
private:
	friend void CallComputeFXBlend( IClientRenderable *&pRenderable );
	friend class CClientLeafSystem;
	virtual void	DO_NOT_USE_ComputeFxBlend( ) = 0;
	virtual int		DO_NOT_USE_GetFxBlend( void ) = 0;
public:

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color ) = 0;

	// Returns false if the entity shouldn't be drawn due to LOD. 
	// (NOTE: This is no longer used/supported, but kept in the vtable for backwards compat)
	virtual bool	LODTest() = 0;

	// Call this to get the current bone transforms for the model.
	// currentTime parameter will affect interpolation
	// nMaxBones specifies how many matrices pBoneToWorldOut can hold. (Should be greater than or
	// equal to studiohdr_t::numbones. Use MAXSTUDIOBONES to be safe.)
	virtual bool	SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime ) = 0;

	virtual void	SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights ) = 0;
	virtual void	DoAnimationEvents( void ) = 0;
	
	// Return this if you want PVS notifications. See IPVSNotify for more info.	
	// Note: you must always return the same value from this function. If you don't,
	// undefined things will occur, and they won't be good.
	virtual IPVSNotify* GetPVSNotifyInterface() = 0;

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs ) = 0;
	
	// returns the bounds as an AABB in worldspace
	virtual void	GetRenderBoundsWorldspace( Vector& mins, Vector& maxs ) = 0;

	// These normally call through to GetRenderAngles/GetRenderBounds, but some entities custom implement them.
	virtual void	GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType ) = 0;

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveProjectedTextures( int flags ) = 0;

	// These methods return true if we want a per-renderable shadow cast direction + distance
	virtual bool	GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const = 0;
	virtual bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const = 0;

	// Other methods related to shadow rendering
	virtual bool	IsShadowDirty( ) = 0;
	virtual void	MarkShadowDirty( bool bDirty ) = 0;

	// Iteration over shadow hierarchy
	virtual IClientRenderable *GetShadowParent() = 0;
	virtual IClientRenderable *FirstShadowChild() = 0;
	virtual IClientRenderable *NextShadowPeer() = 0;

	// Returns the shadow cast type
	virtual ShadowType_t ShadowCastType() = 0;

	// Create/get/destroy model instance
	virtual void CreateModelInstance() = 0;
	virtual ModelInstanceHandle_t GetModelInstance() = 0;

	// Returns the transform from RenderOrigin/RenderAngles to world
	virtual const matrix3x4_t &RenderableToWorldTransform() = 0;

	// Attachments
	virtual int LookupAttachment( const char *pAttachmentName ) = 0;
	virtual	bool GetAttachment( int number, Vector &origin, QAngle &angles ) = 0;
	virtual bool GetAttachment( int number, matrix3x4_t &matrix ) = 0;

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float *GetRenderClipPlane( void ) = 0;

	// Get the skin parameter
	virtual int		GetSkin() = 0;

	// Is this a two-pass renderable?
private:
	virtual bool	DO_NOT_USE_IsTwoPass( void ) = 0;
public:
	virtual void	OnThreadedDrawSetup() = 0;

	virtual bool	UsesFlexDelayedWeights() = 0;

	virtual void	RecordToolMessage() = 0;

private:
	virtual bool	DO_NOT_USE_IgnoresZBuffer( void ) const = 0;
};

abstract_class IClientRenderableMod
{
private:
	RenderableInstance_t m_RenderableInstance;

public:
	IClientRenderableMod()
	{
		m_RenderableInstance.m_nAlpha = 255;
	}

	virtual ~IClientRenderableMod() {}

	virtual RenderableTranslucencyType_t ComputeTranslucencyType() = 0;

	virtual int					    GetRenderFlags( void ) = 0; // ERENDERFLAGS_xxx
	virtual int						DrawModel( int flags, const RenderableInstance_t &instance ) = 0;

	// NOTE: This is used by renderables to override the default alpha modulation,
	// not including fades, for a renderable. The alpha passed to the function
	// is the alpha computed based on the current renderfx.
	virtual uint8	OverrideRenderAlpha( uint8 nAlpha ) = 0;

	// NOTE: This is used by renderables to override the default alpha modulation,
	// not including fades, for a renderable's shadow. The alpha passed to the function
	// is the alpha computed based on the current renderfx + any override
	// computed in OverrideAlphaModulation
	virtual uint8	OverrideShadowRenderAlpha( uint8 nAlpha ) = 0;

	virtual IClientAlphaProperty*	GetClientAlphaProperty() = 0;
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod()
	{
		IClientAlphaProperty *pAlphaProp = GetClientAlphaProperty();
		if(!pAlphaProp)
			return NULL;
		return dynamic_cast<IClientAlphaPropertyMod *>(pAlphaProp);
	}

	const RenderableInstance_t &GetDefaultRenderableInstance() const
	{ return m_RenderableInstance; }
	RenderableInstance_t &GetDefaultRenderableInstance()
	{ return m_RenderableInstance; }
};

abstract_class IClientRenderableEx : public IClientRenderable, public IClientRenderableMod
{
public:
	virtual IClientRenderableMod*	GetClientRenderableMod() { return this; }

private:
#ifdef _DEBUG
	virtual void ComputeFxBlend() final
	{
		DebuggerBreak();
	}
	virtual int GetFxBlend() final
	{
		DebuggerBreak();
		return 0;
	}
#endif

#ifdef _DEBUG
	virtual bool UsesPowerOfTwoFrameBufferTexture() final
	{
		DebuggerBreak();
		return false;
	}
	virtual bool UsesFullFrameBufferTexture() final
	{
		DebuggerBreak();
		return false;
	}
#endif
	virtual bool DO_NOT_USE_UsesPowerOfTwoFrameBufferTexture() final
	{ return (GetRenderFlags() & ERENDERFLAGS_NEEDS_POWER_OF_TWO_FB) != 0; }
	virtual bool DO_NOT_USE_UsesFullFrameBufferTexture() final
	{ return (GetRenderFlags() & ERENDERFLAGS_NEEDS_FULL_FB) != 0; }

#ifdef _DEBUG
	virtual int DrawModel(int flags) final
	{
		DebuggerBreak();
		return 0;
	}
#endif
	virtual int DO_NOT_USE_DrawModel(int flags) final
	{
		RenderableInstance_t &instance = GetDefaultRenderableInstance();
		IClientAlphaPropertyMod *pAlphaProp = GetClientAlphaPropertyMod();
		if(pAlphaProp)
			instance.m_nAlpha = pAlphaProp->GetAlphaBlend();
		else
			instance.m_nAlpha = 255;
		return static_cast<IClientRenderableMod *>(this)->DrawModel(flags, instance);
	}

	virtual void DO_NOT_USE_ComputeFxBlend() final
	{
		IClientAlphaPropertyMod *pAlphaProp = GetClientAlphaPropertyMod();
		if(pAlphaProp)
			pAlphaProp->ComputeAlphaBlend();
	}

	virtual int DO_NOT_USE_GetFxBlend() final
	{
		IClientAlphaPropertyMod *pAlphaProp = GetClientAlphaPropertyMod();
		if(pAlphaProp)
			return pAlphaProp->GetAlphaBlend();
		return 255;
	}

#ifdef _DEBUG
	virtual bool IgnoresZBuffer() const final
	{
		DebuggerBreak();
		return true;
	}
#endif
	virtual bool DO_NOT_USE_IgnoresZBuffer() const final
	{
		IClientAlphaPropertyMod *pAlphaProp = const_cast<IClientRenderableEx *>(this)->GetClientAlphaPropertyMod();
		if(pAlphaProp)
			return pAlphaProp->IgnoresZBuffer();
		return false;
	}

#ifdef _DEBUG
	virtual bool IsTransparent() final
	{
		DebuggerBreak();
		return true;
	}
#endif
	virtual bool DO_NOT_USE_IsTransparent() final
	{ return (ComputeTranslucencyType() == RENDERABLE_IS_TRANSLUCENT); }

#ifdef _DEBUG
	virtual bool IsTwoPass() final
	{
		DebuggerBreak();
		return false;
	}
#endif
	virtual bool DO_NOT_USE_IsTwoPass() final
	{ return (ComputeTranslucencyType() == RENDERABLE_IS_TWO_PASS); }

public:

};

//-----------------------------------------------------------------------------
// Purpose: All client renderables supporting the fast-path mdl
// rendering algorithm must inherit from this interface
//-----------------------------------------------------------------------------
enum RenderableLightingModel_t
{
	LIGHTING_MODEL_NONE = -1,
	LIGHTING_MODEL_STANDARD = 0,
	LIGHTING_MODEL_STATIC_PROP,
	LIGHTING_MODEL_PHYSICS_PROP,

	LIGHTING_MODEL_COUNT,
};

enum ModelDataCategory_t
{
	MODEL_DATA_LIGHTING_MODEL,	// data type returned is a RenderableLightingModel_t
	MODEL_DATA_STENCIL,			// data type returned is a ShaderStencilState_t

	MODEL_DATA_CATEGORY_COUNT,
};


abstract_class IClientModelRenderable
{
public:
	virtual bool GetRenderData( void *pData, ModelDataCategory_t nCategory ) = 0;
};


#endif // ICLIENTRENDERABLE_H
