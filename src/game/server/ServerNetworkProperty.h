//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef SERVERNETWORKPROPERTY_H
#define SERVERNETWORKPROPERTY_H
#pragma once

#include "networkvar.h"
#include "iservernetworkable.h"
#include "server_class.h"
#include "edict.h"
#include "timedeventmgr.h"
#include "ehandle.h"
#include "util.h"

typedef CHandle<CBaseEntity> EHANDLE;

//
// Lightweight base class for networkable data on the server.
//
class CServerNetworkProperty : public IServerNetworkable, public IEventRegisterCallback
{
public:
	DECLARE_CLASS_NOBASE( CServerNetworkProperty );

public:
	CServerNetworkProperty();
	virtual	~CServerNetworkProperty();

public:
// IServerNetworkable implementation.
	virtual IHandleEntity  *GetEntityHandle( );
	virtual edict_t			*GetEdict() const;
	virtual CBaseNetworkable* GetBaseNetworkable();
	virtual CBaseEntity*	GetBaseEntity();
	virtual ServerClass*	GetServerClass();
	virtual const char*		GetClassName() const;
	virtual void			Release();
	virtual int				AreaNum() const;
	virtual PVSInfo_t*		GetPVSInfo();

public:
	// Other public methods
	void Init( CBaseEntity *pEntity );

	void AttachEdict( edict_t *pRequiredEdict = NULL );
	
	// Methods to get the entindex + edict
	int	entindex() const;
	edict_t *edict();
	const edict_t *edict() const;

	// Sets the edict pointer (for swapping edicts)
	void SetEdict( edict_t *pEdict );

	// All these functions call through to CNetStateMgr. 
	// See CNetStateMgr for details about these functions.
	void NetworkStateForceUpdate();
	void NetworkStateChanged();
	void NetworkStateChanged( unsigned short offset );

	// Marks the PVS information dirty
	void MarkPVSInformationDirty();

	// This is useful for entities that don't change frequently or that the client
	// doesn't need updates on very often. If you use this mode, the server will only try to
	// detect state changes every N seconds, so it will save CPU cycles and bandwidth.
	//
	// Note: N must be less than AUTOUPDATE_MAX_TIME_LENGTH.
	//
	// Set back to zero to disable the feature.
	//
	// This feature works on top of manual mode. 
	// - If you turn it on and manual mode is off, it will autodetect changes every N seconds.
	// - If you turn it on and manual mode is on, then every N seconds it will only say there
	//   is a change if you've called NetworkStateChanged.
	void			SetUpdateInterval( float N );

	// You can use this to override any entity's ShouldTransmit behavior.
	// void SetTransmitProxy( CBaseTransmitProxy *pProxy );

	// This version does a PVS check which also checks for connected areas
	bool IsInPVS( const CCheckTransmitInfo *pInfo );

	// This version doesn't do the area check
	bool IsInPVS( const edict_t *pRecipient, const void *pvs, int pvssize );

	// Called by the timed event manager when it's time to detect a state change.
	virtual void FireEvent();

	// Recomputes PVS information
	void RecomputePVSInformation();

	// Detaches the edict.. should only be called by CBaseNetworkable's destructor.
	void ReleaseEdict();
	edict_t *DetachEdict();

	bool TimerEventActive();

	bool UseMinimalSendTable( int iClientIndex ); // Only used by proxies
	void SetUseMinimalSendTable( int iClientIndex, bool state );

private:
	CBaseEntity *GetOuter();

	// Marks the networkable that it will should transmit
	void SetTransmit( CCheckTransmitInfo *pInfo );

private:
	bool m_bDestroyed;

	CBaseEntity *m_pOuter;
	// CBaseTransmitProxy *m_pTransmitProxy;
	edict_t	*m_pPev;
	PVSInfo_t m_PVSInfo;
	ServerClass *m_pServerClass;

	// Counters for SetUpdateInterval.
	CEventRegister	m_TimerEvent;
	bool m_bPendingStateChange : 1;

	CEnginePlayerBitVec m_UseMinimalSendTable;

//	friend class CBaseTransmitProxy;
};

inline bool CServerNetworkProperty::UseMinimalSendTable( int iClientIndex )
{
	return m_UseMinimalSendTable.IsBitSet( iClientIndex );
}

//-----------------------------------------------------------------------------
// inline methods // TODOMO does inline work on virtual functions ?
//-----------------------------------------------------------------------------
inline CBaseNetworkable* CServerNetworkProperty::GetBaseNetworkable()
{
	return NULL;
}

inline CBaseEntity* CServerNetworkProperty::GetBaseEntity()
{
	return m_pOuter;
}

inline CBaseEntity *CServerNetworkProperty::GetOuter()
{
	return m_pOuter;
}

inline PVSInfo_t *CServerNetworkProperty::GetPVSInfo()
{
	return &m_PVSInfo;
}


//-----------------------------------------------------------------------------
// Marks the PVS information dirty
//-----------------------------------------------------------------------------
inline void CServerNetworkProperty::MarkPVSInformationDirty()
{
	if ( m_pPev )
	{
		m_pPev->m_fStateFlags |= FL_EDICT_DIRTY_PVS_INFORMATION;
	}
}


//-----------------------------------------------------------------------------
// Methods related to the net state mgr
//-----------------------------------------------------------------------------
inline void CServerNetworkProperty::NetworkStateForceUpdate()
{ 
	if ( m_pPev )
		m_pPev->StateChanged();
}

inline void CServerNetworkProperty::NetworkStateChanged()
{ 
	// If we're using the timer, then ignore this call.
	if ( m_TimerEvent.IsRegistered() )
	{
		// If we're waiting for a timer event, then queue the change so it happens
		// when the timer goes off.
		m_bPendingStateChange = true;
	}
	else
	{
		if ( m_pPev )
			m_pPev->StateChanged();
	}
}

inline void CServerNetworkProperty::NetworkStateChanged( unsigned short varOffset )
{ 
	// If we're using the timer, then ignore this call.
	if ( m_TimerEvent.IsRegistered() )
	{
		// If we're waiting for a timer event, then queue the change so it happens
		// when the timer goes off.
		m_bPendingStateChange = true;
	}
	else
	{
		if ( m_pPev )
			m_pPev->StateChanged( varOffset );
	}
}

//-----------------------------------------------------------------------------
// Methods to get the entindex + edict
//-----------------------------------------------------------------------------
inline int CServerNetworkProperty::entindex() const
{
	return ENTINDEX( m_pPev );
}

inline edict_t* CServerNetworkProperty::GetEdict() const
{
	// This one's virtual, that's why we have to two other versions
	return m_pPev;
}

inline edict_t *CServerNetworkProperty::edict()
{
	return m_pPev;
}

inline const edict_t *CServerNetworkProperty::edict() const
{
	return m_pPev;
}


//-----------------------------------------------------------------------------
// Sets the edict pointer (for swapping edicts)
//-----------------------------------------------------------------------------
inline void CServerNetworkProperty::SetEdict( edict_t *pEdict )
{
	m_pPev = pEdict;
}


inline int CServerNetworkProperty::AreaNum() const
{
	const_cast<CServerNetworkProperty*>(this)->RecomputePVSInformation();
	return m_PVSInfo.m_nAreaNum;
}

inline bool CServerNetworkProperty::TimerEventActive()
{
	return m_TimerEvent.IsRegistered();
}

#endif // SERVERNETWORKPROPERTY_H
