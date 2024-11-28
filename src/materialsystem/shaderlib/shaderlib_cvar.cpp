//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "icvar.h"
#include "tier1/tier1.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------------- //
// ConVar stuff.
// ------------------------------------------------------------------------------------------- //
class CShaderLibConVarAccessor : public CDefaultAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
	#ifdef _DEBUG
		AssertMsg( !pCommand->IsFlagSet(FCVAR_GAMEDLL), "shader dll tried to register server con var/command named %s", pCommand->GetName() );
	#endif

		return CDefaultAccessor::RegisterConCommandBase( pCommand );
	}
};

CShaderLibConVarAccessor g_ShaderlibConVarAccessor;

ConVarBase* mat_fullbright=NULL;

void InitShaderLibCVars( CreateInterfaceFn cvarFactory )
{
	ConVar_Register( FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_CLIENTDLL, &g_ShaderlibConVarAccessor );

	mat_fullbright = g_pCVar->FindVarBase("mat_fullbright");
}
