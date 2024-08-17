//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SCENETOKENPROCESSOR_H
#define SCENETOKENPROCESSOR_H
#pragma once

class ISceneTokenProcessor;

ISceneTokenProcessor *GetTokenProcessor();
void SetTokenProcessorBuffer( const char *buf );

#endif // SCENETOKENPROCESSOR_H
