#include "cbase.h"
#include "prediction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CPrediction g_Prediction;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSDKPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );
CPrediction *prediction = &g_Prediction;
