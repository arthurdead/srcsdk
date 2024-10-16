//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "decals.h"
#include "model_types.h"
#include "IEffects.h"
#include "util_shared.h"

#if !defined( CLIENT_DLL )
#include "ndebugoverlay.h"
#include "sendproxy.h"
#else
#include "iviewrender_beams.h"
#include "c_pixel_visibility.h"
#include "iclientmode.h"
#include "viewrender.h"
#include "view.h"

#ifdef PORTAL
	#include "c_prop_portal.h"
#endif //ifdef PORTAL

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BEAM_DEFAULT_HALO_SCALE		10

#if !defined( CLIENT_DLL )
// Lightning target, just alias landmark

class CInfoTarget : public CPointEntity
{
public:
	DECLARE_CLASS( CInfoTarget, CPointEntity );

	void	Spawn( void );
	virtual int UpdateTransmitState();
};

//info targets are like point entities except you can force them to spawn on the client
void CInfoTarget::Spawn( void )
{
	BaseClass::Spawn();

	if ( HasSpawnFlags(0x01) )
	{
		SetEFlags( EFL_FORCE_CHECK_TRANSMIT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CInfoTarget::UpdateTransmitState()
{
	// Spawn flags 2 means we always transmit
	if ( HasSpawnFlags(0x02) )
		return SetTransmitState( FL_EDICT_ALWAYS );
	return BaseClass::UpdateTransmitState();
}

LINK_ENTITY_TO_CLASS( info_target, CInfoTarget );
#endif


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given entity is a fixed target for lightning.
//-----------------------------------------------------------------------------
bool IsStaticPointEntity( CSharedBaseEntity *pEnt )
{
	if ( pEnt->GetMoveParent() )
		return false;

	if ( !pEnt->GetModelIndex() )
		return 1;

	if ( FClassnameIs( pEnt, "info_target" ) || FClassnameIs( pEnt, "info_landmark" ) || 
		FClassnameIs( pEnt, "path_corner" ) )
		return true;

	return false;
}

#if defined( CLIENT_DLL )
extern bool ComputeBeamEntPosition( C_BaseEntity *pEnt, int nAttachment, bool bInterpretAttachmentIndexAsHitboxIndex, Vector& pt );

void RecvProxy_Beam_ScrollSpeed( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Beam *beam;
	float	val;

	// Unpack the data.
	val	= pData->m_Value.m_Float;
	val *= 0.1;

	beam = ( C_Beam * )pStruct;
	Assert( pOut == &beam->m_fSpeed );

	beam->m_fSpeed = val;
}
#else
void* SendProxy_SendPredictableId( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;
	if ( !pEntity || !pEntity->m_PredictableID->IsActive() )
		return NULL;

	if ( !pEntity->GetOwnerEntity() )
		return NULL;

	CBaseEntity *owner = pEntity->GetOwnerEntity();
	if ( !owner || !owner->IsPlayer() )
		return NULL;

	CBasePlayer *pOwner = static_cast< CBasePlayer * >( owner );
	if ( !pOwner )
		return NULL;

	int id_player_index = pEntity->m_PredictableID->GetPlayer();
	int owner_player_index = pOwner->entindex() - 1;
	// Only send to owner player
	// FIXME:  Is this ever not the case due to the SetOnly call?
	if ( id_player_index != owner_player_index )
		return NULL;

	pRecipients->SetOnly( owner_player_index );
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendPredictableId );
#endif

LINK_ENTITY_TO_CLASS_ALIASED( beam, Beam );

// This table encodes the CBeam data.
IMPLEMENT_NETWORKCLASS_ALIASED( Beam, DT_Beam )

BEGIN_NETWORK_TABLE_NOBASE( CSharedBeam, DT_BeamPredictableId )
#if !defined( CLIENT_DLL )
	SendPropPredictableId( SENDINFO( m_PredictableID ) ),
	SendPropInt( SENDINFO( m_bIsPlayerSimulated ), 1, SPROP_UNSIGNED ),
#else
	RecvPropPredictableId( RECVINFO( m_PredictableID ) ),
	RecvPropInt( RECVINFO( m_bIsPlayerSimulated ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( CSharedBeam, DT_Beam )
#if !defined( CLIENT_DLL )
	SendPropInt		(SENDINFO(m_nBeamType),		Q_log2(NUM_BEAM_TYPES)+1,	SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_nBeamFlags),	NUM_BEAM_FLAGS,	SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_nNumBeamEnts ),			5,	SPROP_UNSIGNED ),
	SendPropArray3
	(
		SENDINFO_ARRAY3(m_hAttachEntity), 
		SendPropEHandle( SENDINFO_ARRAY(m_hAttachEntity) )
	),
	SendPropArray3
	(
		SENDINFO_ARRAY3(m_nAttachIndex), 
		SendPropInt( SENDINFO_ARRAY(m_nAttachIndex), ATTACHMENT_INDEX_BITS, SPROP_UNSIGNED)
	),
	SendPropInt		(SENDINFO(m_nHaloIndex),	16, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_fHaloScale),	0,	SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_fWidth),		10,	SPROP_ROUNDUP,	0.0f, MAX_BEAM_WIDTH ),
	SendPropFloat	(SENDINFO(m_fEndWidth),		10,	SPROP_ROUNDUP,	0.0f, MAX_BEAM_WIDTH ),
	SendPropFloat	(SENDINFO(m_fFadeLength),	0,	SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_fAmplitude),	8,	SPROP_ROUNDDOWN,	0.0f, MAX_BEAM_NOISEAMPLITUDE ),
	SendPropFloat	(SENDINFO(m_fStartFrame),	8,	SPROP_ROUNDDOWN,	0.0f,   256.0f),
	SendPropFloat	(SENDINFO(m_fSpeed),		8,	SPROP_NOSCALE,	0.0f,	MAX_BEAM_SCROLLSPEED),
	SendPropFloat	(SENDINFO(m_flFrameRate),	10, SPROP_ROUNDUP, -25.0f, 25.0f ),
	SendPropFloat	(SENDINFO(m_flHDRColorScale),	0, SPROP_NOSCALE, 0.0f, 100.0f ),
	SendPropFloat	(SENDINFO(m_flFrame),		20, SPROP_ROUNDDOWN | SPROP_CHANGES_OFTEN,	0.0f,   256.0f),
	SendPropInt		(SENDINFO(m_nClipStyle), CSharedBeam::kBEAMCLIPSTYLE_NUMBITS+1, SPROP_UNSIGNED ),
	SendPropVector	(SENDINFO(m_vecEndPos),		-1,	SPROP_COORD ),
#ifdef PORTAL
	SendPropBool	(SENDINFO(m_bDrawInMainRender) ),
	SendPropBool	(SENDINFO(m_bDrawInPortalRender) ),
#endif
	SendPropModelIndex(SENDINFO(m_nModelIndex) ),
	SendPropVector (SENDINFO(m_vecOrigin), 19, SPROP_CHANGES_OFTEN,	MIN_COORD_INTEGER, MAX_COORD_INTEGER),
	SendPropEHandle(SENDINFO_NAME(m_hMoveParent, moveparent) ),
	SendPropInt		(SENDINFO(m_nMinDXLevel),	8,	SPROP_UNSIGNED ),

	SendPropDataTable( "beampredictable_id", 0, &REFERENCE_SEND_TABLE( DT_BeamPredictableId ), SendProxy_SendPredictableId ),

#else
	RecvPropInt		(RECVINFO(m_nBeamType)),
	RecvPropInt		(RECVINFO(m_nBeamFlags)),
	RecvPropInt		(RECVINFO(m_nNumBeamEnts)),
	RecvPropArray3
	(
		RECVINFO_ARRAY( m_hAttachEntity ),
		RecvPropEHandle (RECVINFO_ARRAYELEM(m_hAttachEntity, 0))
	),
	RecvPropArray3	
	(
		RECVINFO_ARRAY( m_nAttachIndex ),
		RecvPropInt (RECVINFO_ARRAYELEM(m_nAttachIndex, 0))
	),
	RecvPropInt		(RECVINFO(m_nHaloIndex)),
	RecvPropFloat	(RECVINFO(m_fHaloScale)),
	RecvPropFloat	(RECVINFO(m_fWidth)),
	RecvPropFloat	(RECVINFO(m_fEndWidth)),
	RecvPropFloat	(RECVINFO(m_fFadeLength)),
	RecvPropFloat	(RECVINFO(m_fAmplitude)),
	RecvPropFloat	(RECVINFO(m_fStartFrame)),
	RecvPropFloat	(RECVINFO(m_fSpeed), 0, RecvProxy_Beam_ScrollSpeed ),
	RecvPropFloat(RECVINFO(m_flFrameRate)),
	RecvPropFloat(RECVINFO(m_flHDRColorScale)),
	RecvPropFloat(RECVINFO(m_flFrame)),
	RecvPropInt(RECVINFO(m_nClipStyle)),
	RecvPropVector(RECVINFO(m_vecEndPos)),
#ifdef PORTAL
	RecvPropBool(RECVINFO(m_bDrawInMainRender) ),
	RecvPropBool(RECVINFO(m_bDrawInPortalRender) ),
#endif
	RecvPropInt(RECVINFO(m_nModelIndex)),
	RecvPropInt(RECVINFO(m_nMinDXLevel)),

	RecvPropVector(RECVINFO_NAME(m_vecNetworkOrigin, m_vecOrigin)),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),

	RecvPropDataTable( "beampredictable_id", 0, 0, &REFERENCE_RECV_TABLE( DT_BeamPredictableId ) ),

#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )
BEGIN_MAPENTITY( CBeam )
	
	DEFINE_KEYFIELD( m_flHDRColorScale, FIELD_FLOAT, "HDRColorScale" ),

	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),

	DEFINE_KEYFIELD( m_nDissolveType, FIELD_INTEGER, "dissolvetype" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Width", InputWidth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Noise", InputNoise ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorRedValue", InputColorRedValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorGreenValue", InputColorGreenValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ColorBlueValue", InputColorBlueValue ),
	DEFINE_INPUT( m_fSpeed, FIELD_FLOAT, "ScrollSpeed" ),

END_MAPENTITY()

#else

BEGIN_PREDICTION_DATA( C_Beam )

	DEFINE_PRED_FIELD( m_nBeamType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	
	DEFINE_PRED_FIELD( m_nNumBeamEnts, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hAttachEntity, FIELD_EHANDLE, MAX_BEAM_ENTS, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_nAttachIndex, FIELD_INTEGER, MAX_BEAM_ENTS, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nHaloIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fHaloScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fWidth, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fEndWidth, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fFadeLength, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fAmplitude, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fStartFrame, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fSpeed, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	//TODO!!!! Arthurdead
	//DEFINE_PRED_FIELD( m_nRenderFX, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	//DEFINE_PRED_FIELD( m_nRenderMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_flFrameRate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flFrame, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nMinDXLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_vecEndPos, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
#ifdef PORTAL
	DEFINE_PRED_FIELD( m_bDrawInMainRender, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bDrawInPortalRender, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
	DEFINE_PRED_FIELD( m_nModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD_TOL( m_vecOrigin, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	
	//DEFINE_PRED_FIELD( m_pMoveParent, SendProxy_MoveParent ),
	//DEFINE_PRED_FIELD( m_flHDRColorScale, SendProxy_HDRColorScale ),

END_PREDICTION_DATA()

#endif

#if defined( CLIENT_DLL )
#define CBeam C_Beam
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBeam::CBeam( void )
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_vecEndPos.Init();
#endif

	m_nMinDXLevel = 0;
	m_flHDRColorScale = 1.0f; // default value.

#if !defined( CLIENT_DLL )
	m_nDissolveType = -1;
#else
	m_queryHandleHalo = 0;
#endif

#ifdef PORTAL
	m_bDrawInMainRender = true;
	m_bDrawInPortalRender = true;
#endif

#ifdef CLIENT_DLL
	AddToEntityList(ENTITY_LIST_SIMULATE);
#endif
}

#if defined( CLIENT_DLL )
#undef CBeam
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CSharedBeam::SetModel( const char *szModelName )
{
	int modelIndex = modelinfo->GetModelIndex( szModelName );
	const model_t *model = modelinfo->GetModel( modelIndex );
	if ( model && modelinfo->GetModelType( model ) != mod_sprite )
	{
		Msg( "Setting CBeam to non-sprite model %s\n", szModelName );
	}
#if !defined( CLIENT_DLL )
	UTIL_SetModel( this, szModelName );
#else
	BaseClass::SetModel( szModelName );
#endif
}


void CSharedBeam::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );							// Remove model & collisions
	SetRenderMode( kRenderTransTexture );

	// Opt out of all shadow routines
	AddEffects( EF_NOSHADOW | EF_NORECEIVESHADOW );

	Precache( );
}


void CSharedBeam::Precache( void )
{
	if ( GetOwnerEntity() )
	{
		SetStartEntity( GetOwnerEntity() );
	}
	
	if ( m_hEndEntity.Get() )
	{
		SetEndEntity( m_hEndEntity );
	}
}


void CSharedBeam::SetType( int type )		
{ 
	Assert( type < NUM_BEAM_TYPES );
	m_nBeamType = type;
}

void CSharedBeam::SetBeamFlags( int flags )	
{ 
	Assert( flags < (1 << NUM_BEAM_FLAGS) );
	m_nBeamFlags = flags;
}

void CSharedBeam::SetBeamFlag( int flag )		
{ 
	m_nBeamFlags |= flag;
}

int CSharedBeam::GetType( void ) const 
{ 
	return m_nBeamType;
}

int CSharedBeam::GetBeamFlags( void ) const
{ 
	return m_nBeamFlags;
}

void CSharedBeam::SetStartEntity( CSharedBaseEntity *pEntity )
{ 
	Assert( m_nNumBeamEnts >= 2 );
	m_hAttachEntity.Set( 0, pEntity );
	SetOwnerEntity( pEntity );
	RelinkBeam();
	if(pEntity)
		pEntity->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

void CSharedBeam::SetEndEntity( CSharedBaseEntity *pEntity ) 
{ 
	Assert( m_nNumBeamEnts >= 2 );
	m_hAttachEntity.Set( m_nNumBeamEnts-1, pEntity );
	m_hEndEntity = pEntity;
	RelinkBeam();
	if(pEntity)
		pEntity->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}


//-----------------------------------------------------------------------------
// This will change things so the abs position matches the requested spot
//-----------------------------------------------------------------------------
void CSharedBeam::SetAbsStartPos( const Vector &pos )
{
	if (!GetMoveParent())
	{
		SetStartPos( pos );
		return;
	}

	Vector vecLocalPos;
	matrix3x4_t worldToBeam;
	MatrixInvert( EntityToWorldTransform(), worldToBeam );
	VectorTransform( pos, worldToBeam, vecLocalPos );
	SetStartPos( vecLocalPos );
}

void CSharedBeam::SetAbsEndPos( const Vector &pos )
{
	if (!GetMoveParent())
	{
		SetEndPos( pos );
		return;
	}

	Vector vecLocalPos;
	matrix3x4_t worldToBeam;
	MatrixInvert( EntityToWorldTransform(), worldToBeam );
	VectorTransform( pos, worldToBeam, vecLocalPos );
	SetEndPos( vecLocalPos );
}

#if !defined( CLIENT_DLL )

// These don't take attachments into account
const Vector &CSharedBeam::GetAbsStartPos( void ) const
{
	if ( GetType() == BEAM_ENTS && GetStartEntity() )
	{
		edict_t *pent =  engine->PEntityOfEntIndex( GetStartEntity() );
		CBaseEntity *ent = CBaseEntity::Instance( pent );
		if ( !ent )
		{
			return GetAbsOrigin();
		}
		return ent->GetAbsOrigin();
	}
	return GetAbsOrigin();
}


const Vector &CSharedBeam::GetAbsEndPos( void ) const
{
	if ( GetType() != BEAM_POINTS && GetType() != BEAM_HOSE && GetEndEntity() ) 
	{
		edict_t *pent =  engine->PEntityOfEntIndex( GetEndEntity() );
		CBaseEntity *ent = CBaseEntity::Instance( pent );
		if ( ent )
			return ent->GetAbsOrigin();
	}

	if (!const_cast<CBeam*>(this)->GetMoveParent())
		return m_vecEndPos.Get();

	// FIXME: Cache this off?
	static Vector vecAbsPos;
	VectorTransform( m_vecEndPos, EntityToWorldTransform(), vecAbsPos );
	return vecAbsPos;
}

#else

//-----------------------------------------------------------------------------
// Unlike the server, these take attachments into account
//-----------------------------------------------------------------------------
const Vector &C_Beam::GetAbsStartPos( void ) const
{
	static Vector vecStartAbsPosition;
	if ( GetType() != BEAM_POINTS && GetType() != BEAM_HOSE ) 
	{
		if (ComputeBeamEntPosition( m_hAttachEntity[0], m_nAttachIndex[0], false, vecStartAbsPosition ))
			return vecStartAbsPosition;
	}

	return GetAbsOrigin();
}


const Vector &C_Beam::GetAbsEndPos( void ) const
{
	static Vector vecEndAbsPosition;
	if ( GetType() != BEAM_POINTS && GetType() != BEAM_HOSE ) 
	{
		if (ComputeBeamEntPosition( m_hAttachEntity[m_nNumBeamEnts-1], m_nAttachIndex[m_nNumBeamEnts-1], false, vecEndAbsPosition ))
			return vecEndAbsPosition;
	}

	if (!const_cast<C_Beam*>(this)->GetMoveParent())
		return m_vecEndPos.Get();

	// FIXME: Cache this off?
	VectorTransform( m_vecEndPos, EntityToWorldTransform(), vecEndAbsPosition );
	return vecEndAbsPosition;
}
#endif

CSharedBeam *CSharedBeam::BeamCreate( const char *pSpriteName, float width )
{
	// Create a new entity with CBeam private data
	CSharedBeam *pBeam  = CREATE_ENTITY( CSharedBeam, "beam" );
	pBeam->BeamInit( pSpriteName, width );

	return pBeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSprite
//-----------------------------------------------------------------------------
CSharedBeam *CSharedBeam::BeamCreatePredictable( const char *module, int line, const char *pSpriteName, float width, CSharedBasePlayer *pOwner )
{
	CSharedBeam *pBeam = ( CSharedBeam * )CSharedBaseEntity::CreatePredicted( module, line, "beam", vec3_origin, vec3_angle, pOwner );
	if ( pBeam )
	{
		pBeam->BeamInit( pSpriteName, width );
		pBeam->SetPlayerSimulated( pOwner );
	}

	return pBeam;
}

void CSharedBeam::BeamInit( const char *pSpriteName, float width )
{
	SetColor( 255, 255, 255 );
	SetBrightness( 255 );
	SetNoise( 0 );
	SetFrame( 0 );
	SetScrollRate( 0 );
	SetModelName( MAKE_STRING( pSpriteName ) );
	SetRenderMode( kRenderTransTexture );
	SetTexture( PrecacheModel( pSpriteName ) );
	SetWidth( width );
	SetEndWidth( width );
	SetFadeLength( 0 );			// No fade
	for (int i=0;i<MAX_BEAM_ENTS;i++)
	{
		m_hAttachEntity.Set( i, NULL );
		m_nAttachIndex.Set( i, 0 );
	}
	m_nHaloIndex	= 0;
	m_fHaloScale	= BEAM_DEFAULT_HALO_SCALE;
	m_nBeamType		= 0;
	m_nBeamFlags    = 0;
}


void CSharedBeam::PointsInit( const Vector &start, const Vector &end )
{
	SetType( BEAM_POINTS );
	m_nNumBeamEnts = 2;
	SetStartPos( start );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}


void CSharedBeam::HoseInit( const Vector &start, const Vector &direction )
{
	SetType( BEAM_HOSE );
	m_nNumBeamEnts = 2;
	SetStartPos( start );
	SetEndPos( direction );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}


void CSharedBeam::PointEntInit( const Vector &start, CSharedBaseEntity *pEndEntity )
{
	SetType( BEAM_ENTPOINT );
	m_nNumBeamEnts = 2;
	SetStartPos( start );
	SetEndEntity( pEndEntity );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CSharedBeam::EntsInit( CSharedBaseEntity *pStartEntity, CSharedBaseEntity *pEndEntity )
{
	SetType( BEAM_ENTS );
	m_nNumBeamEnts = 2;
	SetStartEntity( pStartEntity );
	SetEndEntity( pEndEntity );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CSharedBeam::LaserInit( CSharedBaseEntity *pStartEntity, CSharedBaseEntity *pEndEntity )
{
	SetType( BEAM_LASER );
	m_nNumBeamEnts = 2;
	SetStartEntity( pStartEntity );
	SetEndEntity( pEndEntity );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CSharedBeam::SplineInit( int nNumEnts, CSharedBaseEntity** pEntList, int *attachment )
{
	if (nNumEnts < 2)
	{
		Msg("ERROR: Min of 2 ents required for spline beam.\n");
	}
	else if (nNumEnts > MAX_BEAM_ENTS)
	{
		Msg("ERROR: Max of %i ents allowed for spline beam.\n",MAX_BEAM_ENTS);
	}
	SetType( BEAM_SPLINE );

	for (int i=0;i<nNumEnts;i++)
	{
		m_hAttachEntity.Set( i, pEntList[i] ); 
		m_nAttachIndex.Set( i, attachment[i] );
	}
	m_nNumBeamEnts = nNumEnts;
	RelinkBeam();
}


void CSharedBeam::RelinkBeam( void )
{
	// FIXME: Why doesn't this just define the absbox too?
	// It seems that we don't need to recompute the absbox
	// in CBaseEntity::SetObjectCollisionBox, in fact the absbox
	// computed there seems way too big
	Vector startPos = GetAbsStartPos(), endPos = GetAbsEndPos();

	Vector vecAbsExtra1, vecAbsExtra2;
	bool bUseExtraPoints = false;

#ifdef PORTAL
	CBaseEntity *pStartEntity = GetStartEntityPtr();
	
	CTraceFilterSkipClassname traceFilter( pStartEntity, "prop_energy_ball", COLLISION_GROUP_NONE );
	
	ITraceFilter *pEntityBeamTraceFilter = NULL;
	if ( pStartEntity )
		pEntityBeamTraceFilter = pStartEntity->GetBeamTraceFilter();

	CTraceFilterChain traceFilterChain( &traceFilter, pEntityBeamTraceFilter );

	bUseExtraPoints = UTIL_Portal_Trace_Beam( this, startPos, endPos, vecAbsExtra1, vecAbsExtra2, &traceFilterChain );
#endif

	// UNDONE: Should we do this to make the boxes smaller?
	//SetAbsOrigin( startPos );

	Vector vecBeamMin, vecBeamMax;
	VectorMin( startPos, endPos, vecBeamMin );
	VectorMax( startPos, endPos, vecBeamMax );

	if ( bUseExtraPoints )
	{
		VectorMin( vecBeamMin, vecAbsExtra1, vecBeamMin );
		VectorMin( vecBeamMin, vecAbsExtra2, vecBeamMin );
		VectorMax( vecBeamMax, vecAbsExtra1, vecBeamMax );
		VectorMax( vecBeamMax, vecAbsExtra2, vecBeamMax );
	}

	SetCollisionBounds( vecBeamMin - GetAbsOrigin(), vecBeamMax - GetAbsOrigin() );
}


CSharedBaseEntity *CSharedBeam::RandomTargetname( const char *szName )
{
#if !defined( CLIENT_DLL )
	int total = 0;

	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while ((pNewEntity = gEntList.FindEntityByName( pNewEntity, szName )) != NULL)
	{
		total++;
		if (random->RandomInt(0,total-1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
#else
	return NULL;
#endif
}


void CSharedBeam::DoSparks( const Vector &start, const Vector &end )
{
#if !defined( CLIENT_DLL )
	if ( HasSpawnFlags(SF_BEAM_SPARKSTART|SF_BEAM_SPARKEND) )
	{
		if ( HasSpawnFlags( SF_BEAM_SPARKSTART ) )
		{
			g_pEffects->Sparks( start );
		}
		if ( HasSpawnFlags( SF_BEAM_SPARKEND ) )
		{
			g_pEffects->Sparks( end );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Damages anything in the beam.
// Input  : ptr - 
//-----------------------------------------------------------------------------
void CSharedBeam::BeamDamage( trace_t *ptr )
{
	RelinkBeam();
#if !defined( CLIENT_DLL )
	if ( ptr->fraction != 1.0 && ptr->m_pEnt != NULL )
	{
		CBaseEntity *pHit = ptr->m_pEnt;

		ClearMultiDamage();
		Vector dir = ptr->endpos - GetAbsOrigin();
		VectorNormalize( dir );
		int nDamageType = DMG_ENERGYBEAM;

		if (m_nDissolveType == 0)
		{
			nDamageType = DMG_DISSOLVE;
		}
		else if ( m_nDissolveType > 0 )
		{
			nDamageType = DMG_DISSOLVE | DMG_SHOCK; 
		}

		CTakeDamageInfo info( this, this, m_flDamage * (gpGlobals->curtime - m_flFireTime), nDamageType );
		CalculateMeleeDamageForce( &info, dir, ptr->endpos );
		pHit->DispatchTraceAttack( info, dir, ptr );
		ApplyMultiDamage();
		if ( HasSpawnFlags( SF_BEAM_DECALS ) )
		{
			if ( pHit->IsBSPModel() )
			{
				UTIL_DecalTrace( ptr, GetDecalName() );
			}
		}
	}
#endif
	m_flFireTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBeam::TurnOn( void )
{
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBeam::TurnOff( void )
{
	RemoveEffects( EF_NODRAW );
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: Input handler for the beam width. Sets the end width based on the
//			beam width.
// Input  : Beam width in tenths of world units.
//-----------------------------------------------------------------------------
void CSharedBeam::InputWidth( inputdata_t &inputdata )
{
	SetWidth( inputdata.value.Float() );
	SetEndWidth( inputdata.value.Float() );
}

void CSharedBeam::InputColorRedValue( inputdata_t &inputdata )
{
	int nNewColor = clamp( FastFloatToSmallInt(inputdata.value.Float()), 0, 255 );
	SetColor( nNewColor, GetRenderColorG(), GetRenderColorB() );
}

void CSharedBeam::InputColorGreenValue( inputdata_t &inputdata )
{
	int nNewColor =clamp( FastFloatToSmallInt(inputdata.value.Float()), 0, 255 );
	SetColor( GetRenderColorR(), nNewColor, GetRenderColorB() );
}

void CSharedBeam::InputColorBlueValue( inputdata_t &inputdata )
{
	int nNewColor = clamp( FastFloatToSmallInt(inputdata.value.Float()), 0, 255 );
	SetColor( GetRenderColorR(), GetRenderColorG(), nNewColor );
}

void CSharedBeam::InputNoise( inputdata_t &inputdata )
{
	SetNoise( inputdata.value.Float() );
}

int CSharedBeam::UpdateTransmitState( void )
{
	// we must call ShouldTransmit() if we have a move parent
	if ( GetMoveParent() )
		return SetTransmitState( FL_EDICT_FULLCHECK );

	return BaseClass::UpdateTransmitState( );
}

void CSharedBeam::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our attached entities to go too...
	for ( int i=0; i < MAX_BEAM_ENTS; ++i )
	{
		if ( m_hAttachEntity[i].Get() )
		{
			m_hAttachEntity[i]->SetTransmit( pInfo, bAlways );
		}
	}
}

int CSharedBeam::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( IsEffectActive( EF_NODRAW ) )
		return FL_EDICT_DONTSEND;

	// Transmit us with the same rules as our move parent
	if ( GetMoveParent() )
	{
		return GetMoveParent()->ShouldTransmit( pInfo );
	}

	return BaseClass::ShouldTransmit( pInfo );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays.
// Output : Returns the new text offset from the top.
//-----------------------------------------------------------------------------
int CSharedBeam::DrawDebugTextOverlays(void)
{
#if !defined( CLIENT_DLL )
	int text_offset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// Print state
		char tempstr[512];
		Q_snprintf(tempstr, sizeof(tempstr), "start: (%.2f,%.2f,%.2f)", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr, sizeof(tempstr), "end  : (%.2f,%.2f,%.2f)", m_vecEndPos.GetX(), m_vecEndPos.GetY(), m_vecEndPos.GetZ());
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	
	return text_offset;
#else
	return 0;
#endif
}

#if defined( CLIENT_DLL )

// Purpose: 
// Input  : isbeingremoved - 
//			*predicted - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSharedBeam::OnPredictedEntityRemove( bool isbeingremoved, C_BaseEntity *predicted )
{
	BaseClass::OnPredictedEntityRemove( isbeingremoved, predicted );

	CSharedBeam *beam = dynamic_cast< CSharedBeam * >( predicted );
	if ( !beam )
	{
		// Hrm, we didn't link up to correct type!!!
		Assert( 0 );
		// Delete right away since it's fucked up
		return true;
	}

	if ( beam->IsEFlagSet( EFL_KILLME ) )
	{
		// Don't delete right away
		AddEFlags( EFL_KILLME );
		return false;
	}

	// Go ahead and delete if it's not short-lived
	return true;
}

extern bool g_bRenderingScreenshot;
extern ConVar r_drawviewmodel;

int CSharedBeam::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if ( !ReadyToDraw() )
		return 0;

	if ( IsMarkedForDeletion() )
		return 0;

	if ( CurrentViewID() == VIEW_SHADOW_DEPTH_TEXTURE )
		return 0;

#ifdef PORTAL
	if ( ( !g_pPortalRender->IsRenderingPortal() && !m_bDrawInMainRender ) || 
		( g_pPortalRender->IsRenderingPortal() && !m_bDrawInPortalRender ) )
	{
		return 0;
	}
#endif //#ifdef PORTAL

	// Tracker 16432:  If rendering a savegame screenshot don't draw beams 
	//   who have viewmodels as their attached entity
	if ( g_bRenderingScreenshot || !r_drawviewmodel.GetBool() )
	{
		// If the beam is attached
		for (int i=0;i<MAX_BEAM_ENTS;i++)
		{
			C_BaseViewModel *vm = ToBaseViewModel(m_hAttachEntity[i].Get());
			if ( vm )
			{
				return 0;
			}
		}
	}

	beams->DrawBeam( this, instance );
	return 0;
}

void CSharedBeam::OnDataChanged( DataUpdateType_t updateType )
{
	MarkMessageReceived();

	// Make sure that the correct model is referenced for this entity
	SetModelPointer( modelinfo->GetModel( GetModelIndex() ) );

	// Convert weapon world models to viewmodels if they're weapons being carried by the local player
	for (int i=0;i<MAX_BEAM_ENTS;i++)
	{
		C_BaseEntity *pEnt = m_hAttachEntity[i].Get();
		if ( pEnt )
		{
			C_BaseCombatWeapon *pWpn = pEnt->MyCombatWeaponPointer();
			if ( pWpn && pWpn->ShouldDrawUsingViewModel() )
			{
				C_BasePlayer *player = ToBasePlayer( pWpn->GetOwner() );

				// Use GetRenderedWeaponModel() instead?
				C_BaseViewModel *pViewModel = player ? player->GetViewModel( 0 ) : NULL;
				if ( pViewModel )
				{
					// Get the viewmodel and use it instead
					m_hAttachEntity.Set( i, pViewModel );
				}
			}
		}
	}

	// Compute the bounds here...
	Vector mins, maxs;
	ComputeBounds( mins, maxs );
	SetCollisionBounds( mins, maxs );
	AddToEntityList( ENTITY_LIST_SIMULATE );
}

RenderableTranslucencyType_t CSharedBeam::ComputeTranslucencyType() 
{ 
	return RENDERABLE_IS_TRANSLUCENT; 
}

bool CSharedBeam::ShouldDraw()
{
	if ( m_nMinDXLevel != 0 )
	{
		if ( m_nMinDXLevel > g_pMaterialSystemHardwareConfig->GetDXSupportLevel() )
			return false;
	}
	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Adds to beam entity list
//-----------------------------------------------------------------------------
bool CSharedBeam::Simulate( void )
{
	bool bRet = false;
	// If set to invisible, skip. Do this before resetting the entity pointer so it has 
	// valid data to decide whether it's visible.
	if ( ShouldDraw() )
	{
		//FIXME: If we're hooked up to an attachment point, then recompute our bounds every frame
		if ( m_hAttachEntity[0].Get() || m_hAttachEntity[1].Get() )
		{
			// Compute the bounds here...
			Vector mins, maxs;
			ComputeBounds( mins, maxs );
			SetCollisionBounds( mins, maxs );
			bRet = true;
		}

		MoveToLastReceivedPosition();
	}

	BaseClass::Simulate();
	return bRet;
}

//-----------------------------------------------------------------------------
// Computes the bounding box of a beam local to the origin of the beam
//-----------------------------------------------------------------------------
void CSharedBeam::ComputeBounds( Vector& mins, Vector& maxs )
{
	Vector vecAbsStart = GetAbsStartPos();
	Vector vecAbsEnd = GetAbsEndPos();

	// May need extra points for creating the min/max bounds
	bool bUseExtraPoints = false;
	Vector vecAbsExtra1, vecAbsExtra2;

#ifdef PORTAL
	CBaseEntity *pStartEntity = GetStartEntityPtr();

	CTraceFilterSkipClassname traceFilter( pStartEntity, "prop_energy_ball", COLLISION_GROUP_NONE );

	ITraceFilter *pEntityBeamTraceFilter = NULL;
	if ( pStartEntity )
		pEntityBeamTraceFilter = pStartEntity->GetBeamTraceFilter();

	CTraceFilterChain traceFilterChain( &traceFilter, pEntityBeamTraceFilter );

	bUseExtraPoints = UTIL_Portal_Trace_Beam( this, vecAbsStart, vecAbsEnd, vecAbsExtra1, vecAbsExtra2, &traceFilterChain );
#endif

	switch( GetType() )
	{
	case BEAM_LASER:
	case BEAM_ENTS:
	case BEAM_SPLINE:
	case BEAM_ENTPOINT:
		{
			// Compute the bounds here...
			Vector attachmentPoint( 0, 0, 0 );
			mins.Init( 99999, 99999, 99999 );
			maxs.Init( -99999, -99999, -99999 );
			for (int i = 0; i < m_nNumBeamEnts; ++i )
			{
				C_BaseEntity *pTestEnt = m_hAttachEntity[i].Get();
				if ( pTestEnt )
				{
					if ( pTestEnt == this )
					{
						mins = maxs = GetAbsOrigin();
					}
					else
					{
						// We do this so we don't have to calculate attachments (and do expensive bone-setup calculations) on our attachments.
						Vector attMins, attMaxs;
						m_hAttachEntity[i]->GetRenderBoundsWorldspace( attMins, attMaxs );

						mins = mins.Min( attMins );
						mins = mins.Min( attMaxs );
						
						maxs = maxs.Max( attMins );
						maxs = maxs.Max( attMaxs );
					}
					
					//ASSERT_COORD( mins );
					//ASSERT_COORD( maxs );
				}
				else
				{
					if (i == 0)
					{
						VectorCopy( vecAbsStart, attachmentPoint );
					}
					else if (i == 1)
					{
						VectorCopy( vecAbsEnd, attachmentPoint );
					}
					else
					{
						Assert(0);
					}

					mins = mins.Min( attachmentPoint );
					maxs = maxs.Max( attachmentPoint );
				}
			}
		}
		break;

	case BEAM_POINTS:
	default:
		{
			for (int i = 0; i < 3; ++i)
			{
				if (vecAbsStart[i] < vecAbsEnd[i])
				{
					mins[i] = vecAbsStart[i];
					maxs[i] = vecAbsEnd[i];
				}
				else
				{
					mins[i] = vecAbsEnd[i];
					maxs[i] = vecAbsStart[i];
				}
			}
		}
		break;
	}

	if ( bUseExtraPoints )
	{
		mins = mins.Min( vecAbsExtra1 );
		mins = mins.Min( vecAbsExtra2 );
		maxs = maxs.Max( vecAbsExtra1 );
		maxs = maxs.Max( vecAbsExtra2 );
	}

	// bloat the bounding box by the width of the beam
	float rad = 0.5f * MAX( m_fWidth.Get(), m_fEndWidth.Get() );
	Vector vecRad( rad, rad, rad );
	mins -= vecRad;
	maxs += vecRad;

	// Make sure the bounds are measured in *relative coords*
	Vector vecAbsOrigin = GetAbsOrigin();
	mins -= vecAbsOrigin;
	maxs -= vecAbsOrigin;
}
#endif
