//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PHONEMECONVERTER_H
#define PHONEMECONVERTER_H
#pragma once

#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL( LOG_PHONEMECONVERTER );

const char *ConvertPhoneme( int code );
int TextToPhoneme( const char *text );
float WeightForPhonemeCode( int code );
float WeightForPhoneme( char *text );

int NumPhonemes();
int TextToPhonemeIndex( const char *text );
const char *NameForPhonemeByIndex( int index );
int CodeForPhonemeByIndex( int index );
const char *DescForPhonemeByIndex( int index );
bool IsStandardPhoneme( int index );
unsigned char CodeToByteCode( int code );
int ByteCodeToCode( unsigned char byteCode );

#endif // PHONEMECONVERTER_H
