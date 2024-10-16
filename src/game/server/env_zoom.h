//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_ZOOM_H
#define ENV_ZOOM_H

#pragma once

class CBaseEntity;

bool CanOverrideEnvZoomOwner( CBaseEntity *pZoomOwner );
float GetZoomOwnerDesiredFOV( CBaseEntity *pZoomOwner );

#endif //ENV_ZOOM_H