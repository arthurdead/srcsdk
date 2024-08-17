//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IHUDLCD_H
#define IHUDLCD_H
#pragma once

class IHudLCD
{
public:
	virtual void	SetGlobalStat( char const *name, char const *value ) = 0;
	virtual void	AddChatLine( char const *txt ) = 0;
};

extern IHudLCD *hudlcd;

#endif // IHUDLCD_H
