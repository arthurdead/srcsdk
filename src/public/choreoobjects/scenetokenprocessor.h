//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SCENETOKENPROCESSOR_H
#define SCENETOKENPROCESSOR_H
#pragma once

#include "iscenetokenprocessor.h"

//-----------------------------------------------------------------------------
// Purpose: Helper for parsing scene data file
//-----------------------------------------------------------------------------
class CSceneTokenProcessor : public ISceneTokenProcessor
{
public:
	const char	*CurrentToken( void );
	bool		GetToken( bool crossline );
	bool		TokenAvailable( void );
	void		Error( PRINTF_FORMAT_STRING const char *fmt, ... );
	void		SetBuffer( char *buffer );
private:
	const char	*m_pBuffer;
	char		m_szToken[ 1024 ];
};

extern CSceneTokenProcessor g_TokenProcessor;

class ISceneTokenProcessor;

ISceneTokenProcessor *GetTokenProcessor();
void SetTokenProcessorBuffer( const char *buf );

#endif // SCENETOKENPROCESSOR_H
