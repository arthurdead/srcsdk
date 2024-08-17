//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICLIENTENTITYINTERNAL_H
#define ICLIENTENTITYINTERNAL_H
#pragma once

#include "icliententity.h"
#include "clientleafsystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class ClientClass;


//-----------------------------------------------------------------------------
// represents a handle used only by the client DLL
//-----------------------------------------------------------------------------

typedef CBaseHandle ClientEntityHandle_t;
typedef unsigned short SpatialPartitionHandle_t;



#endif // ICLIENTENTITYINTERNAL_H