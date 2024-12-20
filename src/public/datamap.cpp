#include "datamap.h"

#if defined GAME_DLL || defined CLIENT_DLL
	#include "predictableid.h"
#endif

#if defined GAME_DLL || defined CLIENT_DLL
	#include "variant_t.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static int g_baseFieldSizes[FIELD_BASE_TYPECOUNT] = 
{
	0,
	FIELD_SIZE( FIELD_FLOAT ),
	FIELD_SIZE( FIELD_INTERVAL ),
	FIELD_SIZE( FIELD_UINTEGER ),
	FIELD_SIZE( FIELD_INTEGER ),
	FIELD_SIZE( FIELD_INTEGER64 ),
	FIELD_SIZE( FIELD_UINTEGER64 ),
	FIELD_SIZE( FIELD_USHORT ),
	FIELD_SIZE( FIELD_SHORT ),
	FIELD_SIZE( FIELD_BOOLEAN ),
	FIELD_SIZE( FIELD_CHARACTER ),
	FIELD_SIZE( FIELD_UCHARACTER ),
	FIELD_SIZE( FIELD_SCHARACTER ),
	FIELD_SIZE( FIELD_MODELINDEX ),
	FIELD_SIZE( FIELD_COLOR32 ),
	FIELD_SIZE( FIELD_COLOR32E ),
	FIELD_SIZE( FIELD_COLOR24 ),
	FIELD_SIZE( FIELD_POOLED_STRING ),
	FIELD_SIZE( FIELD_CSTRING ),
	FIELD_SIZE( FIELD_VECTOR ),
	FIELD_SIZE( FIELD_QUATERNION ),
	FIELD_SIZE( FIELD_VMATRIX ),
	FIELD_SIZE( FIELD_MATRIX3X4 ),
	FIELD_SIZE( FIELD_QANGLE ),
	FIELD_SIZE( FIELD_VECTOR2D ),
	FIELD_SIZE( FIELD_VECTOR4D ),
#if defined GAME_DLL || defined CLIENT_DLL
	FIELD_SIZE( FIELD_PREDICTABLEID ),
#else
	0,
#endif
	FIELD_SIZE( FIELD_ENTITYPTR ),
	FIELD_SIZE( FIELD_EHANDLE ),
	FIELD_SIZE( FIELD_EDICTPTR ),
	FIELD_SIZE( FIELD_FUNCTION ),
	0,
#if defined GAME_DLL || defined CLIENT_DLL
	FIELD_SIZE( FIELD_VARIANT ),
#else
	0,
#endif
	0,
	0,
};

static const char *g_baseFieldNames[ FIELD_BASE_TYPECOUNT ][2] = 
{
	{"FIELD_VOID", "Void"},
	{"FIELD_FLOAT", "Float"},
	{"FIELD_INTERVAL", "Interval"},
	{"FIELD_UINTEGER", "UInt"},
	{"FIELD_INTEGER", "Int"},
	{"FIELD_INTEGER64", "Int64"},
	{"FIELD_UINTEGER64", "UInt64"},
	{"FIELD_USHORT", "Short"},
	{"FIELD_SHORT", "UShort"},
	{"FIELD_BOOLEAN", "Bool"},
	{"FIELD_CHARACTER", "Char"},
	{"FIELD_UCHARACTER", "SChar"},
	{"FIELD_SCHARACTER", "UChar"},
	{"FIELD_MODELINDEX", "ModelIndex"},
	{"FIELD_COLOR32", "Color32"},
	{"FIELD_COLOR32E", "Color32E"},
	{"FIELD_COLOR24", "Color24"},
	{"FIELD_POOLED_STRING", "StringT"},
	{"FIELD_CSTRING", "CString"},
	{"FIELD_VECTOR", "Vector"},
	{"FIELD_QUATERNION", "Quaternion"},
	{"FIELD_VMATRIX", "VMatrix"},
	{"FIELD_MATRIX3X4", "Matrix3x4"},
	{"FIELD_QANGLE", "QAngle"},
	{"FIELD_VECTOR2D", "Vector2D"},
	{"FIELD_VECTOR4D", "Vector4D"},
	{"FIELD_ENTITYPTR", "Entity Ptr"},
	{"FIELD_EHANDLE", "EHandle"},
	{"FIELD_EDICTPTR", "Edict Ptr"},
	{"FIELD_FUNCTION", "Function Ptr"},
	{"FIELD_INPUT", "Input"},
	{"FIELD_VARIANT", "Variant"},
	{"FIELD_CUSTOM", "Custom"},
	{"FIELD_EMBEDDED", "Embedded"},
};

static const char *g_extFieldNames[ FIELD_EXT_TYPECOUNT ][2] = 
{
	{"FIELD_RENDERMODE", "RenderMode"},
	{"FIELD_RENDERFX", "RenderFx"},
	{"FIELD_COLLISIONGROUP", "CollisionGroup"},
	{"FIELD_MOVETYPE", "MoveType"},
	{"FIELD_EFFECTS", "Effects"},
};

const char *GetFieldName( fieldtype_t type, bool pretty )
{
	if( type <= FIELD_BASE_TYPECOUNT ) {
		return g_baseFieldNames[ ((unsigned int)type & (unsigned int)FIELD_TYPE_MASK) ][ pretty ];
	} else if( type >= FIELD_LAST_FLAG ) {
		return g_extFieldNames[ ((unsigned int)type >> (unsigned int)FIELD_END_BITS) - 1 ][ pretty ];
	} else {
		switch(type) {
		case FIELD_TICK: return pretty ? "Tick" : "FIELD_TICK";
		case FIELD_TIME: return pretty ? "Time" : "FIELD_TIME";
		case FIELD_EXACT_CLASSNAME: return pretty ? "Classname" : "FIELD_EXACT_CLASSNAME";
		case FIELD_PARTIAL_CLASSNAME: return pretty ? "Classname" : "FIELD_PARTIAL_CLASSNAME";
		case FIELD_EXACT_TARGETNAME: return pretty ? "Targetname" : "FIELD_EXACT_TARGETNAME";
		case FIELD_PARTIAL_TARGETNAME: return pretty ? "Targetname" : "FIELD_PARTIAL_TARGETNAME";
		case FIELD_EXACT_TARGETNAME_OR_CLASSNAME: return pretty ? "GenericName" : "FIELD_EXACT_TARGETNAME_OR_CLASSNAME";
		case FIELD_PARTIAL_TARGETNAME_OR_CLASSNAME: return pretty ? "GenericName" : "FIELD_PARTIAL_TARGETNAME_OR_CLASSNAME";
		case FIELD_MATERIALINDEX: return pretty ? "MaterialIndex" : "FIELD_MATERIALINDEX";
		case FIELD_BRUSH_MODELIDNEX: return pretty ? "Brush ModelIndex" : "FIELD_BRUSH_MODELIDNEX";
		case FIELD_STUDIO_MODELIDNEX: return pretty ? "Studio ModelIndex" : "FIELD_STUDIO_MODELIDNEX";
		case FIELD_SPRITE_MODELINDEX: return pretty ? "Sprite ModelIndex" : "FIELD_SPRITE_MODELINDEX";
		case FIELD_POOLED_MODELNAME: return pretty ? "ModelName" : "FIELD_POOLED_MODELNAME";
		case FIELD_POOLED_SPRITENAME: return pretty ? "SpriteName" : "FIELD_POOLED_SPRITENAME";
		case FIELD_POOLED_SOUNDNAME: return pretty ? "SoundName" : "FIELD_POOLED_SOUNDNAME";
		case FIELD_VECTOR_WORLDSPACE: return pretty ? "Worldspace Vector" : "FIELD_VECTOR_WORLDSPACE";
		case FIELD_VMATRIX_WORLDSPACE: return pretty ? "Worldspace VMatrix" : "FIELD_VMATRIX_WORLDSPACE";
		case FIELD_MATRIX3X4_WORLDSPACE: return pretty ? "Worldspace Matrix3x4" : "FIELD_MATRIX3X4_WORLDSPACE";
		default: return NULL;
		}
	}
}

int GetFieldSize( fieldtype_t type )
{
	return g_baseFieldSizes[ ((unsigned int)type & (unsigned int)FIELD_TYPE_MASK) ];
}

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
	fieldType_ = FIELD_VOID;
	fieldName = NULL;
	fieldOffset_[TD_OFFSET_NORMAL] = 0;
	fieldOffset_[TD_OFFSET_PACKED] = 0;
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
	fieldType_ = type;
	fieldName = name;
	fieldOffset_[TD_OFFSET_NORMAL] = offset;
	fieldOffset_[TD_OFFSET_PACKED] = 0;
	fieldSize = count;
	flags = flags_;
	if(GetBaseFieldType(type) == FIELD_MODELINDEX) {
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
	fieldType_ = type;
	fieldName = name;
	fieldOffset_[TD_OFFSET_NORMAL] = 0;
	fieldOffset_[TD_OFFSET_PACKED] = 0;
	fieldSize = 0;
	flags = flags_;
	if(GetBaseFieldType(type) == FIELD_MODELINDEX) {
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
	fieldType_ = FIELD_CUSTOM;
	fieldName = name;
	fieldOffset_[TD_OFFSET_NORMAL] = offset;
	fieldOffset_[TD_OFFSET_PACKED] = 0;
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
	fieldType_ = FIELD_EMBEDDED;
	fieldName = name;
	fieldOffset_[TD_OFFSET_NORMAL] = offset;
	fieldOffset_[TD_OFFSET_PACKED] = 0;
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
