//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVGUIMATINFO_H
#define IVGUIMATINFO_H

#pragma once

#include "IVguiMatInfoVar.h"

// wrapper for IMaterial
class IVguiMatInfo
{
public:	
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
#ifdef __MINGW32__
private:
	virtual void __DTOR__() = 0;
	~IVguiMatInfo() = delete;
#else
public:
	virtual					~IVguiMatInfo( void ) { }
#endif

public:
	// make sure to delete the returned object after use!
	virtual IVguiMatInfoVar* FindVarFactory ( const char *varName, bool *found ) = 0;

	virtual int GetNumAnimationFrames ( ) = 0;

	// todo: if you need to add more IMaterial functions add them here
};

#endif //IVGUIMATINFO_H
