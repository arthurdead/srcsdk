#include "cbase.h"
#include "basecombatweapon_shared.h"
#include "weapon_base_heist.h"

#ifdef GAME_DLL
#include "soundent.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//hl2 weapons to use while developing, remove when there's actual weapons

#ifdef CLIENT_DLL
class C_WeaponHL2Pistol;
typedef C_WeaponHL2Pistol CSharedWeaponHL2Pistol;
class C_WeaponHL2Shotgun;
typedef C_WeaponHL2Shotgun CSharedWeaponHL2Shotgun;
class C_WeaponHL2Smg;
typedef C_WeaponHL2Smg CSharedWeaponHL2Smg;
#else
class CWeaponHL2Pistol;
typedef CWeaponHL2Pistol CSharedWeaponHL2Pistol;
class CWeaponHL2Shotgun;
typedef CWeaponHL2Shotgun CSharedWeaponHL2Shotgun;
class CWeaponHL2Smg;
typedef CWeaponHL2Smg CSharedWeaponHL2Smg;
#endif

#ifdef CLIENT_DLL
	#define CWeaponHL2Pistol C_WeaponHL2Pistol
#endif

class CWeaponHL2Pistol : public CSharedBaseHeistWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Pistol, CSharedBaseHeistWeapon);

	CWeaponHL2Pistol()
	{
		m_fMinRange1		= 24;
		m_fMaxRange1		= 1500;
		m_fMinRange2		= 24;
		m_fMaxRange2		= 200;
	}

#ifdef CLIENT_DLL
	#undef CWeaponHL2Pistol
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

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

acttable_t	CSharedWeaponHL2Pistol::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CSharedWeaponHL2Pistol );

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS_ALIASED(weapon_hl2_pistol, WeaponHL2Pistol);
#endif

#ifdef CLIENT_DLL
	#define CWeaponHL2Shotgun C_WeaponHL2Shotgun
#endif

class CWeaponHL2Shotgun : public CSharedBaseHeistWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Shotgun, CSharedBaseHeistWeapon);

	CWeaponHL2Shotgun()
	{
		m_bReloadsSingly = true;

		m_fMinRange1		= 0.0;
		m_fMaxRange1		= 500;
		m_fMinRange2		= 0.0;
		m_fMaxRange2		= 200;
	}

#ifdef CLIENT_DLL
	#undef CWeaponHL2Shotgun
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual float			GetFireRate( void ) { return 0.7f; }

#ifdef GAME_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		Animevent nEvent = pEvent->Event();

		if(nEvent == EVENT_WEAPON_SHOTGUN_FIRE)
		{
			return;
		}

		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
	}
#endif

	DECLARE_ACTTABLE();
};

acttable_t	CSharedWeaponHL2Shotgun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
};

IMPLEMENT_ACTTABLE( CSharedWeaponHL2Shotgun );

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS_ALIASED(weapon_hl2_shotgun, WeaponHL2Shotgun);
#endif

#ifdef CLIENT_DLL
	#define CWeaponHL2Smg C_WeaponHL2Smg
#endif

class CWeaponHL2Smg : public CSharedBaseHeistWeapon
{
public:
	DECLARE_CLASS(CWeaponHL2Smg, CSharedBaseHeistWeapon);

	CWeaponHL2Smg()
	{
		m_fMinRange1		= 0;// No minimum range. 
		m_fMaxRange1		= 1400;
	}

#ifdef CLIENT_DLL
	#undef CWeaponHL2Smg
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual float			GetFireRate( void ) { return 0.075f; }

#ifdef GAME_DLL
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		Animevent nEvent = pEvent->Event();

		if(nEvent == EVENT_WEAPON_SMG1)
		{
			return;
		}

		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
	}
#endif

	DECLARE_ACTTABLE();
};

acttable_t	CSharedWeaponHL2Smg::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },
	
// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
	{ ACT_RUN,						ACT_RUN_RIFLE,					true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
	{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE( CSharedWeaponHL2Smg );

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS_ALIASED(weapon_hl2_smg1, WeaponHL2Smg);
#endif

BEGIN_NETWORK_TABLE(CSharedWeaponHL2Pistol, DT_WeaponHL2Pistol)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHL2Pistol, DT_WeaponHL2Pistol)

BEGIN_NETWORK_TABLE(CSharedWeaponHL2Shotgun, DT_WeaponHL2Shotgun)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHL2Shotgun, DT_WeaponHL2Shotgun)

BEGIN_NETWORK_TABLE(CSharedWeaponHL2Smg, DT_WeaponHL2Smg)
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHL2Smg, DT_WeaponHL2Smg)

BEGIN_PREDICTION_DATA( C_WeaponHL2Pistol )
END_PREDICTION_DATA()

BEGIN_PREDICTION_DATA( C_WeaponHL2Shotgun )
END_PREDICTION_DATA()

BEGIN_PREDICTION_DATA( C_WeaponHL2Smg )
END_PREDICTION_DATA()
