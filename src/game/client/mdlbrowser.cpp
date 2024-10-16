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
	CMdlBrowser();
	~CMdlBrowser();
};

static CMdlBrowser *g_pMdlBrowser;

CMdlBrowser::CMdlBrowser()
	: CMDLPickerFrame( NULL, "Mdl Browser" )
{
	if(!g_pMdlBrowser)
		g_pMdlBrowser = this;

	vgui::VPANEL clientroot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( clientroot );

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

	g_pMdlBrowser = CREATE_PANEL(CMdlBrowser);
}
