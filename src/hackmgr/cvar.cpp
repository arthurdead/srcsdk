#include "hackmgr/hackmgr.h"
#include "hackmgr_internal.h"
#include "createinterface.h"
#include "icvar.h"
#include "vstdlib/cvar.h"

#include "tier0/dbg.h"
#include "tier1/iconvar.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "icvar.h"
#include "Color.h"

#define private public
#include "tier1/convar.h"
#undef private

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ICvar *pCvar = NULL;

void FindCmdCB( const CCommand &args )
{
	const char *search;
	const ConCommandBase *var;

	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage:  find <string>\n" );
		return;
	}

	// Get substring to find
	search = args[1];

	// Loop through vars and print out findings
	for ( var = pCvar->GetCommands(); var; var=var->GetNext() )
	{
		if ( var->IsFlagSet(FCVAR_DEVELOPMENTONLY) || var->IsFlagSet(FCVAR_HIDDEN) )
			continue;

		if ( !Q_stristr( var->GetName(), search ) &&
			!Q_stristr( var->GetHelpText(), search ) )
			continue;

		ConVar_PrintDescription( var );	
	}
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)

if(!VStdLib_GetICVarFactory()) {
	return;
}

int status = IFACE_OK;
pCvar = (ICvar *)VStdLib_GetICVarFactory()(CVAR_INTERFACE_VERSION, &status);
if(!pCvar || status != IFACE_OK) {
	return;
}

ConVar *r_hunkalloclightmaps = pCvar->FindVar("r_hunkalloclightmaps");
if(r_hunkalloclightmaps) {
	r_hunkalloclightmaps->SetValue(false);
}

ConVar *developer = pCvar->FindVar("developer");
if(developer) {
#ifdef _DEBUG
	developer->SetValue(4);
#else
	developer->SetValue(0);
#endif
}

ConVar *sv_cheats = pCvar->FindVar("sv_cheats");
if(sv_cheats) {
#ifdef _DEBUG
	sv_cheats->SetValue(true);
#else
	sv_cheats->SetValue(false);
#endif
}

ConCommand *pFindCmd = pCvar->FindCommand("find");
if(pFindCmd) {
	/*
	pFindCmd->m_fnCommandCallback = FindCmdCB;
	pFindCmd->m_bUsingNewCommandCallback = true;
	pFindCmd->m_bUsingCommandCallbackInterface = false;
	*/
}

HACKMGR_EXECUTE_ON_LOAD_END
