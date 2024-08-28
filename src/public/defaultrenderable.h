#ifndef DEFAULTCLIENTRENDERABLE_H
#define DEFAULTCLIENTRENDERABLE_H
#pragma once

#include "iclientunknown.h"
#include "iclientrenderable.h"
#include "icliententity.h"

// This class can be used to implement default versions of some of the 
// functions of IClientRenderable.
abstract_class CDefaultClientRenderable : public IClientUnknownEx, public IClientRenderableEx
{
public:
	CDefaultClientRenderable()
	{
		m_hRenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	}

	virtual const Vector &			GetRenderOrigin( void ) = 0;
	virtual const QAngle &			GetRenderAngles( void ) = 0;
	virtual const matrix3x4_t &		RenderableToWorldTransform() = 0;
	virtual bool					ShouldDraw( void ) = 0;
	virtual RenderableTranslucencyType_t ComputeTranslucencyType() = 0;
	virtual void					OnThreadedDrawSetup() {}
	virtual int                     GetRenderFlags( void ) { return 0; }
	virtual ClientShadowHandle_t	GetShadowHandle() const
	{
		return CLIENTSHADOW_INVALID_HANDLE;
	}

	virtual ClientRenderHandle_t&	RenderHandle()
	{
		return m_hRenderHandle;
	}

	virtual int						GetBody() { return 0; }
	virtual int						GetSkin() { return 0; }
	virtual bool					UsesFlexDelayedWeights() { return false; }

	virtual const model_t*			GetModel( ) const		{ return NULL; }
	virtual int						DrawModel( int flags, const RenderableInstance_t &instance )	{ return 0; }
	virtual bool					LODTest()				{ return true; }
	virtual bool					SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )	{ return true; }
	virtual void					SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights ) {}
	virtual void					DoAnimationEvents( void )						{}
	virtual IPVSNotify*				GetPVSNotifyInterface() { return NULL; }
	virtual void					GetRenderBoundsWorldspace( Vector& absMins, Vector& absMaxs ) { DefaultRenderBoundsWorldspace( this, absMins, absMaxs ); }

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color )
	{
		Assert(color);
		color[0] = color[1] = color[2] = 1.0f;
	}

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveProjectedTextures( int flags ) 
	{
		return false;
	}

	// These methods return true if we want a per-renderable shadow cast direction + distance
	virtual bool	GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const			{ return false; }
	virtual bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const	{ return false; }

	virtual void	GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
	{
		GetRenderBounds( mins, maxs );
	}

	virtual bool IsShadowDirty( )			     { return false; }
	virtual void MarkShadowDirty( bool bDirty )  {}
	virtual IClientRenderable *GetShadowParent() { return NULL; }
	virtual IClientRenderable *FirstShadowChild(){ return NULL; }
	virtual IClientRenderable *NextShadowPeer()  { return NULL; }
	virtual ShadowType_t ShadowCastType()		 { return SHADOWS_NONE; }
	virtual void CreateModelInstance()			 {}
	virtual ModelInstanceHandle_t GetModelInstance() { return MODEL_INSTANCE_INVALID; }

	// Attachments
	virtual int LookupAttachment( const char *pAttachmentName ) { return -1; }
	virtual	bool GetAttachment( int number, Vector &origin, QAngle &angles ) { return false; }
	virtual bool GetAttachment( int number, matrix3x4_t &matrix ) {	return false; }

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float *GetRenderClipPlane() { return NULL; }

	virtual void RecordToolMessage() {}

	virtual uint8	OverrideRenderAlpha( uint8 nAlpha ) { return nAlpha; }
	virtual uint8	OverrideShadowRenderAlpha( uint8 nAlpha ) { return nAlpha; }

// IClientUnknown implementation.
public:
	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ Assert( false ); return NULL_HANDLE; }

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual IClientUnknownMod*	GetIClientUnknownMod() { return this; }

	virtual ICollideable*		GetCollideable()		{ return NULL; }

	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientRenderableMod*	GetClientRenderableMod()	{ return this; }

	virtual IClientNetworkable*	GetClientNetworkable()	{ return NULL; }

	virtual IClientEntity*		GetIClientEntity()		{ return NULL; }
	virtual IClientEntityMod*		GetIClientEntityMod()
	{
		IClientEntity *pClientEnt = GetIClientEntity();
		if(!pClientEnt)
			return NULL;
		return dynamic_cast<IClientEntityMod *>(pClientEnt);
	}

	virtual C_BaseEntity*		GetBaseEntity()			{ return NULL; }
	virtual IClientThinkable*	GetClientThinkable()	{ return NULL; }
	virtual IClientModelRenderable*	GetClientModelRenderable()	{ return NULL; }

	virtual IClientAlphaProperty*	GetClientAlphaProperty() { return NULL; }
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod()
	{
		IClientAlphaProperty *pAlphaProp = GetClientAlphaProperty();
		if(!pAlphaProp)
			return NULL;
		return dynamic_cast<IClientAlphaPropertyMod *>(pAlphaProp);
	}

public:
	ClientRenderHandle_t m_hRenderHandle;
};

#endif