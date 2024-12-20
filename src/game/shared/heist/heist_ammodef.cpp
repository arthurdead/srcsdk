#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHeistAmmoDef : public CAmmoDef
{
public:
	CHeistAmmoDef()
	{
		AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 0, 0, AMMO_NONE);
		AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, 1, 1, INFINITE_AMMO, BULLET_IMPULSE(200, 1225), AMMO_NONE );
		AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, 1, 1, INFINITE_AMMO, BULLET_IMPULSE(200, 1225), AMMO_NONE );
		AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, 1, 1, INFINITE_AMMO, BULLET_IMPULSE(400, 1200), AMMO_NONE );
	}
};

static CHeistAmmoDef s_AmmoDef;
CAmmoDef *GetAmmoDef()
{
	return &s_AmmoDef;
}
