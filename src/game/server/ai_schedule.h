//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		A schedule
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "bitstring.h"

#ifndef AI_SCHEDULE_H
#define AI_SCHEDULE_H
#pragma once

#include "ai_npcstate.h"
#include "ai_condition.h"

class	CStringRegistry;
class   CAI_ClassScheduleIdSpace;
class	CAI_GlobalScheduleNamespace;
class	CAI_BaseNPC;

struct	Task_t;

typedef CBitVec<MAX_CONDITIONS> CAI_ScheduleBits;

//==================================================
// goalType_t
//==================================================

enum goalType_t
{
	GOAL_NONE = -1,
	GOAL_ENEMY,				//Our current enemy's position
	GOAL_TARGET,			//Our current target's position
	GOAL_ENEMY_LKP,			//Our current enemy's last known position
	GOAL_SAVED_POSITION,	//Our saved position
};

//==================================================
// pathType_t
//==================================================

enum pathType_t
{
	PATH_NONE = -1,
	PATH_TRAVEL,			//Path that will take us to the goal
	PATH_LOS,				//Path that gives us line of sight to our goal
	PATH_FLANK,				//Path that will take us to a flanking position of our goal
	PATH_FLANK_LOS,			//Path that will take us to within line of sight to the flanking position of our goal
	PATH_COVER,				//Path that will give us cover from our goal
	PATH_COVER_LOS,			//Path that will give us line of sight to cover from our goal
};

//=============================================================================
// >> CAI_Schedule
//=============================================================================

class CAI_Schedule;

class CAI_SchedulesManager
{
public:
	CAI_SchedulesManager()
	{
		allSchedules = NULL;
		m_CurLoadSig = 0;		// Note when schedules reset
	}

	int				GetScheduleLoadSignature() { return m_CurLoadSig; }
	CAI_Schedule*	GetScheduleFromID( int schedID );	// Function to return schedule from linked list 
	CAI_Schedule*	GetScheduleByName( const char *name );

	bool LoadAllSchedules(void);

	bool LoadSchedules( const char *pclassname, const char *pfile, const char *pfilename, CAI_ClassScheduleIdSpace *pIdSpace, CAI_GlobalScheduleNamespace *pGlobalNamespace );

private:
	friend class CAI_SystemHook;
	
	int				m_CurLoadSig;					// Note when schedules reset
	CAI_Schedule*	allSchedules;						// A linked list of all schedules

	CAI_Schedule *	CreateSchedule(const char *name, int schedule_id);

	void CreateStringRegistries( void );
	void DestroyStringRegistries( void );
	void DeleteAllSchedules(void);

	//static bool	LoadSchedules( char* prefix,	int taskIDOffset,	int taskENOffset,
	//											int schedIDOffset,  int schedENOffset,
	//											int condIDOffset,	int condENOffset);

public:
	// parsing helpers
	static NPC_STATE	GetStateID(const char *state_name);
	static int	GetMemoryID(const char *memory_name);
	static pathType_t GetPathID( const char *token );
	static goalType_t GetGoalID( const char *token );

};

extern CAI_SchedulesManager g_AI_SchedulesManager;
extern CAI_SchedulesManager g_AI_AgentSchedulesManager;

typedef int SchedId_t;
typedef int SchedGlobalId_t;
typedef int SchedLocalId_t;

class CAI_Schedule
{
// ---------
//	Static
// ---------
// ---------
public:
	SchedId_t GetId() const
	{
		return m_iScheduleID;
	}
	
	const Task_t *GetTaskList() const
	{
		return m_pTaskList;
	}
	
	int NumTasks() const
	{
		return m_iNumTasks;
	}
	
	void GetInterruptMask( CAI_ScheduleBits *pBits ) const
	{
		m_InterruptMask.CopyTo( pBits );
	}

	bool HasInterrupt( int condition ) const
	{
		return m_InterruptMask.IsBitSet( condition );
	}
	
	const char *GetName() const
	{
		return m_pName;
	}
	
private:
	friend class CAI_SchedulesManager;

	SchedId_t			m_iScheduleID;				// The id number of this schedule

	Task_t		*m_pTaskList;
	int			m_iNumTasks;	 

	CAI_ScheduleBits m_InterruptMask;			// a bit mask of conditions that can interrupt this schedule 
	char		*m_pName;

	CAI_Schedule *nextSchedule;				// The next schedule in the list of schedules

	CAI_Schedule(const char *name,int schedule_id, CAI_Schedule *pNext);
	~CAI_Schedule( void );
};

//-----------------------------------------------------------------------------
//
// In-memory schedules
//

#define AI_DEFINE_SCHEDULE( name, text ) \
	const char * g_psz##name = \
		#name \
		"\n{\n" \
		text \
		"\n}\n"

//-------------------------------------

struct AI_SchedLoadStatus_t
{
	bool fValid;
	int  signature;
};

typedef bool (*AIScheduleLoadFunc_t)();

// @Note (toml 02-16-03): The following class exists to allow us to establish an anonymous friendship
// in DEFINE_CUSTOM_SCHEDULE_PROVIDER. The particulars of this implementation is almost entirely
// defined by bugs in MSVC 6.0
class ScheduleLoadHelperImpl
{
public:
	template <typename T> 
	static AIScheduleLoadFunc_t AccessScheduleLoadFunc(T *)
	{
		return (&T::LoadSchedules);
	}
};

#define ScheduleLoadHelper( type ) (ScheduleLoadHelperImpl::AccessScheduleLoadFunc((type *)0))

#define AI_LOAD_SCHEDULE( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if ( classname::gm_SchedLoadStatus.fValid ) \
		{ \
			classname::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedules( #classname,(char *)g_psz##name,NULL,&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() ); \
		} \
	} while (false)


// For loading default schedules in memory  (see ai_default.cpp)
#define AI_LOAD_DEF_SCHEDULE_MEMORY( classname, name ) \
	do \
	{ \
		extern const char * g_psz##name; \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,(char *)g_psz##name,NULL,&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			return false; \
	} while (false)

#define AI_LOAD_DEF_SCHEDULE_FILE( classname, name, folder ) \
	do \
	{ \
		if (!g_AI_SchedulesManager.LoadSchedules( #classname,NULL,UTIL_VarArgs("%s/%s.sch",folder,#name),&classname::gm_ClassScheduleIdSpace, classname::GetSchedulingSymbols() )) \
			return false; \
	} while (false)

//-----------------

#define DECLARE_USES_SCHEDULE_PROVIDER( classname )	reqiredOthers.AddToTail( ScheduleLoadHelper(classname) );

//-----------------

#define EXTERN_SCHEDULE( id ) \
	scheduleIds.PushBack( #id, id ); \
	extern const char * g_psz##id; \
	schedulesToLoad.AddToTail( g_psz##id );

//-----------------

#define DEFINE_SCHEDULE( id, text ) \
	scheduleIds.PushBack( #id, id ); \
	const char * g_psz##id = \
		"\n	Schedule" \
		"\n		" #id \
		text \
		"\n"; \
	schedulesToLoad.AddToTail( g_psz##id );

//-----------------------------------------------------------------------------

#define ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,schedName,schedEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddSchedule( schedName, schedEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SCHEDULE(derivedClass,schedEN) ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,#schedEN,schedEN)

#endif // AI_SCHEDULE_H
