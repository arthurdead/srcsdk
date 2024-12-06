#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_behavior.h"
#include "ai_behavior_follow.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar recast_debug_mesh;

CBaseEntity *pPathTarget = NULL;

class CPatherTarget : public CBaseAnimating
{
public:
	DECLARE_CLASS(CPatherTarget, CBaseAnimating);

	CPatherTarget()
	{
	}

	~CPatherTarget()
	{
		pPathTarget = NULL;
	}

	void Precache() override
	{
		BaseClass::Precache();

		PrecacheModel("models/dog.mdl");
	}

	void Spawn() override
	{
		Precache();

		BaseClass::Spawn();

		SetModel( "models/dog.mdl" );

		pPathTarget = this;
	}
};

LINK_ENTITY_TO_CLASS(prop_dev_pather_target, CPatherTarget);

class CNPCDev_Pather : public CAI_BehaviorHost<CAI_BaseNPC>
{
public:
	DECLARE_CLASS(CNPCDev_Pather, CAI_BehaviorHost<CAI_BaseNPC>);

	void Precache() override
	{
		BaseClass::Precache();

		PrecacheModel("models/alyx.mdl");
	}

	void Spawn() override
	{
		Precache();

		BaseClass::Spawn();

		SetModel("models/alyx.mdl");

		UTIL_SetSize(this, NAI_Hull::Mins(RECAST_NAVMESH_HUMAN), NAI_Hull::Maxs(RECAST_NAVMESH_HUMAN));

		SetSolid(SOLID_BBOX);
		AddSolidFlags(FSOLID_NOT_STANDABLE);

		SetMoveType(MOVETYPE_STEP);
		CapabilitiesAdd(bits_CAP_MOVE_GROUND|bits_CAP_DUCK|bits_CAP_MOVE_JUMP|bits_CAP_OPEN_DOORS);

		m_NPCState = NPC_STATE_NONE;
		SetHealth( 10 );
		m_flFieldOfView = 0.5f;
		NPCInit();

		CBaseEntity::m_nDebugPlayer = 1;
		m_debugOverlays |= (OVERLAY_NPC_NEAREST_BIT|OVERLAY_NPC_ROUTE_BIT);

		CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "prop_dev_pather_target");
		if(!pEntity) {
			pEntity = UTIL_GetNearestPlayer(GetAbsOrigin());
		}
		if(pEntity) {
			m_FollowBehavior.SetFollowTarget(pEntity);
		}
	}

	int SelectSchedule() override
	{
		if(!BehaviorSelectSchedule()) {
		}

		return BaseClass::SelectSchedule();
	}

	bool CreateBehaviors() override
	{
		AddBehavior(&m_FollowBehavior);

		return BaseClass::CreateBehaviors();
	}

private:
	CAI_FollowBehavior m_FollowBehavior;
};

LINK_ENTITY_TO_CLASS(npc_dev_pather, CNPCDev_Pather);
