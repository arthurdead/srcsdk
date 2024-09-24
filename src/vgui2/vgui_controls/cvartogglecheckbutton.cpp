//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier1/KeyValues.h"

#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui_controls/cvartogglecheckbutton.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

template class vgui::CvarToggleCheckButton<ConVarRef>;

vgui::Panel *Create_CvarToggleCheckButton()
{
	return new CvarToggleCheckButton< ConVarRef >( NULL, NULL );
}

DECLARE_BUILD_FACTORY_CUSTOM_ALIAS( CvarToggleCheckButton<ConVarRef>, CvarToggleCheckButton, Create_CvarToggleCheckButton );

vgui::Panel *CvarToggleCheckButton_Factory()
{
	return new CCvarToggleCheckButton( NULL, NULL, "CvarToggleCheckButton", NULL );
}
DECLARE_BUILD_FACTORY_CUSTOM( CCvarToggleCheckButton, CvarToggleCheckButton_Factory );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
BaseCvarToggleCheckButton::BaseCvarToggleCheckButton( Panel *parent, const char *panelName, const char *text, char const *cvarname, bool ignoreMissingCvar )
	: CheckButton( parent, panelName, text )
{
	m_pszCvarName = cvarname ? strdup( cvarname ) : NULL;
	m_bIgnoreMissingCvar = ignoreMissingCvar;

	AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
BaseCvarToggleCheckButton::~BaseCvarToggleCheckButton()
{
	if ( m_pszCvarName )
	{
		free( m_pszCvarName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::Paint()
{
	if ( !IsCvarValid() ) 
	{
		BaseClass::Paint();
		return;
	}

	bool value = GetCvarBool();

	if ( value != m_bStartValue )
	{
		SetSelected( value );
		m_bStartValue = value;
	}
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into cvar.
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::OnApplyChanges()
{
	ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::ApplyChanges()
{
	if ( !IsCvarValid() ) 
		return;

	m_bStartValue = IsSelected();
	SetCvarBool( m_bStartValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::Reset()
{
	if ( !IsCvarValid() ) 
		return;

	m_bStartValue = GetCvarBool();
	SetSelected(m_bStartValue);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool BaseCvarToggleCheckButton::HasBeenModified()
{
	return IsSelected() != m_bStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::SetSelected( bool state )
{
	BaseClass::SetSelected( state );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::OnButtonChecked()
{
	if (HasBeenModified())
	{
		PostActionSignal(new KeyValues("ControlModified"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BaseCvarToggleCheckButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *cvarName = inResourceData->GetString("cvar_name", "");
	const char *cvarValue = inResourceData->GetString("cvar_value", "");

	if( Q_stricmp( cvarName, "") == 0 )
		return;// Doesn't have cvar set up in res file, must have been constructed with it.

	if( m_pszCvarName )
		free( m_pszCvarName );// got a "", not a NULL from the create-control call

	m_pszCvarName = cvarName ? strdup( cvarName ) : NULL;

	if( Q_stricmp( cvarValue, "1") == 0 )
		m_bStartValue = true;
	else
		m_bStartValue = false;

	InitCvar( cvarName, m_bIgnoreMissingCvar );
	if ( IsCvarValid() )
	{
		SetSelected( GetCvarBool() );
	}
}
