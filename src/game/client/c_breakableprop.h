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
	typedef C_BaseAnimating BaseClass;
public:
	DECLARE_CLIENTCLASS();

	C_BreakableProp();
	C_BreakableProp(bool bClientSide);

	virtual bool IsProp( void ) const
	{
		return true;
	}
	
	virtual void SetFadeMinMax( float fademin, float fademax );

	// Copy fade from another breakable prop
	void CopyFadeFrom( C_BreakableProp *pSource );
	virtual void OnDataChanged( DataUpdateType_t type );

private:
	bool m_bClientPhysics;
};

#endif // C_BREAKABLEPROP_H
