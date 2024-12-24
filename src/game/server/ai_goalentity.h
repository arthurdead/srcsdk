//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_GOALENTITY_H
#define AI_GOALENTITY_H
#pragma once

#include "ai_basenpc.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
//
// CAI_GoalEntity
//
// Purpose: Serves as the base class for all entities the designer may place
// 			that establish an NPC goal. Provides standard input, output &
//			fields common to all goals.
//

class CAI_GoalEntity : public CBaseEntity,
					   public IEntityListener
{
public:
	enum Flags_t : unsigned char
	{
		NO_FLAGS,
		ACTIVE			= 0x01,
		RESOLVED_NAME 	= 0x02,
		DORMANT			= 0x04,
	};
	
	enum SearchType_t : unsigned char
	{
		ST_ENTNAME,
		ST_CLASSNAME,
	};

	DECLARE_CLASS( CAI_GoalEntity, CBaseEntity );
	CAI_GoalEntity()
	 :	m_iszActor(NULL_STRING),
	 	m_iszGoal(NULL_STRING),
	 	m_fStartActive(false),
	 	m_SearchType(ST_ENTNAME),
	 	m_iszConceptModifiers(NULL_STRING),
	 	m_hGoalEntity(NULL),
	 	m_flags( NO_FLAGS )
	{
	}

	virtual EntityCaps_t ObjectCaps()	{ return ((BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_NOTIFY_ON_TRANSITION); }
	
	virtual void	Spawn();
	virtual int		DrawDebugTextOverlays();

	virtual void OnActivate() {}
	virtual void OnDeactivate() {}
	
	virtual void 	InputActivate( inputdata_t &&inputdata );
	virtual void 	InputUpdateActors( inputdata_t &&inputdata );
	virtual void 	InputDeactivate( inputdata_t &&inputdata );
	
	// Goal entities can become Dormant if they're left behind on previous maps.
	// Transitioning back to the map with cause a dormant goal entity to reactivate itself.
	void			EnterDormant( void );
	void			ExitDormant( void );

	bool 			IsActive();
	
	int 			NumActors();
	CAI_BaseNPC *	GetActor( int iActor = 0 );

	void			SetGoalEntity( CBaseEntity *pGoalEntity );
	CBaseEntity *	GetGoalEntity();
	const char *	GetGoalEntityName();

	const char *	GetConceptModifiers();

protected:
	virtual void	UpdateOnRemove();

	virtual void 	OnEntityCreated( CBaseEntity *pEntity );
	virtual void 	OnEntityDeleted( CBaseEntity *pEntity );

	virtual void	EnableGoal( CAI_BaseNPC *pAI )	{}
	virtual void	DisableGoal( CAI_BaseNPC *pAI  ) {}
	
	void UpdateActors();

	const CUtlVector<AIHANDLE> &AccessActors()
	{
		return m_actors;
	}
	
private:
	void DelayedRefresh();
	void PruneActors();
	virtual void ResolveNames();
	
	// From Worldcraft
	string_t				m_iszActor;
	string_t 				m_iszGoal;
	bool					m_fStartActive;
	SearchType_t			m_SearchType;
	string_t				m_iszConceptModifiers;
	
	CUtlVector<AIHANDLE>	m_actors;
	EHANDLE					m_hGoalEntity;
	Flags_t 				m_flags;
	
	
protected:
	DECLARE_MAPENTITY();
};

FLAGENUM_OPERATORS( CAI_GoalEntity::Flags_t, unsigned char )

DECLARE_FIELD_ENUM( CAI_GoalEntity::SearchType_t )

//-------------------------------------

// @TODO (toml 03-18-03): Efficiency wart -- make this an explicit duty of the client?
inline void CAI_GoalEntity::UpdateActors()
{
	if ( !( m_flags & ACTIVE ) || !( m_flags & RESOLVED_NAME ) )
	{
		ResolveNames();
		m_flags |= RESOLVED_NAME;
	}
	else
		PruneActors();
}

//-------------------------------------

inline bool CAI_GoalEntity::IsActive()
{
	if ( m_flags & ACTIVE )
	{
		UpdateActors();
		return ( m_actors.Count() != 0 );
	}
	return false;
}

//-------------------------------------

inline int CAI_GoalEntity::NumActors()
{
	UpdateActors();
	return m_actors.Count();
}
	
//-------------------------------------

inline CAI_BaseNPC *CAI_GoalEntity::GetActor( int iActor )
{
	UpdateActors();
	if (  m_actors.Count() > iActor )
		return m_actors[iActor].Get();
	return NULL;
}

//-------------------------------------

inline void CAI_GoalEntity::SetGoalEntity( CBaseEntity *pGoalEntity )
{
	m_iszGoal = pGoalEntity->GetEntityName();
	m_hGoalEntity = pGoalEntity;
}

//-------------------------------------

inline CBaseEntity *CAI_GoalEntity::GetGoalEntity()
{
	UpdateActors();
	return m_hGoalEntity.Get();
}

//-------------------------------------

inline const char *CAI_GoalEntity::GetGoalEntityName()
{
	return STRING( m_iszGoal );
}

//-------------------------------------

inline const char *CAI_GoalEntity::GetConceptModifiers()
{
	return STRING( m_iszConceptModifiers );
}

//-----------------------------------------------------------------------------

#endif // AI_GOALENTITY_H
