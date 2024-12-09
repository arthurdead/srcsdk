#include <tier2/tier2.h>
#include "filesystem.h"

#undef ConnectTier2Libraries

#ifdef __MINGW32__
IBaseFileSystem *g_pBaseFileSystem = 0;
#endif

extern void ConnectTier2Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount );

void ConnectTier2LibrariesNew( CreateInterfaceFn *pFactoryList, int nFactoryCount )
{
	ConnectTier2Libraries( pFactoryList, nFactoryCount );

#ifdef __MINGW32__
	g_pBaseFileSystem = (IBaseFileSystem *)g_pFullFileSystem->QueryInterface( BASEFILESYSTEM_INTERFACE_VERSION );
#endif
}
