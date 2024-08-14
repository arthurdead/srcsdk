#include "cbase.h"
#include "vgui_rootpanel_heist.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

C_HeistRootPanel *g_pRootPanel = NULL;

void VGUI_CreateClientDLLRootPanel()
{
	g_pRootPanel = new C_HeistRootPanel(enginevgui->GetPanel(PANEL_CLIENTDLL));
}

void VGUI_DestroyClientDLLRootPanel()
{
	delete g_pRootPanel;
	g_pRootPanel = NULL;
}

vgui::VPANEL VGui_GetClientDLLRootPanel()
{
	return g_pRootPanel->GetVPanel();
}

C_HeistRootPanel::C_HeistRootPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "Heist Root Panel")
{
	SetParent(parent);
	SetPaintEnabled(false);
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	SetPostChildPaintEnabled(true);

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

C_HeistRootPanel::~C_HeistRootPanel()
{
}

void C_HeistRootPanel::PostChildPaint()
{
	BaseClass::PostChildPaint();

	RenderPanelEffects();
}

void C_HeistRootPanel::RenderPanelEffects()
{
}

void C_HeistRootPanel::OnTick()
{
}

void C_HeistRootPanel::LevelInit()
{
}

void C_HeistRootPanel::LevelShutdown()
{
}
