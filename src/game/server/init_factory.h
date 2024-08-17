//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INIT_FACTORY_H
#define INIT_FACTORY_H
#pragma once


#include "interface.h"

struct factorylist_t
{
	CreateInterfaceFn engineFactory;
	CreateInterfaceFn physicsFactory;
	CreateInterfaceFn fileSystemFactory;
};

// Store off the factories
void FactoryList_Store( const factorylist_t &sourceData );

// retrieve the stored factories
void FactoryList_Retrieve( factorylist_t &destData );

#endif // INIT_FACTORY_H
