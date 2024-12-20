//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <stdarg.h>
#include "engine/IEngineSound.h"
#include "filesystem.h"
#include "igamemovement.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "c_world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CMoveData *g_pMoveData;

class C_MoveHelper : public IMoveHelper
{
public:
	C_MoveHelper( void );
	virtual	 ~C_MoveHelper( void );

	char const*		GetName( EntityHandle_t handle ) const;

	// touch lists
	virtual void	ResetTouchList( void );
	virtual bool	AddToTouched( const trace_t& tr, const Vector& impactvelocity );
	virtual void	ProcessImpacts( void );

	// Numbered line printf
	virtual void	Con_NPrintf( int idx, char const* fmt, ... );
		
	virtual bool	PlayerFallingDamage(void);

	// These have separate server vs client impementations
	virtual void	StartSound( const Vector& origin, int channel, char const* sample, float volume, soundlevel_t soundlevel, int fFlags, int pitch );
	virtual void	StartSound( const Vector& origin, const char *soundname ); 
	virtual void	PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
	virtual IPhysicsSurfaceProps *GetSurfaceProps( void );

	virtual bool IsWorldEntity( const EHANDLE &handle );

	void			SetHost( C_BasePlayer *host );

private:
	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	// of shoving boxes etc. around.
	struct touchlist_t
	{
		Vector	deltavelocity;
		trace_t trace;

		touchlist_t() {}

	private:
		touchlist_t( const touchlist_t &src );
	};

	CUtlVector<touchlist_t>			m_TouchList;

	C_BaseEntity*	m_pHost;
};	

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------

IMPLEMENT_MOVEHELPER();

static C_MoveHelper s_MoveHelperClient;


//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
C_MoveHelper::C_MoveHelper( void )
{
	m_pHost = NULL;
	SetSingleton( this );
}

C_MoveHelper::~C_MoveHelper( void )
{
	SetSingleton( NULL );
}

//-----------------------------------------------------------------------------
// Indicates which entity we're going to move
//-----------------------------------------------------------------------------

void C_MoveHelper::SetHost( C_BasePlayer *host )
{
	m_pHost = host;

	// In case any stuff is ever left over, sigh...
	ResetTouchList();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
char const* C_MoveHelper::GetName( EntityHandle_t handle ) const
{
	return "";
}

//-----------------------------------------------------------------------------
// Touch list
//-----------------------------------------------------------------------------

void C_MoveHelper::ResetTouchList( void )
{
	m_TouchList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Adds to the touched list 
//-----------------------------------------------------------------------------

bool C_MoveHelper::AddToTouched( const trace_t& tr, const Vector& impactvelocity )
{
	int i;

	// Look for duplicates
	for (i = 0; i < m_TouchList.Size(); i++)
	{
		if (m_TouchList[i].trace.m_pEnt == tr.m_pEnt)
		{
			return false;
		}
	}

	i = m_TouchList.AddToTail();
	m_TouchList[i].trace = tr;
	VectorCopy( impactvelocity, m_TouchList[i].deltavelocity );

	return true;
}

void C_MoveHelper::ProcessImpacts( void )
{
	// Relink in order to build absorigin and absmin/max to reflect any changes
	//  from prediction.  Relink will early out on SOLID_NOT

	// TODO: Touch triggers on the client
	m_pHost->PhysicsTouchTriggers();

	// Don't bother if the player ain't solid
	if ( m_pHost->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return;

	// Save off the velocity, cause we need to temporarily reset it
	Vector vel = m_pHost->GetAbsVelocity();

	// Touch other objects that were intersected during the movement.
	for (int i = 0 ; i < m_TouchList.Size(); i++)
	{
		// Run the impact function as if we had run it during movement.
		C_BaseEntity *entity = ClientEntityList().GetEnt( m_TouchList[i].trace.m_pEnt->entindex() );
		if ( !entity )
			continue;

		Assert( entity != m_pHost );
		// Don't ever collide with self!!!!
		if ( entity == m_pHost )
			continue;

		// Reconstruct trace results.
		m_TouchList[i].trace.m_pEnt = entity;

		// Use the velocity we had when we collided, so boxes will move, etc.
		m_pHost->SetAbsVelocity( m_TouchList[i].deltavelocity );

		entity->PhysicsImpact( m_pHost, m_TouchList[i].trace );
	}

	// Restore the velocity
	m_pHost->SetAbsVelocity( vel );

	// So no stuff is ever left over, sigh...
	ResetTouchList();
}

void C_MoveHelper::StartSound( const Vector& origin, const char *soundname )
{
	if ( !soundname )
		return;

	CLocalPlayerFilter filter;
	filter.UsePredictionRules();
	C_BaseEntity::EmitSound( filter, m_pHost->entindex(), soundname, &origin );
}


//-----------------------------------------------------------------------------
// Play a sound
//-----------------------------------------------------------------------------

void C_MoveHelper::StartSound( const Vector& origin, int channel, 
	char const* pSample, float volume, soundlevel_t soundlevel, int fFlags, int pitch )
{
	if ( pSample )
	{
		C_BaseEntity::PrecacheScriptSound( pSample );
		CLocalPlayerFilter filter;
		filter.UsePredictionRules();

		EmitSound_t ep;
		ep.m_nChannel = channel;
		ep.m_pSoundName =  pSample;
		ep.m_flVolume = volume;
		ep.m_SoundLevel = soundlevel;
		ep.m_nPitch = pitch;
		ep.m_pOrigin = &origin;

		C_BaseEntity::EmitSound( filter, m_pHost->entindex(), ep );
	}
}

//-----------------------------------------------------------------------------
// Play a event
//-----------------------------------------------------------------------------

void C_MoveHelper::PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, Vector& origin, Vector& angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	// TODO
	if (g_pMoveData->m_bFirstRunOfFunctions )
	{
	}
}

//-----------------------------------------------------------------------------
// Surface properties interface
//-----------------------------------------------------------------------------

IPhysicsSurfaceProps *C_MoveHelper::GetSurfaceProps( void )
{
	extern IPhysicsSurfaceProps *physprops;
	return physprops;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bDeveloper - 
//			*pFormat - 
//			... - 
//-----------------------------------------------------------------------------
void C_MoveHelper::Con_NPrintf( int idx, char const* pFormat, ...)
{
	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof( msg ), pFormat, marker);
	va_end(marker);
	
	engine->Con_NPrintf( idx, "%s", msg );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the player falls onto a surface fast enough to take
//			damage, according to the rules in CGameMovement::CheckFalling.
// Output : Returns true if the player survived the fall, false if they died.
//-----------------------------------------------------------------------------
bool C_MoveHelper::PlayerFallingDamage(void)
{
	// Do nothing; falling damage is applied in MoveHelper_Server::PlayerFallingDamage.
	return(true);
}

bool C_MoveHelper::IsWorldEntity( const EHANDLE &handle )
{
	if(!GetClientWorldEntity())
		return false;

	return handle == GetClientWorldEntity()->GetRefEHandle();
}