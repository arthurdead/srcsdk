//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CVARTOGGLECHECKBUTTON_H
#define CVARTOGGLECHECKBUTTON_H
#pragma once

#include "vgui/VGUI.h"
#include "vgui_controls/CheckButton.h"
#include "tier1/utlstring.h"
#include "tier1/KeyValues.h"
#include "tier1/convar.h"

namespace vgui
{

class BaseCvarToggleCheckButton : public CheckButton
{
	DECLARE_CLASS_SIMPLE( BaseCvarToggleCheckButton, CheckButton );

public:
	BaseCvarToggleCheckButton( Panel *parent, const char *panelName, const char *text = "", 
		char const *cvarname = NULL, bool ignoreMissingCvar = false );
	~BaseCvarToggleCheckButton();

	virtual void	SetSelected( bool state );

	virtual void	Paint();

	void			Reset();
	void			ApplyChanges();
	bool			HasBeenModified();
	virtual void	ApplySettings( KeyValues *inResourceData );

	virtual bool IsCvarValid() const = 0;
	virtual bool GetCvarBool() const = 0;
	virtual void SetCvarBool(bool value) = 0;
	virtual void InitCvar(const char *name, bool ignoremissing) = 0;

private:
	void InitCvar()
	{ InitCvar(m_pszCvarName, m_bIgnoreMissingCvar); }

	// Called when the OK / Apply button is pressed.  Changed data should be written into cvar.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );
	MESSAGE_FUNC( OnButtonChecked, "CheckButtonChecked" );

	char			*m_pszCvarName;
	bool			m_bStartValue;
	bool			m_bIgnoreMissingCvar;
};

template <typename T>
class CvarToggleCheckButton : public BaseCvarToggleCheckButton
{
	DECLARE_CLASS_SIMPLE( CvarToggleCheckButton, BaseCvarToggleCheckButton );

public:
	CvarToggleCheckButton( Panel *parent, const char *panelName, const char *text = "", 
		char const *cvarname = NULL, bool ignoreMissingCvar = false )
		: BaseCvarToggleCheckButton( parent, panelName, text, cvarname, ignoreMissingCvar ), m_cvar( (cvarname)?cvarname:"", (cvarname)?ignoreMissingCvar:true )
	{
		if (IsCvarValid())
		{
			Reset();
		}
	}

	virtual bool IsCvarValid() const
	{ return m_cvar.IsValid(); }
	virtual bool GetCvarBool() const
	{ return m_cvar.GetBool(); }
	virtual void SetCvarBool(bool value)
	{ m_cvar.SetValue( value); }
	virtual void InitCvar(const char *name, bool ignoremissing)
	{ m_cvar.Init(name, ignoremissing); }

private:
	T			m_cvar;
};

extern template class CvarToggleCheckButton<ConVarRef>;

} // namespace vgui

typedef vgui::CvarToggleCheckButton<ConVarRef> CCvarToggleCheckButton;

#endif // CVARTOGGLECHECKBUTTON_H
