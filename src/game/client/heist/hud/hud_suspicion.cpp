#include "cbase.h"
#include "hudelement.h"
#include "hud_element_helper.h"
#include "vgui_controls/Panel.h"
#include "iclientmode.h"
#include "suspicioner.h"
#include "heist_gamerules.h"
#include "tier1/utlmap.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudSuspicion : public CHudElement, public vgui::Panel
{
	typedef vgui::Panel BasePanel;

public:
	IMPLEMENT_OPERATORS_NEW_AND_DELETE();

	CHudSuspicion(const char *pElementName);

	void OnThink() override;
	bool ShouldDraw() override;

	class SuspicionMeter : public vgui::Panel
	{
		friend class CHudSuspicion;

	public:
		DECLARE_CLASS(SuspicionMeter, vgui::Panel)

		SuspicionMeter();

		void PerformLayout() override;

	private:
		EHANDLE m_hEntity;
	};

private:
	CUtlMap<EHANDLE, SuspicionMeter *> m_Meters;
};

DECLARE_HUDELEMENT_DEPTH(CHudSuspicion, 1)

CHudSuspicion::CHudSuspicion(const char *pElementName)
	: CHudElement(pElementName), BasePanel(NULL, pElementName)
{
	vgui::Panel *pParent = GetClientMode()->GetViewport();
	SetParent(pParent);
	SetScheme("ClientScheme");
}

bool CHudSuspicion::ShouldDraw()
{
	if(!CHudElement::ShouldDraw()) {
		return false;
	}

	if(HeistGameRules() && HeistGameRules()->AnyoneSpotted()) {
		return false;
	}

	return true;
}

CHudSuspicion::SuspicionMeter::SuspicionMeter()
	: BaseClass(GetClientMode()->GetViewport(), "SuspicionMeter")
{

}

void CHudSuspicion::SuspicionMeter::PerformLayout()
{
	BaseClass::PerformLayout();


}

void CHudSuspicion::OnThink()
{
	BasePanel::OnThink();

	if(HeistGameRules() && HeistGameRules()->AnyoneSpotted()) {
		return;
	}

	auto &suspicioner_list = C_Suspicioner::List();
	FOR_EACH_VEC(suspicioner_list, i) {
		const C_Suspicioner &suspicioner = *suspicioner_list[i];

		EHANDLE entity = suspicioner.GetOwner();
		if(!entity.IsValid()) {
			continue;
		}

		auto meter_idx = m_Meters.Find(entity);
		if(meter_idx == m_Meters.InvalidIndex()) {
		#if 0
			meter_idx = m_Meters.Insert(entity, 
				CREATE_PANEL(SuspicionMeter)
			);
		#endif
		}
	}
}
