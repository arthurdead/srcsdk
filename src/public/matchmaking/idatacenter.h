#ifndef _IDATACENTER_H_
#define _IDATACENTER_H_

#pragma once

class IDatacenter;

#include "imatchsystem.h"
#include "tier0/platform.h"

abstract_class IDatacenter
{
public:
	//
	// GetStats
	//	retrieves the last received datacenter stats
	//
	virtual KeyValues * GetStats() = 0;
};


#endif // _IDATACENTER_H_
