#include "cbase.h"
#include "heistviewport.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void HeistViewport::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	gHUD.InitColors(pScheme);

	SetPaintBackgroundEnabled( false );
}

IViewPortPanel *HeistViewport::CreatePanelByName(const char *szPanelName)
{
	IViewPortPanel* newpanel = NULL;

	newpanel = BaseClass::CreatePanelByName(szPanelName);

	return newpanel; 
}

void HeistViewport::CreateDefaultPanels()
{
	BaseClass::CreateDefaultPanels();
}

int HeistViewport::GetDeathMessageStartHeight()
{
	int x = YRES(2);

	IViewPortPanel *spectator = gViewPortInterface->FindPanelByName(PANEL_SPECGUI);
	if(spectator && spectator->IsVisible()) {
		x += YRES(52);
	}

	return x;
}
