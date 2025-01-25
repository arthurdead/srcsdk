//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_SEND_H
#define DATATABLE_SEND_H

#pragma once

#include "dt_common.h"
#include "tier0/dbg.h"
#include "const.h"
#include "bitvec.h"
#include "datamap.h"
#include "coordsize.h"

DECLARE_LOGGING_CHANNEL( LOG_SENDPROP );

#ifdef GNUC
#undef offsetof
#define offsetof(s,m)	__builtin_offsetof(s,m)
#endif

// ------------------------------------------------------------------------ //
// Send proxies can be used to convert a variable into a networkable type 
// (a good example is converting an edict pointer into an integer index).

// These allow you to translate data. For example, if you had a user-entered 
// string number like "10" (call the variable pUserStr) and wanted to encode 
// it as an integer, you would use a SendPropInt32 and write a proxy that said:
// pOut->m_Int = atoi(pUserStr);

// pProp       : the SendProp that has the proxy
// pStructBase : the base structure (like CBaseEntity*).
// pData       : the address of the variable to proxy.
// pOut        : where to output the proxied value.
// iElement    : the element index if this data is part of an array (or 0 if not).
// objectID    : entity index for debugging purposes.

// Return false if you don't want the engine to register and send a delta to
// the clients for this property (regardless of whether it actually changed or not).
// ------------------------------------------------------------------------ //
typedef void (*SendVarProxyFn)( const SendPropInfo *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID );

// Return the pointer to the data for the datatable.
// If the proxy returns null, it's the same as if pRecipients->ClearAllRecipients() was called.
class CSendProxyRecipients;

typedef void* (*SendTableProxyFn)( 
	const SendPropInfo *pProp, 
	const void *pStructBase, 
	const void *pData, 
	CSendProxyRecipients *pRecipients, 
	int objectID );


class CNonModifiedPointerProxy
{
public:
	CNonModifiedPointerProxy( SendTableProxyFn fn );

public:
	
	SendTableProxyFn m_Fn;
	CNonModifiedPointerProxy *m_pNext;
};


// This tells the engine that the send proxy will not modify the pointer
// - it only plays with the recipients. This must be set on proxies that work
// this way, otherwise the engine can't track which properties changed
// in NetworkStateChanged().
#define REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( sendProxyFn ) static CNonModifiedPointerProxy __proxy_##sendProxyFn( sendProxyFn );


class CStandardSendProxiesV1
{
public:
	CStandardSendProxiesV1();

	SendVarProxyFn m_Int8;
	SendVarProxyFn m_Int16;
	SendVarProxyFn m_Int32;

	SendVarProxyFn m_UInt8;
	SendVarProxyFn m_UInt16;
	SendVarProxyFn m_UInt32;

	SendVarProxyFn m_Float;
	SendVarProxyFn m_Vector;

#ifdef DT_INT64_SUPPORTED
	SendVarProxyFn m_Int64;
	SendVarProxyFn m_UInt64;
#endif
};

class CStandardSendProxies : public CStandardSendProxiesV1
{
public:
	CStandardSendProxies();

	SendTableProxyFn m_DataTableToDataTable;
	SendTableProxyFn m_SendLocalDataTable;
	CNonModifiedPointerProxy **m_ppNonModifiedPointerProxies;
};

class CStandardSendProxiesEx : public CStandardSendProxies
{
public:
	CStandardSendProxiesEx();

#ifndef DT_INT64_SUPPORTED
	SendVarProxyFn m_Int64;
	SendVarProxyFn m_UInt64;
#endif
};

extern CStandardSendProxiesEx g_StandardSendProxies;


// Max # of datatable send proxies you can have in a tree.
#define MAX_DATATABLE_PROXIES	32

// ------------------------------------------------------------------------ //
// Datatable send proxies are used to tell the engine where the datatable's 
// data is and to specify which clients should get the data. 
//
// pRecipients is the object that allows you to specify which clients will
// receive the data.
// ------------------------------------------------------------------------ //
class CSendProxyRecipients
{
public:
	void	SetAllRecipients();					// Note: recipients are all set by default when each proxy is called.
	void	ClearAllRecipients();

	void	SetRecipient( int iClient );		// Note: these are CLIENT indices, not entity indices (so the first player's index is 0).
	void	ClearRecipient( int iClient );

	// Clear all recipients and set only the specified one.
	void	SetOnly( int iClient );

	// Set all recipients, save for the specified on
	void    ExcludeOnly( int iClient );

public:
	// Make sure we have enough room for the max possible player count
	CBitVec< ABSOLUTE_PLAYER_LIMIT >	m_Bits;
};

inline void CSendProxyRecipients::SetAllRecipients()
{
	m_Bits.SetAll();
}

inline void CSendProxyRecipients::ClearAllRecipients()
{
	m_Bits.ClearAll();
}

inline void CSendProxyRecipients::SetRecipient( int iClient )
{
	m_Bits.Set( iClient );
}

inline void	CSendProxyRecipients::ClearRecipient( int iClient )
{
	m_Bits.Clear( iClient );
}

inline void CSendProxyRecipients::SetOnly( int iClient )
{
	m_Bits.ClearAll();
	m_Bits.Set( iClient );
}



// ------------------------------------------------------------------------ //
// ArrayLengthSendProxies are used when you want to specify an array's length
// dynamically.
// ------------------------------------------------------------------------ //
typedef int (*ArrayLengthSendProxyFn)( const void *pStruct, int objectID );



class RecvProp;
class SendTableInfo;
class CSendTablePrecalc;


// -------------------------------------------------------------------------------------------------------------- //
// SendProp.
// -------------------------------------------------------------------------------------------------------------- //

// If SendProp::GetDataTableProxyIndex() returns this, then the proxy is one that always sends
// the data to all clients, so we don't need to store the results.
#define DATATABLE_PROXY_INDEX_NOPROXY	255
#define DATATABLE_PROXY_INDEX_INVALID	254

enum DTPriority_t : unsigned char;

UNORDEREDENUM_OPERATORS( DTPriority_t, unsigned char )

#define SENDPROP_DEFAULT_PRIORITY ((DTPriority_t)128)
#define SENDPROP_CHANGES_OFTEN_PRIORITY ((DTPriority_t)64)

class CSendPropExtra_Base
{
public:
	virtual ~CSendPropExtra_Base() {}

#ifndef DT_PRIORITY_SUPPORTED
	byte m_priority;
#endif
};

class SendPropInfo
{
public:
	SendPropInfo();
	virtual ~SendPropInfo();

	SendPropInfo(SendPropInfo &&other);
	SendPropInfo &operator=(SendPropInfo &&other);

	SendPropInfo(const SendPropInfo &other) = delete;
	SendPropInfo &operator=(const SendPropInfo &other) = delete;

	void				Clear();

	int					GetOffset() const;
	void				SetOffset( int i );

	SendVarProxyFn		GetProxyFn() const;
	void				SetProxyFn( SendVarProxyFn f );
	
	SendTableProxyFn	GetDataTableProxyFn() const;
	void				SetDataTableProxyFn( SendTableProxyFn f );
	
	SendTableInfo*			GetDataTable() const;
	void				SetDataTable( SendTableInfo *pTable );

	char const*			GetExcludeDTName() const;
	
	// If it's one of the numbered "000", "001", etc properties in an array, then
	// these can be used to get its array property name for debugging.
	const char*			GetParentArrayPropName() const;
	void				SetParentArrayPropName( char *pArrayPropName );

	const char*			GetName() const;

	bool				IsSigned() const;
	
	bool				IsExcludeProp() const;
	
	bool				IsInsideArray() const;	// Returns true if SPROP_INSIDEARRAY is set.
	void				SetInsideArray();

	// Arrays only.
	void				SetArrayProp( SendPropInfo *pProp );
	SendPropInfo*			GetArrayProp() const;

	// Arrays only.
	void					SetArrayLengthProxy( ArrayLengthSendProxyFn fn );
	ArrayLengthSendProxyFn	GetArrayLengthProxy() const;

	int					GetNumElements() const;
	void				SetNumElements( int nElements );

	// Return the # of bits to encode an array length (must hold GetNumElements()).
	int					GetNumArrayLengthBits() const;

	int					GetElementStride() const;

	SendPropType		GetType() const;

	DTFlags_t					GetFlags() const;
	void				SetFlags( DTFlags_t flags );	

	// Some property types bind more data to the SendProp in here.
	CSendPropExtra_Base*			GetExtraData() const;
	void				SetExtraData( CSendPropExtra_Base *pData );

	DTPriority_t 				GetPriority() const;
	void				SetPriority( DTPriority_t priority );

public:

	RecvProp		*m_pMatchingRecvProp;	// This is temporary and only used while precalculating
												// data for the decoders.

	SendPropType	m_Type;
	int				m_nBits;
	float			m_fLowValue;
	float			m_fHighValue;
	
	SendPropInfo		*m_pArrayProp;					// If this is an array, this is the property that defines each array element.
	ArrayLengthSendProxyFn	m_ArrayLengthProxy;	// This callback returns the array length.
	
	int				m_nElements;		// Number of elements in the array (or 1 if it's not an array).
	int				m_ElementStride;	// Pointer distance between array elements.

	const char *m_pExcludeDTName;			// If this is an exclude prop, then this is the name of the datatable to exclude a prop from.
	const char *m_pParentArrayPropName;

	const char		*m_pVarName;
	float			m_fHighLowMul;

#ifdef DT_PRIORITY_SUPPORTED
	byte			m_priority;
#endif
	
public:

	DTFlags_t					m_Flags;				// SPROP_ flags.

public:

	SendVarProxyFn		m_ProxyFn;				// NULL for DPT_DataTable.
	SendTableProxyFn	m_DataTableProxyFn;		// Valid for DPT_DataTable.
	
	SendTableInfo			*m_pDataTable;
	
	// SENDPROP_VECTORELEM makes this negative to start with so we can detect that and
	// set the SPROP_IS_VECTOR_ELEM flag.
	int					m_Offset;

	// Extra data bound to this property.
	CSendPropExtra_Base			*m_pExtraData;
};

class SendPropInfoEx : public SendPropInfo
{
public:
	using SendPropInfo::SendPropInfo;
	using SendPropInfo::operator=;
};

inline DTPriority_t SendPropInfo::GetPriority() const
{
#ifdef DT_PRIORITY_SUPPORTED
	return (DTPriority_t)m_priority;
#else
	if((m_Flags & SPROP_ALLOCATED_EXTRADATA) != SPROP_NONE) {
		if(m_pExtraData) {
			return (DTPriority_t)m_pExtraData->m_priority;
		}
	}

	return SENDPROP_DEFAULT_PRIORITY;
#endif
}

inline void	SendPropInfo::SetPriority( DTPriority_t priority )
{
#ifdef DT_PRIORITY_SUPPORTED
	m_priority = (byte)priority;
#else
	if(!m_pExtraData) {
		m_pExtraData = new CSendPropExtra_Base;
		m_Flags |= SPROP_ALLOCATED_EXTRADATA;
	}

	if((m_Flags & SPROP_ALLOCATED_EXTRADATA) != SPROP_NONE) {
		m_pExtraData->m_priority = (byte)priority;
	}
#endif
}

inline int SendPropInfo::GetOffset() const
{
	return m_Offset; 
}

inline void SendPropInfo::SetOffset( int i )
{
	m_Offset = i; 
}

inline SendVarProxyFn SendPropInfo::GetProxyFn() const
{
	Assert( m_Type != DPT_DataTable );
	return m_ProxyFn; 
}

inline void SendPropInfo::SetProxyFn( SendVarProxyFn f )
{
	m_ProxyFn = f; 
}

inline SendTableProxyFn SendPropInfo::GetDataTableProxyFn() const
{
	Assert( m_Type == DPT_DataTable );
	return m_DataTableProxyFn; 
}

inline void SendPropInfo::SetDataTableProxyFn( SendTableProxyFn f )
{
	m_DataTableProxyFn = f; 
}

inline SendTableInfo* SendPropInfo::GetDataTable() const
{
	return m_pDataTable;
}

inline void SendPropInfo::SetDataTable( SendTableInfo *pTable )
{
	m_pDataTable = pTable; 
}

inline char const* SendPropInfo::GetExcludeDTName() const
{
	return m_pExcludeDTName; 
}

inline const char* SendPropInfo::GetParentArrayPropName() const
{
	return m_pParentArrayPropName;
}

inline void	SendPropInfo::SetParentArrayPropName( char *pArrayPropName )
{
	Assert( !m_pParentArrayPropName );
	m_pParentArrayPropName = pArrayPropName;
}

inline const char* SendPropInfo::GetName() const
{
	return m_pVarName; 
}


inline bool SendPropInfo::IsSigned() const
{
	return (m_Flags & SPROP_UNSIGNED) == SPROP_NONE; 
}

inline bool SendPropInfo::IsExcludeProp() const
{
	return (m_Flags & SPROP_EXCLUDE) != SPROP_NONE;
}

inline bool	SendPropInfo::IsInsideArray() const
{
	return (m_Flags & SPROP_INSIDEARRAY) != SPROP_NONE;
}

inline void SendPropInfo::SetInsideArray()
{
	m_Flags |= SPROP_INSIDEARRAY;
}

inline void SendPropInfo::SetArrayProp( SendPropInfo *pProp )
{
	m_pArrayProp = pProp;
}

inline SendPropInfo* SendPropInfo::GetArrayProp() const
{
	return m_pArrayProp;
}
	 
inline void SendPropInfo::SetArrayLengthProxy( ArrayLengthSendProxyFn fn )
{
	m_ArrayLengthProxy = fn;
}

inline ArrayLengthSendProxyFn SendPropInfo::GetArrayLengthProxy() const
{
	return m_ArrayLengthProxy;
}
	 
inline int SendPropInfo::GetNumElements() const
{
	return m_nElements; 
}

inline void SendPropInfo::SetNumElements( int nElements )
{
	m_nElements = nElements;
}

inline int SendPropInfo::GetElementStride() const
{
	return m_ElementStride; 
}

inline SendPropType SendPropInfo::GetType() const
{
	return m_Type; 
}

inline DTFlags_t SendPropInfo::GetFlags() const
{
	return m_Flags;
}

inline void SendPropInfo::SetFlags( DTFlags_t flags )
{
	m_Flags = flags;
}

inline CSendPropExtra_Base* SendPropInfo::GetExtraData() const
{
	return m_pExtraData;
}

inline void SendPropInfo::SetExtraData( CSendPropExtra_Base *pData )
{
	if((m_Flags & SPROP_ALLOCATED_EXTRADATA) != SPROP_NONE) {
		if(m_pExtraData) {
			delete m_pExtraData;
		}
	}

	m_pExtraData = pData;
}


// -------------------------------------------------------------------------------------------------------------- //
// SendTable.
// -------------------------------------------------------------------------------------------------------------- //

class SendTableInfo
{
public:

	typedef SendPropInfo PropType;

	SendTableInfo();
	SendTableInfo( SendPropInfo *pProps, int nProps, const char *pNetTableName );
	~SendTableInfo();

	SendTableInfo(const SendTableInfo &) = delete;
	SendTableInfo &operator=(const SendTableInfo &) = delete;
	SendTableInfo(SendTableInfo &&) = delete;
	SendTableInfo &operator=(SendTableInfo &&) = delete;

	void		Construct( SendPropInfo *pProps, int nProps, const char *pNetTableName );

	const char*	GetName() const;
	
	int			GetNumProps() const;
	SendPropInfo*	GetProp( int i );

	// Used by the engine.
	bool		IsInitialized() const;
	void		SetInitialized( bool bInitialized );

	// Used by the engine while writing info into the signon.
	void		SetWriteFlag(bool bHasBeenWritten);
	bool		GetWriteFlag() const;

	bool		HasPropsEncodedAgainstTickCount() const;
	void		SetHasPropsEncodedAgainstTickcount( bool bState );

public:

	SendPropInfo	*m_pProps;
	int			m_nProps;

	const char	*m_pNetTableName;	// The name matched between client and server.

	// The engine hooks the SendTable here.
	CSendTablePrecalc	*m_pPrecalc;


protected:		
	bool		m_bInitialized : 1;	
	bool		m_bHasBeenWritten : 1;		
	bool		m_bHasPropsEncodedAgainstCurrentTickCount : 1; // m_flSimulationTime and m_flAnimTime, e.g.
};


inline const char* SendTableInfo::GetName() const
{
	return m_pNetTableName;
}


inline int SendTableInfo::GetNumProps() const
{
	return m_nProps;
}


inline SendPropInfo* SendTableInfo::GetProp( int i )
{
	Assert( i >= 0 && i < m_nProps );
	return &m_pProps[i];
}


inline bool SendTableInfo::IsInitialized() const
{
	return m_bInitialized;
}


inline void SendTableInfo::SetInitialized( bool bInitialized )
{
	m_bInitialized = bInitialized;
}


inline bool SendTableInfo::GetWriteFlag() const
{
	return m_bHasBeenWritten;
}


inline void SendTableInfo::SetWriteFlag(bool bHasBeenWritten)
{
	m_bHasBeenWritten = bHasBeenWritten;
}

inline bool SendTableInfo::HasPropsEncodedAgainstTickCount() const
{
	return m_bHasPropsEncodedAgainstCurrentTickCount;
}

inline void SendTableInfo::SetHasPropsEncodedAgainstTickcount( bool bState )
{
	m_bHasPropsEncodedAgainstCurrentTickCount = bState;
}

// ------------------------------------------------------------------------------------------------------ //
// Use BEGIN_SEND_TABLE if you want to declare a SendTable and have it inherit all the properties from
// its base class. There are two requirements for this to work:

// 1. Its base class must have a static SendTable pointer member variable called m_pClassSendTable which
//    points to its send table. The DECLARE_SERVERCLASS and IMPLEMENT_SERVERCLASS macros do this automatically.

// 2. Your class must typedef its base class as BaseClass. So it would look like this:
//    class Derived : public CBaseEntity
//    {
//    typedef CBaseEntity BaseClass;
//    };

// If you don't want to interit a base class's properties, use BEGIN_SEND_TABLE_NOBASE.
// ------------------------------------------------------------------------------------------------------ //
#define BEGIN_SEND_TABLE(className, tableName) \
	BEGIN_SEND_TABLE_NOBASE(className, tableName) \
		SendPropDataTable("BaseClass", 0, className::BaseClass::m_pClassSendTable, SendProxy_DataTableToDataTable),

#define BEGIN_SEND_TABLE_NOBASE(className, tableName) \
	template <typename T> void ServerClassInit(); \
	namespace tableName { \
		struct ignored; \
	} \
	template <> void ServerClassInit<tableName::ignored>(); \
	namespace tableName { \
		INIT_PRIORITY(101) SendTableInfo g_SendTable;\
		INIT_PRIORITY(65535) class SendTableInit_t { \
		public: \
			SendTableInit_t() { \
				ServerClassInit<tableName::ignored>(); \
			} \
		} g_SendTableInit; \
	} \
	template <> void ServerClassInit<tableName::ignored>() \
	{ \
		typedef className currentSendDTClass; \
		static const char *g_pSendTableName = #tableName; \
		SendTableInfo &sendTable = tableName::g_SendTable; \
		static SendPropInfo g_SendProps[] = { \
			SendPropInfo(),		// It adds a dummy property at the start so you can define "empty" SendTables.

#define END_SEND_TABLE() \
		};\
		if ( SIZE_OF_ARRAY( g_SendProps ) > 1 ) \
		{ \
			sendTable.Construct(&g_SendProps[1], SIZE_OF_ARRAY(g_SendProps) - 1, g_pSendTableName);\
		} \
		else \
		{ \
			sendTable.Construct(NULL, 0, g_pSendTableName);\
		} \
	} 

// These can simplify creating the variables.
// Note: currentSendDTClass::MakeANetworkVar_##varName equates to currentSendDTClass. It's
// there as a check to make sure all networked variables use the CNetworkXXXX macros in network_var.h.
#define SENDINFO(varName) \
	DT_VARNAME(varName), currentSendDTClass::GetOffset_##varName##_memory(), sizeof(((currentSendDTClass*)0)->varName)
#define SENDINFO_ARRAY(varName) \
	DT_VARNAME(varName), currentSendDTClass::GetOffset_##varName##_memory(), sizeof(((currentSendDTClass*)0)->varName[0])
#define SENDINFO_ARRAY3(varName) \
	DT_VARNAME(varName), currentSendDTClass::GetOffset_##varName##_memory(), sizeof(((currentSendDTClass*)0)->varName[0]), (sizeof(((currentSendDTClass*)0)->varName)/sizeof(((currentSendDTClass*)0)->varName[0]))
#define SENDINFO_ARRAYELEM(varName, i) \
	DT_VARNAME_ARRAYELEM(varName, i), (currentSendDTClass::GetOffset_##varName##_memory() + (sizeof(((currentSendDTClass*)0)->varName[i]) * i)), sizeof(((currentSendDTClass*)0)->varName[i])
#define SENDINFO_NETWORKARRAYELEM(varName, i) \
	DT_VARNAME_ARRAYELEM(varName, i), (currentSendDTClass::GetOffset_##varName##_memory() + (sizeof(((currentSendDTClass*)0)->varName[i]) * i)), sizeof(((currentSendDTClass*)0)->varName[i])

// NOTE: Be VERY careful to specify any other vector elems for the same vector IN ORDER and 
// right after each other, otherwise it might miss the Y or Z component in SP.
//
// Note: this macro specifies a negative offset so the engine can detect it and setup m_pNext
#define SENDINFO_VECTORELEM(varName, i) \
	DT_VARNAME_VECTORELEM(varName, i), -(int)(currentSendDTClass::GetOffset_##varName##_memory() + (sizeof(((currentSendDTClass*)0)->varName[i]) * i)), sizeof(((currentSendDTClass*)0)->varName[i])

#define SENDINFO_STRUCTELEM(structVarName, varName) \
	DT_VARNAME_STRUCTELEM(structVarName, varName), (currentSendDTClass::GetOffset_##structVarName##_memory() + currentSendDTClass::NetworkVar_##structVarName::GetOffset_##varName##_memory()), sizeof(((currentSendDTClass*)0)->structVarName.varName)
#define SENDINFO_NESTEDSTRUCTELEM(structVarName, structVarName2, varName) \
	DT_VARNAME_NESTEDSTRUCTELEM(structVarName, structVarName2, varName), (currentSendDTClass::GetOffset_##structVarName##_memory() + currentSendDTClass::NetworkVar_##structVarName::GetOffset_##structVarName2##_memory() + currentSendDTClass::NetworkVar_##structVarName::NetworkVar_##structVarName2::GetOffset_##varName##_memory()), sizeof(((currentSendDTClass*)0)->structVarName.structVarName2.varName)
#define SENDINFO_STRUCTARRAYELEM(structVarName, varName, i) \
	DT_VARNAME_STRUCTELEM_ARRAYELEM(structVarName, varName, i), (currentSendDTClass::GetOffset_##structVarName##_memory() + currentSendDTClass::NetworkVar_##structVarName::GetOffset_##varName##_memory( i )), sizeof(((currentSendDTClass*)0)->structVarName.varName[i])

// Use this when you're not using a CNetworkVar to represent the data you're sending.
#define SENDINFO_NOCHECK(varName) \
	DT_VARNAME(varName), offsetof(currentSendDTClass, varName), sizeof(((currentSendDTClass*)0)->varName)
#define SENDINFO_NOCHECK_ARRAYELEM(varName, i) \
	DT_VARNAME_ARRAYELEM(varName, i), (offsetof(currentSendDTClass, varName) + (sizeof(((currentSendDTClass*)0)->varName[i]) * i)), sizeof(((currentSendDTClass*)0)->varName[i])
#define SENDINFO_NOCHECK_VECTORELEM(varName, i) \
	DT_VARNAME_VECTORELEM(varName, i), -(int)(offsetof(currentSendDTClass, varName) + (sizeof(((currentSendDTClass*)0)->varName[i]) * i)), sizeof(((currentSendDTClass*)0)->varName[i])

#define SENDINFO_STRING_NOCHECK(varName) \
	DT_VARNAME(varName), offsetof(currentSendDTClass, varName)
#define SENDINFO_DT(varName) \
	DT_VARNAME(varName), offsetof(currentSendDTClass, varName)

#define SENDINFO_DT_NAME(varName, remoteVarName) \
	DT_VARNAME(remoteVarName), offsetof(currentSendDTClass, varName)
#define SENDINFO_NAME(varName,remoteVarName) \
	DT_VARNAME(remoteVarName), offsetof(currentSendDTClass, varName), sizeof(((currentSendDTClass*)0)->varName)

#define SENDEXLCUDE(dtname, varname) \
	#dtname, DT_VARNAME(varname)
#define SENDEXLCUDE_VECTORELEM(dtname, varname, i) \
	#dtname, DT_VARNAME_VECTORELEM(varname, i)

// ------------------------------------------------------------------------ //
// Built-in proxy types.
// See the definition of SendVarProxyFn for information about these.
// ------------------------------------------------------------------------ //
void SendProxy_QAngles			( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_FloatAngle		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Float		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Vector	( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_VectorXY( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Vector2D( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Quaternion( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

void SendProxy_Bool( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UChar( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UShort( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UInt( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_UInt64( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);
void SendProxy_SChar		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Short		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Int		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Int64		( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_CString	( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Color32	( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Color32E	( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Color24	( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

void SendProxy_IntAddOne( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_ShortAddOne( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

// pData is the address of a data table.
void* SendProxy_DataTableToDataTable( const SendPropInfo *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID );

// pData is the address of a pointer to a data table.
void* SendProxy_DataTablePtrToDataTable( const SendPropInfo *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID );

// Used on player entities - only sends the data to the local player (objectID-1).
void* SendProxy_SendLocalDataTable( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );

float AssignRangeMultiplier( int nBits, double range );

#if defined GAME_DLL || defined CLIENT_DLL
#ifndef __cpp_concepts
	#error
#endif

template <typename T>
unsigned short offset_choose(unsigned short base_off)
{
	if constexpr(is_a_networkvar<T>) {
		return T::GetOffset_memory();
	} else {
		return base_off;
	}
}
#endif

#define DEFINE_SEND_FIELD( varName, ... ) \
	SendPropAuto_impl<typename NetworkVarType<decltype(currentSendDTClass::varName)>::type>( DT_VARNAME(varName), CNativeFieldInfo<decltype(currentSendDTClass::varName)>::FIELDTYPE, offset_choose<decltype(currentSendDTClass::varName)>(offsetof(currentSendDTClass, varName)) __VA_OPT__(, __VA_ARGS__) )

void SendPropAuto_impl(
	SendPropInfoEx &ret,
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int sizeofVar,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
);

template <typename T>
class CHandle;

template <typename T> struct dt_base_type_ { using type = T; };
template<> struct dt_base_type_<Vector> { using type = float; };
template<> struct dt_base_type_<QAngle> { using type = float; };
template<> struct dt_base_type_<Vector2D> { using type = float; };
template<> struct dt_base_type_<Vector4D> { using type = float; };
template<> struct dt_base_type_<Quaternion> { using type = float; };
template<> struct dt_base_type_<matrix3x4_t> { using type = float; };
template<> struct dt_base_type_<VMatrix> { using type = float; };
template<> struct dt_base_type_<CPredictableId> { using type = unsigned int; };
template<> struct dt_base_type_<CBaseHandle> { using type = unsigned long; };
template<typename U> struct dt_base_type_<CHandle<U>> { using type = unsigned long; };
template <typename T>
using dt_base_type_t = typename dt_base_type_<T>::type;

template <typename T> struct is_ehandle_ { static constexpr inline const bool value = false; };
template<> struct is_ehandle_<CBaseHandle> { static constexpr inline const bool value = true; };
template<typename U> struct is_ehandle_<CHandle<U>> { static constexpr inline const bool value = true; };
template <typename T>
constexpr inline const bool is_ehandle_v = is_ehandle_<T>::value;

#define TIME_BITS 24
#define PREDICTABLE_ID_BITS 31
#define ANGLE_BITS 13
#define DISTANCE_BITS 12
#define SCALE_BITS 8

#define ANIMATION_SEQUENCE_BITS			12	// 4096 sequences
#define ANIMATION_SKIN_BITS				10	// 1024 body skin selections FIXME: this seems way high
#define ANIMATION_BODY_BITS				32	// body combinations
#define ANIMATION_HITBOXSET_BITS		2	// hit box sets 
#define ANIMATION_POSEPARAMETER_BITS	11	// pose parameter resolution
#define ANIMATION_PLAYBACKRATE_BITS		8	// default playback rate, only used on leading edge detect sequence changes

template <typename T>
inline int DT_BitsOrTypeBits_impl(int nBits, fieldtype_t type, DTFlags_t flags)
{
	if(flags & SPROP_NOSCALE) {
		nBits = (sizeof(T) * 8);
		return nBits;
	} else if(flags & SPROP_NORMAL) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return 1;
		} else if constexpr(is_floating_point_v<dt_base_type_t<T>>) {
			return NORMAL_FRACTIONAL_BITS;
		}
	} else if(flags & SPROP_COORD_MP_LOWPRECISION) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return COORD_INTEGER_BITS_MP;
		} else if constexpr(is_floating_point_v<dt_base_type_t<T>>) {
			return (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS_MP_LOWPRECISION);
		}
	} else if(flags & (SPROP_COORD_MP|SPROP_COORD_MP_INTEGRAL)) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return COORD_INTEGER_BITS_MP;
		} else if constexpr(is_floating_point_v<dt_base_type_t<T>>) {
			return (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS);
		}
	} else if(flags & SPROP_COORD) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return COORD_INTEGER_BITS;
		} else if constexpr(is_floating_point_v<dt_base_type_t<T>>) {
			return (COORD_INTEGER_BITS + COORD_FRACTIONAL_BITS);
		}
	} else if constexpr(is_same_v<dt_base_type_t<T>, bool>) {
		return 1;
	} else if constexpr(is_same_v<T, QAngle>) {
		return ANGLE_BITS;
	} else if constexpr(is_ehandle_v<T>) {
		return NUM_NETWORKED_EHANDLE_BITS;
	} else if constexpr(is_same_v<T, CPredictableId>) {
		return PREDICTABLE_ID_BITS;
	}

	switch(type) {
	case FIELD_TIME:
		return TIME_BITS;
	case FIELD_DISTANCE:
		return DISTANCE_BITS;
	case FIELD_SCALE:
		return SCALE_BITS;
	default:
		break;
	}

	if(nBits <= 0) {
		return -2;
	} else {
		return nBits;
	}
}

template <typename T>
inline int DT_BitsOrTypeBits(int nBits, fieldtype_t type, DTFlags_t flags)
{
	nBits = DT_BitsOrTypeBits_impl<T>(nBits, type, flags);
	if(nBits == -2) {
		nBits = (sizeof(T) * 8);
	}
	return nBits;
}

template <typename T>
inline dt_base_type_t<T> DT_LowValue(fieldtype_t type, DTFlags_t flags)
{
	if(flags & SPROP_NOSCALE) {
		return static_cast<dt_base_type_t<T>>(0);
	} else if(flags & SPROP_UNSIGNED) {
		return static_cast<dt_base_type_t<T>>(0);
	} else if(flags & SPROP_NORMAL) {
		return static_cast<dt_base_type_t<T>>(-1);
	} else if(flags & SPROP_COORD_MP_INTEGRAL) {
		return static_cast<dt_base_type_t<T>>(INT_MIN);
	} else if(flags & (SPROP_COORD|SPROP_COORD_MP|SPROP_COORD_MP_LOWPRECISION)) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return static_cast<dt_base_type_t<T>>(MIN_COORD_INTEGER);
		} else {
			return static_cast<dt_base_type_t<T>>(MIN_COORD_FLOAT);
		}
	} else if constexpr(is_same_v<T, QAngle>) {
		return static_cast<dt_base_type_t<T>>(-360.0f);
	} else if constexpr(is_unsigned_v<dt_base_type_t<T>>) {
		return static_cast<dt_base_type_t<T>>(0);
	} else {
		return static_cast<dt_base_type_t<T>>(0);
	}
}

template <typename T>
inline dt_base_type_t<T> DT_HighValue(int nBits, fieldtype_t type, DTFlags_t flags)
{
	if(flags & SPROP_NOSCALE) {
		return static_cast<dt_base_type_t<T>>(0);
	} else if(flags & SPROP_NORMAL) {
		return static_cast<dt_base_type_t<T>>(1);
	} else if(flags & SPROP_COORD_MP_INTEGRAL) {
		return static_cast<dt_base_type_t<T>>(INT_MAX);
	} else if(flags & (SPROP_COORD|SPROP_COORD_MP|SPROP_COORD_MP_LOWPRECISION)) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return static_cast<dt_base_type_t<T>>(MAX_COORD_INTEGER);
		} else {
			return static_cast<dt_base_type_t<T>>(MAX_COORD_FLOAT);
		}
	} else if constexpr(is_same_v<T, QAngle>) {
		return static_cast<dt_base_type_t<T>>(360.0f);
	}

	switch(type) {
	case FIELD_DISTANCE:
		return MAX_TRACE_LENGTH;
	case FIELD_SCALE:
		return 1.0f;
	default:
		break;
	}

	return static_cast<dt_base_type_t<T>>(1 << DT_BitsOrTypeBits<T>(nBits, type, flags));
}

template <typename T>
inline int DT_BitsOrTypeBits(int nBits, fieldtype_t type, DTFlags_t flags, dt_base_type_t<T> HighValue)
{
	nBits = DT_BitsOrTypeBits_impl<T>(nBits, type, flags);
	if(nBits == -2) {
		if constexpr(is_integral_v<dt_base_type_t<T>>) {
			return UTIL_CountNumBitsSet(HighValue);
		} else {
			nBits = (sizeof(T) * 8);
		}
	}
	return nBits;
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	SendPropInfoEx ret;

	int tmpBits = DT_BitsOrTypeBits<T>(nBits, type, flags, HighValue);

	dt_base_type_t<T> preRoundLowValue = LowValue;
	dt_base_type_t<T> preRoundHighValue = HighValue;

	if(flags & SPROP_ROUNDDOWN) {
		HighValue = preRoundHighValue - ((preRoundHighValue - preRoundLowValue) / (1 << tmpBits));
	}

	if(flags & SPROP_ROUNDUP) {
		LowValue = preRoundLowValue + ((preRoundHighValue - preRoundLowValue) / (1 << tmpBits));
	}

	if(LowValue == static_cast<dt_base_type_t<T>>(-1) && HighValue == static_cast<dt_base_type_t<T>>(1)) {
		flags |= SPROP_NORMAL;
	}

	if(LowValue == static_cast<dt_base_type_t<T>>(0) || LowValue > static_cast<dt_base_type_t<T>>(0)) {
		flags |= SPROP_UNSIGNED;
	}

	SendPropAuto_impl(
		ret,
		pVarName,
		type,
		offset,
		sizeof(T),
		nBits,
		flags,
		varProxy,
		priority
	);

	ret.m_fLowValue = static_cast<float>(LowValue);
	ret.m_fHighValue = static_cast<float>(HighValue);
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, static_cast<double>( HighValue - LowValue ) );

	return ret;
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue,
	DTFlags_t flags,
	SendVarProxyFn varProxy
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		HighValue,
		flags,
		varProxy,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		HighValue,
		flags,
		NULL
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		HighValue,
		SPROP_NONE
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		0,
		LowValue,
		HighValue,
		flags
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	dt_base_type_t<T> LowValue,
	dt_base_type_t<T> HighValue
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		0,
		LowValue,
		HighValue,
		SPROP_NONE
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		DT_HighValue<T>(nBits, type),
		flags,
		varProxy,
		priority
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		flags,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	dt_base_type_t<T> LowValue,
	dt_highdefault_t,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		LowValue,
		DT_HighValue<T>(nBits, type),
		flags,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		DT_LowValue<T>(type),
		DT_HighValue<T>(nBits, type),
		flags,
		varProxy,
		priority
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	DTFlags_t flags,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		flags,
		NULL,
		priority
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		SPROP_NONE,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		flags,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		SPROP_NONE,
		NULL,
		priority
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		nBits,
		flags,
		varProxy,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	SendPropInfoEx ret;

	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 0.0f;
	ret.m_fHighLowMul = 0.0f;

	SendPropAuto_impl(
		ret,
		pVarName,
		type,
		offset,
		sizeof(T),
		0,
		flags|SPROP_NOSCALE,
		varProxy,
		priority
	);

	return ret;
}

template <>
inline SendPropInfoEx SendPropAuto_impl<QAngle>(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<QAngle>(
		pVarName,
		type,
		offset,
		0,
		-360.0f,
		360.0f,
		flags,
		varProxy ? varProxy : SendProxy_QAngles,
		priority
	);
}

template <>
inline SendPropInfoEx SendPropAuto_impl<bool>(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<bool>(
		pVarName,
		type,
		offset,
		1,
		false,
		true,
		flags|SPROP_UNSIGNED,
		varProxy ? varProxy : SendProxy_Bool,
		priority
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	DTFlags_t flags
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		flags,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		SPROP_NONE,
		NULL,
		SENDPROP_DEFAULT_PRIORITY
	);
}

template <typename T>
SendPropInfoEx SendPropAuto_impl(
	const char *pVarName,
	fieldtype_t type,
	int offset,
	DTPriority_t priority
)
{
	return SendPropAuto_impl<T>(
		pVarName,
		type,
		offset,
		SPROP_NONE,
		NULL,
		priority
	);
}

// ------------------------------------------------------------------------ //
// Use these functions to setup your data tables.
// ------------------------------------------------------------------------ //
SendPropInfoEx SendPropFloat(
	const char *pVarName,		// Variable name.
	int offset,					// Offset into container structure.
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,				// Number of bits to use when encoding.
	DTFlags_t flags=SPROP_NONE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_Float,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropVector(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	DTFlags_t flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_Vector,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropVectorXY(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	DTFlags_t flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_VectorXY,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropQuaternion(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	DTFlags_t flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_Quaternion,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	DTFlags_t flags=SPROP_NONE,
	SendVarProxyFn varProxy=SendProxy_FloatAngle,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropQAngles(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	DTFlags_t flags=SPROP_NONE,
	SendVarProxyFn varProxy=SendProxy_QAngles,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropInt(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	int nBits=-1,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	DTFlags_t flags=SPROP_NONE,
	SendVarProxyFn varProxy=0,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropColor32(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropColor32E(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropColor24(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar,	// Handled by SENDINFO macro.
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY );

SendPropInfoEx SendPropIntWithMinusOneFlag(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int bits=-1,
	SendVarProxyFn proxyFn=SendProxy_IntAddOne );

SendPropInfoEx SendPropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	DTFlags_t flags=SPROP_NONE,
	SendVarProxyFn varProxy=SendProxy_CString,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY);

// The data table encoder looks at DVariant::m_pData.
SendPropInfoEx SendPropDataTable(
	const char *pVarName,
	int offset,
	SendTableInfo *pTable, 
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

SendPropInfoEx SendPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	SendPropInfoEx pArrayProp,
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);

//
// Only use this if you change the array infrequently and you usually change all the elements
// when you do change it.
//
// The array data is all sent together, so even if you only change one element,
// it'll send all the elements.
//
// The upside is that it doesn't need extra bits to encode indices of all the variables.
//
#define SendPropArray_AllAtOnce( arrayName, propDefinition ) \
	SendPropArray( propDefinition, arrayName )


//
// This is what you should use for most arrays. It will generate a separate SendProp for each array element
// in your array. That way, if you change one element of the array, it will only transmit that array element
// and not the whole array.
//
// Example: 
//
//     GenerateSendPropsForArrayElements( m_Array, SendPropInt( SENDINFO_ARRAY( m_Array ) )
//
#define SendPropArray_UniqueElements( arrayName, propDefinition ) \
	SendPropArray3( SENDINFO_ARRAY3( arrayName ), propDefinition )

// Use the macro to let it automatically generate a table name. You shouldn't 
// ever need to reference the table name. If you want to exclude this array, then
// reference the name of the variable in varTemplate.
SendPropInfoEx InternalSendPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthSendProxyFn proxy,
	DTPriority_t priority = SENDPROP_DEFAULT_PRIORITY
	);


// Use this and pass the array name and it will figure out the count and stride automatically.
#define SendPropArray( varTemplate, arrayName )			\
	SendPropVariableLengthArray(						\
		0,												\
		varTemplate,									\
		arrayName )

//
// Use this when you want to send a variable-length array of data but there is no physical array you can point it at.
// You need to provide:
// 1. A proxy function that returns the current length of the array.
// 2. The maximum length the array will ever be.
// 2. A SendProp describing what the elements are comprised of.
// 3. In the SendProp, you'll want to specify a proxy function so you can go grab the data from wherever it is.
// 4. A property name that matches the definition on the client.
//
#define SendPropVirtualArray( arrayLengthSendProxy, maxArrayLength, varTemplate, propertyName )	\
	varTemplate,										\
	InternalSendPropArray(								\
		maxArrayLength,									\
		0,												\
		#propertyName,									\
		arrayLengthSendProxy							\
		)


#define SendPropVariableLengthArray( arrayLengthSendProxy, varTemplate, arrayName )	\
	varTemplate,										\
	InternalSendPropArray(								\
		sizeof(((currentSendDTClass*)0)->arrayName) / PROPSIZEOF(currentSendDTClass, arrayName[0]), \
		PROPSIZEOF(currentSendDTClass, arrayName[0]),	\
		#arrayName,										\
		arrayLengthSendProxy							\
		)

// Use this one to specify the element count and stride manually.
#define SendPropArray2( arrayLengthSendProxy, varTemplate, elementCount, elementStride, arrayName )		\
	varTemplate,																	\
	InternalSendPropArray( elementCount, elementStride, #arrayName, arrayLengthSendProxy )


	

// Use these to create properties that exclude other properties. This is useful if you want to use most of 
// a base class's datatable data, but you want to override some of its variables.
SendPropInfoEx SendPropExclude(
	const char *pDataTableName,	// Data table name (given to BEGIN_SEND_TABLE and BEGIN_RECV_TABLE).
	const char *pPropName		// Name of the property to exclude.
	);


#endif // DATATABLE_SEND_H
