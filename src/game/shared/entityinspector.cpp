#include "cbase.h"
#include "ienginevgui.h"
#include "vgui/IVGui.h"
#include "vgui/IPanel.h"
#include "vgui_controls/Frame.h"
#include "toolframework_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CEntityInspector : public vgui::Frame
{
public:
	CEntityInspector( vgui::VPANEL parent );
	~CEntityInspector();
};

static CEntityInspector *g_pEntityInspector = NULL;

CEntityInspector::CEntityInspector( vgui::VPANEL parent )
	: vgui::Frame( parent, "CEntityInspector" )
{
	if(!g_pEntityInspector)
		g_pEntityInspector = this;

	LoadControlSettings( "resource/entityinspector.res" );

	SetAlpha( 255 );

	SetSize( 1230, 740 );

	SetDeleteSelfOnClose( true );

#ifdef GAME_DLL
	SetTitle( "Entity Inspector Server", true );
#else
	SetTitle( "Entity Inspector Client", true );
#endif

	SetVisible( true );
	MoveToFront();
	SetZPos( 1500 );
	SetMouseInputEnabled( true );

	ActivateBuildMode();
}

CEntityInspector::~CEntityInspector()
{
	if(g_pEntityInspector == this)
		g_pEntityInspector = NULL;
}

CON_COMMAND_SHARED(entityinspector, "")
{
#ifdef GAME_DLL
	if(engine->IsDedicatedServer()) {
		return;
	}

	int idx = UTIL_GetCommandClientIndex();
	if(idx != 0) {
		return;
	}
#endif

	if(g_pEntityInspector) {
		g_pEntityInspector->MoveToFront();
		return;
	}

	vgui::VPANEL parent = enginevgui->GetPanel( PANEL_CLIENTDLL );

	g_pEntityInspector = CREATE_PANEL(CEntityInspector, parent);
}
