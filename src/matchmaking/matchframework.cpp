#include "matchframework.h"
#include "tier1/interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMatchFramework g_MatchFramework;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMatchFramework, IMatchFramework, IMATCHFRAMEWORK_VERSION_STRING, g_MatchFramework);

void CMatchFramework::RunFrame()
{
}

IMatchExtensions *CMatchFramework::GetMatchExtensions()
{
	return NULL;
}

IMatchEventsSubscription *CMatchFramework::GetEventsSubscription()
{
	return NULL;
}

IMatchTitle *CMatchFramework::GetMatchTitle()
{
	return NULL;
}

IMatchSession *CMatchFramework::GetMatchSession()
{
	return NULL;
}

IMatchNetworkMsgController *CMatchFramework::GetMatchNetworkMsgController()
{
	return NULL;
}

IMatchSystem *CMatchFramework::GetMatchSystem()
{
	return NULL;
}

void CMatchFramework::CreateSession( KeyValues *pSettings )
{
}

void CMatchFramework::MatchSession( KeyValues *pSettings )
{
}

void CMatchFramework::DO_NOT_USE_AcceptInvite( int )
{
}

void CMatchFramework::CloseSession()
{
}
