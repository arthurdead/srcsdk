//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DT_UTLVECTOR_SEND_H
#define DT_UTLVECTOR_SEND_H
#pragma once


#include "dt_send.h"
#include "dt_utlvector_common.h"

typedef void (*EnsureCapacityFn)( void *pVoid, int offsetToUtlVector, int len );

// This gets associated with SendProps inside a utlvector and stores extra data needed to make it work.
class CSendPropExtra_UtlVector : public CSendPropExtra_Base
{
public:
	CSendPropExtra_UtlVector() :
		m_DataTableProxyFn( NULL ),
		m_ProxyFn( NULL ),
		m_EnsureCapacityFn( NULL ),
		m_ElementStride( 0 ),
		m_Offset( 0 ),
		m_nMaxElements( 0 )	
	{
	}

	SendTableProxyFn m_DataTableProxyFn;	// If it's a datatable, then this is the proxy they specified.
	SendVarProxyFn m_ProxyFn;				// If it's a non-datatable, then this is the proxy they specified.
	EnsureCapacityFn m_EnsureCapacityFn;
	int m_ElementStride;					// Distance between each element in the array.
	int m_Offset;							// # bytes from the parent structure to its utlvector.
	int m_nMaxElements;						// For debugging...
};

#define SENDINFO_UTLVECTOR( varName )	type_identity<typename NetworkVarType<decltype( ((currentSendDTClass*)0)->varName )>::type>{}, \
										#varName, \
										offsetof(currentSendDTClass, varName), \
										sizeof(((currentSendDTClass*)0)->varName[0]), \
										GetEnsureCapacityTemplate( ((currentSendDTClass*)0)->varName )



#define SendPropUtlVectorDataTable( varName, nMaxElements, dataTableName ) \
	SendPropUtlVector( \
		SENDINFO_UTLVECTOR( varName ), \
		nMaxElements, \
		SendPropDataTable( NULL, 0, &REFERENCE_SEND_TABLE( dataTableName ) ) \
		)

//
// Set it up to transmit a CUtlVector of basic types or of structures.
//
// pArrayProp doesn't need a name, offset, or size. You can pass 0 for all those.
// Example usage:
//
//	SendPropUtlVectorDataTable( m_StructArray, 11, DT_TestStruct )
//
//	SendPropUtlVector( 
//		SENDINFO_UTLVECTOR( m_FloatArray ),
//		16,	// max elements
//		SendPropFloat( NULL, 0, 0, 0, SPROP_NOSCALE )
//		)
//
SendPropEx SendPropUtlVector_impl(
	const char *pVarName,		// Use SENDINFO_UTLVECTOR to generate these first 4 parameters.
	int offset,
	int sizeofVar,
	EnsureCapacityFn ensureFn,

	int nMaxElements,			// Max # of elements in the array. Keep this as low as possible.
	SendPropEx pArrayProp,		// Describe the data inside of each element in the array.
	SendTableProxyFn varProxy,	// This can be overridden to control who the array is sent to.
	DTPriority_t priority,

	void(*UtlVectorElement)( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID ),

	void*(UtlVectorElement_DataTable)( 
	const SendProp *pProp,
	const void *pStructBase, 
	const void *pData, 
	CSendProxyRecipients *pRecipients, 
	int objectID ),
	
	void(*UtlVectorLength)( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID )

	);

template<typename T>
void SendProxy_UtlVectorElement( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	Assert( pExtra );

	// Kind of lame overloading element stride to hold the element index,
	// but we can easily move it into its SetExtraData stuff if we need to.
	iElement = pProp->GetElementStride();

	T *pUtlVec = (T*)((char*)pStruct + pExtra->m_Offset);
	if ( iElement >= pUtlVec->Count() )
	{
		// Pass in zero value.
		memset( (void *)pOut, 0, sizeof( *pOut ) );
	}
	else
	{
		// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
		pExtra->m_ProxyFn( pProp, pData, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, pOut, 0, objectID );
	}
}

template<typename T>
void* SendProxy_UtlVectorElement_DataTable( 
	const SendProp *pProp,
	const void *pStructBase, 
	const void *pData, 
	CSendProxyRecipients *pRecipients, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();

	int iElement = pProp->m_ElementStride;
	Assert( iElement < pExtra->m_nMaxElements );

	// This should have gotten called in SendProxy_LengthTable before we get here, so 
	// the capacity should be correct.
#ifdef _DEBUG
	pExtra->m_EnsureCapacityFn( (void*)pStructBase, pExtra->m_Offset, pExtra->m_nMaxElements );
#endif

	// NOTE: this is cheesy because we're assuming the type of the template class, but it does the trick.
	T *pUtlVec = (T*)((char*)pStructBase + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
	return pExtra->m_DataTableProxyFn( pProp, pData, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, pRecipients, objectID );
}

template<typename T>
void SendProxy_UtlVectorLength( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	
	// NOTE: this is cheesy because we're assuming the type of the template class, but it does the trick.
	T *pUtlVec = (T*)((char*)pStruct + pExtra->m_Offset);
	
	// Don't let them overflow the buffer because they might expect that to get transmitted to the client.
	pOut->m_Int = pUtlVec->Count();
	if ( pOut->m_Int > pExtra->m_nMaxElements )
	{
		Assert( false );
		pOut->m_Int = pExtra->m_nMaxElements;
	}
}

template<typename T>
SendPropEx SendPropUtlVector(
	type_identity<T>,
	const char *pVarName,		// Use SENDINFO_UTLVECTOR to generate these first 4 parameters.
	int offset,
	int sizeofVar,
	EnsureCapacityFn ensureFn,

	int nMaxElements,			// Max # of elements in the array. Keep this as low as possible.
	SendPropEx pArrayProp,		// Describe the data inside of each element in the array.
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable,	// This can be overridden to control who the array is sent to.
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	)
{
	return SendPropUtlVector_impl(
		pVarName,
		offset,
		sizeofVar,
		ensureFn,
		nMaxElements,
		pArrayProp,
		varProxy,

		SendProxy_UtlVectorElement<T>,
		SendProxy_UtlVectorElement_DataTable<T>,
		SendProxy_UtlVectorLength<T>
	);
}

#endif	// DT_UTLVECTOR_SEND_H
