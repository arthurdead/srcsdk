#ifndef HL2MP_WEAPON_PARSE_H
#define HL2MP_WEAPON_PARSE_H

#pragma once

#include "weapon_parse.h"

class CHeistWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT(CHeistWeaponInfo, FileWeaponInfo_t);

	virtual void Parse(KeyValues *pKeyValuesData, const char *szWeaponName);
};

#endif
