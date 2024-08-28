#include "cbase.h"
#include "clientmode_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ClientModeShared s_ClientMode;

IClientMode *GetClientMode()
{
	return &s_ClientMode;
}

IClientMode *GetFullscreenClientMode()
{
	return &s_ClientMode;
}

IClientMode *GetClientModeNormal()
{
	return &s_ClientMode;
}
