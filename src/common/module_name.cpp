#include "module_name.h"
#include "tier0/dbg.h"

#if !defined DLLNAME && !defined LIBNAME
	#error
#endif

namespace modulename
{
#ifdef DLLNAME
	const char *dll = V_STRINGIFY(DLLNAME);
#endif

#ifdef LIBNAME
	const char *lib = V_STRINGIFY(LIBNAME);
#endif
}
