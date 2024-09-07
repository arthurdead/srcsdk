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
		DevMsg("%s has SPROP_COORD!!!\n", path.Get());
	}

	if(pProp->GetFlags() & SPROP_VARINT) {
		DevMsg("%s has SPROP_VARINT!!!\n", path.Get());
	}

	if(pProp->GetFlags() & SPROP_CELL_COORD) {
		DevMsg("%s has SPROP_CELL_COORD!!!\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD;
		pProp->m_Flags |= SPROP_COORD;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_LOWPRECISION) {
		DevMsg("%s has SPROP_CELL_COORD_LOWPRECISION!!!\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD_LOWPRECISION;
		pProp->m_Flags |= SPROP_COORD;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_INTEGRAL) {
		DevMsg("%s has SPROP_CELL_COORD_INTEGRAL!!!\n", path.Get());
		pProp->m_Flags &= ~SPROP_CELL_COORD_INTEGRAL;
		pProp->m_Flags |= SPROP_COORD;
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