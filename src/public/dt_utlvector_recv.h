//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DT_UTLVECTOR_RECV_H
#define DT_UTLVECTOR_RECV_H
#pragma once


#include "dt_recv.h"
#include "dt_utlvector_common.h"



#define RECVINFO_UTLVECTOR( varName )	type_identity<typename NetworkVarType<decltype( ((currentRecvDTClass*)0)->varName )>::type>{}, \
										#varName, \
										offsetof(currentRecvDTClass, varName), \
										sizeof(((currentRecvDTClass*)0)->varName[0]), \
										GetResizeUtlVectorTemplate( ((currentRecvDTClass*)0)->varName ), \
										GetEnsureCapacityTemplate( ((currentRecvDTClass*)0)->varName )

// Use this macro to specify a utlvector where you specify the function
// that gets called to make sure the size of the utlvector is correct.
// The size function looks like this: void ResizeUtlVector( void *pVoid, int len )
#define RECVINFO_UTLVECTOR_SIZEFN( varName, resizeFn )	#varName, \
										offsetof(currentRecvDTClass, varName), \
										sizeof(((currentRecvDTClass*)0)->varName[0]), \
										resizeFn, \
										GetEnsureCapacityTemplate( ((currentRecvDTClass*)0)->varName )


#define RecvPropUtlVectorDataTable( varName, nMaxElements, dataTableName ) \
	RecvPropUtlVector( RECVINFO_UTLVECTOR( varName ), nMaxElements, RecvPropDataTable(NULL,0,0, &REFERENCE_RECV_TABLE( dataTableName  ) ) )

template <typename T>
void RecvProxy_UtlVectorElement( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pData->m_pRecvProp->GetExtraData();

	// Kind of lame overloading element stride to hold the element index,
	// but we can easily move it into its SetExtraData stuff if we need to.
	int iElement = pData->m_pRecvProp->GetElementStride();

	// NOTE: this is cheesy, but it does the trick.
	T *pUtlVec = (T*)((char*)pStruct + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector.
	// Note: there should be space here as long as the element is < the max # elements
	// that we ensured capacity for in DataTableRecvProxy_LengthProxy.
	pExtra->m_ProxyFn( pData, pOut, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride );
}

template <typename T>
void RecvProxy_UtlVectorElement_DataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pProp->GetExtraData();

	int iElement = pProp->GetElementStride();
	Assert( iElement < pExtra->m_nMaxElements );

	// NOTE: this is cheesy, but it does the trick.
	T *pUtlVec = (T*)((char*)pData + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
	pExtra->m_DataTableProxyFn( pProp, pOut, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, objectID );
}

//
// Receive a property sent with SendPropUtlVector.
//
// Example usage:
//
//	RecvPropUtlVectorDataTable( m_StructArray, 11, DT_StructArray )
//
//	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_FloatArray ), 16, RecvPropFloat(NULL,0,0) )
//
RecvProp RecvPropUtlVector_impl(
	const char *pVarName,		// Use RECVINFO_UTLVECTOR to generate these first 5 parameters.
	int offset,
	int sizeofVar,
	ResizeUtlVectorFn fn,
	EnsureCapacityFn ensureFn,

	int nMaxElements,	// Max # of elements in the array. Keep this as low as possible.
	RecvProp pArrayProp,	// The definition of the property you're receiving into.
						// You can leave all of its parameters at 0 (name, offset, size, etc).

	void(*UtlVectorElement)( const CRecvProxyData *pData, void *pStruct, void *pOut ),
	void(*UtlVectorElement_DataTable)( const RecvProp *pProp, void **pOut, void *pData, int objectID )

	);

template <typename T>
RecvProp RecvPropUtlVector(
	type_identity<T>,
	const char *pVarName,		// Use RECVINFO_UTLVECTOR to generate these first 5 parameters.
	int offset,
	int sizeofVar,
	ResizeUtlVectorFn fn,
	EnsureCapacityFn ensureFn,

	int nMaxElements,	// Max # of elements in the array. Keep this as low as possible.
	RecvProp pArrayProp	// The definition of the property you're receiving into.
						// You can leave all of its parameters at 0 (name, offset, size, etc).
	)
{
	return RecvPropUtlVector_impl(
		pVarName,
		offset,
		sizeofVar,
		fn,
		ensureFn,
		nMaxElements,
		pArrayProp,

		RecvProxy_UtlVectorElement<T>,
		RecvProxy_UtlVectorElement_DataTable<T>
	);
}

#endif // DT_UTLVECTOR_RECV_H
