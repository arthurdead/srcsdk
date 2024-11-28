//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "cdll_bounded_cvars.h"
#include "convar_serverbounded.h"
#include "tier0/icommandline.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

bool g_bForceCLPredictOff = false;

extern ConVarBase *cl_updaterate;

ConVar  sv_client_min_interp_ratio( "sv_client_min_interp_ratio", "1", FCVAR_REPLICATED, 
								   "This can be used to limit the value of cl_interp_ratio for connected clients "
								   "(only while they are connected).\n"
								   "              -1 = let clients set cl_interp_ratio to anything\n"
								   " any other value = set minimum value for cl_interp_ratio"
								   );
ConVar  sv_client_max_interp_ratio( "sv_client_max_interp_ratio", "5", FCVAR_REPLICATED, 
								   "This can be used to limit the value of cl_interp_ratio for connected clients "
								   "(only while they are connected). If sv_client_min_interp_ratio is -1, "
								   "then this cvar has no effect.",
								   true, 2, false, 5);

ConVar  sv_client_predict( "sv_client_predict", "-1", FCVAR_REPLICATED, 
	"This can be used to force the value of cl_predict for connected clients "
	"(only while they are connected).\n"
	"   -1 = let clients set cl_predict to anything\n"
	"    0 = force cl_predict to 0\n"
	"    1 = force cl_predict to 1"
	);

// ------------------------------------------------------------------------------------------ //
// cl_predict.
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_Predict : public ConVar_ServerBounded
{
public:
	CBoundedCvar_Predict() :
	  ConVar_ServerBounded( "cl_predict", 
		  "1.0", 
		  FCVAR_USERINFO | FCVAR_CHEAT | FCVAR_NOT_CONNECTED, 
		  "Perform client side prediction." )
	  {
	  }

	  virtual float GetFloat() const
	  {
		  // Used temporarily for CS kill cam.
		  if ( g_bForceCLPredictOff )
			  return 0;

		  if ( sv_client_predict.GetInt() != -1 )
		  {
			  // Ok, the server wants to control this value.
			  return sv_client_predict.GetFloat();
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_Predict cl_predict_var;
ConVar_ServerBounded *cl_predict = &cl_predict_var;



// ------------------------------------------------------------------------------------------ //
// cl_interp_ratio.
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_InterpRatio : public ConVar_ServerBounded
{
public:
	CBoundedCvar_InterpRatio() :
	  ConVar_ServerBounded( "cl_interp_ratio", 
		  "2.0", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (final amount is cl_interp_ratio / cl_updaterate)." )
	  {
	  }

	  virtual float GetFloat() const
	  {
		  if ( sv_client_min_interp_ratio.GetFloat() != -1 )
		  {
			  return clamp( GetBaseFloatValue(), sv_client_min_interp_ratio.GetFloat(), sv_client_max_interp_ratio.GetFloat() );
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_InterpRatio cl_interp_ratio_var;
ConVar_ServerBounded *cl_interp_ratio = &cl_interp_ratio_var;


// ------------------------------------------------------------------------------------------ //
// cl_interp
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_Interp : public ConVar_ServerBounded
{
public:
	CBoundedCvar_Interp() :
	  ConVar_ServerBounded( "cl_interp", 
		  "0.1", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (bounded on low side by server interp ratio settings).", true, 0.0f, true, 0.5f )
	  {
	  }

	  virtual float GetFloat() const
	  {
		  if ( sv_client_min_interp_ratio.GetFloat() != -1 )
		  {
			  return MAX( GetBaseFloatValue(), sv_client_min_interp_ratio.GetFloat() / cl_updaterate->GetFloat() );
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_Interp cl_interp_var;
ConVar_ServerBounded *cl_interp = &cl_interp_var;

float GetClientInterpAmount()
{
	return MAX( cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / cl_updaterate->GetFloat() );
}

