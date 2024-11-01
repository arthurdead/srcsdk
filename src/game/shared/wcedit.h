//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions having to do with WC Edit mode
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef WCEDIT_H
#define WCEDIT_H
#pragma once

#include "tier0/logging.h"

DECLARE_LOGGING_CHANNEL( LOG_FOUNDRY );

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

//=============================================================================
//	>> NWCEdit
//=============================================================================
namespace NWCEdit
{
	bool	IsWCVersionValid(void);
	void	RememberEntityPosition( CSharedBaseEntity *pEntity );
	void	UpdateEntityPosition( CSharedBaseEntity *pEntity );
};

#endif // WCEDIT_H