//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHARED_CLASSNAMES_H
#define SHARED_CLASSNAMES_H
#pragma once

#if defined( CLIENT_DLL )
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
class C_BaseAnimating;
typedef C_BaseAnimating CSharedBaseAnimating;
class C_BasePlayer;
typedef C_BasePlayer CSharedBasePlayer;
#else
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
class CBaseAnimating;
typedef CBaseAnimating CSharedBaseAnimating;
class CBasePlayer;
typedef CBasePlayer CSharedBasePlayer;
#endif

#endif // SHARED_CLASSNAMES_H
