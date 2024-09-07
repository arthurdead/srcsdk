#include "cbase.h"
#include "EventLog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CEventLog g_EventLog;
CEventLog *GameLogSystem()
{
	return &g_EventLog;
}
