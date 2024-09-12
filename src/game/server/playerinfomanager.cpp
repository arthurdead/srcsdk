//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implementation of player info manager
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "playerinfomanager.h"
#include "edict.h"

extern CGlobalVars *gpGlobals;
static CPlayerInfoManager s_PlayerInfoManager;

namespace
{

	// 
	//  Old version support
	//
	abstract_class IPlayerInfo_V1
	{
	public:
		// returns the players name (UTF-8 encoded)
		virtual const char *GetName() = 0;
		// returns the userid (slot number)
		virtual int		GetUserID() = 0;
		// returns the string of their network (i.e Steam) ID
		virtual const char *GetNetworkIDString() = 0;
		// returns the team the player is on
		virtual int GetTeamIndex() = 0;
		// changes the player to a new team (if the game dll logic allows it)
		virtual void ChangeTeam( int iTeamNum ) = 0;
		// returns the number of kills this player has (exact meaning is mod dependent)
		virtual int	GetFragCount() = 0;
		// returns the number of deaths this player has (exact meaning is mod dependent)
		virtual int	GetDeathCount() = 0;
		// returns if this player slot is actually valid
		virtual bool IsConnected() = 0;
		// returns the armor/health of the player (exact meaning is mod dependent)
		virtual int	GetArmorValue() = 0;
	};
	
	abstract_class IPlayerInfoManager_V1
	{
	public:
		virtual IPlayerInfo_V1 *GetPlayerInfo( edict_t *pEdict ) = 0;
	};


	class CPlayerInfoManager_V1: public IPlayerInfoManager_V1
	{
	public:
		virtual IPlayerInfo_V1 *GetPlayerInfo( edict_t *pEdict );
	};

	static CPlayerInfoManager_V1 s_PlayerInfoManager_V1;


	IPlayerInfo_V1 *CPlayerInfoManager_V1::GetPlayerInfo( edict_t *pEdict )
	{
		CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
		if ( pPlayer )
		{
			return (IPlayerInfo_V1 *)pPlayer->GetPlayerInfo();
		}
		else
		{
			return NULL;
		}
	}

	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPlayerInfoManager_V1, IPlayerInfoManager_V1, "PlayerInfoManager001", s_PlayerInfoManager_V1);
}

IPlayerInfo *CPlayerInfoManager::GetPlayerInfo( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
	if ( pPlayer )
	{
		return pPlayer->GetPlayerInfo();
	}
	else
	{
		return NULL;
	}
}

IPlayerInfo *CPlayerInfoManager::GetPlayerInfo( int index )
{
	return GetPlayerInfo( engine->PEntityOfEntIndex( index ) );
}

CGlobalVars *CPlayerInfoManager::GetGlobalVars()
{
	return gpGlobals;
}

// Games implementing advanced bot support should override this.
int CPlayerInfoManager::AliasToWeaponId(const char *weaponName)
{
	return -1;
}

// Games implementing advanced bot support should override this.
const char *CPlayerInfoManager::WeaponIdToAlias(int weaponId)
{
	return "MOD_DIDNT_IMPLEMENT_ME";
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPlayerInfoManager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER, s_PlayerInfoManager);
