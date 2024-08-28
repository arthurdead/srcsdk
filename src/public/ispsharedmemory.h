//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef ISPSHAREDMEMORY_H
#define ISPSHAREDMEMORY_H
#pragma once

#include "basetypes.h"
#include "platform.h"

abstract_class ISPSharedMemory
{
public:
	virtual bool	Init( size_t iSize ) = 0; //Initial implementation assumes the size is fixed/hardcoded, returns true if this call actually created the memory, false if it already existed
	virtual uint8 *	Base( void ) = 0;
	virtual size_t	Size( void ) = 0;
	
	virtual void	AddRef( void ) = 0;
	virtual void	Release( void ) = 0;
};

#endif
