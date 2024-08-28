#include "cbase.h"
#include "input.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CInput g_Input;
IInput *input = ( IInput * )&g_Input;
