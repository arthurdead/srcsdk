//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Holds defintion for game ammo types
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef AI_AMMODEF_H
#define AI_AMMODEF_H

#pragma once

#include "tier0/platform.h"

#ifdef GAME_DLL
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
#else
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
#endif

class ConVar;

#define BULLET_MASS_GRAINS_TO_LB(grains) (0.002285f*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains) lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

#define BULLET_IMPULSE_EXAGGERATION 3.5f
#define BULLET_IMPULSE(grains, ftpersec) ((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

struct Ammo_t 
{
	char 				*pName;
	uint64					nDamageType;
	int					eTracerType;
	float				physicsForceImpulse;
	int					nMinSplashSize;
	int					nMaxSplashSize;

	int					nFlags;

	// Values for player/NPC damage and carrying capability
	// If the integers are set, they override the CVars
	int					pPlrDmg;		// CVar for player damage amount
	int					pNPCDmg;		// CVar for NPC damage amount
	int					pMaxCarry;		// CVar for maximum number can carry
	const ConVarBase*		pPlrDmgCVar;	// CVar for player damage amount
	const ConVarBase*		pNPCDmgCVar;	// CVar for NPC damage amount
	const ConVarBase*		pMaxCarryCVar;	// CVar for maximum number can carry
};

// Used to tell AmmoDef to use the cvars, not the integers
#define		USE_CVAR		-1
// Ammo is infinite
#define		INFINITE_AMMO	-2

enum AmmoTracer_t
{
	TRACER_NONE,
	TRACER_LINE,
	TRACER_RAIL,
	TRACER_BEAM,
	TRACER_LINE_AND_WHIZ,
};

enum AmmoFlags_t
{
	AMMO_FORCE_DROP_IF_CARRIED = 0x1,
	AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER = 0x2,
};


#include "shareddefs.h"

//=============================================================================
//	>> CAmmoDef
//=============================================================================
class CAmmoDef
{

public:
	int					m_nAmmoIndex;

	Ammo_t				m_AmmoType[MAX_AMMO_TYPES];

	Ammo_t				*GetAmmoOfIndex(int nAmmoIndex);
	int					Index(const char *psz);
	const char*			Name(int nAmmoIndex);
	int					PlrDamage(int nAmmoIndex);
	int					NPCDamage(int nAmmoIndex);
	int					MaxCarry(int nAmmoIndex, const CSharedBaseCombatCharacter *owner);
	bool				CanCarryInfiniteAmmo(int nAmmoIndex);
	uint64					DamageType(int nAmmoIndex);
	int					TracerType(int nAmmoIndex);
	float				DamageForce(int nAmmoIndex);
	int					MinSplashSize(int nAmmoIndex);
	int					MaxSplashSize(int nAmmoIndex);
	int					Flags(int nAmmoIndex);

	void				AddAmmoType(char const* name, uint64 damageType, int tracerType, int plr_dmg, int npc_dmg, int carry, float physicsForceImpulse, int nFlags, int minSplashSize = 4, int maxSplashSize = 8 );
	void				AddAmmoType(char const* name, uint64 damageType, int tracerType, char const* plr_cvar, char const* npc_var, char const* carry_cvar, float physicsForceImpulse, int nFlags, int minSplashSize = 4, int maxSplashSize = 8 );

	int					NumAmmoTypes() { return m_nAmmoIndex; }
	int					GetNumAmmoTypes() { return m_nAmmoIndex; }

	CAmmoDef(void);
	virtual ~CAmmoDef( void );

private:
	bool				AddAmmoType(char const* name, uint64 damageType, int tracerType, int nFlags, int minSplashSize, int maxSplashSize );
};


// Get the global ammodef object. This is usually implemented in each mod's game rules file somewhere,
// so the mod can setup custom ammo types.
CAmmoDef* GetAmmoDef();


#endif // AI_AMMODEF_H
 
