#include "cbase.h"
#include "c_heist_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_SERVERCLASS(player, CHeistPlayer);

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistLocalPlayerExclusive)
	RecvPropBool(RECVINFO(m_bMaskingUp))
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistNonLocalPlayerExclusive)
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_HeistPlayer, DT_Heist_Player, CHeistPlayer)
	RecvPropFloat(PROPINFO(m_flLeaning), 0),

	RecvPropDataTable("heistlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistLocalPlayerExclusive)),
	RecvPropDataTable("heistnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistNonLocalPlayerExclusive)),
END_RECV_TABLE()

C_HeistPlayer::C_HeistPlayer()
{
}

C_HeistPlayer::~C_HeistPlayer()
{
}

float C_HeistPlayer::CalcRoll(const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed)
{
	float baseVal = BaseClass::CalcRoll(angles, velocity, rollangle, rollspeed);

	baseVal += 25.0f * m_flLeaning;
	baseVal = AngleNormalize(baseVal);

	return baseVal;
}

ConVar cl_heist_lean_dist("cl_heist_lean_dist", "-30");

void C_HeistPlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	BaseClass::CalcPlayerView(eyeOrigin, eyeAngles, fov);

	Vector offset(0, cl_heist_lean_dist.GetFloat() * m_flLeaning, 0);

	Vector offset_rotate;
	VectorRotate(offset, eyeAngles, offset_rotate);

	eyeOrigin += offset_rotate;
}
