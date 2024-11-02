//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Core Temp Entity client implementation.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_NULL(C_BaseTempEntity, DT_BaseTempEntity, CBaseTempEntity);

BEGIN_RECV_TABLE_NOBASE(C_BaseTempEntity, DT_BaseTempEntity)
END_RECV_TABLE()


// Global list of temp entity classes
C_BaseTempEntity *C_BaseTempEntity::s_pTempEntities = NULL;

//-----------------------------------------------------------------------------
// Purpose: Returns head of list
// Output : CBaseTempEntity * -- head of list
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetList( void )
{
	return s_pTempEntities;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_BaseTempEntity::C_BaseTempEntity( void )
{
	// Add to list
	m_pNext			= s_pTempEntities;
	s_pTempEntities = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
C_BaseTempEntity::~C_BaseTempEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get next temp ent in chain
// Output : CBaseTempEntity *
//-----------------------------------------------------------------------------
C_BaseTempEntity *C_BaseTempEntity::GetNext( void )
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTempEntity::Precache( void )
{
	// Nothing...
}

//-----------------------------------------------------------------------------
// Purpose: Called at startup to allow temp entities to precache any models/sounds that they need
//-----------------------------------------------------------------------------
void C_BaseTempEntity::PrecacheTempEnts( void )
{
	C_BaseTempEntity *te = GetList();
	while ( te )
	{
		te->Precache();
		te = te->GetNext();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dynamic/non-singleton temp entities are initialized by
//  calling into here.  They should be added to a list of C_BaseTempEntities so
//  that their memory can be deallocated appropriately.
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool					C_BaseTempEntity::PostConstructor( const char *classname )
{
	return true;
}

bool C_BaseTempEntity::InitializeAsEventEntity()
{
	return true;
}

void C_BaseTempEntity::DO_NOT_USE_Release()
{
	Assert( !"C_BaseTempEntity::Release should never be called" );
}


void C_BaseTempEntity::NotifyShouldTransmit( ShouldTransmitState_t state )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_BaseTempEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	// TE's may or may not implement this
}


int C_BaseTempEntity::entindex( void ) const { Assert( 0 ); return -1; }
void C_BaseTempEntity::PostDataUpdate( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::OnPreDataChanged( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::OnDataChanged( DataUpdateType_t updateType ) { Assert( 0 ); }
void C_BaseTempEntity::SetDormant( bool bDormant ) { Assert( 0 ); }
bool C_BaseTempEntity::IsDormant( void ) { Assert( 0 ); return false; };
void C_BaseTempEntity::ReceiveMessage( int classID, bf_read &msg ) { Assert( 0 ); }
void C_BaseTempEntity::SetDestroyedOnRecreateEntities( void ) { Assert(0); }

void* C_BaseTempEntity::GetDataTableBasePtr()
{
	return this;
}
