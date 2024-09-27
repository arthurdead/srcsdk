#include "tier0/dbg.h"
#include "module_name_shared.h"

namespace modulename
{
#ifdef DLLNAME
	LIB_LOCAL const char *dll = V_STRINGIFY(DLLNAME);
#endif
}
