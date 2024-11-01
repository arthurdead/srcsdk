#include "tier0/platform.h"

#define PANEL_H
#define EDITABLEPANEL_H
#define LABEL_H
#define TREEVIEW_H
#define VGUI_FRAME_H
#include "tier1/utlflags.h"
#include "vgui/VGUI.h"
#include "vgui/Dar.h"
#include "vgui_controls/MessageMap.h"
#include "vgui_controls/KeyBindingMap.h"
#include "vgui/IClientPanel.h"
#include "vgui/IScheme.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/PanelAnimationVar.h"
#include "Color.h"
#include "vstdlib/IKeyValuesSystem.h"
#include "tier1/utlsymbol.h"
#include "vgui_controls/BuildGroup.h"
#include <vgui_controls/FocusNavGroup.h>

#undef PANEL_H
#define protected public
#include "vgui_controls/Panel.h"
#undef protected

#undef LABEL_H
#define protected public
#include "vgui_controls/Label.h"
#undef protected

#undef TREEVIEW_H
#define protected public
#include "vgui_controls/TreeView.h"
#undef protected

#undef EDITABLEPANEL_H
#define protected public
#include "vgui_controls/EditablePanel.h"
#undef protected

#undef VGUI_FRAME_H
#define protected public
#include "vgui_controls/Frame.h"
#undef protected

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef __GNUC__
extern "C" {
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel12OnChildAddedEj(vgui::Panel *pthis, unsigned int pane)
	{ pthis->Panel::OnChildAdded((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY bool THISCALL _ZN4vgui5Panel16RequestFocusPrevEj(vgui::Panel *pthis, unsigned int pane)
	{ return pthis->Panel::RequestFocusPrev((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY bool THISCALL _ZN4vgui5Panel16RequestFocusNextEj(vgui::Panel *pthis, unsigned int pane)
	{ return pthis->Panel::RequestFocusNext((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel9OnMessageEPK9KeyValuesj(vgui::Panel *pthis, const KeyValues *msg, unsigned int pane)
	{ pthis->Panel::OnMessage(msg, (vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel11PostMessageEjP9KeyValuesf(vgui::Panel *pthis, unsigned int pane, KeyValues *msg, float delay)
	{ pthis->Panel::PostMessage((vgui::VPANEL)pane, msg, delay); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel9SetParentEj(vgui::Panel *pthis, unsigned int pane)
	{ pthis->Panel::SetParent((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY bool THISCALL _ZN4vgui5Panel9HasParentEj(vgui::Panel *pthis, unsigned int pane)
	{ return pthis->Panel::HasParent((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel21AddActionSignalTargetEj(vgui::Panel *pthis, unsigned int pane)
	{ pthis->Panel::AddActionSignalTarget((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel9SetCursorEm(vgui::Panel *pthis, unsigned long pane)
	{ pthis->Panel::SetCursor((vgui::HCursor)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel9SetSchemeEm(vgui::Panel *pthis, unsigned long pane)
	{ pthis->Panel::SetScheme((vgui::HScheme)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Panel14OnRequestFocusEjj(vgui::Panel *pthis, unsigned int pane, unsigned int pane2)
	{ pthis->Panel::OnRequestFocus((vgui::VPANEL)pane, (vgui::VPANEL)pane2); }

	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui13EditablePanel12OnChildAddedEj(vgui::EditablePanel *pthis, unsigned int pane)
	{ pthis->EditablePanel::OnChildAdded((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY bool THISCALL _ZN4vgui13EditablePanel16RequestFocusPrevEj(vgui::EditablePanel *pthis, unsigned int pane)
	{ return pthis->EditablePanel::RequestFocusPrev((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY bool THISCALL _ZN4vgui13EditablePanel16RequestFocusNextEj(vgui::EditablePanel *pthis, unsigned int pane)
	{ return pthis->EditablePanel::RequestFocusNext((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui13EditablePanel14OnRequestFocusEjj(vgui::EditablePanel *pthis, unsigned int pane,  unsigned int pane2)
	{ pthis->EditablePanel::OnRequestFocus((vgui::VPANEL)pane, (vgui::VPANEL)pane2); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui13EditablePanel18OnDefaultButtonSetEj(vgui::EditablePanel *pthis, unsigned int pane)
	{ pthis->EditablePanel::OnDefaultButtonSet((vgui::VPANEL)pane); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui13EditablePanel25OnCurrentDefaultButtonSetEj(vgui::EditablePanel *pthis, unsigned int pane)
	{ pthis->EditablePanel::OnCurrentDefaultButtonSet((vgui::VPANEL)pane); }

	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Label7SetFontEm(vgui::Label *pthis, unsigned long fnt)
	{ pthis->Label::SetFont((vgui::HFont)fnt); }
	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Label14OnRequestFocusEjj(vgui::Label *pthis, unsigned int pane, unsigned int pane2)
	{ pthis->Label::OnRequestFocus((vgui::VPANEL)pane, (vgui::VPANEL)pane2); }

	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui8TreeView7SetFontEm(vgui::TreeView *pthis, unsigned long fnt)
	{ pthis->TreeView::SetFont((vgui::HFont)fnt); }

	LIB_LOCAL SELECTANY void THISCALL _ZN4vgui5Frame12OnChildAddedEj(vgui::Frame *pthis, unsigned int pane)
	{ pthis->Frame::OnChildAdded((vgui::VPANEL)pane); }
}
#endif
