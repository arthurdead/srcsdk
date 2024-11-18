#ifndef IN_BUTTONS_HEIST_H
#define IN_BUTTONS_HEIST_H
#pragma once

#include "in_buttons.h"

enum : uint64
{
	IN_LEAN_LEFT =  (IN_LAST_SHARED_BUTTON << 1),
	IN_LEAN_RIGHT = (IN_LAST_SHARED_BUTTON << 2),
};

#endif
