#include "cbase.h"
#include "player_command.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CPlayerMove g_PlayerMove;
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}
