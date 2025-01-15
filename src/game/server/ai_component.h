//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_COMPONENT_H
#define AI_COMPONENT_H

#pragma once

#include "networkvar.h"
#include "ai_activity.h"
#include "recast/recast_imgr.h"
#include "shareddefs.h"

class CAI_BaseNPC;
class CAI_Enemies;
typedef int AI_TaskFailureCode_t;
struct Task_t;
struct edict_t;
enum Capability_t : uint64;
enum MemoryFlags_t : uint64;

//-----------------------------------------------------------------------------
// CAI_Component
//
// Purpose: Shared functionality of all classes that assume some of the 
//			responsibilities of an owner AI. 
//-----------------------------------------------------------------------------

class CAI_Component
{
public:
	DECLARE_CLASS_NOBASE( CAI_Component );
protected:
	CAI_Component( CAI_BaseNPC *pOuter = NULL )
	 : m_pOuter(pOuter)
	{
	}

	virtual ~CAI_Component() {}

public:
	virtual void SetOuter( CAI_BaseNPC *pOuter )	{ m_pOuter = pOuter; }

	CAI_BaseNPC *		GetOuter() 			{ return m_pOuter; }
	const CAI_BaseNPC *	GetOuter() const 	{ return m_pOuter; }

	NavMeshType_t		GetNavMeshType() const;
	float 				GetNavMeshWidth() const;
	float 				GetNavMeshHeight() const;
	float 				GetNavMeshLength() const;
	const Vector &		GetNavMeshMins() const;
	const Vector &		GetNavMeshMaxs() const;
	int					GetNavMeshTraceMask() const;

	float 				GetHullWidth() const;
	float 				GetHullHeight() const;
	float 				GetHullLength() const;
	const Vector &		GetHullMins() const;
	const Vector &		GetHullMaxs() const;

protected:
	//
	// Common services provided by CAI_BaseNPC, Convenience methods to simplify derived code
	//
	edict_t *			GetEdict();
	
	const Vector &		GetLocalOrigin() const;
	void 				SetLocalOrigin( const Vector &origin );

	const Vector &		GetAbsOrigin() const;
	const QAngle&		GetAbsAngles() const;
	
	void				SetLocalAngles( const QAngle& angles );
	const QAngle &		GetLocalAngles( void ) const;
	
	const Vector&		WorldAlignMins() const;
	const Vector&		WorldAlignMaxs() const;
	Vector 				WorldSpaceCenter() const;
	
	int 				GetCollisionGroup() const;
	
	void				SetSolid( SolidType_t val );
	SolidType_t			GetSolid() const;
	
	float				GetGravity() const;
	void				SetGravity( float );

	CBaseEntity*		GetEnemy();
	const Vector &		GetEnemyLKP() const;
	void				TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition);
	
	CBaseEntity*		GetTarget();
	void				SetTarget( CBaseEntity *pTarget );
	
	const Task_t*		GetCurTask( void );
	virtual void		TaskFail( AI_TaskFailureCode_t );
	void				TaskFail( const char *pszGeneralFailText );
	virtual void		TaskComplete( bool fIgnoreSetFailedCondition = false );
	int					TaskIsRunning();
	inline int			TaskIsComplete();

	Activity			GetActivity();
	void				SetActivity( Activity NewActivity );
	float				GetIdealSpeed() const;
	float				GetIdealAccel() const;
	sequence_t					GetSequence();

	EntityBehaviorFlags_t					GetEntFlags() const;
	void				AddEntFlag( EntityBehaviorFlags_t flags );
	void				RemoveEntFlag( EntityBehaviorFlags_t flagsToRemove );
	void				ToggleEntFlag( EntityBehaviorFlags_t flagToToggle );

	void				SetGroundEntity( CBaseEntity *ground );

	CBaseEntity*		GetGoalEnt();
	void				SetGoalEnt( CBaseEntity *pGoalEnt );
	
	void				Remember( MemoryFlags_t iMemory );
	void				Forget( MemoryFlags_t iMemory );
	bool				HasMemory( MemoryFlags_t iMemory );

	CAI_Enemies *		GetEnemies();
	
	const char * 		GetEntClassname();
	
	Capability_t					CapabilitiesGet();

	float				GetLastThink( const char *szContext = NULL );

public:
	#pragma push_macro("new")
	#pragma push_macro("delete")
	#undef new
	#undef delete

	void *operator new( size_t nBytes );
	void *operator new( size_t nBytes, int nBlockUse, const char *pFileName, int nLine );

	#pragma pop_macro("delete")
	#pragma pop_macro("new")

private:
	CAI_BaseNPC *m_pOuter;
};

//-----------------------------------------------------------------------------

template <class NPC_CLASS, class BASE_COMPONENT = CAI_Component>
class CAI_ComponentWithOuter : public BASE_COMPONENT
{
protected:
	CAI_ComponentWithOuter(NPC_CLASS *pOuter = NULL)
	 : BASE_COMPONENT(pOuter)
	{
	}

public:
	DECLARE_CLASS_NOFRIEND( CAI_ComponentWithOuter, BASE_COMPONENT );

	// Hides base version
	void SetOuter( NPC_CLASS *pOuter )		{ BASE_COMPONENT::SetOuter((CAI_BaseNPC *)pOuter); }
	NPC_CLASS * 		GetOuter() 			{ return (NPC_CLASS *)(BASE_COMPONENT::GetOuter()); }
	const NPC_CLASS *	GetOuter() const 	{ return (NPC_CLASS *)(BASE_COMPONENT::GetOuter()); }
};

//-----------------------------------------------------------------------------

#define DEFINE_AI_COMPONENT_OUTER( NPC_CLASS ) \
	void SetOuter( NPC_CLASS *pOuter )		{ CAI_Component::SetOuter((CAI_BaseNPC *)pOuter); } \
	NPC_CLASS * 		GetOuter() 			{ return (NPC_CLASS *)(CAI_Component::GetOuter()); } \
	const NPC_CLASS *	GetOuter() const 	{ return (NPC_CLASS *)(CAI_Component::GetOuter()); }

//-----------------------------------------------------------------------------

#endif // AI_COMPONENT_H
