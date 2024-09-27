//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#include <stdarg.h>
#include "gameui_util.h"
#include "strtools.h"
#include "engineinterface.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Performs a var args printf into a static return buffer
// Input  : *format - 
//			... - 
// Output : char
//-----------------------------------------------------------------------------
static char		varargs_string[5][1024];
static int curr_varargs = 0;
const char *VarArgs( const char *format, ... )
{
	va_list		argptr;
	
	va_start (argptr, format);
	Q_vsnprintf(varargs_string[curr_varargs], sizeof(varargs_string[curr_varargs]), format,argptr);
	va_end (argptr);

	const char *string = varargs_string[curr_varargs++];
	if(curr_varargs == ARRAYSIZE(varargs_string)) {
		curr_varargs = 0;
	}

	return string;	
}

void GetHudSize( int& w, int &h )
{
	vgui::surface()->GetScreenSize( w, h );

	VPANEL hudParent = enginevgui->GetPanel( PANEL_CLIENTDLL );
	if ( hudParent != vgui::INVALID_VPANEL )
	{
		vgui::ipanel()->GetSize( hudParent, w, h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: ScreenHeight returns the height of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenHeight( void )
{
	int w, h;
	GetHudSize( w, h );
	return h;
}

//-----------------------------------------------------------------------------
// Purpose: ScreenWidth returns the width of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenWidth( void )
{
	int w, h;
	GetHudSize( w, h );
	return w;
}

//-----------------------------------------------------------------------------
// Purpose: Scans player names
//Passes the player name to be checked in a KeyValues pointer
//with the keyname "name"
// - replaces '&' with '&&' so they will draw in the scoreboard
// - replaces '#' at the start of the name with '*'
//-----------------------------------------------------------------------------

void GameUI_MakeSafeName( const char *oldName, char *newName, int newNameBufSize )
{
	if ( !oldName )
	{
		newName[0] = 0;
		return;
	}

	int newpos = 0;

	for( const char *p=oldName; *p != 0 && newpos < newNameBufSize-1; p++ )
	{
		//check for a '#' char at the beginning

		/*
		if( p == oldName && *p == '#' )
				{
					newName[newpos] = '*';
					newpos++;
				}
				else */

		if( *p == '%' )
		{
			// remove % chars
			newName[newpos] = '*';
			newpos++;
		}
		else if( *p == '&' )
		{
			//insert another & after this one
			if ( newpos+2 < newNameBufSize )
			{
				newName[newpos] = '&';
				newName[newpos+1] = '&';
				newpos+=2;
			}
		}
		else
		{
			newName[newpos] = *p;
			newpos++;
		}
	}
	newName[newpos] = 0;
}
