//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( C_BASEDOOR_H )
#define C_BASEDOOR_H
#pragma once

#include "c_baseentity.h"
#include "c_basetoggle.h"

class C_BaseDoor : public C_BaseToggle
{
public:
	DECLARE_CLASS( C_BaseDoor, C_BaseToggle );
	DECLARE_CLIENTCLASS();

	C_BaseDoor( void );
	~C_BaseDoor( void );

public:
	float		m_flWaveHeight;
};

typedef C_BaseDoor CSharedBaseDoor;

#endif // C_BASEDOOR_H