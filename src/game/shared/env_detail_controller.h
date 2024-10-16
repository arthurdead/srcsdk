//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ENVDETAILCONTROLLER_H
#define ENVDETAILCONTROLLER_H

#pragma once

#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#endif

#ifdef CLIENT_DLL
class C_EnvDetailController;
typedef C_EnvDetailController CSharedEnvDetailController;
#else
class CEnvDetailController;
typedef CEnvDetailController CSharedEnvDetailController;
#endif

#ifdef CLIENT_DLL
	#define CEnvDetailController C_EnvDetailController
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Implementation of the class that controls detail prop fade distances
//-----------------------------------------------------------------------------
class CEnvDetailController : public CSharedBaseEntity
{
public:
	DECLARE_CLASS( CEnvDetailController, CSharedBaseEntity );
	CEnvDetailController();
	virtual ~CEnvDetailController();

#ifdef CLIENT_DLL
	#undef CEnvDetailController
#endif // CLIENT_DLL

	DECLARE_NETWORKCLASS();

#ifndef CLIENT_DLL
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif // !CLIENT_DLL

	CNetworkVar( float, m_flFadeStartDist );
	CNetworkVar( float, m_flFadeEndDist );

	// ALWAYS transmit to all clients.
	virtual int UpdateTransmitState( void );

private:
#ifdef CLIENT_DLL
	float m_fOldFadeStartDist;
	float m_fOldFadeEndDist;
#endif // CLIENT_DLL
};

CSharedEnvDetailController * GetDetailController();

#endif
