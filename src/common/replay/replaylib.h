//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYLIB_H
#define REPLAYLIB_H
#pragma once

//----------------------------------------------------------------------------------------

class IClientReplayContext;

//----------------------------------------------------------------------------------------

bool ReplayLib_Init( const char *pGameDir, IClientReplayContext *pClientReplayContext );

//----------------------------------------------------------------------------------------

#endif // REPLAYLIB_H