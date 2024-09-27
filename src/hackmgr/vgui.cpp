#include "vgui/ISurface.h"
#include "hackmgr/hackmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

void vgui::ISurface::DrawTexturedRectEx( vgui::DrawTexturedRectParms_t *pDrawParms )
{
	DrawTexturedSubRect(
		pDrawParms->x0,
		pDrawParms->y0,
		pDrawParms->x1,
		pDrawParms->y1,
		pDrawParms->s0,
		pDrawParms->t0,
		pDrawParms->s1,
		pDrawParms->t1
	);
}
