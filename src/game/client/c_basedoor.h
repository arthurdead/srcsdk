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

#if defined( CLIENT_DLL )
#define CBaseDoor C_BaseDoor
#endif

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

#endif // C_BASEDOOR_H