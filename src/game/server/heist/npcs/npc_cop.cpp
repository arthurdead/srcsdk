#include "cbase.h"
#include "npc_cop.h"
#include "heist_player.h"
#include "heist_gamerules.h"
#include "npcevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_cop, CNPC_Cop);

static const char *g_pszCopModels[]{
	"models/police.mdl"
};

void CNPC_Cop::Precache()
{
	BaseClass::Precache();

	for(int i = 0; i < ARRAYSIZE(g_pszCopModels); ++i) {
		PrecacheModel(g_pszCopModels[i]);
	}
}

class CWeaponPistol : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponPistol, CBaseCombatWeapon);

	CWeaponPistol()
	{
		m_fMinRange1		= 24;
		m_fMaxRange1		= 1500;
		m_fMinRange2		= 24;
		m_fMaxRange2		= 200;
	}

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

	DECLARE_ACTTABLE();
};

acttable_t	CWeaponPistol::m_acttable[] = 
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

IMPLEMENT_ACTTABLE( CWeaponPistol );

LINK_ENTITY_TO_CLASS(weapon_pistol, CWeaponPistol);

void CNPC_Cop::Spawn()
{
	if(GetModelName() == NULL_STRING) {
		SetModel(g_pszCopModels[random->RandomInt(0, ARRAYSIZE(g_pszCopModels)-1)]);
	}

	UTIL_SetSize(this, NAI_Hull::Mins(RECAST_NAVMESH_HUMAN), NAI_Hull::Maxs(RECAST_NAVMESH_HUMAN));

	CapabilitiesAdd(
		bits_CAP_NO_HIT_SQUADMATES|
		bits_CAP_SQUAD|
		bits_CAP_AIM_GUN|
		bits_CAP_MOVE_SHOOT|
		bits_CAP_USE_WEAPONS
	);

	ChangeTeam( TEAM_POLICE );
	ChangeFaction( FACTION_LAW_ENFORCEMENT );

	CBaseCombatWeapon *pPistol = GiveWeapon( MAKE_STRING("weapon_pistol") );
	GiveAmmo(99, pPistol->m_iPrimaryAmmoType);

	BaseClass::Spawn();
}

void CNPC_Cop::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch(pOther);

	if(pOther->IsPlayer()) {
		CHeistPlayer *pPlayer = (CHeistPlayer *)pOther;
		if(!pPlayer->IsSpotted()) {
			m_Suspicioner.SetSuspicion(pPlayer, 100.0f);
			pPlayer->SetSpotted(true);
			HeistGameRules()->SetSpotted(true);
		}
	}
}
