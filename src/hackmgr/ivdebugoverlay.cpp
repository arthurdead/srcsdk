#include "engine/ivdebugoverlay.h"
#include "overlaytext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_CLASS_API void IVDebugOverlay::PurgeTextOverlays()
{
	OverlayText_t *it = GetFirst();
	while(it) {
		it->m_flEndTime = 0.0f;
		it = GetNext(it);
	}
}
