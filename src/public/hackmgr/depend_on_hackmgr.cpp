#include "hackmgr.h"
#include "module_name.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)

HackMgr_DependantModuleLoaded(modulename::dll);

HACKMGR_EXECUTE_ON_LOAD_END
