//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "ServerNetworkProperty.h"
#include "tier0/dbg.h"
#include "gameinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CTimedEventMgr g_NetworkPropertyEventMgr;

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CServerNetworkProperty::CServerNetworkProperty()
{
	Init( NULL );
	m_bDestroyed = false;
}


CServerNetworkProperty::~CServerNetworkProperty()
{
	/* Free our transmit proxy.
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->Release();
	}*/

	engine->CleanUpEntityClusterList( &m_PVSInfo );

	// remove the attached edict if it exists
	ReleaseEdict();
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CServerNetworkProperty::Init( CBaseEntity *pEntity )
{
	m_pPev = NULL;
	m_pOuter = pEntity;
	m_pServerClass = NULL;
//	m_pTransmitProxy = NULL;
	m_bPendingStateChange = false;
	m_PVSInfo.m_nClusterCount = 0;
	m_TimerEvent.Init( &g_NetworkPropertyEventMgr, this );
}


//-----------------------------------------------------------------------------
// Connects, disconnects edicts
//-----------------------------------------------------------------------------
void CServerNetworkProperty::AttachEdict( edict_t *pRequiredEdict )
{
	Assert ( !m_pPev );

	Assert(pRequiredEdict);

	m_pPev = pRequiredEdict;
	m_pPev->SetEdict( GetBaseEntity(), true );
}

void CServerNetworkProperty::ReleaseEdict()
{
	if ( m_pPev )
	{
		m_pPev->SetEdict( NULL, false );
		engine->RemoveEdict( m_pPev );
		m_pPev = NULL;
	}
}

edict_t *CServerNetworkProperty::DetachEdict()
{
	if ( m_pPev )
	{
		m_pPev->SetEdict( NULL, false );
		edict_t *pOld = m_pPev;
		m_pPev = NULL;
		return pOld;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Entity handles
//-----------------------------------------------------------------------------
IHandleEntity *CServerNetworkProperty::GetEntityHandle( )
{
	return m_pOuter;
}

void CServerNetworkProperty::Release()
{
	UTIL_Remove( m_pOuter );
}


//-----------------------------------------------------------------------------
// PVS information
//-----------------------------------------------------------------------------
void CServerNetworkProperty::RecomputePVSInformation()
{
	if ( m_pPev && ( ( m_pPev->m_fStateFlags & FL_EDICT_DIRTY_PVS_INFORMATION ) != 0 ) )
	{
		m_pPev->m_fStateFlags &= ~FL_EDICT_DIRTY_PVS_INFORMATION;
		engine->BuildEntityClusterList( edict(), &m_PVSInfo );
	}
}


//-----------------------------------------------------------------------------
// Serverclass
//-----------------------------------------------------------------------------
ServerClass* CServerNetworkProperty::GetServerClass()
{
	if ( !m_pServerClass )
		m_pServerClass = m_pOuter->GetServerClass();
	return m_pServerClass;
}

const char* CServerNetworkProperty::GetClassName() const
{
	return m_pOuter->GetClassname();
}


//-----------------------------------------------------------------------------
// Transmit proxies
/*-----------------------------------------------------------------------------
void CServerNetworkProperty::SetTransmitProxy( CBaseTransmitProxy *pProxy )
{
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->Release();
	}

	m_pTransmitProxy = pProxy;
	
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->AddRef();
	}
}*/

//-----------------------------------------------------------------------------
// PVS rules
//-----------------------------------------------------------------------------
bool CServerNetworkProperty::IsInPVS( const edict_t *pRecipient, const void *pvs, int pvssize )
{
	RecomputePVSInformation();

	// ignore if not touching a PV leaf
	// negative leaf count is a node number
	// If no pvs, add any entity

	Assert( pvs && ( edict() != pRecipient ) );

	unsigned char *pPVS = ( unsigned char * )pvs;
	
	if ( m_PVSInfo.m_nClusterCount < 0 )   // too many clusters, use headnode
	{
		return ( engine->CheckHeadnodeVisible( m_PVSInfo.m_nHeadNode, pPVS, pvssize ) != 0);
	}
	
	for ( int i = m_PVSInfo.m_nClusterCount; --i >= 0; )
	{
		if (pPVS[m_PVSInfo.m_pClusters[i] >> 3] & (1 << (m_PVSInfo.m_pClusters[i] & 7) ))
			return true;
	}

	return false;		// not visible
}

//TODO!!! test this?
/*
// Cull transmission based on the camera limits
		// These limits are send in cl_strategic_cam_limits (basically just the fov angles)
		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
		Assert( pRecipientEntity && pRecipientEntity->IsPlayer() );
		if ( !pRecipientEntity )
			return false;
		CHL2WarsPlayer *pRecipientPlayer = static_cast<CHL2WarsPlayer*>( pRecipientEntity );

		// Get player camera position and limits
		Vector vPlayerPos = pRecipientPlayer->Weapon_ShootPosition() + pRecipientPlayer->GetCameraOffset();
		const Vector &vCamLimits = pRecipientPlayer->GetCamLimits();

		// Get player angles
		matrix3x4_t matAngles;
		AngleMatrix( pRecipientPlayer->GetAbsAngles(), matAngles );

		// Now check if the entity is within the camera limits
		const Vector &center = GetOuter()->GetAbsOrigin();
		if( GetOuter()->IsPointSized() )
		{
			if( TestPointInCamera( center, vCamLimits, matAngles, vPlayerPos ) )
				return true;
		}
		else
		{
			// TODO: Do a better (and fast) check
			const Vector &vOffset1 = GetOuter()->CollisionProp()->OBBMins();
			Vector vOffset2 = GetOuter()->CollisionProp()->OBBMaxs();
			vOffset2.z = vOffset1.z;

			if( TestPointInCamera( center + vOffset1, vCamLimits, matAngles, vPlayerPos ) ||
				TestPointInCamera( center + vOffset2, vCamLimits, matAngles, vPlayerPos ) )
				return true;
		}

		return false;
*/

//-----------------------------------------------------------------------------
// PVS: this function is called a lot, so it avoids function calls
//-----------------------------------------------------------------------------
bool CServerNetworkProperty::IsInPVS( const CCheckTransmitInfo *pInfo )
{
	// PVS data must be up to date
	Assert( !m_pPev || ( ( m_pPev->m_fStateFlags & FL_EDICT_DIRTY_PVS_INFORMATION ) == 0 ) );
	
	int i;

	// Early out if the areas are connected
	if ( !m_PVSInfo.m_nAreaNum2 )
	{
		for ( i=0; i< pInfo->m_AreasNetworked; i++ )
		{
			int clientArea = pInfo->m_Areas[i];
			if ( clientArea == m_PVSInfo.m_nAreaNum || engine->CheckAreasConnected( clientArea, m_PVSInfo.m_nAreaNum ) )
				break;
		}
	}
	else
	{
		// doors can legally straddle two areas, so
		// we may need to check another one
		for ( i=0; i< pInfo->m_AreasNetworked; i++ )
		{
			int clientArea = pInfo->m_Areas[i];
			if ( clientArea == m_PVSInfo.m_nAreaNum || clientArea == m_PVSInfo.m_nAreaNum2 )
				break;

			if ( engine->CheckAreasConnected( clientArea, m_PVSInfo.m_nAreaNum ) )
				break;

			if ( engine->CheckAreasConnected( clientArea, m_PVSInfo.m_nAreaNum2 ) )
				break;
		}
	}

	if ( i == pInfo->m_AreasNetworked )
	{
		// areas not connected
		return false;
	}

	// ignore if not touching a PV leaf
	// negative leaf count is a node number
	// If no pvs, add any entity

	Assert( edict() != pInfo->m_pClientEnt );

	unsigned char *pPVS = ( unsigned char * )pInfo->m_PVS;
	
	if ( m_PVSInfo.m_nClusterCount < 0 )   // too many clusters, use headnode
	{
		return (engine->CheckHeadnodeVisible( m_PVSInfo.m_nHeadNode, pPVS, pInfo->m_nPVSSize ) != 0);
	}
	
	for ( i = m_PVSInfo.m_nClusterCount; --i >= 0; )
	{
		int nCluster = m_PVSInfo.m_pClusters[i];
		if ( ((int)(pPVS[nCluster >> 3])) & BitVec_BitInByte( nCluster ) )
			return true;
	}

	return false;		// not visible

}


void CServerNetworkProperty::SetUpdateInterval( float val )
{
	if ( val == 0 )
	{
		m_TimerEvent.StopUpdates();
		FireEvent(); // Fire event in case changed!
	}
	else
		m_TimerEvent.SetUpdateInterval( val );
}


void CServerNetworkProperty::FireEvent()
{
	// Our timer went off. If our state has changed in the background, then 
	// trigger a state change in the edict.
	if ( m_bPendingStateChange && m_pPev )
	{
		m_pPev->StateChanged();
		m_bPendingStateChange = false;
	}
}



