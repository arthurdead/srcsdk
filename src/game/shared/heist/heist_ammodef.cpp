#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHeistAmmoDef : public CAmmoDef
{
public:
	CHeistAmmoDef()
	{
		AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 8, 0, 0);
	}
};

static CHeistAmmoDef s_AmmoDef;
CAmmoDef *GetAmmoDef()
{
	return &s_AmmoDef;
}
