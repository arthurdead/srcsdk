//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( ICONSOLE_H )
#define ICONSOLE_H
#pragma once

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

abstract_class IConsole
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
};

extern IConsole *console;

#endif // ICONSOLE_H