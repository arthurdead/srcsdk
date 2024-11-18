//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_baseentity.h"
#include "prediction.h"
#include "model_types.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "iviewrender.h"
#include "view.h"
#include "iefx.h"
#include "c_team.h"
#include "clientmode.h"
#include "usercmd.h"
#include "engine/IEngineSound.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"
#include "fx_line.h"
#include "interface.h"
#include "materialsystem/imaterialsystem.h"
#include "soundinfo.h"
#include "mathlib/vmatrix.h"
#include "interval.h"
#include "engine/ivdebugoverlay.h"
#include "c_ai_basenpc.h"
#include "apparent_velocity_helper.h"
#include "c_baseanimatingoverlay.h"
#include "tier1/KeyValues.h"
#include "hltvcamera.h"
#include "datacache/imdlcache.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"
#include "decals.h"
#include "cdll_bounded_cvars.h"
#include "inetchannelinfo.h"
#include "proto_version.h"
#include "clientalphaproperty.h"
#include "cellcoord.h"
#include "gamestringpool.h"
#include "recast/recast_mgr.h"
#include "collisionproperty.h"
#include "tempent.h"
#include "game_loopback/igameserverloopback.h"

#if defined __GNUC__ && defined __linux__
#include <cxxabi.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_BASEENTITY, "BaseEntity Client" );

#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	int g_nInterpolatedVarsChanged = 0;
	bool g_bRestoreInterpolatedVarValues = false;
#endif

extern ConVar pwatchent;

static bool g_bWasSkipping = (bool)-1;
static bool g_bWasThreaded =(bool)-1;
static int  g_nThreadModeTicks = 0;
static ConVar cl_interp_threadmodeticks( "cl_interp_threadmodeticks", "0", 0, "Additional interpolation ticks to use when interpolating with threaded engine mode set." );


void cc_cl_interp_all_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	if ( ((ConVar *)pConVar)->GetInt() )
	{
		C_BaseEntityIterator iterator;
		C_BaseEntity *pEnt;
		while ( (pEnt = iterator.Next()) != NULL )	
		{
			if ( pEnt->ShouldInterpolate() )
			{
				pEnt->AddToInterpolationList();
			}
		}
	}
}


static ConVar  report_cliententitysim( "report_cliententitysim", "0", FCVAR_CHEAT, "List all clientside simulations and time - will report and turn itself off." );
static ConVar  cl_extrapolate( "cl_extrapolate", "1", FCVAR_CHEAT, "Enable/disable extrapolation if interpolation history runs out." );

static ConVar  cl_interp_npcs( "cl_interp_npcs", "0.3", FCVAR_USERINFO, "Interpolate NPC positions starting this many seconds in past (or cl_interp, if greater)" );
static ConVar  cl_interp_all( "cl_interp_all", "0", 0, "Disable interpolation list optimizations.", 0, 0, 0, 0, cc_cl_interp_all_changed );
extern ConVar  *r_drawmodeldecals;
extern ConVar	cl_showerror;
int C_BaseEntity::m_nPredictionRandomSeed = -1;
C_BasePlayer *C_BaseEntity::m_pPredictionPlayer = NULL;
bool C_BaseEntity::s_bAbsQueriesValid = true;
bool C_BaseEntity::s_bAbsRecomputationEnabled = true;
bool C_BaseEntity::s_bInterpolate = true;

bool C_BaseEntity::sm_bDisableTouchFuncs = false;	// Disables PhysicsTouch and PhysicsStartTouch function calls

static ConVar  r_drawrenderboxes( "r_drawrenderboxes", "0", FCVAR_CHEAT );  

static bool g_bAbsRecomputationStack[8];
static unsigned short g_iAbsRecomputationStackPos = 0;

static CUtlLinkedList<C_BaseEntity*, unsigned short> g_EntityLists[NUM_ENTITY_LISTS];
bool C_BaseEntity::s_bImmediateRemovesAllowed = true;

// Create singleton
static CPredictableList g_Predictables;
IPredictableList *predictables = &g_Predictables;

CPredictableList *GetPredictables( )
{
	return &g_Predictables;
}

//-----------------------------------------------------------------------------
// Purpose: Add entity to list
// Input  : add - 
// Output : int
//-----------------------------------------------------------------------------
void CPredictableList::AddToPredictableList( ClientEntityHandle_t add )
{
	// This is a hack to remap slot to index
	if ( m_Predictables.Find( add ) != m_Predictables.InvalidIndex() )
	{
		return;
	}

	// Add to general list
	m_Predictables.Insert( add );

	// Maintain sort order by entindex
	int count = m_Predictables.Size();
	if ( count < 2 )
		return;

	int i, j;
	for ( i = 0; i < count; i++ )
	{
		for ( j = i + 1; j < count; j++ )
		{
			ClientEntityHandle_t h1 = m_Predictables[ i ];
			ClientEntityHandle_t h2 = m_Predictables[ j ];

			C_BaseEntity *p1 = cl_entitylist->GetBaseEntityFromHandle( h1 );
			C_BaseEntity *p2 = cl_entitylist->GetBaseEntityFromHandle( h2 );

			if ( !p1 || !p2 )
			{
				Assert( 0 );
				continue;
			}

			if ( p1->entindex() != -1 && 
				 p2->entindex() != -1 )
			{
				if ( p1->entindex() < p2->entindex() )
					continue;
			}

			if ( p2->entindex() == -1 )
				continue;

			m_Predictables[ i ] = h2;
			m_Predictables[ j ] = h1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : remove - 
//-----------------------------------------------------------------------------
void CPredictableList::RemoveFromPredictablesList( ClientEntityHandle_t remove )
{
	m_Predictables.FindAndRemove( remove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *CPredictableList::GetPredictable( int slot )
{
	return cl_entitylist->GetBaseEntityFromHandle( m_Predictables[ slot ] );
}

//-----------------------------------------------------------------------------
// Purpose: Searc predictables for previously created entity (by testId)
// Input  : testId - 
// Output : static C_BaseEntity
//-----------------------------------------------------------------------------
static C_BaseEntity *FindPreviouslyCreatedEntity( CPredictableId& testId )
{
	int c = predictables->GetPredictableCount();

	int i;
	for ( i = 0; i < c; i++ )
	{
		C_BaseEntity *e = predictables->GetPredictable( i );
		if ( !e || !e->IsClientCreated() )
			continue;

		// Found it, note use of operator ==
		if ( testId == e->GetPredictableID() )
		{
			return e;
		}
	}

	return NULL;
}

abstract_class IRecordingList
{
public:
	virtual ~IRecordingList() {};
	virtual void	AddToList( ClientEntityHandle_t add ) = 0;
	virtual void	RemoveFromList( ClientEntityHandle_t remove ) = 0;

	virtual int		Count() = 0;
	virtual IClientRenderable *Get( int index ) = 0;
};

class CRecordingList : public IRecordingList
{
public:
	virtual void	AddToList( ClientEntityHandle_t add );
	virtual void	RemoveFromList( ClientEntityHandle_t remove );

	virtual int		Count();
	IClientRenderable *Get( int index );
private:
	CUtlVector< ClientEntityHandle_t > m_Recording;
};

static CRecordingList g_RecordingList;
IRecordingList *recordinglist = &g_RecordingList;

//-----------------------------------------------------------------------------
// Purpose: Add entity to list
// Input  : add - 
// Output : int
//-----------------------------------------------------------------------------
void CRecordingList::AddToList( ClientEntityHandle_t add )
{
	// This is a hack to remap slot to index
	if ( m_Recording.Find( add ) != m_Recording.InvalidIndex() )
	{
		return;
	}

	// Add to general list
	m_Recording.AddToTail( add );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : remove - 
//-----------------------------------------------------------------------------
void CRecordingList::RemoveFromList( ClientEntityHandle_t remove )
{
	m_Recording.FindAndRemove( remove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : IClientRenderable
//-----------------------------------------------------------------------------
IClientRenderable *CRecordingList::Get( int index )
{
	return cl_entitylist->GetClientRenderableFromHandle( m_Recording[ index ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CRecordingList::Count()
{
	return m_Recording.Count();
}

// Helper object implementation
CCurTimeScopeGuard::CCurTimeScopeGuard( float flNewCurTime, bool bOptionalCondition /*= true*/ )
{
	m_bActive = bOptionalCondition;
	if ( m_bActive )
	{
		m_flSavedTime = gpGlobals->curtime;
		gpGlobals->curtime = flNewCurTime;
	}
	else
	{
		m_flSavedTime = 0.0f;
	}
}

CCurTimeScopeGuard::~CCurTimeScopeGuard()
{
	if ( m_bActive )
	{
		gpGlobals->curtime = m_flSavedTime;
	}
}

// Should these be somewhere else?
#define PITCH 0

//-----------------------------------------------------------------------------
// Purpose: Decodes animtime and notes when it changes
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_AnimTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEntity = ( C_BaseEntity * )pStruct;

	int t;
	int tickbase;
	int addt;

	// Unpack the data.
	addt	= pData->m_Value.m_Int;

	// Note, this needs to be encoded relative to packet timestamp, not raw client clock
	tickbase = gpGlobals->GetNetworkBase( gpGlobals->tickcount, pEntity->entindex() );

	t = tickbase;
											//  and then go back to floating point time.
	t += addt;				// Add in an additional up to 256 100ths from the server

	// center m_flAnimTime around current time.
	while (t < gpGlobals->tickcount - 127)
		t += 256;
	while (t > gpGlobals->tickcount + 127)
		t -= 256;
	
	pEntity->SetAnimTime( t * TICK_INTERVAL );
}

void RecvProxy_SimulationTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEntity = ( C_BaseEntity * )pStruct;

	int t;
	int tickbase;
	int addt;

	// Unpack the data.
	addt	= pData->m_Value.m_Int;

	// Note, this needs to be encoded relative to packet timestamp, not raw client clock
	tickbase = gpGlobals->GetNetworkBase( gpGlobals->tickcount, pEntity->entindex() );

	t = tickbase;
											//  and then go back to floating point time.
	t += addt;				// Add in an additional up to 256 100ths from the server

	// center m_flSimulationTime around current time.
	while (t < gpGlobals->tickcount - 127)
		t += 256;
	while (t > gpGlobals->tickcount + 127)
		t -= 256;
	
	pEntity->SetSimulationTime( t * TICK_INTERVAL );
}

void C_BaseEntity::RecvProxy_CellBits( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	if ( pEnt->SetCellBits( pData->m_Value.m_Int ) )
	{
	#ifdef DT_CELL_COORD_SUPPORTED
		if ( pEnt->ShouldRegenerateOriginFromCellBits() )
		{
			pEnt->m_vecNetworkOrigin.x = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellX, pEnt->m_vecCellOrigin.x );
			pEnt->m_vecNetworkOrigin.y = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellY, pEnt->m_vecCellOrigin.y );
			pEnt->m_vecNetworkOrigin.z = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellZ, pEnt->m_vecCellOrigin.z );
		}
	#endif
	}
}

void C_BaseEntity::RecvProxy_CellX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	int *cellX = (int *)pOut;
	Assert( cellX == &pEnt->m_cellX );

	*cellX = pData->m_Value.m_Int;

#ifdef DT_CELL_COORD_SUPPORTED
	// Cell changed, update world position
	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		pEnt->m_vecNetworkOrigin.x = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellX, pEnt->m_vecCellOrigin.x );
	}
#endif
}

void C_BaseEntity::RecvProxy_CellY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	int *cellY = (int *)pOut;
	Assert( cellY == &pEnt->m_cellY );

	*cellY = pData->m_Value.m_Int;

#ifdef DT_CELL_COORD_SUPPORTED
	// Cell changed, update world position
	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		pEnt->m_vecNetworkOrigin.y = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellY, pEnt->m_vecCellOrigin.y );
	}
#endif
}

void C_BaseEntity::RecvProxy_CellZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	int *cellZ = (int *)pOut;
	Assert( cellZ == &pEnt->m_cellZ );

	*cellZ = pData->m_Value.m_Int;

#ifdef DT_CELL_COORD_SUPPORTED
	// Cell changed, update world position
	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		pEnt->m_vecNetworkOrigin.z = CoordFromCell( pEnt->m_cellwidth, pEnt->m_cellZ, pEnt->m_vecCellOrigin.z );
	}
#endif
}

#ifdef DT_CELL_COORD_SUPPORTED
void C_BaseEntity::RecvProxy_CellOrigin( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	Vector *vecNetworkOrigin = (Vector *)pOut;

	Assert( vecNetworkOrigin == &pEnt->m_vecNetworkOrigin );

	pEnt->m_vecCellOrigin.x = pData->m_Value.m_Vector[0];
	pEnt->m_vecCellOrigin.y = pData->m_Value.m_Vector[1];
	pEnt->m_vecCellOrigin.z = pData->m_Value.m_Vector[2];

	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		register int const cellwidth = pEnt->m_cellwidth; // Load it into a register
		vecNetworkOrigin->x = CoordFromCell( cellwidth, pEnt->m_cellX, pData->m_Value.m_Vector[0] );
		vecNetworkOrigin->y = CoordFromCell( cellwidth, pEnt->m_cellY, pData->m_Value.m_Vector[1] );
		vecNetworkOrigin->z = CoordFromCell( cellwidth, pEnt->m_cellZ, pData->m_Value.m_Vector[2] );
	}
}

void C_BaseEntity::RecvProxy_CellOriginXY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	Vector *vecNetworkOrigin = (Vector *)pOut;

	Assert( vecNetworkOrigin == &pEnt->m_vecNetworkOrigin );

	pEnt->m_vecCellOrigin.x = pData->m_Value.m_Vector[0];
	pEnt->m_vecCellOrigin.y = pData->m_Value.m_Vector[1];

	register int const cellwidth = pEnt->m_cellwidth; // Load it into a register

	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		vecNetworkOrigin->x = CoordFromCell( cellwidth, pEnt->m_cellX, pData->m_Value.m_Vector[0] );
		vecNetworkOrigin->y = CoordFromCell( cellwidth, pEnt->m_cellY, pData->m_Value.m_Vector[1] );
	}
}

void C_BaseEntity::RecvProxy_CellOriginZ( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	float *vecNetworkOriginZ = (float *)pOut;

	Assert( vecNetworkOriginZ == &pEnt->m_vecNetworkOrigin[2] );

	pEnt->m_vecCellOrigin.z = pData->m_Value.m_Float;

	register int const cellwidth = pEnt->m_cellwidth; // Load it into a register

	if ( pEnt->ShouldRegenerateOriginFromCellBits() )
	{
		*vecNetworkOriginZ = CoordFromCell( cellwidth, pEnt->m_cellZ, pData->m_Value.m_Float );
	}
}
#endif

void RecvProxy_LocalVelocity( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;

	Vector vecVelocity;
	
	vecVelocity.x = pData->m_Value.m_Vector[0];
	vecVelocity.y = pData->m_Value.m_Vector[1];
	vecVelocity.z = pData->m_Value.m_Vector[2];

	// SetLocalVelocity checks to see if the value has changed
	pEnt->SetLocalVelocity( vecVelocity );
}
void RecvProxy_ToolRecording( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	if ( !ToolsEnabled() )
		return;

	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;
	pEnt->SetToolRecording( pData->m_Value.m_Int != 0 );
}

// Expose it to the engine.
IMPLEMENT_CLIENTCLASS(C_BaseEntity, DT_BaseEntity, CBaseEntity);

static void RecvProxy_MoveType( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetMoveType( (MoveType_t)(pData->m_Value.m_Int) );
}

static void RecvProxy_MoveCollide( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetMoveCollide( (MoveCollide_t)(pData->m_Value.m_Int) );
}

static void RecvProxy_Solid( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetSolid( (SolidType_t)pData->m_Value.m_Int );
}

static void RecvProxy_SolidFlags( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetSolidFlags( pData->m_Value.m_Int );
}

void RecvProxy_EffectFlags( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_BaseEntity*)pStruct)->SetEffects( pData->m_Value.m_Int );
}

void RecvProxy_ClrRender( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	uint32 color = LittleDWord((uint32)pData->m_Value.m_Int);
	color32 c = *(color32*)( &color );
	pEnt->SetRenderColor( c.r, c.g, c.b );
	pEnt->SetRenderAlpha( c.a );
}

void RecvProxy_RenderMode( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	pEnt->SetRenderMode( (RenderMode_t)(pData->m_Value.m_Int) );
}

void RecvProxy_RenderFX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	pEnt->SetRenderFX( (RenderFx_t)(pData->m_Value.m_Int), FLT_MAX, 0.0f );
}

BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_AnimTimeMustBeFirst )
	RecvPropInt( RECVINFO(m_flAnimTime), 0, RecvProxy_AnimTime ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_PredictableId )
	RecvPropPredictableId( RECVINFO( m_PredictableID ) ),
	RecvPropInt( RECVINFO( m_bIsPlayerSimulated ) ),
END_RECV_TABLE()

void RecvProxyArrayLength_ModelIndexesOverrides( void *pStruct, int objectID, int currentArrayLength )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)pStruct;
	if(currentArrayLength == 0) {
		if(pEnt->m_nModelIndexOverrides) {
			free(pEnt->m_nModelIndexOverrides);
		}
		pEnt->m_nModelIndexOverrides = NULL;
		pEnt->m_nModelIndexOverridesLength = currentArrayLength;
	} else if(pEnt->m_nModelIndexOverridesLength != currentArrayLength) {
		if(!pEnt->m_nModelIndexOverrides) {
			pEnt->m_nModelIndexOverrides = (int *)malloc(sizeof(int) * currentArrayLength);
		} else {
			pEnt->m_nModelIndexOverrides = (int *)realloc(pEnt->m_nModelIndexOverrides, sizeof(int) * currentArrayLength);
		}
		pEnt->m_nModelIndexOverridesLength = currentArrayLength;
	}
}

void RecvProxy_FadeMinDist( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	pEnt->SetDistanceFade( pData->m_Value.m_Float, pEnt->GetMaxFadeDist() );
}

void RecvProxy_FadeMaxDist( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	pEnt->SetDistanceFade( pEnt->GetMinFadeDist(), pData->m_Value.m_Float );
}

void RecvProxy_FadeScale( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// This proxy will cause the alpha modulation to get updated correctly
	C_BaseEntity *pEnt = (C_BaseEntity*)pStruct;
	pEnt->SetGlobalFadeScale( pData->m_Value.m_Float );
}

BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_MinimalTable )
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_BaseEntity, DT_FullTable )
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_BaseEntity, DT_BaseEntity)
	RecvPropDataTable( "AnimTimeMustBeFirst", 0, 0, &REFERENCE_RECV_TABLE(DT_AnimTimeMustBeFirst) ),
	RecvPropInt( RECVINFO(m_flSimulationTime), 0, RecvProxy_SimulationTime ),
	RecvPropInt( RECVINFO( m_ubInterpolationFrame ) ),

	RecvPropFloat( RECVINFO( m_flCreateTime ) ),

	RecvPropInt( RECVINFO( m_cellbits ), 0, C_BaseEntity::RecvProxy_CellBits ),
//	RecvPropArray( RecvPropInt( RECVINFO(m_cellXY[0]) ), m_cellXY ),
	RecvPropInt( RECVINFO( m_cellX ), 0, C_BaseEntity::RecvProxy_CellX ),
	RecvPropInt( RECVINFO( m_cellY ), 0, C_BaseEntity::RecvProxy_CellY ),
	RecvPropInt( RECVINFO( m_cellZ ), 0, C_BaseEntity::RecvProxy_CellZ ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ), 0, RECVPROP_VECORIGIN_PROXY ),

#if PREDICTION_ERROR_CHECK_LEVEL > 1 
	RecvPropVector( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
#else
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
#endif

	RecvPropInt( RECVINFO(m_nModelIndex) ),

	RecvPropInt(RECVINFO(m_fEffects), 0, RecvProxy_EffectFlags ),
	RecvPropInt("m_nRenderMode", 0, sizeof(unsigned char), 0, RecvProxy_RenderMode),
	RecvPropInt("m_nRenderFX", 0, sizeof(unsigned char), 0, RecvProxy_RenderFX),

	RecvPropInt("m_clrRender", 0, sizeof(color32), 0, RecvProxy_ClrRender),

	RecvPropInt(RECVINFO(m_iViewHideFlags)),
	RecvPropBool(RECVINFO(m_bDisableFlashlight)),

	RecvPropInt(RECVINFO(m_iTeamNum)),
	RecvPropInt(RECVINFO(m_CollisionGroup)),
	RecvPropFloat(RECVINFO(m_flElasticity)),
	RecvPropFloat(RECVINFO(m_flShadowCastDistance)),
	RecvPropEHandle( RECVINFO(m_hOwnerEntity) ),
	RecvPropEHandle( RECVINFO(m_hEffectEntity) ),
	RecvPropInt( RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent ),
	RecvPropInt( RECVINFO( m_iParentAttachment ), 0 ),

	RecvPropStringT( RECVINFO( m_iName ) ),

	RecvPropInt( "movetype", 0, SIZEOF_IGNORE, 0, RecvProxy_MoveType ),
	RecvPropInt( "movecollide", 0, SIZEOF_IGNORE, 0, RecvProxy_MoveCollide ),

	RecvPropDataTable( RECVINFO_DT( m_pCollision ), 0, &REFERENCE_RECV_TABLE(DT_CollisionProperty), DataTableRecvProxy_PointerDataTable ),

	RecvPropInt( RECVINFO ( m_iTextureFrameIndex ) ),

	RecvPropEHandle (RECVINFO(m_hPlayerSimulationOwner)),
	RecvPropDataTable( "predictable_id", 0, 0, &REFERENCE_RECV_TABLE( DT_PredictableId ) ),

	RecvPropInt		( RECVINFO( m_bSimulatedEveryTick ), 0, RecvProxy_InterpolationAmountChanged ),
	RecvPropInt		( RECVINFO( m_bAnimatedEveryTick ), 0, RecvProxy_InterpolationAmountChanged ),
	RecvPropBool	( RECVINFO( m_bAlternateSorting ) ),

	RecvPropFloat( "m_fadeMinDist", 0, sizeof(float), 0, RecvProxy_FadeMinDist ), 
	RecvPropFloat( "m_fadeMaxDist", 0, sizeof(float), 0, RecvProxy_FadeMaxDist ), 
	RecvPropFloat( "m_flFadeScale", 0, sizeof(float), 0, RecvProxy_FadeScale ), 

	RecvPropInt( RECVINFO( m_nMinCPULevel ) ), 
	RecvPropInt( RECVINFO( m_nMaxCPULevel ) ), 
	RecvPropInt( RECVINFO( m_nMinGPULevel ) ), 
	RecvPropInt( RECVINFO( m_nMaxGPULevel ) ), 

	RecvPropFloat( RECVINFO( m_fViewDistance ) ),

	RecvPropArray2( RecvProxyArrayLength_ModelIndexesOverrides,	RecvPropInt( RECVINFO_ARRAYELEM(m_nModelIndexOverrides, 0) ), NUM_SHARED_VISION_FILTERS, sizeof(int), m_nModelIndexOverrides ),

	RecvPropDataTable( "minimaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_MinimalTable) ),
	RecvPropDataTable( "fulldata", 0, 0, &REFERENCE_RECV_TABLE(DT_FullTable) ),

END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_LogicalEntity, DT_LogicalEntity, CLogicalEntity)
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_PointEntity, DT_PointEntity, CPointEntity)
END_RECV_TABLE()

const float coordTolerance = 2.0f / (float)( 1 << COORD_FRACTIONAL_BITS );

BEGIN_PREDICTION_DATA_NO_BASE( C_BaseEntity )

	// These have a special proxy to handle send/receive
	DEFINE_PRED_EMBEDDED_PTR_FLAGS( m_pCollision, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD_FLAGS( m_MoveType, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_MoveCollide, FIELD_CHARACTER, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( m_vecAbsVelocity, FIELD_VECTOR ),
	DEFINE_FIELD_FLAGS_TOL( m_vecVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.5f ),
//	DEFINE_FIELD_FLAGS( m_fEffects, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_EMBEDDED_PTR( m_pClientAlphaProperty ),

//	DEFINE_FIELD_FLAGS( m_flAnimTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
//	DEFINE_FIELD_FLAGS( m_flSimulationTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_fFlags, FIELD_INTEGER64, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS_TOL( m_vecViewOffset, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.25f ),
	DEFINE_FIELD_FLAGS( m_nModelIndex, FIELD_SHORT, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_FIELD_FLAGS( m_flFriction, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD_FLAGS( m_iTeamNum, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_iHealth, FIELD_INTEGER ),
	DEFINE_FIELD_FLAGS( m_hOwnerEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

//	DEFINE_FIELD( m_nSimulationTick, FIELD_INTEGER ),

	DEFINE_FIELD_FLAGS( m_hNetworkMoveParent, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
//	DEFINE_FIELD_FLAGS( m_pMoveParent, FIELD_EHANDLE ),
//	DEFINE_FIELD_FLAGS( m_pMoveChild, FIELD_EHANDLE ),
//	DEFINE_FIELD_FLAGS( m_pMovePeer, FIELD_EHANDLE ),
//	DEFINE_FIELD_FLAGS( m_pMovePrevPeer, FIELD_EHANDLE ),

	DEFINE_FIELD_FLAGS_TOL( m_vecNetworkOrigin, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, coordTolerance ),
	DEFINE_FIELD_FLAGS( m_angNetworkAngles, FIELD_VECTOR, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_FIELD( m_vecAbsOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_angAbsRotation, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_angRotation, FIELD_VECTOR ),

//	DEFINE_FIELD( m_hGroundEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nWaterLevel, FIELD_CHARACTER ),
	DEFINE_FIELD( m_nWaterType, FIELD_CHARACTER ),
	DEFINE_FIELD( m_vecAngVelocity, FIELD_VECTOR ),
//	DEFINE_FIELD( m_vecAbsAngVelocity, FIELD_VECTOR ),

//	DEFINE_FIELD( m_nMinCPULevel, FIELD_CHARACTER ),
//	DEFINE_FIELD( m_nMaxCPULevel, FIELD_CHARACTER ),
//	DEFINE_FIELD( m_nMinGPULevel, FIELD_CHARACTER ),
//	DEFINE_FIELD( m_nMaxGPULevel, FIELD_CHARACTER ),

//	DEFINE_FIELD( model, FIELD_INTEGER ), // writing pointer literally
//	DEFINE_FIELD( index, FIELD_INTEGER ),
//	DEFINE_FIELD( m_ClientHandle, FIELD_SHORT ),
//	DEFINE_FIELD( m_Partition, FIELD_SHORT ),
//	DEFINE_FIELD( m_hRender, FIELD_SHORT ),
	DEFINE_FIELD( m_bDormant, FIELD_BOOLEAN ),
//	DEFINE_FIELD( current_position, FIELD_INTEGER ),
//	DEFINE_FIELD( m_flLastMessageTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecBaseVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_iEFlags, FIELD_INTEGER64 ),
	DEFINE_FIELD( m_flGravity, FIELD_FLOAT ),
//	DEFINE_FIELD( m_ModelInstance, FIELD_SHORT ),
	DEFINE_FIELD( m_flProxyRandomValue, FIELD_FLOAT ),

	DEFINE_FIELD_FLAGS( m_hPlayerSimulationOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),	
//	DEFINE_FIELD( m_PredictableID, FIELD_INTEGER ),
//	DEFINE_FIELD( m_pPredictionContext, FIELD_POINTER ),
	// Stuff specific to rendering and therefore not to be copied back and forth
	// DEFINE_FIELD_FLAGS( m_clrRender, color32, FTYPEDESC_INSENDTABLE  ),
	// DEFINE_FIELD( m_bReadyToDraw, FIELD_BOOLEAN ),
	// DEFINE_FIELD( anim, CLatchedAnim ),
	// DEFINE_FIELD( mouth, CMouthInfo ),
	// DEFINE_FIELD( GetAbsOrigin(), FIELD_VECTOR ),
	// DEFINE_FIELD( GetAbsAngles(), FIELD_VECTOR ),
	// DEFINE_FIELD( m_nNumAttachments, FIELD_SHORT ),
	// DEFINE_FIELD( m_pAttachmentAngles, FIELD_VECTOR ),
	// DEFINE_FIELD( m_pAttachmentOrigin, FIELD_VECTOR ),
	// DEFINE_FIELD( m_listentry, CSerialEntity ),
	// DEFINE_FIELD( m_ShadowHandle, ClientShadowHandle_t ),
	// DEFINE_FIELD( m_hThink, ClientThinkHandle_t ),
	// Definitely private and not copied around
	// DEFINE_FIELD( m_bPredictable, FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_CollisionGroup, FIELD_INTEGER ),
	// DEFINE_FIELD( m_DataChangeEventRef, FIELD_INTEGER ),
#if !defined( CLIENT_DLL )
	// DEFINE_FIELD( m_bPredictionEligible, FIELD_BOOLEAN ),
#endif
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Helper functions.
//-----------------------------------------------------------------------------

void SpewInterpolatedVar( CInterpolatedVar< Vector > *pVar )
{
	Msg( "--------------------------------------------------\n" );
	int i = pVar->GetHead();
	Vector v0(0, 0, 0);
	CApparentVelocity<Vector> apparent(v0);
	float prevtime = 0.0f;
	while ( 1 )
	{
		float changetime;
		Vector *pVal = pVar->GetHistoryValue( i, changetime );
		if ( !pVal )
			break;

		float vel = apparent.AddSample( changetime, *pVal );
		Msg( "%6.6f: (%.2f %.2f %.2f), vel: %.2f [dt %.1f]\n", changetime, VectorExpand( *pVal ), vel, prevtime == 0.0f ? 0.0f : 1000.0f * ( changetime - prevtime ) );
		i = pVar->GetNext( i );
		prevtime = changetime;
	}
	Msg( "--------------------------------------------------\n" );
}

void SpewInterpolatedVar( CInterpolatedVar< Vector > *pVar, float flNow, float flInterpAmount, bool bSpewAllEntries = true )
{
	float target = flNow - flInterpAmount;

	Msg( "--------------------------------------------------\n" );
	int i = pVar->GetHead();
	Vector v0(0, 0, 0);
	CApparentVelocity<Vector> apparent(v0);
	float newtime = 999999.0f;
	Vector newVec( 0, 0, 0 );
	bool bSpew = true;

	while ( 1 )
	{
		float changetime;
		Vector *pVal = pVar->GetHistoryValue( i, changetime );
		if ( !pVal )
			break;

		if ( bSpew && target >= changetime )
		{
			Vector o;
			pVar->DebugInterpolate( &o, flNow );
			bool bInterp = newtime != 999999.0f;
			float frac = 0.0f;
			char desc[ 32 ];

			if ( bInterp )
			{
				frac = ( target - changetime ) / ( newtime - changetime );
				Q_snprintf( desc, sizeof( desc ), "interpolated [%.2f]", frac );
			}
			else
			{
				bSpew = true;
				int savei = i;
				i = pVar->GetNext( i );
				float oldtertime = 0.0f;
				pVar->GetHistoryValue( i, oldtertime );

				if ( changetime != oldtertime )
				{
					frac = ( target - changetime ) / ( changetime - oldtertime );
				}

				Q_snprintf( desc, sizeof( desc ), "extrapolated [%.2f]", frac );
				i = savei;
			}

			if ( bSpew )
			{
				Msg( "  > %6.6f: (%.2f %.2f %.2f) %s for %.1f msec\n", 
					target, 
					VectorExpand( o ), 
					desc,
					1000.0f * ( target - changetime ) );
				bSpew = false;
			}
		}

		float vel = apparent.AddSample( changetime, *pVal );
		if ( bSpewAllEntries )
		{
			Msg( "    %6.6f: (%.2f %.2f %.2f), vel: %.2f [dt %.1f]\n", changetime, VectorExpand( *pVal ), vel, newtime == 999999.0f ? 0.0f : 1000.0f * ( newtime - changetime ) );
		}
		i = pVar->GetNext( i );
		newtime = changetime;
		newVec = *pVal;
	}
	Msg( "--------------------------------------------------\n" );
}
void SpewInterpolatedVar( CInterpolatedVar< float > *pVar )
{
	Msg( "--------------------------------------------------\n" );
	int i = pVar->GetHead();
	CApparentVelocity<float> apparent(0.0f);
	while ( 1 )
	{
		float changetime;
		float *pVal = pVar->GetHistoryValue( i, changetime );
		if ( !pVal )
			break;

		float vel = apparent.AddSample( changetime, *pVal );
		Msg( "%6.6f: (%.2f), vel: %.2f\n", changetime, *pVal, vel );
		i = pVar->GetNext( i );
	}
	Msg( "--------------------------------------------------\n" );
}

template<class T>
void GetInterpolatedVarTimeRange( CInterpolatedVar<T> *pVar, float &flMin, float &flMax )
{
	flMin = 1e23;
	flMax = -1e23;

	int i = pVar->GetHead();
	Vector v0(0, 0, 0);
	CApparentVelocity<Vector> apparent(v0);
	while ( 1 )
	{
		float changetime;
		if ( !pVar->GetHistoryValue( i, changetime ) )
			return;

		flMin = MIN( flMin, changetime );
		flMax = MAX( flMax, changetime );
		i = pVar->GetNext( i );
	}
}


//-----------------------------------------------------------------------------
// Global methods related to when abs data is correct
//-----------------------------------------------------------------------------
void C_BaseEntity::SetAbsQueriesValid( bool bValid )
{
	// @MULTICORE: Always allow in worker threads, assume higher level code is handling correctly
	if ( !ThreadInMainThread() )
		return;

	if ( !bValid )
	{
		s_bAbsQueriesValid = false;
	}
	else
	{
		s_bAbsQueriesValid = true;
	}
}

bool C_BaseEntity::IsAbsQueriesValid( void )
{
	if ( !ThreadInMainThread() )
		return true;
	return s_bAbsQueriesValid;
}

void C_BaseEntity::PushEnableAbsRecomputations( bool bEnable )
{
	if ( !ThreadInMainThread() )
		return;
	if ( g_iAbsRecomputationStackPos < ARRAYSIZE( g_bAbsRecomputationStack ) )
	{
		g_bAbsRecomputationStack[g_iAbsRecomputationStackPos] = s_bAbsRecomputationEnabled;
		++g_iAbsRecomputationStackPos;
		s_bAbsRecomputationEnabled = bEnable;
	}
	else
	{
		Assert( false );
	}
}

void C_BaseEntity::PopEnableAbsRecomputations()
{
	if ( !ThreadInMainThread() )
		return;
	if ( g_iAbsRecomputationStackPos > 0 )
	{
		--g_iAbsRecomputationStackPos;
		s_bAbsRecomputationEnabled = g_bAbsRecomputationStack[g_iAbsRecomputationStackPos];
	}
	else
	{
		Assert( false );
	}
}

void C_BaseEntity::EnableAbsRecomputations( bool bEnable )
{
	if ( !ThreadInMainThread() )
		return;
	// This should only be called at the frame level. Use PushEnableAbsRecomputations
	// if you're blocking out a section of code.
	Assert( g_iAbsRecomputationStackPos == 0 );

	s_bAbsRecomputationEnabled = bEnable;
}

bool C_BaseEntity::IsAbsRecomputationsEnabled()
{
	if ( !ThreadInMainThread() )
		return true;
	return s_bAbsRecomputationEnabled;
}

int	C_BaseEntity::GetTextureFrameIndex( void )
{
	return m_iTextureFrameIndex;
}

void C_BaseEntity::SetTextureFrameIndex( int iIndex )
{
	m_iTextureFrameIndex = iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *map - 
//-----------------------------------------------------------------------------
void C_BaseEntity::Interp_SetupMappings( VarMapping_t *map )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;
		void *data = e->data;
		int type = e->type;

		watcher->Setup( data, type );
		watcher->SetInterpolationAmount( GetInterpolationAmount( watcher->GetType() ) ); 
	}
}

void C_BaseEntity::Interp_RestoreToLastNetworked( VarMapping_t *map )
{
	VPROF( "C_BaseEntity::Interp_RestoreToLastNetworked" );

	PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( this, "restoretolastnetworked" );

	Vector oldOrigin = GetLocalOrigin();
	QAngle oldAngles = GetLocalAngles();
	Vector oldVel = GetLocalVelocity();

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;
		watcher->RestoreToLastNetworked();
	}

	BaseInterpolatePart2( oldOrigin, oldAngles, oldVel, 0 );
}

void C_BaseEntity::Interp_UpdateInterpolationAmounts( VarMapping_t *map )
{
	if( !map )
		return;

	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;
		watcher->SetInterpolationAmount( GetInterpolationAmount( watcher->GetType() ) ); 
	}
}

void C_BaseEntity::Interp_HierarchyUpdateInterpolationAmounts()
{
	Interp_UpdateInterpolationAmounts( GetVarMapping() );

	for ( C_BaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		pChild->Interp_HierarchyUpdateInterpolationAmounts();
	}
}

inline int C_BaseEntity::Interp_Interpolate( VarMapping_t *map, float currentTime )
{
	int bNoMoreChanges = 1;
	if ( currentTime < map->m_lastInterpolationTime )
	{
		for ( int i = 0; i < map->m_nInterpolatedEntries; i++ )
		{
			VarMapEntry_t *e = &map->m_Entries[ i ];

			e->m_bNeedsToInterpolate = true;
		}
	}
	map->m_lastInterpolationTime = currentTime;

	for ( int i = 0; i < map->m_nInterpolatedEntries; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];

		if ( !e->m_bNeedsToInterpolate )
			continue;
			
		IInterpolatedVar *watcher = e->watcher;
		Assert( !( watcher->GetType() & EXCLUDE_AUTO_INTERPOLATE ) );


		if ( watcher->Interpolate( currentTime ) )
			e->m_bNeedsToInterpolate = false;
		else
			bNoMoreChanges = 0;
	}

	return bNoMoreChanges;
}

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------
C_BaseEntity::C_BaseEntity( uint64 iEFlags ) : 
	m_iv_vecOrigin( "C_BaseEntity::m_iv_vecOrigin" ),
	m_iv_angRotation( "C_BaseEntity::m_iv_angRotation" ),
	m_iv_vecVelocity( "C_BaseEntity::m_iv_vecVelocity" )
{
	m_iEFlags = 0;

	if( iEFlags != 0 )
		AddEFlags( iEFlags );

	m_pPrevByClass = m_pNextByClass = NULL;
	m_ListByClass = (UtlHashHandle_t)~0;

	m_pCollision = NULL;

	if( !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
	{
		m_pCollision = new C_CollisionProperty;

		IMPLEMENT_NETWORKVAR_CHAIN( m_pCollision )

		CollisionProp()->Init( this );
	}

	m_pClientAlphaProperty = NULL;

	if( !IsEFlagSet( EFL_NOT_RENDERABLE ) )
	{
		m_pClientAlphaProperty = static_cast< CClientAlphaProperty * >( g_pClientAlphaPropertyMgr->CreateClientAlphaProperty( this ) );
	}

	m_nMinCPULevel = m_nMaxCPULevel = 0;
	m_nMinGPULevel = m_nMaxGPULevel = 0;

	AddVar( &m_vecOrigin, &m_iv_vecOrigin, LATCH_SIMULATION_VAR );
	AddVar( &m_angRotation, &m_iv_angRotation, LATCH_SIMULATION_VAR );
	// Removing this until we figure out why velocity introduces view hitching.
	// One possible fix is removing the player->ResetLatched() call in CGameMovement::FinishDuck(), 
	// but that re-introduces a third-person hitching bug.  One possible cause is the abrupt change
	// in player size/position that occurs when ducking, and how prediction tries to work through that.
	//
	// AddVar( &m_vecVelocity, &m_iv_vecVelocity, LATCH_SIMULATION_VAR );

	m_DataChangeEventRef = -1;
	m_EntClientFlags = 0;

	m_iParentAttachment = 0;

	m_bRenderWithViewModels = false;
	m_bDisableCachedRenderBounds = false;
	m_bIsValidIKAttachment = false;

	SetPredictionEligible( false );
	m_bPredictable = false;

	m_bSimulatedEveryTick = false;
	m_bAnimatedEveryTick = false;
	m_pPhysicsObject = NULL;

#ifdef _DEBUG
	m_vecAbsOrigin = vec3_origin;
	m_angAbsRotation = vec3_angle;
	m_vecNetworkOrigin.Init();
	m_angNetworkAngles.Init();
	m_vecAbsOrigin.Init();
//	m_vecAbsAngVelocity.Init();
	m_vecVelocity.Init();
	m_vecAbsVelocity.Init();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.Init();

	m_iCurrentThinkContext = NO_THINK_CONTEXT;

#endif

	SetIdentityMatrix( m_rgflCoordinateFrame );
	m_nSimulationTick = -1;

	// Assume drawing everything
	m_bReadyToDraw = true;
	m_flProxyRandomValue = 0.0f;

	m_fBBoxVisFlags = 0;
	m_pPredictionContext = NULL;
	
	for ( int i = 0; i < NUM_ENTITY_LISTS; i++ )
	{
		m_ListEntry[i] = 0xFFFF;
	}

	if( !IsEFlagSet( EFL_NOT_RENDERABLE ) )
	{
		AddToEntityList( ENTITY_LIST_PRERENDER );
	}

	m_bDormant = true;

	m_nCreationTick = -1;
	m_RefEHandle.Term();
	m_ModelInstance = MODEL_INSTANCE_INVALID;
	m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_hRender = INVALID_CLIENT_RENDER_HANDLE;
	m_hThink = INVALID_THINK_HANDLE;
	m_AimEntsListHandle = INVALID_AIMENTS_LIST_HANDLE;

	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
	m_pModel = NULL;
	m_vecAbsOrigin.Init();
	m_angAbsRotation.Init();
	m_vecVelocity.Init();
	ClearFlags();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.Init();
	m_nModelIndex = 0;
	m_flAnimTime = 0;
	m_flSimulationTime = 0;

	if( !IsEFlagSet( EFL_NOT_COLLIDEABLE ) )
	{
		SetSolid( SOLID_NONE );
		SetSolidFlags( 0 );
	}

	SetMoveCollide( MOVECOLLIDE_DEFAULT );
	SetMoveType( MOVETYPE_NONE );

	if( !IsEFlagSet( EFL_NOT_RENDERABLE ) )
	{
		SetDistanceFade( 0.0f, 0.0f );
		SetGlobalFadeScale( 0.0f );
	}

	ClearEffects();
	SetRenderColor( 255, 255, 255 );

	if( !IsEFlagSet( EFL_NOT_RENDERABLE ) )
	{
		SetRenderFX( kRenderFxNone );
		SetRenderAlpha( 255 );
	}
	else
	{
		AddEffects( EF_NODRAW|EF_NOSHADOW|EF_NORECEIVESHADOW|EF_NOFLASHLIGHT|EF_NOSHADOWDEPTH );
		m_bDisableFlashlight = true;
	}

	m_flFriction = 0.0f;       
	m_flGravity = 0.0f;
	SetCheckUntouch( false );
	m_ShadowDirUseOtherEntity = NULL;

	m_nLastThinkTick = gpGlobals->tickcount;

	// Do not enable this on all entities. It forces bone setup for entities that
	// don't need it.
	//AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	UpdateVisibility();

	m_bEnabledInToolView = true;
	m_bToolRecording = false;
	m_ToolHandle = HTOOLHANDLE_INVALID;
	m_nLastRecordedFrame = -1;
	m_bRecordInTools = true;

#ifdef TF_CLIENT_DLL
	m_bValidatedOwner = false;
	m_bDeemedInvalid = false;
	m_bWasDeemedInvalid = false;
#endif

	ParticleProp()->Init( this );

	m_spawnflags = 0;

	m_flCreateTime = 0.0f;

	m_nModelIndexOverridesLength = 0;
	m_nModelIndexOverrides = NULL;

	m_bIsClientCreated = true;

	m_NavObstacleRef = NAV_OBSTACLE_INVALID_INDEX;

	m_LastShouldTransmitState = SHOULDTRANSMIT_END;
}

IClientNetworkable*C_BaseEntity::GetClientNetworkable()
{
	return this;
}

bool C_BaseEntity::IsNetworked( void ) const
{
	if ( m_pPredictionContext != NULL )
	{
		// For now can't be both
		Assert( !GetPredictable() );
		return true;
	}

	if(IsEFlagSet(EFL_NOT_NETWORKED))
		return false;

	if(!m_RefEHandle.IsValid())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this entity was created on the client.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsServerEntity( void )
{
	if ( m_pPredictionContext != NULL )
	{
		// For now can't be both
		Assert( !GetPredictable() );
		return true;
	}

	if(!m_RefEHandle.IsValid())
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsClientCreated( void ) const
{
	if ( m_pPredictionContext != NULL )
	{
		// For now can't be both
		Assert( !GetPredictable() );
		return true;
	}

	if(m_bIsClientCreated)
		return true;

	if(!m_RefEHandle.IsValid())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Deallocates the alpha property
//-----------------------------------------------------------------------------
void C_BaseEntity::CleanUpAlphaProperty()
{
	if ( m_pClientAlphaProperty )
	{
		g_pClientAlphaPropertyMgr->DestroyClientAlphaProperty( m_pClientAlphaProperty );
		m_pClientAlphaProperty = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
C_BaseEntity::~C_BaseEntity()
{
	C_BaseEntity::PhysicsRemoveTouchedList( this );
	C_BaseEntity::PhysicsRemoveGroundList( this );
	DestroyAllDataObjects();

	// Remove from the predictables list
	if ( GetPredictable() || IsClientCreated() )
	{
		g_Predictables.RemoveFromPredictablesList( GetClientHandle() );
	}

	// If it's play simulated, remove from simulation list if the player still exists...
	if ( IsPlayerSimulated()  )
	{
		UnsetPlayerSimulated();
	}

	if ( GetClientHandle() != INVALID_CLIENTENTITY_HANDLE )
	{
		if ( GetThinkHandle() != INVALID_THINK_HANDLE )
		{
			ClientThinkList()->RemoveThinkable( GetClientHandle() );
		}

		// Remove from the client entity list.
		ClientEntityList().RemoveEntity( GetClientHandle() );

		m_RefEHandle = INVALID_CLIENTENTITY_HANDLE;
	}

	if( CollisionProp() ) {
		// Are we in the partition?
		CollisionProp()->DestroyPartitionHandle();

		delete m_pCollision;
		m_pCollision = NULL;
	}

	beams->KillDeadBeams( this );

	// Clean up the model instance
	DestroyModelInstance();

	// Clean up drawing
	RemoveFromLeafSystem();

	RemoveFromAimEntsList();

	CleanUpAlphaProperty();
	ClearDataChangedEvent( m_DataChangeEventRef );
	delete m_pPredictionContext;
	for ( int i = 0; i < NUM_ENTITY_LISTS; i++ )
	{
		RemoveFromEntityList(entity_list_ids_t(i));
	}

	if(m_nModelIndexOverrides) {
		free(m_nModelIndexOverrides);
	}
}

//-----------------------------------------------------------------------------
// IClientUnknown 
//-----------------------------------------------------------------------------
IClientAlphaProperty* C_BaseEntity::GetClientAlphaProperty()
{
	return AlphaProp();
}
IClientAlphaPropertyEx* C_BaseEntity::GetClientAlphaPropertyEx()
{
	return AlphaProp();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::Spawn( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::Activate()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Attach to entity
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::InitializeAsServerEntity( int entnum, int iSerialNum )
{
	Assert( entnum >= 0 && entnum < GAME_NUM_ENT_ENTRIES );

	m_bIsClientCreated = false;

	cl_entitylist->AddNetworkableEntity( this, entnum, iSerialNum );

	CheckHasThinkFunction( TICK_NEVER_THINK );

	if(!IsEFlagSet( EFL_NOT_RENDERABLE ))
	{
		AlphaProp()->SetDesyncOffset( entnum );
	}

#ifdef _DEBUG
	if( g_pGameServerLoopback )
	{
		const char *sv_classname = g_pGameServerLoopback->GetEntityClassname( entnum, iSerialNum );

		if(sv_classname)
			AssertMsg( m_iClassname != NULL_STRING, "missing classname for %s (%s)", STRING(m_iRTTIClassname), sv_classname );
		else
			AssertMsg( m_iClassname != NULL_STRING, "missing classname for %s", STRING(m_iRTTIClassname) );
	}
#endif

	return true;
}

bool C_BaseEntity::InitializeAsPredictedEntity( const char *className, const char *module, int line )
{
	// Add the client entity to the master entity list.
	cl_entitylist->AddNonNetworkableEntity( this );
	Assert( GetClientHandle() != ClientEntityList().InvalidHandle() );

	CheckHasThinkFunction( TICK_NEVER_THINK );

	if(!IsEFlagSet( EFL_NOT_RENDERABLE ))
	{
		AlphaProp()->SetDesyncOffset( m_RefEHandle.GetEntryIndex() );
	}

	C_BasePlayer *player = C_BaseEntity::GetPredictionPlayer();

	Assert( player );
	Assert( player->m_pCurrentCommand );
	Assert( prediction->InPrediction() );

	// What's my birthday (should match server)
	int command_number	= player->m_pCurrentCommand->command_number;
	// Who's my daddy?
	int player_index	= player->entindex() - 1;

	// Create id/context
	CPredictableId testId;
	testId.Init( player_index, command_number, className, module, line );

	// It's predictable
	SetPredictionEligible( true );

	// Set up "shared" id number
	m_PredictableID.SetRaw( testId.GetRaw() );

	// Get a context (mostly for debugging purposes)
	PredictionContext *context			= new PredictionContext;
	context->m_bActive					= true;
	context->m_nCreationCommandNumber	= command_number;
	context->m_nCreationLineNumber		= line;
	context->m_pszCreationModule		= module;

	// Attach to entity
	m_pPredictionContext = context;

	//  and predictables
	g_Predictables.AddToPredictableList( GetClientHandle() );

	// Duhhhh..., but might as well be safe
	Assert( !GetPredictable() );
	Assert( IsClientCreated() );

	return true;
}

bool C_BaseEntity::InitializeAsClientEntity()
{
	// Add the client entity to the master entity list.
	cl_entitylist->AddNonNetworkableEntity( this );
	Assert( GetClientHandle() != ClientEntityList().InvalidHandle() );

	CheckHasThinkFunction( TICK_NEVER_THINK );

	if(!IsEFlagSet( EFL_NOT_RENDERABLE ))
	{
		AlphaProp()->SetDesyncOffset( m_RefEHandle.GetEntryIndex() );
	}

	return true;
}

bool C_BaseEntity::PostConstructor( const char *szClassname )
{
	if ( szClassname )
	{
		SetClassname(szClassname);
	}

	char *pRTTIClassname=NULL;
#if defined __GNUC__ && defined __linux__
	int status=0;
	size_t len=64;
	pRTTIClassname = (char *)malloc(len);
	pRTTIClassname = abi::__cxa_demangle( typeid( *this ).name(), pRTTIClassname, &len, &status );
#else
	char szRTTIClassname[64];
	Q_strncpy( szRTTIClassname, typeid( *this ).name(), sizeof( szRTTIClassname ) );
	pRTTIClassname = szRTTIClassname;
#endif

	m_iRTTIClassname = AllocPooledString( pRTTIClassname );

#if defined __GNUC__ && defined __linux__
	free(pRTTIClassname);
#endif

#ifdef _DEBUG
	if( IsEFlagSet( EFL_NOT_NETWORKED ) || !g_pGameServerLoopback )
	{
		AssertMsg( szClassname, "missing classname for %s", STRING(m_iRTTIClassname) );
	}
#endif

	CheckHasGamePhysicsSimulation();

	if(!IsEFlagSet( EFL_NOT_COLLIDEABLE ))
	{
		CollisionProp()->CreatePartitionHandle();
	}

	Interp_SetupMappings( GetVarMapping() );

	m_nCreationTick = gpGlobals->tickcount;

	if(IsEFlagSet( EFL_NOT_NETWORKED )) {
		if(!InitializeAsClientEntity()) {
			return false;
		}
	}

	return true;
}

C_PointEntity::C_PointEntity( uint64 iEFlags )
 : C_BaseEntity( EFL_NOT_COLLIDEABLE|EFL_NOT_RENDERABLE|iEFlags )
{
}

C_LogicalEntity::C_LogicalEntity( uint64 iEFlags )
 : C_PointEntity( iEFlags )
{
}

// Landmark class
void C_PointEntity::Spawn( void )
{
//	SetSolid( SOLID_NONE );
//	UTIL_SetSize(this, vec3_origin, vec3_origin);
}

void C_BaseEntity::TrackAngRotation( bool bTrack )
{
	if ( bTrack )
		AddVar( &m_angRotation, &m_iv_angRotation, LATCH_SIMULATION_VAR );
	else
		RemoveVar( &m_angRotation, false );
}

void C_BaseEntity::SetRefEHandle( const CBaseHandle &handle )
{
	m_RefEHandle = handle;
}


const EHANDLE& C_BaseEntity::GetRefEHandle() const
{
	return m_RefEHandle;
}

//-----------------------------------------------------------------------------
// Purpose: Free beams and destroy object
//-----------------------------------------------------------------------------
void C_BaseEntity::DO_NOT_USE_Release()
{
	UpdateOnRemove();

	if(dynamic_cast<C_LocalTempEntity *>(this) == NULL) {
		delete this;
	}
}


//-----------------------------------------------------------------------------
// Only meant to be called from subclasses.
// Returns true if instance valid, false otherwise
//-----------------------------------------------------------------------------
void C_BaseEntity::CreateModelInstance()
{
	if ( m_ModelInstance == MODEL_INSTANCE_INVALID )
	{
		m_ModelInstance = modelrender->CreateInstance( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DestroyModelInstance()
{
	if (m_ModelInstance != MODEL_INSTANCE_INVALID)
	{
		modelrender->DestroyInstance( m_ModelInstance );
		m_ModelInstance = MODEL_INSTANCE_INVALID;
	}
}

void C_BaseEntity::SetRemovalFlag( bool bRemove ) 
{ 
	if (bRemove) 
		m_iEFlags |= EFL_KILLME; 
	else 
		m_iEFlags &= ~EFL_KILLME; 
}

//-----------------------------------------------------------------------------
// Alpha
//-----------------------------------------------------------------------------
void C_BaseEntity::SetRenderAlpha( byte a )
{
	Assert( AlphaProp() );
	if ( GetRenderAlpha() != a )
	{
		AlphaProp()->SetAlphaModulation( a );
	}
}

byte C_BaseEntity::GetRenderAlpha() const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetAlphaModulation( );
}


//-----------------------------------------------------------------------------
// Methods related to fade scale
//-----------------------------------------------------------------------------
float C_BaseEntity::GetMinFadeDist( ) const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetMinFadeDist();
}

float C_BaseEntity::GetMaxFadeDist( ) const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetMaxFadeDist();
}

void C_BaseEntity::SetDistanceFade( float flMinDist, float flMaxDist )
{
	// NOTE: Setting the fade will not necessarily produce the same values
	// as what was passed in. I'm deliberately choosing not to grab them back out
	// because I'm not sure what client logic depends on them being negative, for example

	// Specifically, I'm certain the loading logic in C_PhysPropClientside,
	// as well as code inside of C_PhysPropClientside::Initialize
	// will definitely not work unless I'm doing it the way I'm currently doing it.
	Assert( AlphaProp() );
	AlphaProp()->SetFade( GetGlobalFadeScale(), flMinDist, flMaxDist );
}

void C_BaseEntity::SetGlobalFadeScale( float flFadeScale )
{
	Assert( AlphaProp() );
	int modelType = modelinfo->GetModelType( m_pModel );
	if ( modelType == mod_studio )
	{
		MDLCACHE_CRITICAL_SECTION();
		MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle( m_pModel );
		if ( hStudioHdr != MDLHANDLE_INVALID )
		{
			const studiohdr_t *pStudioHdr = g_pMDLCache->LockStudioHdr( hStudioHdr );
			if ( pStudioHdr->flags & STUDIOHDR_FLAGS_NO_FORCED_FADE )
			{
				flFadeScale = 0.0f;
			}
			g_pMDLCache->UnlockStudioHdr( hStudioHdr );
		}
	}
	AlphaProp()->SetFade( flFadeScale, GetMinFadeDist(), GetMaxFadeDist() );
}

float C_BaseEntity::GetGlobalFadeScale( ) const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetGlobalFadeScale();
}

//-----------------------------------------------------------------------------
// VPhysics objects..
//-----------------------------------------------------------------------------
int C_BaseEntity::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		// multi-object entities must implement this function
		Assert( !(pPhys->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY) );
		if ( listMax > 0 )
		{
			pList[0] = pPhys;
			return 1;
		}
	}
	return 0;
}

bool C_BaseEntity::VPhysicsIsFlesh( void )
{
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		int material = pList[i]->GetMaterialIndex();
		const surfacedata_t *pSurfaceData = physprops->GetSurfaceData( material );
		// Is flesh ?, don't allow pickup
		if ( IsTexFlesh(pSurfaceData->game.material) )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Retrieves the coordinate frame for this entity.
// Input  : forward - Receives the entity's forward vector.
//			right - Receives the entity's right vector.
//			up - Receives the entity's up vector.
//-----------------------------------------------------------------------------
void C_BaseEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pRight ); 
		*pRight *= -1.0f;
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}

//------------------------------------------------------------------------------
// Purpose : Returns velcocity of base entity.  If physically simulated gets
//			 velocity from physics object
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_BaseEntity::GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity)
{
	if (GetMoveType()==MOVETYPE_VPHYSICS && m_pPhysicsObject)
	{
		m_pPhysicsObject->GetVelocity(vVelocity,vAngVelocity);
	}
	else
	{
		if (vVelocity != NULL)
		{
			*vVelocity = GetAbsVelocity();
		}
		if (vAngVelocity != NULL)
		{
			QAngle tmp = GetLocalAngularVelocity();
			QAngleToAngularImpulse( tmp, *vAngVelocity );
		}
	}
}

bool C_BaseEntity::IsMoving()
{ 
	Vector velocity;
	GetVelocity( &velocity, NULL );
	return velocity != vec3_origin; 
}

void C_BaseEntity::UpdateVisibility()
{
#ifdef TF_CLIENT_DLL
	// TF prevents drawing of any entity attached to players that aren't items in the inventory of the player.
	// This is to prevent servers creating fake cosmetic items and attaching them to players.
	if ( !engine->IsPlayingDemo() )
	{
		static bool bIsStaging = ( engine->GetAppID() == 810 );
		if ( !m_bValidatedOwner )
		{
			bool bRetry = false;

			// Check it the first time we call update visibility (Source TV doesn't bother doing validation)
			m_bDeemedInvalid = engine->IsHLTV() ? false : !ValidateEntityAttachedToPlayer( bRetry );
			m_bValidatedOwner = !bRetry;
		}

		if ( m_bDeemedInvalid )
		{
			if ( bIsStaging )
			{
				if ( !m_bWasDeemedInvalid )
				{
					m_PreviousRenderMode = GetRenderMode();
					m_PreviousRenderColor = GetRenderColor();
					m_bWasDeemedInvalid = true;
				}

				SetRenderMode( kRenderTransColor );
				SetRenderColor( 255, 0, 0, 200 );

			}
			else
			{
				RemoveFromLeafSystem();
				return;
			}
		}
		else if ( m_bWasDeemedInvalid )
		{
			if ( bIsStaging )
			{
				// We need to fix up the rendering.
				SetRenderMode( m_PreviousRenderMode );
				SetRenderColor( m_PreviousRenderColor.r, m_PreviousRenderColor.g, m_PreviousRenderColor.b, m_PreviousRenderColor.a );
			}

			m_bWasDeemedInvalid = false;
		}
	}
#endif

	if ( ShouldDraw() && !IsDormant() && ( !ToolsEnabled() || IsEnabledInToolView() ) )
	{
		// add/update leafsystem
		AddToLeafSystem();
	}
	else
	{
		// remove from leaf system
		RemoveFromLeafSystem();
	}
}

void C_BaseEntity::UpdateVisibilityAllEntities()
{
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ( (pEnt = iterator.Next()) != NULL )	
	{
		pEnt->UpdateVisibility();	
	}
}

// (static function)
CON_COMMAND( cl_updatevisibility, "Updates visibility bits." )
{
	C_BaseEntity::UpdateVisibilityAllEntities();
}

void C_BaseEntity::RenderWithViewModels( bool bEnable )
{
	m_bRenderWithViewModels = bEnable;
	ClientLeafSystem()->RenderWithViewModels( m_hRender, bEnable );
}

bool C_BaseEntity::IsRenderingWithViewModels() const
{
	Assert( ( m_hRender == INVALID_CLIENT_RENDER_HANDLE ) || 
		( m_bRenderWithViewModels == ClientLeafSystem()->IsRenderingWithViewModels( m_hRender ) ) );
	return m_bRenderWithViewModels;
}

void C_BaseEntity::DisableCachedRenderBounds( bool bDisabled )
{
	m_bDisableCachedRenderBounds = bDisabled;
	ClientLeafSystem()->DisableCachedRenderBounds( m_hRender, bDisabled );
}

bool C_BaseEntity::IsCachedRenderBoundsDisabled() const
{
	return m_bDisableCachedRenderBounds;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether object should render.
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldDraw()
{
	// Let the client mode (like commander mode) reject drawing entities.
	if (GetClientMode() && !GetClientMode()->ShouldDrawEntity(this) )
		return false;

	if ( IsEFlagSet( EFL_NOT_RENDERABLE ) )
		return false;

	CPULevel_t nCPULevel = GetCPULevel();
	bool bNoDraw = ( m_nMinCPULevel && m_nMinCPULevel-1 > nCPULevel );
	bNoDraw = bNoDraw || ( m_nMaxCPULevel && m_nMaxCPULevel-1 < nCPULevel );
	if ( bNoDraw )
		return false;

	GPULevel_t nGPULevel = GetGPULevel();
	bNoDraw = ( m_nMinGPULevel && m_nMinGPULevel-1 > nGPULevel );
	bNoDraw = bNoDraw || ( m_nMaxGPULevel && m_nMaxGPULevel-1 < nGPULevel );
	if ( bNoDraw )
		return false;

	// Some rendermodes prevent rendering
	if ( GetRenderMode() == kRenderNone )
		return false;

	return (m_pModel != NULL) && !IsEffectActive(EF_NODRAW) && !IsWorld();
}

bool C_BaseEntity::TestCollision( const Ray_t& ray, unsigned int mask, trace_t& trace )
{
	return false;
}

bool C_BaseEntity::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return false;
}

//-----------------------------------------------------------------------------
// Used when the collision prop is told to ask game code for the world-space surrounding box
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// This should only be called if you're using USE_GAME_CODE on the server
	// and you forgot to implement the client-side version of this method.
	Assert(0);
}


//-----------------------------------------------------------------------------
// Purpose: Derived classes will have to write their own message cracking routines!!!
// Input  : length - 
//			*data - 
//-----------------------------------------------------------------------------
void C_BaseEntity::ReceiveMessage( int classID, bf_read &msg )
{
	// BaseEntity doesn't have a base class we could relay this message to
	Assert( classID == GetClientClass()->m_ClassID );
	
	int messageType = msg.ReadByte();
	switch( messageType )
	{
		case BASEENTITY_MSG_REMOVE_DECALS:
			RemoveAllDecals();
			break;
	}
}


void* C_BaseEntity::GetDataTableBasePtr()
{
	return this;
}


//-----------------------------------------------------------------------------
// Should this object cast shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_BaseEntity::ShadowCastType()
{
	if (IsEffectActive(EF_NODRAW | EF_NOSHADOW))
		return SHADOWS_NONE;

	int modelType = modelinfo->GetModelType( m_pModel );
	return (modelType == mod_studio) ? SHADOWS_RENDER_TO_TEXTURE : SHADOWS_NONE;
}


//-----------------------------------------------------------------------------
// Per-entity shadow cast distance + direction
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetShadowCastDistance( float *pDistance, ShadowType_t shadowType ) const			
{ 
	if ( m_flShadowCastDistance != 0.0f )
	{
		*pDistance = m_flShadowCastDistance; 
		return true;
	}
	return false;
}

void C_BaseEntity::OnDisableShadowDepthRenderingChanged()
{
	bool bIsShadowDepthRenderingDisabled = IsEffectActive(EF_NOSHADOWDEPTH);
	ClientLeafSystem()->DisableShadowDepthRendering(m_hRender, bIsShadowDepthRenderingDisabled);
}

void C_BaseEntity::OnShadowDepthRenderingCacheableStateChanged()
{
	bool bIsShadowDepthRenderingCacheDisabled = IsEffectActive(EF_SHADOWDEPTH_NOCACHE);
	ClientLeafSystem()->DisableShadowDepthCaching(m_hRender, bIsShadowDepthRenderingCacheDisabled);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseEntity *C_BaseEntity::GetShadowUseOtherEntity( void ) const
{
	return m_ShadowDirUseOtherEntity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetShadowUseOtherEntity( C_BaseEntity *pEntity )
{
	m_ShadowDirUseOtherEntity = pEntity;
}

CInterpolatedVar< QAngle >& C_BaseEntity::GetRotationInterpolator()
{
	return m_iv_angRotation;
}

CInterpolatedVar< Vector >& C_BaseEntity::GetOriginInterpolator()
{
	return m_iv_vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: Return a per-entity shadow cast direction
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const			
{ 
	if ( m_ShadowDirUseOtherEntity )
		return m_ShadowDirUseOtherEntity->GetShadowCastDirection( pDirection, shadowType );

	return false;
}


//-----------------------------------------------------------------------------
// Should this object receive shadows?
//-----------------------------------------------------------------------------
bool C_BaseEntity::ShouldReceiveProjectedTextures( int flags )
{
	Assert( flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );

	if ( IsEffectActive( EF_NODRAW ) || IsEffectActive( EF_NOFLASHLIGHT ) )
		 return false;

	if ( m_bDisableFlashlight )
		return false;

	if( ( flags & ( SHADOW_FLAGS_FLASHLIGHT | SHADOW_FLAGS_SIMPLE_PROJECTION ) ) != 0 )
	{
		if ( GetRenderMode() > kRenderNormal && GetRenderAlpha() == 0 )
			 return false;

		return true;
	}

	Assert( flags & SHADOW_FLAGS_SHADOW );

	if ( IsEffectActive( EF_NORECEIVESHADOW ) )
		 return false;

	if (modelinfo->GetModelType( m_pModel ) == mod_studio)
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Shadow-related methods
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsShadowDirty( )
{
	return IsEFlagSet( EFL_DIRTY_SHADOWUPDATE );
}

void C_BaseEntity::MarkShadowDirty( bool bDirty )
{
	if ( bDirty )
	{
		AddEFlags( EFL_DIRTY_SHADOWUPDATE );
	}
	else
	{
		RemoveEFlags( EFL_DIRTY_SHADOWUPDATE );
	}
}

IClientRenderable *C_BaseEntity::GetShadowParent()
{
	C_BaseEntity *pParent = GetMoveParent();
	return pParent ? pParent->GetClientRenderable() : NULL;
}

IClientRenderable *C_BaseEntity::FirstShadowChild()
{
	C_BaseEntity *pChild = FirstMoveChild();
	return pChild ? pChild->GetClientRenderable() : NULL;
}

IClientRenderable *C_BaseEntity::NextShadowPeer()
{
	C_BaseEntity *pPeer = NextMovePeer();
	return pPeer ? pPeer->GetClientRenderable() : NULL;
}

	
//-----------------------------------------------------------------------------
// Purpose: Returns index into entities list for this entity
// Output : Index
//-----------------------------------------------------------------------------
int	C_BaseEntity::entindex( void ) const
{
	return m_RefEHandle.GetEntryIndex();
}

int	C_BaseEntity::entserial( void ) const
{
	return m_RefEHandle.GetSerialNumber();
}

int C_BaseEntity::GetSoundSourceIndex() const
{
	return entindex();
}

//-----------------------------------------------------------------------------
// Get render origin and angles
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::GetRenderOrigin( void )
{
	return GetAbsOrigin();
}

const QAngle& C_BaseEntity::GetRenderAngles( void )
{
	return GetAbsAngles();
}

const matrix3x4_t &C_BaseEntity::RenderableToWorldTransform()
{
	return EntityToWorldTransform();
}

IPVSNotify* C_BaseEntity::GetPVSNotifyInterface()
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : theMins - 
//			theMaxs - 
//-----------------------------------------------------------------------------
void C_BaseEntity::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	int nModelType = modelinfo->GetModelType( m_pModel );
	if (nModelType == mod_studio || nModelType == mod_brush)
	{
		modelinfo->GetModelRenderBounds( GetModel(), theMins, theMaxs );
	}
	else
	{
		Assert( CollisionProp() );
		// By default, we'll just snack on the collision bounds, transform
		// them into entity-space, and call it a day.
		if ( GetRenderAngles() == CollisionProp()->GetCollisionAngles() )
		{
			theMins = CollisionProp()->OBBMins();
			theMaxs = CollisionProp()->OBBMaxs();
		}
		else
		{
			Assert( CollisionProp()->GetCollisionAngles() == vec3_angle );
			if ( IsPointSized() )
			{
				//theMins = CollisionProp()->GetCollisionOrigin();
				//theMaxs	= theMins;
				theMins = theMaxs = vec3_origin;
			}
			else
			{
				// NOTE: This shouldn't happen! Or at least, I haven't run
				// into a valid case where it should yet.
//				Assert(0);
				IRotateAABB( EntityToWorldTransform(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), theMins, theMaxs );
			}
		}
	}
}

void C_BaseEntity::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	DefaultRenderBoundsWorldspace( this, mins, maxs );
}


void C_BaseEntity::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	m_EntClientFlags |= ENTCLIENTFLAG_GETTINGSHADOWRENDERBOUNDS;
	GetRenderBounds( mins, maxs );
	m_EntClientFlags &= ~ENTCLIENTFLAG_GETTINGSHADOWRENDERBOUNDS;
}


//-----------------------------------------------------------------------------
// Purpose: Last received origin
// Output : const float
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::GetAbsOrigin( void ) const
{
	//Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsolutePosition();
	return m_vecAbsOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: Last received angles
// Output : const
//-----------------------------------------------------------------------------
const QAngle& C_BaseEntity::GetAbsAngles( void ) const
{
	//Assert( s_bAbsQueriesValid );
	const_cast<C_BaseEntity*>(this)->CalcAbsolutePosition();
	return m_angAbsRotation;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : org - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetNetworkOrigin( const Vector& org )
{
	m_vecNetworkOrigin = org;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ang - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetNetworkAngles( const QAngle& ang )
{
	m_angNetworkAngles = ang;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelIndex( int index )
{
	m_nModelIndex = index;
	const model_t *pModel = modelinfo->GetModel( m_nModelIndex );
	SetModelPointer( pModel );
}

void C_BaseEntity::SetModelPointer( const model_t *pModel )
{
	if ( pModel != m_pModel )
	{
		DestroyModelInstance();
		m_pModel = pModel;
		OnNewModel();

		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : val - 
//			moveCollide - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetMoveType( MoveType_t val, MoveCollide_t moveCollide /*= MOVECOLLIDE_DEFAULT*/ )
{
	// Make sure the move type + move collide are compatible...
#ifdef _DEBUG
	if ((val != MOVETYPE_FLY) && (val != MOVETYPE_FLYGRAVITY))
	{
		Assert( moveCollide == MOVECOLLIDE_DEFAULT );
	}
#endif

 	m_MoveType = val;
	SetMoveCollide( moveCollide );
}

void C_BaseEntity::SetMoveCollide( MoveCollide_t val )
{
	m_MoveCollide = val;
}


//-----------------------------------------------------------------------------
// Purpose: Get rendermode
// Output : int - the render mode
//-----------------------------------------------------------------------------
RenderableTranslucencyType_t C_BaseEntity::ComputeTranslucencyType()
{
	if ( m_bIsBlurred )
		return RENDERABLE_IS_TRANSLUCENT;

	// When an entity has a material proxy, we have to recompute
	// translucency here because the proxy may have changed it.
	if (modelinfo->ModelHasMaterialProxy( GetModel() ))
	{
		modelinfo->RecomputeTranslucency( const_cast<model_t*>(GetModel()), GetSkin(), GetBody(), GetClientRenderable() );
	}

	if( modelinfo->IsTranslucentTwoPass( m_pModel ) )
	{
		return RENDERABLE_IS_TWO_PASS;
	}

	if( modelinfo->IsTranslucent( m_pModel ) )
	{
		return RENDERABLE_IS_TRANSLUCENT;
	}

	return RENDERABLE_IS_OPAQUE;
}

void C_BaseEntity::OnTranslucencyTypeChanged()
{
	if ( m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->SetTranslucencyType( m_hRender, ComputeTranslucencyType() );
	}
}

int C_BaseEntity::GetRenderFlags( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to CMouthInfo data
// Output : CMouthInfo
//-----------------------------------------------------------------------------
CMouthInfo *C_BaseEntity::GetMouth( void )
{
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Retrieve sound spatialization info for the specified sound on this entity
// Input  : info - 
// Output : Return false to indicate sound is not audible
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetSoundSpatialization( SpatializationInfo_t& info )
{
	// World is always audible
	if ( IsWorld() )
	{
		return true;
	}

	// Out of PVS
	if ( IsDormant() )
	{
		return false;
	}

	// pModel might be NULL, but modelinfo can handle that
	const model_t *pModel = GetModel();
	
	if ( info.pflRadius )
	{
		*info.pflRadius = modelinfo->GetModelRadius( pModel );
	}
	
	if ( info.pOrigin )
	{
		*info.pOrigin = GetAbsOrigin();

		// move origin to middle of brush
		if ( modelinfo->GetModelType( pModel ) == mod_brush )
		{
			Vector mins, maxs, center;

			modelinfo->GetModelBounds( pModel, mins, maxs );
			VectorAdd( mins, maxs, center );
			VectorScale( center, 0.5f, center );

			(*info.pOrigin) += center;
		}
	}

	if ( info.pAngles )
	{
		VectorCopy( GetAbsAngles(), *info.pAngles );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	origin = GetAbsOrigin();
	angles = GetAbsAngles();
	return true;
}

bool C_BaseEntity::GetAttachment( int number, Vector &origin )
{
	origin = GetAbsOrigin();
	return true;
}

bool C_BaseEntity::GetAttachment( int number, matrix3x4_t &matrix )
{
	MatrixCopy( EntityToWorldTransform(), matrix );
	return true;
}

bool C_BaseEntity::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	originVel = GetAbsVelocity();
	angleVel.Init();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Get this entity's rendering clip plane if one is defined
// Output : float * - The clip plane to use, or NULL if no clip plane is defined
//-----------------------------------------------------------------------------
float *C_BaseEntity::GetRenderClipPlane( void )
{
	if( m_bEnableRenderingClipPlane )
		return m_fRenderingClipPlane;
	else
		return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseEntity::DrawBrushModel( bool bDrawingTranslucency, int nFlags, bool bTwoPass )
{
	VPROF_BUDGET( "C_BaseEntity::DrawBrushModel", VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING );
	// Identity brushes are drawn in view->DrawWorld as an optimization
	Assert ( modelinfo->GetModelType( m_pModel ) == mod_brush );

	ERenderDepthMode DepthMode = DEPTH_MODE_NORMAL;
	if ( ( nFlags & STUDIO_SSAODEPTHTEXTURE ) != 0 )
	{
		DepthMode = DEPTH_MODE_SSA0;
	}
	else if ( ( nFlags & STUDIO_SHADOWDEPTHTEXTURE ) != 0 )
	{
		DepthMode = DEPTH_MODE_SHADOW;
	}

	if ( DepthMode != DEPTH_MODE_NORMAL )
	{
		render->DrawBrushModelShadowDepth( this, (model_t *)m_pModel, GetAbsOrigin(), GetAbsAngles(), DepthMode );
	}
	else
	{
		DrawBrushModelMode_t mode = DBM_DRAW_ALL;
		if ( bTwoPass )
		{
			mode = bDrawingTranslucency ? DBM_DRAW_TRANSLUCENT_ONLY : DBM_DRAW_OPAQUE_ONLY;
		}
		render->DrawBrushModelEx( this, (model_t *)m_pModel, GetAbsOrigin(), GetAbsAngles(), mode );
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
int C_BaseEntity::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if ( !m_bReadyToDraw )
		return 0;

	int drawn = 0;
	if ( !m_pModel )
	{
		return drawn;
	}

	if (m_iViewHideFlags > 0)
	{
		// Hide this entity if it's not supposed to be drawn in this view.
		if (m_iViewHideFlags & (1 << CurrentViewID()))
		{
			return 0;
		}
	}

	int modelType = modelinfo->GetModelType( m_pModel );
	switch ( modelType )
	{
	case mod_brush:
		render->SetBlend( ( flags & STUDIO_SHADOWDEPTHTEXTURE ) ? 1.0f : (float)instance.m_nAlpha * ( 1.0f / 255.0f ) );
		drawn = DrawBrushModel( flags & STUDIO_TRANSPARENCY ? true : false, flags, ( flags & STUDIO_TWOPASS ) ? true : false );
		break;
	case mod_studio:
		// All studio models must be derived from C_BaseAnimating.  Issue warning.
		Warning( "ERROR:  Can't draw studio model %s because %s is not derived from C_BaseAnimating\n",
			modelinfo->GetModelName( m_pModel ), GetClientClass()->m_pNetworkName ? GetClientClass()->m_pNetworkName : "unknown" );
		break;
	case mod_sprite:
		//drawn = DrawSprite();
		Warning( "ERROR:  Sprite model's not supported any more except in legacy temp ents\n" );
		break;
	default:
		break;
	}

	// If we're visualizing our bboxes, draw them
	DrawBBoxVisualizations();

	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: Setup the bones for drawing
//-----------------------------------------------------------------------------
bool C_BaseEntity::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_BaseEntity::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
}


//-----------------------------------------------------------------------------
// Purpose: Process any local client-side animation events
//-----------------------------------------------------------------------------
void C_BaseEntity::DoAnimationEvents( )
{
}


void C_BaseEntity::UpdatePartitionListEntry()
{
	// Don't add the world entity
	CollideType_t shouldCollide = GetCollideType();

	// Choose the list based on what kind of collisions we want
	int list = PARTITION_CLIENT_NON_STATIC_EDICTS;
	if (shouldCollide == ENTITY_SHOULD_COLLIDE)
		list |= PARTITION_CLIENT_SOLID_EDICTS;
	else if (shouldCollide == ENTITY_SHOULD_RESPOND)
		list |= PARTITION_CLIENT_RESPONSIVE_EDICTS;

	if ( m_bIsValidIKAttachment )
	{
		list |= PARTITION_CLIENT_IK_ATTACHMENT;
	}

	// add the entity to the KD tree so we will collide against it
	partition->RemoveAndInsert( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, list, CollisionProp()->GetPartitionHandle() );
}


void C_BaseEntity::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	// Init should have been called before we get in here.
	Assert( IsEFlagSet( EFL_NOT_COLLIDEABLE ) || CollisionProp()->GetPartitionHandle() != PARTITION_INVALID_HANDLE );
	if ( !m_RefEHandle.IsValid() )
		return;
	
	m_LastShouldTransmitState = state;
	switch( state )
	{
	case SHOULDTRANSMIT_START:
		{
			// We've just been sent by the server. Become active.
			SetDormant( false );
			
			if(!IsEFlagSet( EFL_NOT_COLLIDEABLE ))
			{
				UpdatePartitionListEntry();
			}

			// Note that predictables get a chance to hook up to their server counterparts here
			if ( m_PredictableID.IsActive() )
			{
				// Find corresponding client side predicted entity and remove it from predictables
				m_PredictableID.SetAcknowledged( true );

				C_BaseEntity *otherEntity = FindPreviouslyCreatedEntity( m_PredictableID );
				if ( otherEntity )
				{
					Assert( otherEntity->IsClientCreated() );
					Assert( otherEntity->m_PredictableID.IsActive() );
					Assert( ClientEntityList().IsHandleValid( otherEntity->GetClientHandle() ) );

					otherEntity->m_PredictableID.SetAcknowledged( true );

					if ( OnPredictedEntityRemove( false, otherEntity ) )
					{
						// Mark it for delete after receive all network data
						UTIL_Remove( otherEntity );
					}
				}
			}
		}
		break;

	case SHOULDTRANSMIT_END:
		{
			// Clear out links if we're out of the picture...
			UnlinkFromHierarchy();

			// We're no longer being sent by the server. Become dormant.
			SetDormant( true );
			
			// remove the entity from the KD tree so we won't collide against it
			partition->Remove( PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle() );
		
		}
		break;

	default:
		Assert( 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Call this in PostDataUpdate if you don't chain it down!
//-----------------------------------------------------------------------------
void C_BaseEntity::MarkMessageReceived()
{
	m_flLastMessageTime = engine->GetLastTimeStamp();
}


//-----------------------------------------------------------------------------
// Purpose: Entity is about to be decoded from the network stream
// Input  : bnewentity - is this a new entity this update?
//-----------------------------------------------------------------------------
void C_BaseEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_BaseEntity::PreDataUpdate" );

	// Register for an OnDataChanged call and call OnPreDataChanged().
	if ( AddDataChangeEvent( this, updateType, &m_DataChangeEventRef ) )
	{
		OnPreDataChanged( updateType );
	}

	// Need to spawn on client before receiving original network data 
	// in case it overrides any values set up in spawn ( e.g., m_iState )
	bool bnewentity = (updateType == DATA_UPDATE_CREATED);

	if ( !bnewentity )
	{
		Interp_RestoreToLastNetworked( GetVarMapping() );
	}

	if ( bnewentity && !IsClientCreated() )
	{
		m_flSpawnTime = engine->GetLastTimeStamp();
		MDLCACHE_CRITICAL_SECTION();
		Spawn();
	}

#if 0 // Yahn suggesting commenting this out as a fix to demo recording not working
	// If the entity moves itself every FRAME on the server but doesn't update animtime,
	// then use the current server time as the time for interpolation.
	if ( IsSelfAnimating() )
	{
		m_flAnimTime = engine->GetLastTimeStamp();
	}
#endif

	m_vecOldOrigin = GetNetworkOrigin();
	m_vecOldAngRotation = GetNetworkAngles();

	m_flOldAnimTime = m_flAnimTime;
	m_flOldSimulationTime = m_flSimulationTime;

	if ( m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->EnableAlternateSorting( m_hRender, m_bAlternateSorting );
	}

	m_ubOldInterpolationFrame = m_ubInterpolationFrame;
}

const Vector& C_BaseEntity::GetOldOrigin()
{
	return m_vecOldOrigin;
}


void C_BaseEntity::UnlinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild )
{
	Assert( pChild );
	Assert( pParent != pChild );
	Assert( pChild->GetMoveParent() == pParent );

	// Unlink from parent
	// NOTE: pParent *may well be NULL*! This occurs
	// when a child has unlinked from a parent, and the child
	// remains in the PVS but the parent has not
	if (pParent && (pParent->m_pMoveChild == pChild))
	{
		Assert( !(pChild->m_pMovePrevPeer.IsValid()) );
		pParent->m_pMoveChild = pChild->m_pMovePeer;
	}

	// Unlink from siblings...
	if (pChild->m_pMovePrevPeer)
	{
		pChild->m_pMovePrevPeer->m_pMovePeer = pChild->m_pMovePeer;
	}
	if (pChild->m_pMovePeer)
	{
		pChild->m_pMovePeer->m_pMovePrevPeer = pChild->m_pMovePrevPeer;
	}

	pChild->m_pMovePeer = NULL;
	pChild->m_pMovePrevPeer = NULL;
	pChild->m_pMoveParent = NULL;
	pChild->RemoveFromAimEntsList();

	Interp_HierarchyUpdateInterpolationAmounts();
}

void C_BaseEntity::LinkChild( C_BaseEntity *pParent, C_BaseEntity *pChild )
{
	Assert( !pChild->m_pMovePeer.IsValid() );
	Assert( !pChild->m_pMovePrevPeer.IsValid() );
	Assert( !pChild->m_pMoveParent.IsValid() );
	Assert( pParent != pChild );

#ifdef _DEBUG
	// Make sure the child isn't already in this list
	C_BaseEntity *pExistingChild;
	for ( pExistingChild = pParent->FirstMoveChild(); pExistingChild; pExistingChild = pExistingChild->NextMovePeer() )
	{
		Assert( pChild != pExistingChild );
	}
#endif

	pChild->m_pMovePrevPeer = NULL;
	pChild->m_pMovePeer = pParent->m_pMoveChild;
	if (pChild->m_pMovePeer)
	{
		pChild->m_pMovePeer->m_pMovePrevPeer = pChild;
	}
	pParent->m_pMoveChild = pChild;
	pChild->m_pMoveParent = pParent;
	pChild->AddToAimEntsList();

	Interp_HierarchyUpdateInterpolationAmounts();
}

CUtlVector< C_BaseEntity * >	g_AimEntsList;


//-----------------------------------------------------------------------------
// Moves all aiments
//-----------------------------------------------------------------------------
void C_BaseEntity::MarkAimEntsDirty()
{
	// FIXME: With the dirty bits hooked into cycle + sequence, it's unclear
	// that this is even necessary any more (provided aiments are always accessing
	// joints or attachments of the move parent).
	//
	// NOTE: This is a tricky algorithm. This list does not actually contain
	// all aim-ents in its list. It actually contains all hierarchical children,
	// of which aim-ents are a part. We can tell if something is an aiment if it has
	// the EF_BONEMERGE effect flag set.
	// 
	// We will first iterate over all aiments and clear their DIRTY_ABSTRANSFORM flag, 
	// which is necessary to cause them to recompute their aim-ent origin 
	// the next time CalcAbsPosition is called. Because CalcAbsPosition calls MoveToAimEnt
	// and MoveToAimEnt calls SetAbsOrigin/SetAbsAngles, that is how CalcAbsPosition
	// will cause the aim-ent's (and all its children's) dirty state to be correctly updated.
	//
	// Then we will iterate over the loop a second time and call CalcAbsPosition on them,
	int i;
	int c = g_AimEntsList.Count();
	for ( i = 0; i < c; ++i )
	{
		C_BaseEntity *pEnt = g_AimEntsList[ i ];
		Assert( pEnt && pEnt->GetMoveParent() );
		if ( pEnt->IsEffectActive(EF_BONEMERGE | EF_PARENT_ANIMATES) )
		{
			pEnt->AddEFlags( EFL_DIRTY_ABSTRANSFORM );
		}
	}
}


void C_BaseEntity::CalcAimEntPositions()
{
	VPROF("CalcAimEntPositions");
	int i;
	int c = g_AimEntsList.Count();
	for ( i = 0; i < c; ++i )
	{
		C_BaseEntity *pEnt = g_AimEntsList[ i ];
		Assert( pEnt );
		Assert( pEnt->GetMoveParent() );
		if ( pEnt->IsEffectActive(EF_BONEMERGE) )
		{
			pEnt->CalcAbsolutePosition( );
		}
	}
}


void C_BaseEntity::AddToAimEntsList()
{
	// Already in list
	if ( m_AimEntsListHandle != INVALID_AIMENTS_LIST_HANDLE )
		return;

	m_AimEntsListHandle = g_AimEntsList.AddToTail( this );
}

void C_BaseEntity::RemoveFromAimEntsList()
{
	// Not in list yet
	if ( INVALID_AIMENTS_LIST_HANDLE == m_AimEntsListHandle )
	{
		return;
	}

	unsigned int c = g_AimEntsList.Count();

	Assert( m_AimEntsListHandle < c );

	unsigned int last = c - 1;

	if ( last == m_AimEntsListHandle )
	{
		// Just wipe the final entry
		g_AimEntsList.FastRemove( last );
	}
	else
	{
		C_BaseEntity *lastEntity = g_AimEntsList[ last ];
		// Remove the last entry
		g_AimEntsList.FastRemove( last );

		// And update it's handle to point to this slot.
		lastEntity->m_AimEntsListHandle = m_AimEntsListHandle;
		g_AimEntsList[ m_AimEntsListHandle ] = lastEntity;
	}

	// Invalidate our handle no matter what.
	m_AimEntsListHandle = INVALID_AIMENTS_LIST_HANDLE;
}

//-----------------------------------------------------------------------------
// Update move-parent if needed. For SourceTV.
//-----------------------------------------------------------------------------
void C_BaseEntity::HierarchyUpdateMoveParent()
{
	if ( m_hNetworkMoveParent.ToInt() == m_pMoveParent.ToInt() )
		return;

	HierarchySetParent( m_hNetworkMoveParent );
}


//-----------------------------------------------------------------------------
// Connects us up to hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::HierarchySetParent( C_BaseEntity *pNewParent )
{
	// NOTE: When this is called, we expect to have a valid
	// local origin, etc. that we received from network daa
	EHANDLE newParentHandle;
	newParentHandle.Set( pNewParent );
	if (newParentHandle.ToInt() == m_pMoveParent.ToInt())
		return;
	
	if (m_pMoveParent.IsValid())
	{
		UnlinkChild( m_pMoveParent, this );
	}
	if (pNewParent)
	{
		LinkChild( pNewParent, this );
	}

	InvalidatePhysicsRecursive( POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED );

#ifdef TF_CLIENT_DLL
	m_bValidatedOwner = false;
#endif

	// iterate the hierarchy using a ring buffer
	C_BaseEntity *list[1024];	// assume power of 2 elements
	int listReadIndex = 0;
	int listWriteIndex = 1;
	list[0] = this;

	while ( listReadIndex != listWriteIndex )
	{
		C_BaseEntity *pParent = list[listReadIndex];
		pParent->InvalidateAttachments();
		listReadIndex = (listReadIndex+1) & (ARRAYSIZE(list)-1);
		for (C_BaseEntity *pChild = pParent->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
		{
			list[listWriteIndex] = pChild;
			listWriteIndex = (listWriteIndex+1) & (ARRAYSIZE(list)-1);
		}
	}
}


//-----------------------------------------------------------------------------
// Unlinks from hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::SetParent( C_BaseEntity *pParentEntity, int iParentAttachment )
{
	// NOTE: This version is meant to be called *outside* of PostDataUpdate
	// as it assumes the moveparent has a valid handle
	EHANDLE newParentHandle;
	newParentHandle.Set( pParentEntity );
	if (newParentHandle.ToInt() == m_pMoveParent.ToInt())
		return;

	// NOTE: Have to do this before the unlink to ensure local coords are valid
	Vector vecAbsOrigin = GetAbsOrigin();
	QAngle angAbsRotation = GetAbsAngles();
	Vector vecAbsVelocity = GetAbsVelocity();

	// First deal with unlinking
	if (m_pMoveParent.IsValid())
	{
		UnlinkChild( m_pMoveParent, this );
	}

	if (pParentEntity)
	{
		LinkChild( pParentEntity, this );
	}

	if ( !IsServerEntity() )
	{
		m_hNetworkMoveParent = pParentEntity;
	}
	
	m_iParentAttachment = iParentAttachment;
	
	m_vecAbsOrigin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_angAbsRotation.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_vecAbsVelocity.Init( FLT_MAX, FLT_MAX, FLT_MAX );

	SetAbsOrigin(vecAbsOrigin);
	SetAbsAngles(angAbsRotation);
	SetAbsVelocity(vecAbsVelocity);

}


//-----------------------------------------------------------------------------
// Unlinks from hierarchy
//-----------------------------------------------------------------------------
void C_BaseEntity::UnlinkFromHierarchy()
{
	// Clear out links if we're out of the picture...
	if ( m_pMoveParent.IsValid() )
	{
		UnlinkChild( m_pMoveParent, this );
	}

	//Adrian: This was causing problems with the local network backdoor with entities coming in and out of the PVS at certain times.
	//This would work fine if a full entity update was coming (caused by certain factors like too many entities entering the pvs at once).
	//but otherwise it would not detect the change on the client (since the server and client shouldn't be out of sync) and the var would not be updated like it should.
	//m_iParentAttachment = 0;

	// unlink also all move children
	C_BaseEntity *pChild = FirstMoveChild();
	while( pChild )
	{
		if ( pChild->m_pMoveParent != this )
		{
			Warning( "C_BaseEntity::UnlinkFromHierarchy(): Entity has a child with the wrong parent!\n" );
			Assert( 0 );
			UnlinkChild( this, pChild );
			pChild->UnlinkFromHierarchy();
		}
		else
			pChild->UnlinkFromHierarchy();
		pChild = FirstMoveChild();
	}
}

bool C_BaseEntity::IsParentChanging()
{
	return ( m_hNetworkMoveParent.ToInt() != m_pMoveParent.ToInt() );
}

//-----------------------------------------------------------------------------
// Purpose: Make sure that the correct model is referenced for this entity
//-----------------------------------------------------------------------------
void C_BaseEntity::ValidateModelIndex( void )
{
	for(int i = 0; i < m_nModelIndexOverridesLength; ++i) {
		int flags = (m_nModelIndexOverrides[i] & 255);
		int modelindex = (m_nModelIndexOverrides[i] >> 8);

		if ( IsLocalPlayerUsingVisionFilterFlags( flags ) )
		{
			if ( modelindex > 0 )
			{
				SetModelByIndex( modelindex );
				return;
			}
		}
	}

	SetModelByIndex( m_nModelIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Entity data has been parsed and unpacked.  Now do any necessary decoding, munging
// Input  : bnewentity - was this entity new in this update packet?
//-----------------------------------------------------------------------------
void C_BaseEntity::PostDataUpdate( DataUpdateType_t updateType )
{
	MDLCACHE_CRITICAL_SECTION();

	PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( this, "postdataupdate" );

	// NOTE: This *has* to happen first. Otherwise, Origin + angles may be wrong 
	if ( m_bClientSideRagdoll && updateType == DATA_UPDATE_CREATED )
	{
		MoveToLastReceivedPosition( true );
	}
	else
	{
		MoveToLastReceivedPosition( false );
	}

	// If it's the world, force solid flags
	if ( IsWorld() )
	{
		m_nModelIndex = 1;
		SetSolid( SOLID_BSP );

		// FIXME: Should these be assertions?
		SetAbsOrigin( vec3_origin );
		SetAbsAngles( vec3_angle );
	}

	bool animTimeChanged = ( m_flAnimTime != m_flOldAnimTime ) ? true : false;
	bool originChanged = ( m_vecOldOrigin != GetLocalOrigin() ) ? true : false;
	bool anglesChanged = ( m_vecOldAngRotation != GetLocalAngles() ) ? true : false;
	bool simTimeChanged = ( m_flSimulationTime != m_flOldSimulationTime ) ? true : false;

	// Detect simulation changes 
	bool simulationChanged = originChanged || anglesChanged || simTimeChanged;

	bool bPredictable = GetPredictable();

	// For non-predicted and non-client only ents, we need to latch network values into the interpolation histories
	if ( !bPredictable && !IsClientCreated() )
	{
		if ( animTimeChanged )
		{
			OnLatchInterpolatedVariables( LATCH_ANIMATION_VAR );
		}

		if ( simulationChanged )
		{
			OnLatchInterpolatedVariables( LATCH_SIMULATION_VAR );
		}
	}
	// For predictables, we also need to store off the last networked value
	else if ( bPredictable )
	{
		// Just store off last networked value for use in prediction
		OnStoreLastNetworkedValue();
	}

	// Deal with hierarchy. Have to do it here (instead of in a proxy)
	// because this is the only point at which all entities are loaded
	// If this condition isn't met, then a child was sent without its parent
	Assert( m_hNetworkMoveParent.Get() || !m_hNetworkMoveParent.IsValid() );
	HierarchySetParent(m_hNetworkMoveParent);

	MarkMessageReceived();

	// Make sure that the correct model is referenced for this entity
	ValidateModelIndex();

	// If this entity was new, then latch in various values no matter what.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Construct a random value for this instance
		m_flProxyRandomValue = random_valve->RandomFloat( 0, 1 );

		ResetLatched();

		m_nCreationTick = gpGlobals->tickcount;
	}

	CheckInitPredictable( "PostDataUpdate" );

	// It's possible that a new entity will need to be forceably added to the 
	//   player simulation list.  If so, do this here
	if ( IsPlayerSimulated() && C_BasePlayer::IsLocalPlayer( m_hPlayerSimulationOwner ) )
	{
		// Make sure player is driving simulation (field is only ever sent to local player)
		SetPlayerSimulated( ToBasePlayer( m_hPlayerSimulationOwner ) );
	}

	if(!IsEFlagSet( EFL_NOT_COLLIDEABLE ))
	{
		UpdatePartitionListEntry();
	}
	
	// Add the entity to the nointerp list.
	if ( !IsClientCreated() )
	{
		if ( Teleported() || IsNoInterpolationFrame() )
			AddToTeleportList();
	}

	// if we changed parents, recalculate visibility
	if ( m_hOldMoveParent != m_hNetworkMoveParent )
	{
		UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Latch simulation values when the entity has not changed
//-----------------------------------------------------------------------------
void C_BaseEntity::OnDataUnchangedInPVS()
{
	Assert( m_hNetworkMoveParent.Get() || !m_hNetworkMoveParent.IsValid() );
	HierarchySetParent(m_hNetworkMoveParent);
	
	MarkMessageReceived();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *context - 
//-----------------------------------------------------------------------------
void C_BaseEntity::CheckInitPredictable( const char *context )
{
	// Prediction is disabled
	if ( !cl_predict->GetInt() )
		return;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if ( !player )
		return;

	if ( !GetPredictionEligible() )
	{
		if ( m_PredictableID.IsActive() &&
			( player->entindex() - 1 ) == m_PredictableID.GetPlayer() )
		{
			// If it comes through with an ID, it should be eligible
			SetPredictionEligible( true );
		}
		else
		{
			return;
		}
	}

	if ( IsClientCreated() )
		return;

	if ( !ShouldPredict() )
		return;

	if ( IsIntermediateDataAllocated() )
		return;

	// Msg( "Predicting init %s at %s\n", GetClassname(), context );

	// It's either a player, a weapon or a view model
	C_BasePlayer *pOwner = GetPredictionOwner();
	Assert( pOwner );
	if ( !pOwner )
		return;

	InitPredictable( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: Return the player who will predict this entity
//-----------------------------------------------------------------------------
C_BasePlayer* C_BaseEntity::GetPredictionOwner()
{
	C_BasePlayer *pOwner = ToBasePlayer( this );
	if ( !pOwner )
	{
		pOwner = ToBasePlayer( GetOwnerEntity() );
		if ( !pOwner )
		{
			C_BaseViewModel *vm = ToBaseViewModel(this);
			if ( vm )
			{
				pOwner = ToBasePlayer( vm->GetOwner() );
			}
		}
	}
	return pOwner;
}

bool C_BaseEntity::IsSelfAnimating()
{
	return true;
}


//-----------------------------------------------------------------------------
// EFlags.. 
//-----------------------------------------------------------------------------
uint64 C_BaseEntity::GetEFlags() const
{
	return m_iEFlags;
}

//-----------------------------------------------------------------------------
// Sets the model... 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelByIndex( int nModelIndex )
{
	SetModelIndex( nModelIndex );
}


//-----------------------------------------------------------------------------
// Set model... (NOTE: Should only be used by client-only entities
//-----------------------------------------------------------------------------
bool C_BaseEntity::SetModel( const char *pModelName )
{
	if ( pModelName )
	{
		int nModelIndex = modelinfo->GetModelIndex( pModelName );
		SetModelByIndex( nModelIndex );
		return ( nModelIndex != -1 );
	}
	else
	{
		SetModelByIndex( -1 );
		return false;
	}
}

void C_BaseEntity::OnStoreLastNetworkedValue()
{
	bool bRestore = false;
	Vector savePos;
	QAngle saveAng;

	// Kind of a hack, but we want to latch the actual networked value for origin/angles, not what's sitting in m_vecOrigin in the
	//  ragdoll case where we don't copy it over in MoveToLastNetworkOrigin
	if ( m_bClientSideRagdoll && GetPredictable() )
	{
		bRestore = true;
		savePos = GetLocalOrigin();
		saveAng = GetLocalAngles();

		MoveToLastReceivedPosition( true );
	}

	int c = m_VarMap.m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &m_VarMap.m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		int type = watcher->GetType();

		if ( type & EXCLUDE_AUTO_LATCH )
			continue;

		watcher->NoteLastNetworkedValue();
	}

	if ( bRestore )
	{
		SetLocalOrigin( savePos );
		SetLocalAngles( saveAng );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The animtime is about to be changed in a network update, store off various fields so that
//  we can use them to do blended sequence transitions, etc.
// Input  : *pState - the (mostly) previous state data
//-----------------------------------------------------------------------------

void C_BaseEntity::OnLatchInterpolatedVariables( int flags )
{
	float changetime = GetLastChangeTime( flags );

	bool bUpdateLastNetworkedValue = !(flags & INTERPOLATE_OMIT_UPDATE_LAST_NETWORKED) ? true : false;

	PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( this, bUpdateLastNetworkedValue ? "latch+net" : "latch" );

	int c = m_VarMap.m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &m_VarMap.m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		int type = watcher->GetType();

		if ( !(type & flags) )
			continue;

		if ( type & EXCLUDE_AUTO_LATCH )
			continue;

		if ( watcher->NoteChanged( gpGlobals->curtime, changetime, bUpdateLastNetworkedValue ) )
			e->m_bNeedsToInterpolate = true;
	}
	
	if ( ShouldInterpolate() )
	{
		AddToInterpolationList();
	}
}

int C_BaseEntity::BaseInterpolatePart1( float &currentTime, Vector &oldOrigin, QAngle &oldAngles, Vector &oldVel, int &bNoMoreChanges )
{
	// Don't mess with the world!!!
	bNoMoreChanges = 1;
	

	// These get moved to the parent position automatically
	if ( IsFollowingEntity() || !IsInterpolationEnabled() )
	{
		// Assume current origin ( no interpolation )
		MoveToLastReceivedPosition();
		return INTERPOLATE_STOP;
	}


	if ( GetPredictable() || IsClientCreated() )
	{
		C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();
		if ( localplayer && currentTime == gpGlobals->curtime )
		{
			currentTime = localplayer->GetFinalPredictedTime();
			currentTime -= TICK_INTERVAL;
			currentTime += ( gpGlobals->interpolation_amount * TICK_INTERVAL );
		}
	}

	oldOrigin = m_vecOrigin;
	oldAngles = m_angRotation;
	oldVel = m_vecVelocity;

	bNoMoreChanges = Interp_Interpolate( GetVarMapping(), currentTime );
	if ( cl_interp_all.GetInt() || (m_EntClientFlags & ENTCLIENTFLAG_ALWAYS_INTERPOLATE) )
		bNoMoreChanges = 0;

	return INTERPOLATE_CONTINUE;
}

#if 0
static ConVar cl_watchplayer( "cl_watchplayer", "-1", 0 );
#endif

void C_BaseEntity::BaseInterpolatePart2( Vector &oldOrigin, QAngle &oldAngles, Vector &oldVel, int nChangeFlags )
{
	if ( m_vecOrigin != oldOrigin )
	{
		nChangeFlags |= POSITION_CHANGED;
	}

	if( m_angRotation != oldAngles )
	{
		nChangeFlags |= ANGLES_CHANGED;
	}

	if ( m_vecVelocity != oldVel )
	{
		nChangeFlags |= VELOCITY_CHANGED;
	}

	if ( nChangeFlags != 0 )
	{
		InvalidatePhysicsRecursive( nChangeFlags );
	}

#if 0
	if ( index == 1 )
	{
		SpewInterpolatedVar( &m_iv_vecOrigin, gpGlobals->curtime, GetInterpolationAmount( LATCH_SIMULATION_VAR ), true );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Default interpolation for entities
// Output : true means entity should be drawn, false means probably not
//-----------------------------------------------------------------------------
bool C_BaseEntity::Interpolate( float currentTime )
{
	VPROF( "C_BaseEntity::Interpolate" );

	Vector oldOrigin;
	QAngle oldAngles;
	Vector oldVel;

	int bNoMoreChanges;
	int retVal = BaseInterpolatePart1( currentTime, oldOrigin, oldAngles, oldVel, bNoMoreChanges );

	// If all the Interpolate() calls returned that their values aren't going to
	// change anymore, then get us out of the interpolation list.
	if ( bNoMoreChanges )
		RemoveFromInterpolationList();

	if ( retVal == INTERPOLATE_STOP )
		return true;

	int nChangeFlags = 0;
	BaseInterpolatePart2( oldOrigin, oldAngles, oldVel, nChangeFlags );

	return true;
}

CStudioHdr *C_BaseEntity::OnNewModel()
{
#ifdef TF_CLIENT_DLL
	m_bValidatedOwner = false;
#endif

	OnTranslucencyTypeChanged();
	ClientLeafSystem()->SetModelType( m_hRender );
	InvalidatePhysicsRecursive( BOUNDS_CHANGED | SEQUENCE_CHANGED );
	SetGlobalFadeScale( GetGlobalFadeScale() );

	return NULL;
}

void C_BaseEntity::OnNewParticleEffect( const char *pszParticleName, CNewParticleEffect *pNewParticleEffect )
{
	return;
}

void C_BaseEntity::OnParticleEffectDeleted( CNewParticleEffect *pParticleEffect )
{
	return;
}

// Above this velocity and we'll assume a warp/teleport
#define MAX_INTERPOLATE_VELOCITY 4000.0f
#define MAX_INTERPOLATE_VELOCITY_PLAYER 1250.0f

//-----------------------------------------------------------------------------
// Purpose: Determine whether entity was teleported ( so we can disable interpolation )
// Input  : *ent - 
// Output : bool
//-----------------------------------------------------------------------------
bool C_BaseEntity::Teleported( void )
{
	// Disable interpolation when hierarchy changes
	if (m_hOldMoveParent != m_hNetworkMoveParent || m_iOldParentAttachment != m_iParentAttachment)
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Is this a submodel of the world ( model name starts with * )?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsSubModel( void )
{
	if ( m_pModel &&
		modelinfo->GetModelType( m_pModel ) == mod_brush &&
		modelinfo->GetModelName( m_pModel )[0] == '*' )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Create entity lighting effects
//-----------------------------------------------------------------------------
bool C_BaseEntity::CreateLightEffects( void )
{
	dlight_t *dl;

	bool bHasLightEffects = false;
	// Is this for player flashlights only, if so move to linkplayers?
	if ( !IsViewEntity() )
	{
		if (IsEffectActive(EF_BRIGHTLIGHT))
		{
			dl = effects->CL_AllocDlight ( entindex() );
			if(dl) {
				dl->origin = GetAbsOrigin();
				dl->origin[2] += 16;
				dl->color.r = dl->color.g = dl->color.b = 250;
				dl->radius = random_valve->RandomFloat(400,431);
				dl->die = gpGlobals->curtime + 0.001;
				bHasLightEffects = true;
			}
		}

		if (IsEffectActive(EF_DIMLIGHT))
		{			
			dl = effects->CL_AllocDlight ( entindex() );
			if(dl) {
				dl->origin = GetAbsOrigin();
				dl->color.r = dl->color.g = dl->color.b = 100;
				dl->radius = random_valve->RandomFloat(200,231);
				dl->die = gpGlobals->curtime + 0.001;
				bHasLightEffects = true;
			}
		}
	}

	return bHasLightEffects;
}

void C_BaseEntity::MoveToLastReceivedPosition( bool force )
{
	if ( force || ( !m_bClientSideRagdoll ) )
	{
		SetLocalOrigin( GetNetworkOrigin() );
		SetLocalAngles( GetNetworkAngles() );
	}
}

bool C_BaseEntity::ShouldInterpolate()
{
	// Never bother interpolating client side entities
	if( IsClientCreated() )
		return false;

	if ( IsViewEntity() )
		return true;

	if ( IsWorld() || !GetModel() )
		return false;

	// always interpolate if visible
	if ( IsVisible() )
		return true;

	// if any movement child needs interpolation, we have to interpolate too
	C_BaseEntity *pChild = FirstMoveChild();
	while( pChild )
	{
		if ( pChild->ShouldInterpolate() )	
			return true;

		pChild = pChild->NextMovePeer();
	}

	// don't interpolate
	return false;
}


void C_BaseEntity::ProcessTeleportList()
{
	int iNext;
	for ( int iCur=g_EntityLists[ENTITY_LIST_TELEPORT].Head(); iCur != g_EntityLists[ENTITY_LIST_TELEPORT].InvalidIndex(); iCur=iNext )
	{
		iNext = g_EntityLists[ENTITY_LIST_TELEPORT].Next( iCur );
		C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_TELEPORT].Element(iCur);

		bool teleport = pCur->Teleported();
		bool ef_nointerp = pCur->IsNoInterpolationFrame();
	
		if ( teleport || ef_nointerp )
		{
			// Undo the teleport flag..
			pCur->m_hOldMoveParent = pCur->m_hNetworkMoveParent;			
			pCur->m_iOldParentAttachment = pCur->m_iParentAttachment;
			// Zero out all but last update.
			pCur->MoveToLastReceivedPosition( true );
			pCur->ResetLatched();
		}
		else
		{
			// Get it out of the list as soon as we can.
			pCur->RemoveFromTeleportList();
		}
	}
}


void C_BaseEntity::CheckInterpolatedVarParanoidMeasurement()
{
	// What we're doing here is to check all the entities that were not in the interpolation
	// list and make sure that there's no entity that should be in the list that isn't.
	
#ifdef INTERPOLATEDVAR_PARANOID_MEASUREMENT
	int iHighest = ClientEntityList().GetHighestEntityIndex();
	for ( int i=0; i <= iHighest; i++ )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( i );
		if ( !pEnt || pEnt->m_InterpolationListEntry != 0xFFFF || !pEnt->ShouldInterpolate() )
			continue;
		
		// Player angles always generates this error when the console is up.
		if ( pEnt->entindex() == 1 && engine->Con_IsVisible() )
			continue;
			
		// View models tend to screw up this test unnecesarily because they modify origin,
		// angles, and 
		if ( dynamic_cast<C_BaseViewModel*>( pEnt ) )
			continue;

		g_bRestoreInterpolatedVarValues = true;
		g_nInterpolatedVarsChanged = 0;
		pEnt->Interpolate( gpGlobals->curtime );
		g_bRestoreInterpolatedVarValues = false;
		
		if ( g_nInterpolatedVarsChanged > 0 )
		{
			static int iWarningCount = 0;
			Warning( "(%d): An entity (%d) should have been in g_InterpolationList.\n", iWarningCount++, pEnt->entindex() );
			break;
		}
	}
#endif
}


void C_BaseEntity::ProcessInterpolatedList()
{
	CheckInterpolatedVarParanoidMeasurement();

	// Interpolate the minimal set of entities that need it.
	int iNext;
	for ( int iCur=g_EntityLists[ENTITY_LIST_INTERPOLATE].Head(); iCur != g_EntityLists[ENTITY_LIST_INTERPOLATE].InvalidIndex(); iCur=iNext )
	{
		iNext = g_EntityLists[ENTITY_LIST_INTERPOLATE].Next( iCur );
		C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_INTERPOLATE].Element(iCur);
		
		pCur->m_bReadyToDraw = pCur->Interpolate( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Returns the aiment render origin + angles
//-----------------------------------------------------------------------------
void C_BaseEntity::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles )
{
	// Should be overridden for things that attach to attchment points

	// Slam origin to the origin of the entity we are attached to...
	*pOrigin = pAttachedTo->GetAbsOrigin();
	*pAngles = pAttachedTo->GetAbsAngles();
}


void C_BaseEntity::StopFollowingEntity( )
{
	Assert( IsFollowingEntity() );

	SetParent( NULL );
	RemoveEffects( EF_BONEMERGE );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
}

bool C_BaseEntity::IsFollowingEntity()
{
	return IsEffectActive(EF_BONEMERGE) && (GetMoveType() == MOVETYPE_NONE) && GetMoveParent();
}

C_BaseEntity *C_BaseEntity::GetFollowedEntity()
{
	if (!IsFollowingEntity())
		return NULL;
	return GetMoveParent();
}


//-----------------------------------------------------------------------------
// Default implementation for GetTextureAnimationStartTime
//-----------------------------------------------------------------------------
float C_BaseEntity::GetTextureAnimationStartTime()
{
	return m_flSpawnTime;
}


//-----------------------------------------------------------------------------
// Default implementation, indicates that a texture animation has wrapped
//-----------------------------------------------------------------------------
void C_BaseEntity::TextureAnimationWrapped()
{
}

bool C_BaseEntity::Simulate()
{
	return false;
}

// Defined in engine
extern ConVar *cl_interpolate;

// (static function)
void C_BaseEntity::InterpolateServerEntities()
{
	VPROF_BUDGET( "C_BaseEntity::InterpolateServerEntities", VPROF_BUDGETGROUP_INTERPOLATION );

	bool bPrevInterpolate = s_bInterpolate;

	s_bInterpolate = cl_interpolate->GetBool();

	// Don't interpolate during timedemo playback
	if ( engine->IsPlayingTimeDemo() || engine->IsPaused() )
	{										 
		s_bInterpolate = false;
	}

	if ( !engine->IsPlayingDemo() )
	{
		// Don't interpolate, either, if we are timing out
		INetChannelInfo *nci = engine->GetNetChannelInfo();
		if ( nci && nci->GetTimeSinceLastReceived() > 0.5f )
		{
			s_bInterpolate = false;
		}
	}

	if ( IsSimulatingOnAlternateTicks() != g_bWasSkipping || IsEngineThreaded() != g_bWasThreaded ||
		 cl_interp_threadmodeticks.GetInt() != g_nThreadModeTicks )
	{
		g_bWasSkipping = IsSimulatingOnAlternateTicks();
		g_bWasThreaded = IsEngineThreaded();
		g_nThreadModeTicks = cl_interp_threadmodeticks.GetInt();

		int iNext;
		for ( int iCur=g_EntityLists[ENTITY_LIST_INTERPOLATE].Head(); iCur != g_EntityLists[ENTITY_LIST_INTERPOLATE].InvalidIndex(); iCur=iNext )
		//C_BaseEntityIterator iterator;
		//C_BaseEntity *pEnt;
		//while ( (pEnt = iterator.Next()) != NULL )
		{
			iNext = g_EntityLists[ENTITY_LIST_INTERPOLATE].Next( iCur );
			C_BaseEntity *pEnt = g_EntityLists[ENTITY_LIST_INTERPOLATE].Element(iCur);

			pEnt->Interp_UpdateInterpolationAmounts( pEnt->GetVarMapping() );
		}
	}

	// Enable extrapolation?
	CInterpolationContext context;
	context.SetLastTimeStamp( engine->GetLastTimeStamp() );
	if ( cl_extrapolate.GetBool() && !engine->IsPaused() )
	{
		context.EnableExtrapolation( true );
	}

	if ( bPrevInterpolate != s_bInterpolate && !s_bInterpolate )
	{
		// Clear interp history when we disable interpolation
		C_BaseEntityIterator iterator;
		C_BaseEntity *pEnt;
		while ( (pEnt = iterator.Next()) != NULL )
		{
			pEnt->ResetLatched();
		}
	}

	// Smoothly interpolate position for server entities.
	ProcessTeleportList();
	ProcessInterpolatedList();
}


// (static function)
void C_BaseEntity::AddVisibleEntities()
{
	VPROF_BUDGET( "C_BaseEntity::AddVisibleEntities", VPROF_BUDGETGROUP_WORLD_RENDERING );

	// Let non-dormant client created predictables get added, too
	int c = predictables->GetPredictableCount();
	for ( int i = 0 ; i < c ; i++ )
	{
		C_BaseEntity *pEnt = predictables->GetPredictable( i );
		if ( !pEnt )
			continue;

		if ( !pEnt->IsClientCreated() )
			continue;

		// Only draw until it's ack'd since that means a real entity has arrived
		if ( pEnt->m_PredictableID.GetAcknowledged() )
			continue;

		// Don't draw if dormant
		if ( pEnt->IsDormantPredictable() )
			continue;

		pEnt->UpdateVisibility();	
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_BaseEntity::OnPreDataChanged( DataUpdateType_t type )
{
	m_hOldMoveParent = m_hNetworkMoveParent;
	m_iOldParentAttachment = m_iParentAttachment;
}

void C_BaseEntity::OnDataChanged( DataUpdateType_t type )
{
	// See if it needs to allocate prediction stuff
	CheckInitPredictable( "OnDataChanged" );

	if(GetPredictable() && !ShouldPredict()) {
		ShutdownPredictable();
	}

	// Set up shadows; do it here so that objects can change shadowcasting state
	CreateShadow();

	if ( type == DATA_UPDATE_CREATED )
	{
		UpdateVisibility();
	}

	if( !IsEFlagSet( EFL_NOT_RENDERABLE ) )
	{
		// These may have changed in the network update
		AlphaProp()->SetRenderFX( GetRenderFX(), GetRenderMode(), FLT_MAX, 0.0f );
		AlphaProp()->SetDesyncOffset( entindex() );

		// Copy in fade parameters
		AlphaProp()->SetFade( GetGlobalFadeScale(), GetMinFadeDist(), GetMaxFadeDist() );
	}
}

ClientThinkHandle_t C_BaseEntity::GetThinkHandle()
{
	return m_hThink;
}


void C_BaseEntity::SetThinkHandle( ClientThinkHandle_t hThink )
{
	m_hThink = hThink;
}

//-----------------------------------------------------------------------------
// Determine the color modulation amount
//-----------------------------------------------------------------------------

void C_BaseEntity::GetColorModulation( float* color )
{
	color[0] = m_clrRenderRGB->r / 255.0f;
	color[1] = m_clrRenderRGB->g / 255.0f;
	color[2] = m_clrRenderRGB->b / 255.0f;
}


//-----------------------------------------------------------------------------
// Returns true if we should add this to the collision list
//-----------------------------------------------------------------------------
CollideType_t C_BaseEntity::GetCollideType( void )
{
	if ( !m_nModelIndex || !m_pModel )
		return ENTITY_SHOULD_NOT_COLLIDE;

	if ( !IsSolid( ) )
		return ENTITY_SHOULD_NOT_COLLIDE;

	// If the model is a bsp or studio (i.e. it can collide with the player
	if ( ( modelinfo->GetModelType( m_pModel ) != mod_brush ) && ( modelinfo->GetModelType( m_pModel ) != mod_studio ) )
		return ENTITY_SHOULD_NOT_COLLIDE;

	// Don't get stuck on point sized entities ( world doesn't count )
	if ( m_nModelIndex != 1 )
	{
		if ( IsPointSized() )
			return ENTITY_SHOULD_NOT_COLLIDE;
	}

	return ENTITY_SHOULD_COLLIDE;
}


//-----------------------------------------------------------------------------
// Is this a brush model?
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsBrushModel() const
{
	int modelType = modelinfo->GetModelType( m_pModel );
	return (modelType == mod_brush);
}


//-----------------------------------------------------------------------------
// This method works when we've got a studio model
//-----------------------------------------------------------------------------
void C_BaseEntity::AddStudioDecal( const Ray_t& ray, int hitbox, int decalIndex, 
								  bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if (doTrace)
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );

		// Trace the ray against the entity
		if (tr.fraction == 1.0f)
			return;

		// Set the trace index appropriately...
		tr.m_pEnt = this;
	}

	// Exit out after doing the trace so any other effects that want to happen can happen.
	if ( !r_drawmodeldecals->GetBool() )
		return;

	// Found the point, now lets apply the decals
	CreateModelInstance();

	// FIXME: Pass in decal up?
	Vector up(0, 0, 1);

	if (doTrace && (GetSolid() == SOLID_VPHYSICS) && !tr.startsolid && !tr.allsolid)
	{
		// Choose a more accurate normal direction
		// Also, since we have more accurate info, we can avoid pokethru
		Vector temp;
		VectorSubtract( tr.endpos, tr.plane.normal, temp );
		Ray_t betterRay;
		betterRay.Init( tr.endpos, temp );
		modelrender->AddDecal( m_ModelInstance, betterRay, up, decalIndex, GetStudioBody(), true, maxLODToDecal );
	}
	else
	{
		modelrender->AddDecal( m_ModelInstance, ray, up, decalIndex, GetStudioBody(), false, maxLODToDecal );
	}
}

//-----------------------------------------------------------------------------
void C_BaseEntity::AddColoredStudioDecal( const Ray_t& ray, int hitbox, int decalIndex, 
	bool doTrace, trace_t& tr, Color cColor, int maxLODToDecal )
{
	if (doTrace)
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );

		// Trace the ray against the entity
		if (tr.fraction == 1.0f)
			return;

		// Set the trace index appropriately...
		tr.m_pEnt = this;
	}

	// Exit out after doing the trace so any other effects that want to happen can happen.
	if ( !r_drawmodeldecals->GetBool() )
		return;

	// Found the point, now lets apply the decals
	CreateModelInstance();

	// FIXME: Pass in decal up?
	Vector up(0, 0, 1);

	if (doTrace && (GetSolid() == SOLID_VPHYSICS) && !tr.startsolid && !tr.allsolid)
	{
		// Choose a more accurate normal direction
		// Also, since we have more accurate info, we can avoid pokethru
		Vector temp;
		VectorSubtract( tr.endpos, tr.plane.normal, temp );
		Ray_t betterRay;
		betterRay.Init( tr.endpos, temp );
		modelrender->AddColoredDecal( m_ModelInstance, betterRay, up, decalIndex, GetStudioBody(), cColor, true, maxLODToDecal );
	}
	else
	{
		modelrender->AddColoredDecal( m_ModelInstance, ray, up, decalIndex, GetStudioBody(), cColor, false, maxLODToDecal );
	}
}


//-----------------------------------------------------------------------------
// This method works when we've got a brush model
//-----------------------------------------------------------------------------
void C_BaseEntity::AddBrushModelDecal( const Ray_t& ray, const Vector& decalCenter, 
									  int decalIndex, bool doTrace, trace_t& tr )
{
	if ( doTrace )
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );
		if ( tr.fraction == 1.0f )
			return;
	}

	effects->DecalShoot( decalIndex, entindex(), 
		m_pModel, GetAbsOrigin(), GetAbsAngles(), decalCenter, NULL, 0 );
}


//-----------------------------------------------------------------------------
// A method to apply a decal to an entity
//-----------------------------------------------------------------------------
void C_BaseEntity::AddDecal( const Vector& rayStart, const Vector& rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	Ray_t ray;
	ray.Init( rayStart, rayEnd );

	// FIXME: Better bloat?
	// Bloat a little bit so we get the intersection
	ray.m_Delta *= 1.1f;

	int modelType = modelinfo->GetModelType( m_pModel );
	switch ( modelType )
	{
	case mod_studio:
		AddStudioDecal( ray, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
		break;

	case mod_brush:
		AddBrushModelDecal( ray, decalCenter, decalIndex, doTrace, tr );
		break;

	default:
		// By default, no collision
		tr.fraction = 1.0f;
		break;
	}
}

//-----------------------------------------------------------------------------
void C_BaseEntity::AddColoredDecal( const Vector& rayStart, const Vector& rayEnd,
	const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, Color cColor, int maxLODToDecal )
{
	Ray_t ray;
	ray.Init( rayStart, rayEnd );
	
	// FIXME: Better bloat?
	// Bloat a little bit so we get the intersection
	ray.m_Delta *= 1.1f;

	int modelType = modelinfo->GetModelType( m_pModel );
	if ( doTrace )
	{
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, this, &tr );
		switch ( modelType )
		{
		case mod_studio:
			tr.m_pEnt = this;
			break;
		case mod_brush:
			if ( tr.fraction == 1.0f )
				return;		// Explicitly end
		default:
			// By default, no collision
			tr.fraction = 1.0f;
			break;
		}
	}

	switch ( modelType )
	{
	case mod_studio:
		AddColoredStudioDecal( ray, hitbox, decalIndex, doTrace, tr, cColor, maxLODToDecal );
		break;

	case mod_brush:
		{
			color32 cColor32 = { (byte)cColor.r(), (byte)cColor.g(), (byte)cColor.b(), (byte)cColor.a() };
			effects->DecalColorShoot( decalIndex, entindex(), m_pModel, GetAbsOrigin(), GetAbsAngles(), decalCenter, 0, 0, cColor32 );
		}
		break;

	default:
		// By default, no collision
		tr.fraction = 1.0f;
		break;
	}
}

//-----------------------------------------------------------------------------
// A method to remove all decals from an entity
//-----------------------------------------------------------------------------
void C_BaseEntity::RemoveAllDecals( void )
{
	// For now, we only handle removing decals from studiomodels
	if ( modelinfo->GetModelType( m_pModel ) == mod_studio )
	{
		CreateModelInstance();
		modelrender->RemoveAllDecals( m_ModelInstance );
	}
}

bool C_BaseEntity::SnatchModelInstance( C_BaseEntity *pToEntity )
{
	if ( !modelrender->ChangeInstance(  GetModelInstance(), pToEntity ) )
		return false;  // engine could move modle handle

	// remove old handle from toentity if any
	if ( pToEntity->GetModelInstance() != MODEL_INSTANCE_INVALID )
		 pToEntity->DestroyModelInstance();

	// move the handle to other entity
	pToEntity->SetModelInstance(  GetModelInstance() );

	// delete own reference
	SetModelInstance( MODEL_INSTANCE_INVALID );

	return true;
}

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// C_BaseEntity new/delete
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *C_BaseEntity::operator new( size_t stAllocateBlock )
{
	Assert( stAllocateBlock != 0 );	
	MEM_ALLOC_CREDIT();
	void *pMem = MemAlloc_Alloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void *C_BaseEntity::operator new[]( size_t stAllocateBlock )
{
	Assert( stAllocateBlock != 0 );				
	MEM_ALLOC_CREDIT();
	void *pMem = MemAlloc_Alloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void *C_BaseEntity::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	Assert( stAllocateBlock != 0 );	
	void *pMem = MemAlloc_Alloc( stAllocateBlock, pFileName, nLine );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}

void *C_BaseEntity::operator new[]( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	Assert( stAllocateBlock != 0 );				
	void *pMem = MemAlloc_Alloc( stAllocateBlock, pFileName, nLine );
	memset( pMem, 0, stAllocateBlock );
	return pMem;												
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMem - 
//-----------------------------------------------------------------------------
void C_BaseEntity::operator delete( void *pMem )
{
	// get the engine to free the memory
	MemAlloc_Free( pMem );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pMem -
//-----------------------------------------------------------------------------
void C_BaseEntity::operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )
{
	// get the engine to free the memory
	MemAlloc_Free( pMem, pFileName, nLine );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pMem -
//-----------------------------------------------------------------------------
void C_BaseEntity::operator delete[]( void *pMem )
{
	// get the engine to free the memory
	MemAlloc_Free( pMem );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pMem -
//-----------------------------------------------------------------------------
void C_BaseEntity::operator delete[]( void *pMem, int nBlockUse, const char *pFileName, int nLine )
{
	// get the engine to free the memory
	MemAlloc_Free( pMem, pFileName, nLine );
}

#include "tier0/memdbgon.h"

//========================================================================================
// TEAM HANDLING
//========================================================================================
C_Team *C_BaseEntity::GetTeam( void )
{
	return GetGlobalTeamByTeam( m_iTeamNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Team_t C_BaseEntity::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Team_t	C_BaseEntity::GetRenderTeamNumber( void )
{
	return GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if these entities are both in at least one team together
//-----------------------------------------------------------------------------
bool C_BaseEntity::InSameTeam( C_BaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	return ( pEntity->GetTeam() == GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity's on the same team as the local player
//-----------------------------------------------------------------------------
bool C_BaseEntity::InLocalTeam( void )
{
	return ( GetTeam() == GetLocalTeam() );
}


void C_BaseEntity::AddToLeafSystem()
{
	AddToLeafSystem( IsRenderingWithViewModels() );
}

void C_BaseEntity::AddToLeafSystem( bool bRenderWithViewModels )
{
	m_bRenderWithViewModels = bRenderWithViewModels;
	if( m_hRender == INVALID_CLIENT_RENDER_HANDLE )
	{
		// create new renderer handle
		ClientLeafSystem()->AddRenderable( this, bRenderWithViewModels, ComputeTranslucencyType(), RENDERABLE_MODEL_UNKNOWN_TYPE );
		ClientLeafSystem()->EnableAlternateSorting( m_hRender, m_bAlternateSorting );
		ClientLeafSystem()->DisableCachedRenderBounds( m_hRender, m_bDisableCachedRenderBounds );
	}
	else
	{
		// handle already exists, just update group & origin
		ClientLeafSystem()->RenderWithViewModels( m_hRender, bRenderWithViewModels );
		// this should already be edge detected, no need to do it per frame
		// ClientLeafSystem()->SetTranslucencyType( m_hRender, ComputeTranslucencyType() );
		ClientLeafSystem()->SetModelType( m_hRender );
		ClientLeafSystem()->DisableCachedRenderBounds( m_hRender, m_bDisableCachedRenderBounds );
	}
}


//-----------------------------------------------------------------------------
// Creates the shadow (if it doesn't already exist) based on shadow cast type
//-----------------------------------------------------------------------------
void C_BaseEntity::CreateShadow()
{
	ShadowType_t shadowType = ShadowCastType();
	if (shadowType == SHADOWS_NONE)
	{
		DestroyShadow();
	}
	else
	{
		if (m_ShadowHandle == CLIENTSHADOW_INVALID_HANDLE)
		{
			int flags = SHADOW_FLAGS_SHADOW;
			if (shadowType != SHADOWS_SIMPLE)
				flags |= SHADOW_FLAGS_USE_RENDER_TO_TEXTURE;
			if (shadowType == SHADOWS_RENDER_TO_TEXTURE_DYNAMIC || shadowType == SHADOWS_RENDER_TO_TEXTURE_DYNAMIC_CUSTOM)
				flags |= SHADOW_FLAGS_ANIMATING_SOURCE;
			if (shadowType != SHADOWS_RENDER_TO_TEXTURE_DYNAMIC_CUSTOM)
				flags |= SHADOW_FLAGS_ANIMATING_SOURCE | SHADOW_FLAGS_CUSTOM_DRAW;
			m_ShadowHandle = g_pClientShadowMgr->CreateShadow(GetClientHandle(), entindex(), flags);
		}
	}
}

//-----------------------------------------------------------------------------
// Removes the shadow
//-----------------------------------------------------------------------------
void C_BaseEntity::DestroyShadow()
{
	// NOTE: This will actually cause the shadow type to be recomputed
	// if the entity doesn't immediately go away
	if (m_ShadowHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyShadow(m_ShadowHandle);
		m_ShadowHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Removes the entity from the leaf system
//-----------------------------------------------------------------------------
void C_BaseEntity::RemoveFromLeafSystem()
{
	// Detach from the leaf lists.
	if( m_hRender != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_hRender );
		m_hRender = INVALID_CLIENT_RENDER_HANDLE;
	}
	DestroyShadow();
}


//-----------------------------------------------------------------------------
// Purpose: Flags this entity as being inside or outside of this client's PVS
//			on the server.
//			NOTE: this is meaningless for client-side only entities.
// Input  : inside_pvs - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetDormant( bool bDormant )
{
	Assert( IsServerEntity() );
	m_bDormant = bDormant;

	// Kill drawing if we became dormant.
	UpdateVisibility();

	ParticleProp()->OwnerSetDormantTo( bDormant );

	OnSetDormant( bDormant );
	cl_entitylist->SetDormant(entindex(), bDormant);
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this entity is dormant. Client/server entities become
//			dormant when they leave the PVS on the server. Client side entities
//			can decide for themselves whether to become dormant.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsDormant( void )
{
	if ( IsServerEntity() )
	{
		return m_bDormant;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Tells the entity that it's about to be destroyed due to the client receiving
// an uncompressed update that's caused it to destroy all entities & recreate them.
//-----------------------------------------------------------------------------
void C_BaseEntity::SetDestroyedOnRecreateEntities( void )
{
	// Robin: We need to destroy all our particle systems immediately, because 
	// we're about to be recreated, and their owner EHANDLEs will match up to 
	// the new entity, but it won't know anything about them.
	ParticleProp()->StopEmissionAndDestroyImmediately();
}

//-----------------------------------------------------------------------------
// These methods recompute local versions as well as set abs versions
//-----------------------------------------------------------------------------
void C_BaseEntity::SetAbsOrigin( const Vector& absOrigin )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	if ( m_vecAbsOrigin == absOrigin )
		return;

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( POSITION_CHANGED );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_vecAbsOrigin = absOrigin;
	MatrixSetColumn( absOrigin, 3, m_rgflCoordinateFrame ); 

	C_BaseEntity *pMoveParent = GetMoveParent();

	if (!pMoveParent)
	{
		m_vecOrigin = absOrigin;
		return;
	}

	// Moveparent case: transform the abs position into local space
	VectorITransform( absOrigin, pMoveParent->EntityToWorldTransform(), (Vector&)m_vecOrigin );
}

void C_BaseEntity::SetAbsAngles( const QAngle& absAngles )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// FIXME: The normalize caused problems in server code like momentary_rot_button that isn't
	//        handling things like +/-180 degrees properly. This should be revisited.
	//QAngle angleNormalize( AngleNormalize( absAngles.x ), AngleNormalize( absAngles.y ), AngleNormalize( absAngles.z ) );

	if ( m_angAbsRotation == absAngles )
		return;

	InvalidatePhysicsRecursive( ANGLES_CHANGED );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_angAbsRotation = absAngles;
	AngleMatrix( absAngles, m_rgflCoordinateFrame );
	MatrixSetColumn( m_vecAbsOrigin, 3, m_rgflCoordinateFrame ); 

	C_BaseEntity *pMoveParent = GetMoveParent();
	
	if (!pMoveParent)
	{
		m_angRotation = absAngles;
		return;
	}

	// Moveparent case: we're aligned with the move parent
	if ( m_angAbsRotation == pMoveParent->GetAbsAngles() )
	{
		m_angRotation.Init( );
	}
	else
	{
		// Moveparent case: transform the abs transform into local space
		matrix3x4_t worldToParent, localMatrix;
		MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
		ConcatTransforms( worldToParent, m_rgflCoordinateFrame, localMatrix );
		MatrixAngles( localMatrix, (QAngle &)m_angRotation );
	}
}

void C_BaseEntity::SetAbsVelocity( const Vector &vecAbsVelocity )
{
	if ( m_vecAbsVelocity == vecAbsVelocity )
		return;

	// The abs velocity won't be dirty since we're setting it here
	InvalidatePhysicsRecursive( VELOCITY_CHANGED );
	m_iEFlags &= ~EFL_DIRTY_ABSVELOCITY;

	m_vecAbsVelocity = vecAbsVelocity;

	C_BaseEntity *pMoveParent = GetMoveParent();

	if (!pMoveParent)
	{
		m_vecVelocity = vecAbsVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// velocity measured in world space
	Vector relVelocity;
	VectorSubtract( vecAbsVelocity, pMoveParent->GetAbsVelocity(), relVelocity );

	// Transform velocity into parent space
	VectorIRotate( relVelocity, pMoveParent->EntityToWorldTransform(), m_vecVelocity );
}

/*
void C_BaseEntity::SetAbsAngularVelocity( const QAngle &vecAbsAngVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
	m_iEFlags &= ~EFL_DIRTY_ABSANGVELOCITY;

	m_vecAbsAngVelocity = vecAbsAngVelocity;

	C_BaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecAngVelocity = vecAbsAngVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// angular velocity measured in world space
	QAngle relAngVelocity;
	relAngVelocity = vecAbsAngVelocity - pMoveParent->GetAbsAngularVelocity();

	matrix3x4_t entityToWorld;
	AngleMatrix( relAngVelocity, entityToWorld );

	// Moveparent case: transform the abs angular vel into local space
	matrix3x4_t worldToParent, localMatrix;
	MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
	ConcatTransforms( worldToParent, entityToWorld, localMatrix );
	MatrixAngles( localMatrix, m_vecAngVelocity );
}
*/


// Prevent these for now until hierarchy is properly networked
const Vector& C_BaseEntity::GetLocalOrigin( void ) const
{
	return m_vecOrigin;
}

vec_t C_BaseEntity::GetLocalOriginDim( int iDim ) const
{
	return m_vecOrigin[iDim];
}

// Prevent these for now until hierarchy is properly networked
void C_BaseEntity::SetLocalOrigin( const Vector& origin )
{
	if (m_vecOrigin != origin)
	{
		InvalidatePhysicsRecursive( POSITION_CHANGED );
		m_vecOrigin = origin;
	}
}

void C_BaseEntity::SetLocalOriginDim( int iDim, vec_t flValue )
{
	if (m_vecOrigin[iDim] != flValue)
	{
		InvalidatePhysicsRecursive( POSITION_CHANGED );
		m_vecOrigin[iDim] = flValue;
	}
}


// Prevent these for now until hierarchy is properly networked
const QAngle& C_BaseEntity::GetLocalAngles( void ) const
{
	return m_angRotation;
}

vec_t C_BaseEntity::GetLocalAnglesDim( int iDim ) const
{
	return m_angRotation[iDim];
}

// Prevent these for now until hierarchy is properly networked
void C_BaseEntity::SetLocalAngles( const QAngle& angles )
{
	// NOTE: The angle normalize is a little expensive, but we can save
	// a bunch of time in interpolation if we don't have to invalidate everything
	// and sometimes it's off by a normalization amount

	// FIXME: The normalize caused problems in server code like momentary_rot_button that isn't
	//        handling things like +/-180 degrees properly. This should be revisited.
	//QAngle angleNormalize( AngleNormalize( angles.x ), AngleNormalize( angles.y ), AngleNormalize( angles.z ) );

	if (m_angRotation != angles)
	{
		// This will cause the velocities of all children to need recomputation
		InvalidatePhysicsRecursive( ANGLES_CHANGED );
		m_angRotation = angles;
	}
}

void C_BaseEntity::SetLocalAnglesDim( int iDim, vec_t flValue )
{
	flValue = AngleNormalize( flValue );
	if (m_angRotation[iDim] != flValue)
	{
		// This will cause the velocities of all children to need recomputation
		InvalidatePhysicsRecursive( ANGLES_CHANGED );
		m_angRotation[iDim] = flValue;
	}
}

void C_BaseEntity::SetLocalVelocity( const Vector &vecVelocity )
{
	if (m_vecVelocity != vecVelocity)
	{
		InvalidatePhysicsRecursive( VELOCITY_CHANGED );
		m_vecVelocity = vecVelocity; 
	}
}

void C_BaseEntity::SetLocalAngularVelocity( const QAngle &vecAngVelocity )
{
	if (m_vecAngVelocity != vecAngVelocity)
	{
//		InvalidatePhysicsRecursive( ANG_VELOCITY_CHANGED );
		m_vecAngVelocity = vecAngVelocity;
	}
}


//-----------------------------------------------------------------------------
// Sets the local position from a transform
//-----------------------------------------------------------------------------
void C_BaseEntity::SetLocalTransform( const matrix3x4_t &localTransform )
{
	Vector vecLocalOrigin;
	QAngle vecLocalAngles;
	MatrixGetColumn( localTransform, 3, vecLocalOrigin );
	MatrixAngles( localTransform, vecLocalAngles );
	SetLocalOrigin( vecLocalOrigin );
	SetLocalAngles( vecLocalAngles );
}


//-----------------------------------------------------------------------------
// FIXME: REMOVE!!!
//-----------------------------------------------------------------------------
void C_BaseEntity::MoveToAimEnt( )
{
	Vector vecAimEntOrigin;
	QAngle vecAimEntAngles;
	GetAimEntOrigin( GetMoveParent(), &vecAimEntOrigin, &vecAimEntAngles );
	SetAbsOrigin( vecAimEntOrigin );
	SetAbsAngles( vecAimEntAngles );
}


void C_BaseEntity::BoneMergeFastCullBloat( Vector &localMins, Vector &localMaxs, const Vector &thisEntityMins, const Vector &thisEntityMaxs ) const
{
	// By default, we bloat the bbox for fastcull ents by the maximum length it could hang out of the parent bbox,
	// it one corner were touching the edge of the parent's box, and the whole diagonal stretched out.
	float flExpand = (thisEntityMaxs - thisEntityMins).Length();

	localMins.x -= flExpand;
	localMins.y -= flExpand;
	localMins.z -= flExpand;

	localMaxs.x += flExpand;
	localMaxs.y += flExpand;
	localMaxs.z += flExpand;
}


matrix3x4_t& C_BaseEntity::GetParentToWorldTransform( matrix3x4_t &tempMatrix )
{
	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		Assert( false );
		SetIdentityMatrix( tempMatrix );
		return tempMatrix;
	}

	if ( m_iParentAttachment != 0 )
	{
		Vector vOrigin;
		QAngle vAngles;
		if ( pMoveParent->GetAttachment( m_iParentAttachment, vOrigin, vAngles ) )
		{
			AngleMatrix( vAngles, vOrigin, tempMatrix );
			return tempMatrix;
		}
	}
	
	// If we fall through to here, then just use the move parent's abs origin and angles.
	return pMoveParent->EntityToWorldTransform();
}


//-----------------------------------------------------------------------------
// Purpose: Calculates the absolute position of an edict in the world
//			assumes the parent's absolute origin has already been calculated
//-----------------------------------------------------------------------------
void C_BaseEntity::CalcAbsolutePosition( )
{
	// There are periods of time where we're gonna have to live with the
	// fact that we're in an indeterminant state and abs queries (which
	// shouldn't be happening at all; I have assertions for those), will
	// just have to accept stale data.
	if (!s_bAbsRecomputationEnabled)
		return;

	// FIXME: Recompute absbox!!!
	if ((m_iEFlags & EFL_DIRTY_ABSTRANSFORM) == 0)
	{
		// quick check to make sure we really don't need an update
		// Assert( m_pMoveParent || m_vecAbsOrigin == GetLocalOrigin() );
		return;
	}

	AUTO_LOCK( m_CalcAbsolutePositionMutex );

	if ((m_iEFlags & EFL_DIRTY_ABSTRANSFORM) == 0) // need second check in event another thread grabbed mutex and did the calculation
	{
		return;
	}

	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	if (!m_pMoveParent)
	{
		// Construct the entity-to-world matrix
		// Start with making an entity-to-parent matrix
		AngleMatrix( GetLocalAngles(), GetLocalOrigin(), m_rgflCoordinateFrame );
		m_vecAbsOrigin = GetLocalOrigin();
		m_angAbsRotation = GetLocalAngles();
		NormalizeAngles( m_angAbsRotation );
		return;
	}
	
	if ( IsEffectActive(EF_BONEMERGE) )
	{
		MoveToAimEnt();
		return;
	}

	// Construct the entity-to-world matrix
	// Start with making an entity-to-parent matrix
	matrix3x4_t matEntityToParent;
	AngleMatrix( GetLocalAngles(), matEntityToParent );
	MatrixSetColumn( GetLocalOrigin(), 3, matEntityToParent );

	// concatenate with our parent's transform
	matrix3x4_t scratchMatrix;
	ConcatTransforms( GetParentToWorldTransform( scratchMatrix ), matEntityToParent, m_rgflCoordinateFrame );

	// pull our absolute position out of the matrix
	MatrixGetColumn( m_rgflCoordinateFrame, 3, m_vecAbsOrigin );

	// if we have any angles, we have to extract our absolute angles from our matrix
	if ( m_angRotation == vec3_angle && m_iParentAttachment == 0 )
	{
		// just copy our parent's absolute angles
		VectorCopy( m_pMoveParent->GetAbsAngles(), m_angAbsRotation );
	}
	else
	{
		MatrixAngles( m_rgflCoordinateFrame, m_angAbsRotation );
	}

	// This is necessary because it's possible that our moveparent's CalculateIKLocks will trigger its move children 
	// (ie: this entity) to call GetAbsOrigin(), and they'll use the moveparent's OLD bone transforms to get their attachments
	// since the moveparent is right in the middle of setting up new transforms. 
	//
	// So here, we keep our absorigin invalidated. It means we're returning an origin that is a frame old to CalculateIKLocks,
	// but we'll still render with the right origin.
	if ( m_iParentAttachment != 0 && (m_pMoveParent->GetEFlags() & EFL_SETTING_UP_BONES) )
	{
		m_iEFlags |= EFL_DIRTY_ABSTRANSFORM;
	}
}

void C_BaseEntity::CalcAbsoluteVelocity()
{
	if ((m_iEFlags & EFL_DIRTY_ABSVELOCITY ) == 0)
		return;

	AUTO_LOCK( m_CalcAbsoluteVelocityMutex );

	if ((m_iEFlags & EFL_DIRTY_ABSVELOCITY) == 0) // need second check in event another thread grabbed mutex and did the calculation
	{
		return;
	}

	m_iEFlags &= ~EFL_DIRTY_ABSVELOCITY;

	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsVelocity = m_vecVelocity;
		return;
	}

	VectorRotate( m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity );


	// Add in the attachments velocity if it exists
	if ( m_iParentAttachment != 0 )
	{
		Vector vOriginVel;
		Quaternion vAngleVel;
		if ( pMoveParent->GetAttachmentVelocity( m_iParentAttachment, vOriginVel, vAngleVel ) )
		{
			m_vecAbsVelocity += vOriginVel;
			return;
		}
	}

	// Now add in the parent abs velocity
	m_vecAbsVelocity += pMoveParent->GetAbsVelocity();
}

/*
void C_BaseEntity::CalcAbsoluteAngularVelocity()
{
	if ((m_iEFlags & EFL_DIRTY_ABSANGVELOCITY ) == 0)
		return;

	m_iEFlags &= ~EFL_DIRTY_ABSANGVELOCITY;

	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsAngVelocity = m_vecAngVelocity;
		return;
	}

	matrix3x4_t angVelToParent, angVelToWorld;
	AngleMatrix( m_vecAngVelocity, angVelToParent );
	ConcatTransforms( pMoveParent->EntityToWorldTransform(), angVelToParent, angVelToWorld );
	MatrixAngles( angVelToWorld, m_vecAbsAngVelocity );

	// Now add in the parent abs angular velocity
	m_vecAbsAngVelocity += pMoveParent->GetAbsAngularVelocity();
}
*/


//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition )
{
	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsPosition = vecLocalPosition;
	}
	else
	{
		VectorTransform( vecLocalPosition, pMoveParent->EntityToWorldTransform(), *pAbsPosition );
	}
}


//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection )
{
	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsDirection = vecLocalDirection;
	}
	else
	{
		VectorRotate( vecLocalDirection, pMoveParent->EntityToWorldTransform(), *pAbsDirection );
	}
}



//-----------------------------------------------------------------------------
// Mark shadow as dirty 
//-----------------------------------------------------------------------------
void C_BaseEntity::MarkRenderHandleDirty( )
{
	// Invalidate render leaf too
	ClientRenderHandle_t handle = GetRenderHandle();
	if ( handle != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RenderableChanged( handle );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ShutdownPredictable( void )
{
	Assert( GetPredictable() );

	g_Predictables.RemoveFromPredictablesList( GetClientHandle() );
	DestroyIntermediateData();
	SetPredictable( false );
}

//-----------------------------------------------------------------------------
// Purpose: Turn entity into something the predicts locally
//-----------------------------------------------------------------------------
void C_BaseEntity::InitPredictable( C_BasePlayer *pOwner )
{
	Assert( !GetPredictable() );

	// Mark as predictable
	SetPredictable( true );
	// Allocate buffers into which we copy data
	AllocateIntermediateData();
	// Add to list of predictables
	g_Predictables.AddToPredictableList( GetClientHandle() );
	// Copy everything from "this" into the original_state_data
	//  object.  Don't care about client local stuff, so pull from slot 0 which

	//  should be empty anyway...
	PostNetworkDataReceived( 0 );

	// Copy original data into all prediction slots, so we don't get an error saying we "mispredicted" any
	//  values which are still at their initial values
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		SaveData( "InitPredictable", i, PC_EVERYTHING );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetPredictable( bool state )
{
	m_bPredictable = state;

	// update interpolation times
	Interp_UpdateInterpolationAmounts( GetVarMapping() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetPredictable( void ) const
{
	return m_bPredictable;
}

//-----------------------------------------------------------------------------
// Purpose: Transfer data for intermediate frame to current entity
// Input  : copyintermediate - 
//			last_predicted - 
//-----------------------------------------------------------------------------
void C_BaseEntity::PreEntityPacketReceived( int commands_acknowledged )					
{				
	// Don't need to copy intermediate data if server did ack any new commands
	bool copyintermediate = ( commands_acknowledged > 0 ) ? true : false;

	Assert( GetPredictable() );
	Assert( cl_predict->GetInt() );

	// First copy in any intermediate predicted data for non-networked fields
	if ( copyintermediate )
	{
		RestoreData( "PreEntityPacketReceived", commands_acknowledged - 1, PC_NON_NETWORKED_ONLY );
		RestoreData( "PreEntityPacketReceived", SLOT_ORIGINALDATA, PC_NETWORKED_ONLY );
	}
	else
	{
		RestoreData( "PreEntityPacketReceived(no commands ack)", SLOT_ORIGINALDATA, PC_EVERYTHING );
	}

	// At this point the entity has original network data restored as of the last time the 
	// networking was updated, and it has any intermediate predicted values properly copied over
	// Unpacked and OnDataChanged will fill in any changed, networked fields.

	// That networked data will be copied forward into the starting slot for the next prediction round
}	

//-----------------------------------------------------------------------------
// Purpose: Called every time PreEntityPacket received is called
//  copy any networked data into original_state
// Input  : errorcheck - 
//			last_predicted - 
//-----------------------------------------------------------------------------
void C_BaseEntity::PostEntityPacketReceived( void )
{
	Assert( GetPredictable() );
	Assert( cl_predict->GetInt() );

	// Always mark as changed
	AddDataChangeEvent( this, DATA_UPDATE_DATATABLE_CHANGED, &m_DataChangeEventRef );

	// Save networked fields into "original data" store
	SaveData( "PostEntityPacketReceived", SLOT_ORIGINALDATA, PC_NETWORKED_ONLY );
}

//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all updating is done
// Input  : errorcheck - 
//			last_predicted - 
//-----------------------------------------------------------------------------
bool C_BaseEntity::PostNetworkDataReceived( int commands_acknowledged )
{
	bool haderrors = false;

	Assert( GetPredictable() );

	bool errorcheck = ( commands_acknowledged > 0 ) ? true : false;

	// Store network data into post networking pristine state slot (slot 64) 
	SaveData( "PostNetworkDataReceived", SLOT_ORIGINALDATA, PC_EVERYTHING );

	// Show any networked fields that are different
	bool showthis = cl_showerror.GetInt() >= 2;

	if ( cl_showerror.GetInt() < 0 )
	{
		if ( entindex() == -cl_showerror.GetInt() )
		{
			showthis = true;
		}
		else
		{
			showthis = false;
		}
	}

	if ( errorcheck )
	{
		void *predicted_state_data = GetPredictedFrame( commands_acknowledged - 1 );	
		Assert( predicted_state_data );												
		const void *original_state_data = GetOriginalNetworkDataObject();
		Assert( original_state_data );

		bool counterrors = true;
		bool reporterrors = showthis;
		bool copydata	= false;

		CPredictionCopy errorCheckHelper( PC_NETWORKED_ONLY, 
			predicted_state_data, PC_DATA_PACKED, 
			original_state_data, PC_DATA_PACKED, 
			counterrors, reporterrors, copydata );
		// Suppress debugging output
		int ecount = errorCheckHelper.TransferData( "", -1, GetPredDescMap() );
		if ( ecount > 0 )
		{
			haderrors = true;
		//	Msg( "%i errors %i on entity %i %s\n", gpGlobals->tickcount, ecount, index, IsClientCreated() ? "true" : "false" );
		}
	}

	return haderrors;
}

// Stuff implemented for weapon prediction code
void C_BaseEntity::SetSize( const Vector &vecMin, const Vector &vecMax )
{
	SetCollisionBounds( vecMin, vecMax );
}

//-----------------------------------------------------------------------------
// Purpose: Just look up index
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::PrecacheModel( const char *name )
{
	return modelinfo->GetModelIndex( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *obj - 
//-----------------------------------------------------------------------------
void UTIL_Remove( C_BaseEntity *pEntity )
{
	if(!pEntity)
		return;

	if ( pEntity->IsMarkedForDeletion( ) )
		return;
	pEntity->AddEFlags( EFL_KILLME );	// Make sure to ignore further calls into here or UTIL_Remove.

	if ( !C_BaseEntity::s_bImmediateRemovesAllowed )
	{
		pEntity->AddToEntityList( ENTITY_LIST_DELETE );
		return;
	}

	pEntity->DO_NOT_USE_Release();
}

static bool in_purge_entities = false;

void UTIL_RemoveImmediate( C_BaseEntity *pEntity )
{
	if(!pEntity)
		return;

	bool lastallowed = C_BaseEntity::s_bImmediateRemovesAllowed;
	C_BaseEntity::s_bImmediateRemovesAllowed = true;
	pEntity->AddEFlags( EFL_KILLME );
	if(!in_purge_entities) {
		pEntity->RemoveFromEntityList( ENTITY_LIST_DELETE );
	}
	pEntity->DO_NOT_USE_Release();
	C_BaseEntity::s_bImmediateRemovesAllowed = lastallowed;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::GetPredictionEligible( void ) const
{
	return m_bPredictionEligible;
}


C_BaseEntity* C_BaseEntity::Instance( CBaseHandle hEnt )
{
	return ClientEntityList().GetBaseEntityFromHandle( hEnt );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iEnt - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *C_BaseEntity::Instance( int iEnt )
{
	return ClientEntityList().GetBaseEntity( iEnt );
}

bool EntityNamesMatch( const char *pszQuery, string_t nameToMatch )
{
	// NamesMatch has been turned into Matcher_NamesMatch in matchers.h
	// for a wider range of accessibility and flexibility.
	return Matcher_NamesMatch(pszQuery, STRING(nameToMatch));
}

bool C_BaseEntity::NameMatchesComplex( const char *pszNameOrWildcard )
{
	if ( !Q_stricmp( "!player", pszNameOrWildcard) )
		return IsPlayer();

	return Matcher_NamesMatch( pszNameOrWildcard, STRING(m_iName) );
}

bool C_BaseEntity::ClassMatchesComplex( const char *pszClassOrWildcard )
{
	return Matcher_NamesMatch( pszClassOrWildcard, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_BaseEntity::GetClassname( void )
{
	if(m_iClassname != NULL_STRING)
		return STRING(m_iClassname);

	return STRING(m_iRTTIClassname);
}

string_t C_BaseEntity::GetClassnameStr()
{
	if(m_iClassname != NULL_STRING)
		return m_iClassname;

	return m_iRTTIClassname;
}

bool C_BaseEntity::HasClassname()
{
	if(m_iClassname != NULL_STRING)
		return true;

	return false;
}

const char *C_BaseEntity::GetDebugName( void )
{
	if ( m_iName != NULL_STRING ) 
	{
		return STRING(m_iName);
	}
	else
	{
		if ( ToBasePlayer( this ) )
		{
			return ToBasePlayer( this )->GetPlayerName();
		}

		return GetClassname();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates an entity by string name, but does not spawn it
// Input  : *className - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *CreateEntityByName( const char *className )
{
	C_BaseEntity *ent = GetClassMap().CreateEntity( className );
	if ( ent )
	{
		return ent;
	}

	return NULL;
}

int DispatchSpawn( C_BaseEntity *pEntity )
{
	if ( pEntity )
	{
		MDLCACHE_CRITICAL_SECTION();

		// keep a smart pointer that will now if the object gets deleted
		EHANDLE pEntSafe;
		pEntSafe = pEntity;

		// Initialize these or entities who don't link to the world won't have anything in here
		// is this necessary?
		//pEntity->SetAbsMins( pEntity->GetOrigin() - Vector(1,1,1) );
		//pEntity->SetAbsMaxs( pEntity->GetOrigin() + Vector(1,1,1) );

#if defined(TRACK_ENTITY_MEMORY) && defined(USE_MEM_DEBUG)
		const char *pszClassname = GetClassMap().MapNameToClassname( pEntity->GetClassname() );
		if ( pszClassname )
		{
			MemAlloc_PushAllocDbgInfo( pszClassname, __LINE__ );
		}
#endif
		bool bAsyncAnims = g_pMDLCache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
		pEntity->m_flSpawnTime = engine->GetLastTimeStamp();
		pEntity->Spawn();
		g_pMDLCache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );

#if defined(TRACK_ENTITY_MEMORY) && defined(USE_MEM_DEBUG)
		if ( pszClassname )
		{
			MemAlloc_PopAllocDbgInfo();
		}
#endif
		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.

		if ( pEntSafe == NULL || pEntity->IsMarkedForDeletion() )
			return -1;
	}

	return 0;
}

#ifdef _DEBUG
CON_COMMAND( cl_sizeof, "Determines the size of the specified client class." )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "cl_sizeof <gameclassname>\n" );
		return;
	}

	int size = GetClassMap().GetClassSize( args[ 1 ] );

	Msg( "%s is %i bytes\n", args[ 1 ], size );
}
#endif

CON_COMMAND_F( dlight_debug, "Creates a dlight in front of the player", FCVAR_CHEAT )
{
	dlight_t *el = effects->CL_AllocDlight( 1 );
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;
	Vector start = player->EyePosition();
	Vector forward;
	player->EyeVectors( &forward );
	Vector end = start + forward * MAX_TRACE_LENGTH;
	trace_t tr;
	UTIL_TraceLine( start, end, MASK_SHOT_HULL & (~CONTENTS_GRATE), player, COLLISION_GROUP_NONE, &tr );
	el->origin = tr.endpos - forward * 12.0f;
	el->radius = 200; 
	el->decay = el->radius / 5.0f;
	el->die = gpGlobals->curtime + 5.0f;
	el->color.r = 255;
	el->color.g = 192;
	el->color.b = 64;
	el->color.exponent = 5;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classname - 
//			*module - 
//			line - 
// Output : C_BaseEntity
//-----------------------------------------------------------------------------
C_BaseEntity *C_BaseEntity::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, C_BaseEntity *pOwner )
{
	C_BaseEntity *ent = NULL;

	// Try to create it
	ent = GetClassMap().CreateEntity( szName );
	if ( !ent )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	ent->InitializeAsClientEntity();

	ent->SetLocalOrigin( vecOrigin );
	ent->SetLocalAngles( vecAngles );
	ent->SetOwnerEntity( pOwner );

	return ent;
}

C_BaseEntity *C_BaseEntity::Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, C_BaseEntity *pOwner )
{
	C_BaseEntity *ent = NULL;

	// Try to create it
	ent = CreateNoSpawn( szName, vecOrigin, vecAngles, pOwner );
	if ( !ent )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	if(DispatchSpawn( ent ) == -1)
		return NULL;

	return ent;
}

C_BaseEntity *C_BaseEntity::DO_NOT_USE_CreatePredictedNoSpawn( const char *module, int line, const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, C_BaseEntity *pOwner )
{
	C_BaseEntity *ent = NULL;

	// Try to create it
	ent = GetClassMap().CreateEntity( szName );
	if ( !ent )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	ent->InitializeAsPredictedEntity( szName, module, line );

	ent->SetLocalOrigin( vecOrigin );
	ent->SetLocalAngles( vecAngles );
	ent->SetOwnerEntity( pOwner );

	return ent;
}

C_BaseEntity *C_BaseEntity::DO_NOT_USE_CreatePredicted( const char *module, int line, const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, C_BaseEntity *pOwner )
{
	C_BaseEntity *ent = NULL;

	// Try to create it
	ent = DO_NOT_USE_CreatePredictedNoSpawn( module, line, szName, vecOrigin, vecAngles, pOwner );
	if ( !ent )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	if(DispatchSpawn( ent ) == -1)
		return NULL;

	return ent;
}

int C_BaseEntity::DO_NOT_USE_FindPredicted( C_BaseEntity **pEntity, const char *szName, const char *module, int line )
{
	C_BasePlayer *player = C_BaseEntity::GetPredictionPlayer();

	Assert( player );
	Assert( player->m_pCurrentCommand );
	Assert( prediction->InPrediction() );

	C_BaseEntity *ent = NULL;

	// What's my birthday (should match server)
	int command_number	= player->m_pCurrentCommand->command_number;
	// Who's my daddy?
	int player_index	= player->entindex() - 1;

	// Create id/context
	CPredictableId testId;
	testId.Init( player_index, command_number, szName, module, line );

	// If repredicting, should be able to find the entity in the previously created list
	if ( !prediction->IsFirstTimePredicted() )
	{
		ent = FindPreviouslyCreatedEntity( testId );
		if ( ent )
		{
			*pEntity = ent;
			return 1;
		}
	}

	return 2;
}

//-----------------------------------------------------------------------------
// Purpose: Called each packet that the entity is created on and finally gets called after the next packet
//  that doesn't have a create message for the "parent" entity so that the predicted version
//  can be removed.  Return true to delete entity right away.
//-----------------------------------------------------------------------------
bool C_BaseEntity::OnPredictedEntityRemove( bool isbeingremoved, C_BaseEntity *predicted )
{
	// Nothing right now, but in theory you could look at the error in origins and set
	//  up something to smooth out the error
	PredictionContext *ctx = predicted->m_pPredictionContext;
	Assert( ctx );
	if ( ctx )
	{
		// Create backlink to actual entity
		ctx->m_hServerEntity = this;

		/*
		Msg( "OnPredictedEntity%s:  %s created %s(%i) instance(%i)\n",
			isbeingremoved ? "Remove" : "Acknowledge",
			predicted->GetClassname(),
			ctx->m_pszCreationModule,
			ctx->m_nCreationLineNumber,
			predicted->m_PredictableID.GetInstanceNumber() );
		*/
	}

	// If it comes through with an ID, it should be eligible
	SetPredictionEligible( true );

	// Start predicting simulation forward from here
	CheckInitPredictable( "OnPredictedEntityRemove" );

	// Always mark it dormant since we are the "real" entity now
	predicted->SetDormantPredictable( true );

	InvalidatePhysicsRecursive( POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED );

	// By default, signal that it should be deleted right away
	// If a derived class implements this method, it might chain to here but return
	// false if it wants to keep the dormant predictable around until the chain of
	//  DATA_UPDATE_CREATED messages passes

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetOwnerEntity( C_BaseEntity *pOwner )
{
	m_hOwnerEntity = pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: Put the entity in the specified team
//-----------------------------------------------------------------------------
void C_BaseEntity::ChangeTeam( Team_t iTeamNum )
{
	m_iTeamNum = iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : name - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetModelName( string_t name )
{
	m_ModelName = name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : string_t
//-----------------------------------------------------------------------------
string_t C_BaseEntity::GetModelName( void ) const
{
	return m_ModelName;
}

ICollideable *C_BaseEntity::GetCollideable()
{
	return CollisionProp();
}

//-----------------------------------------------------------------------------
// Methods relating to solid type + flags
//-----------------------------------------------------------------------------
void C_BaseEntity::SetSolidFlags( int nFlags )
{
	Assert( CollisionProp() );
	CollisionProp()->SetSolidFlags( nFlags );
}

bool C_BaseEntity::IsSolidFlagSet( int flagMask ) const
{
	Assert( CollisionProp() );
	return CollisionProp()->IsSolidFlagSet( flagMask );
}

int	C_BaseEntity::GetSolidFlags( void ) const
{
	Assert( CollisionProp() );
	return CollisionProp()->GetSolidFlags( );
}

void C_BaseEntity::AddSolidFlags( int nFlags )
{
	Assert( CollisionProp() );
	CollisionProp()->AddSolidFlags( nFlags );
}

void C_BaseEntity::RemoveSolidFlags( int nFlags )
{
	Assert( CollisionProp() );
	CollisionProp()->RemoveSolidFlags( nFlags );
}

bool C_BaseEntity::IsSolid() const
{
	Assert( CollisionProp() );
	return CollisionProp()->IsSolid( );
}

void C_BaseEntity::SetSolid( SolidType_t val )
{
	Assert( CollisionProp() );
	CollisionProp()->SetSolid( val );
}

SolidType_t C_BaseEntity::GetSolid( ) const
{
	Assert( CollisionProp() );
	return CollisionProp()->GetSolid( );
}

void C_BaseEntity::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	Assert( CollisionProp() );
	CollisionProp()->SetCollisionBounds( mins, maxs );
}


//-----------------------------------------------------------------------------
// Methods relating to bounds
//-----------------------------------------------------------------------------
const Vector& C_BaseEntity::WorldAlignMins( ) const
{
	Assert( CollisionProp() );
	Assert( !CollisionProp()->IsBoundsDefinedInEntitySpace() );
	Assert( CollisionProp()->GetCollisionAngles() == vec3_angle );
	return CollisionProp()->OBBMins();
}

const Vector& C_BaseEntity::WorldAlignMaxs( ) const
{
	Assert( CollisionProp() );
	Assert( !CollisionProp()->IsBoundsDefinedInEntitySpace() );
	Assert( CollisionProp()->GetCollisionAngles() == vec3_angle );
	return CollisionProp()->OBBMaxs();
}

const Vector& C_BaseEntity::WorldAlignSize( ) const
{
	Assert( CollisionProp() );
	Assert( !CollisionProp()->IsBoundsDefinedInEntitySpace() );
	Assert( CollisionProp()->GetCollisionAngles() == vec3_angle );
	return CollisionProp()->OBBSize();
}

float C_BaseEntity::BoundingRadius() const
{
	Assert( CollisionProp() );
	return CollisionProp()->BoundingRadius();
}

float C_BaseEntity::BoundingRadius2D() const
{
	Assert( CollisionProp() );
	return CollisionProp()->BoundingRadius2D();
}

bool C_BaseEntity::IsPointSized() const
{
	Assert( CollisionProp() );
	return CollisionProp()->BoundingRadius() == 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Nothing yet, could eventually supercede Term()
//-----------------------------------------------------------------------------
void C_BaseEntity::UpdateOnRemove( void )
{
	// Nothing for now, if it's a predicted entity, could flag as "delete" or dormant
	if ( GetPredictable() || IsClientCreated() )
	{
		// Make it solid
		if(!IsEFlagSet(EFL_NOT_COLLIDEABLE))
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
		}
		SetMoveType( MOVETYPE_NONE );
	}

	{
		C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, true );
		UnlinkFromHierarchy();
	}

	// Note that this must be called from here, not the destructor, because otherwise the
	//  vtable is hosed and the derived classes function is not going to get called!!!
	if ( IsIntermediateDataAllocated() )
	{
		DestroyIntermediateData();
	}

	VPhysicsDestroyObject(); 

	Assert( !GetMoveParent() );
	UnlinkFromHierarchy();
	SetGroundEntity( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : canpredict - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetPredictionEligible( bool canpredict )
{
	m_bPredictionEligible = canpredict;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done by this entity.
//-----------------------------------------------------------------------------
float C_BaseEntity::GetAttackDamageScale( void )
{
	float flScale = 1;
// Not hooked up to prediction yet
#if 0
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( !m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
#endif
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsDormantPredictable( void ) const
{
	return m_bDormantPredictable;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dormant - 
//-----------------------------------------------------------------------------
void C_BaseEntity::SetDormantPredictable( bool dormant )
{
	Assert( IsClientCreated() );

	m_bDormantPredictable = true;
	m_nIncomingPacketEntityBecameDormant = prediction->GetIncomingPacketNumber();

// Do we need to do the following kinds of things?
#if 0
	// Remove from collisions
	SetSolid( SOLID_NOT );
	// Don't render
	AddEffects( EF_NODRAW );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine when a dorman client predictable can be safely deleted
//  Note that it can be deleted earlier than this by OnPredictedEntityRemove returning true
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::BecameDormantThisPacket( void ) const
{
	Assert( IsDormantPredictable() );

	if ( m_nIncomingPacketEntityBecameDormant != prediction->GetIncomingPacketNumber() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsIntermediateDataAllocated( void ) const
{
	return m_pOriginalData != NULL ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::AllocateIntermediateData( void )
{				
	if ( m_pOriginalData )
		return;
	size_t allocsize = GetIntermediateDataSize();
	Assert( allocsize > 0 );

	m_pOriginalData = new unsigned char[ allocsize ];
	Q_memset( m_pOriginalData, 0, allocsize );
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		m_pIntermediateData[ i ] = new unsigned char[ allocsize ];
		Q_memset( m_pIntermediateData[ i ], 0, allocsize );
	}

	m_nIntermediateDataCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DestroyIntermediateData( void )
{
	if ( !m_pOriginalData )
		return;
	for ( int i = 0; i < MULTIPLAYER_BACKUP; i++ )
	{
		delete[] m_pIntermediateData[ i ];
		m_pIntermediateData[ i ] = NULL;
	}
	delete[] m_pOriginalData;
	m_pOriginalData = NULL;

	m_nIntermediateDataCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slots_to_remove - 
//			number_of_commands_run - 
//-----------------------------------------------------------------------------
void C_BaseEntity::ShiftIntermediateDataForward( int slots_to_remove, int number_of_commands_run )
{
	Assert( number_of_commands_run >= slots_to_remove );

	// Just moving pointers, yeah
	CUtlVector< unsigned char * > saved;

	// Remember first slots
	int i = 0;
	for ( ; i < slots_to_remove; i++ )
	{
		saved.AddToTail( m_pIntermediateData[ i ] );
	}

	// Move rest of slots forward up to last slot
	for ( ; i < number_of_commands_run; i++ )
	{
		m_pIntermediateData[ i - slots_to_remove ] = m_pIntermediateData[ i ];
	}

	// Put remembered slots onto end
	for ( i = 0; i < slots_to_remove; i++ )
	{
		int slot = number_of_commands_run - slots_to_remove + i;

		m_pIntermediateData[ slot ] = saved[ i ];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : framenumber - 
//-----------------------------------------------------------------------------
void *C_BaseEntity::GetPredictedFrame( int framenumber )
{
	Assert( framenumber >= 0 );

	if ( !m_pOriginalData )
	{
		Assert( 0 );
		return NULL;
	}
	return (void *)m_pIntermediateData[ framenumber % MULTIPLAYER_BACKUP ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void *C_BaseEntity::GetOriginalNetworkDataObject( void )
{
	if ( !m_pOriginalData )
	{
		Assert( 0 );
		return NULL;
	}
	return (void *)m_pOriginalData;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ComputePackedOffsets( void )
{
	datamap_t *map = GetPredDescMap();
	if ( !map )
		return;

	if ( map->packed_offsets_computed )
		return;

	ComputePackedSize_R( map );

	Assert( map->packed_offsets_computed );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::GetIntermediateDataSize( void )
{
	ComputePackedOffsets();

	const datamap_t *map = GetPredDescMap();

	Assert( map->packed_offsets_computed );

	int size = map->packed_size;

	Assert( size > 0 );	

	// At least 4 bytes to avoid some really bad stuff
	return MAX( size, 4 );
}	

static int g_FieldSizes[FIELD_TYPECOUNT] = 
{
	0,					// FIELD_VOID
	sizeof(float),		// FIELD_FLOAT
	sizeof(int),		// FIELD_STRING
	sizeof(Vector),		// FIELD_VECTOR
	sizeof(Quaternion),	// FIELD_QUATERNION
	sizeof(int),		// FIELD_INTEGER
	sizeof(char),		// FIELD_BOOLEAN
	sizeof(short),		// FIELD_SHORT
	sizeof(char),		// FIELD_CHARACTER
	sizeof(color32),	// FIELD_COLOR32
	sizeof(color24),	// FIELD_COLOR24
	sizeof(int),		// FIELD_EMBEDDED	(handled specially)
	sizeof(int),		// FIELD_CUSTOM		(handled specially)
	
	//---------------------------------

	sizeof(int),		// FIELD_CLASSPTR
	sizeof(EHANDLE),	// FIELD_EHANDLE
	sizeof(int),		// FIELD_EDICT

	sizeof(Vector),		// FIELD_POSITION_VECTOR
	sizeof(float),		// FIELD_TIME
	sizeof(int),		// FIELD_TICK
	sizeof(int),		// FIELD_MODELNAME
	sizeof(int),		// FIELD_SOUNDNAME

	sizeof(int),		// FIELD_INPUT		(uses custom type)
#ifdef GNUC
	// pointer to members under gnuc are 8bytes if you have a virtual func
	sizeof(uint64),		// FIELD_FUNCTION
#else
	sizeof(int *),		// FIELD_FUNCTION
#endif
	sizeof(VMatrix),	// FIELD_VMATRIX
	sizeof(VMatrix),	// FIELD_VMATRIX_WORLDSPACE
	sizeof(matrix3x4_t),// FIELD_MATRIX3X4_WORLDSPACE	// NOTE: Use array(FIELD_FLOAT, 12) for matrix3x4_t NOT in worldspace
	sizeof(interval_t), // FIELD_INTERVAL
	sizeof(int),		// FIELD_MODELINDEX
	sizeof(int),		// FIELD_MATERIALINDEX
	sizeof(Vector2D),		// FIELD_VECTOR2D
	sizeof(int64),		// FIELD_INTEGER64
	sizeof(Vector4D),		// FIELD_VECTOR4D
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *map - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::ComputePackedSize_R( datamap_t *map )
{
	if ( !map )
	{
		Assert( 0 );
		return 0;
	}

	// Already computed
	if ( map->packed_offsets_computed )
	{
		return map->packed_size;
	}

	int current_position = 0;

	// Recurse to base classes first...
	if ( map->baseMap )
	{
		current_position += ComputePackedSize_R( map->baseMap );
	}

	int c = map->dataNumFields;
	int i;
	typedescription_t *field;

	for ( i = 0; i < c; i++ )
	{
		field = &map->dataDesc[ i ];

		// Always descend into embedded types...
		if ( field->fieldType != FIELD_EMBEDDED )
		{
			// Skip all private fields
			if ( field->flags & FTYPEDESC_PRIVATE )
				continue;
		}

		switch ( field->fieldType )
		{
		default:
		case FIELD_MODELINDEX:
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_TIME:
		case FIELD_TICK:
		case FIELD_CUSTOM:
		case FIELD_CLASSPTR:
		case FIELD_EDICT:
		case FIELD_POSITION_VECTOR:
		case FIELD_FUNCTION:
			Assert( 0 );
			break;

		case FIELD_EMBEDDED:
			{
				Assert( field->td != NULL );

				int embeddedsize = ComputePackedSize_R( field->td );

				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;

				current_position += embeddedsize;
			}
			break;

		case FIELD_FLOAT:
		case FIELD_VECTOR:
		case FIELD_VECTOR2D:
		case FIELD_QUATERNION:
		case FIELD_VECTOR4D:
		case FIELD_INTEGER:
		case FIELD_INTEGER64:
		case FIELD_EHANDLE:
			{
				// These should be dword aligned
				current_position = (current_position + 3) & ~3;
				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;
				Assert( field->fieldSize >= 1 );
				current_position += g_FieldSizes[ field->fieldType ] * field->fieldSize;
			}
			break;

		case FIELD_SHORT:
			{
				// This should be word aligned
				current_position = (current_position + 1) & ~1;
				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;
				Assert( field->fieldSize >= 1 );
				current_position += g_FieldSizes[ field->fieldType ] * field->fieldSize;
			}
			break;

		case FIELD_STRING:
		case FIELD_COLOR32:
		case FIELD_COLOR24:
		case FIELD_BOOLEAN:
		case FIELD_CHARACTER:
			{
				field->fieldOffset[ TD_OFFSET_PACKED ] = current_position;
				Assert( field->fieldSize >= 1 );
				current_position += g_FieldSizes[ field->fieldType ] * field->fieldSize;
			}
			break;
		case FIELD_VOID:
			{
				// Special case, just skip it
			}
			break;
		}
	}

	map->packed_size = current_position;
	map->packed_offsets_computed = true;

	return current_position;
}

// Convenient way to delay removing oneself
void C_BaseEntity::SUB_Remove( void )
{
	if (m_iHealth > 0)
	{
		// this situation can screw up NPCs who can't tell their entity pointers are invalid.
		m_iHealth = 0;
		DevWarning( 2, "SUB_Remove called on entity with health > 0\n");
	}

	UTIL_Remove( this );
}

C_BaseEntity *FindEntityInFrontOfLocalPlayer()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		// Get the entity under my crosshair
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,	MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
		{
			return tr.m_pEnt;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Debug command to wipe the decals off an entity
//-----------------------------------------------------------------------------
static void RemoveDecals_f( void )
{
	C_BaseEntity *pHit = FindEntityInFrontOfLocalPlayer();
	if ( pHit )
	{
		pHit->RemoveAllDecals();
	}
}

static ConCommand cl_removedecals( "cl_removedecals", RemoveDecals_f, "Remove the decals from the entity under the crosshair.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ClearBBoxVisualization( void )
{
	m_fBBoxVisFlags = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::ToggleBBoxVisualization( int fVisFlags )
{
	if ( m_fBBoxVisFlags & fVisFlags )
	{
		m_fBBoxVisFlags &= ~fVisFlags;
	}
	else
	{
		m_fBBoxVisFlags |= fVisFlags;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ToggleBBoxVisualization( int fVisFlags, const CCommand &args )
{
	C_BaseEntity *pHit;

	int iEntity = -1;
	if ( args.ArgC() >= 2 )
	{
		iEntity = atoi( args[ 1 ] );
	}

	if ( iEntity == -1 )
	{
		pHit = FindEntityInFrontOfLocalPlayer();
	}
	else
	{
		pHit = cl_entitylist->GetBaseEntity( iEntity );
	}

	if ( pHit )
	{
		pHit->ToggleBBoxVisualization( fVisFlags );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command to toggle visualizations of bboxes on the client
//-----------------------------------------------------------------------------
CON_COMMAND_F( cl_ent_bbox, "Displays the client's bounding box for the entity under the crosshair.", FCVAR_CHEAT )
{
	ToggleBBoxVisualization( C_BaseEntity::VISUALIZE_COLLISION_BOUNDS, args );
}


//-----------------------------------------------------------------------------
// Purpose: Command to toggle visualizations of bboxes on the client
//-----------------------------------------------------------------------------
CON_COMMAND_F( cl_ent_absbox, "Displays the client's absbox for the entity under the crosshair.", FCVAR_CHEAT )
{
	ToggleBBoxVisualization( C_BaseEntity::VISUALIZE_SURROUNDING_BOUNDS, args );
}


//-----------------------------------------------------------------------------
// Purpose: Command to toggle visualizations of bboxes on the client
//-----------------------------------------------------------------------------
CON_COMMAND_F( cl_ent_rbox, "Displays the client's render box for the entity under the crosshair.", FCVAR_CHEAT )
{
	ToggleBBoxVisualization( C_BaseEntity::VISUALIZE_RENDER_BOUNDS, args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseEntity::DrawBBoxVisualizations( void )
{
	if ( m_fBBoxVisFlags & VISUALIZE_COLLISION_BOUNDS )
	{
		debugoverlay->AddBoxOverlay( CollisionProp()->GetCollisionOrigin(), CollisionProp()->OBBMins(),
			CollisionProp()->OBBMaxs(), CollisionProp()->GetCollisionAngles(), 190, 190, 0, 0, 0.01 );
	}

	if ( m_fBBoxVisFlags & VISUALIZE_SURROUNDING_BOUNDS )
	{
		Vector vecSurroundMins, vecSurroundMaxs;
		CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
		debugoverlay->AddBoxOverlay( vec3_origin, vecSurroundMins,
			vecSurroundMaxs, vec3_angle, 0, 255, 255, 0, 0.01 );
	}

	if ( m_fBBoxVisFlags & VISUALIZE_RENDER_BOUNDS || r_drawrenderboxes.GetInt() )
	{
		Vector vecRenderMins, vecRenderMaxs;
		GetRenderBounds( vecRenderMins, vecRenderMaxs );
		debugoverlay->AddBoxOverlay( GetRenderOrigin(), vecRenderMins, vecRenderMaxs,
			GetRenderAngles(), 255, 0, 255, 0, 0.01 );
	}
}

RenderMode_t C_BaseEntity::GetRenderMode() const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetRenderMode();
}

RenderFx_t C_BaseEntity::GetRenderFX() const
{
	Assert( AlphaProp() );
	return AlphaProp()->GetRenderFX();
}

//-----------------------------------------------------------------------------
// Sets the render mode
//-----------------------------------------------------------------------------
void C_BaseEntity::SetRenderMode( RenderMode_t nRenderMode )
{
	Assert( AlphaProp() );
	if ( nRenderMode != GetRenderMode() )
	{
		AlphaProp()->SetRenderFX( GetRenderFX(), nRenderMode, FLT_MAX, 0.0f );
	}
}

void C_BaseEntity::SetRenderFX( RenderFx_t nRenderFX, float flStartTime, float flDuration )
{
	Assert( AlphaProp() );
	bool bStartTimeUnspecified = ( flStartTime == FLT_MAX );
	if ( nRenderFX != GetRenderFX() || !bStartTimeUnspecified )
	{
		if ( bStartTimeUnspecified )
		{
			flStartTime = gpGlobals->curtime;
		}
		AlphaProp()->SetRenderFX( nRenderFX, GetRenderMode(), flStartTime, flDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy from this entity into one of the save slots (original or intermediate)
// Input  : slot - 
//			type - 
//			false - 
//			false - 
//			true - 
//			false - 
//			NULL - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::SaveData( const char *context, int slot, int type )
{
	VPROF( "C_BaseEntity::SaveData" );

	void *dest = ( slot == SLOT_ORIGINALDATA ) ? GetOriginalNetworkDataObject() : GetPredictedFrame( slot );
	Assert( dest );

	char sz[ 64 ];
	sz[0] = 0;
	// don't build debug strings per entity per frame, unless we are watching the entity
	if ( pwatchent.GetInt() == entindex() )
	{
		if ( slot == SLOT_ORIGINALDATA )
		{
			Q_snprintf( sz, sizeof( sz ), "%s SaveData(original)", context );
		}
		else
		{
			Q_snprintf( sz, sizeof( sz ), "%s SaveData(slot %02i)", context, slot );
		}
	}

	if ( slot != SLOT_ORIGINALDATA )
	{
		// Remember high water mark so that we can detect below if we are reading from a slot not yet predicted into...
		m_nIntermediateDataCount = slot;
	}

	CPredictionCopy copyHelper( type, dest, PC_DATA_PACKED, this, PC_DATA_NORMAL );
	int error_count = copyHelper.TransferData( sz, entindex(), GetPredDescMap() );
	return error_count;
}

//-----------------------------------------------------------------------------
// Purpose: Restore data from specified slot into current entity
// Input  : slot - 
//			type - 
//			false - 
//			false - 
//			true - 
//			false - 
//			NULL - 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseEntity::RestoreData( const char *context, int slot, int type )
{
	VPROF( "C_BaseEntity::RestoreData" );

	const void *src = ( slot == SLOT_ORIGINALDATA ) ? GetOriginalNetworkDataObject() : GetPredictedFrame( slot );
	Assert( src );

	// This assert will fire if the server ack'd a CUserCmd which we hadn't predicted yet...
	// In that case, we'd be comparing "old" data from this "unused" slot with the networked data and reporting all kinds of prediction errors possibly.
	Assert( slot == SLOT_ORIGINALDATA || slot <= m_nIntermediateDataCount );

	char sz[ 64 ];
	sz[0] = 0;
	// don't build debug strings per entity per frame, unless we are watching the entity
	if ( pwatchent.GetInt() == entindex() )
	{
		if ( slot == SLOT_ORIGINALDATA )
		{
			Q_snprintf( sz, sizeof( sz ), "%s RestoreData(original)", context );
		}
		else
		{
			Q_snprintf( sz, sizeof( sz ), "%s RestoreData(slot %02i)", context, slot );
		}
	}

	// some flags shouldn't be predicted - as we find them, add them to the savedEFlagsMask
	const uint64 savedEFlagsMask = EFL_DIRTY_SHADOWUPDATE | EFL_DIRTY_SPATIAL_PARTITION;
	uint64 savedEFlags = GetEFlags() & savedEFlagsMask;

	// model index needs to be set manually for dynamic model refcounting purposes
	int oldModelIndex = m_nModelIndex;

	CPredictionCopy copyHelper( type, this, PC_DATA_NORMAL, src, PC_DATA_PACKED );
	int error_count = copyHelper.TransferData( sz, entindex(), GetPredDescMap() );

	// set non-predicting flags back to their prior state
	RemoveEFlags( savedEFlagsMask );
	AddEFlags( savedEFlags );

	// restore original model index and change via SetModelIndex
	int newModelIndex = m_nModelIndex;
	m_nModelIndex = oldModelIndex;
	int overrideModelIndex = CalcOverrideModelIndex();
	if( overrideModelIndex != -1 )
		newModelIndex = overrideModelIndex;
	if ( oldModelIndex != newModelIndex )
	{
		MDLCACHE_CRITICAL_SECTION(); // ???
		SetModelIndex( newModelIndex );
	}

	OnPostRestoreData();

	return error_count;
}


void C_BaseEntity::OnPostRestoreData()
{
	// HACK Force recomputation of origin
	InvalidatePhysicsRecursive( POSITION_CHANGED | ANGLES_CHANGED | VELOCITY_CHANGED );

	if ( GetMoveParent() )
	{
		AddToAimEntsList();
	}

	// If our model index has changed, then make sure it's reflected in our model pointer.
	// (Mostly superseded by new modelindex delta check in RestoreData, but I'm leaving it
	// because it might be band-aiding any other missed calls to SetModelByIndex --henryg)
	if ( GetModel() != modelinfo->GetModel( GetModelIndex() ) )
	{
		MDLCACHE_CRITICAL_SECTION();
		SetModelByIndex( GetModelIndex() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine approximate velocity based on updates from server
// Input  : vel - 
//-----------------------------------------------------------------------------
void C_BaseEntity::EstimateAbsVelocity( Vector& vel )
{
	if ( C_BasePlayer::IsLocalPlayer( this ) )
	{
		// This is interpolated and networked
		vel = GetAbsVelocity();
		return;
	}

	CInterpolationContext context;
	context.EnableExtrapolation( true );
	m_iv_vecOrigin.GetDerivative_SmoothVelocity( &vel, gpGlobals->curtime );
}

void C_BaseEntity::Interp_Reset( VarMapping_t *map )
{
	PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( this, "reset" );
	int c = map->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		VarMapEntry_t *e = &map->m_Entries[ i ];
		IInterpolatedVar *watcher = e->watcher;

		watcher->Reset( gpGlobals->curtime );
	}
}

void C_BaseEntity::ResetLatched()
{
	if ( IsClientCreated() )
		return;

	Interp_Reset( GetVarMapping() );
}

//-----------------------------------------------------------------------------
// Purpose: Fixme, this needs a better solution
// Input  : flags - 
// Output : float
//-----------------------------------------------------------------------------

static float AdjustInterpolationAmount( C_BaseEntity *pEntity, float baseInterpolation )
{
	if ( cl_interp_npcs.GetFloat() > 0 )
	{
		const float minNPCInterpolationTime = cl_interp_npcs.GetFloat();
		const float minNPCInterpolation = TICK_INTERVAL * ( TIME_TO_TICKS( minNPCInterpolationTime ) + 1 );

		if ( minNPCInterpolation > baseInterpolation )
		{
			while ( pEntity )
			{
				if ( pEntity->IsNPC() )
					return minNPCInterpolation;

				pEntity = pEntity->GetMoveParent();
			}
		}
	}

	return baseInterpolation;
}

//-------------------------------------
float C_BaseEntity::GetInterpolationAmount( int flags )
{
	// If single player server is "skipping ticks" everything needs to interpolate for a bit longer
	int serverTickMultiple = 1;
	if ( IsSimulatingOnAlternateTicks() )
	{
		serverTickMultiple = 2;
	}

	if ( GetPredictable() || IsClientCreated() )
	{
		return TICK_INTERVAL * serverTickMultiple;
	}

	// Always fully interpolate during multi-player or during demo playback, if the recorded
	// demo was recorded locally.
	const bool bPlayingDemo = engine->IsPlayingDemo();
	const bool bPlayingMultiplayer = !bPlayingDemo && ( gpGlobals->maxClients > 1 );
	const bool bPlayingNonLocallyRecordedDemo = bPlayingDemo && !engine->IsPlayingDemoALocallyRecordedDemo();
	if ( bPlayingMultiplayer || bPlayingNonLocallyRecordedDemo )
	{
		return AdjustInterpolationAmount( this, TICKS_TO_TIME( TIME_TO_TICKS( GetClientInterpAmount() ) + serverTickMultiple ) );
	}

	int expandedServerTickMultiple = serverTickMultiple;
	if ( IsEngineThreaded() )
	{
		expandedServerTickMultiple += cl_interp_threadmodeticks.GetInt();
	}

	if ( IsAnimatedEveryTick() && IsSimulatedEveryTick() )
	{
		return TICK_INTERVAL * expandedServerTickMultiple;
	}

	if ( ( flags & LATCH_ANIMATION_VAR ) && IsAnimatedEveryTick() )
	{
		return TICK_INTERVAL * expandedServerTickMultiple;
	}
	if ( ( flags & LATCH_SIMULATION_VAR ) && IsSimulatedEveryTick() )
	{
		return TICK_INTERVAL * expandedServerTickMultiple;
	}

	return AdjustInterpolationAmount( this, TICKS_TO_TIME( TIME_TO_TICKS( GetClientInterpAmount() ) + serverTickMultiple ) );
}


float C_BaseEntity::GetLastChangeTime( int flags )
{
	if ( GetPredictable() || IsClientCreated() )
	{
		return gpGlobals->curtime;
	}
	
	// make sure not both flags are set, we can't resolve that
	Assert( !( (flags & LATCH_ANIMATION_VAR) && (flags & LATCH_SIMULATION_VAR) ) );
	
	if ( flags & LATCH_ANIMATION_VAR )
	{
		return GetAnimTime();
	}

	if ( flags & LATCH_SIMULATION_VAR )
	{
		float st = GetSimulationTime();
		if ( st == 0.0f )
		{
			return gpGlobals->curtime;
		}
		return st;
	}

	Assert( 0 );

	return gpGlobals->curtime;
}

const Vector& C_BaseEntity::GetPrevLocalOrigin() const
{
	return m_iv_vecOrigin.GetPrev();
}

const QAngle& C_BaseEntity::GetPrevLocalAngles() const
{
	return m_iv_angRotation.GetPrev();
}

//-----------------------------------------------------------------------------
// Simply here for game shared 
//-----------------------------------------------------------------------------
bool C_BaseEntity::IsFloating()
{
	// NOTE: This is only here because it's called by game shared.
	// The server uses it to lower falling impact damage
	return false;
}


BEGIN_MAPENTITY_NO_BASE( C_BaseEntity )
END_MAPENTITY()

void C_BaseEntity::SetClassname( const char *className )
{
	m_iClassname = AllocPooledString( className );
}

//-----------------------------------------------------------------------------
// capabilities
//-----------------------------------------------------------------------------
int C_BaseEntity::ObjectCaps( void ) 
{
	return 0; 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : C_AI_BaseNPC
//-----------------------------------------------------------------------------
C_AI_BaseNPC *C_BaseEntity::MyNPCPointer( void )
{
	if ( IsNPC() ) 
	{
		return assert_cast<C_AI_BaseNPC *>(this);
	}

	return NULL;
}

extern ConVar *closecaption;

//-----------------------------------------------------------------------------
// Purpose: For each client (only can be local client in client .dll ) checks the client has disabled CC and if so, removes them from 
//  the recipient list.
// Input  : filter - 
//-----------------------------------------------------------------------------
void C_BaseEntity::RemoveRecipientsIfNotCloseCaptioning( C_RecipientFilter& filter )
{
	if ( !closecaption->GetBool() )
	{
		filter.Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : recording - 
// Output : inline void
//-----------------------------------------------------------------------------
void C_BaseEntity::EnableInToolView( bool bEnable )
{
	m_bEnabledInToolView = bEnable;
	UpdateVisibility();
}

void C_BaseEntity::SetToolRecording( bool recording )
{
	m_bToolRecording = recording;
	if ( m_bToolRecording )
	{
		recordinglist->AddToList( GetClientHandle() );
	}
	else
	{
        recordinglist->RemoveFromList( GetClientHandle() );
	}
}

bool C_BaseEntity::HasRecordedThisFrame() const
{
	Assert( m_nLastRecordedFrame <= gpGlobals->framecount );
	return m_nLastRecordedFrame == gpGlobals->framecount;
}

void C_BaseEntity::GetToolRecordingState( KeyValues *msg )
{
	Assert( ToolsEnabled() );
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "C_BaseEntity::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	C_BaseEntity *pOwner = m_hOwnerEntity;

	static BaseEntityRecordingState_t state;
	state.m_flTime = gpGlobals->curtime;
	state.m_pModelName = modelinfo->GetModelName( GetModel() );
	state.m_nOwner = pOwner ? pOwner->entindex() : -1;
	state.m_nEffects = m_fEffects;
	state.m_bVisible = ShouldDraw() && !IsDormant();
	state.m_bRecordFinalVisibleSample = false;
	state.m_vecRenderOrigin = GetRenderOrigin();
	state.m_vecRenderAngles = GetRenderAngles();

	// use EF_NOINTERP if the owner or a hierarchical parent has NO_INTERP
	if ( pOwner && pOwner->IsNoInterpolationFrame() )
	{
		state.m_nEffects |= EF_DEPRECATED_NOINTERP;
	}
	C_BaseEntity *pParent = GetMoveParent();
	while ( pParent )
	{
		if ( pParent->IsNoInterpolationFrame() )
		{
			state.m_nEffects |= EF_DEPRECATED_NOINTERP;
			break;
		}

		pParent = pParent->GetMoveParent();
	}

	msg->SetPtr( "baseentity", &state );
}

void C_BaseEntity::CleanupToolRecordingState( KeyValues *msg )
{
}

void C_BaseEntity::RecordToolMessage()
{
	Assert( IsToolRecording() );
	if ( !IsToolRecording() )
		return;

	if ( HasRecordedThisFrame() )
		return;

	KeyValues *msg = new KeyValues( "entity_state" );

	// Post a message back to all IToolSystems
	GetToolRecordingState( msg );
	Assert( (int)GetToolHandle() != 0 );
	ToolFramework_PostToolMessage( GetToolHandle(), msg );
	CleanupToolRecordingState( msg );

	msg->deleteThis();

	m_nLastRecordedFrame = gpGlobals->framecount;
}

// (static function)
void C_BaseEntity::ToolRecordEntities()
{
	VPROF_BUDGET( "C_BaseEntity::ToolRecordEnties", VPROF_BUDGETGROUP_TOOLS );

	if ( !ToolsEnabled() || !clienttools->IsInRecordingMode() )
		return;

	// Let non-dormant client created predictables get added, too
	int c = recordinglist->Count();
	for ( int i = 0 ; i < c ; i++ )
	{
		IClientRenderable *pRenderable = recordinglist->Get( i );
		if ( !pRenderable )
			continue;

		pRenderable->RecordToolMessage();
	}
}

#ifdef _DEBUG
static entity_list_ids_t s_nSuppressChanges = NUM_ENTITY_LISTS;
#endif

void C_BaseEntity::AddToEntityList( entity_list_ids_t listId )
{
	if(listId == ENTITY_LIST_DELETE && in_purge_entities)
		return;
	Assert(listId < NUM_ENTITY_LISTS);
	if ( m_ListEntry[listId] == 0xFFFF )
	{
		m_ListEntry[listId] = g_EntityLists[listId].AddToTail( this );
	}
}

void C_BaseEntity::RemoveFromEntityList( entity_list_ids_t listId )
{
	if(listId == ENTITY_LIST_DELETE && in_purge_entities)
		return;
	Assert( s_nSuppressChanges != listId );
	Assert( listId < NUM_ENTITY_LISTS );
	if ( m_ListEntry[listId] != 0xFFFF )
	{
		g_EntityLists[listId].Remove( m_ListEntry[listId] );
		m_ListEntry[listId] = 0xFFFF;
	}
}

void C_BaseEntity::AddToInterpolationList()
{
	AddToEntityList( ENTITY_LIST_INTERPOLATE );
}


void C_BaseEntity::RemoveFromInterpolationList()
{
	RemoveFromEntityList( ENTITY_LIST_INTERPOLATE );
}

				
void C_BaseEntity::AddToTeleportList()
{
	AddToEntityList( ENTITY_LIST_TELEPORT );
}


void C_BaseEntity::RemoveFromTeleportList()
{
	RemoveFromEntityList( ENTITY_LIST_TELEPORT );
}

#ifdef TF_CLIENT_DLL
bool C_BaseEntity::ValidateEntityAttachedToPlayer( bool &bShouldRetry )
{
	bShouldRetry = false;
	C_BaseEntity *pParent = GetRootMoveParent();
	if ( pParent == this )
		return true;

	// Some wearables parent to the view model
	C_BasePlayer *pPlayer = ToBasePlayer( pParent );
	if ( pPlayer && pPlayer->GetViewModel() == this )
	{
		return true;
	}

	// always allow the briefcase model
	const char *pszModel = modelinfo->GetModelName( GetModel() );
	if ( pszModel && pszModel[0] )
	{
		if ( FStrEq( pszModel, "models/flag/briefcase.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/passtime/ball/passtime_ball.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/props_doomsday/australium_container.mdl" ) )
			return true;

		// Temp for MVM testing
		if ( FStrEq( pszModel, "models/buildables/sapper_placement_sentry1.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/props_td/atom_bomb.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/props_lakeside_event/bomb_temp_hat.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/props_moonbase/powersupply_flag.mdl" ) )
			return true;

		// The Halloween 2014 doomsday flag replacement
		if ( FStrEq( pszModel, "models/flag/ticket_case.mdl" ) )
			return true;

		if ( FStrEq( pszModel, "models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl" ) )
			return true;
	}

	// Any entity that's not an item parented to a player is invalid.
	// This prevents them creating some other entity to pretend to be a cosmetic item.
	return !pParent->IsPlayer();
}
#endif // TF_CLIENT_DLL


void C_BaseEntity::AddVar( void *data, IInterpolatedVar *watcher, int type, bool bSetup )
{
	// Only add it if it hasn't been added yet.
	bool bAddIt = true;
	for ( int i=0; i < m_VarMap.m_Entries.Count(); i++ )
	{
		if ( m_VarMap.m_Entries[i].watcher == watcher )
		{
			if ( (type & EXCLUDE_AUTO_INTERPOLATE) != (watcher->GetType() & EXCLUDE_AUTO_INTERPOLATE) )
			{
				// Its interpolation mode changed, so get rid of it and re-add it.
				RemoveVar( m_VarMap.m_Entries[i].data, true );
			}
			else
			{
				// They're adding something that's already there. No need to re-add it.
				bAddIt = false;
			}
			
			break;	
		}
	}
	
	if ( bAddIt )
	{
		// watchers must have a debug name set
		Assert( watcher->GetDebugName() != NULL );

		VarMapEntry_t map;
		map.data = data;
		map.watcher = watcher;
		map.type = type;
		map.m_bNeedsToInterpolate = true;
		if ( type & EXCLUDE_AUTO_INTERPOLATE )
		{
			m_VarMap.m_Entries.AddToTail( map );
		}
		else
		{
			m_VarMap.m_Entries.AddToHead( map );
			++m_VarMap.m_nInterpolatedEntries;
		}
	}

	if ( bSetup )
	{
		watcher->Setup( data, type );
		watcher->SetInterpolationAmount( GetInterpolationAmount( watcher->GetType() ) );
	}
}


void C_BaseEntity::RemoveVar( void *data, bool bAssert )
{
	for ( int i=0; i < m_VarMap.m_Entries.Count(); i++ )
	{
		if ( m_VarMap.m_Entries[i].data == data )
		{
			if ( !( m_VarMap.m_Entries[i].type & EXCLUDE_AUTO_INTERPOLATE ) )
				--m_VarMap.m_nInterpolatedEntries;

			m_VarMap.m_Entries.Remove( i );
			return;
		}
	}
	if ( bAssert )
	{
		Assert( !"RemoveVar" );
	}
}

void C_BaseEntity::CheckCLInterpChanged()
{
	float flCurValue_Interp = GetClientInterpAmount();
	static float flLastValue_Interp = flCurValue_Interp;

	float flCurValue_InterpNPCs = cl_interp_npcs.GetFloat();
	static float flLastValue_InterpNPCs = flCurValue_InterpNPCs;
	
	if ( flLastValue_Interp != flCurValue_Interp || 
		 flLastValue_InterpNPCs != flCurValue_InterpNPCs  )
	{
		flLastValue_Interp = flCurValue_Interp;
		flLastValue_InterpNPCs = flCurValue_InterpNPCs;
	
		// Tell all the existing entities to update their interpolation amounts to account for the change.
		C_BaseEntityIterator iterator;
		C_BaseEntity *pEnt;
		while ( (pEnt = iterator.Next()) != NULL )
		{
			pEnt->Interp_UpdateInterpolationAmounts( pEnt->GetVarMapping() );
		}
	}
}

void C_BaseEntity::DontRecordInTools()
{
	m_bRecordInTools = false;
}

int C_BaseEntity::GetCreationTick() const
{
	return m_nCreationTick;
}

// static method
void C_BaseEntity::SimulateEntities()
{
	VPROF_("C_BaseEntity::Simulate", 1, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);

	s_bImmediateRemovesAllowed = false;

	if ( !report_cliententitysim.GetBool() )
	{
		int iNext = 0;
		for ( int iCur = g_EntityLists[ENTITY_LIST_SIMULATE].Head(); iCur != g_EntityLists[ENTITY_LIST_SIMULATE].InvalidIndex(); iCur = iNext )
		{
			if (!g_EntityLists[ENTITY_LIST_SIMULATE].IsValidIndex(iCur))
			{
				iNext++;
				continue;
			}

			iNext = g_EntityLists[ENTITY_LIST_SIMULATE].Next( iCur );
			C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_SIMULATE].Element(iCur);
			if ( pCur->IsEFlagSet( EFL_KILLME ) )
				continue;

#ifdef _DEBUG
			s_nSuppressChanges = ENTITY_LIST_SIMULATE;
#endif
			bool bRemove = !pCur->Simulate();
#ifdef _DEBUG
			s_nSuppressChanges = NUM_ENTITY_LISTS;
#endif
			if ( bRemove )
			{
				pCur->RemoveFromEntityList(ENTITY_LIST_SIMULATE);
			}
		}
	}
	else
	{
		CFastTimer fastTimer;
	
		int iNext;
		for ( int iCur = g_EntityLists[ENTITY_LIST_SIMULATE].Head(); iCur != g_EntityLists[ENTITY_LIST_SIMULATE].InvalidIndex(); iCur = iNext )
		{
			iNext = g_EntityLists[ENTITY_LIST_SIMULATE].Next( iCur );

			if (!g_EntityLists[ENTITY_LIST_SIMULATE].IsValidIndex(iCur))
				continue;

			C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_SIMULATE].Element(iCur);
			if ( pCur->IsEFlagSet( EFL_KILLME ) )
				continue;

			fastTimer.Start();
#ifdef _DEBUG
			s_nSuppressChanges = ENTITY_LIST_SIMULATE;
#endif
			bool bRemove = !pCur->Simulate();
#ifdef _DEBUG
			s_nSuppressChanges = NUM_ENTITY_LISTS;
#endif
			if ( bRemove )
			{
				pCur->RemoveFromEntityList(ENTITY_LIST_SIMULATE);
			}
			fastTimer.End();
			Msg( "Entity(%d): %s - %f\n", pCur->entindex(), pCur->GetDebugName(), fastTimer.GetDuration().GetMillisecondsF() );
		}

		// Report only once per turn on.
		report_cliententitysim.SetValue( 0 );
	}

	s_bImmediateRemovesAllowed = true;
	PurgeRemovedEntities();
}

// static method
void C_BaseEntity::PurgeRemovedEntities()
{
	in_purge_entities = true;
	int iNext;
	for ( int iCur = g_EntityLists[ENTITY_LIST_DELETE].Head(); iCur != g_EntityLists[ENTITY_LIST_DELETE].InvalidIndex(); iCur = iNext )
	{
		iNext = g_EntityLists[ENTITY_LIST_DELETE].Next( iCur );
		C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_DELETE].Element(iCur);
		UTIL_RemoveImmediate( pCur );
	}
	g_EntityLists[ENTITY_LIST_DELETE].RemoveAll();
	in_purge_entities = false;
}

// static method
// This is the per-viewport setup hook
void C_BaseEntity::PreRenderEntities()
{
	MDLCACHE_CRITICAL_SECTION();
	int iNext;
	for ( int iCur = g_EntityLists[ENTITY_LIST_PRERENDER].Head(); iCur != g_EntityLists[ENTITY_LIST_PRERENDER].InvalidIndex(); iCur = iNext )
	{
		iNext = g_EntityLists[ENTITY_LIST_PRERENDER].Next( iCur );
		C_BaseEntity *pCur = g_EntityLists[ENTITY_LIST_PRERENDER].Element(iCur);

#ifdef _DEBUG
		s_nSuppressChanges = ENTITY_LIST_PRERENDER;
#endif
		bool bRemove = !pCur->PreRender();
#ifdef _DEBUG
		s_nSuppressChanges = NUM_ENTITY_LISTS;
#endif	
		if ( bRemove )
		{
			pCur->RemoveFromEntityList(ENTITY_LIST_PRERENDER);
		}
	}

}

bool C_BaseEntity::PreRender()
{
	bool bNeedsPrerender = false;

	// Create flashlight effects, etc.
	if ( CreateLightEffects() )
	{
		bNeedsPrerender = true;
	}
	return bNeedsPrerender;
}

bool C_BaseEntity::IsViewEntity() const
{
	return render->GetViewEntity() == entindex();
}

bool C_BaseEntity::IsAbleToHaveFireEffect( void ) const
{
	return true;
}


void C_BaseEntity::SetBlurState( bool bShouldBlur )
{
	if( bShouldBlur != m_bIsBlurred )
	{
		m_bIsBlurred = bShouldBlur;
		OnTranslucencyTypeChanged();
	}
}

bool C_BaseEntity::IsBlurred( void )
{
	return m_bIsBlurred;
}

void C_BaseEntity::OnParseMapDataFinished()
{
}

//-----------------------------------------------------------------------------
// Adjust the number of cell bits
//-----------------------------------------------------------------------------
bool C_BaseEntity::SetCellBits( int cellbits )
{
	if ( m_cellbits == cellbits )
		return false;

	m_cellbits = cellbits;
	m_cellwidth = ( 1 << cellbits );
	return true;
}


bool C_BaseEntity::ShouldRegenerateOriginFromCellBits() const
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *C_BaseEntity::GetResponseSystem()
{
	extern IResponseSystem *g_pResponseSystem;
	return g_pResponseSystem;
}

//------------------------------------------------------------------------------
void CC_CL_Find_Ent( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Format: cl_find_ent <substring>\n" );
		return;
	}

	int iCount = 0;
	const char *pszSubString = args[1];
	Msg("Searching for client entities with classname containing substring: '%s'\n", pszSubString );

	C_BaseEntity *ent = NULL;
	while ( (ent = ClientEntityList().NextBaseEntity(ent)) != NULL )
	{
		const char *pszClassname = ent->GetClassname();

		bool bMatches = false;
		if ( pszClassname && pszClassname[0] )
		{
			if ( Q_stristr( pszClassname, pszSubString ) )
			{
				bMatches = true;
			}
		}

		if ( bMatches )
		{
			iCount++;
			Msg("   '%s' (entindex %d) %s \n", pszClassname ? pszClassname : "[NO NAME]", ent->entindex(), ent->IsDormant() ? "(DORMANT)" : "" );
		}
	}

	Msg("Found %d matches.\n", iCount);
}
static ConCommand cl_find_ent("cl_find_ent", CC_CL_Find_Ent, "Find and list all client entities with classnames that contain the specified substring.\nFormat: cl_find_ent <substring>\n", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_CL_Find_Ent_Index( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Format: cl_find_ent_index <index>\n" );
		return;
	}

	int iIndex = atoi(args[1]);
	C_BaseEntity *ent = ClientEntityList().GetBaseEntity( iIndex );
	if ( ent )
	{
		const char *pszClassname = ent->GetClassname();
		Msg("   '%s' (entindex %d) %s \n", pszClassname ? pszClassname : "[NO NAME]", iIndex, ent->IsDormant() ? "(DORMANT)" : "" );
	}
	else
	{
		Msg("Found no entity at %d.\n", iIndex);
	}
}
static ConCommand cl_find_ent_index("cl_find_ent_index", CC_CL_Find_Ent_Index, "Display data for clientside entity matching specified index.\nFormat: cl_find_ent_index <index>\n", FCVAR_CHEAT);
