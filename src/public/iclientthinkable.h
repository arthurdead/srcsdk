//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLIENTTHINKABLE_H
#define ICLIENTTHINKABLE_H
#pragma once


#include "iclientunknown.h"


class CClientThinkHandlePtr;
typedef CClientThinkHandlePtr* ClientThinkHandle_t;


// Entities that implement this interface can be put into the client think list.
abstract_class IClientThinkable
{
	friend class CClientThinkList;
	friend class CClientEntityList;

public:
	// Gets at the containing class...
	virtual IClientUnknown*		GetIClientUnknown() = 0;

	virtual void				Think() = 0;

	// Called when you're added to the think list.
	// GetThinkHandle's return value must be initialized to INVALID_THINK_HANDLE.
	virtual ClientThinkHandle_t	GetThinkHandle() = 0;
	virtual void				SetThinkHandle( ClientThinkHandle_t hThink ) = 0;

private:
	// Called by the client when it deletes the entity.
	virtual void				DO_NOT_USE_Release() = 0;
};


#endif // ICLIENTTHINKABLE_H
