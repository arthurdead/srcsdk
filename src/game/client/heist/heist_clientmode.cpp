#include "cbase.h"
#include "clientmode_shared.h"
#include "weapon_selection.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHeistClientMode : public ClientModeShared
{
public:
	DECLARE_CLASS(CHeistClientMode, ClientModeShared)

	void InitWeaponSelectionHudElement() override;
};

static CHeistClientMode s_ClientMode;

IClientMode *GetClientMode()
{
	return &s_ClientMode;
}

IClientMode *GetFullscreenClientMode()
{
	return &s_ClientMode;
}

IClientMode *GetClientModeNormal()
{
	return &s_ClientMode;
}

void CHeistClientMode::InitWeaponSelectionHudElement()
{
	BaseClass::InitWeaponSelectionHudElement();

	CBaseHudWeaponSelection *pWeaponSelection = GetWeaponSelection();
	if(pWeaponSelection) {
		pWeaponSelection->RemoveHiddenBits( HIDEHUD_NEEDSUIT );
	}
}
