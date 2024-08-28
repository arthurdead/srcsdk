//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_gib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//NOTENOTE: This is not yet coupled with the server-side implementation of CGib
//			This is only a client-side version of gibs at the moment

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_Gib::~C_Gib( void )
{
	VPhysicsDestroyObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszModelName - 
//			vecOrigin - 
//			vecForceDir - 
//			vecAngularImp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
C_Gib *C_Gib::CreateClientsideGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp, float flLifetime )
{
	C_Gib *pGib = new C_Gib;
	pGib->SetClassname("gib");
	if ( pGib->InitializeGib( pszModelName, vecOrigin, vecForceDir, vecAngularImp, flLifetime ) == false )
		return NULL;

	return pGib;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszModelName - 
//			vecOrigin - 
//			vecForceDir - 
//			vecAngularImp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_Gib::InitializeGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp, float flLifetime )
{
	if ( InitializeAsClientEntity() == false )
	{
		UTIL_Remove( this );
		return false;
	}

	SetModel( pszModelName );

	SetAbsOrigin( vecOrigin );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	
	m_pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );
	
	if ( m_pPhysicsObject )
	{
		float flForce = m_pPhysicsObject->GetMass();
		vecForceDir *= flForce;	

		m_pPhysicsObject->ApplyForceOffset( vecForceDir, GetAbsOrigin() );
		m_pPhysicsObject->SetCallbackFlags( m_pPhysicsObject->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC );
	}
	else
	{
		// failed to create a physics object
		UTIL_Remove( this );
		return false;
	}

	SetContextThink( &C_Gib::FadeThink, gpGlobals->curtime + flLifetime, "FadeThink" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Gib::FadeThink( void )
{
	SetRenderMode( kRenderTransAlpha );
	SetRenderFX( kRenderFxFadeFast );

	if ( GetRenderAlpha() == 0 )
	{
#ifdef HL2_CLIENT_DLL
		s_AntlionGibManager.RemoveGib( this );
#endif
		UTIL_Remove( this );
		return;
	}

	SetNextThink( gpGlobals->curtime + 1.0f, "FadeThink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void C_Gib::StartTouch( C_BaseEntity *pOther )
{
	// Limit the amount of times we can bounce
	if ( m_flTouchDelta < gpGlobals->curtime )
	{
		HitSurface( pOther );
		m_flTouchDelta = gpGlobals->curtime + 0.1f;
	}

	BaseClass::StartTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void C_Gib::HitSurface( C_BaseEntity *pOther )
{
	//TODO: Implement splatter or effects in child versions
}
