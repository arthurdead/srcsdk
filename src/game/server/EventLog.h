//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined EVENTLOG_H
#define EVENTLOG_H

#pragma once

#include "GameEventListener.h"
#include <igamesystem.h>

class CEventLog : public CGameEventListener, public CBaseGameSystem
{
	
public:
	CEventLog();
	virtual ~CEventLog();

public: // IGameEventListener Interface
	
	virtual void FireGameEvent( IGameEvent * event );
	
public: // CBaseGameSystem overrides

	virtual bool Init();
	virtual void Shutdown();

protected:

	virtual bool PrintEvent( IGameEvent * event );
	virtual bool PrintGameEvent( IGameEvent * event );
	virtual bool PrintPlayerEvent( IGameEvent * event );
	virtual bool PrintTeamEvent( IGameEvent * event );
	virtual bool PrintOtherEvent( IGameEvent * event );

public:
	enum
	{
		PLAYER_LOGINFO_SIZE = 256,
	};

	virtual void FormatPlayer( CBaseEntity *ent, char *str, int len ) const;
	const char *FormatPlayer( CBaseEntity *ent ) const;
};

extern CEventLog* GameLogSystem();

#endif // EVENTLOG_H
