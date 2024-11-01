#include "cbase.h"
#include "c_heist_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistLocalPlayerExclusive)
	RecvPropBool(RECVINFO(m_bMaskingUp))
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_HeistPlayer, DT_HeistNonLocalPlayerExclusive)
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_HeistPlayer, DT_Heist_Player, CHeistPlayer)
	RecvPropDataTable("heistlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistLocalPlayerExclusive)),
	RecvPropDataTable("heistnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HeistNonLocalPlayerExclusive)),
END_RECV_TABLE()

C_HeistPlayer::C_HeistPlayer()
{
}

C_HeistPlayer::~C_HeistPlayer()
{
}
