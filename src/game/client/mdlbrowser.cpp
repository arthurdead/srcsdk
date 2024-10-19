#include "cbase.h"
#include "matsys_controls/mdlpicker.h"
#include "ienginevgui.h"
#include "vgui/IVGui.h"
#include "vgui/IPanel.h"
#include "cdll_client_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CMdlBrowser : public CMDLPickerFrame
{
public:
	CMdlBrowser( vgui::VPANEL parent );
	~CMdlBrowser();
};

static CMdlBrowser *g_pMdlBrowser = NULL;

CMdlBrowser::CMdlBrowser( vgui::VPANEL parent )
	: CMDLPickerFrame( parent, "Mdl Browser" )
{
	if(!g_pMdlBrowser)
		g_pMdlBrowser = this;

	SetSize( 1230, 740 );

	SetDeleteSelfOnClose( true );
	SetVisible( true );
	MoveToFront();
	SetZPos( 1500 );
	SetMouseInputEnabled( true );
}

CMdlBrowser::~CMdlBrowser()
{
	if(g_pMdlBrowser == this)
		g_pMdlBrowser = NULL;
}

CON_COMMAND(mdlbrowser, "")
{
	if(g_pMdlBrowser) {
		g_pMdlBrowser->MoveToFront();
		return;
	}

	vgui::VPANEL clientroot = enginevgui->GetPanel( PANEL_GAMEUIDLL );

	g_pMdlBrowser = CREATE_PANEL(CMdlBrowser, clientroot);
}
