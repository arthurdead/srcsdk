//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef ENTITYAPI_H
#define ENTITYAPI_H

#pragma once

class SendTable;
struct edict_t;
class ServerClass;

extern void LoadMapEntities( const char *pMapEntities );
extern void	DispatchObjectCollisionBox( edict_t *pent );
extern float DispatchObjectPhysicsVelocity( edict_t *pent, float moveTime );
extern ServerClass* DispatchGetObjectServerClass(edict_t *pent);
extern ServerClass* GetAllServerClasses();

extern void ClearEntities( void );
extern void FreeContainingEntity( edict_t *ed );


#endif			// ENTITYAPI_H
