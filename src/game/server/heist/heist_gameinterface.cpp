#include "cbase.h"
#include "gameinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CServerGameClients::GetPlayerLimits(int &minplayers, int &maxplayers, int &defaultMaxPlayers) const
{
	minplayers = 1;
	defaultMaxPlayers = 4; 
	maxplayers = 4;
}

void CServerGameDLL::LevelInit_ParseAllEntities(const char *pMapEntities)
{
}
