//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_BREAKABLEPROP_H
#define C_BREAKABLEPROP_H
#pragma once

#include "c_baseanimating.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BreakableProp : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_BreakableProp, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_BreakableProp() : C_BreakableProp( 0 ) {}
	C_BreakableProp( int iEFlags );

	virtual bool IsProp( void ) const
	{
		return true;
	}

	//virtual bool	ShouldPredict( void );
	//virtual C_BasePlayer *GetPredictionOwner( void );
	//virtual bool PredictionErrorShouldResetLatchedForAllPredictables( void ) { return false; }

	
	//IPlayerPickupVPhysics
	virtual bool HasPreferredCarryAnglesForPlayer( C_BasePlayer *pPlayer );
	virtual QAngle PreferredCarryAngles( void );

	virtual void SetFadeMinMax( float fademin, float fademax );

	// Copy fade from another breakable prop
	void CopyFadeFrom( C_BreakableProp *pSource );
	virtual void OnDataChanged( DataUpdateType_t type );

	const QAngle &GetNetworkedPreferredPlayerCarryAngles( void ) { return m_qPreferredPlayerCarryAngles; }

protected:
	QAngle m_qPreferredPlayerCarryAngles;
};

typedef C_ClientOnlyWrapper<C_BreakableProp> C_ClientOnlyBreakableProp;

#endif // C_BREAKABLEPROP_H
