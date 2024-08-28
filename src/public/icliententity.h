//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLIENTENTITY_H
#define ICLIENTENTITY_H
#pragma once


#include "iclientrenderable.h"
#include "iclientnetworkable.h"
#include "iclientthinkable.h"

struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;
class CMouthInfo;
class IClientEntityInternal;
struct SpatializationInfo_t;


//-----------------------------------------------------------------------------
// Purpose: All client entities must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientEntity : public IClientUnknown, public IClientRenderable, public IClientNetworkable, public IClientThinkable
{
public:
	virtual IClientUnknown*	GetIClientUnknown() { return this; }
	virtual IClientNetworkable*	GetClientNetworkable() { return this; }
	virtual IClientRenderable*	GetClientRenderable() { return this; }
	virtual IClientEntity*		GetIClientEntity() { return this; }
	virtual IClientThinkable*	GetClientThinkable() { return this; }

private:
	// Delete yourself.
	virtual void			DO_NOT_USE_Release( void ) = 0;

public:
	// Network origin + angles
	virtual const Vector&	GetAbsOrigin( void ) const = 0;
	virtual const QAngle&	GetAbsAngles( void ) const = 0;

	virtual CMouthInfo		*GetMouth( void ) = 0;

	// Retrieve sound spatialization info for the specified sound on this entity
	// Return false to indicate sound is not audible
	virtual bool			GetSoundSpatialization( SpatializationInfo_t& info ) = 0;
};

abstract_class IClientEntityMod
{
public:
	virtual bool			IsBlurred( void ) = 0;
};

abstract_class IClientEntityEx : public IClientEntity, public IClientEntityMod, public IClientUnknownMod, public IClientRenderableMod
{
public:
	virtual IClientEntityMod*		GetIClientEntityMod() { return this; }

	virtual IClientRenderable*	GetClientRenderable() { return this; }
	virtual IClientRenderableMod*	GetClientRenderableMod() { return this; }

	virtual IClientUnknown*	GetIClientUnknown() { return this; }
	virtual IClientUnknownMod*	GetIClientUnknownMod() { return this; }

	virtual IClientAlphaProperty*	GetClientAlphaProperty() = 0;
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod()
	{
		IClientAlphaProperty *pAlphaProp = GetClientAlphaProperty();
		if(pAlphaProp)
			return dynamic_cast<IClientAlphaPropertyMod *>(pAlphaProp);
		return NULL;
	}

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
		IClientAlphaPropertyMod *pAlphaProp = const_cast<IClientEntityEx *>(this)->GetClientAlphaPropertyMod();
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

	virtual bool IsTwoPass() final
	{
		DebuggerBreak();
		return false;
	}
	virtual bool DO_NOT_USE_IsTwoPass() final
	{ return (ComputeTranslucencyType() == RENDERABLE_IS_TWO_PASS); }

public:

};

inline IClientEntityMod*IClientUnknownEx::GetIClientEntityMod()
{
	IClientEntity *pClientEnt = GetIClientEntity();
	if(!pClientEnt)
		return NULL;
	return dynamic_cast<IClientEntityMod *>(pClientEnt);
}

#endif // ICLIENTENTITY_H
