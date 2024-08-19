#include "cbase.h"
#include "hudelement.h"
#include "hud_element_helper.h"
#include "vgui_controls/Panel.h"
#include "iclientmode.h"
#include "suspicioner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudSuspicion : public CHudElement, public vgui::Panel
{
	typedef vgui::Panel BasePanel;

public:
	IMPLEMENT_OPERATORS_NEW_AND_DELETE();

	CHudSuspicion(const char *pElementName);

	void OnThink() override;
};

DECLARE_HUDELEMENT_DEPTH(CHudSuspicion, 1)

CHudSuspicion::CHudSuspicion(const char *pElementName)
	: CHudElement(pElementName), BasePanel(NULL, pElementName)
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetScheme("ClientScheme");
}

void CHudSuspicion::OnThink()
{
	BasePanel::OnThink();

	auto &suspicioner_list = CSuspicioner::List();
	FOR_EACH_VEC(suspicioner_list, i) {
		const CSuspicioner &suspicioner = *suspicioner_list[i];

		C_BaseEntity *entity = suspicioner.GetOwner().Get();
		if(!entity) {
			continue;
		}

		
	}
}
