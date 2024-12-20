//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic header for using vgui
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_H
#define VGUI_H

#pragma once

#ifdef NULL
#undef NULL
#endif
#define NULL    nullptr

namespace std
{
	using nullptr_t = decltype(nullptr);
}

#pragma warning( disable: 4800 )	// disables 'performance warning converting int to bool'
#pragma warning( disable: 4786 )	// disables 'identifier truncated in browser information' warning
#pragma warning( disable: 4355 )	// disables 'this' : used in base member initializer list
#pragma warning( disable: 4097 )	// warning C4097: typedef-name 'BaseClass' used as synonym for class-name
#pragma warning( disable: 4514 )	// warning C4514: 'Color::Color' : unreferenced inline function has been removed
#pragma warning( disable: 4100 )	// warning C4100: 'code' : unreferenced formal parameter
#pragma warning( disable: 4127 )	// warning C4127: conditional expression is constant

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#ifndef _WCHAR_T_DEFINED
// DAL - wchar_t is a built in define in gcc 3.2 with a size of 4 bytes
#if !defined( __x86_64__ ) && !defined( __WCHAR_TYPE__  )
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif

// do this in GOLDSRC only!!!
//#define Assert assert

namespace vgui
{
// handle to an internal vgui panel
// this is the only handle to a panel that is valid across dll boundaries
enum VPANEL: unsigned int;
const VPANEL INVALID_VPANEL = (VPANEL)0;

// handles to vgui objects
// NULL values signify an invalid value
enum HScheme : unsigned long;
const HScheme INVALID_SCHEME = (HScheme)0;
// Both -1 and 0 are used for invalid textures. Be careful.
enum HTexture : int;
const HTexture INVALID_TEXTURE = (HTexture)-1;
enum HCursor: unsigned long;
const HCursor INVALID_CURSOR = (HCursor)0;
enum HPanel: unsigned long;
const HPanel INVALID_PANEL = (HPanel)0xffffffff;
enum HFont: unsigned long;
const HFont INVALID_FONT = (HFont)0; // the value of an invalid font handle
}

inline bool operator==(vgui::VPANEL rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_VPANEL; }
inline bool operator!=(vgui::VPANEL rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_VPANEL; }
inline bool operator==(std::nullptr_t, vgui::VPANEL rhs)
{ return vgui::INVALID_VPANEL == rhs; }
inline bool operator!=(std::nullptr_t, vgui::VPANEL rhs)
{ return vgui::INVALID_VPANEL != rhs; }

inline bool operator==(vgui::HScheme rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_SCHEME; }
inline bool operator!=(vgui::HScheme rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_SCHEME; }
inline bool operator==(std::nullptr_t, vgui::HScheme rhs)
{ return vgui::INVALID_SCHEME == rhs; }
inline bool operator!=(std::nullptr_t, vgui::HScheme rhs)
{ return vgui::INVALID_SCHEME != rhs; }

inline bool operator==(vgui::HPanel rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_PANEL; }
inline bool operator!=(vgui::HPanel rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_PANEL; }
inline bool operator==(std::nullptr_t, vgui::HPanel rhs)
{ return vgui::INVALID_PANEL == rhs; }
inline bool operator!=(std::nullptr_t, vgui::HPanel rhs)
{ return vgui::INVALID_PANEL != rhs; }

inline bool operator==(vgui::HCursor rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_CURSOR; }
inline bool operator!=(vgui::HCursor rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_CURSOR; }
inline bool operator==(std::nullptr_t, vgui::HCursor rhs)
{ return vgui::INVALID_CURSOR == rhs; }
inline bool operator!=(std::nullptr_t, vgui::HCursor rhs)
{ return vgui::INVALID_CURSOR != rhs; }

inline bool operator==(vgui::HFont rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_FONT; }
inline bool operator!=(vgui::HFont rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_FONT; }
inline bool operator==(std::nullptr_t, vgui::HFont rhs)
{ return vgui::INVALID_FONT == rhs; }
inline bool operator!=(std::nullptr_t, vgui::HFont rhs)
{ return vgui::INVALID_FONT != rhs; }

inline bool operator==(vgui::HTexture rhs, std::nullptr_t)
{ return rhs == vgui::INVALID_TEXTURE; }
inline bool operator!=(vgui::HTexture rhs, std::nullptr_t)
{ return rhs != vgui::INVALID_TEXTURE; }
inline bool operator==(std::nullptr_t, vgui::HTexture rhs)
{ return vgui::INVALID_TEXTURE == rhs; }
inline bool operator!=(std::nullptr_t, vgui::HTexture rhs)
{ return vgui::INVALID_TEXTURE != rhs; }

#include "tier1/strtools.h"

#if 0 // defined( OSX ) // || defined( LINUX )
// Disabled all platforms. Did a major cleanup of osxfont.cpp, and having this
//  turned off renders much closer to Windows and Linux and also uses the same
//  code paths (which is good).
#define USE_GETKERNEDCHARWIDTH 1
#else
#define USE_GETKERNEDCHARWIDTH 0
#endif


#endif // VGUI_H
