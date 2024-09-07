//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#if !defined( CLIENTEFFECTPRECACHESYSTEM_H )
#define CLIENTEFFECTPRECACHESYSTEM_H
#pragma once

#include "igamesystem.h"
#include "commonmacros.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "tier1/utldict.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Interface to automated system for precaching materials
//-----------------------------------------------------------------------------
class IClientEffect
{
public:
	virtual ~IClientEffect() {}
	virtual void Init()	= 0;
	virtual void Precache()	= 0;
	virtual void Shutdown()	= 0;
	virtual void Release()	= 0;
};

//-----------------------------------------------------------------------------
// Responsible for managing precaching of particles
//-----------------------------------------------------------------------------

class CClientEffectPrecacheSystem : public IGameSystem
{
public:
	virtual char const *Name() { return "CCLientEffectPrecacheSystem"; }

	virtual bool	IsPerFrame() { return false; }

	// constructor, destructor
	CClientEffectPrecacheSystem() {}
	virtual ~CClientEffectPrecacheSystem() {}

	// Init, shutdown
	virtual bool Init();
	virtual void PostInit() {}
	virtual void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	void Register( IClientEffect *effect, const char *pName );
	IClientEffect *Find( const char *pName );

protected:

	CUtlDict< IClientEffect * >	m_Effects;
};

//Singleton accessor
extern CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem();

//-----------------------------------------------------------------------------
// Deals with automated registering and precaching of materials for effects
//-----------------------------------------------------------------------------

class CClientEffect : public IClientEffect
{
public:

	CClientEffect( const char *pName );
	CClientEffect( const char *pName, bool (*pCondFunc)() );
	~CClientEffect();

//-----------------------------------------------------------------------------
// Purpose: Precache a material by artificially incrementing its reference counter
// Input  : *materialName - name of the material
//		  : increment - whether to increment or decrement the reference counter
//-----------------------------------------------------------------------------

	void AddMaterial( const char *materialName );

	void Init();
	void Precache();
	void Shutdown();
	void Release();

private:
	bool m_bPrecached;
	CUtlVector< CUtlString > m_MaterialsNames;
	CUtlVector< IMaterial	* > m_Materials;
	bool (*m_pCondFunc)();
};

//Automatic precache macros

//Beginning
#define	CLIENTEFFECT_REGISTER_BEGIN( className )		\
namespace className {									\
static const char *EffectName = #className; \
class ClientEffectRegister : public CClientEffect		\
{														\
public: \
	ClientEffectRegister(bool(*pCondFunc)()); \
};														\
ClientEffectRegister::ClientEffectRegister(bool(*pCondFunc)()) : CClientEffect(EffectName, pCondFunc) {

//Material definitions
#define	CLIENTEFFECT_MATERIAL( materialName )	AddMaterial(materialName);

//End
#define	CLIENTEFFECT_REGISTER_END( )	}					\
INIT_PRIORITY(65535) ClientEffectRegister	register_ClientEffectRegister(NULL);		\
}

#define	CLIENTEFFECT_REGISTER_END_CONDITIONAL( ... )	}					\
static bool EffectCond() { return (__VA_ARGS__); } \
INIT_PRIORITY(65535) static ClientEffectRegister	register_ClientEffectRegister(EffectCond);		\
}

#endif	//CLIENTEFFECTPRECACHESYSTEM_H
