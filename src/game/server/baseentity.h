//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEENTITY_H
#define BASEENTITY_H
#pragma once

#include "entitylist.h"
#include "entityoutput.h"
#include "networkvar.h"
//#include "collisionproperty.h"
#include "ServerNetworkProperty.h"
#include "shareddefs.h"
#include "engine/ivmodelinfo.h"
#include "predictableid.h"
#include "engine/IEngineTrace.h"
#include "takedamageinfo.h"
#include "touchlink.h"
#include "groundlink.h"
#include "irecipientfilter.h"
#include "map_entity.h"
#include "vphysics_interface.h"
//#include "density_weight_map.h"
#include "videocfg/videocfg.h"
#include "imovehelper.h"

class CDamageModifier;
class CDmgAccumulator;

struct CSoundParameters;

#ifndef AI_CriteriaSet
#define AI_CriteriaSet ResponseRules::CriteriaSet 
#endif
namespace ResponseRules 
{ 
	class CriteriaSet; 
	class IResponseSystem;
};
using ResponseRules::IResponseSystem;
class CRecipientFilter;
class CStudioHdr;

// Matching the high level concept is significantly better than other criteria
// FIXME:  Could do this in the script file by making it required and bumping up weighting there instead...
#define CONCEPT_WEIGHT 5.0f

typedef CHandle<CBaseEntity> EHANDLE;

#define MANUALMODE_GETSET_PROP(type, accessorName, varName) \
	private:\
		type varName;\
	public:\
		inline const type& Get##accessorName##() const { return varName; } \
		inline type& Get##accessorName##() { return varName; } \
		inline void Set##accessorName##( const type &val ) { varName = val; m_NetStateMgr.StateChanged(); }

#define MANUALMODE_GETSET_EHANDLE(type, accessorName, varName) \
	private:\
		CHandle<type> varName;\
	public:\
		inline type* Get##accessorName##() { return varName.Get(); } \
		inline void Set##accessorName##( type *pType ) { varName = pType; m_NetStateMgr.StateChanged(); }


// saverestore.h declarations
struct typedescription_t;
class CBaseEntity;
class CEntityMapData;
class CBaseCombatWeapon;
class IPhysicsObject;
class IPhysicsShadowController;
class CBaseCombatCharacter;
class CTeam;
class Vector;
struct gamevcollisionevent_t;
class CBaseAnimating;
class CBaseAnimatingOverlay;
class CBasePlayer;
class IServerVehicle;
struct solid_t;
struct notify_system_event_params_t;
class CAI_BaseNPC;
class CAI_Senses;
class CSquadNPC;
class variant_t;
class CEventAction;
typedef struct KeyValueData_s KeyValueData;
class CUserCmd;
class CSkyCamera;
class CEntityMapData;
class CWorld;
typedef unsigned int UtlHashHandle_t;
class CGlobalEvent;
class CCollisionProperty;
class DensityWeightsMap;
enum density_type_t : unsigned char;

typedef CUtlVector< CBaseEntity* > EntityList_t;

// Serializable list of context as set by entity i/o and used for deducing proper
//  speech state, et al.
struct ResponseContext_t
{
	string_t		m_iszName;
	string_t		m_iszValue;
	float			m_fExpirationTime;		// when to expire context (0 == never)
};


//-----------------------------------------------------------------------------
// Entity events... targetted to a particular entity
// Each event has a well defined structure to use for parameters
//-----------------------------------------------------------------------------
enum EntityEvent_t : unsigned char
{
	ENTITY_EVENT_WATER_TOUCH = 0,		// No data needed
	ENTITY_EVENT_WATER_UNTOUCH,			// No data needed
	ENTITY_EVENT_PARENT_CHANGED,		// No data needed
};


//-----------------------------------------------------------------------------

typedef void (CBaseEntity::*BASEPTR)(void);
typedef void (CBaseEntity::*ENTITYFUNCPTR)(CBaseEntity *pOther );
typedef void (CBaseEntity::*USEPTR)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#define DEFINE_THINKFUNC( function ) DEFINE_FUNCTION_RAW( function, BASEPTR )
#define DEFINE_ENTITYFUNC( function ) DEFINE_FUNCTION_RAW( function, ENTITYFUNCPTR )
#define DEFINE_USEFUNC( function ) DEFINE_FUNCTION_RAW( function, USEPTR )

// Things that toggle (buttons/triggers/doors) need this
enum TOGGLE_STATE : unsigned char
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
};

// Debug overlay bits
enum DebugOverlayBits_t : uint64
{
	OVERLAY_NONE = 0,

#if !defined SWDS || 1
	OVERLAY_TEXT_BIT			=	0x00000001,		// show text debug overlay for this entity
	OVERLAY_NAME_BIT			=	0x00000002,		// show name debug overlay for this entity
	OVERLAY_BBOX_BIT			=	0x00000004,		// show bounding box overlay for this entity
	OVERLAY_PIVOT_BIT			=	0x00000008,		// show pivot for this entity
	OVERLAY_MESSAGE_BIT			=	0x00000010,		// show messages for this entity
	OVERLAY_ABSBOX_BIT			=	0x00000020,		// show abs bounding box overlay
	OVERLAY_RBOX_BIT			=   0x00000040,     // show the rbox overlay
	OVERLAY_SHOW_BLOCKSLOS		=	0x00000080,		// show entities that block NPC LOS
	OVERLAY_ATTACHMENTS_BIT		=	0x00000100,		// show attachment points
	OVERLAY_AUTOAIM_BIT			=	0x00000200,		// Display autoaim radius
#endif

	OVERLAY_NPC_SELECTED_BIT	=	0x00001000,		// the npc is current selected
#if !defined SWDS || 1
	OVERLAY_NPC_NEAREST_BIT		=	0x00002000,		// show the nearest node of this npc
	OVERLAY_NPC_ROUTE_BIT		=	0x00004000,		// draw the route for this npc
	OVERLAY_NPC_TRIANGULATE_BIT =	0x00008000,		// draw the triangulation for this npc
#endif
	OVERLAY_NPC_ZAP_BIT			=	0x00010000,		// destroy the NPC
#if !defined SWDS || 1
	OVERLAY_NPC_ENEMIES_BIT		=	0x00020000,		// show npc's enemies
	OVERLAY_NPC_CONDITIONS_BIT	=	0x00040000,		// show NPC's current conditions
	OVERLAY_NPC_SQUAD_BIT		=	0x00080000,		// show npc squads
	OVERLAY_NPC_TASK_BIT		=	0x00100000,		// show npc task details
	OVERLAY_NPC_FOCUS_BIT		=	0x00200000,		// show line to npc's enemy and target
	OVERLAY_NPC_VIEWCONE_BIT	=	0x00400000,		// show npc's viewcone
#endif
	OVERLAY_NPC_KILL_BIT		=	0x00800000,		// kill the NPC, running all appropriate AI.

	OVERLAY_WC_CHANGE_ENTITY	=	0x01000000,		// object changed during WC edit
	OVERLAY_BUDDHA_MODE			=	0x02000000,		// take damage but don't die

#if !defined SWDS || 1
	OVERLAY_NPC_STEERING_REGULATIONS	=	0x04000000,	// Show the steering regulations associated with the NPC

	OVERLAY_TASK_TEXT_BIT		=	0x08000000,		// show task and schedule names when they start

	OVERLAY_PROP_DEBUG			=	0x10000000,

	OVERLAY_NPC_RELATION_BIT	=	0x20000000,		// show relationships between target and all children

	OVERLAY_VIEWOFFSET			=	0x40000000,		// show view offset
#endif
};

FLAGENUM_OPERATORS( DebugOverlayBits_t, uint64 )

#if !defined SWDS || 1
struct TimedOverlay_t;
#endif

DECLARE_LOGGING_CHANNEL( LOG_BASEENTITY );

/* =========  CBaseEntity  ======== 

  All objects in the game are derived from this.

a list of all CBaseEntitys is kept in gEntList
================================ */

// creates an entity by string name, but does not spawn it
// If iForceEdictIndex is not -1, then it will use the edict by that index. If the index is 
// invalid or there is already an edict using that index, it will error out.
//CBaseEntity *CreateEntityByName( const char *className, int iForceEdictIndex = -1, bool bNotify = true );

// creates an entity and calls all the necessary spawn functions
extern void SpawnEntityByName( const char *className, CEntityMapData *mapData = NULL );

// calls the spawn functions for an entity
extern int DispatchSpawn( CBaseEntity *pEntity );

inline CBaseEntity *GetContainingEntity( edict_t *pent );

//-----------------------------------------------------------------------------
// Purpose: think contexts
//-----------------------------------------------------------------------------
struct thinkfunc_t
{
	BASEPTR		m_pfnThink;
	string_t	m_iszContext;
	int			m_nNextThinkTick;
	int			m_nLastThinkTick;
};

struct EmitSound_t;
struct rotatingpushmove_t;

#ifdef DT_CELL_COORD_SUPPORTED
const DTFlags_t SENDPROP_VECORIGIN_FLAGS = SPROP_CELL_COORD;
#else
const DTFlags_t SENDPROP_VECORIGIN_FLAGS = SPROP_COORD_MP;
#endif

#ifdef DT_CELL_COORD_SUPPORTED
#define SENDPROP_VECORIGIN_BITS CELL_BASEENTITY_ORIGIN_CELL_BITS
#else
#define SENDPROP_VECORIGIN_BITS -1
#endif

#ifdef DT_CELL_COORD_SUPPORTED
#define SENDPROP_VECORIGIN_PROXY CBaseEntity::SendProxy_CellOrigin
#else
#define SENDPROP_VECORIGIN_PROXY SendProxy_Origin
#endif

#define CREATE_PREDICTED_ENTITY( className, ... )	\
	CBaseEntity::CreatePredicted( __FILE__, __LINE__, className __VA_OPT__(, __VA_ARGS__) );

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity : public IServerEntity, public INetworkableObject
{
public:
	DECLARE_CLASS_NOBASE( CBaseEntity );	

	//----------------------------------------
	// Class vars and functions
	//----------------------------------------
	static inline void Debug_Pause(bool bPause);
	static inline bool Debug_IsPaused(void);
	static inline void Debug_SetSteps(int nSteps);
	static inline bool Debug_ShouldStep(void);
	static inline bool Debug_Step(void);

	static bool				m_bInDebugSelect;
	static int				m_nDebugPlayer;

protected:

	static bool				m_bDebugPause;		// Whether entity i/o is paused for debugging.
	static int				m_nDebugSteps;		// Number of entity outputs to fire before pausing again.

	static bool				sm_bDisableTouchFuncs;	// Disables PhysicsTouch and PhysicsStartTouch function calls
public:
	static bool				sm_bAccurateTriggerBboxChecks;	// SOLID_BBOX entities do a fully accurate trigger vs bbox check when this is set

public:
	// If bServerOnly is true, then the ent never goes to the client. This is used
	// by logical entities.
	CBaseEntity() : CBaseEntity(EFL_NONE) {}
	CBaseEntity( EntityFlags_t iEFlags );
	virtual ~CBaseEntity();

	// prediction system
	DECLARE_PREDICTABLE();
	// network data
	DECLARE_SERVERCLASS();
	// data description
	DECLARE_MAPENTITY();

	bool IsNetworked() const { return !IsEFlagSet(EFL_NOT_NETWORKED); }
	
	// memory handling
    void *operator new( size_t stAllocateBlock );
    void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	void operator delete( void *pMem );
	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }

// IHandleEntity overrides.
public:
	virtual void			SetRefEHandle( const CBaseHandle &handle );
	virtual const			EHANDLE& GetRefEHandle() const;

// IServerUnknown overrides
	virtual ICollideable	*GetCollideable();
	virtual IServerNetworkable *GetNetworkable();
	virtual CBaseEntity		*GetBaseEntity();

// IServerEntity overrides.
public:
	virtual void			SetModelIndex( modelindex_t index );
	virtual modelindex_t				GetModelIndex( void ) const;
 	virtual string_t		GetModelName( void ) const;

 	virtual string_t		GetAIAddOn( void ) const;

	void					ClearModelIndexOverrides( void );
	virtual void			AddModelIndexOverride( vision_filter_t flags, modelindex_t nValue );

public:
	// virtual methods for derived classes to override
	virtual bool			TestCollision( const Ray_t& ray, ContentsFlags_t mask, trace_t& trace );
	virtual	bool			TestHitboxes( const Ray_t &ray, ContentsFlags_t fContentsMask, trace_t& tr );
	virtual void			ComputeWorldSpaceSurroundingBox( Vector *pWorldMins, Vector *pWorldMaxs );

	// non-virtual methods. Don't override these!
public:
	// An inline version the game code can use
	virtual CCollisionProperty		*CollisionProp();
	virtual const CCollisionProperty*CollisionProp() const;
	virtual CServerNetworkProperty *NetworkProp();
	virtual const CServerNetworkProperty *NetworkProp() const;

	bool					IsCurrentlyTouching( void ) const;
	const Vector&			GetAbsOrigin( void ) const;
	const QAngle&			GetAbsAngles( void ) const;
	inline Vector			Forward() const RESTRICT; ///< get my forward (+x) vector
	inline Vector			Left() const RESTRICT;    ///< get my left    (+y) vector
	inline Vector			Up() const RESTRICT;      ///< get my up      (+z) vector

	SolidType_t				GetSolid() const;
	SolidFlags_t			 			GetSolidFlags( void ) const;

	EntityFlags_t						GetEFlags() const;
	void					AddEFlags( EntityFlags_t nEFlagMask );
	void					RemoveEFlags( EntityFlags_t nEFlagMask );
	bool					IsEFlagSet( EntityFlags_t nEFlagMask ) const;

	// Quick way to ask if we have a player entity as a child anywhere in our hierarchy.
	void					RecalcHasPlayerChildBit();
	bool					DoesHavePlayerChild();

	bool					IsTransparent() const;

	bool					AllowNavIgnore();
	void					SetAllowNavIgnore( bool bAllowNavIgnore );
	void					SetNavIgnore( float duration = FLT_MAX );
	void					ClearNavIgnore();
	bool					IsNavIgnored() const;
	void					SetAlwaysNavIgnore( bool bAlwaysNavIgnore );
	bool					AlwaysNavIgnore();

	// Is the entity floating?
	bool					IsFloating();

	// Called by physics to see if we should avoid a collision test....
	virtual	bool			ShouldCollide( Collision_Group_t collisionGroup, ContentsFlags_t contentsMask ) const;

	// Move type / move collide
	MoveType_t				GetMoveType() const;
	MoveCollide_t			GetMoveCollide() const;
	void					SetMoveType( MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );
	void					SetMoveCollide( MoveCollide_t val );

	// Returns the entity-to-world transform
	matrix3x4_t				&EntityToWorldTransform();
	const matrix3x4_t		&EntityToWorldTransform() const;

	// Some helper methods that transform a point from entity space to world space + back
	void					EntityToWorldSpace( const Vector &in, Vector *pOut ) const;
	void					WorldToEntitySpace( const Vector &in, Vector *pOut ) const;

	// This function gets your parent's transform. If you're parented to an attachment,
	// this calculates the attachment's transform and gives you that.
	//
	// You must pass in tempMatrix for scratch space - it may need to fill that in and return it instead of 
	// pointing you right at a variable in your parent.
	matrix3x4_t&			GetParentToWorldTransform( matrix3x4_t &tempMatrix );

	// Externalized data objects ( see sharreddefs.h for DataObjectType_t )
	bool					HasDataObjectType( DataObject_t type ) const;
	void					AddDataObjectType( DataObject_t type );
	void					RemoveDataObjectType( DataObject_t type );

	void					*GetDataObject( DataObject_t type );
	void					*CreateDataObject( DataObject_t type );
	void					DestroyDataObject( DataObject_t type );
	void					DestroyAllDataObjects( void );

public:
	void SetScaledPhysics( IPhysicsObject *pNewObject );

	// virtual methods; you can override these
public:
	// Owner entity.
	// FIXME: These are virtual only because of CNodeEnt
	CBaseEntity				*GetOwnerEntity() const;
	virtual void			SetOwnerEntity( CBaseEntity* pOwner );
	void					SetEffectEntity( CBaseEntity *pEffectEnt );
	CBaseEntity				*GetEffectEntity() const;

	// Only CBaseEntity implements these. CheckTransmit calls the virtual ShouldTransmit to see if the
	// entity wants to be sent. If so, it calls SetTransmit, which will mark any dependents for transmission too.
	virtual EdictStateFlags_t				ShouldTransmit( const CCheckTransmitInfo *pInfo );

	// update the global transmit state if a transmission rule changed
    EdictStateFlags_t				SetTransmitState( EdictStateFlags_t nFlag);
	EdictStateFlags_t				GetTransmitState( void );
	EdictStateFlags_t						DispatchUpdateTransmitState();
	
	// Do NOT call this directly. Use DispatchUpdateTransmitState.
	virtual EdictStateFlags_t				UpdateTransmitState();
	
	// Entities (like ropes) use this to own the transmit state of another entity
	// by forcing it to not call UpdateTransmitState.
	void					IncrementTransmitStateOwnedCounter();
	void					DecrementTransmitStateOwnedCounter();

	// This marks the entity for transmission and passes the SetTransmit call to any dependents.
	virtual void			SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

	// This function finds out if the entity is in the 3D skybox. If so, it sets the EFL_IN_SKYBOX
	// flag so the entity gets transmitted to all the clients.
	// Entities usually call this during their Activate().
	// Returns true if the entity is in the skybox (and EFL_IN_SKYBOX was set).
	bool					DetectInSkybox();

	// Returns which skybox the entity is in
	CSkyCamera				*GetEntitySkybox();

	bool					IsSimulatedEveryTick() const;
	bool					IsAnimatedEveryTick() const;
	void					SetSimulatedEveryTick( bool sim );
	void					SetAnimatedEveryTick( bool anim );

public:

	virtual const char	*GetTracerType( void );

	// returns a pointer to the entities edict, if it has one.  should be removed!
	virtual edict_t			*edict( void );
	virtual const edict_t	*edict( void ) const;
	virtual int				entindex( ) const;
	int				entserial( ) const;
	int				GetSoundSourceIndex() const;

	void	SetFadeDistance( float minFadeDist, float maxFadeDist );
	float							GetMinFadeDist( ) const;
	float							GetMaxFadeDist( ) const;
	void	SetGlobalFadeScale( float flFadeScale );
	float	GetGlobalFadeScale() const;

	// These methods encapsulate MOVETYPE_FOLLOW, which became obsolete
	void FollowEntity( CBaseEntity *pBaseEntity, bool bBoneMerge = true );
	void StopFollowingEntity( );	// will also change to MOVETYPE_NONE
	bool IsFollowingEntity();
	CBaseEntity *GetFollowedEntity();

	// initialization
	virtual void Spawn( void );
	virtual void Precache( void ) {}

	virtual void SetModel( const char *szModelName );

protected:
	// Notification on model load. May be called multiple times for dynamic models.
	// Implementations must call BaseClass::OnNewModel and pass return value through.
	virtual CStudioHdr *OnNewModel();

public:
	virtual void PostConstructor( const char *szClassname );
	virtual void PostClientActive( void );
	virtual void ParseMapData( CEntityMapData *mapData );
	virtual void OnParseMapDataFinished();
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool KeyValue( const char *szKeyName, float flValue );
	virtual bool KeyValue( const char *szKeyName, int nValue );
	virtual bool KeyValue( const char *szKeyName, const Vector &vecValue );
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );
	bool KeyValueFromString( const char *szKeyName, const char *szValue )		{ return KeyValue( szKeyName, szValue ); }
	bool KeyValueFromFloat( const char *szKeyName, float flValue )				{ return KeyValue( szKeyName, flValue ); }
	bool KeyValueFromInt( const char *szKeyName, int nValue )					{ return KeyValue( szKeyName, nValue ); }
	bool KeyValueFromVector( const char *szKeyName, const Vector &vecValue )	{ return KeyValue( szKeyName, vecValue ); }

	void ValidateEntityConnections();
	void FireNamedOutput( const char *pszOutput, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller, float flDelay = 0.0f );
	CBaseEntityOutput *FindNamedOutput( const char *pszOutput );
	float GetMaxOutputDelay( const char *pszOutput );
	//void CancelEventsByInput( const char *szInput );

	// Activate - called for each entity after each load game and level load
	virtual void Activate( void );

	// Called once per frame after the server frame loop has finished and after all messages being
	//  sent to clients have been sent.
	// NOTE: This will not be called unless the entity requests it via gEntList.AddPostClientMessageEntity
	void PostClientMessagesSent( void );

	// Hierarchy traversal
	CBaseEntity *GetMoveParent( void );
	CBaseEntity *GetRootMoveParent();
	CBaseEntity *FirstMoveChild( void );
	CBaseEntity *NextMovePeer( void );

	void		SetName( string_t newTarget );
	void		SetNameAsCStr( const char *newTarget );
	void		SetParent( string_t newParent, CBaseEntity *pActivator, int iAttachment = -1 );
	
	// Set the movement parent. Your local origin and angles will become relative to this parent.
	// If iAttachment is a valid attachment on the parent, then your local origin and angles 
	// are relative to the attachment on this entity. If iAttachment == -1, it'll preserve the
	// current m_iParentAttachment.
	virtual void	SetParent( CBaseEntity* pNewParent, int iAttachment = -1 );
	CBaseEntity* GetParent();
	int			GetParentAttachment();

	string_t	GetEntityName();
	string_t	GetEntityNameAsTStr();
	const char *GetEntityNameAsCStr();	// This method is temporary for VSCRIPT functionality until we figure out what to do with string_t (sjb)
	const char *GetPreTemplateName(); // Not threadsafe. Get the name stripped of template unique decoration

	bool		NameMatches( const char *pszNameOrWildcard );
	bool		ClassMatches( const char *pszClassOrWildcard );
	bool		NameMatches( string_t nameStr );
	bool		ClassMatches( string_t nameStr );
	bool		NameMatchesExact( string_t nameStr );
	bool		ClassMatchesExact( string_t nameStr );
	bool		ClassMatchesExact( const char *nameStr );
	bool		ClassMatchesExact( const CGameString &nameStr );

	template <typename T>
	bool		Downcast( string_t iszClass, T **ppResult );

private:
	bool		NameMatchesComplex( const char *pszNameOrWildcard );
	bool		ClassMatchesComplex( const char *pszClassOrWildcard );
	void		TransformStepData_WorldToParent( CBaseEntity *pParent );
	void		TransformStepData_ParentToParent( CBaseEntity *pOldParent, CBaseEntity *pNewParent );
	void		TransformStepData_ParentToWorld( CBaseEntity *pParent );


public:
	#define DECLARE_SPAWNFLAGS( type ) \
		type GetSpawnFlags( void ) const \
		{ return static_cast<type>(m_spawnflags.Get());  } \
		void AddSpawnFlags( type nFlags ) \
		{ m_spawnflags |= static_cast<uint64>(nFlags); } \
		void SetSpawnFlags( type nFlags ) \
		{ m_spawnflags = static_cast<uint64>(nFlags); } \
		void RemoveSpawnFlags( type nFlags ) \
		{ m_spawnflags &= ~static_cast<uint64>(nFlags); } \
		void ClearSpawnFlags( void ) \
		{ m_spawnflags = 0; } \
		bool HasSpawnFlags( type nFlags ) const \
		{ return (m_spawnflags & static_cast<uint64>(nFlags)) != 0; }

	Effects_t			GetEffects( void ) const;
	void		AddEffects( Effects_t nEffects );
	void		RemoveEffects( Effects_t nEffects );
	void		ClearEffects( void );
	void		SetEffects( Effects_t nEffects );
	bool		IsEffectActive( Effects_t nEffects ) const;

	// makes the entity inactive
	void		MakeDormant( void );
	int			IsDormant( void );

	void		RemoveDeferred( void );	// Sets the entity invisible, and makes it remove itself on the next frame

	// checks to see if the entity is marked for deletion
	bool		IsMarkedForDeletion( void );
	void MarkForDeletion();

	// capabilities
	virtual EntityCaps_t	ObjectCaps( void );

	// Verifies that the data description is valid in debug builds.
	#ifdef _DEBUG
	void ValidateDataDescription(void);
	#endif // _DEBUG

	// handles an input (usually caused by outputs)
	// returns true if the the value in the pass in should be set, false if the input is to be ignored
	virtual bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &&Value, int outputID );
	bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, const variant_t &Value, int outputID );

	//
	// Input handlers.
	//
	void InputAlternativeSorting( inputdata_t &&inputdata );
	void InputAlpha( inputdata_t &&inputdata );
	void InputColor( inputdata_t &&inputdata );
	void InputSetParent( inputdata_t &&inputdata );
	void SetParentAttachment( const char *szInputName, const char *szAttachment, bool bMaintainOffset );
	void InputSetParentAttachment( inputdata_t &&inputdata );
	void InputSetParentAttachmentMaintainOffset( inputdata_t &&inputdata );
	void InputClearParent( inputdata_t &&inputdata );
	void InputSetTeam( inputdata_t &&inputdata );
	void InputUse( inputdata_t &&inputdata );
	void InputKill( inputdata_t &&inputdata );
	void InputKillHierarchy( inputdata_t &&inputdata );
	void InputSetDamageFilter( inputdata_t &&inputdata );
	void InputDispatchEffect( inputdata_t &&inputdata );
	void InputEnableDamageForces( inputdata_t &&inputdata );
	void InputDisableDamageForces( inputdata_t &&inputdata );
	void InputAddContext( inputdata_t &&inputdata );
	void InputRemoveContext( inputdata_t &&inputdata );
	void InputClearContext( inputdata_t &&inputdata );
	void InputDispatchResponse( inputdata_t &&inputdata );
	void InputDisableShadow( inputdata_t &&inputdata );
	void InputEnableShadow( inputdata_t &&inputdata );
	void InputDisableDraw( inputdata_t &&inputdata );
	void InputEnableDraw( inputdata_t &&inputdata );
	void InputDisableReceivingFlashlight( inputdata_t &&inputdata );
	void InputEnableReceivingFlashlight( inputdata_t &&inputdata );
	void InputAddOutput( inputdata_t &&inputdata );
	void InputFireUser1( inputdata_t &&inputdata );
	void InputFireUser2( inputdata_t &&inputdata );
	void InputFireUser3( inputdata_t &&inputdata );
	void InputFireUser4( inputdata_t &&inputdata );
	void InputChangeVariable( inputdata_t &&inputdata );

	void InputPassUser1( inputdata_t &&inputdata );
	void InputPassUser2( inputdata_t &&inputdata );
	void InputPassUser3( inputdata_t &&inputdata );
	void InputPassUser4( inputdata_t &&inputdata );

	void InputFireRandomUser( inputdata_t &&inputdata );
	void InputPassRandomUser( inputdata_t &&inputdata );

	void InputSetEntityName( inputdata_t &&inputdata );

	virtual void InputSetTarget( inputdata_t &&inputdata );
	virtual void InputSetOwnerEntity( inputdata_t &&inputdata );

	virtual void InputAddHealth( inputdata_t &&inputdata );
	virtual void InputRemoveHealth( inputdata_t &&inputdata );
	virtual void InputSetHealth( inputdata_t &&inputdata );

	virtual void InputSetMaxHealth( inputdata_t &&inputdata );

	void InputFireOutput( inputdata_t &&inputdata );
	void InputRemoveOutput( inputdata_t &&inputdata );
	//virtual void InputCancelOutput( inputdata_t &&inputdata ); // Find a way to implement this
	void InputReplaceOutput( inputdata_t &&inputdata );
	void InputAcceptInput( inputdata_t &&inputdata );
	virtual void InputCancelPending( inputdata_t &&inputdata );

	void InputFreeChildren( inputdata_t &&inputdata );

	void InputSetLocalOrigin( inputdata_t &&inputdata );
	void InputSetLocalAngles( inputdata_t &&inputdata );
	void InputSetAbsOrigin( inputdata_t &&inputdata );
	void InputSetAbsAngles( inputdata_t &&inputdata );
	void InputSetLocalVelocity( inputdata_t &&inputdata );
	void InputSetLocalAngularVelocity( inputdata_t &&inputdata );

	void InputAddSpawnFlags( inputdata_t &&inputdata );
	void InputRemoveSpawnFlags( inputdata_t &&inputdata );
	void InputSetRenderMode( inputdata_t &&inputdata );
	void InputSetRenderFX( inputdata_t &&inputdata );
	void InputSetViewHideFlags( inputdata_t &&inputdata );
	void InputAddEffects( inputdata_t &&inputdata );
	void InputRemoveEffects( inputdata_t &&inputdata );
	void InputDrawEntity( inputdata_t &&inputdata );
	void InputUndrawEntity( inputdata_t &&inputdata );
	void InputAddEFlags( inputdata_t &&inputdata );
	void InputRemoveEFlags( inputdata_t &&inputdata );
	void InputAddSolidFlags( inputdata_t &&inputdata );
	void InputRemoveSolidFlags( inputdata_t &&inputdata );
	void InputSetMoveType( inputdata_t &&inputdata );
	void InputSetCollisionGroup( inputdata_t &&inputdata );

	void InputTouch( inputdata_t &&inputdata );

	virtual void InputKilledNPC( inputdata_t &&inputdata );

	void InputKillIfNotVisible( inputdata_t &&inputdata );
	void InputKillWhenNotVisible( inputdata_t &&inputdata );

	void InputSetThinkNull( inputdata_t &&inputdata );

	// Returns the origin at which to play an inputted dispatcheffect 
	virtual void GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles );

	// tries to read a field from the entities data description - result is placed in variant_t
	bool ReadKeyField( const char *varName, variant_t *var );

	// classname access
	void		SetClassname( const char *className );
	void		SetClassname( string_t className );
	const char* GetClassname();
	string_t GetClassnameStr();

	virtual const char *GetPlayerName() const { return NULL; }

	const char	*GetDebugName(void); // do not make this virtual -- designed to handle NULL this

#if !defined SWDS || 1
	// Debug Overlays
	void		 EntityText( int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255 );
	void         DrawVPhysicsObjectCenterAndContactPoints(IPhysicsObject *obj);
	virtual	void DrawDebugGeometryOverlays(void);					
	virtual int  DrawDebugTextOverlays(void);
	void		 DrawTimedOverlays( void );
	void		 DrawBBoxOverlay( float flDuration = 0.0f );
	void		 DrawAbsBoxOverlay();
	void		 DrawRBoxOverlay();
	void		 SendDebugPivotOverlay( void );
	void		 AddTimedOverlay( const char *msg, int endTime );
#endif

	void		 DrawInputOverlay(const char *szInputName, CBaseEntity *pCaller, variant_t Value);
	void		 DrawOutputOverlay(CEventAction *ev);

	void		SetSolid( SolidType_t val );

	int			 GetTextureFrameIndex( void );
	void		 SetTextureFrameIndex( int iIndex );

	// Entities block Line-Of-Sight for NPCs by default.
	// Set this to false if you want to change this behavior.
	void		 SetBlocksLOS( bool bBlocksLOS );
	bool		 BlocksLOS( void );


	void		 SetAIWalkable( bool bBlocksLOS );
	bool		 IsAIWalkable( void );

	// This comes from the "id" key/value that Hammer adds to entities.
	// It is used by Foundry to match up live (engine) entities with Hammer entities.
	inline int		GetHammerID() const { return m_iHammerID; }

public:
	// Networking related methods
	virtual void	NetworkStateChanged();
	virtual void	NetworkStateChanged( unsigned short offset );

public:
 	void CalcAbsolutePosition();

	// interface function pts
	void (CBaseEntity::*m_pfnMoveDone)(void);
	virtual void MoveDone( void ) { if (m_pfnMoveDone) (this->*m_pfnMoveDone)();};

	// Why do we have two separate static Instance functions?
	static CBaseEntity *Instance( const CBaseHandle &hEnt );
	static CBaseEntity *Instance( const edict_t *pent );
	static CBaseEntity *Instance( edict_t *pent );
	static CBaseEntity* Instance( int iEnt );

	// Think function handling
	void (CBaseEntity::*m_pfnThink)(void);
	virtual void Think( void ) { if (m_pfnThink) (this->*m_pfnThink)();};

	// Think functions with contexts
	int		RegisterThinkContext( const char *szContext );
	BASEPTR	ThinkSet( BASEPTR func, float flNextThinkTime = 0, const char *szContext = NULL );
	void	SetNextThink( float nextThinkTime, const char *szContext = NULL );
	float	GetNextThink( const char *szContext = NULL );
	float	GetLastThink( const char *szContext = NULL );
	int		GetNextThinkTick( const char *szContext = NULL );
	int		GetLastThinkTick( const char *szContext = NULL );
	bool AlwaysThink( const char *szContext = NULL );
	bool NeverThink( const char *szContext = NULL );

	float				GetAnimTime() const;
	void				SetAnimTime( float at );

	float				GetSimulationTime() const;
	float				GetOldSimulationTime() const;
	void				SetSimulationTime( float st );

	float				GetCreateTime()										{ return m_flCreateTime; }
	void				SetCreateTime( float flCreateTime )					{ m_flCreateTime = flCreateTime; }

	void				SetRenderMode( RenderMode_t nRenderMode );
	RenderMode_t		GetRenderMode() const;

	void				SetRenderFX( RenderFx_t nRenderFX );
	RenderFx_t			GetRenderFX() const;

private:
	// NOTE: Keep this near vtable so it's in cache with vtable.
	CServerNetworkProperty *m_Network;

private:
	// members
	string_t m_iClassname;  // identifier for entity creation and save/restore
public:
	string_t m_iGlobalname; // identifier for carrying entity across level transitions
	string_t m_iParent;	// the name of the entities parent; linked into m_pParent during Activate()

	// This comes from the "id" key/value that Hammer adds to entities.
	// It is used by Foundry to match up live (engine) entities with Hammer entities.
	int		m_iHammerID; // Hammer unique edit id number

public:
	// was pev->speed
	float		m_flSpeed;

private:
	// was pev->renderfx
	CNetworkVar( RenderFx_t, m_nRenderFX );
	// was pev->rendermode
	CNetworkVar( RenderMode_t, m_nRenderMode );
	CNetworkModelIndex( m_nModelIndex );
	
	CNetworkUtlVector( VisionModelIndex_t, m_VisionModelIndexOverrides ); // used to override the base model index on the client if necessary

public:
	// Prevents this entity from drawing under certain view IDs. Each flag is (1 << the view ID to hide from).
	// For example, hiding an entity from VIEW_MONITOR prevents it from showing up on RT camera monitors
	// and hiding an entity from VIEW_MAIN just prevents it from showing up through the player's own "eyes".
	// Doing this via flags allows for the entity to be hidden from multiple view IDs at the same time.
	// 
	// This was partly inspired by Underhell's keyvalue that allows entities to only render in mirrors and cameras.
	CNetworkVar( view_flags_t, m_iViewHideFlags );

protected:
	// Disables receiving projected textures. Based on a keyvalue from later Source games.
	CNetworkVar( bool, m_bDisableFlashlight );

private:
	// was pev->rendercolor
	CNetworkColor32( m_clrRender );
public:
	const color24 GetRenderColor() const;
	byte GetRenderColorR() const;
	byte GetRenderColorG() const;
	byte GetRenderColorB() const;
	byte GetRenderAlpha() const;
	void SetRenderColor( byte r, byte g, byte b );
	void SetRenderColor( color24 clr );
	void SetRenderColorR( byte r );
	void SetRenderColorG( byte g );
	void SetRenderColorB( byte b );
	void SetRenderAlpha( byte a );

	// was pev->animtime:  consider moving to CBaseAnimating
	float		m_flPrevAnimTime;
	CNetworkTime( m_flAnimTime );  // this is the point in time that the client will interpolate to position,angle,frame,etc.
	CNetworkTime( m_flSimulationTime );
	CNetworkTime( m_flCreateTime );

	void IncrementInterpolationFrame(); // Call this to cause a discontinuity (teleport)

	CNetworkVar( int, m_ubInterpolationFrame );

	int				m_nLastThinkTick;

	// Certain entities (projectiles) can be created on the client and thus need a matching id number
	CNetworkPredictableId( m_PredictableID );

	// used so we know when things are no longer touching
	int			m_nTouchStamp;			

protected:

	// think function handling
	enum thinkmethods_t : unsigned char
	{
		THINK_FIRE_ALL_FUNCTIONS,
		THINK_FIRE_BASE_ONLY,
		THINK_FIRE_ALL_BUT_BASE,
	};
	int		GetIndexForThinkContext( const char *pszContext );
	CUtlVector< thinkfunc_t >	m_aThinkFunctions;

#ifdef _DEBUG
	int							m_iCurrentThinkContext;
#endif

public:
	void RemoveExpiredConcepts( void );
	int	GetContextCount() const;						// Call RemoveExpiredConcepts to clean out expired concepts
	const char *GetContextName( int index ) const;		// note: context may be expired
	const char *GetContextValue( int index ) const; 	// note: context may be expired
	bool ContextExpired( int index ) const;
	int FindContextByName( const char *name ) const;
	void	AddContext( const char *nameandvalue );
	void		AddContext( const char *pName, const char *pValue, float duration );
	inline const ResponseContext_t	*GetContextData( int index ) const; // note: context may be expired
	void		ClearAllContexts( void );

	bool	HasContext( const char *name, const char *value ) const;
	bool	HasContext( string_t name, string_t value ) const; // NOTE: string_t version only compares pointers!
	bool	HasContext( const char *nameandvalue ) const;
	const char *GetContextValue( const char *contextName ) const;
	float	GetContextExpireTime( const char *name );
	void	RemoveContext( const char *nameandvalue );


protected:
	CUtlVector< ResponseContext_t > m_ResponseContexts;

	// Map defined context sets
	string_t	m_iszResponseContext;

private:
	CBaseEntity( CBaseEntity& );

	// list handling
	friend class CGlobalEntityList;
	friend class CThinkSyncTester;

	// was pev->nextthink
	CNetworkVarForDerived( int, m_nNextThinkTick, private );
	// was pev->effects
	CNetworkVar( Effects_t, m_fEffects );

////////////////////////////////////////////////////////////////////////////


public:

	// Returns a CBaseAnimating if the entity is derived from CBaseAnimating.
	virtual CBaseAnimating*	GetBaseAnimating() { return NULL; }
	virtual CBaseAnimatingOverlay *	GetBaseAnimatingOverlay() { return NULL; }

	virtual ResponseRules::IResponseSystem *GetResponseSystem();
	virtual void	DispatchResponse( const char *conceptName );

// Classify - returns the type of group (i.e, "houndeye", or "human military" so that NPCs with different classnames
// still realize that they are teammates. (overridden for NPCs that form groups)
	virtual Class_T Classify ( void );
	virtual void	DeathNotice ( CBaseEntity *pVictim ) {}// NPC maker children use this to tell the NPC maker that they have died.
	virtual bool	ShouldAttractAutoAim( CBaseEntity *pAimingEnt ) { return ((GetFlags() & FL_AIMTARGET) != FL_NO_ENTITY_FLAGS); }
	virtual float	GetAutoAimRadius();
	virtual Vector	GetAutoAimCenter() { return WorldSpaceCenter(); }

	virtual ITraceFilter*	GetBeamTraceFilter( void );

	// Call this to do a TraceAttack on an entity, performs filtering. Don't call TraceAttack() directly except when chaining up to base class
	void			DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator = NULL );
	virtual bool	PassesDamageFilter( const CTakeDamageInfo &info );
	// Special filter functions made for the "damage" family of filters, including filter_damage_transfer.
	bool			PassesFinalDamageFilter( const CTakeDamageInfo &info );
	bool			DamageFilterAllowsBlood( const CTakeDamageInfo &info );
	bool			DamageFilterDamageMod( CTakeDamageInfo &info );

protected:
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator = NULL );

public:

	virtual bool	CanBeHitByMeleeAttack( CBaseEntity *pAttacker ) { return true; }

	// returns the amount of damage inflicted
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	// This is what you should call to apply damage to an entity.
	int TakeDamage( const CTakeDamageInfo &info );
	virtual void AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt ) {}

	virtual int		TakeHealth( float flHealth, DamageTypes_t bitsDamageType );

	virtual bool	IsAlive( void );
	// Entity killed (only fired once)
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	
	void SendOnKilledGameEvent( const CTakeDamageInfo &info );

	// Notifier that I've killed some other entity. (called from Victim's Event_Killed).
	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	// UNDONE: Make this data?
	virtual BloodColor_t				BloodColor( void );

	void					TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, DamageTypes_t bitsDamageType );
	virtual bool			IsTriggered( CBaseEntity *pActivator ) {return true;}
	virtual bool			IsNPC( void ) const { return false; }
	virtual CAI_BaseNPC				*MyNPCPointer( void ); 
	virtual CBaseCombatCharacter *MyCombatCharacterPointer( void ) { return NULL; }
	virtual CBasePlayer *MyPlayerPointer( void ) { return NULL; }
	virtual float			GetDelay( void ) { return 0; }
	virtual bool			IsMoving( void );
	bool					IsWorld() const { extern CWorld *g_WorldEntity; return (void *)this == (void *)g_WorldEntity; }
	virtual char const		*DamageDecal( int bitsDamageType, int gameMaterial );
	virtual void			DecalTrace( trace_t *pTrace, char const *decalName );
	virtual void			ImpactTrace( trace_t *pTrace, DamageTypes_t iDamageType, const char *pCustomImpactName = NULL );

	void			AddPoints( int score, bool bAllowNegativeScore );
	void			AddPointsToTeam( int score, bool bAllowNegativeScore );
	void			RemoveAllDecals( void );

	virtual bool	OnControls( CBaseEntity *pControls ) { return false; }
	virtual bool	HasTarget( string_t targetname );
	virtual	bool	IsPlayer( void ) const { return false; }
	virtual bool	IsNetClient( void ) const { return false; }
	virtual bool	IsTemplate( void ) { return false; }
	virtual bool	IsBaseObject( void ) const { return false; }
	virtual bool	IsBaseTrain( void ) const { return false; }
	bool			IsBSPModel() const;
	bool			IsCombatCharacter() { return MyCombatCharacterPointer() == NULL ? false : true; }
	bool			IsInWorld( void ) const;
	virtual bool	IsCombatItem( void ) const { return false; }

	virtual bool	IsWearable( void ) const { return false; }
	virtual CBaseCombatWeapon *MyCombatWeaponPointer( void ) { return NULL; }
	virtual bool	IsBaseCombatWeapon( void ) const { return const_cast<CBaseEntity *>(this)->MyCombatWeaponPointer() == NULL ? false : true; }

	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle*			GetServerVehicle() { return NULL; }

	// UNDONE: Make this data instead of procedural?
	virtual bool	IsViewable( void );					// is this something that would be looked at (model, sprite, etc.)?

	// Team Handling
	CTeam			*GetTeam( void ) const;				// Get the Team this entity is on
	Team_t				GetTeamNumber( void ) const;		// Get the Team number of the team this entity is on
	virtual void	ChangeTeam( Team_t iTeamNum );			// Assign this entity to a team.
	bool			IsInTeam( CTeam *pTeam ) const;		// Returns true if this entity's in the specified team
	bool			InSameTeam( CBaseEntity *pEntity ) const;	// Returns true if the specified entity is on the same team as this one
	bool			IsInAnyTeam( void ) const;			// Returns true if this entity is in any team
	const char		*TeamID( void ) const;				// Returns the name of the team this entity is on.

	// Entity events... these are events targetted to a particular entity
	// Each event defines its own well-defined event data structure
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );

	// can stand on this entity?
	bool IsStandable() const;

	// UNDONE: Do these three functions actually need to be virtual???
	virtual bool	CanStandOn( CBaseEntity *pSurface ) const { return (pSurface && !pSurface->IsStandable()) ? false : true; }
	virtual bool	CanStandOn( edict_t	*ent ) const { return CanStandOn( GetContainingEntity( ent ) ); }
	virtual CBaseEntity		*GetEnemy( void ) { return NULL; }
	virtual CBaseEntity		*GetEnemy( void ) const { return NULL; }


	void	ViewPunch( const QAngle &angleOffset );
	void	VelocityPunch( const Vector &vecForce );

	CBaseEntity *GetNextTarget( void );
	
	// fundamental callbacks
	void (CBaseEntity ::*m_pfnTouch)( CBaseEntity *pOther );
	void (CBaseEntity ::*m_pfnUse)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void (CBaseEntity ::*m_pfnBlocked)( CBaseEntity *pOther );

	virtual void			Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void			StartTouch( CBaseEntity *pOther );
	virtual void			Touch( CBaseEntity *pOther ); 
	virtual void			EndTouch( CBaseEntity *pOther );
	virtual void			StartBlocked( CBaseEntity *pOther ) {}
	virtual void			Blocked( CBaseEntity *pOther );
	virtual void			EndBlocked( void ) {}

	// Physics simulation
	virtual void			PhysicsSimulate( void );

public:
	// HACKHACK:Get the trace_t from the last physics touch call (replaces the even-hackier global trace vars)
	static const trace_t &	GetTouchTrace( void );

	// FIXME: Should be private, but I can't make em private just yet
	void					PhysicsImpact( CBaseEntity *other, trace_t &trace );
 	void					PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace );
	void					PhysicsMarkEntitiesAsTouchingEventDriven( CBaseEntity *other, trace_t &trace );
	void					PhysicsTouchTriggers( const Vector *pPrevAbsOrigin = NULL );
	virtual void			PhysicsLandedOnGround( float flFallingSpeed ) { return; }

	// Physics helper
	static void				PhysicsRemoveTouchedList( CBaseEntity *ent );
	static void				PhysicsNotifyOtherOfUntouch( CBaseEntity *ent, CBaseEntity *other );
	static void				PhysicsRemoveToucher( CBaseEntity *other, touchlink_t *link );

	groundlink_t			*AddEntityToGroundList( CBaseEntity *other );
	void					PhysicsStartGroundContact( CBaseEntity *pentOther );

	static void				PhysicsNotifyOtherOfGroundRemoval( CBaseEntity *ent, CBaseEntity *other );
	static void				PhysicsRemoveGround( CBaseEntity *other, groundlink_t *link );
	static void				PhysicsRemoveGroundList( CBaseEntity *ent );

	void					StartGroundContact( CBaseEntity *ground );
	void					EndGroundContact( CBaseEntity *ground );

	void					SetGroundChangeTime( float flTime );
	float					GetGroundChangeTime( void );

	// Remove this as ground entity for all object resting on this object
	void					WakeRestingObjects();
	bool					HasNPCsOnIt();

	virtual void			UpdateOnRemove( void );
	virtual void			StopLoopingSounds( void ) {}

	// common member functions
	void					SUB_Remove( void );
	void					SUB_DoNothing( void );
	void					SUB_StartFadeOut( float delay = 10.0f, bool bNotSolid = true );
	void					SUB_StartFadeOutInstant();
	void					SUB_FadeOut ( void );
	void					SUB_Vanish( void );
	void					SUB_CallUseToggle( void ) { this->Use( this, this, USE_TOGGLE, 0 ); }
	void					SUB_PerformFadeOut( void );
	virtual	bool			SUB_AllowedToFade( void );
	// For KillWhenNotVisible
	void					SUB_RemoveWhenNotVisible( void );

	// change position, velocity, orientation instantly
	// passing NULL means no change
	virtual void			Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	// notify that another entity (that you were watching) was teleported
	virtual void			NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

	int						ShouldToggle( USE_TYPE useType, int currentState );

	// UNDONE: Move these virtuals to CBaseCombatCharacter?
	virtual void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual int	GetTracerAttachment( void );
	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual void DoImpactEffect( trace_t &tr, int nDamageType ); // give shooter a chance to do a custom impact.

	virtual void ModifyFireBulletsDamage( CTakeDamageInfo* dmgInfo ) {}

	virtual CBaseEntity *Respawn( void ) { return NULL; }

	// Method used to deal with attacks passing through triggers
	void TraceAttackToTriggers( const CTakeDamageInfo &info, const Vector& start, const Vector& end, const Vector& dir );

	// Do the bounding boxes of these two intersect?
	bool	Intersects( CBaseEntity *pOther );
	virtual bool IsLockedByMaster( void ) { return false; }

	// Health accessors.
	virtual int		GetMaxHealth()  const	{ return m_iMaxHealth; }
	void	SetMaxHealth( int amt )	{ m_iMaxHealth = amt; }

	int		GetHealth() const		{ return m_iHealth; }
	virtual void	SetHealth( int amt )	{ m_iHealth = amt; }

	float HealthFraction() const;

	// Ugly code to lookup all functions to make sure they are in the table when set.
#ifdef _DEBUG

#ifdef GNUC
#define ENTITYFUNCPTR_SIZE	8
#else
#define ENTITYFUNCPTR_SIZE	4
#endif

	void FunctionCheck( void *pFunction, const char *name );
	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, const char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == ENTITYFUNCPTR_SIZE );
		m_pfnTouch = func; 
		FunctionCheck( *(reinterpret_cast<void **>(&m_pfnTouch)), name ); 
		return func;
	}
	USEPTR	UseSet( USEPTR func, const char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == ENTITYFUNCPTR_SIZE );
		m_pfnUse = func; 
		FunctionCheck( *(reinterpret_cast<void **>(&m_pfnUse)), name ); 
		return func;
	}
	ENTITYFUNCPTR	BlockedSet( ENTITYFUNCPTR func, const char *name ) 
	{ 
		COMPILE_TIME_ASSERT( sizeof(func) == ENTITYFUNCPTR_SIZE );
		m_pfnBlocked = func; 
		FunctionCheck( *(reinterpret_cast<void **>(&m_pfnBlocked)), name ); 
		return func;
	}

#endif // _DEBUG

	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& set );
	void			AppendContextToCriteria( AI_CriteriaSet& set, const char *prefix = "" );
	void			DumpResponseCriteria( void );
	// this computes criteria that depend on the other criteria having been set. 
	// needs to be done in a second pass because we may have multiple overrids for
	// a context before it all settles out.
	virtual void	ModifyOrAppendDerivedCriteria( AI_CriteriaSet& set ) {};
	void			ReAppendContextCriteria( AI_CriteriaSet& set );

private:
	friend class CAI_Senses;
	CBaseEntity	*m_pLink;// used for temporary link-list operations. 

public:
	// variables promoted from edict_t
	string_t	m_target;
	CNetworkVarForDerived( int, m_iMaxHealth ); // CBaseEntity doesn't care about changes to this variable, but there are derived classes that do.
private:
	CNetworkVarForDerived( int, m_iHealth, private );

public:
	CNetworkVarForDerived( Lifestate_t, m_lifeState );
	CNetworkVarForDerived( Takedamage_t , m_takedamage );

	// Damage filtering
	string_t	m_iszDamageFilterName;	// The name of the entity to use as our damage filter.
	EHANDLE		m_hDamageFilter;		// The entity that controls who can damage us.

	// Debugging / devolopment fields
	DebugOverlayBits_t				m_debugOverlays;	// For debug only (bitfields)
#if !defined SWDS || 1
	TimedOverlay_t*	m_pTimedOverlay;	// For debug only
#endif

	// virtual functions used by a few classes
	
	// creates an entity of a specified class, by name
	static CBaseEntity *Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	static CBaseEntity *CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );

private:
	static bool DO_NOT_USE_SetupAsPredicted( CBaseEntity *pEntity, const char *szName, const char *module, int line );

public:
	static CBaseEntity *CreatePredicted( const char *module, int line, const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	static CBaseEntity *CreatePredictedNoSpawn( const char *module, int line, const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );

	// Collision group accessors
	Collision_Group_t				GetCollisionGroup() const;
	void			SetCollisionGroup( Collision_Group_t collisionGroup );
	void			CollisionRulesChanged();

	// Damage accessors
	virtual DamageTypes_t		GetDamageType() const;
	virtual float	GetDamage() { return 0; }
	virtual void	SetDamage(float flDamage) {}

	// Some entities want to use interactions regardless of whether they're a CBaseCombatCharacter.
	// Valve ran into this issue with frag grenades when they started deriving from CBaseAnimating instead of CBaseCombatCharacter,
	// preventing them from using the barnacle interactions for rigged grenade timing so it's guaranteed to blow up in the barnacle's face.
	// We're used to unaltered behavior now, so we're not restoring that as default, but making this a "base entity" thing is supposed to help in situtions like those.
	// 
	// Also, keep in mind pretty much all existing DispatchInteraction() calls are only performed on CBaseCombatCharacters.
	// You'll need to change their code manually if you want other, non-character entities to use the interaction.
	bool				DispatchInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt );

	// Do not call HandleInteraction directly, use DispatchInteraction
	virtual bool		HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt ) { return false; }

	virtual Vector	EyePosition( void );			// position of eyes
	virtual const QAngle &EyeAngles( void );		// Direction of eyes in world space
	virtual const QAngle &LocalEyeAngles( void );	// Direction of eyes
	virtual Vector	EarPosition( void );			// position of ears
	void	EyePositionZOnly( Vector *pPosition );	// position of eyes, ignoring X and Y

	float GetDistanceToEntity( const CBaseEntity *other ) const; // Distance between GetAbsOrigins.

	Vector	EyePosition( void ) const;			// position of eyes
	const QAngle &EyeAngles( void ) const;		// Direction of eyes in world space
	const QAngle &LocalEyeAngles( void ) const;	// Direction of eyes
	Vector	EarPosition( void ) const;			// position of ears

	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true);		// position to shoot at
	virtual Vector	HeadTarget( const Vector &posSrc );
	virtual void	GetVectors(Vector* forward, Vector* right, Vector* up) const;

	virtual const Vector &GetViewOffset() const;
	virtual void SetViewOffset( const Vector &v );

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void			SetLocalVelocity( const Vector &vecVelocity );
	void			ApplyLocalVelocityImpulse( const Vector &vecImpulse );
	void			SetAbsVelocity( const Vector &vecVelocity );
	void			ApplyAbsVelocityImpulse( const Vector &vecImpulse );
	void			ApplyLocalAngularVelocityImpulse( const AngularImpulse &angImpulse );

	const Vector&	GetLocalVelocity( ) const;
	const Vector&	GetAbsVelocity( ) const;

	// NOTE: Setting the abs velocity in either space will cause a recomputation
	// in the other space, so setting the abs velocity will also set the local vel
	void			SetLocalAngularVelocity( const QAngle &vecAngVelocity );
	const QAngle&	GetLocalAngularVelocity( ) const;

	// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
	// representation, we can't actually solve this problem
//	void			SetAbsAngularVelocity( const QAngle &vecAngVelocity );
//	const QAngle&	GetAbsAngularVelocity( ) const;

	const Vector&	GetBaseVelocity() const;
	void			SetBaseVelocity( const Vector& v );

	virtual Vector	GetSmoothedVelocity( void );

	// FIXME: Figure out what to do about this
	virtual void	GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity = NULL);

	float			GetGravity( void ) const;
	void			SetGravity( float gravity );
	virtual float			GetFriction( void ) const;
	void			SetFriction( float flFriction );

	// Mechanism for overriding friction for a short duration
	void			OverrideFriction( float duration, float friction );

	void			SetMass(float mass);
	float			GetMass();

	virtual	bool FVisible ( CBaseEntity *pEntity, ContentsFlags_t traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual bool FVisible( const Vector &vecTarget, ContentsFlags_t traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );

	virtual bool CanBeSeenBy( CAI_BaseNPC *pNPC ); // allows entities to be 'invisible' to NPC senses.
	virtual bool CanBeSeenBy( CBaseEntity *pEnt ); // allows entities to be 'invisible' to NPC senses.

	// This function returns a value that scales all damage done by this entity.
	// Use CDamageModifier to hook in damage modifiers on a guy.
	virtual float			GetAttackDamageScale( CBaseEntity *pVictim );
	// This returns a value that scales all damage done to this entity
	// Use CDamageModifier to hook in damage modifiers on a guy.
	virtual float			GetReceivedDamageScale( CBaseEntity *pAttacker );

 	void					SetCheckUntouch( bool check );
	bool					GetCheckUntouch() const;

	void					SetGroundEntity( CBaseEntity *ground );
	CBaseEntity				*GetGroundEntity( void );
	CBaseEntity				*GetGroundEntity( void ) const { return const_cast<CBaseEntity *>(this)->GetGroundEntity(); }
	virtual void			OnGroundChanged( CBaseEntity *oldGround, CBaseEntity *newGround ) {}

	// Gets the velocity we impart to a player standing on us
	virtual void			GetGroundVelocityToApply( Vector &vecGroundVel ) { vecGroundVel = vec3_origin; }

	WaterLevel_t						GetWaterLevel() const;
	void					SetWaterLevel( WaterLevel_t nLevel );
	WaterType_t						GetWaterType() const;
	void					SetWaterType( WaterType_t nType );

	virtual bool			PhysicsSplash( const Vector &centerPoint, const Vector &normal, float rawSpeed, float scaledSpeed ) { return false; }
	virtual void			Splash() {}

	void					ClearSolidFlags( void );	
	void					RemoveSolidFlags( SolidFlags_t flags );
	void					AddSolidFlags( SolidFlags_t flags );
	bool					IsSolidFlagSet( SolidFlags_t flagMask ) const;
	void				 	SetSolidFlags( SolidFlags_t flags );
	bool					IsSolid() const;
	
	void					SetModelName( string_t name );

	model_t					*GetModel( void );

	void					SetAIAddOn( string_t name );

	// These methods return a *world-aligned* box relative to the absorigin of the entity.
	// This is used for collision purposes and is *not* guaranteed
	// to surround the entire entity's visual representation
	// NOTE: It is illegal to ask for the world-aligned bounds for
	// SOLID_BSP objects
	const Vector&			WorldAlignMins( ) const;
	const Vector&			WorldAlignMaxs( ) const;

	// This defines collision bounds in OBB space
	virtual void					SetCollisionBounds( const Vector& mins, const Vector &maxs );

	// NOTE: The world space center *may* move when the entity rotates.
	virtual const Vector&	WorldSpaceCenter( ) const;
 	const Vector&			WorldAlignSize( ) const;

	// Returns a radius of a sphere 
	// *centered at the world space center* bounding the collision representation 
	// of the entity. NOTE: The world space center *may* move when the entity rotates.
	float					BoundingRadius() const;
	float					BoundingRadius2D() const;
	bool					IsPointSized() const;

	// NOTE: Setting the abs origin or angles will cause the local origin + angles to be set also
	void					SetAbsOrigin( const Vector& origin );
	void					SetAbsAngles( const QAngle& angles );

	// Origin and angles in local space ( relative to parent )
	// NOTE: Setting the local origin or angles will cause the abs origin + angles to be set also
	void					SetLocalOrigin( const Vector& origin );
	const Vector&			GetLocalOrigin( void ) const;

	void					SetLocalAngles( const QAngle& angles );
	const QAngle&			GetLocalAngles( void ) const;

	void					SetElasticity( float flElasticity );
	float					GetElasticity( void ) const;

	void					SetShadowCastDistance( float flDistance );
	float					GetShadowCastDistance( void ) const;
	void					SetShadowCastDistance( float flDesiredDistance, float flDelay );

	float					GetLocalTime( void ) const;
	void					IncrementLocalTime( float flTimeDelta );
	float					GetMoveDoneTime( ) const;
	void					SetMoveDoneTime( float flTime );

	// Cell position
	void					SetCellBits( int cellbits = CELL_BASEENTITY_ORIGIN_CELL_BITS );
	void					UpdateCell();
	
	static void SendProxy_CellX( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
	static void SendProxy_CellY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
	static void SendProxy_CellZ( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
	static void SendProxy_CellOrigin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void SendProxy_CellOriginXY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void SendProxy_CellOriginZ( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void SendProxy_AnglesX( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void SendProxy_AnglesY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	static void SendProxy_AnglesZ( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
		
	
	// Used by the PAS filters to ask the entity where in world space the sounds it emits come from.
	// This is used right now because if you have something sitting on an incline, using our axis-aligned 
	// bounding boxes can return a position in solid space, so you won't hear sounds emitted by the object.
	// For now, we're hacking around it by moving the sound emission origin up on certain objects like vehicles.
	//
	// When OBBs get in, this can probably go away.
	virtual Vector			GetSoundEmissionOrigin() const;

	void					AddFlag( EntityBehaviorFlags_t flags );
	void					RemoveFlag( EntityBehaviorFlags_t flagsToRemove );
	void					ToggleFlag( EntityBehaviorFlags_t flagToToggle );
	EntityBehaviorFlags_t						GetFlags( void ) const;
	void					ClearFlags( void );

	// Sets the local position from a transform
	void					SetLocalTransform( const matrix3x4_t &localTransform );

	// See CSoundEmitterSystem
	void					EmitSound( const char *soundname, float soundtime = 0.0f, float *duration = NULL );  // Override for doing the general case of CPASAttenuationFilter filter( this ), and EmitSound( filter, entindex(), etc. );
	void					EmitSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, float soundtime = 0.0f, float *duration = NULL );  // Override for doing the general case of CPASAttenuationFilter filter( this ), and EmitSound( filter, entindex(), etc. );
	void					StopSound( const char *soundname );
	void					StopSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle );
	void					GenderExpandString( char const *in, char *out, int maxlen );

	virtual void ModifyEmitSoundParams( EmitSound_t &params );

	// Same as above, but for sentences
	// (which don't actually have EmitSound_t params)
	virtual void ModifySentenceParams( int &iSentenceIndex, int &iChannel, float &flVolume, soundlevel_t &iSoundlevel, int &iFlags, int &iPitch,
		const Vector **pOrigin, const Vector **pDirection, bool &bUpdatePositions, float &soundtime, int &iSpecialDSP, int &iSpeakerIndex );

	static float GetSoundDuration( const char *soundname, char const *actormodel );

	static bool	GetParametersForSound( const char *soundname, CSoundParameters &params, char const *actormodel );
	static bool	GetParametersForSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters &params, char const *actormodel );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin = NULL, float soundtime = 0.0f, float *duration = NULL );
	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, HSOUNDSCRIPTHANDLE& handle, const Vector *pOrigin = NULL, float soundtime = 0.0f, float *duration = NULL );
	static void StopSound( int iEntIndex, const char *soundname );
	static soundlevel_t LookupSoundLevel( const char *soundname );
	static soundlevel_t LookupSoundLevel( const char *soundname, HSOUNDSCRIPTHANDLE& handle );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params );
	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params, HSOUNDSCRIPTHANDLE& handle );

	static void StopSound( int iEntIndex, int iChannel, const char *pSample, bool bIsStoppingSpeakerSound = false );

	static void EmitAmbientSound( int entindex, const Vector& origin, const char *soundname, int flags = 0, float soundtime = 0.0f, float *duration = NULL );

	// These files need to be listed in scripts/game_sounds_manifest.txt
	static HSOUNDSCRIPTHANDLE PrecacheScriptSound( const char *soundname );
	static void PrefetchScriptSound( const char *soundname );

	// For each client who appears to be a valid recipient, checks the client has disabled CC and if so, removes them from 
	//  the recipient list.
	static void RemoveRecipientsIfNotCloseCaptioning( CRecipientFilter& filter );
	static void EmitCloseCaption( IRecipientFilter& filter, int entindex, char const *token, CUtlVector< Vector >& soundorigins, float duration, bool warnifmissing = false );
	static void	EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, bool bUpdatePositions = true, float soundtime = 0.0f
		, int iSpecialDSP = 0, int iSpeakerIndex = -1 // Needed for env_microphone
		 );

	static bool IsPrecacheAllowed();
	static void SetAllowPrecache( bool allow );

	static bool m_bAllowPrecache;

	static bool IsSimulatingOnAlternateTicks();

	virtual bool IsDeflectable() { return false; }
	virtual void Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir ) {}

//	void Relink() {}

public:

	// VPHYSICS Integration -----------------------------------------------
	//
	// --------------------------------------------------------------------
	// UNDONE: Move to IEntityVPhysics? or VPhysicsProp() ?
	// Called after spawn, and in the case of self-managing objects, after load
	virtual bool	CreateVPhysics();

	// Convenience routines to init the vphysics simulation for this object.
	// This creates a static object.  Something that behaves like world geometry - solid, but never moves
	IPhysicsObject *VPhysicsInitStatic( void );

	// This creates a normal vphysics simulated object - physics determines where it goes (gravity, friction, etc)
	// and the entity receives updates from vphysics.  SetAbsOrigin(), etc do not affect the object!
	IPhysicsObject *VPhysicsInitNormal( SolidType_t solidType, SolidFlags_t nSolidFlags, bool createAsleep, solid_t *pSolid = NULL );

	// This creates a vphysics object with a shadow controller that follows the AI
	// Move the object to where it should be and call UpdatePhysicsShadowToCurrentPosition()
	IPhysicsObject *VPhysicsInitShadow( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid = NULL );

	// Force a non-solid (ie. solid_trigger) physics object to collide with other entities.
	virtual bool	ForceVPhysicsCollide( CBaseEntity *pEntity ) { return false; }

private:
	// called by all vphysics inits
	bool			VPhysicsInitSetup();
public:

	void			VPhysicsSetObject( IPhysicsObject *pPhysics );
	// destroy and remove the physics object for this entity
	virtual void	VPhysicsDestroyObject( void );
	void			VPhysicsSwapObject( IPhysicsObject *pSwap );

	inline IPhysicsObject *VPhysicsGetObject( void ) const { return m_pPhysicsObject; }
	virtual void	VPhysicsUpdate( IPhysicsObject *pPhysics );
	void			VPhysicsUpdatePusher( IPhysicsObject *pPhysics );
	float			VPhysicsGetNonShadowMass( void ) const { return m_flNonShadowMass; }
	
	// react physically to damage (called from CBaseEntity::OnTakeDamage() by default)
	virtual int		VPhysicsTakeDamage( const CTakeDamageInfo &info );
	virtual void	VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	VPhysicsShadowUpdate( IPhysicsObject *pPhysics ) {}
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );
	
	// update the shadow so it will coincide with the current AI position at some time
	// in the future (or 0 for now)
	virtual void	UpdatePhysicsShadowToCurrentPosition( float deltaTime );
	virtual int		VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );
	virtual bool	VPhysicsIsFlesh( void );
	// --------------------------------------------------------------------

public:
	// The player drives simulation of this entity
	void					SetPlayerSimulated( CBasePlayer *pOwner );
	void					UnsetPlayerSimulated( void );
	bool					IsPlayerSimulated( void ) const;
	CBasePlayer				*GetSimulatingPlayer( void );

	// FIXME: Make these private!
	void					PhysicsCheckForEntityUntouch( void );
 	bool					PhysicsRunThink( thinkmethods_t thinkMethod = THINK_FIRE_ALL_FUNCTIONS, bool bForce = false );
	bool					PhysicsRunSpecificThink( int nContextIndex, BASEPTR thinkFunc, bool bForce = false );
	bool					PhysicsTestEntityPosition( CBaseEntity **ppEntity = NULL );
	void					PhysicsPushEntity( const Vector& push, trace_t *pTrace );
	bool					PhysicsCheckWater( void );
	void					PhysicsCheckWaterTransition( void );
	void					PhysicsStepRecheckGround();
	// Computes the water level + type
	void					UpdateWaterState();
	bool					IsEdictFree() const { return edict()->IsFree(); }
	virtual bool			CanPushEntity( CBaseEntity *other ) const {return true;}	//Any special reason we can't push this other thing while moving?

	// Callbacks for the physgun/cannon picking up an entity
	virtual	CBasePlayer		*HasPhysicsAttacker( float dt ) { return NULL; }

	// UNDONE: Make this data?
	virtual ContentsFlags_t	PhysicsSolidMaskForEntity( void ) const;

	// Computes the abs position of a point specified in local space
	void					ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition );

	// Computes the abs position of a direction specified in local space
	void					ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection );

	// Precache model sounds + particles
	static void PrecacheModelComponents( modelindex_t nModelIndex );
	static void PrecacheSoundHelper( const char *pName );

protected:
	// Invalidates the abs state of all children
	void					InvalidatePhysicsRecursive( int nChangeFlags );

	int						PhysicsClipVelocity (const Vector& in, const Vector& normal, Vector& out, float overbounce );
	void					PhysicsRelinkChildren( float dt );

	// Performs the collision resolution for fliers.
	void					PerformFlyCollisionResolution( trace_t &trace, Vector &move );
	void					ResolveFlyCollisionBounce( trace_t &trace, Vector &vecVelocity, float flMinTotalElasticity = 0.0f );
	void					ResolveFlyCollisionSlide( trace_t &trace, Vector &vecVelocity );
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

private:
	// Physics-related private methods
	void					PhysicsStep( void );
	void					PhysicsPusher( void );
	void					PhysicsNone( void );
	void					PhysicsNoclip( void );
	void					PhysicsStepRunTimestep( float timestep );
	void					PhysicsToss( void );
	void					PhysicsCustom( void );
	void					PerformPush( float movetime );

	// Simulation in local space of rigid children
	void					PhysicsRigidChild( void );

	// Computes the base velocity
	void					UpdateBaseVelocity( void );

	// Implement this if you use MOVETYPE_CUSTOM
	virtual void			PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	void					PhysicsDispatchThink( BASEPTR thinkFunc );

	touchlink_t				*PhysicsMarkEntityAsTouched( CBaseEntity *other );
	void					PhysicsTouch( CBaseEntity *pentOther );
	void					PhysicsStartTouch( CBaseEntity *pentOther );

	CBaseEntity				*PhysicsPushMove( float movetime );
	CBaseEntity				*PhysicsPushRotate( float movetime );

	CBaseEntity				*PhysicsCheckRotateMove( rotatingpushmove_t &rotPushmove, CBaseEntity **pPusherList, int pusherListCount );
	CBaseEntity				*PhysicsCheckPushMove( const Vector& move, CBaseEntity **pPusherList, int pusherListCount );
	int						PhysicsTryMove( float flTime, trace_t *steptrace );

	void					PhysicsCheckVelocity( void );
	void					PhysicsAddHalfGravity( float timestep );
	void					PhysicsAddGravityMove( Vector &move );

	void					CalcAbsoluteVelocity();
	void					CalcAbsoluteAngularVelocity();

	// Checks a sweep without actually performing the move
	void					PhysicsCheckSweep( const Vector& vecAbsStart, const Vector &vecAbsDelta, trace_t *pTrace );

	// Computes new angles based on the angular velocity
	void					SimulateAngles( float flFrameTime );

	void					CheckStepSimulationChanged();
	// Run regular think and latch off angle/origin changes so we can interpolate them on the server to fake simulation
	void					StepSimulationThink( float dt );

	// Compute network origin
private:
	void					ComputeStepSimulationNetwork( StepSimulationData *step );

public:
	bool					UseStepSimulationNetworkOrigin( const Vector **out_v, int cell[3] = NULL ); // cell should be a 3 int array
	bool					UseStepSimulationNetworkAngles( const QAngle **out_a );

public:
	// Add a discontinuity to a step
	bool					AddStepDiscontinuity( float flTime, const Vector &vecOrigin, const QAngle &vecAngles );
	int						GetFirstThinkTick();	// get first tick thinking on any context
	void					RebaseThinkTicks( bool bMakeDeltas );	// Rebase all the think times as deltas or from deltas to current ticks

private:
	// origin and angles to use in step calculations
	virtual	Vector			GetStepOrigin( void ) const;
	virtual	QAngle			GetStepAngles( void ) const;
	
	// These set entity flags (EFL_*) to help optimize queries
	void					CheckHasThinkFunction( int thinkTick );
	void					CheckHasGamePhysicsSimulation();
	bool					WillThink();
	bool					WillSimulateGamePhysics();

	friend class CPushBlockerEnum;

	// Sets/Gets the next think based on context index
	void SetNextThink( int nContextIndex, float thinkTime );
	void SetLastThink( int nContextIndex, float thinkTime );
	float GetNextThink( int nContextIndex ) const;
	int	GetNextThinkTick( int nContextIndex ) const;
	bool AlwaysThink( int nContextIndex ) const;
	bool NeverThink( int nContextIndex ) const;

	// Shot statistics
	void UpdateShotStatistics( const trace_t &tr );

	// Handle shot entering water
	bool HandleShotImpactingWater( const FireBulletsInfo_t &info, const Vector &vecEnd, ITraceFilter *pTraceFilter, Vector *pVecTracerDest );

	// Handle shot entering water
	void HandleShotImpactingGlass( const FireBulletsInfo_t &info, const trace_t &tr, const Vector &vecDir, ITraceFilter *pTraceFilter );

	// Should we draw bubbles underwater?
	bool ShouldDrawUnderwaterBulletBubbles();

	// Computes the tracer start position
	void ComputeTracerStartPosition( const Vector &vecShotSrc, Vector *pVecTracerStart );

	// Computes the tracer start position
	void CreateBubbleTrailTracer( const Vector &vecShotSrc, const Vector &vecShotEnd, const Vector &vecShotDir );

	virtual bool ShouldDrawWaterImpacts() { return true; }

	// Changes shadow cast distance over time
	void ShadowCastDistThink( );

protected:
	// Which frame did I simulate?
	int						m_nSimulationTick;

protected:
	// FIXME: Make this private! Still too many references to do so...
	CNetworkVarForDerived( uint64, m_spawnflags, protected );

private:
	EntityFlags_t		m_iEFlags;	// entity flags EFL_*
	// was pev->flags
	CNetworkVarForDerived( EntityBehaviorFlags_t, m_fFlags, private );

	CNetworkStringT( m_iName ); // name used to identify this entity

	// Damage modifiers
	friend class CDamageModifier;
	CUtlLinkedList<CDamageModifier*,int>	m_DamageModifiers;

	EHANDLE m_pParent;  // for movement hierarchy

	byte	m_nTransmitStateOwnedCounter;
	CNetworkVar( unsigned char,  m_iParentAttachment ); // 0 if we're relative to the parent's absorigin and absangles.
	CNetworkVar( MoveType_t, m_MoveType );		// One of the MOVETYPE_ defines.
	CNetworkVar( MoveCollide_t, m_MoveCollide );

	// Our immediate parent in the movement hierarchy.
	// FIXME: clarify m_pParent vs. m_pMoveParent
	CNetworkHandle( CBaseEntity, m_hMoveParent );
	// cached child list
	EHANDLE m_hMoveChild;	
	// generated from m_pMoveParent
	EHANDLE m_hMovePeer;	

	friend class CCollisionProperty;
	friend class CServerNetworkProperty;
	CNetworkVarEmbeddedPtr( CCollisionProperty, m_pCollision );

	CNetworkHandle( CBaseEntity, m_hOwnerEntity );	// only used to point to an edict it won't collide with
	CNetworkHandle( CBaseEntity, m_hEffectEntity );	// Fire/Dissolve entity.
	CNetworkDistance( m_fadeMinDist );	// Point at which fading is absolute
	CNetworkDistance( m_fadeMaxDist );	// Point at which fading is inactive
	CNetworkScale( m_flFadeScale );	// Scale applied to min / max

	CNetworkVar( Collision_Group_t, m_CollisionGroup );		// used to cull collision tests
	IPhysicsObject	*m_pPhysicsObject;	// pointer to the entity's physics object (vphysics.dll)
	float m_flNonShadowMass;	// cached mass (shadow controllers set mass to VPHYSICS_MAX_MASS, or 50000)

	CNetworkDistance( m_flShadowCastDistance );
	float		m_flDesiredShadowCastDistance;

	// Team handling
	Team_t			m_iInitialTeamNum;		// Team number of this entity's team read from file
	CNetworkVar( Team_t, m_iTeamNum );				// Team number of this entity's team. 

	// Sets water type + level for physics objects
	unsigned char	m_nWaterTouch;
	unsigned char	m_nSlimeTouch;
	WaterType_t	m_nWaterType;
	CNetworkVarForDerived( WaterLevel_t, m_nWaterLevel, private );
	float			m_flNavIgnoreUntilTime;
	bool			m_bAlwaysIgnoreNav;

	CNetworkHandleForDerived( CBaseEntity, m_hGroundEntity, private );
	float			m_flGroundChangeTime; // Time that the ground entity changed
	
	string_t		m_ModelName;
	string_t		m_AIAddOn;

	// Velocity of the thing we're standing on (world space)
	CNetworkVectorForDerived( m_vecBaseVelocity, private );

	// Global velocity
	Vector			m_vecAbsVelocity;

	// Local angular velocity
	QAngle			m_vecAngVelocity;

	// Global angular velocity
//	QAngle			m_vecAbsAngVelocity;

	// local coordinate frame of entity
	matrix3x4_t		m_rgflCoordinateFrame;

	// Physics state
	EHANDLE			m_pBlocker;

	// was pev->gravity;
	float			m_flGravity;  // rename to m_flGravityScale;
	// was pev->friction
	CNetworkVarForDerived( float, m_flFriction, private );
	CNetworkScale( m_flElasticity );
	float m_flOverriddenFriction;
	void FrictionRevertThink( void );

	// was pev->ltime
	float			m_flLocalTime;
	// local time at the beginning of this frame
	float			m_flVPhysicsUpdateLocalTime;
	// local time the movement has ended
	float			m_flMoveDoneTime;

	// A counter to help quickly build a list of potentially pushed objects for physics
	int				m_nPushEnumCount;

	Vector			m_vecAbsOrigin;
	CNetworkVectorForDerived( m_vecVelocity, private );
	
	//Adrian
	CNetworkVar( unsigned char, m_iTextureFrameIndex );
	
	CNetworkVar( bool, m_bSimulatedEveryTick );
	CNetworkVar( bool, m_bAnimatedEveryTick );
	CNetworkVar( bool, m_bAlternateSorting );

	CNetworkVar( CPULevel_t, m_nMinCPULevel );
	CNetworkVar( CPULevel_t, m_nMaxCPULevel );
	CNetworkVar( GPULevel_t, m_nMinGPULevel );
	CNetworkVar( GPULevel_t, m_nMaxGPULevel );

public:
	CNetworkVarForDerived( bool, m_bClientSideRagdoll );

private:
	// User outputs. Fired when the "FireInputX" input is triggered.
	COutputVariant m_OutUser1;
	COutputVariant m_OutUser2;
	COutputVariant m_OutUser3;
	COutputVariant m_OutUser4;
	COutputEvent m_OnUser1;
	COutputEvent m_OnUser2;
	COutputEvent m_OnUser3;
	COutputEvent m_OnUser4;

	COutputEvent m_OnKilled;

	QAngle			m_angAbsRotation;

	CNetworkVector( m_vecOrigin );
	CNetworkQAngle( m_angRotation );

private:
	EHANDLE m_RefEHandle;

private:
	// We cache the cell width for convience
	int m_cellwidth;

	CNetworkVar( unsigned int, m_cellbits );

	// Cell of the current origin
//	CNetworkArray( int, m_cellXY, 2 );
	CNetworkVar( unsigned int, m_cellX );
	CNetworkVar( unsigned int, m_cellY );
	CNetworkVar( unsigned int, m_cellZ );

	// was pev->view_ofs ( FIXME:  Move somewhere up the hierarch, CBaseAnimating, etc. )
	CNetworkVectorForDerived( m_vecViewOffset, private );

private:
	// dynamic model state tracking
	bool m_bDynamicModelAllowed;
	bool m_bDynamicModelPending;
	bool m_bDynamicModelSetBounds;
	void OnModelLoadComplete( const model_t* model );
	friend class CBaseEntityModelLoadProxy;

protected:
	void EnableDynamicModels() { m_bDynamicModelAllowed = true; }

public:
	bool IsDynamicModelLoading() const { return m_bDynamicModelPending; } 
	void SetCollisionBoundsFromModel();

	CNetworkVar( bool, m_bIsPlayerSimulated );
	// Player who is driving my simulation
	CNetworkHandle( CBasePlayer, m_hPlayerSimulationOwner );

	int								m_fDataObjectTypes;

	UtlHashHandle_t		m_ListByClass;
	CBaseEntity	*		m_pPrevByClass;
	CBaseEntity	*		m_pNextByClass;

	// So it can get at the physics methods
	friend class CCollisionEvent;

// Methods shared by client and server
public:
	void							SetSize( const Vector &vecMin, const Vector &vecMax ); // UTIL_SetSize( this, mins, maxs );
	static modelindex_t						PrecacheModel( const char *name, bool bPreload = true ); 
	static bool						PrecacheSound( const char *name );
	static void						PrefetchSound( const char *name );

private:
	void							Remove( ); // UTIL_Remove( this );

public:
	void							SetNetworkQuantizeOriginAngAngles( bool bQuantize );

	// Default implementation, assumes SPROP_COORD precision and default CBaseEntity SendPropQAngles!!!
	void							NetworkQuantize( Vector &org, QAngle &angles );

	bool							ShouldLagCompensate() const;

private:

	// This is a random seed used by the networking code to allow client - side prediction code
	//  randon number generators to spit out the same random numbers on both sides for a particular
	//  usercmd input.
	static int						m_nPredictionRandomSeed;
	static int						m_nPredictionRandomSeedServer;
	static CBasePlayer				*m_pPredictionPlayer;

	// FIXME: Make hierarchy a member of CBaseEntity
	// or a contained private class...
	friend void UnlinkChild( CBaseEntity *pParent, CBaseEntity *pChild );
	friend void LinkChild( CBaseEntity *pParent, CBaseEntity *pChild );
	friend void ClearParent( CBaseEntity *pEntity );
	friend void UnlinkAllChildren( CBaseEntity *pParent );
	friend void UnlinkFromParent( CBaseEntity *pRemove );
	friend void TransferChildren( CBaseEntity *pOldParent, CBaseEntity *pNewParent );

	bool m_bNetworkQuantizeOriginAndAngles;
	bool m_bLagCompensate; // Special flag for certain l4d2 props to use

protected:
	CGlobalEvent	*m_pEvent;
	
public:
	// Accessors for above
	static int						GetPredictionRandomSeed( bool bUseUnSyncedServerPlatTime = false );
	static void						SetPredictionRandomSeed( const CUserCmd *cmd );
	static CBasePlayer				*GetPredictionPlayer( void );
	static void						SetPredictionPlayer( CBasePlayer *player );

	// Used to access m_vecAbsOrigin during restore when it's unsafe to call GetAbsOrigin.
	friend class CPlayerRestoreHelper;
	
	static bool s_bAbsQueriesValid;

	// Call this when hierarchy is not completely set up (such as during Restore) to throw asserts
	// when people call GetAbsAnything. 
	static inline void SetAbsQueriesValid( bool bValid )
	{
		s_bAbsQueriesValid = bValid;
	}
	
	static inline bool IsAbsQueriesValid()
	{
		return s_bAbsQueriesValid;
	}

	virtual bool ShouldBlockNav() const { return true; }

public:
	// Density map
	virtual float GetDensityMultiplier() { return 1.0f; }
	DensityWeightsMap *DensityMap();
	void SetDensityMapType( density_type_t iType );

	// Nav obstacle ref for recast mesh.
	void SetNavObstacleRef( int ref ) { m_NavObstacleRef = ref; }
	int GetNavObstacleRef() { return m_NavObstacleRef; }
	float						GetViewDistance();
	void						SetViewDistance( float dist );

private:
	DensityWeightsMap *m_pDensityMap;

	bool					m_bAllowNavIgnore;
	int						m_NavObstacleRef;
	CNetworkDistance( m_fViewDistance );
};

// Send tables exposed in this module.
EXTERN_SEND_TABLE(DT_Edict);
EXTERN_SEND_TABLE(DT_BaseEntity);



// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#ifdef _DEBUG

#define SetTouch( a ) TouchSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define SetUse( a ) UseSet( static_cast <void (CBaseEntity::*)(	CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a), #a )
#define SetBlocked( a ) BlockedSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )

#else

#define SetTouch( a ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse( a ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a)
#define SetBlocked( a ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)

#endif

// handling entity/edict transforms
inline CBaseEntity *GetContainingEntity( edict_t *pent )
{
	if ( pent && pent->GetUnknown() )
	{
		return pent->GetUnknown()->GetBaseEntity();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Pauses or resumes entity i/o events. When paused, no outputs will
//			fire unless Debug_SetSteps is called with a nonzero step value.
// Input  : bPause - true to pause, false to resume.
//-----------------------------------------------------------------------------
inline void CBaseEntity::Debug_Pause(bool bPause)
{
	CBaseEntity::m_bDebugPause = bPause;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if entity i/o is paused, false if not.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_IsPaused(void)
{
	return(CBaseEntity::m_bDebugPause);
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the debug step counter. Used when the entity i/o system
//			is in single step mode, this is called every time an output is fired.
// Output : Returns true on to continue firing outputs, false to stop.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_Step(void)
{
	if (CBaseEntity::m_nDebugSteps > 0)
	{
		CBaseEntity::m_nDebugSteps--;
	}
	return(CBaseEntity::m_nDebugSteps > 0);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the number of entity outputs to allow to fire before pausing
//			the entity i/o system.
// Input  : nSteps - Number of steps to execute.
//-----------------------------------------------------------------------------
inline void CBaseEntity::Debug_SetSteps(int nSteps)
{
	CBaseEntity::m_nDebugSteps = nSteps;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if we should allow outputs to be fired, false if not.
//-----------------------------------------------------------------------------
inline bool CBaseEntity::Debug_ShouldStep(void)
{
	return(!CBaseEntity::m_bDebugPause || CBaseEntity::m_nDebugSteps > 0);
}

//-----------------------------------------------------------------------------
// Methods relating to traversing hierarchy
//-----------------------------------------------------------------------------
inline CBaseEntity *CBaseEntity::GetMoveParent( void )
{
	return m_hMoveParent.Get(); 
}

inline CBaseEntity *CBaseEntity::FirstMoveChild( void )
{
	return m_hMoveChild.Get(); 
}

inline CBaseEntity *CBaseEntity::NextMovePeer( void )
{
	return m_hMovePeer.Get();
}

// FIXME: Remove this! There shouldn't be a difference between moveparent + parent
inline CBaseEntity* CBaseEntity::GetParent()
{
	return m_pParent.Get();
}

inline int CBaseEntity::GetParentAttachment()
{
	return m_iParentAttachment;
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline string_t CBaseEntity::GetEntityName() 
{ 
	return m_iName; 
}

inline string_t CBaseEntity::GetEntityNameAsTStr() 
{ 
	return m_iName; 
}

inline const char *CBaseEntity::GetEntityNameAsCStr()
{
	return STRING(m_iName.Get());
}

inline const char *CBaseEntity::GetPreTemplateName()
{
	const char *pszDelimiter = V_strrchr( STRING(m_iName.Get()), '&' );
	if ( !pszDelimiter )
		return STRING( m_iName.Get() );
	static char szStrippedName[128];
	V_strncpy( szStrippedName, STRING( m_iName.Get() ), MIN( ARRAYSIZE(szStrippedName), pszDelimiter - STRING( m_iName.Get() ) + 1 ) );
	return szStrippedName;
}

inline void CBaseEntity::SetName( string_t newName )
{
	m_iName = newName;
}

inline void CBaseEntity::SetNameAsCStr( const char *newName )
{
	m_iName = AllocPooledString(newName);
}

inline bool CBaseEntity::NameMatches( const char *pszNameOrWildcard )
{
	if ( IDENT_STRINGS(m_iName, pszNameOrWildcard) )
		return true;
	return NameMatchesComplex( pszNameOrWildcard );
}

inline bool CBaseEntity::NameMatches( string_t nameStr )
{
	if ( IDENT_STRINGS(m_iName, nameStr) )
		return true;
	return NameMatchesComplex( STRING(nameStr) );
}

inline bool CBaseEntity::ClassMatches( const char *pszClassOrWildcard )
{
	if ( IDENT_STRINGS(m_iClassname, pszClassOrWildcard ) )
		return true;
	return ClassMatchesComplex( pszClassOrWildcard );
}

inline bool CBaseEntity::NameMatchesExact( string_t nameStr )
{
	return IDENT_STRINGS(m_iName, nameStr);
}

inline bool CBaseEntity::ClassMatchesExact( string_t nameStr )
{
	return IDENT_STRINGS(m_iClassname, nameStr );
}

inline bool CBaseEntity::ClassMatchesExact( const CGameString &nameStr )
{
	string_t tmp = nameStr.Get();
	return IDENT_STRINGS(m_iClassname, tmp);
}

inline bool CBaseEntity::ClassMatchesExact( const char *nameStr )
{
	if ( IDENT_STRINGS(m_iClassname, nameStr ) )
		return true;
	return V_strcmp( STRING(m_iClassname), nameStr ) == 0;
}

inline const char* CBaseEntity::GetClassname()
{
	return STRING(m_iClassname);
}

inline string_t CBaseEntity::GetClassnameStr()
{
	return m_iClassname;
}

inline bool CBaseEntity::ClassMatches( string_t nameStr )
{
	if ( IDENT_STRINGS(m_iClassname, nameStr ) )
		return true;
	return ClassMatchesComplex( STRING(nameStr) );
}

template <typename T>
inline bool CBaseEntity::Downcast( string_t iszClass, T **ppResult )
{
	if ( IDENT_STRINGS( iszClass, m_iClassname ) )
	{
		*ppResult = static_cast< T* >( this );
		return true;
	}
	*ppResult = NULL;
	return false;
}

//-----------------------------------------------------------------------------
// checks to see if the entity is marked for deletion
//-----------------------------------------------------------------------------
inline bool CBaseEntity::IsMarkedForDeletion( void ) 
{ 
	return (m_iEFlags & EFL_KILLME); 
}

inline void CBaseEntity::MarkForDeletion()
{
	AddEFlags( EFL_KILLME );
}

//-----------------------------------------------------------------------------
// EFlags
//-----------------------------------------------------------------------------
inline EntityFlags_t CBaseEntity::GetEFlags() const
{
	return m_iEFlags;
}

inline void CBaseEntity::AddEFlags( EntityFlags_t nEFlagMask )
{
	m_iEFlags |= nEFlagMask;

	if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
	{
		DispatchUpdateTransmitState();
	}
}

inline void CBaseEntity::RemoveEFlags( EntityFlags_t nEFlagMask )
{
	nEFlagMask &= ~(
		EFL_KILLME|
		EFL_NOT_RENDERABLE|
		EFL_NOT_NETWORKED|
		EFL_NOT_COLLIDEABLE
	);

	m_iEFlags &= ~nEFlagMask;
	
	if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
		DispatchUpdateTransmitState();
}

inline bool CBaseEntity::IsEFlagSet( EntityFlags_t nEFlagMask ) const
{
	return (m_iEFlags & nEFlagMask) != EFL_NONE;
}

inline bool CBaseEntity::AllowNavIgnore()
{
	return m_bAllowNavIgnore;
}

inline void	CBaseEntity::SetAllowNavIgnore( bool bAllowNavIgnore )
{
	m_bAllowNavIgnore = bAllowNavIgnore;
}

inline void	CBaseEntity::SetNavIgnore( float duration )
{
	float flNavIgnoreUntilTime = ( duration == FLT_MAX ) ? FLT_MAX : gpGlobals->curtime + duration;
	if ( flNavIgnoreUntilTime > m_flNavIgnoreUntilTime )
		m_flNavIgnoreUntilTime = flNavIgnoreUntilTime;
}

inline void	CBaseEntity::SetAlwaysNavIgnore( bool bAlwaysNavIgnore )
{
	m_bAlwaysIgnoreNav = bAlwaysNavIgnore;
}

inline bool CBaseEntity::AlwaysNavIgnore()
{
	return m_bAlwaysIgnoreNav;
}

inline void	CBaseEntity::ClearNavIgnore()
{
	m_flNavIgnoreUntilTime = 0;
}

inline bool	CBaseEntity::IsNavIgnored() const
{
	return ( gpGlobals->curtime <= m_flNavIgnoreUntilTime );
}

inline float CBaseEntity::GetViewDistance()
{ 
	return m_fViewDistance; 
}

inline void CBaseEntity::SetViewDistance( float dist ) 
{ 
	m_fViewDistance = dist; 
}

inline bool CBaseEntity::GetCheckUntouch() const
{
	return IsEFlagSet( EFL_CHECK_UNTOUCH );
}

//-----------------------------------------------------------------------------
// Network state optimization
//-----------------------------------------------------------------------------
inline CBaseCombatCharacter *ToBaseCombatCharacter( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	return pEntity->MyCombatCharacterPointer();
}


//-----------------------------------------------------------------------------
// Physics state accessor methods
//-----------------------------------------------------------------------------
inline const Vector& CBaseEntity::GetLocalOrigin( void ) const
{
	return m_vecOrigin.Get();
}

inline const QAngle& CBaseEntity::GetLocalAngles( void ) const
{
	return m_angRotation.Get();
}

inline const Vector& CBaseEntity::GetAbsOrigin( void ) const
{
	Assert( CBaseEntity::IsAbsQueriesValid() );

	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_vecAbsOrigin;
}

inline const QAngle& CBaseEntity::GetAbsAngles( void ) const
{
	Assert( CBaseEntity::IsAbsQueriesValid() );

	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_angAbsRotation;
}

inline float CBaseEntity::GetMass()
{
	IPhysicsObject *vPhys = VPhysicsGetObject();
	if (vPhys)
	{
		return vPhys->GetMass();
	}
	else
	{
		Log_Warning( LOG_BASEENTITY, "Tried to call GetMass() on %s but it has no physics.\n", GetDebugName());
		return 0;
	}
}

inline void CBaseEntity::SetMass(float mass)
{
	mass = clamp(mass, VPHYSICS_MIN_MASS, VPHYSICS_MAX_MASS);

	IPhysicsObject *vPhys = VPhysicsGetObject();
	if (vPhys)
	{
		vPhys->SetMass(mass);
	}
	else
	{
		Log_Warning( LOG_BASEENTITY, "Tried to call SetMass() on %s but it has no physics.\n", GetDebugName());
	}
}

//-----------------------------------------------------------------------------
// Returns the entity-to-world transform
//-----------------------------------------------------------------------------
inline matrix3x4_t &CBaseEntity::EntityToWorldTransform() 
{ 
	Assert( CBaseEntity::IsAbsQueriesValid() );

	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}

inline const matrix3x4_t &CBaseEntity::EntityToWorldTransform() const
{ 
	Assert( CBaseEntity::IsAbsQueriesValid() );

	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}


//-----------------------------------------------------------------------------
// Some helper methods that transform a point from entity space to world space + back
//-----------------------------------------------------------------------------
inline void CBaseEntity::EntityToWorldSpace( const Vector &in, Vector *pOut ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorAdd( in, GetAbsOrigin(), *pOut );
	}
	else
	{
		VectorTransform( in, EntityToWorldTransform(), *pOut );
	}
}

inline void CBaseEntity::WorldToEntitySpace( const Vector &in, Vector *pOut ) const
{
	if ( GetAbsAngles() == vec3_angle )
	{
		VectorSubtract( in, GetAbsOrigin(), *pOut );
	}
	else
	{
		VectorITransform( in, EntityToWorldTransform(), *pOut );
	}
}


//-----------------------------------------------------------------------------
// Velocity
//-----------------------------------------------------------------------------
inline Vector CBaseEntity::GetSmoothedVelocity( void )
{
	Vector vel;
	GetVelocity( &vel, NULL );
	return vel;
}

inline const Vector &CBaseEntity::GetLocalVelocity( ) const
{
	return m_vecVelocity.Get();
}

inline const Vector &CBaseEntity::GetAbsVelocity( ) const
{
	Assert( CBaseEntity::IsAbsQueriesValid() );

	if (IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsoluteVelocity();
	}
	return m_vecAbsVelocity;
}

inline const QAngle &CBaseEntity::GetLocalAngularVelocity( ) const
{
	return m_vecAngVelocity;
}

/*
// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
inline const QAngle &CBaseEntity::GetAbsAngularVelocity( ) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSANGVELOCITY))
	{
		const_cast<CBaseEntity*>(this)->CalcAbsoluteAngularVelocity();
	}

	return m_vecAbsAngVelocity;
}
*/

inline const Vector& CBaseEntity::GetBaseVelocity() const 
{ 
	return m_vecBaseVelocity.Get(); 
}

inline void CBaseEntity::SetBaseVelocity( const Vector& v ) 
{ 
	m_vecBaseVelocity = v; 
}

inline float CBaseEntity::GetGravity( void ) const
{ 
	return m_flGravity; 
}

inline void CBaseEntity::SetGravity( float gravity )
{ 
	m_flGravity = gravity; 
}

inline float CBaseEntity::GetFriction( void ) const
{ 
	return m_flFriction; 
}

inline void	CBaseEntity::SetElasticity( float flElasticity )
{ 
	m_flElasticity = flElasticity; 
}

inline float CBaseEntity::GetElasticity( void )	const			
{ 
	return m_flElasticity; 
}

inline void	CBaseEntity::SetShadowCastDistance( float flDistance )
{ 
	m_flShadowCastDistance = flDistance; 
}

inline float CBaseEntity::GetShadowCastDistance( void )	const			
{ 
	return m_flShadowCastDistance; 
}

inline float CBaseEntity::GetLocalTime( void ) const
{ 
	return m_flLocalTime; 
}

inline void CBaseEntity::IncrementLocalTime( float flTimeDelta )
{ 
	m_flLocalTime += flTimeDelta; 
}

inline float CBaseEntity::GetMoveDoneTime( ) const
{
	return (m_flMoveDoneTime >= 0) ? m_flMoveDoneTime - GetLocalTime() : -1;
}

inline CBaseEntity *CBaseEntity::Instance( const edict_t *pent )
{
	return GetContainingEntity( const_cast<edict_t*>(pent) );
}

inline CBaseEntity *CBaseEntity::Instance( edict_t *pent ) 
{ 
	if ( !pent )
	{
		pent = INDEXENT(0);
	}
	return GetContainingEntity( pent );
}

inline CBaseEntity* CBaseEntity::Instance( int iEnt )
{
	return Instance( INDEXENT( iEnt ) );
}

inline WaterLevel_t CBaseEntity::GetWaterLevel() const
{
	return m_nWaterLevel;
}

inline void CBaseEntity::SetWaterLevel( WaterLevel_t nLevel )
{
	m_nWaterLevel = nLevel;
}

inline const color24 CBaseEntity::GetRenderColor() const
{
	color24 c( m_clrRender.GetR(), m_clrRender.GetG(), m_clrRender.GetB() );
	return c;
}

inline byte CBaseEntity::GetRenderColorR() const
{
	return m_clrRender.GetR();
}

inline byte CBaseEntity::GetRenderColorG() const
{
	return m_clrRender.GetG();
}

inline byte CBaseEntity::GetRenderColorB() const
{
	return m_clrRender.GetB();
}

inline void CBaseEntity::SetRenderAlpha( byte a )
{
	m_clrRender.SetA( a );
}

inline byte CBaseEntity::GetRenderAlpha( ) const
{
	return m_clrRender.GetA();
}

inline void CBaseEntity::SetRenderColor( byte r, byte g, byte b )
{
	m_clrRender.Init( r, g, b );
}

inline void CBaseEntity::SetRenderColor( color24 clr )
{
	m_clrRender.Init( clr.r(), clr.g(), clr.b() );
}

inline void CBaseEntity::SetMoveCollide( MoveCollide_t val )
{ 
	m_MoveCollide = val; 
}

inline int	CBaseEntity::GetTextureFrameIndex( void )
{
	return m_iTextureFrameIndex;
}

inline void CBaseEntity::SetTextureFrameIndex( int iIndex )
{
	m_iTextureFrameIndex = iIndex;
}

inline void	CBaseEntity::EyePositionZOnly( Vector *pPosition )
{
	*pPosition = EyePosition();
	Vector vecAbsOrigin = GetAbsOrigin();
	pPosition->x = vecAbsOrigin.x;
	pPosition->y = vecAbsOrigin.y;
}

//-----------------------------------------------------------------------------
// An inline version the game code can use
//-----------------------------------------------------------------------------
inline CCollisionProperty *CBaseEntity::CollisionProp()
{
	return m_pCollision.GetWithoutModifyRaw();
}

inline const CCollisionProperty *CBaseEntity::CollisionProp() const
{
	return m_pCollision;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
inline DensityWeightsMap *CBaseEntity::DensityMap()
{
	return m_pDensityMap;
}

inline CBaseEntity *CBaseEntity::GetBaseEntity()
{
	return this;
}
	

//-----------------------------------------------------------------------------
// Model related methods
//-----------------------------------------------------------------------------
inline void CBaseEntity::SetModelName( string_t name )
{
	m_ModelName = name;
	DispatchUpdateTransmitState();
}

inline string_t CBaseEntity::GetModelName( void ) const
{
	return m_ModelName;
}

inline modelindex_t CBaseEntity::GetModelIndex( void ) const
{
	return m_nModelIndex.Get();
}

//-----------------------------------------------------------------------------
// AddOn related methods
//-----------------------------------------------------------------------------

inline void CBaseEntity::SetAIAddOn( string_t addonName )
{
	m_AIAddOn = addonName;
}

inline string_t CBaseEntity::GetAIAddOn( void ) const
{
	return m_AIAddOn;
}

inline void CBaseEntity::SetRenderMode( RenderMode_t nRenderMode )
{
	m_nRenderMode = nRenderMode;
}

inline RenderMode_t CBaseEntity::GetRenderMode() const
{
	return (RenderMode_t)m_nRenderMode.Get();
}

inline void CBaseEntity::SetRenderFX( RenderFx_t nRenderFX )
{
	m_nRenderFX = nRenderFX;
}

inline RenderFx_t CBaseEntity::GetRenderFX() const
{
	return (RenderFx_t)m_nRenderFX.Get();
}

inline float CBaseEntity::GetDistanceToEntity( const CBaseEntity *other ) const
{
	if( other == NULL )
		return -1.0f;

	return ( GetAbsOrigin() - other->GetAbsOrigin() ).Length();
}

//-----------------------------------------------------------------------------
// Methods to cast away const
//-----------------------------------------------------------------------------
inline Vector CBaseEntity::EyePosition( void ) const
{
	return const_cast<CBaseEntity*>(this)->EyePosition();
}

inline const QAngle &CBaseEntity::EyeAngles( void ) const		// Direction of eyes in world space
{
	return const_cast<CBaseEntity*>(this)->EyeAngles();
}

inline const QAngle &CBaseEntity::LocalEyeAngles( void ) const	// Direction of eyes
{
	return const_cast<CBaseEntity*>(this)->LocalEyeAngles();
}

inline Vector	CBaseEntity::EarPosition( void ) const			// position of ears
{
	return const_cast<CBaseEntity*>(this)->EarPosition();
}


//-----------------------------------------------------------------------------
// IHandleEntity overrides.
//-----------------------------------------------------------------------------
inline const EHANDLE& CBaseEntity::GetRefEHandle() const
{
	return m_RefEHandle;
}

inline void CBaseEntity::IncrementTransmitStateOwnedCounter()
{
	Assert( m_nTransmitStateOwnedCounter != 255 );
	m_nTransmitStateOwnedCounter++;
}

inline void CBaseEntity::DecrementTransmitStateOwnedCounter()
{
	Assert( m_nTransmitStateOwnedCounter != 0 );
	m_nTransmitStateOwnedCounter--;
}

inline const ResponseContext_t	*CBaseEntity::GetContextData( int index ) const
{
	return &m_ResponseContexts[index];
}

// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), 0, NULL )
#define SetContextThink( a, b, context ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), (b), context )

#ifdef _DEBUG
#define SetMoveDone( a ) \
	do \
	{ \
		m_pfnMoveDone = static_cast <void (CBaseEntity::*)(void)> (a); \
		FunctionCheck( (void *)*((int *)((char *)this + ( offsetof(CBaseEntity,m_pfnMoveDone)))), "BaseMoveFunc" ); \
	} while ( 0 )
#else
#define SetMoveDone( a ) \
		(void)(m_pfnMoveDone = static_cast <void (CBaseEntity::*)(void)> (a))
#endif


inline bool FClassnameIs(CBaseEntity *pEntity, const char *szClassname)
{ 
	return pEntity->ClassMatchesExact(szClassname); 
}

inline bool FClassnameIs(CBaseEntity *pEntity, string_t szClassname)
{ 
	return pEntity->ClassMatchesExact(szClassname); 
}

inline bool FClassnameIs(CBaseEntity *pEntity, const CGameString &szClassname)
{ 
	return pEntity->ClassMatchesExact(szClassname); 
}

#include "baseentity_shared.h"

class CPointEntity : public CBaseEntity
{
public:
	DECLARE_CLASS( CPointEntity, CBaseEntity );
	DECLARE_SERVERCLASS();

	CPointEntity() : CPointEntity( EFL_NONE ) {}
	CPointEntity( EntityFlags_t iEFlags );

	ICollideable	*GetCollideable() { return NULL; }
	CCollisionProperty		*CollisionProp() { return NULL; }
	const CCollisionProperty*CollisionProp() const { return NULL; }

	void	Spawn( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	EdictStateFlags_t UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

typedef CPointEntity CSharedPointEntity;

// Has no position or size
class CLogicalEntity : public CPointEntity
{
public:
	DECLARE_CLASS( CLogicalEntity, CPointEntity );

	DECLARE_SERVERCLASS();

	CLogicalEntity() : CLogicalEntity( EFL_NONE ) {}
	CLogicalEntity( EntityFlags_t iEFlags );

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
};

typedef CLogicalEntity CSharedLogicalEntity;

template <typename T>
class CServerOnlyWrapper : public T
{
public:
	DECLARE_CLASS( CServerOnlyWrapper, T );
	CServerOnlyWrapper() : CServerOnlyWrapper( EFL_NONE ) {}
	CServerOnlyWrapper( EntityFlags_t iEFlags ) : T( EFL_NOT_NETWORKED|iEFlags ) {}

	virtual void	NetworkStateChanged() {}
	virtual void	NetworkStateChanged( void *pVar ) {}

	virtual ServerClass* GetServerClass() { return NULL; }

	virtual IServerNetworkable *GetNetworkable() { return NULL; }

	CServerNetworkProperty *NetworkProp() { return NULL; }
	const CServerNetworkProperty *NetworkProp() const { return NULL; }

	edict_t			*edict( void ) { return NULL; }
	const edict_t	*edict( void ) const { return NULL; }

	virtual EdictStateFlags_t				ShouldTransmit( const CCheckTransmitInfo *pInfo ) { return FL_EDICT_DONTSEND; }

	EdictStateFlags_t UpdateTransmitState()
	{
		return FL_EDICT_DONTSEND;
	}
};

typedef CServerOnlyWrapper<CBaseEntity> CServerOnlyEntity;

typedef CServerOnlyEntity CLocalOnlyEntity;

// Has only a position, no size
typedef CServerOnlyWrapper<CPointEntity> CServerOnlyPointEntity;

typedef CServerOnlyPointEntity CLocalOnlyPointEntity;

// Has no position or size
typedef CServerOnlyWrapper<CLogicalEntity> CServerOnlyLogicalEntity;

typedef CServerOnlyLogicalEntity CLocalOnlyLogicalEntity;

// Network proxy functions

void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_OriginXY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_OriginZ( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

class CAbsQueryScopeGuard
{
public:
	CAbsQueryScopeGuard( bool state )
	{
		m_bSavedState = CBaseEntity::IsAbsQueriesValid();
		CBaseEntity::SetAbsQueriesValid( state );
	}
	~CAbsQueryScopeGuard()
	{
		CBaseEntity::SetAbsQueriesValid( m_bSavedState );
	}
private:
	bool	m_bSavedState;
};

#define ABS_QUERY_GUARD( state ) CAbsQueryScopeGuard s_AbsQueryGuard( state );

bool EntityNamesMatch( const char *pszQuery, string_t nameToMatch );

#endif // BASEENTITY_H
