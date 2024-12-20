//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "modelentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFuncReflectiveGlass : public CFuncBrush
{
	DECLARE_MAPENTITY();
	DECLARE_CLASS( CFuncReflectiveGlass, CFuncBrush );
	DECLARE_SERVERCLASS();

	CFuncReflectiveGlass()
	{
		m_iszReflectRenderTarget = AllocPooledString( "_rt_WaterReflection" );
		m_iszRefractRenderTarget = AllocPooledString( "_rt_WaterRefraction" );
	}

	void InputSetReflectRenderTarget( inputdata_t &&inputdata ) { m_iszReflectRenderTarget = inputdata.value.StringID(); }
	void InputSetRefractRenderTarget( inputdata_t &&inputdata ) { m_iszRefractRenderTarget = inputdata.value.StringID(); }

	CNetworkStringT( m_iszReflectRenderTarget );
	CNetworkStringT( m_iszRefractRenderTarget );
};

LINK_ENTITY_TO_CLASS( func_reflective_glass, CFuncReflectiveGlass );

// automatically hooks in the system's callbacks
BEGIN_MAPENTITY( CFuncReflectiveGlass )

	DEFINE_KEYFIELD_AUTO( m_iszReflectRenderTarget, "ReflectRenderTarget" ),
	DEFINE_KEYFIELD_AUTO( m_iszRefractRenderTarget, "RefractRenderTarget" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetReflectRenderTarget", InputSetReflectRenderTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRefractRenderTarget", InputSetRefractRenderTarget ),

END_MAPENTITY()

IMPLEMENT_SERVERCLASS_ST( CFuncReflectiveGlass, DT_FuncReflectiveGlass )
	SendPropStringT( SENDINFO( m_iszReflectRenderTarget ) ),
	SendPropStringT( SENDINFO( m_iszRefractRenderTarget ) ),
END_SEND_TABLE()
