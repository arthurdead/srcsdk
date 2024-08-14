#include "cbase.h"
#include "gamemovement.h"

class CHeistGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS(CHeistGameMovement, CGameMovement);
};

static CHeistGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement);
