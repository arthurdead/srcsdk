#include "cbase.h"
#include "igamemovement.h"
#include "player_command.h"
#include "ipredictionsystem.h"
#include "iservervehicle.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;

class CHeistPlayerMove : public CPlayerMove
{
	DECLARE_CLASS(CHeistPlayerMove, CPlayerMove);

public:
	void StartCommand(CBasePlayer *player, CUserCmd *cmd) override;
	void SetupMove(CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move) override;
	void FinishMove(CBasePlayer *player, CUserCmd *ucmd, CMoveData *move) override;
};

static CHeistPlayerMove g_PlayerMove;

CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

void CHeistPlayerMove::StartCommand(CBasePlayer *player, CUserCmd *cmd)
{
	BaseClass::StartCommand( player, cmd );
}

void CHeistPlayerMove::SetupMove(CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move)
{
	BaseClass::SetupMove( player, ucmd, pHelper, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if(pVehicle && gpGlobals->frametime != 0) {
		pVehicle->SetupMove( player, ucmd, pHelper, move ); 
	}
}

void CHeistPlayerMove::FinishMove(CBasePlayer *player, CUserCmd *ucmd, CMoveData *move)
{
	BaseClass::FinishMove( player, ucmd, move );

	IServerVehicle *pVehicle = player->GetVehicle();
	if(pVehicle && gpGlobals->frametime != 0) {
		pVehicle->FinishMove( player, ucmd, move );
	}
}
