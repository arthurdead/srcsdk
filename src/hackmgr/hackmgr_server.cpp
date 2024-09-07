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