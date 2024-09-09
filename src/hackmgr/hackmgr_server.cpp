#include "hackmgr/hackmgr.h"
#include "eiface.h"
#include "server_class.h"
#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
static void ProcessSendTable(CUtlString &path, SendTable *pTable);
#endif

#ifdef _DEBUG
static void ProcessSendProp(CUtlString &path, SendProp *pProp)
#else
static void ProcessSendProp(SendProp *pProp)
#endif
{
	if(!pProp)
		return;

#ifdef _DEBUG
	path.Append(pProp->GetName());
#endif

	if(pProp->GetFlags() & SPROP_COORD) {
	#ifdef _DEBUG
		DevMsg("Replacing SPROP_COORD with SPROP_COORD_MP in:\n\"%s\"\n", path.Get());
	#endif
		pProp->m_Flags &= ~SPROP_COORD;
		pProp->m_Flags |= SPROP_COORD_MP;
	}

	if(pProp->GetFlags() & SPROP_CELL_COORD) {
	#ifdef _DEBUG
		DevMsg("Replacing SPROP_CELL_COORD with SPROP_COORD_MP in:\n\"%s\"\n", path.Get());
	#endif
		pProp->m_Flags &= ~SPROP_CELL_COORD;
		pProp->m_Flags |= SPROP_COORD_MP;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_LOWPRECISION) {
	#ifdef _DEBUG
		DevMsg("Replacing SPROP_CELL_COORD_LOWPRECISION with SPROP_COORD_MP_LOWPRECISION in:\n\"%s\"\n", path.Get());
	#endif
		pProp->m_Flags &= ~SPROP_CELL_COORD_LOWPRECISION;
		pProp->m_Flags |= SPROP_COORD_MP_LOWPRECISION;
	}
	if(pProp->GetFlags() & SPROP_CELL_COORD_INTEGRAL) {
	#ifdef _DEBUG
		DevMsg("Replacing SPROP_CELL_COORD_INTEGRAL with SPROP_COORD_MP_INTEGRAL in:\n\"%s\"\n", path.Get());
	#endif
		pProp->m_Flags &= ~SPROP_CELL_COORD_INTEGRAL;
		pProp->m_Flags |= SPROP_COORD_MP_INTEGRAL;
	}

#ifdef _DEBUG
	path.Append("::");

	ProcessSendTable(path, pProp->GetDataTable());
	ProcessSendProp(path, pProp->GetArrayProp());
#else
	ProcessSendTable(pProp->GetDataTable());
	ProcessSendProp(pProp->GetArrayProp());
#endif
}

#ifdef _DEBUG
static void ProcessSendTable(CUtlString &path, SendTable *pTable)
#else
static void ProcessSendTable(SendTable *pTable)
#endif
{
	if(!pTable)
		return;

#ifdef _DEBUG
	path.Append(pTable->GetName());
	path.Append("::");
#endif

	int numProps = pTable->GetNumProps();
	for(int i = 0; i < numProps; ++i) {
		SendProp *pProp = pTable->GetProp(i);
		if(!pProp)
			continue;
	#ifdef _DEBUG
		CUtlString newpath(path);
		newpath.Append(pProp->GetName());
		newpath.Append("::");
		ProcessSendProp(newpath, pProp);
	#else
		ProcessSendProp(pProp);
	#endif
	}
}

HACKMGR_API bool HackMgr_Server_PreInit(IServerGameDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals, bool bDedicated)
{
	ServerClass *pClasses = pdll->GetAllServerClasses();
	while(pClasses) {
	#ifdef _DEBUG
		CUtlString path(pClasses->GetName());
		path.Append("::");
		ProcessSendTable(path, pClasses->m_pTable);
	#else
		ProcessSendTable(pClasses->m_pTable);
	#endif
		pClasses = pClasses->m_pNext;
	}

	return true;
}