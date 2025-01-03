//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASETEMPENTITY_H
#define C_BASETEMPENTITY_H
#pragma once


#include "client_class.h"
#include "iclientnetworkable.h"
#include "c_recipientfilter.h"


//-----------------------------------------------------------------------------
// Purpose: Base class for TEs.  All TEs should derive from this and at
//  least implement OnDataChanged to be notified when the TE has been received
//  from the server
//-----------------------------------------------------------------------------
class C_BaseTempEntity : public IClientUnknown, public IClientNetworkable

{
public:
	DECLARE_CLASS_NOBASE( C_BaseTempEntity );
	DECLARE_CLIENTCLASS();
	
									C_BaseTempEntity( void );
	virtual							~C_BaseTempEntity( void );

	bool IsNetworked( void ) const { return true; }

// IClientUnknown implementation.
public:

	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ return NULL_BASEHANDLE; }

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetCollideable()		{ return NULL; }
	virtual IClientNetworkable*	GetClientNetworkable()	{ return this; }
	virtual IClientRenderable*	GetClientRenderable()	{ return NULL; }
	virtual IClientEntity*		GetIClientEntity()		{ return NULL; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return NULL; }
	virtual IClientThinkable*	GetClientThinkable()	{ return NULL; }
	virtual IClientModelRenderable*	GetClientModelRenderable()	{ return NULL; }
	virtual IClientAlphaProperty*	GetClientAlphaProperty()	{ return NULL; }


// IClientNetworkable overrides.
private:

	virtual void					DO_NOT_USE_Release() final;	

public:
	virtual void					NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void					PreDataUpdate( DataUpdateType_t updateType );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );
	virtual void					OnDataUnchangedInPVS( void ) { }
	virtual void					OnPreDataChanged( DataUpdateType_t updateType );
	virtual void					OnDataChanged( DataUpdateType_t updateType );
	virtual void					SetDormant( bool bDormant );
	virtual bool					IsDormant( void );
	virtual int						entindex( void ) const;
	virtual void					ReceiveMessage( int classID, bf_read &msg );
	virtual void*					GetDataTableBasePtr();
	virtual void					SetDestroyedOnRecreateEntities( void );

public:

	// Dummy for CNetworkVars.
	void NetworkStateChanged() {}
	void NetworkStateChanged( void *pVar ) {}

	bool InitializeAsEventEntity();

	bool					PostConstructor( const char *classname );

	virtual void					Precache( void );

	C_BaseTempEntity				*GetNext( void );

	// Get list of tempentities
	static C_BaseTempEntity			*GetList( void );

	// Determine the color modulation amount
	void	GetColorModulation( float* color )
	{
		Assert(color);
		color[0] = color[1] = color[2] = 1.0f;
	}

// Static members
public:
	// Called at startup to allow temp entities to precache any models/sounds that they need
	static void						PrecacheTempEnts( void );

private:

	// Next in chain
	C_BaseTempEntity		*m_pNext;

	// TEs add themselves to this list for the executable.
	static C_BaseTempEntity	*s_pTempEntities;
};

void UTIL_Remove( C_BaseTempEntity *pEntity ) = delete;
void UTIL_RemoveImmediate( C_BaseTempEntity *pEntity ) = delete;


#endif // C_BASETEMPENTITY_H
