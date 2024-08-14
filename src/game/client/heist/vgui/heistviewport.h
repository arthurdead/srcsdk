#ifndef HEISTVIEWPORT_H
#define HEISTVIEWPORT_H

#pragma once

#include "baseviewport.h"

using namespace vgui;

namespace vgui 
{
	class Panel;
}

class HeistViewport : public CBaseViewport
{
	DECLARE_CLASS_SIMPLE(HeistViewport, CBaseViewport);

public:
	IViewPortPanel *CreatePanelByName(const char *szPanelName) override;
	void CreateDefaultPanels() override;

	void ApplySchemeSettings(vgui::IScheme *pScheme) override;

	int GetDeathMessageStartHeight() override;
};

#endif
