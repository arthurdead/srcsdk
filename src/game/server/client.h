//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef CLIENT_H
#define CLIENT_H

#pragma once


class CCommand;
class CUserCmd;
class CBasePlayer;
struct edict_t;
class CBaseEntity;

void ClientActive( edict_t *pEdict );
void ClientPutInServer( edict_t *pEdict, const char *playername );
void ClientFullyConnect( edict_t *pEdict );
void ClientCommand( CBasePlayer *pSender, const CCommand &args );
void ClientPrecache( void );
// Game specific precaches
void ClientGamePrecache( void );
const char *GetGameDescription( void );
void Host_Say( edict_t *pEdict, bool teamonly );
void respawn(CBaseEntity *pEdict, bool fCopyCorpse);


#endif		// CLIENT_H
