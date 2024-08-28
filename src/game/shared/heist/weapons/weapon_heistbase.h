#ifndef WEAPON_HEISTBASE_H
#define WEAPON_HEISTBASE_H

#pragma once

#include "basecombatweapon_shared.h"
#include "heist_player_shared.h"
#include "heist_weapon_parse.h"

#ifdef CLIENT_DLL
	#define CWeaponHeistBase C_WeaponHeistBase
#endif

class CWeaponHeistBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponHeistBase, CBaseCombatWeapon);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CWeaponHeistBase();

#ifdef GAME_DLL
	void SendReloadSoundEvent();
	void Materialize() override;
	int ObjectCaps() override;
#endif

#ifdef CLIENT_DLL
	bool ShouldPredict() override;
	void OnDataChanged(DataUpdateType_t type) override;
	bool ViewModel_FireEvent(C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options) override;
#else
	void Spawn() override;
#endif

	bool IsPredicted() const override;

	CBasePlayer *GetPlayerOwner() const;
	CHeistPlayer *GetHeistPlayerOwner() const;

	void WeaponSound(WeaponSound_t sound_type, float soundtime = 0.0f) override;

	void FireBullets(const FireBulletsInfo_t &info) override;
	void FallInit();
	bool Reload() override;

	const CHeistWeaponInfo &GetHeistWpnData() const;

	Vector GetOriginalSpawnOrigin()
	{ return m_vOriginalSpawnOrigin; }
	QAngle GetOriginalSpawnAngles()
	{ return m_vOriginalSpawnAngles; }

	float m_flNextResetCheckTime;

private:
	CWeaponHeistBase(const CWeaponHeistBase &);

	float m_flPrevAnimTime;

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;
};

#endif
