//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dt_utlvector_send.h"

#include "tier0/memdbgon.h"


extern const char *s_ElementNames[MAX_ARRAY_ELEMENTS];

void* SendProxy_LengthTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Make sure the array has space to hold all the elements.
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	pExtra->m_EnsureCapacityFn( (void*)pStructBase, pExtra->m_Offset, pExtra->m_nMaxElements );
	return (void*)pData;
}


// Note: your pArrayProp will NOT get iElement set to anything other than 0, because this function installs its
// own proxy in front of yours. pStruct will point at the CUtlVector and pData will point at the element in the CUtlVector.It will pass you the direct pointer to the element inside the CUtlVector.
//
// You can skip the first 3 parameters in pArrayProp because they're ignored. So your array specification
// could look like this:
//   	 SendPropUtlVector( 
//	    	SENDINFO_UTLVECTOR( m_FloatArray ),
//			SendPropFloat( NULL, 0, 0, 0 [# bits], SPROP_NOSCALE [flags] ) );
//
// Note: you have to be DILIGENT about calling NetworkStateChanged whenever an element in your CUtlVector changes
// since CUtlVector doesn't do this automatically.
SendPropEx SendPropUtlVector_impl(
	const char *pVarName,		// Use SENDINFO_UTLVECTOR to generate these 4.
	int offset,			// Used to generate pData in the function specified in varProxy.
	int sizeofVar,		// The size of each element in the utlvector.
	EnsureCapacityFn ensureFn,	// This is the value returned for elements out of the array's current range.
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

	)
{
	SendPropEx ret;

	Assert( nMaxElements <= MAX_ARRAY_ELEMENTS );

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( 0 );
	ret.SetPriority( priority );
	ret.SetDataTableProxyFn( varProxy );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}

	// Extra data bound to each of the properties.
	CSendPropExtra_UtlVector *pExtraData = new CSendPropExtra_UtlVector;
	
	pExtraData->m_nMaxElements = nMaxElements;
	pExtraData->m_ElementStride = sizeofVar;
	pExtraData->m_EnsureCapacityFn = ensureFn;
	pExtraData->m_Offset = offset;
#ifndef DT_PRIORITY_SUPPORTED
	pExtraData->m_priority = (byte)SENDPROP_DEFAULT_PRIORITY;
#endif

	if ( pArrayProp.m_Type == DPT_DataTable )
		pExtraData->m_DataTableProxyFn = pArrayProp.GetDataTableProxyFn();
	else
		pExtraData->m_ProxyFn = pArrayProp.GetProxyFn();


	SendProp *pProps = new SendProp[nMaxElements+1]; // TODO free that again

	// The first property is datatable with an int that tells the length of the array.
	// It has to go in a datatable, otherwise if this array holds datatable properties, it will be received last.
	SendProp *pLengthProp = new SendProp;
	*pLengthProp = SendPropInt( AllocateStringHelper( "lengthprop%d", nMaxElements ), 0, 0, NumBitsForCount( nMaxElements ), SPROP_UNSIGNED, UtlVectorLength );
	pLengthProp->SetExtraData( pExtraData );
	pLengthProp->m_Flags |= SPROP_UTLVECTOR_EXTRADATA;

	char *pLengthProxyTableName = AllocateUniqueDataTableName( true, "_LPT_%s_%d", pVarName, nMaxElements );
	SendTable *pLengthTable = new SendTable( pLengthProp, 1, pLengthProxyTableName );
	pProps[0] = SendPropDataTable( "lengthproxy", 0, pLengthTable, SendProxy_LengthTable );
	pProps[0].SetExtraData( pExtraData );
	pProps[0].m_Flags |= SPROP_UTLVECTOR_EXTRADATA;

	// TERROR:
	char *pParentArrayPropName = AllocateStringHelper( "%s", pVarName );
	Assert( pParentArrayPropName && *pParentArrayPropName ); // TERROR

	// The first element is a sub-datatable.
	for ( int i = 1; i < nMaxElements+1; i++ )
	{
		// copy array element property setting
		pProps[i].m_pMatchingRecvProp = pArrayProp.m_pMatchingRecvProp;
		pProps[i].m_Type = pArrayProp.m_Type;
		pProps[i].m_nBits = pArrayProp.m_nBits;
		pProps[i].m_fLowValue = pArrayProp.m_fLowValue;
		pProps[i].m_fHighValue = pArrayProp.m_fHighValue;
		pProps[i].m_pArrayProp = pArrayProp.m_pArrayProp;
		pProps[i].m_ArrayLengthProxy = pArrayProp.m_ArrayLengthProxy;
		pProps[i].m_nElements = pArrayProp.m_nElements;
		pProps[i].m_pExcludeDTName = pArrayProp.m_pExcludeDTName;
		pProps[i].m_fHighLowMul = pArrayProp.m_fHighLowMul;
	#ifdef DT_PRIORITY_SUPPORTED
		pProps[i].m_priority = pArrayProp.m_priority;
	#endif
		pProps[i].m_Flags = pArrayProp.m_Flags;
		pProps[i].m_ProxyFn = pArrayProp.m_ProxyFn;
		pProps[i].m_DataTableProxyFn = pArrayProp.m_DataTableProxyFn;
		pProps[i].m_pDataTable = pArrayProp.m_pDataTable;

		pProps[i].SetOffset( 0 ); // leave offset at 0 so pStructBase is always a pointer to the CUtlVector
		pProps[i].m_pVarName = s_ElementNames[i-1];	// give unique name
		pProps[i].m_pParentArrayPropName = pParentArrayPropName; // TERROR: For debugging...
		pProps[i].SetExtraData( pExtraData );
		pProps[i].m_Flags |= SPROP_UTLVECTOR_EXTRADATA;
		pProps[i].m_ElementStride = i-1;	// Kind of lame overloading element stride to hold the element index,
											// but we can easily move it into its SetExtraData stuff if we need to.
		
		// We provide our own proxy here.
		if ( pArrayProp.m_Type == DPT_DataTable )
		{
			pProps[i].SetDataTableProxyFn( UtlVectorElement_DataTable );
			pProps[i].SetFlags( SPROP_PROXY_ALWAYS_YES );
		}
		else
		{
			pProps[i].SetProxyFn( UtlVectorElement );
		}
	}

	SendTable *pTable = new SendTable( 
		pProps, 
		nMaxElements+1, 
		AllocateUniqueDataTableName( true, "_ST_%s_%d", pVarName, nMaxElements )
		);

	ret.SetDataTable( pTable );
	return ret;
}
