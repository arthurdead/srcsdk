#include "hackmgr/hackmgr.h"
#include "cdll_int.h"
#include "client_class.h"
#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _DEBUG
static void ProcessRecvTable(CUtlString &path, RecvTable *pTable);
#else
static void ProcessRecvTable(RecvTable *pTable);
#endif

#ifdef _DEBUG
static void ProcessRecvProp(CUtlString &path, RecvProp *pProp)
#else
static void ProcessRecvProp(RecvProp *pProp)
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

	//TODO!!!!! actually implement the flags
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

	ProcessRecvTable(path, pProp->GetDataTable());
	ProcessRecvProp(path, pProp->GetArrayProp());
#else
	ProcessRecvTable(pProp->GetDataTable());
	ProcessRecvProp(pProp->GetArrayProp());
#endif
}

#ifdef _DEBUG
static void ProcessRecvTable(CUtlString &path, RecvTable *pTable)
#else

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
		RecvProp *pProp = pTable->GetProp(i);
		if(!pProp)
			continue;
	#ifdef _DEBUG
		CUtlString newpath(path);
		newpath.Append(pProp->GetName());
		newpath.Append("::");
		ProcessRecvProp(newpath, pProp);
	#else
		ProcessRecvProp(pProp);
	#endif
	}
}

HACKMGR_API bool HackMgr_Client_PreInit(IBaseClientDLL *pdll, CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals)
{
	ClientClass *pClasses = pdll->GetAllClasses();
	while(pClasses) {
	#ifdef _DEBUG
		CUtlString path(pClasses->GetName());
		path.Append("::");
		ProcessRecvTable(path, pClasses->m_pRecvTable);
	#else
		ProcessRecvTable(pClasses->m_pRecvTable);
	#endif
		pClasses = pClasses->m_pNext;
	}

	return true;
}