//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENT_TEXTMESSAGE_H
#define CLIENT_TEXTMESSAGE_H
#pragma once

#include "Color.h"

struct client_textmessage_t
{
	int		effect;
	color32 clr1;		// 2 colors for effects
	color32 clr2;
	float	x;
	float	y;
	float	fadein;
	float	fadeout;
	float	holdtime;
	float	fxtime;
	const char *pVGuiSchemeFontName; // If null, use default font for messages
	const char *pName;
	const char *pMessage;
	bool    bRoundedRectBackdropBox;
	float	flBoxSize; // as a function of font height
	color32	boxcolor;
	char const *pClearMessage; // message to clear
};

#endif // CLIENT_TEXTMESSAGE_H
