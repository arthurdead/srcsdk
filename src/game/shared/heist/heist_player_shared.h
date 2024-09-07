#ifndef HEIST_PLAYER_SHARED_H
#define HEIST_PLAYER_SHARED_H

#pragma once

#ifdef GAME_DLL
class CHeistPlayer;
#else
class C_HeistPlayer;
#define CHeistPlayer C_HeistPlayer
#endif

#endif
