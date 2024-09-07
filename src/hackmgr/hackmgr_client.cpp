#include "hackmgr/hackmgr.h"
#include "cdll_int.h"
#include "client_class.h"
#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void ProcessRecvTable(CUtlString &path, RecvTable *pTable);

static void ProcessRecvProp(CUtlString &path, RecvProp *pProp)
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

	ProcessRecvTable(path, pProp->GetDataTable());
	ProcessRecvProp(path, pProp->GetArrayProp());
}

static void ProcessRecvTable(CUtlString &path, RecvTable *pTable)
{
	if(!pTable)
		return;

	path.Append(pTable->GetName());
	path.Append("::");

	int numProps = pTable->GetNumProps();
	for(int i = 0; i < numProps; ++i) {
		RecvProp *pProp = pTable->GetProp(i);
		if(!pProp)
			continue;
		CUtlString newpath(path);
		newpath.Append(pProp->GetName());
		newpath.Append("::");
		ProcessRecvProp(newpath, pProp);
	}
}

HACKMGR_API bool HackMgr_Client_PreInit(IBaseClientDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals)
{
	ClientClass *pClasses = pdll->GetAllClasses();
	while(pClasses) {
		CUtlString path(pClasses->GetName());
		path.Append("::");
		ProcessRecvTable(path, pClasses->m_pRecvTable);
		pClasses = pClasses->m_pNext;
	}

	return true;
}