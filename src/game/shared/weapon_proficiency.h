//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_PROFICIENCY_H
#define WEAPON_PROFICIENCY_H

#pragma once

struct WeaponProficiencyInfo_t
{
	float	spreadscale;
	float	bias;
};

enum WeaponProficiency_t : unsigned char
{
	// For the override
	WEAPON_PROFICIENCY_INVALID = (unsigned char)-1,
	WEAPON_PROFICIENCY_POOR = 0,
	WEAPON_PROFICIENCY_AVERAGE,
	WEAPON_PROFICIENCY_GOOD,
	WEAPON_PROFICIENCY_VERY_GOOD,
	WEAPON_PROFICIENCY_PERFECT,
};

const char *GetWeaponProficiencyName( WeaponProficiency_t proficiency );


#endif // WEAPON_PROFICIENCY_H
