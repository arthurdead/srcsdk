#include "cbase.h"
#include "weapon_heistbase.h"
#include "heist_gamerules.h"

#ifdef GAME_DLL
#include "heist_player.h"
#else
#include "c_heist_player.h"
#endif

#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#endif

#ifndef CLIENT_DLL
#include "vphysics/constraints.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NUM_MUZZLE_FLASH_TYPES 4

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHeistBase, DT_WeaponHeistBase)

BEGIN_NETWORK_TABLE(CWeaponHeistBase, DT_WeaponHeistBase)
#ifndef CLIENT_DLL
	//SendPropExclude("DT_AnimTimeMustBeFirst", "m_flAnimTime"),
	//SendPropExclude("DT_BaseAnimating", "m_nSequence"),
	//SendPropExclude("DT_LocalActiveWeaponData", "m_flTimeWeaponIdle"),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponHeistBase)
END_PREDICTION_DATA()

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponHeistBase)
END_DATADESC()
#endif

CWeaponHeistBase::CWeaponHeistBase()
{
#ifdef CLIENT_DLL
	SetPredictionEligible(true);
#endif

	AddSolidFlags(FSOLID_TRIGGER);

	m_flNextResetCheckTime = 0.0f;
}

const CHeistWeaponInfo &CWeaponHeistBase::GetHeistWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CHeistWeaponInfo *pHeistInfo;

#ifdef _DEBUG
	pHeistInfo = dynamic_cast<const CHeistWeaponInfo *>(pWeaponInfo);
	Assert(pHeistInfo);
#else
	pHeistInfo = static_cast<const CHeistWeaponInfo *>(pWeaponInfo);
#endif

	return *pHeistInfo;
}

bool CWeaponHeistBase::IsPredicted() const
{ 
	return true;
}

bool CWeaponHeistBase::Reload()
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if(fRet) {
		//WeaponSound( RELOAD );
		ToHeistPlayer(GetOwner())->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
	}

	return fRet;
}

void CWeaponHeistBase::WeaponSound(WeaponSound_t sound_type, float soundtime /* = 0.0f */)
{
#ifdef CLIENT_DLL
	const char *shootsound = GetWpnData().aShootSounds[sound_type]; 
	if(!shootsound || !shootsound[0]) {
		return;
	}

	CBroadcastRecipientFilter filter;
	if(!te->CanPredict()) {
		return;
	}

	CBaseEntity::EmitSound( filter, GetHeistPlayerOwner()->entindex(), shootsound, &GetHeistPlayerOwner()->GetAbsOrigin() ); 
#else
	BaseClass::WeaponSound( sound_type, soundtime );
#endif
}

CBasePlayer *CWeaponHeistBase::GetPlayerOwner() const
{
	return dynamic_cast<CBasePlayer *>(GetOwner());
}

CHeistPlayer *CWeaponHeistBase::GetHeistPlayerOwner() const
{
	return dynamic_cast<CHeistPlayer *>(GetOwner());
}

#ifdef CLIENT_DLL
void CWeaponHeistBase::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if(GetPredictable() && !ShouldPredict()) {
		ShutdownPredictable();
	}
}

bool CWeaponHeistBase::ShouldPredict()
{
	if(GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer()) {
		return true;
	}

	return BaseClass::ShouldPredict();
}
#else
void CWeaponHeistBase::Spawn()
{
	BaseClass::Spawn();

	SetCollisionGroup( COLLISION_GROUP_WEAPON );
}

void CWeaponHeistBase::Materialize( void )
{
	if(IsEffectActive(EF_NODRAW)) {
		EmitSound("AlyxEmp.Charge");

		RemoveEffects(EF_NODRAW);
		DoMuzzleFlash();
	}

	if(HasSpawnFlags(SF_NORESPAWN) == false) {
		VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags()|FSOLID_TRIGGER, false);
		SetMoveType(MOVETYPE_VPHYSICS);

		HeistGamerules()->AddLevelDesignerPlacedObject(this);
	}

	if(HasSpawnFlags(SF_NORESPAWN) == false) {
		if(GetOriginalSpawnOrigin() == vec3_origin) {
			m_vOriginalSpawnOrigin = GetAbsOrigin();
			m_vOriginalSpawnAngles = GetAbsAngles();
		}
	}

	SetPickupTouch();

	SetThink(NULL);
}

int CWeaponHeistBase::ObjectCaps()
{
	return BaseClass::ObjectCaps() & ~FCAP_IMPULSE_USE;
}
#endif

void CWeaponHeistBase::FallInit()
{
#ifndef CLIENT_DLL
	SetModel(GetWorldModel());
	VPhysicsDestroyObject();

	if(HasSpawnFlags(SF_NORESPAWN) == false) {
		SetMoveType(MOVETYPE_NONE);
		SetSolid(SOLID_BBOX);
		AddSolidFlags(FSOLID_TRIGGER);

		UTIL_DropToFloor(this, MASK_SOLID);
	} else {
		if(!VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags()|FSOLID_TRIGGER, false)) {
			SetMoveType(MOVETYPE_NONE);
			SetSolid(SOLID_BBOX);
			AddSolidFlags(FSOLID_TRIGGER);
		} else {
		#ifndef CLIENT_DLL
			if(HasSpawnFlags(SF_WEAPON_START_CONSTRAINED)) {
				IPhysicsObject *pReferenceObject, *pAttachedObject;

				pReferenceObject = g_PhysWorldObject;
				pAttachedObject = VPhysicsGetObject();

				if(pReferenceObject && pAttachedObject) {
					constraint_fixedparams_t fixed;
					fixed.Defaults();
					fixed.InitWithCurrentObjectState(pReferenceObject, pAttachedObject);

					fixed.constraint.forceLimit	= lbs2kg(10000);
					fixed.constraint.torqueLimit = lbs2kg(10000);

					IPhysicsConstraint *pConstraint = GetConstraint();

					pConstraint = physenv->CreateFixedConstraint(pReferenceObject, pAttachedObject, NULL, fixed);

					pConstraint->SetGameData((void *)this);
				}
			}
		#endif
		}
	}

	SetPickupTouch();

	SetThink(&CBaseCombatWeapon::FallThink);

	SetNextThink(gpGlobals->curtime + 0.1f);
#endif
}

void CWeaponHeistBase::FireBullets(const FireBulletsInfo_t &info)
{
	FireBulletsInfo_t modinfo = info;

	BaseClass::FireBullets( modinfo );
}

#ifdef CLIENT_DLL
bool CWeaponHeistBase::OnFireEvent(C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options)
{
	return BaseClass::OnFireEvent(pViewModel, origin, angles, event, options);
}
#endif
