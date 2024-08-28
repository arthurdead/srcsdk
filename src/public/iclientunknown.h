//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ICLIENTUNKNOWN_H
#define ICLIENTUNKNOWN_H
#pragma once


#include "tier0/platform.h"
#include "ihandleentity.h"
#include "iclientalphaproperty.h"
#include "iclientrenderable.h"

class IClientNetworkable;
class C_BaseEntity;
class IClientRenderable;
class IClientRenderableMod;
class ICollideable;
class IClientEntity;
class IClientEntityMod;
class IClientThinkable;
class IClientModelRenderable;
class IClientAlphaProperty;
class IClientAlphaPropertyMod;


// This is the client's version of IUnknown. We may want to use a QueryInterface-like
// mechanism if this gets big.
abstract_class IClientUnknown : public IHandleEntity
{
public:
	virtual ICollideable*		GetCollideable() = 0;
	virtual IClientNetworkable*	GetClientNetworkable() = 0;
	virtual IClientRenderable*	GetClientRenderable() = 0;
	virtual IClientEntity*		GetIClientEntity() = 0;
	virtual C_BaseEntity*		GetBaseEntity() = 0;
	virtual IClientThinkable*	GetClientThinkable() = 0;
};

abstract_class IClientUnknownMod
{
public:
	virtual IClientModelRenderable*	GetClientModelRenderable() = 0;

	virtual IClientEntity*		GetIClientEntity() = 0;
	virtual IClientEntityMod*		GetIClientEntityMod() = 0;

	virtual IClientAlphaProperty*	GetClientAlphaProperty() = 0;
	virtual IClientAlphaPropertyMod*	GetClientAlphaPropertyMod()
	{
		IClientAlphaProperty *pAlphaProp = GetClientAlphaProperty();
		if(pAlphaProp)
			return dynamic_cast<IClientAlphaPropertyMod *>(pAlphaProp);
		return NULL;
	}

	virtual IClientRenderable*	GetClientRenderable() = 0;
	virtual IClientRenderableMod*	GetClientRenderableMod() = 0;
};

abstract_class IClientUnknownEx : public IClientUnknown, public IClientUnknownMod
{
public:
	virtual IClientUnknownMod*	GetIClientUnknownMod() { return this; }

	virtual IClientEntity*		GetIClientEntity() = 0;
	virtual IClientEntityMod*		GetIClientEntityMod();

	virtual IClientRenderable*	GetClientRenderable() = 0;
	virtual IClientRenderableMod*	GetClientRenderableMod()
	{
		IClientRenderable *pRenderable = GetClientRenderable();
		if(pRenderable)
			return dynamic_cast<IClientRenderableMod *>(pRenderable);
		return NULL;
	}
};

inline IClientUnknownMod*	IClientAlphaPropertyMod::GetIClientUnknownMod()
{
	IClientUnknown *pUnk = GetIClientUnknown();
	if(pUnk)
		return dynamic_cast<IClientUnknownMod *>(pUnk);
	return NULL;
}

inline IClientUnknownMod*	IClientAlphaPropertyEx::GetIClientUnknownMod()
{
	IClientUnknown *pUnk = GetIClientUnknown();
	if(pUnk)
		return dynamic_cast<IClientUnknownMod *>(pUnk);
	return NULL;
}

#endif // ICLIENTUNKNOWN_H
