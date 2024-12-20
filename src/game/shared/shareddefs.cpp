#include "cbase.h"
#include "shareddefs.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CViewVectors::CViewVectors( 
	Vector vHullMin,
	Vector vHullMax,
	Vector vView,
	Vector vDuckHullMin,
	Vector vDuckHullMax,
	Vector vDuckView,
	Vector vObsHullMin,
	Vector vObsHullMax,
	Vector vDeadViewHeight )
{
	m_vHullMin = vHullMin;
	m_vHullMax = vHullMax;
	m_vView = vView;

	Vector vecSize;
	VectorSubtract( m_vHullMax, m_vHullMin, vecSize );
	m_flRadius = vecSize.Length() * 0.5f;
	m_flRadius2D = vecSize.Length2D() * 0.5f;

	m_flWidth = vecSize.y;
	m_flHeight = vecSize.z;
	m_flLength = vecSize.x;

	m_vDuckHullMin = vDuckHullMin;
	m_vDuckHullMax = vDuckHullMax;
	m_vDuckView = vDuckView;

	VectorSubtract( m_vDuckHullMax, m_vDuckHullMin, vecSize );
	m_flDuckRadius = vecSize.Length() * 0.5f;
	m_flDuckRadius2D = vecSize.Length2D() * 0.5f;

	m_flDuckWidth = vecSize.y;
	m_flDuckHeight = vecSize.z;
	m_flDuckLength = vecSize.x;

	m_vObsHullMin = vObsHullMin;
	m_vObsHullMax = vObsHullMax;

	m_vDeadViewHeight = vDeadViewHeight;
}

FireBulletsInfo_t::FireBulletsInfo_t()
{
	m_iShots = 1;
	m_vecSpread.Init( 0, 0, 0 );
	m_flDistance = 8192;
	m_iTracerFreq = 4;
	m_flDamage = 0.0f;
	m_flPlayerDamage = 0.0f;
	m_pAttacker = NULL;
	m_nFlags = FIRE_BULLETS_NO_FLAGS;
	m_pAdditionalIgnoreEnt = NULL;
	m_flDamageForceScale = 1.0f;

#ifdef _DEBUG
	m_iAmmoType = AMMO_INVALID_INDEX;
	m_vecSrc.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
	m_vecDirShooting.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
#endif
	m_bPrimaryAttack = true;
	m_bUseServerRandomSeed = false;

	m_pIgnoreEntList = NULL;
}

FireBulletsInfo_t::FireBulletsInfo_t( int nShots, const Vector &vecSrc, const Vector &vecDir, const Vector &vecSpread, float flDistance, AmmoIndex_t nAmmoType, bool bPrimaryAttack )
{
	m_iShots = nShots;
	m_vecSrc = vecSrc;
	m_vecDirShooting = vecDir;
	m_vecSpread = vecSpread;
	m_flDistance = flDistance;
	m_iAmmoType = nAmmoType;
	m_iTracerFreq = 4;
	m_flDamage = 0;
	m_flPlayerDamage = 0;
	m_pAttacker = NULL;
	m_nFlags = FIRE_BULLETS_NO_FLAGS;
	m_pAdditionalIgnoreEnt = NULL;
	m_flDamageForceScale = 1.0f;
	m_bPrimaryAttack = bPrimaryAttack;
	m_bUseServerRandomSeed = false;

	m_pIgnoreEntList = NULL;
}

FireBulletsInfo_t::FireBulletsInfo_t( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, AmmoIndex_t iAmmoType, int iTracerFreq, float flDamage, CSharedBaseEntity *pAttacker, bool bFirstShotAccurate, bool bPrimaryAttack )
{
	m_iShots = cShots;
	m_vecSrc = vecSrc;
	m_vecDirShooting = vecDirShooting;
	m_vecSpread = vecSpread;
	m_flDistance = flDistance;
	m_iAmmoType = iAmmoType;
	m_iTracerFreq = iTracerFreq;
	m_flDamage = flDamage;
	m_flPlayerDamage = 0;
	m_pAttacker = pAttacker;
	m_nFlags = bFirstShotAccurate ? FIRE_BULLETS_FIRST_SHOT_ACCURATE : FIRE_BULLETS_NO_FLAGS;
	m_pAdditionalIgnoreEnt = NULL;
	m_flDamageForceScale = 1.0f;
	m_bPrimaryAttack = bPrimaryAttack;
	m_bUseServerRandomSeed = false;

	m_pIgnoreEntList = NULL;
}
