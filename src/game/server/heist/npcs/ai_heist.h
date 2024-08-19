#ifndef AI_HEIST_H
#define AI_HEIST_H

#pragma once

#include "ai_default.h"
#include "ai_task.h"
#include "ai_condition.h"

enum
{
	SCHED_SCAN_FOR_SUSPICIOUS_ACTIVITY = LAST_SHARED_SCHEDULE, 

	LAST_HEIST_SCHEDULE
};

enum 
{
	TASK_HEIST_UNUSED = LAST_SHARED_TASK,



	LAST_HEIST_TASK
};

enum 
{
	COND_UNCOVERED_HEISTER = LAST_SHARED_CONDITION,

	

	LAST_HEIST_CONDITION
};


#endif
