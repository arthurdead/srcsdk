#include "hackmgr/hackmgr.h"
#include "tier0/cpumonitoring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if HACKMGR_ENGINE_TARGET == HACKMGR_ENGINE_TARGET_SDK2013SP
DLL_EXPORT CPUFrequencyResults GetCPUFrequencyResults( bool fGetDisabledResults )
{
	// Return zero initialized results which means no data available.
	CPUFrequencyResults results = {};
	return results;
}
#endif
