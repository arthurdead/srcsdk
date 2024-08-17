//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYSESSIONRECORDER_H
#define IREPLAYSESSIONRECORDER_H
#pragma once

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class IReplaySessionRecorder : public IBaseInterface
{
public:
	virtual void		StartRecording() = 0;
	virtual void		StopRecording( bool bAborting ) = 0;
	virtual void		SetCurrentRecordingStartTick( int nStartTick ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYSESSIONRECORDER_H