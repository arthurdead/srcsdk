//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the enumerated list of default cursors
//
// $NoKeywords: $
//=============================================================================//

#ifndef CURSOR_H
#define CURSOR_H

#pragma once

#include <vgui/VGUI.h>

namespace vgui
{

enum CursorCode
{
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_waitarrow,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_blank, // don't show any custom vgui cursor, just let windows do it stuff (for HTML widget)
	dc_last,
	dc_alwaysvisible_push,
	dc_alwaysvisible_pop,
};

inline HCursor CursorCodeToCursor(CursorCode code)
{
	return (HCursor)code;
}

}

inline bool operator==(vgui::HCursor rhs, vgui::CursorCode lhs)
{ return rhs == vgui::CursorCodeToCursor(lhs); }
inline bool operator!=(vgui::HCursor rhs, vgui::CursorCode lhs)
{ return rhs != vgui::CursorCodeToCursor(lhs); }
inline bool operator==(vgui::CursorCode lhs, vgui::HCursor rhs)
{ return vgui::CursorCodeToCursor(lhs) == rhs; }
inline bool operator!=(vgui::CursorCode lhs, vgui::HCursor rhs)
{ return vgui::CursorCodeToCursor(lhs)!= rhs; }

#endif // CURSOR_H
