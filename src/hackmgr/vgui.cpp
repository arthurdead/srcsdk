#include "vgui/ISurface.h"
#include "hackmgr/hackmgr.h"
#include "vgui/IPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class IPanelInternal : public vgui::IPanel
{
public:
	virtual vgui::IClientPanel *Client(vgui::VPANEL vguiPanel) = 0;

	virtual const char *GetModuleName(vgui::VPANEL vguiPanel) = 0;
};

void vgui::ISurface::DrawTexturedRectEx( vgui::DrawTexturedRectParms_t *pDrawParms )
{
	DrawTexturedSubRect(
		pDrawParms->x0,
		pDrawParms->y0,
		pDrawParms->x1,
		pDrawParms->y1,
		pDrawParms->s0,
		pDrawParms->t0,
		pDrawParms->s1,
		pDrawParms->t1
	);
}

vgui::IClientPanel *vgui::IPanel::Client(vgui::VPANEL vguiPanel)
{
	return ((IPanelInternal *)this)->Client(vguiPanel);
}

const char *vgui::IPanel::GetModuleName(vgui::VPANEL vguiPanel)
{
	return ((IPanelInternal *)this)->GetModuleName(vguiPanel);
}
