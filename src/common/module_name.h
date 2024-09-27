#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#pragma once

#include "tier0/dbg.h"
#include "module_name_shared.h"

namespace modulename
{
#ifdef DLLNAME
	static const char *dll = V_STRINGIFY(DLLNAME);
#else
	extern LIB_LOCAL const char *dll;
#endif

#ifdef LIBNAME
	static const char *lib = V_STRINGIFY(LIBNAME);
#endif
}

#endif
