//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOLIDSETDEFAULTS_H
#define SOLIDSETDEFAULTS_H
#pragma once

#include "vcollide_parse.h"

enum ContentsFlags_t : unsigned int;

// solid_t parsing
class CSolidSetDefaults : public IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue );
	virtual void SetDefaults( void *pData );

	ContentsFlags_t GetContentsMask() { return m_contentsMask; }

private:
	ContentsFlags_t m_contentsMask;
};

extern CSolidSetDefaults g_SolidSetup;

#endif // SOLIDSETDEFAULTS_H
