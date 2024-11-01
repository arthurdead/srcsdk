//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef SERVER_CLASS_H
#define SERVER_CLASS_H

#pragma once

#include "tier0/dbg.h"
#include "dt_send.h"
#include "networkstringtabledefs.h"
#include "networkvar.h"

class ServerClass;
class SendTable;

extern ServerClass *g_pServerClassHead;


class ServerClass
{
public:
	ServerClass( const char *pNetworkName, SendTable *pTable );

	const char*	GetName()		{ return m_pNetworkName; }

public:
	const char					*m_pNetworkName;
	SendTable					*m_pTable;
	ServerClass					*m_pNext;
	int							m_ClassID;	// Managed by the engine.

	// This is an index into the network string table (sv.GetInstanceBaselineTable()).
	int							m_InstanceBaselineIndex; // INVALID_STRING_INDEX if not initialized yet.
};


class CBaseNetworkable;

// If you do a DECLARE_SERVERCLASS, you need to do this inside the class definition.
#define DECLARE_SERVERCLASS()									\
	public:														\
		virtual ServerClass* GetServerClass();					\
		static SendTable *m_pClassSendTable;					\
		template <typename T> friend int ServerClassInit(T *);	\
		virtual int YouForgotToImplementOrDeclareServerClass();	\

#define DECLARE_SERVERCLASS_NOBASE()							\
	public:														\
		template <typename T> friend int ServerClassInit(T *);	\

// Use this macro to expose your class's data across the network.
#define IMPLEMENT_SERVERCLASS( DLLClassName, sendTable ) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )

// You can use this instead of BEGIN_SEND_TABLE and it will do a DECLARE_SERVERCLASS automatically.
#define IMPLEMENT_SERVERCLASS_ST(DLLClassName, sendTable) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )\
	BEGIN_SEND_TABLE(DLLClassName, sendTable)

#define IMPLEMENT_SERVERCLASS_ST_NOBASE(DLLClassName, sendTable) \
	IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable )\
	BEGIN_SEND_TABLE_NOBASE( DLLClassName, sendTable )

#define IMPLEMENT_SERVERCLASS_INTERNAL( DLLClassName, sendTable ) \
	namespace sendTable \
	{ \
		struct ignored; \
		extern SendTable g_SendTable; \
	} \
	CHECK_DECLARE_CLASS_IMPL( DLLClassName, sendTable ) \
	static ServerClass V_CONCAT3(g_, DLLClassName, _ClassReg)(\
		V_STRINGIFY(DLLClassName), \
		&sendTable::g_SendTable\
	); \
	\
	ServerClass* DLLClassName::GetServerClass() { \
		if(!IsNetworked()) \
			return NULL; \
		return &V_CONCAT3(g_, DLLClassName, _ClassReg); \
	} \
	SendTable *DLLClassName::m_pClassSendTable = &sendTable::g_SendTable;\
	int DLLClassName::YouForgotToImplementOrDeclareServerClass() {return 0;}


#endif



