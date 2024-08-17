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
#if !defined( IDEBUGOVERLAYPANEL_H )
#define IDEBUGOVERLAYPANEL_H
#pragma once

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

abstract_class IDebugOverlayPanel
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
};

extern IDebugOverlayPanel *debugoverlaypanel;

#endif // IDEBUGOVERLAYPANEL_H