//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Template entities are used by spawners to create copies of entities
//			that were configured by the level designer. This allows us to spawn
//			entities with arbitrary sets of key/value data and entity I/O
//			connections.
//
//=============================================================================//

#ifndef TEMPLATEENTITIES_H
#define TEMPLATEENTITIES_H
#pragma once

#include "string_t.h"

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
class CPointTemplate;
typedef CPointTemplate CSharedPointTemplate;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
class C_PointTemplate;
typedef C_PointTemplate CSharedPointTemplate;
#endif

int			Templates_Add(CSharedBaseEntity *pEntity, const char *pszMapData, int nLen, int nHammerID=-1);
string_t	Templates_FindByIndex( int iIndex );
int			Templates_GetStringSize( int iIndex );
string_t	Templates_FindByTargetName(const char *pszName);
void		Templates_ReconnectIOForGroup( CSharedPointTemplate *pGroup );

// Some templates have Entity I/O connecting the entities within the template.
// Unique versions of these templates need to be created whenever they're instanced.
void		Templates_StartUniqueInstance( void );
bool		Templates_IndexRequiresEntityIOFixup( int iIndex );
const char		*Templates_GetEntityIOFixedMapData( int iIndex );

// Used by Foundry.
void		Templates_RemoveByHammerID( int nHammerID );

#endif // TEMPLATEENTITIES_H
