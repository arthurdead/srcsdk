//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "sendproxy.h"
#include "basetypes.h"
#include "baseentity.h"
#include "team.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SendProxy_EHandle( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	CBaseHandle *pHandle = (CBaseHandle*)pVarData;

	if ( pHandle && pHandle->Get() )
	{
		int iSerialNum = pHandle->GetSerialNumber() & ( (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1 );
		pOut->m_Int = pHandle->GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		pOut->m_Int = INVALID_NETWORKED_EHANDLE_VALUE;
	}
}

void SendProxy_ModelIndex( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	modelindex_t *pHandle = (modelindex_t*)pVarData;
	if( (int)*pHandle == 0 )
		*pHandle = INVALID_MODEL_INDEX;
	Assert( IsNetworkedModelIndex( *pHandle ) );
	pOut->m_Int = (int)*pHandle;
}

SendProp SendPropEHandle(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int flags,
	SendVarProxyFn proxyFn )
{
	return SendPropInt( pVarName, offset, sizeofVar, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED|flags, proxyFn );
}

//-----------------------------------------------------------------------------
// Purpose: Proxy that only sends data to team members
// Input  : *pStruct - 
//			*pData - 
//			*pOut - 
//			objectID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void* SendProxy_OnlyToTeam( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity*)pStruct;
	if ( pEntity )
	{
		CTeam *pTeam = pEntity->GetTeam();
		if ( pTeam )
		{
			pRecipients->ClearAllRecipients();
			for ( int i=0; i < pTeam->GetNumPlayers(); i++ )
				pRecipients->SetRecipient( pTeam->GetPlayer( i )->GetClientIndex() );
		
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_OnlyToTeam );

#define TIME_BITS 24

// This table encodes edict data.
#if 0
static void SendProxy_Time( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	float clock_base = floor( gpGlobals->curtime );
	float t = *( float * )pVarData;
	float dt = t - clock_base;
	int addt = Floor2Int( 1000.0f * dt + 0.5f );
	// TIME_BITS bits gives us TIME_BITS-1 bits plus sign bit
	int maxoffset = 1 << ( TIME_BITS - 1);

	addt = clamp( addt, -maxoffset, maxoffset );

	pOut->m_Int = addt;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
//			flags - 
//			pId - 
// Output : SendProp
//-----------------------------------------------------------------------------
SendProp SendPropTime(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
//	return SendPropInt( pVarName, offset, sizeofVar, TIME_BITS, 0, SendProxy_Time );
	// FIXME:  Re-enable above when it doesn't cause lots of deltas
	return SendPropFloat( pVarName, offset, sizeofVar, -1, SPROP_NOSCALE );
}

#define PREDICTABLE_ID_BITS 31

//-----------------------------------------------------------------------------
// Purpose: Converts a predictable Id to an integer
// Input  : *pStruct - 
//			*pVarData - 
//			*pOut - 
//			iElement - 
//			objectID - 
//-----------------------------------------------------------------------------
static void SendProxy_PredictableIdToInt( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CPredictableId* pId = ( CPredictableId * )pVarData;
	if ( pId )
	{
		pOut->m_Int = pId->GetRaw();
	}
	else
	{
		pOut->m_Int = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
//			flags - 
//			pId - 
// Output : SendProp
//-----------------------------------------------------------------------------
SendProp SendPropPredictableId(
	const char *pVarName,
	int offset,
	int sizeofVar )
{
	return SendPropInt( pVarName, offset, sizeofVar, PREDICTABLE_ID_BITS, SPROP_UNSIGNED, SendProxy_PredictableIdToInt );
}

void SendProxy_StringT( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	string_t &str = *((string_t*)pVarData);
	pOut->m_pString = (char*)STRING( str );
}


SendProp SendPropStringT( const char *pVarName, int offset, int sizeofVar )
{
	// Make sure it's the right type.
	Assert( sizeofVar == sizeof( string_t ) );

	return SendPropString( pVarName, offset, DT_MAX_STRING_BUFFERSIZE, 0, SendProxy_StringT );
}

void* SendProxy_SendMinimalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pUnit = (CBaseEntity*)pVarData;
	if ( pUnit )
	{
		for( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			if( !pUnit->NetworkProp()->UseMinimalSendTable( i ) )
				pRecipients->ClearRecipient( i );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendMinimalDataTable );

void* SendProxy_SendFullDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the unit entity
	CBaseEntity *pUnit = (CBaseEntity*)pVarData;
	if ( pUnit )
	{
		for( int i = 0; i < gpGlobals->maxClients; i++ )
		{
			if( pUnit->NetworkProp()->UseMinimalSendTable( i ) )
				pRecipients->ClearRecipient( i );
		}
	}

	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendFullDataTable );
