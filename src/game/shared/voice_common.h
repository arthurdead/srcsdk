//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VOICE_COMMON_H
#define VOICE_COMMON_H
#pragma once


#include "bitvec.h"
#include "const.h"
#include "shareddefs.h"


#define VOICE_MAX_PLAYERS		MAX_PLAYERS
#define VOICE_MAX_PLAYERS_DW	((VOICE_MAX_PLAYERS / 32) + !!(VOICE_MAX_PLAYERS & 31))

typedef CBitVec<VOICE_MAX_PLAYERS> CVoicePlayerBitVec;

#define VOICE_DEFAULT_PROXIMITY_RANGE 1200 //100 feet


#endif // VOICE_COMMON_H
