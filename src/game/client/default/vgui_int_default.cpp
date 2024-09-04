#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void VGUI_CreateClientDLLRootPanel( void )
{
	
}

void VGUI_DestroyClientDLLRootPanel( void )
{
	
}

vgui::VPANEL VGui_GetClientDLLRootPanel( void )
{
	vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
	return root;
}

vgui::Panel *VGui_GetFullscreenRootPanel( void )
{
	return NULL;
}

vgui::VPANEL VGui_GetFullscreenRootVPANEL( void )
{
	vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
	return root;
}

void VGui_GetPanelList( CUtlVector< vgui::Panel * > &list )
{
	
}
