//========== Copyright � Valve Corporation, All rights reserved. ============
//
// Purpose: 
//
//===========================================================================

#ifndef PLAYER_VOICE_LISTENER_H
#define PLAYER_VOICE_LISTENER_H

#pragma once

#include "igamesystem.h"

class CPlayerVoiceListener : public CAutoGameSystem
{
public:
	CPlayerVoiceListener( void );

	// Auto-game cleanup
	virtual void LevelInitPreEntity( void );
	virtual void LevelShutdownPreEntity( void );

	bool IsPlayerSpeaking( int nPlayerIndex );
	bool IsPlayerSpeaking( CBasePlayer *pPlayer );

	void AddPlayerSpeakTime( int nPlayerIndex );
	void AddPlayerSpeakTime( CBasePlayer *pPlayer );

	float GetPlayerSpeechDuration( int nPlayerIndex );
	float GetPlayerSpeechDuration( CBasePlayer *pPlayer );

private:
	void	InitData( void );

	float	m_flLastPlayerSpeechTime[MAX_PLAYERS];
	float	m_flPlayerSpeechDuration[MAX_PLAYERS];
};

extern CPlayerVoiceListener &PlayerVoiceListener( void );

#endif // PLAYER_VOICE_LISTENER_H