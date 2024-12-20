//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ITOOLENTITY_H
#define ITOOLENTITY_H
#pragma once

#ifndef SWDS

#include "tier1/interface.h"
#include "tier1/utlvector.h"
#include "Color.h"
#include "basehandle.h"
#include "iclientrenderable.h"
#include "engine/ishadowmgr.h"
#include "engine/ivmodelinfo.h"
#include "engine/IClientLeafSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IServerEntity;
class IClientEntity;
class IToolSystem;
class IClientRenderable;
class Vector;
class QAngle;
class ITempEntsSystem;
class IEntityFactoryDictionary;
class CGlobalEntityList;
class IEntityFindFilter;

#ifdef GAME_DLL
class CBaseEntity;
class CBaseAnimating;
class CTakeDamageInfo;
class CBaseTempEntity;
typedef CBaseEntity CServerBaseEntity;
typedef CBaseAnimating CServerBaseAnimating;
typedef CTakeDamageInfo CServerTakeDamageInfo;
typedef CBaseTempEntity CServerBaseTempEntity;
#else
class CServerBaseEntity;
class CServerBaseAnimating;
class CServerTakeDamageInfo;
class CServerBaseTempEntity;
#endif

//-----------------------------------------------------------------------------
// Safe accessor to an entity
//-----------------------------------------------------------------------------
enum HTOOLHANDLE : unsigned int;
inline const HTOOLHANDLE HTOOLHANDLE_INVALID = (HTOOLHANDLE)0;


//-----------------------------------------------------------------------------
// If you change this, change the flags in IClientShadowMgr.h also
//-----------------------------------------------------------------------------
enum ClientShadowFlags_t : unsigned int
{
	SHADOW_FLAGS_USE_RENDER_TO_TEXTURE	= (SHADOW_FLAGS_LAST_FLAG<<1),
	SHADOW_FLAGS_ANIMATING_SOURCE		= (SHADOW_FLAGS_LAST_FLAG<<2),
	SHADOW_FLAGS_USE_DEPTH_TEXTURE		= (SHADOW_FLAGS_LAST_FLAG<<3),
	SHADOW_FLAGS_CUSTOM_DRAW			= (SHADOW_FLAGS_LAST_FLAG<<4),
	CLIENT_SHADOW_FLAGS_LAST_FLAG		= SHADOW_FLAGS_CUSTOM_DRAW

	//if you add new flags you must update ShadowFlags_t from public/engine/ishadowmgr.h
};

FLAGENUM_OPERATORS( ClientShadowFlags_t, unsigned int )

//-----------------------------------------------------------------------------
// Opaque pointer returned from Find* methods, don't store this, you need to 
// Attach it to a tool entity or discard after searching
//-----------------------------------------------------------------------------
typedef void *EntitySearchResult;
typedef void *ParticleSystemSearchResult;

//-----------------------------------------------------------------------------
// Purpose: Client side tool interace (right now just handles IClientRenderables).
//  In theory could support hooking into client side entities directly
//-----------------------------------------------------------------------------
class IClientTools : public IBaseInterface
{
public:
	// Allocates or returns the handle to an entity previously found using the Find* APIs below
	virtual HTOOLHANDLE		AttachToEntity( EntitySearchResult entityToAttach ) = 0;
	virtual void			DetachFromEntity( EntitySearchResult entityToDetach ) = 0;

	// Checks whether a handle is still valid.
	virtual bool			IsValidHandle( HTOOLHANDLE handle ) = 0;

	// Iterates the list of entities which have been associated with tools
	virtual int				GetNumRecordables() = 0;
	virtual HTOOLHANDLE		GetRecordable( int index ) = 0;

	// Iterates through ALL entities (separate list for client vs. server)
	virtual EntitySearchResult	NextEntity( EntitySearchResult currentEnt ) = 0;
	EntitySearchResult			FirstEntity() { return NextEntity( NULL ); }

	// Use this to turn on/off the presence of an underlying game entity
	virtual void			SetEnabled( HTOOLHANDLE handle, bool enabled ) = 0;
	// Use this to tell an entity to post "state" to all listening tools
	virtual void			SetRecording( HTOOLHANDLE handle, bool recording ) = 0;
	// Some entities are marked with ShouldRecordInTools false, such as ui entities, etc.
	virtual bool			ShouldRecord( HTOOLHANDLE handle ) = 0;

	virtual HTOOLHANDLE		GetToolHandleForEntityByIndex( int entindex ) = 0;

	virtual modelindex_t				GetModelIndex( HTOOLHANDLE handle ) = 0;
	virtual const char*		GetModelName ( HTOOLHANDLE handle ) = 0;
	virtual const char*		GetClassname ( HTOOLHANDLE handle ) = 0;

public:
	virtual void			DO_NOT_USE_AddClientRenderable( IClientRenderable *pRenderable, int renderGroup ) = 0;
public:
	virtual void			RemoveClientRenderable( IClientRenderable *pRenderable ) = 0;
	virtual void			SetRenderGroup( IClientRenderable *pRenderable, int renderGroup ) = 0;
	virtual void			MarkClientRenderableDirty( IClientRenderable *pRenderable ) = 0;
    virtual void			UpdateProjectedTexture( ClientShadowHandle_t h, bool bForce ) = 0;

	virtual bool			DrawSprite( IClientRenderable *pRenderable, float scale, float frame, int rendermode, int renderfx, const Color &color, float flProxyRadius, int *pVisHandle ) = 0;

	virtual EntitySearchResult	GetLocalPlayer() = 0;
	virtual bool			GetLocalPlayerEyePosition( Vector& org, QAngle& ang, float &fov ) = 0;

	// See ClientShadowFlags_t above
	virtual ClientShadowHandle_t CreateShadow( CBaseHandle handle, int nFlags ) = 0;
	virtual void			DestroyShadow( ClientShadowHandle_t h ) = 0;

	virtual ClientShadowHandle_t CreateFlashlight( const FlashlightState_t &lightState ) = 0;
	virtual void			DestroyFlashlight( ClientShadowHandle_t h ) = 0;
	virtual void			UpdateFlashlightState( ClientShadowHandle_t h, const FlashlightState_t &lightState ) = 0;

	virtual void			AddToDirtyShadowList( ClientShadowHandle_t h, bool force = false ) = 0;
	virtual void			MarkRenderToTextureShadowDirty( ClientShadowHandle_t h ) = 0;

	// Global toggle for recording
	virtual void			EnableRecordingMode( bool bEnable ) = 0;
	virtual bool			IsInRecordingMode() const = 0;

	// Trigger a temp entity
	virtual void			TriggerTempEntity( KeyValues *pKeyValues ) = 0;

	// get owning weapon (for viewmodels)
	virtual int				GetOwningWeaponEntIndex( int entindex ) = 0;
	virtual int				GetEntIndex( EntitySearchResult entityToAttach ) = 0;

	virtual int				FindGlobalFlexcontroller( char const *name ) = 0;
	virtual char const		*GetGlobalFlexControllerName( int idx ) = 0;

	// helper for traversing ownership hierarchy
	virtual EntitySearchResult	GetOwnerEntity( EntitySearchResult currentEnt ) = 0;

	// common and useful types to query for hierarchically
	virtual bool			IsPlayer			 ( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsBaseCombatCharacter( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsNPC				 ( EntitySearchResult currentEnt ) = 0;

	virtual Vector			GetAbsOrigin( HTOOLHANDLE handle ) = 0;
	virtual QAngle			GetAbsAngles( HTOOLHANDLE handle ) = 0;

	// This reloads a portion or all of a particle definition file.
	// It's up to the client to decide if it cares about this file
	// Use a UtlBuffer to crack the data
	virtual void			ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen ) = 0;

	// Sends a mesage from the tool to the client
	virtual void			PostToolMessage( KeyValues *pKeyValues ) = 0;

	// Indicates whether the client should render particle systems
	virtual void			EnableParticleSystems( bool bEnable ) = 0;

	// Is the game rendering in 3rd person mode?
	virtual bool			IsRenderingThirdPerson() const = 0;
};

class IClientToolsEx : public IClientTools
{
private:
	virtual void			DO_NOT_USE_AddClientRenderable( IClientRenderable *pRenderable, int renderGroup ) override final;

private:
#ifdef _DEBUG
	virtual void			AddClientRenderable( IClientRenderable *pRenderable, int renderGroup ) final
	{
		DebuggerBreak();
	}
#endif

public:
	virtual void			AddClientRenderable( IClientRenderable *pRenderable, bool bDrawWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType = RENDERABLE_MODEL_UNKNOWN_TYPE ) = 0;
	virtual void			SetTranslucencyType( IClientRenderable *pRenderable, RenderableTranslucencyType_t nType ) = 0;

	bool			IsCombatCharacter	( EntitySearchResult currentEnt )
	{ return IsBaseCombatCharacter( currentEnt ); }

	virtual EntitySearchResult	GetEntity( HTOOLHANDLE handle ) = 0;
	virtual void			DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color ) = 0;
	virtual bool			IsRagdoll			( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsViewModel			( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsViewModelOrAttachment( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsWeapon			( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsSprite			( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsProp				( EntitySearchResult currentEnt ) = 0;
	virtual bool			IsBrush				( EntitySearchResult currentEnt ) = 0;

	// ParticleSystem iteration, query, modification
	virtual ParticleSystemSearchResult	FirstParticleSystem() { return NextParticleSystem( NULL ); }
	virtual ParticleSystemSearchResult	NextParticleSystem( ParticleSystemSearchResult sr ) = 0;
	virtual void						SetRecording( ParticleSystemSearchResult sr, bool bRecord ) = 0;
};

inline void IClientToolsEx::DO_NOT_USE_AddClientRenderable( IClientRenderable *pRenderable, int renderGroup )
{
	bool bDrawWithViewModels = false;
	RenderableTranslucencyType_t nType = RENDERABLE_IS_OPAQUE;
	RenderableModelType_t nModelType = RENDERABLE_MODEL_UNKNOWN_TYPE;

	switch(renderGroup) {
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_STATIC_TINY:
		nType = RENDERABLE_IS_OPAQUE;
		nModelType = RENDERABLE_MODEL_STATIC_PROP;
		break;
	case ENGINE_RENDER_GROUP_TRANSLUCENT_ENTITY:
		nType = RENDERABLE_IS_TRANSLUCENT;
		nModelType = RENDERABLE_MODEL_ENTITY;
		break;
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_HUGE:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_MEDIUM:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_SMALL:
	case ENGINE_RENDER_GROUP_OPAQUE_ENTITY_TINY:
		nType = RENDERABLE_IS_OPAQUE;
		nModelType = RENDERABLE_MODEL_ENTITY;
		break;
	case ENGINE_RENDER_GROUP_OPAQUE_BRUSH:
		nType = RENDERABLE_IS_OPAQUE;
		nModelType = RENDERABLE_MODEL_BRUSH;
		break;
	case ENGINE_RENDER_GROUP_VIEW_MODEL_TRANSLUCENT:
		nType = RENDERABLE_IS_TRANSLUCENT;
		nModelType = RENDERABLE_MODEL_ENTITY;
		bDrawWithViewModels = true;
		break;
	case ENGINE_RENDER_GROUP_VIEW_MODEL_OPAQUE:
		nType = RENDERABLE_IS_OPAQUE;
		nModelType = RENDERABLE_MODEL_ENTITY;
		bDrawWithViewModels = true;
		break;
	case ENGINE_RENDER_GROUP_TWOPASS:
		nType = RENDERABLE_IS_TWO_PASS;
		nModelType = RENDERABLE_MODEL_ENTITY;
		break;
	case ENGINE_RENDER_GROUP_OTHER:
		nType = RENDERABLE_IS_TRANSLUCENT;
		nModelType = RENDERABLE_MODEL_UNKNOWN_TYPE;
		break;
	}
	AddClientRenderable( pRenderable, bDrawWithViewModels, nType, nModelType );
}

#define VCLIENTTOOLS_INTERFACE_VERSION "VCLIENTTOOLS001"
#define VCLIENTTOOLS_EX_INTERFACE_VERSION "VCLIENTTOOLSEX001"

class CEntityRespawnInfo
{
public:
	int m_nHammerID;
	const char *m_pEntText;
};

//-----------------------------------------------------------------------------
// Purpose: Interface from engine to tools for manipulating entities
//-----------------------------------------------------------------------------
class IServerTools : public IBaseInterface
{
public:
	virtual IServerEntity *GetIServerEntity( IClientEntity *pClientEntity ) = 0;
	virtual bool SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer = NULL ) = 0;
	virtual bool GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer = NULL ) = 0;
	virtual bool SetPlayerFOV( int fov, IClientEntity *pClientPlayer = NULL ) = 0;
	virtual int GetPlayerFOV( IClientEntity *pClientPlayer = NULL ) = 0;
	virtual bool IsInNoClipMode( IClientEntity *pClientPlayer = NULL ) = 0;

	// entity searching
	virtual CServerBaseEntity *FirstEntity( void ) = 0;
	virtual CServerBaseEntity *NextEntity( CServerBaseEntity *pEntity ) = 0;
	virtual CServerBaseEntity *FindEntityByHammerID( int iHammerID ) = 0;

	// entity query
	virtual bool GetKeyValue( CServerBaseEntity *pEntity, const char *szField, char *szValue, int iMaxLen ) = 0;
	virtual bool SetKeyValue( CServerBaseEntity *pEntity, const char *szField, const char *szValue ) = 0;
	virtual bool SetKeyValue( CServerBaseEntity *pEntity, const char *szField, float flValue ) = 0;
	virtual bool SetKeyValue( CServerBaseEntity *pEntity, const char *szField, const Vector &vecValue ) = 0;

	// entity spawning
	virtual CServerBaseEntity *CreateEntityByName( const char *szClassName ) = 0;
	virtual void DispatchSpawn( CServerBaseEntity *pEntity ) = 0;

	// This reloads a portion or all of a particle definition file.
	// It's up to the server to decide if it cares about this file
	// Use a UtlBuffer to crack the data
	virtual void ReloadParticleDefintions( const char *pFileName, const void *pBufData, int nLen ) = 0;

	virtual void AddOriginToPVS( const Vector &org ) = 0;
	virtual void MoveEngineViewTo( const Vector &vPos, const QAngle &vAngles ) = 0;

	virtual bool DestroyEntityByHammerId( int iHammerID ) = 0;
	virtual CServerBaseEntity *GetBaseEntityByEntIndex( int iEntIndex ) = 0;
	virtual void RemoveEntity( CServerBaseEntity *pEntity ) = 0;
	virtual void RemoveEntityImmediate( CServerBaseEntity *pEntity ) = 0;
	virtual IEntityFactoryDictionary *GetEntityFactoryDictionary( void ) = 0;

	virtual void SetMoveType( CServerBaseEntity *pEntity, int val ) = 0;
	virtual void SetMoveType( CServerBaseEntity *pEntity, int val, int moveCollide ) = 0;
	virtual void ResetSequence( CServerBaseAnimating *pEntity, int nSequence ) = 0;
	virtual void ResetSequenceInfo( CServerBaseAnimating *pEntity ) = 0;

	virtual void ClearMultiDamage( void ) = 0;
	virtual void ApplyMultiDamage( void ) = 0;
	virtual void AddMultiDamage( const CServerTakeDamageInfo &pTakeDamageInfo, CServerBaseEntity *pEntity ) = 0;
	virtual void RadiusDamage( const CServerTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CServerBaseEntity *pEntityIgnore ) = 0;

	virtual ITempEntsSystem *GetTempEntsSystem( void ) = 0;
	virtual CServerBaseTempEntity *GetTempEntList( void ) = 0;

	virtual CGlobalEntityList *GetEntityList( void ) = 0;
	virtual bool IsEntityPtr( void *pTest ) = 0;
	virtual CServerBaseEntity *FindEntityByClassname( CServerBaseEntity *pStartEntity, const char *szName ) = 0;
	virtual CServerBaseEntity *FindEntityByName( CServerBaseEntity *pStartEntity, const char *szName, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL, IEntityFindFilter *pFilter = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityInSphere( CServerBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius ) = 0;
	virtual CServerBaseEntity *FindEntityByTarget( CServerBaseEntity *pStartEntity, const char *szName ) = 0;
	virtual CServerBaseEntity *FindEntityByModel( CServerBaseEntity *pStartEntity, const char *szModelName ) = 0;
	virtual CServerBaseEntity *FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityByNameWithin( CServerBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius ) = 0;
	virtual CServerBaseEntity *FindEntityByClassnameWithin( CServerBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius ) = 0;
	virtual CServerBaseEntity *FindEntityByClassnameWithin( CServerBaseEntity *pStartEntity, const char *szName, const Vector &vecMins, const Vector &vecMaxs ) = 0;
	virtual CServerBaseEntity *FindEntityGeneric( CServerBaseEntity *pStartEntity, const char *szName, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityGenericWithin( CServerBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
	virtual CServerBaseEntity *FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold ) = 0;
	virtual CServerBaseEntity *FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname ) = 0;
	virtual CServerBaseEntity *FindEntityProcedural( const char *szName, CServerBaseEntity *pSearchingEntity = NULL, CServerBaseEntity *pActivator = NULL, CServerBaseEntity *pCaller = NULL ) = 0;
};

class IServerToolsEx : public IServerTools
{
public:
	// This function respawns the entity into the same entindex slot AND tricks the EHANDLE system into thinking it's the same
	// entity version so anyone holding an EHANDLE to the entity points at the newly-respawned entity.
	virtual bool RespawnEntitiesWithEdits( CEntityRespawnInfo *pInfos, int nInfos ) = 0;

	// Call UTIL_Remove on the entity.
	virtual void RemoveEntity( int nHammerID ) = 0;
};

#define VSERVERTOOLS_INTERFACE_VERSION		"VSERVERTOOLS003"
#define VSERVERTOOLS_INTERFACE_VERSION_INT	3
#define VSERVERTOOLS_EX_INTERFACE_VERSION		"VSERVERTOOLSEX001"

//-----------------------------------------------------------------------------
// Purpose: Client side tool interace (right now just handles IClientRenderables).
//  In theory could support hooking into client side entities directly
//-----------------------------------------------------------------------------
class IServerChoreoTools : public IBaseInterface
{
public:

	// Iterates through ALL entities (separate list for client vs. server)
	virtual EntitySearchResult	NextChoreoEntity( EntitySearchResult currentEnt ) = 0;
	EntitySearchResult			FirstChoreoEntity() { return NextChoreoEntity( NULL ); } 
	virtual const char			*GetSceneFile( EntitySearchResult sr ) = 0;

	// For interactive editing
	virtual int					GetEntIndex( EntitySearchResult sr ) = 0;
	virtual void				ReloadSceneFromDisk( int entindex ) = 0;
};

#define VSERVERCHOREOTOOLS_INTERFACE_VERSION "VSERVERCHOREOTOOLS001"

#endif

#endif // ITOOLENTITY_H
