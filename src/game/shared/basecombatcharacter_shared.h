#ifndef BASECOMBATCHARACTER_SHARED_H
#define BASECOMBATCHARACTER_SHARED_H

#pragma once

enum Disposition_t : unsigned char
{
	D_ER,		// Undefined - error
	D_HT,		// Hate
	D_FR,		// Fear
	D_LI,		// Like
	D_NU,		// Neutral

	// The following are duplicates of the above, only with friendlier names
	D_ERROR = D_ER,		// Undefined - error
	D_HATE = D_HT,		// Hate
	D_FEAR = D_FR,		// Fear
	D_LIKE = D_LI,		// Like
	D_NEUTRAL = D_NU,	// Neutral
};

enum WeaponSwitchResult_t : unsigned char
{
	WEAPON_SWITCH_FAILED,
	WEAPON_SWITCH_HOLSTERED,
	WEAPON_SWITCH_DEPLOYED,
};

enum FieldOfViewCheckType : unsigned char
{
	USE_FOV,
	DISREGARD_FOV
};

#endif
