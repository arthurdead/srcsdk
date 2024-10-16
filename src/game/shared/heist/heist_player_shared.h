#ifndef HEIST_PLAYER_SHARED_H
#define HEIST_PLAYER_SHARED_H

#pragma once

#ifdef GAME_DLL
class CHeistPlayer;
typedef CHeistPlayer CSharedHeistPlayer;
#else
class C_HeistPlayer;
typedef C_HeistPlayer CSharedHeistPlayer;
#endif

#endif
