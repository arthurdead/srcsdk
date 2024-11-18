#include "cbase.h"
#include "input.h"
#include "in_buttons_heist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CInput g_Input;
IInput *input = ( IInput * )&g_Input;

DECLARE_KEY_PRESS_COMMAND_NAMED(LEAN_LEFT, leanleft)
DECLARE_KEY_PRESS_COMMAND_NAMED(LEAN_RIGHT, leanright)

void CInput::CalcModButtonBits(uint64 &bits, bool bResetState)
{
	CALC_KEY_PRESS(LEAN_LEFT)
	CALC_KEY_PRESS(LEAN_RIGHT)
}
