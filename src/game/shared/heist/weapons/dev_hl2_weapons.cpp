#include "cbase.h"
#include "basecombatweapon_shared.h"

#ifdef GAME_DLL
#include "soundent.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//hl2 weapons to use while developing, remove when there's actual weapons

class CWeaponHL2Pistol : public CSharedBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Pistol, CSharedBaseCombatWeapon);

	CWeaponHL2Pistol()
	{
		m_fMinRange1		= 24;
		m_fMaxRange1		= 1500;
		m_fMinRange2		= 24;
		m_fMaxRange2		= 200;
	}

	virtual float			GetFireRate( void ) { return 0.5f; }

#ifdef GAME_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		Animevent nEvent = pEvent->Event();

		if(nEvent == EVENT_WEAPON_PISTOL_FIRE)
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( FireBulletsInfo_t(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2) );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
			return;
		}

		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
	}
#endif

	DECLARE_ACTTABLE();
};

acttable_t	CWeaponHL2Pistol::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },
};

IMPLEMENT_ACTTABLE( CWeaponHL2Pistol );

LINK_ENTITY_TO_CLASS(weapon_hl2_pistol, CWeaponHL2Pistol);

class CWeaponHL2Shotgun : public CSharedBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Shotgun, CSharedBaseCombatWeapon);

	CWeaponHL2Shotgun()
	{
	}

	virtual float			GetFireRate( void ) { return 0.5f; }

	DECLARE_ACTTABLE();
};

acttable_t	CWeaponHL2Shotgun::m_acttable[] = 
{
};

IMPLEMENT_ACTTABLE( CWeaponHL2Shotgun );

LINK_ENTITY_TO_CLASS(weapon_hl2_shotgun, CWeaponHL2Shotgun);

class CWeaponHL2Smg : public CSharedBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Smg, CSharedBaseCombatWeapon);

	CWeaponHL2Smg()
	{
	}

	virtual float			GetFireRate( void ) { return 0.2f; }

	DECLARE_ACTTABLE();
};

acttable_t	CWeaponHL2Smg::m_acttable[] = 
{
};

IMPLEMENT_ACTTABLE( CWeaponHL2Smg );

LINK_ENTITY_TO_CLASS(weapon_hl2_smg1, CWeaponHL2Smg);
