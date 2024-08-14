#include "cbase.h"
#include "EventLog.h"

class CHeistEventLog : public CEventLog
{
	using BaseClass = CEventLog;

public:
	bool PrintEvent(IGameEvent *event) override
	{
		if(BaseClass::PrintEvent(event)) {
			return true;
		}

		if(Q_strcmp(event->GetName(), "heist_") == 0) {
			return PrintHeistEvent(event);
		}

		return false;
	}

private:
	bool PrintHeistEvent(IGameEvent *event)
	{
		return false;
	}
};

CHeistEventLog g_HeistEventLog;

IGameSystem *GameLogSystem()
{
	return &g_HeistEventLog;
}
