//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVGUIMATINFOVAR_H
#define IVGUIMATINFOVAR_H

#pragma once


// wrapper for IMaterialVar
class IVguiMatInfoVar
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
#ifdef __MINGW32__
private:
	virtual void __DTOR__() = 0;
	~IVguiMatInfoVar() = delete;
#else
public:
	virtual					~IVguiMatInfoVar( void ) { }
#endif

public:
	virtual int GetIntValue ( void ) const = 0;
	virtual void SetIntValue ( int val ) = 0;

	// todo: if you need to add more IMaterialVar functions add them here
};

#endif //IVGUIMATINFOVAR_H
