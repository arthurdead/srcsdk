//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef IREPLAYCAMERA_H
#define IREPLAYCAMERA_H
#pragma once

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

abstract_class IReplayCamera : public IBaseInterface
{
public:
	virtual void		ClearOverrideView() = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYCAMERA_H
