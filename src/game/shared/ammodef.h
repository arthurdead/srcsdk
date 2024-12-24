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

enum DamageTypes_t : uint64;

#ifdef GAME_DLL
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
#else
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
#endif

class ConVar;

#ifdef __MINGW32__
class ConVarBase;
#else
typedef ConVar ConVarBase;
#endif

#define BULLET_MASS_GRAINS_TO_LB(grains) (0.002285f*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains) lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

#define BULLET_IMPULSE_EXAGGERATION 3.5f
#define BULLET_IMPULSE(grains, ftpersec) ((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

enum AmmoTracer_t : unsigned char
{
	TRACER_NONE,
	TRACER_LINE,
	TRACER_RAIL,
	TRACER_BEAM,
	TRACER_LINE_AND_WHIZ,
};

enum AmmoFlags_t : unsigned char
{
	AMMO_NONE = 0,
	AMMO_FORCE_DROP_IF_CARRIED = 0x1,
	AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER = 0x2,
};

FLAGENUM_OPERATORS( AmmoFlags_t, unsigned char )

struct Ammo_t 
{
	char 				*pName;
	DamageTypes_t					nDamageType;
	AmmoTracer_t					eTracerType;
	float				physicsForceImpulse;
	int					nMinSplashSize;
	int					nMaxSplashSize;

	AmmoFlags_t					nFlags;

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

enum AmmoIndex_t : unsigned short;
inline const AmmoIndex_t AMMO_INVALID_INDEX = (AmmoIndex_t)-1;

UNORDEREDENUM_OPERATORS( AmmoIndex_t, unsigned short )

// is this required?
#define	MAX_AMMO_TYPES	128		// ???
#define MAX_AMMO_SLOTS  128		// not really slots

//=============================================================================
//	>> CAmmoDef
//=============================================================================
class CAmmoDef
{

private:
	unsigned int					m_nAmmoIndex;

	Ammo_t				m_AmmoType[MAX_AMMO_TYPES];

public:
	Ammo_t				*GetAmmoOfIndex(AmmoIndex_t nAmmoIndex);
	AmmoIndex_t					Index(const char *psz);
	const char*			Name(AmmoIndex_t nAmmoIndex);
	int					PlrDamage(AmmoIndex_t nAmmoIndex);
	int					NPCDamage(AmmoIndex_t nAmmoIndex);
	int					MaxCarry(AmmoIndex_t nAmmoIndex, const CSharedBaseCombatCharacter *owner);
	bool				CanCarryInfiniteAmmo(AmmoIndex_t nAmmoIndex);
	DamageTypes_t					DamageType(AmmoIndex_t nAmmoIndex);
	AmmoTracer_t					TracerType(AmmoIndex_t nAmmoIndex);
	float				DamageForce(AmmoIndex_t nAmmoIndex);
	int					MinSplashSize(AmmoIndex_t nAmmoIndex);
	int					MaxSplashSize(AmmoIndex_t nAmmoIndex);
	AmmoFlags_t					Flags(AmmoIndex_t nAmmoIndex);

	void				AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, int plr_dmg, int npc_dmg, int carry, float physicsForceImpulse, AmmoFlags_t nFlags, int minSplashSize = 4, int maxSplashSize = 8 );
	void				AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, char const* plr_cvar, char const* npc_var, char const* carry_cvar, float physicsForceImpulse, AmmoFlags_t nFlags, int minSplashSize = 4, int maxSplashSize = 8 );

	int					NumAmmoTypes() { return m_nAmmoIndex; }
	int					GetNumAmmoTypes() { return m_nAmmoIndex; }

	CAmmoDef(void);
	virtual ~CAmmoDef( void );

private:
	bool				AddAmmoType(char const* name, DamageTypes_t damageType, AmmoTracer_t tracerType, AmmoFlags_t nFlags, int minSplashSize, int maxSplashSize );
};


// Get the global ammodef object. This is usually implemented in each mod's game rules file somewhere,
// so the mod can setup custom ammo types.
CAmmoDef* GetAmmoDef();


#endif // AI_AMMODEF_H
 
