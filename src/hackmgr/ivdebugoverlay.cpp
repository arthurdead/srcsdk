#include "engine/ivdebugoverlay.h"
#include "overlaytext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API void IVDebugOverlay::PurgeTextOverlays()
{
	OverlayText_t *it = GetFirst();
	while(it) {
		if(it->m_flEndTime == 0.0f &&
			it->m_nCreationTick != -1)
		{
			it->m_nCreationTick = 0;
		}
		it = GetNext(it);
	}
}
