//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMOVEHELPER_H
#define IMOVEHELPER_H

#pragma once

#include "shareddefs.h"
#include "soundflags.h"
#include "ehandle.h"
#include "bspflags.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class IPhysicsSurfaceProps;
class Vector;
struct model_t;
struct cmodel_t;
struct vcollide_t;
class CGameTrace;
enum soundlevel_t : unsigned int;

#ifdef GAME_DLL
class CBasePlayer;
typedef CBasePlayer CSharedBasePlayer;
#else
class C_BasePlayer;
typedef C_BasePlayer CSharedBasePlayer;
#endif

//-----------------------------------------------------------------------------
// Purpose: Identifies how submerged in water a player is.
//-----------------------------------------------------------------------------

enum WaterLevel_t : unsigned char
{
	WL_NotInWater=0,
	WL_Feet,
	WL_Waist,
	WL_Eyes
};

enum WaterType_t : unsigned char
{
	WT_None =               0,
	WT_Water =        (1 << 0),
	WT_Slime =        (1 << 1),
	WT_current_0 =    (1 << 2),
	WT_current_90 =   (1 << 3),
	WT_current_180 =  (1 << 4),
	WT_current_270 =  (1 << 5),
	WT_current_up =   (1 << 6),
	WT_current_down = (1 << 7),

	WT_mask_water = (WT_Water|WT_Slime),
	WT_mask_current = (WT_current_0|WT_current_90|WT_current_180|WT_current_270|WT_current_up|WT_current_down),
};

FLAGENUM_OPERATORS( WaterType_t, unsigned char )

inline WaterType_t WaterTypeFromContents(ContentsFlags_t cont)
{
	WaterType_t ret = WT_None;

	if(cont & CONTENTS_WATER)
		ret |= WT_Water;

	if(cont & CONTENTS_SLIME)
		ret |= WT_Slime;

	if(cont & CONTENTS_CURRENT_0)
		ret |= WT_current_0;

	if(cont & CONTENTS_CURRENT_90)
		ret |= WT_current_90;

	if(cont & CONTENTS_CURRENT_180)
		ret |= WT_current_180;

	if(cont & CONTENTS_CURRENT_270)
		ret |= WT_current_270;

	if(cont & CONTENTS_CURRENT_UP)
		ret |= WT_current_up;

	if(cont & CONTENTS_CURRENT_DOWN)
		ret |= WT_current_down;

	return ret;
}

//-----------------------------------------------------------------------------
// An entity identifier that works in both game + client dlls
//-----------------------------------------------------------------------------

typedef EHANDLE EntityHandle_t;


#define INVALID_ENTITY_HANDLE INVALID_EHANDLE_INDEX

//-----------------------------------------------------------------------------
// Functions the engine provides to IGameMovement to assist in its movement.
//-----------------------------------------------------------------------------

abstract_class IMoveHelper
{
public:
	// Call this to set the singleton
	static IMoveHelper* GetSingleton( ) { return sm_pSingleton; }
	
	// Methods associated with a particular entity
	virtual	char const*		GetName( EntityHandle_t handle ) const = 0;

	// sets the entity being moved
	virtual void	SetHost( CSharedBasePlayer *host ) = 0;

	// Adds the trace result to touch list, if contact is not already in list.
	virtual void	ResetTouchList( void ) = 0;
	virtual bool	AddToTouched( const CGameTrace& tr, const Vector& impactvelocity ) = 0;
	virtual void	ProcessImpacts( void ) = 0;
	
	// Numbered line printf
	virtual void	Con_NPrintf( int idx, PRINTF_FORMAT_STRING char const* fmt, ... ) = 0;

	// These have separate server vs client impementations
	virtual void	StartSound( const Vector& origin, int channel, char const* sample, float volume, soundlevel_t soundlevel, int fFlags, int pitch ) = 0;
	virtual void	StartSound( const Vector& origin, const char *soundname ) = 0; 
	virtual void	PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 ) = 0;

	// Apply falling damage to m_pHostPlayer based on m_pHostPlayer->m_flFallVelocity.
	virtual bool	PlayerFallingDamage( void ) = 0;

	virtual IPhysicsSurfaceProps *GetSurfaceProps( void ) = 0;

	virtual bool IsWorldEntity( const EHANDLE &handle ) = 0;

protected:
	// Inherited classes can call this to set the singleton
	static void SetSingleton( IMoveHelper* pMoveHelper ) { sm_pSingleton = pMoveHelper; }

	// Clients shouldn't call delete directly
	virtual			~IMoveHelper() {}

	// The global instance
	static IMoveHelper* sm_pSingleton;
};

//-----------------------------------------------------------------------------
// Add this to the CPP file that implements the IMoveHelper
//-----------------------------------------------------------------------------

#define IMPLEMENT_MOVEHELPER()	\
	IMoveHelper* IMoveHelper::sm_pSingleton = 0

//-----------------------------------------------------------------------------
// Call this to set the singleton
//-----------------------------------------------------------------------------

inline IMoveHelper* MoveHelper( )
{
	return IMoveHelper::GetSingleton();
}


#endif // IMOVEHELPER_H
