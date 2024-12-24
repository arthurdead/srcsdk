//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "utlrbtree.h"
#include "ai_goalentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// CAI_GoalEntity implementation
//

BEGIN_MAPENTITY( CAI_GoalEntity, MAPENT_POINTCLASS )

	DEFINE_KEYFIELD_AUTO( m_iszActor, "Actor" ),
	DEFINE_KEYFIELD_AUTO( m_iszGoal, "Goal" ),
	DEFINE_KEYFIELD_AUTO( m_fStartActive, "StartActive" ),
	DEFINE_KEYFIELD_AUTO( m_iszConceptModifiers, "BaseConceptModifiers" ),
	DEFINE_KEYFIELD_AUTO( m_SearchType, "SearchType" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", 		InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UpdateActors",	InputUpdateActors ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate",		InputDeactivate ),

END_MAPENTITY()


//-------------------------------------

void CAI_GoalEntity::Spawn()
{
	SetContextThink( &CAI_GoalEntity::DelayedRefresh, gpGlobals->curtime + 0.1f, "Refresh" );
}


//-------------------------------------

void CAI_GoalEntity::DelayedRefresh()
{
	inputdata_t ignored;
	if ( m_fStartActive )
	{
		Assert( !(m_flags & ACTIVE) );
		InputActivate( Move(ignored) );
		m_fStartActive = false;
	}
	else
		InputUpdateActors( Move(ignored) );
	
	SetContextThink( NULL, 0, "Refresh" );
}

//-------------------------------------

void CAI_GoalEntity::PruneActors()
{
	for ( int i = m_actors.Count() - 1; i >= 0; i-- )
	{
		if ( m_actors[i] == NULL || m_actors[i]->IsMarkedForDeletion() || m_actors[i]->GetState() == NPC_STATE_DEAD )
			m_actors.FastRemove( i );
	}
}

//-------------------------------------

void CAI_GoalEntity::ResolveNames()
{
	m_actors.SetCount( 0 );
	
	CBaseEntity *pEntity = NULL;
	for (;;)
	{
		switch ( m_SearchType )
		{
			case ST_ENTNAME:
			{
				pEntity = gEntList.FindEntityByName( pEntity, m_iszActor );
				break;
			}
			
			case ST_CLASSNAME:
			{
				pEntity = gEntList.FindEntityByClassname( pEntity, STRING( m_iszActor ) );
				break;
			}
		}
		
		if ( !pEntity )
			break;
			
		CAI_BaseNPC *pActor = pEntity->MyNPCPointer();
		
		if ( pActor  && pActor->GetState() != NPC_STATE_DEAD )
		{
			AIHANDLE temp;
			temp = pActor;
			m_actors.AddToTail( temp );
		}
	}
		
	m_hGoalEntity = gEntList.FindEntityByName( NULL, m_iszGoal );
}

//-------------------------------------

void CAI_GoalEntity::InputActivate( inputdata_t &&inputdata )
{
	if ( !( m_flags & ACTIVE ) )
	{
		OnActivate();

		gEntList.AddListenerEntity( this );
		
		UpdateActors();
		m_flags |= ACTIVE;
		
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			EnableGoal( m_actors[i].Get() );
		}
	}
}

//-------------------------------------

void CAI_GoalEntity::InputUpdateActors( inputdata_t &&inputdata )
{
	int i;
	CUtlRBTree<CAI_BaseNPC *> prevActors;
	CUtlRBTree<CAI_BaseNPC *>::IndexType_t index;

	SetDefLessFunc( prevActors );
	
	PruneActors();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		prevActors.Insert( m_actors[i].Get() );
	}
	
	ResolveNames();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		index = prevActors.Find( m_actors[i].Get() );
		if ( index == prevActors.InvalidIndex() )
		{
			if ( m_flags & ACTIVE )
				EnableGoal( m_actors[i].Get() );
		}
		else
			prevActors.Remove( m_actors[i].Get() );
	}
	
	for ( index = prevActors.FirstInorder(); index != prevActors.InvalidIndex(); index = prevActors.NextInorder( index ) )
	{
		if ( m_flags & ACTIVE )
			DisableGoal( prevActors[ index ] );
	}
}

//-------------------------------------

void CAI_GoalEntity::InputDeactivate( inputdata_t &&inputdata ) 	
{
	if ( m_flags & ACTIVE )
	{
		OnDeactivate();

		gEntList.RemoveListenerEntity( this );
		UpdateActors();
		m_flags &= ~ACTIVE;

		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			DisableGoal( m_actors[i].Get() );
		}		
	}
}

//-------------------------------------

void CAI_GoalEntity::EnterDormant( void )
{
	if ( m_flags & ACTIVE )
	{
		m_flags |= DORMANT;
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			DisableGoal( m_actors[i].Get() );
		}
	}
}

//-------------------------------------

void CAI_GoalEntity::ExitDormant( void )
{
	if ( m_flags & DORMANT )
	{
		m_flags &= ~DORMANT;

		inputdata_t ignored;
		InputUpdateActors( Move(ignored) );
	}
}

//-------------------------------------

void CAI_GoalEntity::UpdateOnRemove()
{
	if ( m_flags & ACTIVE )
	{
		inputdata_t inputdata;
		InputDeactivate( Move(inputdata) );
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

void CAI_GoalEntity::OnEntityCreated( CBaseEntity *pEntity )
{
	Assert( m_flags & ACTIVE );
	
	if ( pEntity->MyNPCPointer() )
	{
		SetContextThink( &CAI_GoalEntity::DelayedRefresh, gpGlobals->curtime + 0.1f, "Refresh" );
	}
	
}

//-------------------------------------

void CAI_GoalEntity::OnEntityDeleted( CBaseEntity *pEntity )
{
	Assert( pEntity != this );
}

//-----------------------------------------------------------------------------

int CAI_GoalEntity::DrawDebugTextOverlays()
{
	char tempstr[512];
	int offset = BaseClass::DrawDebugTextOverlays();

	Q_snprintf( tempstr, sizeof(tempstr), "Active: %s", IsActive() ? "yes" : "no" );
	EntityText( offset, tempstr, 0 );
	offset++;
		
	return offset;
}


