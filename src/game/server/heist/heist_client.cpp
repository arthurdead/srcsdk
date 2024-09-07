#include "cbase.h"
#include "heist_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	CBasePlayer *pPlayer = CBasePlayer::CreatePlayer("player", pEdict);
	pPlayer->SetPlayerName(playername);
}

void ClientActive(edict_t *pEdict )
{
	CHeistPlayer *pPlayer = ToHeistPlayer(CBaseEntity::Instance(pEdict));

	pPlayer->Precache();
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	char sName[128];
	Q_strncpy(sName, pPlayer->GetPlayerName(), sizeof(sName));

	for(char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++) {
		if(*pApersand == '%') {
			*pApersand = ' ';
		}
	}

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>");
}

void ClientGamePrecache( void )
{
}

void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	CHeistPlayer *pPlayer = ToHeistPlayer( pEdict );
	pPlayer->Spawn();
}

void GameStartFrame()
{
}

void InstallGameRules()
{
	CreateGameRulesObject( "CHeistGamerules" );
}

void ClientFullyConnect( edict_t *pEntity )
{
}

void InitBodyQue()
{
}