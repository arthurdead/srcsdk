#include "datamap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

pred_datamap_t *g_pPredDatamapsHead = NULL;
map_datamap_t *g_pMapDatamapsHead = NULL;

map_datamap_t::map_datamap_t(const char *name)
	: map_datamap_t(name, NULL, -1, NULL)
{
}

map_datamap_t::map_datamap_t(const char *name, datamap_t *base)
	: map_datamap_t(name, base, -1, NULL)
{
}

map_datamap_t::map_datamap_t(const char *name, const char *description)
	: map_datamap_t(name, NULL, -1, description)
{
}

map_datamap_t::map_datamap_t(const char *name, datamap_t *base, const char *description)
	: map_datamap_t(name, base, -1, description)
{
}

map_datamap_t::map_datamap_t(const char *name, int type)
	: map_datamap_t(name, NULL, type, NULL)
{
}

map_datamap_t::map_datamap_t(const char *name, datamap_t *base, int type)
	: map_datamap_t(name, base, type, NULL)
{

}

map_datamap_t::map_datamap_t(const char *name, int type, const char *description)
	: map_datamap_t(name, NULL, type, description)
{
}

map_datamap_t::map_datamap_t(const char *name, datamap_t *base, int type, const char *description)
{
	m_nType = type;
	m_pDescription = description;

	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = base;

	packed_offsets_computed = false;
	packed_size = 0;

#ifdef _DEBUG
	bValidityChecked = false;
#endif

	m_pNext = g_pMapDatamapsHead;
	g_pMapDatamapsHead = this;
}

pred_datamap_t::pred_datamap_t(const char *name)
	: pred_datamap_t(name, NULL)
{
}

pred_datamap_t::pred_datamap_t(const char *name, datamap_t *base)
{
	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = base;

	m_pNext = g_pPredDatamapsHead;
	g_pPredDatamapsHead = this;
}

typedescription_t::typedescription_t()
{
	fieldType = FIELD_VOID;
	fieldName = NULL;
	fieldOffset[0] = 0;
	fieldOffset[1] = 0;
	fieldSize = 0;
	flags = FTYPEDESC_NONE;
	externalName = NULL;
	pFieldOps = NULL;
	inputFunc = NULL;
	td = NULL;
	fieldSizeInBytes = 0;
	override_field = NULL;
	override_count = 0;
	fieldTolerance = 0;

	m_pDefValue = NULL;
	m_pGuiName = NULL;
	m_pDescription = NULL;
}

typedescription_t::typedescription_t(fieldtype_t type, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname, float tol)
{
	fieldType = type;
	fieldName = name;
	fieldOffset[0] = offset;
	fieldOffset[1] = 0;
	fieldSize = count;
	flags = flags_;
	if(type == FIELD_MODELINDEX) {
		flags |= FTYPEDESC_MODELINDEX;
	}
	externalName = fgdname;
	pFieldOps = NULL;
	inputFunc = NULL;
	td = NULL;
	fieldSizeInBytes = bytes;
	override_field = NULL;
	override_count = 0;
	fieldTolerance = tol;

	m_pDefValue = NULL;
	m_pGuiName = NULL;
	m_pDescription = NULL;
}

typedescription_t::typedescription_t(fieldtype_t type, const char *name, inputfunc_t func, fieldflags_t flags_, const char *fgdname)
{
	fieldType = type;
	fieldName = name;
	fieldOffset[0] = 0;
	fieldOffset[1] = 0;
	fieldSize = 0;
	flags = flags_;
	if(type == FIELD_MODELINDEX) {
		flags |= FTYPEDESC_MODELINDEX;
	}
	externalName = fgdname;
	pFieldOps = NULL;
	inputFunc = func;
	td = NULL;
	fieldSizeInBytes = 0;
	override_field = NULL;
	override_count = 0;
	fieldTolerance = 0;

	m_pDefValue = NULL;
	m_pGuiName = NULL;
	m_pDescription = NULL;
}

typedescription_t::typedescription_t(ICustomFieldOps *funcs, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname, float tol)
{
	fieldType = FIELD_CUSTOM;
	fieldName = name;
	fieldOffset[0] = offset;
	fieldOffset[1] = 0;
	fieldSize = count;
	flags = flags_;
	externalName = fgdname;
	pFieldOps = funcs;
	inputFunc = NULL;
	td = NULL;
	fieldSizeInBytes = bytes;
	override_field = NULL;
	override_count = 0;
	fieldTolerance = tol;

	m_pDefValue = NULL;
	m_pGuiName = NULL;
	m_pDescription = NULL;
}

typedescription_t::typedescription_t(datamap_t *embed, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname)
{
	fieldType = FIELD_EMBEDDED;
	fieldName = name;
	fieldOffset[0] = offset;
	fieldOffset[1] = 0;
	fieldSize = count;
	flags = flags_;
	externalName = fgdname;
	pFieldOps = NULL;
	inputFunc = NULL;
	td = embed;
	fieldSizeInBytes = bytes;
	override_field = NULL;
	override_count = 0;
	fieldTolerance = 0;

	m_pDefValue = NULL;
	m_pGuiName = NULL;
	m_pDescription = NULL;
}
