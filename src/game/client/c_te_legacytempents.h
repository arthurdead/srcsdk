//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( C_TE_LEGACYTEMPENTS_H )
#define C_TE_LEGACYTEMPENTS_H
#pragma once

class C_BaseEntity;
class C_LocalTempEntity;
struct model_t;

#include "mempool.h"
#include "utllinkedlist.h"
#include "icliententityinternal.h"
#include "particlemgr.h"

class C_BasePlayer;

enum
{
	SHELL_GENERIC = 0,
	SHELL_SHOTGUN,
	SHELL_RIFLE,

	SHELL_9MM,
	SHELL_57,
	SHELL_12GAUGE,
	SHELL_556,
	SHELL_762NATO,
	SHELL_338MAG
};

//-----------------------------------------------------------------------------
// Purpose: Interface for lecacy temp entities
//-----------------------------------------------------------------------------
abstract_class ITempEnts
{
public:
	virtual						~ITempEnts() {}

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;
	virtual void				LevelInit() = 0;
	virtual void				LevelShutdown() = 0;

	virtual void				Update( void ) = 0;
	virtual void				Clear( void ) = 0;

	virtual C_LocalTempEntity	*FindTempEntByID( int nID, int nSubID ) = 0;

	virtual void				BloodSprite( const Vector &org, int r, int g, int b, int a, int modelIndex, int modelIndex2, float size ) = 0;
	virtual void				RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale ) = 0;
	virtual void				MuzzleFlash( int type, ClientEntityHandle_t hEntity, int attachmentIndex, bool firstPerson ) = 0;
	virtual void				EjectBrass( const Vector& pos1, const QAngle& angles, const QAngle& gunAngles, int type, C_BasePlayer *pShooter ) = 0;
	virtual C_LocalTempEntity   *SpawnTempModel( const model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags ) = 0;
	virtual void				BreakModel( const Vector &pos, const QAngle &angles, const Vector &size, const Vector &dir, float random, float life, int count, int modelIndex, char flags) = 0;
	virtual void				Bubbles( const Vector &mins, const Vector &maxs, float height, int modelIndex, int count, float speed ) = 0;
	virtual void				BubbleTrail( const Vector &start, const Vector &end, float flWaterZ, int modelIndex, int count, float speed ) = 0;
	virtual void				Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags ) = 0;
	virtual void				FizzEffect( C_BaseEntity *pent, int modelIndex, int density, int current ) = 0;
	virtual C_LocalTempEntity	*DefaultSprite( const Vector &pos, int spriteIndex, float framerate ) = 0;
	virtual void				Sprite_Smoke( C_LocalTempEntity *pTemp, float scale ) = 0;
	virtual C_LocalTempEntity	*TempSprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal = vec3_origin ) = 0;
	virtual void				AttachTentToPlayer( int client, int modelIndex, float zoffset, float life ) = 0;
	virtual void				KillAttachedTents( int client ) = 0;
	virtual void				Sprite_Spray( const Vector &pos, const Vector &dir, int modelIndex, int count, int speed, int iRand ) = 0;
	virtual void				Sprite_Trail( const Vector &vecStart, const Vector &vecEnd, int modelIndex, int nCount, float flLife, float flSize, float flAmplitude, int nRenderamt, float flSpeed ) = 0;
	virtual void				RocketFlare( const Vector& pos ) = 0;
	
	virtual void				PlaySound ( C_LocalTempEntity *pTemp, float damp ) = 0;
	virtual void				PhysicsProp( int modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects = 0 ) = 0;
	virtual C_LocalTempEntity	*ClientProjectile( const Vector& vecOrigin, const Vector& vecVelocity, const Vector& vecAccel, int modelindex, int lifetime, C_BaseEntity *pOwner, const char *pszImpactEffect = NULL, const char *pszParticleEffect = NULL ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Default implementation of the temp entity interface
//-----------------------------------------------------------------------------
class CTempEnts : public ITempEnts
{
// Construction
public:
							CTempEnts( void );
	virtual					~CTempEnts( void );
// Exposed interface
public:
	virtual void			Init( void );
	virtual void			Shutdown( void );

	virtual void			LevelInit();

	virtual void			LevelShutdown();

	virtual void			Update( void );
	virtual void			Clear( void );

	virtual C_LocalTempEntity	*FindTempEntByID( int nID, int nSubID );

	// Legacy temp entities still supported
	virtual void			BloodSprite( const Vector &org, int r, int g, int b, int a, int modelIndex, int modelIndex2, float size );
	virtual void			RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale );

	virtual void			MuzzleFlash( int type, ClientEntityHandle_t hEntity, int attachmentIndex, bool firstPerson );
	
	virtual void			BreakModel(const Vector &pos, const QAngle &angles, const Vector &size, const Vector &dir, float random, float life, int count, int modelIndex, char flags);
	virtual void			Bubbles( const Vector &mins, const Vector &maxs, float height, int modelIndex, int count, float speed );
	virtual void			BubbleTrail( const Vector &start, const Vector &end, float height, int modelIndex, int count, float speed );
	virtual void			Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags );
	virtual void			FizzEffect( C_BaseEntity *pent, int modelIndex, int density, int current );
	virtual C_LocalTempEntity		*DefaultSprite( const Vector &pos, int spriteIndex, float framerate );
	virtual void			Sprite_Smoke( C_LocalTempEntity *pTemp, float scale );
	virtual C_LocalTempEntity		*TempSprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal = vec3_origin );
	virtual void			AttachTentToPlayer( int client, int modelIndex, float zoffset, float life );
	virtual void			KillAttachedTents( int client );
	virtual void			Sprite_Spray( const Vector &pos, const Vector &dir, int modelIndex, int count, int speed, int iRand );
	void					Sprite_Trail( const Vector &vecStart, const Vector &vecEnd, int modelIndex, int nCount, float flLife, float flSize, float flAmplitude, int nRenderamt, float flSpeed );

	virtual void			PlaySound ( C_LocalTempEntity *pTemp, float damp );
	virtual void			EjectBrass( const Vector &pos1, const QAngle &angles, const QAngle &gunAngles, int type, C_BasePlayer *pShooter );
	virtual C_LocalTempEntity		*SpawnTempModel( const model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags );
	void					RocketFlare( const Vector& pos );
	void					PhysicsProp( int modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects = 0 );
	C_LocalTempEntity		*ClientProjectile( const Vector& vecOrigin, const Vector& vecVelocity, const Vector& vecAcceleration, int modelindex, int lifetime, C_BaseEntity *pOwner, const char *pszImpactEffect = NULL, const char *pszParticleEffect = NULL );

// Data
public:
	enum
	{ 
		MAX_TEMP_ENTITIES = 500,
		MAX_TEMP_ENTITY_SPRITES = 200,
		MAX_TEMP_ENTITY_STUDIOMODEL = 50,
	};

private:
	// Global temp entity pool
	CClassMemoryPool< C_LocalTempEntity >	m_TempEntsPool;
	CUtlLinkedList< C_LocalTempEntity *, unsigned short >	m_TempEnts;

	// Muzzle flash sprites
	struct model_t			*m_pSpriteMuzzleFlash;

	struct model_t			*m_pShells[9];

// Internal methods also available to children
protected:
	C_LocalTempEntity		*TempEntAlloc( const Vector& org, const model_t *model, const char *classname );
	C_LocalTempEntity		*TempEntAllocHigh( const Vector& org, const model_t *model, const char *classname );

// Material handle caches
private:

	inline void				CacheMuzzleFlashes( void );
	PMaterialHandle			m_Material_MuzzleFlash[4][2];

// Internal methods
private:
	CTempEnts( const CTempEnts & );

	void					TempEntFree( int index );
	C_LocalTempEntity		*TempEntAlloc( const char *classname );	
	bool					FreeLowPriorityTempEnt();

	bool						IsVisibleTempEntity( C_LocalTempEntity *pEntity );

	void					MuzzleFlash_Shotgun_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex );
	void					MuzzleFlash_Shotgun_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex );

	void					MuzzleFlash_Pistol_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex );
	void					MuzzleFlash_Pistol_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex );

	void					MuzzleFlash_357_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex );

	void					MuzzleFlash_RPG_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex );
};


extern ITempEnts *tempents;


#endif // C_TE_LEGACYTEMPENTS_H
