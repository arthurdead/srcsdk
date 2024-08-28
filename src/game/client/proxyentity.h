//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the client dll
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROXY_ENTITY_H
#define PROXY_ENTITY_H

#include "materialsystem/imaterialproxy.h"
#include "imaterialproxydict.h"

class IMaterialVar;
class C_BaseEntity;

//-----------------------------------------------------------------------------
// Base class all material proxies should inherit from
//-----------------------------------------------------------------------------
abstract_class CEntityMaterialProxy : public IMaterialProxy
{
public:
	virtual void Release( void );

private:
	virtual void OnBind( void *pC_BaseEntity ) final;

protected:
	// base classes should implement these
	virtual void OnBind( C_BaseEntity *pBaseEntity ) = 0;
};


#endif // PROXY_ENTITY_H

