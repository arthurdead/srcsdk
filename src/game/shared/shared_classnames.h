//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHARED_CLASSNAMES_H
#define SHARED_CLASSNAMES_H
#pragma once

// Hacky macros to allow shared code to work without even worse macro-izing
#if defined( CLIENT_DLL )

#define CBaseEntity				C_BaseEntity
#define CBaseCombatCharacter	C_BaseCombatCharacter
#define CBaseAnimating			C_BaseAnimating
#define CBasePlayer				C_BasePlayer

#endif


#endif // SHARED_CLASSNAMES_H
