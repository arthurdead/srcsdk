#ifndef MATCHFRAMEWORK_H
#define MATCHFRAMEWORK_H

#pragma once

#include "matchmaking/imatchframework.h"

class CMatchFramework : public CBaseAppSystem<IMatchFramework>
{
public:
	// Run frame of the matchmaking framework
	virtual void RunFrame();

	// Get matchmaking extensions
	virtual IMatchExtensions * GetMatchExtensions();

	// Get events container
	virtual IMatchEventsSubscription * GetEventsSubscription();

	// Get the matchmaking title interface
	virtual IMatchTitle * GetMatchTitle();

	// Get the match session interface of the current match framework type
	virtual IMatchSession * GetMatchSession();

	// Get the network msg encode/decode factory
	virtual IMatchNetworkMsgController * GetMatchNetworkMsgController();

	// Get the match system
	virtual IMatchSystem * GetMatchSystem();

	// Entry point to create session
	virtual void CreateSession( KeyValues *pSettings );

	// Entry point to match into a session
	virtual void MatchSession( KeyValues *pSettings );

	// Accept invite
	virtual void DO_NOT_USE_AcceptInvite( int );

	// Close the session
	virtual void CloseSession();
};

#endif
