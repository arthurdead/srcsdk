#include "hackmgr/hackmgr.h"
#include "eiface.h"
#include "server_class.h"
#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void ProcessSendTable(CUtlString &path, SendTable *pTable);

static void ProcessSendProp(CUtlString &path, SendProp *pProp)
{
	if(!pProp)
		return;

	path.Append(pProp->GetName());

	if(pProp->GetFlags() & SPROP_COORD) {
		DevMsg("Replacing SPROP_COORD with SPROP_COORD_MP in:\n\"%s\"\n", path.Get());
		pProp->m_Flags &= ~SPROP_COORD;
		pProp->m_Flags |= SPROP_COORD_MP;
	}

	if(pProp->GetFlags() & SPROP_CELL_COORD) {
		DevMsg("Replacing SPROP_CELL_COORD with SPROP_COORD_MP in:\n\"%s\"\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD;
		pProp->m_Flags |= SPROP_COORD_MP;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_LOWPRECISION) {
		DevMsg("Replacing SPROP_CELL_COORD_LOWPRECISION with SPROP_COORD_MP_LOWPRECISION in:\n\"%s\"\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD_LOWPRECISION;
		pProp->m_Flags |= SPROP_COORD_MP_LOWPRECISION;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_INTEGRAL) {
		DevMsg("Replacing SPROP_CELL_COORD_INTEGRAL with SPROP_COORD_MP_INTEGRAL in:\n\"%s\"\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD_INTEGRAL;
		pProp->m_Flags |= SPROP_COORD_MP_INTEGRAL;
	}

	path.Append("::");

	ProcessSendTable(path, pProp->GetDataTable());
	ProcessSendProp(path, pProp->GetArrayProp());
}

static void ProcessSendTable(CUtlString &path, SendTable *pTable)
{
	if(!pTable)
		return;

	path.Append(pTable->GetName());
	path.Append("::");

	int numProps = pTable->GetNumProps();
	for(int i = 0; i < numProps; ++i) {
		SendProp *pProp = pTable->GetProp(i);
		if(!pProp)
			continue;
		CUtlString newpath(path);
		newpath.Append(pProp->GetName());
		newpath.Append("::");
		ProcessSendProp(newpath, pProp);
	}
}

HACKMGR_API bool HackMgr_Server_PreInit(IServerGameDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals)
{
	ServerClass *pClasses = pdll->GetAllServerClasses();
	while(pClasses) {
		CUtlString path(pClasses->GetName());
		path.Append("::");
		ProcessSendTable(path, pClasses->m_pTable);
		pClasses = pClasses->m_pNext;
	}

	return true;
}