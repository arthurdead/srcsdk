//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recvproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "cdll_client_int.h"
#include "proto_version.h"

//-----------------------------------------------------------------------------
// Purpose: Okay, so we have to queue up the actual ehandle to entity lookup for the following reason:
//  If a player has an EHandle/CHandle to an object such as a weapon, since the player is in slot 1-31, then
//  if the weapon is created and given to the player in the same frame, then the weapon won't have been
//  created at the time we parse this EHandle index, since the player is ahead of every other entity in the
//  packet (except for the world).
// So instead, we remember which ehandles need to be set and we set them after all network data has
//  been received.  Sigh.
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------

void RecvProxy_EHandle( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CBaseHandle *pEHandle = (CBaseHandle*)pOut;
	
	if ( pData->m_Value.m_Int == INVALID_NETWORKED_EHANDLE_VALUE )
	{
		*pEHandle = NULL;
	}
	else
	{
		int iEntity = pData->m_Value.m_Int & ((1 << MAX_EDICT_BITS) - 1);
		int iSerialNum = pData->m_Value.m_Int >> MAX_EDICT_BITS;
		
		pEHandle->Init( iEntity, iSerialNum );
	}
}

RecvProp RecvPropEHandle(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	RecvVarProxyFn proxyFn )
{
	return RecvPropInt( pVarName, offset, sizeofVar, 0, proxyFn );
}


//-----------------------------------------------------------------------------
// Moveparent receive proxies
//-----------------------------------------------------------------------------
void RecvProxy_MoveParent( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CHandle<C_BaseEntity> *pHandle = (CHandle<C_BaseEntity>*)pOut;
	RecvProxy_EHandle( pData, pStruct, (CBaseHandle*)pHandle );
}


void RecvProxy_InterpolationAmountChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	bool before = *((bool*)pOut);

	// Have the regular proxy store the data.
	RecvProxy_UInt8( pData, pStruct, pOut );

	// m_bSimulatedEveryTick & m_bAnimatedEveryTick are boolean
	if ( *((bool*)pOut) != before )
	{
		C_BaseEntity *pEntity = (C_BaseEntity *) pStruct;
		pEntity->Interp_UpdateInterpolationAmounts( pEntity->GetVarMapping() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a time value
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
static void RecvProxy_Time( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	float	t;
	float	clock_base;
	float	offset;

	// Get msec offset
	offset	= ( float )pData->m_Value.m_Int / 1000.0f;

	// Get base
	clock_base = floor( engine->GetLastTimeStamp() );

	// Add together and clamp to msec precision
	t = ClampToMsec( clock_base + offset );

	// Store decoded value
	*( float * )pOut = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
// Output : RecvProp
//-----------------------------------------------------------------------------
RecvProp RecvPropTime(
	const char *pVarName, 
	int offset, 
	int sizeofVar/*=SIZEOF_IGNORE*/ )
{
//	return RecvPropInt( pVarName, offset, sizeofVar, 0, RecvProxy_Time );
	return RecvPropFloat( pVarName, offset, sizeofVar );
};

//-----------------------------------------------------------------------------
// Purpose: Okay, so we have to queue up the actual ehandle to entity lookup for the following reason:
//  If a player has an EHandle/CHandle to an object such as a weapon, since the player is in slot 1-31, then
//  if the weapon is created and given to the player in the same frame, then the weapon won't have been
//  created at the time we parse this EHandle index, since the player is ahead of every other entity in the
//  packet (except for the world).
// So instead, we remember which ehandles need to be set and we set them after all network data has
//  been received.  Sigh.
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------
static void RecvProxy_IntToPredictableId( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CPredictableId *pId = (CPredictableId*)pOut;
	Assert( pId );
	pId->SetRaw( pData->m_Value.m_Int );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVarName - 
//			sizeofVar - 
// Output : RecvProp
//-----------------------------------------------------------------------------
RecvProp RecvPropPredictableId(
	const char *pVarName, 
	int offset, 
	int sizeofVar/*=SIZEOF_IGNORE*/ )
{
	return RecvPropInt( pVarName, offset, sizeofVar, 0, RecvProxy_IntToPredictableId );
}

void RecvProxy_StringT( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	char pStrOut[DT_MAX_STRING_BUFFERSIZE];
	if ( pData->m_pRecvProp->m_StringBufferSize <= 0 )
	{
		return;
	}

	for ( int i=0; i < pData->m_pRecvProp->m_StringBufferSize; i++ )
	{
		pStrOut[i] = pData->m_Value.m_pString[i];
		if ( pStrOut[i] == 0 )
			break;
	}

	pStrOut[pData->m_pRecvProp->m_StringBufferSize-1] = 0;

	string_t *pTStr = (string_t*)pOut;
	*pTStr = AllocPooledString( pStrOut );
}

RecvProp RecvPropStringT(
	const char *pVarName, 
	int offset, 
	int sizeofVar )
{
	// Make sure it's the right type.
	Assert( sizeofVar == sizeof( string_t ) );

	return RecvPropString( pVarName, offset, DT_MAX_STRING_BUFFERSIZE, 0, RecvProxy_StringT );
}
