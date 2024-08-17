//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TE_EFFECT_DISPATCH_H
#define TE_EFFECT_DISPATCH_H
#pragma once


#include "effect_dispatch_data.h"
#include "recipientfilter.h"


void DispatchEffect( const char *pName, const CEffectData &data );
void DispatchEffect( const char *pName, const CEffectData &data, CRecipientFilter &filter );


#endif // TE_EFFECT_DISPATCH_H
