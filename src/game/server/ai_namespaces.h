//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAMESPACES_H
#define AI_NAMESPACES_H
#pragma once

class CStringRegistry;

#include "tier0/platform.h"
#include "ai_condition.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "tier1/utlmap.h"

#define MAX_STRING_INDEX 9999
const int GLOBAL_IDS_BASE = 1000000000; // decimal for debugging readability

//-----------------------------------------------------------------------------

inline bool AI_IdIsGlobal( int id )			{ return ( id >= GLOBAL_IDS_BASE || id == -1 ); }
inline bool AI_IdIsLocal( int id )			{ return ( id < GLOBAL_IDS_BASE || id == -1 );  }
inline int  AI_RemapToGlobal( int id )		{ return ( id != -1 ) ? id + GLOBAL_IDS_BASE : -1; }
inline int  AI_RemapFromGlobal( int id )	{ return ( id != -1 ) ? id - GLOBAL_IDS_BASE : -1; }

inline int	AI_MakeGlobal( int id )			{ return AI_IdIsLocal( id ) ? AI_RemapToGlobal( id ) : id; }

//-----------------------------------------------------------------------------
// CAI_GlobalNamespace
//
// Purpose: Symbol table for all symbols across a given namespace, a
//			bi-directional mapping of "text" to global ID
//

class CAI_GlobalNamespace
{
public:
	CAI_GlobalNamespace();
	~CAI_GlobalNamespace();

	void Clear();

	void AddSymbol( const char *pszSymbol, int symbolID );
	int NextGlobalBase() const;

	const char *IdToSymbol( int symbolID ) const;
	int SymbolToId( const char *pszSymbol ) const;

private:
	CStringRegistry * 	m_pSymbols;
	int					m_NextGlobalBase;
};

//-----------------------------------------------------------------------------
// CAI_LocalIdSpace
//
// Purpose: Maps per class IDs to global IDs, so that various classes can use
//			the same integer in local space to represent different globally
//			unique integers. Used for schedules, tasks, conditions and squads
//

class CAI_LocalIdSpace
{
public:
	CAI_LocalIdSpace( bool fIsRoot = false );

	bool Init( CAI_GlobalNamespace *pGlobalNamespace, CAI_LocalIdSpace *pParentIDSpace = NULL );
	bool IsGlobalBaseSet() const { return ( m_globalBase != -1 ); }

	bool AddSymbol( const char *pszSymbol, int localId, const char *pszDebugSymbolType = "", const char *pszDebugOwner = "" );

	int GlobalToLocal( int globalID ) const;
	int LocalToGlobal( int localID ) const;

	CAI_GlobalNamespace *GetGlobalNamespace() { return m_pGlobalNamespace; }
	const CAI_GlobalNamespace *GetGlobalNamespace() const { return m_pGlobalNamespace; }

private:
	bool IsLocalBaseSet() const	{ return ( m_localBase != MAX_STRING_INDEX );	}
	int GetLocalBase() const	{ return m_localBase;  }
	int GetGlobalBase() const	{ return m_globalBase; }
	int GetLocalTop() const		{ return m_localTop;  }
	int GetGlobalTop() const	{ return m_globalTop; }

	bool SetLocalBase( int newBase );

	// --------------------------------

	int 					m_globalBase;
	int 					m_localBase;
	int 					m_localTop;
	int 					m_globalTop;

	CAI_LocalIdSpace *		m_pParentIDSpace;
	CAI_GlobalNamespace *	m_pGlobalNamespace;
};

//-----------------------------------------------------------------------------
//
// Namespaces used by CAI_BaseNPC
//
//-----------------------------------------------------------------------------

class CAI_GlobalScheduleNamespace
{
public:
	CAI_GlobalScheduleNamespace()
		: m_TaskParamsChecks(0, 0, DefLessFunc(TaskGlobalId_t))
	{
	}

	void Clear()
	{
		m_ScheduleNamespace.Clear();
		m_TaskNamespace.Clear();
		m_ConditionNamespace.Clear();
	}

	void 		AddSchedule( const char *pszSchedule, SchedGlobalId_t scheduleID );
	const char *ScheduleIdToSymbol( SchedGlobalId_t scheduleID ) const;
	SchedGlobalId_t 		ScheduleSymbolToId( const char *pszSchedule ) const;

	void 		AddTask( const char *pszTask, TaskGlobalId_t taskID, TaskParamCheck_t paramCheck );
	const char *TaskIdToSymbol( TaskGlobalId_t taskID ) const;
	TaskGlobalId_t 		TaskSymbolToId( const char *pszTask ) const;

	void 		AddCondition( const char *pszCondition, AiCondGlobalId_t conditionID );
	const char *ConditionIdToSymbol( AiCondGlobalId_t conditionID ) const;
	AiCondGlobalId_t 		ConditionSymbolToId( const char *pszCondition ) const;
	int			NumConditions() const;

	const TaskParamCheck_t *TaskParamsCheck( TaskGlobalId_t id ) const
	{
		int index = m_TaskParamsChecks.Find( id );
		if(index == m_TaskParamsChecks.InvalidIndex())
			return NULL;
		return &m_TaskParamsChecks[index];
	}

private:
	friend class CAI_ClassScheduleIdSpace;

	CAI_GlobalNamespace m_ScheduleNamespace;
	CAI_GlobalNamespace m_TaskNamespace;
	CAI_GlobalNamespace m_ConditionNamespace;

	CUtlMap<TaskGlobalId_t, TaskParamCheck_t> m_TaskParamsChecks;
};

//-------------------------------------

class CAI_ClassScheduleIdSpace
{
public:
	CAI_ClassScheduleIdSpace( bool fIsRoot = false )
	 :	m_ScheduleIds( fIsRoot ),
	 	m_TaskIds( fIsRoot ),
	 	m_ConditionIds( fIsRoot ),
	 	m_TaskParamsChecks(0, 0, DefLessFunc(TaskLocalId_t))
	{
	}

	bool Init( const char *pszClassName, CAI_GlobalScheduleNamespace *pGlobalNamespace, CAI_ClassScheduleIdSpace *pParentIDSpace = NULL );

	const char *GetClassName() const { return m_pszClassName; }

	bool IsGlobalBaseSet() const;

	bool AddSchedule( const char *pszSymbol, SchedLocalId_t localId, const char *pszDebugOwner = "" );
	SchedLocalId_t ScheduleGlobalToLocal( SchedGlobalId_t globalID ) const;
	SchedGlobalId_t ScheduleLocalToGlobal( SchedLocalId_t localID ) const;

	bool AddTask( const char *pszSymbol, TaskLocalId_t localId, TaskParamCheck_t paramCheck, const char *pszDebugOwner = "" );
	TaskLocalId_t TaskGlobalToLocal( TaskGlobalId_t globalID ) const;
	TaskGlobalId_t TaskLocalToGlobal( TaskLocalId_t localID ) const;

	bool AddCondition( const char *pszSymbol, int localId, const char *pszDebugOwner = "" );
	AiCondLocalId_t ConditionGlobalToLocal( AiCondGlobalId_t globalID ) const;
	AiCondGlobalId_t ConditionLocalToGlobal( AiCondLocalId_t localID ) const;

	const TaskParamCheck_t *TaskParamsCheck( TaskLocalId_t id ) const
	{
		int index = m_TaskParamsChecks.Find( id );
		if(index == m_TaskParamsChecks.InvalidIndex())
			return NULL;
		return &m_TaskParamsChecks[index];
	}

private:
	const char *	 m_pszClassName;
	CAI_LocalIdSpace m_ScheduleIds;
	CAI_LocalIdSpace m_TaskIds;
	CAI_LocalIdSpace m_ConditionIds;

	CUtlMap<TaskGlobalId_t, TaskParamCheck_t> m_TaskParamsChecks;
};

//-----------------------------------------------------------------------------

inline void CAI_GlobalScheduleNamespace::AddSchedule( const char *pszSchedule, SchedGlobalId_t scheduleID )
{
	m_ScheduleNamespace.AddSymbol( pszSchedule, scheduleID);
}

inline const char *CAI_GlobalScheduleNamespace::ScheduleIdToSymbol( SchedGlobalId_t scheduleID ) const
{
	return m_ScheduleNamespace.IdToSymbol( scheduleID );
}

inline SchedGlobalId_t CAI_GlobalScheduleNamespace::ScheduleSymbolToId( const char *pszSchedule ) const
{
	return m_ScheduleNamespace.SymbolToId( pszSchedule );
}

inline void CAI_GlobalScheduleNamespace::AddTask( const char *pszTask, TaskGlobalId_t taskID, TaskParamCheck_t paramCheck )
{
	m_TaskNamespace.AddSymbol( pszTask, taskID);
	m_TaskParamsChecks.InsertOrReplace( taskID, paramCheck );
}

inline const char *CAI_GlobalScheduleNamespace::TaskIdToSymbol( TaskGlobalId_t taskID ) const
{
	return m_TaskNamespace.IdToSymbol( taskID );
}

inline TaskGlobalId_t CAI_GlobalScheduleNamespace::TaskSymbolToId( const char *pszTask ) const
{
	return m_TaskNamespace.SymbolToId( pszTask );
}

inline void CAI_GlobalScheduleNamespace::AddCondition( const char *pszCondition, AiCondGlobalId_t conditionID )
{
	m_ConditionNamespace.AddSymbol( pszCondition, conditionID);
}

inline const char *CAI_GlobalScheduleNamespace::ConditionIdToSymbol( AiCondGlobalId_t conditionID ) const
{
	return m_ConditionNamespace.IdToSymbol( conditionID );
}

inline AiCondGlobalId_t CAI_GlobalScheduleNamespace::ConditionSymbolToId( const char *pszCondition ) const
{
	return m_ConditionNamespace.SymbolToId( pszCondition );
}

inline int CAI_GlobalScheduleNamespace::NumConditions() const
{ 
	return m_ConditionNamespace.NextGlobalBase() - GLOBAL_IDS_BASE; 
}

inline bool CAI_ClassScheduleIdSpace::Init( const char *pszClassName, CAI_GlobalScheduleNamespace *pGlobalNamespace, CAI_ClassScheduleIdSpace *pParentIDSpace )
{
	m_pszClassName = pszClassName;
	return ( m_ScheduleIds.Init( &pGlobalNamespace->m_ScheduleNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_ScheduleIds : NULL ) &&
			 m_TaskIds.Init( &pGlobalNamespace->m_TaskNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_TaskIds : NULL ) &&
			 m_ConditionIds.Init( &pGlobalNamespace->m_ConditionNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_ConditionIds : NULL ) );
}

//-----------------------------------------------------------------------------

inline bool CAI_ClassScheduleIdSpace::IsGlobalBaseSet() const
{
	return m_ScheduleIds.IsGlobalBaseSet();
}

inline bool CAI_ClassScheduleIdSpace::AddSchedule( const char *pszSymbol, SchedLocalId_t localId, const char *pszDebugOwner )
{
	return m_ScheduleIds.AddSymbol( pszSymbol, localId, "schedule", pszDebugOwner );
}

inline SchedLocalId_t CAI_ClassScheduleIdSpace::ScheduleGlobalToLocal( SchedGlobalId_t globalID ) const
{
	return m_ScheduleIds.GlobalToLocal( globalID );
}

inline SchedGlobalId_t CAI_ClassScheduleIdSpace::ScheduleLocalToGlobal( SchedLocalId_t localID ) const
{
	return m_ScheduleIds.LocalToGlobal( localID );
}

inline bool CAI_ClassScheduleIdSpace::AddTask( const char *pszSymbol, TaskLocalId_t localId, TaskParamCheck_t paramCheck, const char *pszDebugOwner )
{
	bool added = m_TaskIds.AddSymbol( pszSymbol, localId, "task", pszDebugOwner );
	if(added) {
		m_TaskParamsChecks.InsertOrReplace( localId, paramCheck );
	}
	return added;
}

inline TaskLocalId_t CAI_ClassScheduleIdSpace::TaskGlobalToLocal( TaskGlobalId_t globalID ) const
{
	return m_TaskIds.GlobalToLocal( globalID );
}

inline TaskGlobalId_t CAI_ClassScheduleIdSpace::TaskLocalToGlobal( TaskLocalId_t localID ) const
{
	return m_TaskIds.LocalToGlobal( localID );
}

inline bool CAI_ClassScheduleIdSpace::AddCondition( const char *pszSymbol, AiCondLocalId_t localId, const char *pszDebugOwner )
{
	return m_ConditionIds.AddSymbol( pszSymbol, localId, "condition", pszDebugOwner );
}

inline AiCondLocalId_t CAI_ClassScheduleIdSpace::ConditionGlobalToLocal( AiCondGlobalId_t globalID ) const
{
	return m_ConditionIds.GlobalToLocal( globalID );
}

inline AiCondGlobalId_t CAI_ClassScheduleIdSpace::ConditionLocalToGlobal( AiCondLocalId_t localID ) const
{
	return m_ConditionIds.LocalToGlobal( localID );
}

//=============================================================================

//-------------------------------------

struct AI_NamespaceAddInfo_t
{
	AI_NamespaceAddInfo_t( const char *pszName, int localId )
	 :	pszName( pszName ),
		localId( localId )
	{
	}
	
	const char *pszName;
	int			localId;
};

struct AI_TaskNamespaceAddInfo_t : public AI_NamespaceAddInfo_t
{
	AI_TaskNamespaceAddInfo_t( const char *pszName, int localId ) = delete;

	AI_TaskNamespaceAddInfo_t( const char *pszName, int localId, TaskParamCheck_t params_ )
	 :	AI_NamespaceAddInfo_t( pszName, localId ), params(params_)
	{
	}
	
	TaskParamCheck_t params;
};

struct AI_ScheduleNamespaceAddInfo_t : public AI_NamespaceAddInfo_t
{
	AI_ScheduleNamespaceAddInfo_t( const char *pszName, int localId ) = delete;

	AI_ScheduleNamespaceAddInfo_t( const char *pszName, int localId, const char *pszValue, bool filename_ )
	 :	AI_NamespaceAddInfo_t( pszName, localId ), pszValue(pszValue), filename(filename_)
	{
	}

	const char *pszValue;
	bool filename;
};

class CAI_NamespaceInfos : public CUtlVector<AI_NamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId )
	{
		AddToTail( AI_NamespaceAddInfo_t( pszName, localId ) );
	}

	void Sort()
	{
		CUtlVector<AI_NamespaceAddInfo_t>::Sort( Compare );
	}
	
private:
	static int __cdecl Compare(const AI_NamespaceAddInfo_t *pLeft, const AI_NamespaceAddInfo_t *pRight )
	{
		return pLeft->localId - pRight->localId;
	}
	
};

class CAI_TaskNamespaceInfos : public CUtlVector<AI_TaskNamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId ) = delete;

	void PushBack(  const char *pszName, int localId, TaskParamCheck_t params )
	{
		AddToTail( AI_TaskNamespaceAddInfo_t( pszName, localId, params ) );
	}

	void Sort()
	{
		CUtlVector<AI_TaskNamespaceAddInfo_t>::Sort( Compare );
	}

private:
	static int __cdecl Compare(const AI_TaskNamespaceAddInfo_t *pLeft, const AI_TaskNamespaceAddInfo_t *pRight )
	{
		return pLeft->localId - pRight->localId;
	}
};

class CAI_ScheduleNamespaceInfos : public CUtlVector<AI_ScheduleNamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId ) = delete;

	void PushBack(  const char *pszName, int localId, const char *pszValue, bool filename )
	{
		AddToTail( AI_ScheduleNamespaceAddInfo_t( pszName, localId, pszValue, filename ) );
	}

	void Sort()
	{
		CUtlVector<AI_ScheduleNamespaceAddInfo_t>::Sort( Compare );
	}

private:
	static int __cdecl Compare(const AI_ScheduleNamespaceAddInfo_t *pLeft, const AI_ScheduleNamespaceAddInfo_t *pRight )
	{
		return pLeft->localId - pRight->localId;
	}
};

#endif // AI_NAMESPACES_H
