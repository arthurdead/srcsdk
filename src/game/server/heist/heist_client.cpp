#include "cbase.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "team.h"
#include "viewport_panel_names.h"
#include "tier0/vprof.h"
#include "filesystem.h"
#include "cdll_int.h"

#include "heist_player.h"
#include "heist_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say( edict_t *pEdict, bool teamonly );

ConVar sv_motd_unload_on_dismissal("sv_motd_unload_on_dismissal", "0", 0, "If enabled, the MOTD contents will be unloaded when the player closes the MOTD.");

extern CBaseEntity *FindPickerEntityClass(CBasePlayer *pPlayer, char *classname);
extern bool g_fGameOver;

void FinishClientPutInServer(CHeistPlayer *pPlayer)
{
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

	if(gpGlobals->maxClients > 1) {
		/*
		const ConVar *hostname = cvar->FindVar("hostname");
		const char *title = hostname ? hostname->GetString() : "MESSAGE OF THE DAY";

		KeyValues *data = new KeyValues("data");
		data->SetString("title", title);
		data->SetString("type", "1");
		data->SetString("msg", "motd");
		data->SetBool("unload", sv_motd_unload_on_dismissal.GetBool());

		pPlayer->ShowViewPortPanel(PANEL_INFO, true, data);
		data->deleteThis();
		*/
	}

	KeyValues *pkvTransitionRestoreFile = new KeyValues("transition.cfg");
	if(pkvTransitionRestoreFile->LoadFromFile(filesystem, "transition.cfg")) {
		const char *pszSteamID = pkvTransitionRestoreFile->GetName();
		const char *PlayerSteamID = engine->GetPlayerNetworkIDString(pPlayer->edict());

		if(Q_strcmp(PlayerSteamID, pszSteamID) == 0) {
			KeyValues *pkvHealth = pkvTransitionRestoreFile->FindKey("Health");
			KeyValues *pkvArmour = pkvTransitionRestoreFile->FindKey("Armour");
			KeyValues *pkvActiveWep = pkvTransitionRestoreFile->FindKey("ActiveWeapon");

			struct TranWeapInfo
			{
				KeyValues *pkvWeapon = NULL;
				KeyValues *pkvWeapon_PriClip = NULL;
				KeyValues *pkvWeapon_SecClip = NULL;
				KeyValues *pkvWeapon_PriClipAmmo = NULL;
				KeyValues *pkvWeapon_SecClipAmmo = NULL;
				KeyValues *pkvWeapon_PriClipAmmoLeft = NULL;
				KeyValues *pkvWeapon_SecClipAmmoLeft = NULL;
			};

			TranWeapInfo tran_weap_info[12];

			char szKeyName[128];

			for(int i = 0; i < ARRAYSIZE(tran_weap_info); ++i) {
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i", i);
				tran_weap_info[i].pkvWeapon = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_PriClip", i);
				tran_weap_info[i].pkvWeapon_PriClip = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_SecClip", i);
				tran_weap_info[i].pkvWeapon_SecClip = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_PriClipAmmo", i);
				tran_weap_info[i].pkvWeapon_PriClipAmmo = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_SecClipAmmo", i);
				tran_weap_info[i].pkvWeapon_SecClipAmmo = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_PriClipAmmoLeft", i);
				tran_weap_info[i].pkvWeapon_PriClipAmmoLeft = pkvTransitionRestoreFile->FindKey(szKeyName);
				Q_snprintf(szKeyName, sizeof(szKeyName), "Weapon_%i_SecClipAmmoLeft", i);
				tran_weap_info[i].pkvWeapon_SecClipAmmoLeft = pkvTransitionRestoreFile->FindKey(szKeyName);
			}

			if(pszSteamID) {
				int PlayerHealthValue = pkvHealth->GetInt();
				int PlayerArmourValue = pkvArmour->GetInt();

				const char *pkvActiveWep_Value = pkvActiveWep->GetString();

				pPlayer->SetHealth( PlayerHealthValue );
				pPlayer->m_iMaxHealth = 125;
				pPlayer->SetArmorValue(PlayerArmourValue);
				pPlayer->SetModel("models/sdk/Humans/Group03/male_06_sdk.mdl");

				pPlayer->EquipSuit();

				const char *pkvWeapon_Value = NULL;
				int Weapon_PriClip_Value = 0;
				const char *pkvWeapon_PriClipAmmo_Value = NULL;
				int Weapon_SecClip_Value = 0;
				const char *pkvWeapon_SecClipAmmo_Value = NULL;
				int Weapon_PriClipCurrent_Value = 0;
				int Weapon_SecClipCurrent_Value = 0;

				for(int i = 0; i < ARRAYSIZE(tran_weap_info); ++i) {
					pkvWeapon_Value = tran_weap_info[i].pkvWeapon->GetString();
					Weapon_PriClip_Value = tran_weap_info[i].pkvWeapon_PriClip->GetInt();
					pkvWeapon_PriClipAmmo_Value = tran_weap_info[i].pkvWeapon_PriClipAmmo->GetString();
					Weapon_SecClip_Value = tran_weap_info[i].pkvWeapon_SecClip->GetInt();
					pkvWeapon_SecClipAmmo_Value = tran_weap_info[i].pkvWeapon_SecClipAmmo->GetString();
					Weapon_PriClipCurrent_Value = tran_weap_info[i].pkvWeapon_PriClipAmmoLeft->GetInt();
					Weapon_SecClipCurrent_Value = tran_weap_info[i].pkvWeapon_SecClipAmmoLeft->GetInt();

					pPlayer->GiveNamedItem(pkvWeapon_Value);
					pPlayer->Weapon_Switch(pPlayer->Weapon_OwnsThisType(pkvWeapon_Value));

					CBaseCombatWeapon *pActiveWep = pPlayer->GetActiveWeapon();

					if(pActiveWep->UsesClipsForAmmo1()) {
						if(Weapon_PriClipCurrent_Value != -1) {
							pActiveWep->m_iClip1 = Weapon_PriClipCurrent_Value;
							pActiveWep->m_iPrimaryAmmoType = Weapon_PriClip_Value;
							pActiveWep->SetPrimaryAmmoCount(int(Weapon_PriClip_Value));
							pPlayer->GiveAmmo(Weapon_PriClip_Value, Weapon_PriClip_Value);
						}
					} else {
						pActiveWep->m_iPrimaryAmmoType = Weapon_PriClip_Value;
						pActiveWep->SetPrimaryAmmoCount(Weapon_PriClip_Value);
						pPlayer->GiveAmmo(Weapon_PriClip_Value, Weapon_PriClip_Value);
					}

					if(pActiveWep->UsesClipsForAmmo2()) {
						if(Weapon_SecClipCurrent_Value != -1) {
							pActiveWep->m_iClip2 = Weapon_SecClipCurrent_Value;
							pActiveWep->m_iSecondaryAmmoType = Weapon_SecClip_Value;
							pActiveWep->SetSecondaryAmmoCount(int(Weapon_SecClip_Value));
							pPlayer->GiveAmmo(Weapon_SecClip_Value, Weapon_SecClip_Value);
						}
					} else {
						pActiveWep->m_iSecondaryAmmoType = Weapon_SecClip_Value;
						pActiveWep->SetSecondaryAmmoCount(int(Weapon_SecClip_Value));
						pPlayer->GiveAmmo(Weapon_SecClip_Value, Weapon_SecClip_Value);
					}
				}

				pPlayer->Weapon_Switch(pPlayer->Weapon_OwnsThisType(pkvActiveWep_Value));
			} else {
				pPlayer->ShowViewPortPanel(PANEL_CLASS, true, NULL);
			}
		} else {
			pPlayer->ShowViewPortPanel(PANEL_CLASS, true, NULL);
		}
	} else {
		pPlayer->ShowViewPortPanel(PANEL_CLASS, true, NULL);
	}
}

void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	CBasePlayer *pPlayer = CBasePlayer::CreatePlayer("player", pEdict);
	pPlayer->SetPlayerName(playername);
}

void ClientActive(edict_t *pEdict, bool bLoadGame)
{
	if(gpGlobals->maxClients == 1) {
		CHeistPlayer *pPlayer = ToHeistPlayer(CBaseEntity::Instance(pEdict));
		Assert( pPlayer );
		if(!pPlayer) {
			return;
		}

		pPlayer->InitialSpawn();

		if(!bLoadGame) {
			pPlayer->Spawn();
		}

		return;
	}

	Assert(!bLoadGame);

	CHeistPlayer *pPlayer = ToHeistPlayer(CBaseEntity::Instance(pEdict));
	FinishClientPutInServer(pPlayer);
}

const char *GetGameDescription()
{
	if(g_pGameRules) {
		return g_pGameRules->GetGameDescription();
	} else {
		return "The Heist: Source";
	}
}

CBaseEntity *FindEntity(edict_t *pEdict, char *classname)
{
	if(FStrEq(classname, "")) {
		return FindPickerEntityClass(static_cast<CBasePlayer *>(GetContainingEntity(pEdict)), classname);
	}

	return NULL;
}

void ClientGamePrecache( void )
{
}

void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
	if(gpGlobals->maxClients > 1) {
		CHeistPlayer *pPlayer = ToHeistPlayer( pEdict );
		if(pPlayer) {
			if(gpGlobals->curtime > pPlayer->GetDeathTime() + DEATH_ANIMATION_TIME) {
				pPlayer->Spawn();
			} else {
				pPlayer->SetNextThink(gpGlobals->curtime + 0.1f);
			}
		}
	} else {
		engine->ServerCommand( "reload\n" );
	}
}

void GameStartFrame()
{
	VPROF("GameStartFrame()");

	if(g_fGameOver) {
		return;
	}

	gpGlobals->teamplay = (teamplay.GetInt() != 0);
}

void InstallGameRules()
{
	CreateGameRulesObject( "CHeistGamerules" );
}
