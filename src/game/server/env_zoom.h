//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_ZOOM_H
#define ENV_ZOOM_H

class CBaseEntity;

bool CanOverrideEnvZoomOwner( CBaseEntity *pZoomOwner );
float GetZoomOwnerDesiredFOV( CBaseEntity *pZoomOwner );

#endif //ENV_ZOOM_H