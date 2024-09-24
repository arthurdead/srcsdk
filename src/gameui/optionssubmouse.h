//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONS_SUB_MOUSE_H
#define OPTIONS_SUB_MOUSE_H
#pragma once

#include <vgui_controls/PropertyPage.h>
#include "gameui_cvartogglecheckbutton.h"

class CCvarNegateCheckButton;
class CKeyToggleCheckButton;
class CCvarSlider;

namespace vgui
{
    class Label;
    class Panel;
}

//-----------------------------------------------------------------------------
// Purpose: Mouse Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class COptionsSubMouse : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubMouse, vgui::PropertyPage );

public:
	COptionsSubMouse(vgui::Panel *parent);
	~COptionsSubMouse();

	virtual void OnResetData();
	virtual void OnApplyChanges();

protected:
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
    MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel )
	{
		OnControlModified( panel );
	}

    void UpdateSensitivityLabel();
	void UpdateJoystickPanels();

	CCvarNegateCheckButton		*m_pReverseMouseCheckBox;
	CGameUICvarToggleCheckButton		*m_pMouseFilterCheckBox;
	CGameUICvarToggleCheckButton		*m_pJoystickCheckBox;
	CGameUICvarToggleCheckButton		*m_pJoystickSouthpawCheckBox;
	CGameUICvarToggleCheckButton		*m_pQuickInfoCheckBox;
	CGameUICvarToggleCheckButton		*m_pReverseJoystickCheckBox;

	CCvarSlider					*m_pMouseSensitivitySlider;
    vgui::TextEntry             *m_pMouseSensitivityLabel;

	CCvarSlider					*m_pJoyYawSensitivitySlider;
	vgui::Label					*m_pJoyYawSensitivityPreLabel;
	CCvarSlider					*m_pJoyPitchSensitivitySlider;
	vgui::Label					*m_pJoyPitchSensitivityPreLabel;
};



#endif // OPTIONS_SUB_MOUSE_H