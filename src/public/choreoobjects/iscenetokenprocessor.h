//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISCENETOKENPROCESSOR_H
#define ISCENETOKENPROCESSOR_H
#pragma once

#include "tier0/platform.h"

abstract_class ISceneTokenProcessor
{
public:
	virtual const char	*CurrentToken( void ) = 0;
	virtual bool		GetToken( bool crossline ) = 0;
	virtual bool		TokenAvailable( void ) = 0;
	virtual void		Error( PRINTF_FORMAT_STRING const char *fmt, ... ) = 0;
};

#endif // ISCENETOKENPROCESSOR_H