//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MAPBASE_DATADESC_MOD_H
#define MAPBASE_DATADESC_MOD_H

#pragma once

#include "datamap.h"

struct variant_t;

char *Datadesc_SetFieldString( const char *szValue, CBaseEntity *pObject, typedescription_t *pField, fieldtype_t *pFieldType = NULL );

bool ReadUnregisteredKeyfields( CBaseEntity *pTarget, const char *szKeyName, variant_t *variant );

#endif
