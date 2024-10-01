//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "bitmap/imageformat.h"
#include "cl_mat_stub.h"
#include "materialsystem/imaterialsystemstub.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Hook the engine's mat_stub cvar.
ConVar mat_stub( "mat_stub", "0", FCVAR_CHEAT );
extern ConVar gl_clear;

// ---------------------------------------------------------------------------------------- //
// CMatStubHandler implementation.
// ---------------------------------------------------------------------------------------- //

CMatStubHandler::CMatStubHandler()
{
	if ( mat_stub.GetInt() )
	{
		m_pOldMaterialSystem = g_pMaterialSystem;
		// Replace all material system pointers with the stub.
		materials_stub->SetRealMaterialSystem( g_pMaterialSystem );
		g_pMaterialSystem->SetInStubMode( true );
		g_pMaterialSystem = materials_stub;
		engine->Mat_Stub( g_pMaterialSystem );
	}
	else
	{
		m_pOldMaterialSystem = NULL;
	}
}


CMatStubHandler::~CMatStubHandler()
{
	End();
}


void CMatStubHandler::End()
{
	// Put back the original material system pointer.
	if ( m_pOldMaterialSystem )
	{
		g_pMaterialSystem = m_pOldMaterialSystem;
		g_pMaterialSystem->SetInStubMode( false );
		engine->Mat_Stub( g_pMaterialSystem );
		m_pOldMaterialSystem = NULL;

//		if( gl_clear.GetBool() )
		{
			g_pMaterialSystem->ClearBuffers( true, true );
		}
	}
}


bool IsMatStubEnabled()
{
	return mat_stub.GetBool();
}
