//--------------------------------------------------------------------------------------------------------
// Copyright (c) 2007 Turtle Rock Studios, Inc.

#if !defined( C_BASETOGGLE_H )
#define C_BASETOGGLE_H

#pragma once

#include "c_baseentity.h"

//--------------------------------------------------------------------------------------------------------
class C_BaseToggle: public C_BaseEntity
{
public:
	DECLARE_CLASS( C_BaseToggle, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	virtual void GetGroundVelocityToApply( Vector &vecGroundVel );
};


//--------------------------------------------------------------------------------------------------------
class C_BaseButton: public C_BaseToggle
{
public:
	DECLARE_CLASS( C_BaseButton, C_BaseToggle );
	DECLARE_CLIENTCLASS();

	C_BaseButton()
	{
	}

	virtual bool IsPotentiallyUsable( void );

private:
	bool m_usable;
};


#endif // C_BASETOGGLE_H