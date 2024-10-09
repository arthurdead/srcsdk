#ifndef RMLCONTEXT_H
#define RMLCONTEXT_H

#pragma once

#include <vgui/IClientPanel.h>
#include <tier1/utlvector.h>

#pragma push_macro("Assert")
#undef Assert
#include <RmlUi/Core/Context.h>
#pragma pop_macro("Assert")

class RmlContext : public Rml::Context, public vgui::IClientPanel
{
public:
	RmlContext(const Rml::String& name, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler);
	~RmlContext();

	virtual vgui::VPANEL GetVPanel() { return m_VPanel; }

	// straight interface to Panel functions
	virtual void Think();
	virtual void PerformApplySchemeSettings() {}
	virtual void PaintTraverse(bool forceRepaint, bool allowForce);
	virtual void Repaint();
	virtual vgui::VPANEL IsWithinTraverse(int x, int y, bool traversePopups) { return vgui::INVALID_VPANEL; }
	virtual void GetInset(int &top, int &left, int &right, int &bottom) {}
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1);
	virtual void OnChildAdded(vgui::VPANEL child) {}
	virtual void OnSizeChanged(int newWide, int newTall);

	virtual void InternalFocusChanged(bool lost) {}
	virtual bool RequestInfo(KeyValues *outputData);
	virtual void RequestFocus(int direction) {}
	virtual bool RequestFocusPrev(vgui::VPANEL existingPanel) { return false; }
	virtual bool RequestFocusNext(vgui::VPANEL existingPanel) { return false; }
	virtual void OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel);
	virtual vgui::VPANEL GetCurrentKeyFocus() { return vgui::INVALID_VPANEL; }
	virtual int GetTabPosition() { return -1; }

	// for debugging purposes
	virtual const char *GetName();
	virtual const char *GetClassName() { return "RmlContext"; }

	// get scheme handles from panels
	virtual vgui::HScheme GetScheme() { return vgui::INVALID_SCHEME; }
	// gets whether or not this panel should scale with screen resolution
	virtual bool IsProportional() { return false; }
	// auto-deletion
	virtual bool IsAutoDeleteSet() { return false; }
	// deletes this
	virtual void DeletePanel();

	// interfaces
	virtual void *QueryInterface(vgui::EInterfaceID id);

	// returns a pointer to the vgui controls baseclass Panel *
	virtual vgui::Panel *GetPanel() { return NULL; }

	// returns the name of the module this panel is part of
	virtual const char *GetModuleName();

	virtual void OnTick();

private:
	vgui::VPANEL m_VPanel;
	bool m_bNeedsRepaint;
};

#endif
