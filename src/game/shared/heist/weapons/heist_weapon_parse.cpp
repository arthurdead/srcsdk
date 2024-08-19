#include "cbase.h"
#include "heist_weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CHeistWeaponInfo;
}

void CHeistWeaponInfo::Parse(KeyValues *pKeyValuesData, const char *szWeaponName)
{
	BaseClass::Parse(pKeyValuesData, szWeaponName);
}
