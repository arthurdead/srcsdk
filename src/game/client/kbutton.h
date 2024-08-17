//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( KBUTTON_H )
#define KBUTTON_H
#pragma once

struct kbutton_t
{
	// key nums holding it down
	int		down[ 2 ];		
	// low bit is down state
	int		state;			
};

#endif // KBUTTON_H