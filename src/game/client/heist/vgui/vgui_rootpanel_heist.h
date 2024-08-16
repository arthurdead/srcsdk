#ifndef VGUI_ROOTPANEL_HEIST_H
#define VGUI_ROOTPANEL_HEIST_H

#pragma once

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "utlvector.h"

class CPanelEffect;

typedef unsigned int EFFECT_HANDLE;

class C_HeistRootPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;

public:
	C_HeistRootPanel(vgui::VPANEL parent);
	~C_HeistRootPanel();

	void PostChildPaint() override;

	void LevelInit();
	void LevelShutdown();

	void OnTick() override;

private:
	void RenderPanelEffects();

	CUtlVector<CPanelEffect *> m_Effects;
};

#endif
