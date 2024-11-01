//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef STDSTRING_H
#define STDSTRING_H

#pragma once

#ifdef _WIN32
#pragma warning(push)
#include <yvals.h>	// warnings get enabled in yvals.h 
#pragma warning(disable:4663)
#pragma warning(disable:4530)
#pragma warning(disable:4245)
#pragma warning(disable:4018)
#pragma warning(disable:4511)
#endif

#include "tier0/valve_minmax_off.h"	// GCC 4.2.2 headers screw up our min/max defs.
#include <string>
#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

#ifdef _WIN32
#pragma warning(pop)
#endif

#include "datamap.h"

class CStdStringSaveRestoreOps : public CDefCustomFieldOps
{
public:
	enum
	{
		MAX_SAVE_LEN = 4096,
	};

	virtual void MakeEmpty( const FieldInfo_t &fieldInfo )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		pString->erase();
	}

	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		return pString->empty();
	}

	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		pString->assign(szValue);
		return true;
	}
};

extern ICustomFieldOps *GetStdStringDataOps();

#define DEFINE_STDSTRING(name) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, 0, NULL, GetStdStringDataOps(), NULL }

#define DEFINE_KEYSTDSTRING(name,mapname) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef, name), 0 }, 1, FTYPEDESC_KEY, mapname, GetStdStringDataOps(), NULL }

#endif // STDSTRING_H
