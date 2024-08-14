#include "cbase.h"
#include "prediction.h"
#include "igamemovement.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

class CHeistPrediction : public CPrediction
{
	DECLARE_CLASS(CHeistPrediction, CPrediction);

public:
	virtual void SetupMove(C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move);
	virtual void FinishMove(C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move);
};

void CHeistPrediction::SetupMove(C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move)
{
	BaseClass::SetupMove( player, ucmd, pHelper, move );
}

void CHeistPrediction::FinishMove(C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move)
{
	BaseClass::FinishMove( player, ucmd, move );
}

static CHeistPrediction g_Prediction;
CPrediction *prediction = &g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CHeistPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction);
