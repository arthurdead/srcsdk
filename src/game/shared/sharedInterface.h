//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exposes client-server neutral interfaces implemented in both places
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHAREDINTERFACE_H
#define SHAREDINTERFACE_H

#pragma once

class IFileSystem;
class IUniformRandomStream;
class CGaussianRandomStream;
class IEngineSound;
class IMapData;

#if defined(_STATIC_LINKED) && defined(_SUBSYSTEM) && (defined(CLIENT_DLL) || defined(GAME_DLL))
namespace _SUBSYSTEM
{
extern IUniformRandomStream		*random_valve;
}
#else
extern IUniformRandomStream		*random_valve;
#endif
extern CGaussianRandomStream *randomgaussian;
extern IEngineSound				*enginesound;
extern IMapData					*g_pMapData;			// TODO: current implementations of the 
														// interface are in TF2, should probably move
														// to TF2/HL2 neutral territory

#endif // SHAREDINTERFACE_H

