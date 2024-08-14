#include "cbase.h"
#include "heist_weapon_parse.h"

FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CHeistWeaponInfo;
}

void CHeistWeaponInfo::Parse(KeyValues *pKeyValuesData, const char *szWeaponName)
{
	BaseClass::Parse(pKeyValuesData, szWeaponName);
}
