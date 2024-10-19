#include "hackmgr/hackmgr.h"
#include "createinterface.h"
#include "hackmgr_internal.h"
#include "tier1/convar.h"
#include "toolframework/itooldictionary.h"
#include "toolframework/itoolframework.h"
#include "toolframework/itoolsystem.h"
#include "dlloverride.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static IToolFrameworkInternal *s_pToolFrameworkInternal = NULL;

class CToolFrameworkInternal : public IToolFrameworkInternal
{
public:
	CUtlVector< IToolSystem * >		m_ToolSystems;
	CUtlVector< IToolDictionary * >	m_Dictionaries;
	CUtlVector< CSysModule * >		m_Modules;
	int m_nActiveToolIndex;
	bool m_bInToolMode;
};

static void CToolFrameworkInternal_LoadToolsFromLibrary( CToolFrameworkInternal *pthis, const char *dllname )
{
	CSysModule *module = Sys_LoadModule( dllname );
	if ( !module )
	{
		Warning( "CToolFrameworkInternal::LoadToolsFromLibrary:  Unable to load '%s'\n", dllname );
		return;
	}

	CreateInterfaceFn factory = Sys_GetFactory( module );
	if ( !factory )
	{
		Sys_UnloadModule( module );
		Warning( "CToolFrameworkInternal::LoadToolsFromLibrary:  Dll '%s' has no factory\n", dllname );
		return;
	}

	IToolDictionary *dictionary = ( IToolDictionary * )factory( VTOOLDICTIONARY_INTERFACE_VERSION, NULL );
	if ( !dictionary )
	{
		Sys_UnloadModule( module );
		Warning( "CToolFrameworkInternal::LoadToolsFromLibrary:  Dll '%s' doesn't support '%s'\n", dllname, VTOOLDICTIONARY_INTERFACE_VERSION );
		return;
	}

	if ( !dictionary->Connect( Launcher_AppSystemCreateInterface ) )
	{
		Sys_UnloadModule( module );
		Warning( "CToolFrameworkInternal::LoadToolsFromLibrary:  Dll '%s' connection phase failed.\n", dllname );
		return;
	}

	if ( dictionary->Init( ) != INIT_OK )
	{
		Sys_UnloadModule( module );
		Warning( "CToolFrameworkInternal::LoadToolsFromLibrary:  Dll '%s' initialization phase failed.\n", dllname );
		return;
	}

	dictionary->CreateTools();

	int toolCount = dictionary->GetToolCount();
	for ( int i = 0; i < toolCount; ++i )
	{
		IToolSystem *tool = dictionary->GetTool( i );
		if ( tool )
		{
			Msg( "Loaded tool '%s'\n", tool->GetToolName() );
			pthis->m_ToolSystems.AddToTail( tool );
		}
	}

	pthis->m_Dictionaries.AddToTail( dictionary );
	pthis->m_Modules.AddToTail( module );
}

static void CToolFrameworkInternal_UnloadToolsFromLibrary( CToolFrameworkInternal *pthis, const char *dllname )
{

}

CON_COMMAND(toolload, "")
{
	if(args.ArgC() != 2) {
		Msg("toolload <name>\n");
		return;
	}

	if(!s_pToolFrameworkInternal) {
		return;
	}

	CToolFrameworkInternal_LoadToolsFromLibrary( (CToolFrameworkInternal *)s_pToolFrameworkInternal, args.Arg(1) );
}

CON_COMMAND(toolunload, "")
{
	if(args.ArgC() != 2) {
		Msg("toolunload <name>\n");
		return;
	}

	if(!s_pToolFrameworkInternal) {
		return;
	}

	CToolFrameworkInternal_UnloadToolsFromLibrary( (CToolFrameworkInternal *)s_pToolFrameworkInternal, args.Arg(1) );
}

CON_COMMAND(toollist, "")
{
	if(!s_pToolFrameworkInternal) {
		return;
	}

	CToolFrameworkInternal *pToolFramework = (CToolFrameworkInternal *)s_pToolFrameworkInternal;

	for( int i =0; i < pToolFramework->m_ToolSystems.Count(); ++i) {
		Msg("%s\n", pToolFramework->m_ToolSystems[i]->GetToolName());
	}
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)

if(!GetEngineInterfaceFactory())
	return;

int status = IFACE_OK;
s_pToolFrameworkInternal = (IToolFrameworkInternal *)GetEngineInterfaceFactory()(VTOOLFRAMEWORK_INTERFACE_VERSION, &status);
if(status != IFACE_OK)
	s_pToolFrameworkInternal = NULL;

HACKMGR_EXECUTE_ON_LOAD_END
