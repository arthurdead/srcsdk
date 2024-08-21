#pragma once

#if defined(_WIN32) && !(_MSC_VER >= 1900)
#include "typeinfo.h"
// BUGBUG: typeinfo stomps some of the warning settings (in yvals.h)
#pragma warning(disable:4244)
#else
#include <typeinfo>
#endif