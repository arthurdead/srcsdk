//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DATAMAP_H
#define DATAMAP_H
#pragma once

#ifndef VECTOR_H
#include "mathlib/vector.h"
#endif
#include "ihandleentity.h"
#include "string_t.h"
#include "mathlib/vector4d.h"
#include "basehandle.h"
#include "interval.h"
#include "mathlib/vmatrix.h"
#include "bittools.h"
#include "networkvar.h"

#if defined( CLIENT_DLL ) || defined( GAME_DLL )
	#include "ehandle.h"
#endif

#include "tier1/utlvector.h"

#include "engine/ivmodelinfo.h"

#ifdef GNUC
#undef offsetof
#define offsetof(s,m)	__builtin_offsetof(s,m)
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

struct edict_t;

// SINGLE_INHERITANCE restricts the size of CBaseEntity pointers-to-member-functions to 4 bytes
#ifdef GAME_DLL
class SINGLE_INHERITANCE CBaseEntity;
typedef CBaseEntity CGameBaseEntity;
#elif defined CLIENT_DLL
class SINGLE_INHERITANCE C_BaseEntity;
typedef C_BaseEntity CGameBaseEntity;
#else
class SINGLE_INHERITANCE CGameBaseEntity;
#endif
struct inputdata_t;

#define INVALID_TIME (FLT_MAX * -1.0) // Special value not rebased on save/load

enum fieldtype_t : uint64
{
	FIELD_VOID = 0,			// No type or value

	FIELD_FLOAT,			// Any floating point value
	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )

	FIELD_UINTEGER,
	FIELD_INTEGER,			// Any integer or enum
	FIELD_INTEGER64,		// 64bit integer
	FIELD_UINTEGER64,
	FIELD_USHORT,
	FIELD_SHORT,			// 2 byte integer
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_CHARACTER,		// a byte
	FIELD_UCHARACTER,
	FIELD_SCHARACTER,

	FIELD_MODELINDEX,

	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_COLOR32E,
	FIELD_COLOR24,

	FIELD_POOLED_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_CSTRING,

	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_VMATRIX,
	FIELD_MATRIX3X4,
	FIELD_QANGLE,
	FIELD_VECTOR2D,			// 2 floats
	FIELD_VECTOR4D,			// 4 floats

	FIELD_PREDICTABLEID,

	FIELD_ENTITYPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICTPTR,			// edict_t *

	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_INPUT,
	FIELD_VARIANT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription

	FIELD_BASE_TYPECOUNT,		// MUST BE LAST

	FIELD_TYPE_BITS = MINIMUM_BITS_NEEDED(FIELD_EMBEDDED),
	FIELD_TYPE_MASK = 0x3F,

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_TYPE_FLAG_WORLDSPACE = ((1 << 0) << FIELD_TYPE_BITS), //maps some local space to world space (translation is fixed up on level transitions)
	FIELD_TYPE_FLAG_MODEL      = ((1 << 1) << FIELD_TYPE_BITS), // Engine string that is a model name (needs precache)
	FIELD_TYPE_FLAG_MATERIAL   = ((1 << 2) << FIELD_TYPE_BITS), // a material index (using the material precache string table)
	FIELD_TYPE_FLAG_SOUND      = ((1 << 3) << FIELD_TYPE_BITS), // Engine string that is a sound name (needs precache)
	FIELD_TYPE_FLAG_PARTIAL    = ((1 << 4) << FIELD_TYPE_BITS),
	FIELD_TYPE_FLAG_TARGETNAME = ((1 << 5) << FIELD_TYPE_BITS),

	FIELD_FLAGS_MASK = 0x1F,

	FIELD_LAST_FLAG = FIELD_TYPE_FLAG_TARGETNAME,
	FIELD_END_BITS = MINIMUM_BITS_NEEDED(FIELD_LAST_FLAG << 1),

	FIELD_TICK = (FIELD_INTEGER|FIELD_TYPE_FLAG_SOUND),				// an integer tick count( fixed up similarly to time)
	FIELD_TIME = (FIELD_FLOAT|FIELD_TYPE_FLAG_SOUND),				// a floating point time (these are fixed up automatically too!)
	FIELD_DISTANCE = (FIELD_FLOAT|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_SCALE = (FIELD_FLOAT|FIELD_TYPE_FLAG_PARTIAL),
	FIELD_EXACT_CLASSNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_PARTIAL_CLASSNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_WORLDSPACE|FIELD_TYPE_FLAG_PARTIAL),
	FIELD_EXACT_TARGETNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_TARGETNAME),
	FIELD_PARTIAL_TARGETNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_TARGETNAME|FIELD_TYPE_FLAG_PARTIAL),
	FIELD_EXACT_TARGETNAME_OR_CLASSNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_TARGETNAME|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_PARTIAL_TARGETNAME_OR_CLASSNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_TARGETNAME|FIELD_TYPE_FLAG_WORLDSPACE|FIELD_TYPE_FLAG_PARTIAL),
	FIELD_MATERIALINDEX = (FIELD_INTEGER|FIELD_TYPE_FLAG_MATERIAL),
	FIELD_BRUSH_MODELIDNEX = (FIELD_MODELINDEX|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_STUDIO_MODELIDNEX = (FIELD_MODELINDEX|FIELD_TYPE_FLAG_MODEL),
	FIELD_SPRITE_MODELINDEX = (FIELD_MODELINDEX|FIELD_TYPE_FLAG_MATERIAL),
	FIELD_POOLED_MODELNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_MODEL),
	FIELD_POOLED_SPRITENAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_MATERIAL),
	FIELD_POOLED_SOUNDNAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_SOUND),
	FIELD_POOLED_SCENENAME = (FIELD_POOLED_STRING|FIELD_TYPE_FLAG_PARTIAL),
	FIELD_VECTOR_WORLDSPACE = (FIELD_VECTOR|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_VMATRIX_WORLDSPACE = (FIELD_VMATRIX|FIELD_TYPE_FLAG_WORLDSPACE),
	FIELD_MATRIX3X4_WORLDSPACE = (FIELD_MATRIX3X4|FIELD_TYPE_FLAG_WORLDSPACE),

	FIELD_RENDERMODE =     (FIELD_UCHARACTER|(1 << FIELD_END_BITS)),
	FIELD_RENDERFX =       (FIELD_UCHARACTER|(2 << FIELD_END_BITS)),
	FIELD_COLLISIONGROUP = (FIELD_UINTEGER|  (3 << FIELD_END_BITS)),
	FIELD_MOVETYPE =       (FIELD_UCHARACTER|(4 << FIELD_END_BITS)),
	FIELD_EFFECTS =        (FIELD_USHORT|    (5 << FIELD_END_BITS)),
	FIELD_MOVECOLLIDE =    (FIELD_UCHARACTER|(6 << FIELD_END_BITS)),
	FIELD_TEAMNUM =        (FIELD_UCHARACTER|(7 << FIELD_END_BITS)),
	FIELD_CPULEVEL =       (FIELD_UCHARACTER|(8 << FIELD_END_BITS)),
	FIELD_GPULEVEL =       (FIELD_UCHARACTER|(9 << FIELD_END_BITS)),
	FIELD_THREESTATE =     (FIELD_UCHARACTER|(10 << FIELD_END_BITS)),
	FIELD_NPCSTATE =       (FIELD_UCHARACTER|(11 << FIELD_END_BITS)),

	FIELD_EXT_TYPECOUNT = (FIELD_MOVECOLLIDE >> FIELD_END_BITS),		// MUST BE LAST
};

COMPILE_TIME_ASSERT(MINIMUM_BITS_NEEDED(FIELD_LAST_FLAG >> FIELD_TYPE_BITS) == 5); //update FIELD_FLAGS_MASK
COMPILE_TIME_ASSERT(FIELD_TYPE_BITS == 6); //update FIELD_TYPE_MASK


//
// Function prototype for all input handlers.
//
typedef void (CGameBaseEntity::*inputfunc_t)(inputdata_t &&data);

//-----------------------------------------------------------------------------
// Field sizes... 
//-----------------------------------------------------------------------------
template <int FIELD_TYPE>
class CDatamapFieldInfo;

template <typename T>
class CNativeFieldInfo;

#define DECLARE_FIELD_TYPE_INFO( _fieldType, ... )	\
	template<> \
	class CDatamapFieldInfo<(_fieldType)> \
	{ \
	public: \
		using native_type = __VA_ARGS__; \
		using info_type = CDatamapFieldInfo<(_fieldType)>; \
		static inline auto FIELDTYPE = (_fieldType); \
		static inline auto NATIVESIZE = sizeof(native_type); \
	};

#define DECLARE_FIELD_NATIVE_INFO( _fieldType, ... )	\
	template <> \
	class CNativeFieldInfo<__VA_ARGS__> \
	{ \
	public: \
		using native_type = __VA_ARGS__; \
		using info_type = CDatamapFieldInfo<(_fieldType)>; \
		static inline auto FIELDTYPE = (_fieldType); \
		static inline auto NATIVESIZE = sizeof(native_type); \
	};

#define DECLARE_FIELD_NETWORK_INFO( _fieldType, netclass, ... )	\
	template <typename H> \
	class CNativeFieldInfo<netclass< __VA_OPT__(__VA_ARGS__,) H>> \
	{ \
	public: \
		using native_type = typename NetworkVarType< netclass< __VA_OPT__(__VA_ARGS__,) H> >::type; \
		using info_type = CDatamapFieldInfo<(_fieldType)>; \
		static inline auto FIELDTYPE = (_fieldType); \
		static inline auto NATIVESIZE = sizeof(native_type); \
	};

#define DECLARE_FIELD_ENUM( ... )	\
	template <> \
	class CNativeFieldInfo<__VA_ARGS__> \
	{ \
	public: \
		using native_type = __VA_ARGS__; \
		using info_type = typename CNativeFieldInfo<__underlying_type(__VA_ARGS__)>::info_type; \
		static inline auto FIELDTYPE = info_type::FIELDTYPE; \
		static inline auto NATIVESIZE = sizeof(native_type); \
	};

#define DECLARE_FIELD_INFO( _fieldType, ... )	\
	DECLARE_FIELD_TYPE_INFO( _fieldType, __VA_ARGS__ ) \
	DECLARE_FIELD_NATIVE_INFO( _fieldType, __VA_ARGS__ )

#define FIELD_SIZE( _fieldType )	CDatamapFieldInfo<(_fieldType)>::NATIVESIZE
#define FIELD_BITS( _fieldType )	(FIELD_SIZE( _fieldType ) * 8)

class variant_t;
typedef variant_t game_variant_t;

class CPredictableId;
typedef CPredictableId CGamePredictableId;

template<>
class CDatamapFieldInfo<FIELD_VARIANT>;
template<>
class CNativeFieldInfo<game_variant_t>;

enum Team_t : unsigned char;
enum CPULevel_t : unsigned char;
enum GPULevel_t : unsigned char;
enum MemLevel_t : unsigned char;
enum GPUMemLevel_t : unsigned char;
enum NPC_STATE : unsigned char;

DECLARE_FIELD_INFO( FIELD_FLOAT,		float )
DECLARE_FIELD_INFO( FIELD_POOLED_STRING,		string_t )
DECLARE_FIELD_INFO( FIELD_VECTOR,		Vector )
DECLARE_FIELD_INFO( FIELD_QUATERNION,	Quaternion)
DECLARE_FIELD_INFO( FIELD_INTEGER,		int)
DECLARE_FIELD_INFO( FIELD_BOOLEAN,		bool)
DECLARE_FIELD_INFO( FIELD_SHORT,		short)
DECLARE_FIELD_INFO( FIELD_CHARACTER,	char)
DECLARE_FIELD_INFO( FIELD_COLOR32,		color32)
DECLARE_FIELD_INFO( FIELD_COLOR32E,		ColorRGBExp32)
DECLARE_FIELD_INFO( FIELD_COLOR24,		color24)
DECLARE_FIELD_INFO( FIELD_ENTITYPTR,		CGameBaseEntity *)
DECLARE_FIELD_INFO( FIELD_EHANDLE,		CBaseHandle)
DECLARE_FIELD_INFO( FIELD_EDICTPTR,		edict_t *)
DECLARE_FIELD_INFO( FIELD_FUNCTION,		inputfunc_t)
DECLARE_FIELD_INFO( FIELD_VMATRIX,		VMatrix)
DECLARE_FIELD_INFO( FIELD_MATRIX3X4,	matrix3x4_t)
DECLARE_FIELD_INFO( FIELD_INTERVAL,		 interval_t)   // NOTE:  Must match interval.h definition
DECLARE_FIELD_INFO( FIELD_MODELINDEX,	modelindex_t) 
DECLARE_FIELD_INFO( FIELD_VECTOR2D,		Vector2D) 
DECLARE_FIELD_INFO( FIELD_INTEGER64,	int64)
DECLARE_FIELD_INFO( FIELD_VECTOR4D,		 Vector4D ) 
DECLARE_FIELD_INFO( FIELD_CSTRING,		 const char * ) 
DECLARE_FIELD_INFO( FIELD_UINTEGER,		 unsigned int ) 
DECLARE_FIELD_INFO( FIELD_USHORT,		 unsigned short ) 
DECLARE_FIELD_INFO( FIELD_UINTEGER64,		 uint64 ) 
DECLARE_FIELD_INFO( FIELD_UCHARACTER,		 unsigned char ) 
DECLARE_FIELD_INFO( FIELD_SCHARACTER,		 signed char ) 
DECLARE_FIELD_INFO( FIELD_QANGLE,		 QAngle ) 
DECLARE_FIELD_INFO( FIELD_RENDERMODE,		 RenderMode_t ) 
DECLARE_FIELD_INFO( FIELD_RENDERFX,		 RenderFx_t ) 
DECLARE_FIELD_INFO( FIELD_EFFECTS,		 Effects_t ) 
DECLARE_FIELD_INFO( FIELD_MOVETYPE,		 MoveType_t ) 
DECLARE_FIELD_INFO( FIELD_MOVECOLLIDE,		 MoveCollide_t ) 
DECLARE_FIELD_INFO( FIELD_COLLISIONGROUP,		 Collision_Group_t ) 
DECLARE_FIELD_INFO( FIELD_TEAMNUM,		 Team_t ) 
DECLARE_FIELD_INFO( FIELD_CPULEVEL,		 CPULevel_t ) 
DECLARE_FIELD_INFO( FIELD_GPULEVEL,		 GPULevel_t ) 
DECLARE_FIELD_INFO( FIELD_THREESTATE,		 ThreeState_t ) 
DECLARE_FIELD_INFO( FIELD_NPCSTATE,		 NPC_STATE ) 

DECLARE_FIELD_TYPE_INFO( FIELD_TICK, int )
DECLARE_FIELD_TYPE_INFO( FIELD_TIME, float )

DECLARE_FIELD_TYPE_INFO( FIELD_EXACT_CLASSNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_PARTIAL_CLASSNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_EXACT_TARGETNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_PARTIAL_TARGETNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_EXACT_TARGETNAME_OR_CLASSNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_PARTIAL_TARGETNAME_OR_CLASSNAME, string_t )

DECLARE_FIELD_TYPE_INFO( FIELD_MATERIALINDEX, int )

DECLARE_FIELD_TYPE_INFO( FIELD_BRUSH_MODELIDNEX, modelindex_t )
DECLARE_FIELD_TYPE_INFO( FIELD_STUDIO_MODELIDNEX, modelindex_t )
DECLARE_FIELD_TYPE_INFO( FIELD_SPRITE_MODELINDEX, modelindex_t )

DECLARE_FIELD_TYPE_INFO( FIELD_POOLED_MODELNAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_POOLED_SPRITENAME, string_t )
DECLARE_FIELD_TYPE_INFO( FIELD_POOLED_SOUNDNAME, string_t )

DECLARE_FIELD_TYPE_INFO( FIELD_VECTOR_WORLDSPACE, Vector )
DECLARE_FIELD_TYPE_INFO( FIELD_VMATRIX_WORLDSPACE, VMatrix )
DECLARE_FIELD_TYPE_INFO( FIELD_MATRIX3X4_WORLDSPACE, matrix3x4_t )

DECLARE_FIELD_NETWORK_INFO( FIELD_COLOR32E, CNetworkColor32EBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_COLOR32, CNetworkColor32Base )
DECLARE_FIELD_NETWORK_INFO( FIELD_COLOR24, CNetworkColor24Base )
DECLARE_FIELD_NETWORK_INFO( FIELD_QANGLE, CNetworkQAngleBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_QANGLE, CNetworkVectorBaseImpl, QAngle )
DECLARE_FIELD_NETWORK_INFO( FIELD_VECTOR, CNetworkVectorBaseImpl, Vector )
DECLARE_FIELD_NETWORK_INFO( FIELD_VECTOR, CNetworkVectorBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_VECTOR_WORLDSPACE, CNetworkVectorWorldspaceBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_QUATERNION, CNetworkQuaternionBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_POOLED_STRING, CNetworkStringTBaseImpl )
DECLARE_FIELD_NETWORK_INFO( FIELD_MODELINDEX, CNetworkModelIndexBaseImpl )
DECLARE_FIELD_NETWORK_INFO( FIELD_TIME, CNetworkTimeBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_SCALE, CNetworkScaleBase )
DECLARE_FIELD_NETWORK_INFO( FIELD_DISTANCE, CNetworkDistanceBase )

#if defined( CLIENT_DLL ) || defined( GAME_DLL )
template <typename T, typename H>
class CNativeFieldInfo<CNetworkHandleBaseImpl<T, H>>
{
public:
	using native_type = CHandle<T>;
	using info_type = CDatamapFieldInfo<FIELD_EHANDLE>;
	static inline auto FIELDTYPE = FIELD_EHANDLE;
	static inline auto NATIVESIZE = sizeof(native_type);
};

template <typename T, typename H>
class CNativeFieldInfo<CNetworkHandleBase<T, H>>
{
public:
	using native_type = CHandle<T>;
	using info_type = CDatamapFieldInfo<FIELD_EHANDLE>;
	static inline auto FIELDTYPE = FIELD_EHANDLE;
	static inline auto NATIVESIZE = sizeof(native_type);
};
#endif

template <typename T, typename H>
class CNativeFieldInfo<CNetworkVarArithmeticBaseImpl<T, H>>
{
public:
	using native_type = T;
	using info_type = typename CNativeFieldInfo<T>::info_type;
	static inline auto FIELDTYPE = info_type::FIELDTYPE;
	static inline auto NATIVESIZE = sizeof(native_type);
};

template <typename T, typename H>
class CNativeFieldInfo<CNetworkVarBase<T, H>>
{
public:
	using native_type = T;
	using info_type = typename CNativeFieldInfo<T>::info_type;
	static inline auto FIELDTYPE = info_type::FIELDTYPE;
	static inline auto NATIVESIZE = sizeof(native_type);
};

template <typename T, typename H>
class CNativeFieldInfo<CNetworkVarBaseImpl<T, H>>
{
public:
	using native_type = T;
	using info_type = typename CNativeFieldInfo<T>::info_type;
	static inline auto FIELDTYPE = info_type::FIELDTYPE;
	static inline auto NATIVESIZE = sizeof(native_type);
};

inline fieldtype_t GetBaseFieldType( fieldtype_t type )
{ return (fieldtype_t)((uint64)type & (uint64)FIELD_TYPE_MASK); }
inline unsigned char GetFieldTypeFlags( fieldtype_t type )
{ return (unsigned char)(((uint64)type >> (uint64)FIELD_TYPE_BITS) & (uint64)FIELD_FLAGS_MASK); }

int GetFieldSize( fieldtype_t type );
const char *GetFieldName( fieldtype_t type, bool pretty );

#define ARRAYSIZE2D(p)		(sizeof((p))/sizeof((p)[0][0]))
#define SIZE_OF_ARRAY(p)	_ARRAYSIZE(p)

#define FIELD_BUILDER(...) \
	(([]() -> typedescription_t __VA_ARGS__)())

#define _FIELD(name,fieldtype,count,flags,mapname,tolerance) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), (count), (flags), mapname, (tolerance))

#define _FIELD_ARRAYELEM(name,i,fieldtype,count,flags,mapname,tolerance) \
	typedescription_t((fieldtype), #name "[" #i "]", sizeof(((classNameTypedef *)0)->name[(i)]), (offsetof(classNameTypedef, name) + (sizeof(((classNameTypedef *)0)->name[(i)]) * (i))), (count), (flags), mapname, (tolerance))

#define DEFINE_FIELD_NULL \
	typedescription_t()

#define DEFINE_FIELD(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL, 0)
#define DEFINE_FIELD_FLAGS(name,fieldtype, flags) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL, 0)
#define DEFINE_FIELD_FLAGS_TOL(name,fieldtype, flags,tolerance) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL, (tolerance))

#define DEFINE_KEYFIELD(name,fieldtype, mapname) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, mapname, 0)

#define DEFINE_FIELD_NAME(localname,netname,fieldtype) \
	typedescription_t((fieldtype), #localname, sizeof(((classNameTypedef *)0)->localname), 1, offsetof(classNameTypedef, localname), FTYPEDESC_NONE, netname, 0)
#define DEFINE_FIELD_NAME_TOL(localname,netname,fieldtolerance) \
	typedescription_t((fieldtype), #localname, sizeof(((classNameTypedef *)0)->localname), 1, offsetof(classNameTypedef, localname), FTYPEDESC_NONE, netname, (fieldtolerance))

#define DEFINE_KEYFIELD_ARRAYELEM(name,i,fieldtype, mapname) \
	typedescription_t((fieldtype), #name "[" #i "]", sizeof(((classNameTypedef *)0)->name[(i)]), 1, (offsetof(classNameTypedef, name) + (sizeof(((classNameTypedef *)0)->name[(i)]) * (i))), FTYPEDESC_KEY, mapname, 0)

#define DEFINE_AUTO_ARRAY(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL, 0)
#define DEFINE_AUTO_ARRAY_KEYFIELD(name,fieldtype,mapname) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), FTYPEDESC_NONE, mapname, 0)

#define DEFINE_ARRAY(name,fieldtype, count) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL, 0)
#define DEFINE_ARRAY_FLAGS(name,fieldtype, count,flags) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), (flags), NULL, 0)
#define DEFINE_ARRAY_FLAGS_TOL(name,fieldtype, count,flags,tolerance) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), (flags), NULL, (tolerance))

#define DEFINE_ENTITY_FIELD(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, #name, 0)

#define DEFINE_CUSTOM_FIELD(name,datafuncs) \
	typedescription_t((datafuncs), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL, 0)
#define DEFINE_CUSTOM_KEYFIELD(name,datafuncs,mapname) \
	typedescription_t((datafuncs), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, mapname, 0)

#define DEFINE_AUTO_ARRAY2D(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), ARRAYSIZE2D(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), 0, NULL, 0)
// Used by byteswap datadescs
#define DEFINE_BITFIELD(name,fieldtype,bitcount) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (((bitcount+FIELD_BITS(fieldtype)-1)&~(FIELD_BITS(fieldtype)-1)) / FIELD_BITS(fieldtype)), offsetof(classNameTypedef, name), 0, NULL, 0)
#define DEFINE_INDEX(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_INDEX, NULL, 0)

#define DEFINE_EMBEDDED( name )						\
	typedescription_t(&(((classNameTypedef *)0)->name.m_DataMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL)
#define DEFINE_PRED_EMBEDDED( name )						\
	typedescription_t(&(((classNameTypedef *)0)->name.m_PredMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL)
#define DEFINE_PRED_EMBEDDED_FLAGS( name, flags )						\
	typedescription_t(&(((classNameTypedef *)0)->name.m_PredMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL)

#define DEFINE_EMBEDDED_OVERRIDE( name, overridetype )	\
	typedescription_t(&(((overridetype *)0)->name.m_DataMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL)

#define DEFINE_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_DataMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_MAP_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_MapDataDesc), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_PRED_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_PredMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_PRED_EMBEDDED_PTR_FLAGS( name, flags )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_PredMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR|(flags), NULL)

#define DEFINE_EMBEDDED_ARRAY( name, count )			\
	typedescription_t(&(((classNameTypedef *)0)->name[0].m_DataMap), #name, sizeof(((classNameTypedef *)0)->name[0]), (count), offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL)
#define DEFINE_EMBEDDED_AUTO_ARRAY( name )			\
	typedescription_t(&(((classNameTypedef *)0)->name[0].m_DataMap), #name, sizeof(((classNameTypedef *)0)->name[0]), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL)

//#define DEFINE_DATA( name, fieldextname, flags ) _FIELD(name, fieldtype, 1,  flags, extname )

// INPUTS
#define DEFINE_INPUT( name, fieldtype, inputname ) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_INPUT|FTYPEDESC_KEY, inputname, 0)
#define DEFINE_INPUTFUNC( fieldtype, inputname, inputfunc ) \
	typedescription_t((fieldtype), #inputfunc, static_cast <inputfunc_t> (&classNameTypedef::inputfunc), FTYPEDESC_INPUT, inputname)

// OUTPUTS
// the variable 'name' MUST BE derived from CBaseOutput
// we know the output type from the variable itself, so it doesn't need to be specified here

class ICustomFieldOps;
extern ICustomFieldOps *eventFuncs;
#define DEFINE_OUTPUT( name, outputname ) \
	typedescription_t(eventFuncs, #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_OUTPUT|FTYPEDESC_KEY, outputname, 0)

// Quick way to define variants in a datadesc.
extern ICustomFieldOps *variantFuncs;
#define DEFINE_VARIANT(name) \
	typedescription_t(variantFuncs, #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_NONE, NULL, 0)
#define DEFINE_KEYVARIANT(name,mapname) \
	typedescription_t(variantFuncs, #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, mapname, 0)

// replaces EXPORT table for portability and non-DLL based systems (xbox)
#define DEFINE_FUNCTION_RAW( function, func_type ) \
	typedescription_t(FIELD_VOID, nameHolder.GenerateName(#function), ((func_type)(&classNameTypedef::function)), FTYPEDESC_FUNCTIONTABLE, NULL)
#define DEFINE_FUNCTION( function ) \
	typedescription_t(FIELD_VOID, nameHolder.GenerateName(#function), ((inputfunc_t)(&classNameTypedef::function)), FTYPEDESC_FUNCTIONTABLE, NULL)

enum fieldflags_t : uint64
{
	FTYPEDESC_NONE =                    0,

	FTYPEDESC_KEY =               (1 << 2),		// This field can be requested and written to by string name at load time
	FTYPEDESC_INPUT =             (1 << 3),		// This field can be written to by string name at run time, and a function called
	FTYPEDESC_OUTPUT =            (1 << 4),		// This field propogates it's value to all targets whenever it changes
	FTYPEDESC_FUNCTIONTABLE =     (1 << 5),		// This is a table entry for a member function pointer
	FTYPEDESC_PTR =               (1 << 6),		// This field is a pointer, not an embedded object
	FTYPEDESC_OVERRIDE =          (1 << 7),		// The field is an override for one in a base class (only used by prediction system for now)

// Flags used by other systems (e.g., prediction system)
	FTYPEDESC_INSENDTABLE =       (1 << 8),		// This field is present in a network SendTable
	FTYPEDESC_PRIVATE =           (1 << 9),		// The field is local to the client or server only (not referenced by prediction code and not replicated by networking)
	FTYPEDESC_NOERRORCHECK =      (1 << 10),		// The field is part of the prediction typedescription, but doesn't get compared when checking for errors

	FTYPEDESC_MODELINDEX =        (1 << 11),		// The field is a model index (used for debugging output)

	FTYPEDESC_INDEX =             (1 << 12),		// The field is an index into file data, used for byteswapping. 

// These flags apply to C_BasePlayer derived objects only
	FTYPEDESC_VIEW_OTHER_PLAYER = (1 << 13),		// By default you can only view fields on the local player (yourself), 
													//   but if this is set, then we allow you to see fields on other players
	FTYPEDESC_VIEW_OWN_TEAM =     (1 << 14),		// Only show this data if the player is on the same team as the local player
	FTYPEDESC_VIEW_NEVER =        (1 << 15),		// Never show this field to anyone, even the local player (unusual)
};

FLAGENUM_OPERATORS( fieldflags_t, uint64 )

#define TD_MSECTOLERANCE		0.001f		// This is a FIELD_FLOAT and should only be checked to be within 0.001 of the networked info

struct datamap_t;
struct typedescription_t;

struct FieldInfo_t
{
private:
	void *			   pField;

public:
	// Note that it is legal for the following two fields to be NULL,
	// though it may be disallowed by implementors of ISaveRestoreOps
	void *			   pOwner;

	const typedescription_t *pTypeDesc;

	template <typename T>
	T *GetField() const;
};

abstract_class ICustomFieldOps
{
private:
	virtual void Unused1( const FieldInfo_t &fieldInfo, void * ) {}
	virtual void Unused2( const FieldInfo_t &fieldInfo, void * ) {}

public:
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo ) = 0;
	virtual void MakeEmpty( const FieldInfo_t &fieldInfo ) = 0;
	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue ) = 0;
};

enum : unsigned char
{
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,

	// Must be last
	TD_OFFSET_COUNT,
};

template <typename T>
T *GetField(void *pObject, const typedescription_t &desc);

extern void MapField_impl( typedescription_t &ret, const char *name, int offset, int size, fieldtype_t type, fieldflags_t flags_ );

struct FGdChoice
{
	FGdChoice(const char *value_, const char *name_)
		: value(value_), name(name_)
	{
	}

	const char *value;
	const char *name;
};

struct typedescription_t
{
	friend class CByteswap;

	typedescription_t();
	typedescription_t(const typedescription_t &) = delete;
	typedescription_t &operator=(const typedescription_t &) = delete;
	typedescription_t(typedescription_t &&) = default;
	typedescription_t &operator=(typedescription_t &&) = delete;

	typedescription_t(fieldtype_t type, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname, float tol);
	typedescription_t(fieldtype_t type, const char *name, inputfunc_t func, fieldflags_t flags_, const char *fgdname);
	typedescription_t(ICustomFieldOps *funcs, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname, float tol);
	typedescription_t(datamap_t *embed, const char *name, int bytes, int count, int offset, fieldflags_t flags_, const char *fgdname);

private:
	friend void MapField_impl( typedescription_t &ret, const char *name, int offset, int size, fieldtype_t type, fieldflags_t flags_ );

	fieldtype_t			fieldType_;

public:
	fieldtype_t baseType() const
	{ return GetBaseFieldType(fieldType_); }
	fieldtype_t rawType() const
	{ return fieldType_; }

	unsigned char typeFlags() const
	{ return GetFieldTypeFlags(fieldType_); }

	const char			*fieldName;

private:
	unsigned int					fieldOffset_[ TD_OFFSET_COUNT ]; // 0 == normal, 1 == packed offset

	template <typename T>
	friend T *GetField(void *pObject, const typedescription_t &desc);

public:
	int rawOffset() const
	{ return fieldOffset_[TD_OFFSET_NORMAL]; }

	unsigned short		fieldSize;
	fieldflags_t				flags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char			*externalName;	
	// pointer to the function set for save/restoring of custom data types
	ICustomFieldOps		*pFieldOps; 
	// for associating function with string names
	inputfunc_t			inputFunc; 
	// For embedding additional datatables inside this one
	datamap_t			*td;

	// Stores the actual member variable size in bytes
	unsigned int					fieldSizeInBytes;

	// FTYPEDESC_OVERRIDE point to first baseclass instance if chains_validated has occurred
	typedescription_t *override_field;

	// Used to track exclusion of baseclass fields
	int					override_count;
  
	// Tolerance for field errors for float fields
	float				fieldTolerance;

	const char *m_pDefValue;
	const char *m_pGuiName;
	const char *m_pDescription;
	const FGdChoice *m_pChoices;
	int m_nChoicesLen;
};

template <typename T>
T *GetField(void *pObject, const typedescription_t &desc)
{
	Assert( sizeof(T) == desc.fieldSizeInBytes );
	if(desc.flags & FTYPEDESC_PTR) {
		T *ptr = *(T **)((unsigned char *)pObject + desc.fieldOffset_[TD_OFFSET_NORMAL]);
		Assert( ptr != NULL );
		return ptr;
	} else {
		return (T *)((unsigned char *)pObject + desc.fieldOffset_[TD_OFFSET_NORMAL]);
	}
}

template <typename T>
T *FieldInfo_t::GetField() const
{
	Assert( sizeof(T) == pTypeDesc->fieldSizeInBytes );
	if(pTypeDesc->flags & FTYPEDESC_PTR) {
		T *ptr = *(T **)((unsigned char *)pField);
		Assert( ptr != NULL );
		return ptr;
	} else {
		return (T *)((unsigned char *)pField);
	}
}

#define DEFINE_MAP_FIELD(name, ...) \
	MapField_impl<decltype(classNameTypedef::name)>( #name, offsetof(classNameTypedef, name) __VA_OPT__(, __VA_ARGS__) )

#define DEFINE_MAP_INPUT(name, ...) \
	MapField_impl( #name, static_cast<inputfunc_t>(&classNameTypedef::name) __VA_OPT__(, __VA_ARGS__) )

inline typedescription_t MapField_impl( const char *name, inputfunc_t func, const char *fgdname, fieldtype_t type )
{
	typedescription_t ret;
	MapField_impl(ret, name, 0, sizeof(inputfunc_t), type, FTYPEDESC_INPUT );
	ret.m_pGuiName = fgdname;
	ret.externalName = fgdname;
	ret.inputFunc = func;
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, fieldtype_t type, fieldflags_t flags_ )
{
	typedescription_t ret;
	MapField_impl(ret, name, offset, sizeof(T), type, FTYPEDESC_KEY|flags_ );
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset )
{
	return MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE );
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, const char *fgdname )
{
	typedescription_t ret = MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE );
	ret.externalName = fgdname;
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, const char *fgdname, const char *uiname )
{
	typedescription_t ret = MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE, fgdname );
	ret.m_pGuiName = uiname;
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, const char *fgdname, const char *uiname, const char *desc )
{
	typedescription_t ret = MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE, fgdname, uiname );
	ret.m_pDescription = desc;
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, fieldtype_t type, const char *fgdname, const char *uiname, const char *desc )
{
	typedescription_t ret = MapField_impl<T>( name, offset, type, FTYPEDESC_NONE, fgdname, uiname, desc );
	return ret;
}

template <typename T>
typedescription_t MapField_impl( const char *name, int offset, const char *fgdname, const char *uiname, const char *desc, const char *defval )
{
	typedescription_t ret = MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE, fgdname, uiname, desc );
	ret.m_pDefValue = defval;
	return ret;
}

template <typename T, int len>
typedescription_t MapField_impl( const char *name, int offset, const char *fgdname, const char *uiname, const char *desc, const char *defval, const FGdChoice (&choices)[len] )
{
	typedescription_t ret = MapField_impl<T>( name, offset, CNativeFieldInfo<T>::FIELDTYPE, FTYPEDESC_NONE, fgdname, uiname, desc, defval );
	ret.m_pChoices = choices;
	ret.m_nChoicesLen = len;
	return ret;
}

//-----------------------------------------------------------------------------
// Used by ops that deal with pointers
//-----------------------------------------------------------------------------
class CClassPtrFieldOps : public ICustomFieldOps
{
public:
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		void **ppClassPtr = fieldInfo.GetField<void *>();
		int nObjects = fieldInfo.pTypeDesc->fieldSize;
		for ( int i = 0; i < nObjects; i++ )
		{
			if ( ppClassPtr[i] != NULL )
				return false;
		}
		return true;
	}

	virtual void MakeEmpty( const FieldInfo_t &fieldInfo )
	{
		memset( fieldInfo.GetField<void *>(), 0, fieldInfo.pTypeDesc->fieldSize * sizeof( void * ) );
	}

	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue )
	{
		Assert(0);
		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: stores the list of objects in the hierarchy
//			used to iterate through an object's data descriptions
//-----------------------------------------------------------------------------
struct datamap_t
{
	typedescription_t	*dataDesc;
	int					dataNumFields;
	char const			*dataClassName;
	datamap_t			*baseMap;

	bool				chains_validated;
	// Have the "packed" offsets been computed
	bool				packed_offsets_computed;
	int					packed_size;

#if defined( _DEBUG )
	bool				bValidityChecked;
#endif // _DEBUG
};

struct pred_datamap_t : public datamap_t
{
	pred_datamap_t(const pred_datamap_t &) = delete;
	pred_datamap_t &operator=(const pred_datamap_t &) = delete;
	pred_datamap_t(pred_datamap_t &&) = delete;
	pred_datamap_t &operator=(pred_datamap_t &&) = delete;

	pred_datamap_t(const char *name);
	pred_datamap_t(const char *name, datamap_t *base);

	pred_datamap_t *m_pNext;
};

extern pred_datamap_t *g_pPredDatamapsHead;

struct map_datamap_t : public datamap_t
{
	map_datamap_t(const map_datamap_t &) = delete;
	map_datamap_t &operator=(const map_datamap_t &) = delete;
	map_datamap_t(map_datamap_t &&) = delete;
	map_datamap_t &operator=(map_datamap_t &&) = delete;

	map_datamap_t(const char *name);
	map_datamap_t(const char *name, datamap_t *base);

	map_datamap_t(const char *name, int type);
	map_datamap_t(const char *name, datamap_t *base, int type);

	map_datamap_t(const char *name, const char *description);
	map_datamap_t(const char *name, datamap_t *base, const char *description);

	map_datamap_t(const char *name, int type, const char *description);
	map_datamap_t(const char *name, datamap_t *base, int type, const char *description);

	map_datamap_t *m_pNext;
	int m_nType;
	const char *m_pDescription;
};

extern map_datamap_t *g_pMapDatamapsHead;

//-----------------------------------------------------------------------------
//
// Macros used to implement datadescs
//
#define DECLARE_FRIEND_DATADESC_ACCESS()	\
	template <typename T> friend void DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *DataMapInit(T *);

#define DECLARE_SIMPLE_DATADESC() \
	static datamap_t m_DataMap; \
	static datamap_t *GetBaseMap(); \
	template <typename T> friend void DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *DataMapInit(T *);

#define DECLARE_SIMPLE_DATADESC_INSIDE_NAMESPACE() \
	static datamap_t m_DataMap; \
	static datamap_t *GetBaseMap(); \
	template <typename T> friend void ::DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *::DataMapInit(T *);

#define	DECLARE_DATADESC() \
	DECLARE_SIMPLE_DATADESC() \
	virtual datamap_t *GetDataDescMap( void );

#define BEGIN_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC_NO_BASE( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_SIMPLE_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_SIMPLE_DATADESC_( className, BaseClass ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC_GUTS( className ) \
	template <typename T> datamap_t *DataMapInit(T *); \
	template <> datamap_t *DataMapInit<className>( className * ); \
	namespace className##_DataDescInit \
	{ \
		datamap_t *g_DataMapHolder = DataMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static CDatadescGeneratedNameHolder nameHolder(#className); \
		className::m_DataMap.baseMap = className::GetBaseMap(); \
		static typedescription_t dataDesc[] = \
		{ \
		typedescription_t(), /* so you can define "empty" tables */

#define BEGIN_DATADESC_GUTS_NAMESPACE( className, nameSpace ) \
	template <typename T> datamap_t *nameSpace::DataMapInit(T *); \
	template <> datamap_t *nameSpace::DataMapInit<className>( className * ); \
	namespace className##_DataDescInit \
	{ \
		datamap_t *g_DataMapHolder = nameSpace::DataMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *nameSpace::DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static CDatadescGeneratedNameHolder nameHolder(#className); \
		className::m_DataMap.baseMap = className::GetBaseMap(); \
		static typedescription_t dataDesc[] = \
		{ \
		typedescription_t(), /* so you can define "empty" tables */

#define END_DATADESC() \
		}; \
		\
		if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) ) \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = SIZE_OF_ARRAY( dataDesc ) - 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = &dataDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = dataDesc; \
		} \
		return &classNameTypedef::m_DataMap; \
	}

// used for when there is no data description
#define IMPLEMENT_NULL_SIMPLE_DATADESC( derivedClass ) \
	BEGIN_SIMPLE_DATADESC( derivedClass ) \
	END_DATADESC()

#define IMPLEMENT_NULL_SIMPLE_DATADESC_( derivedClass, baseClass ) \
	BEGIN_SIMPLE_DATADESC_( derivedClass, baseClass ) \
	END_DATADESC()

#define IMPLEMENT_NULL_DATADESC( derivedClass ) \
	BEGIN_DATADESC( derivedClass ) \
	END_DATADESC()

// helps get the offset of a bitfield
#define BEGIN_BITFIELD( name ) \
	union \
	{ \
		char name; \
		struct \
		{

#define END_BITFIELD() \
		}; \
	};

//-----------------------------------------------------------------------------
// Forward compatability with potential seperate byteswap datadescs

#define DECLARE_BYTESWAP_DATADESC() DECLARE_SIMPLE_DATADESC()
#define BEGIN_BYTESWAP_DATADESC(name) BEGIN_SIMPLE_DATADESC(name) 
#define BEGIN_BYTESWAP_DATADESC_(name,base) BEGIN_SIMPLE_DATADESC_(name,base) 
#define END_BYTESWAP_DATADESC() END_DATADESC()

//-----------------------------------------------------------------------------

template <typename T> 
inline void DataMapAccess(T *ignored, datamap_t **p)
{
	*p = &T::m_DataMap;
}

template <typename T> datamap_t* DataMapInit(T*);

//-----------------------------------------------------------------------------

class CDatadescGeneratedNameHolder
{
public:
	CDatadescGeneratedNameHolder( const char *pszBase );
	
	~CDatadescGeneratedNameHolder();
	
	const char *GenerateName( const char *pszIdentifier );
	
private:
	const char *m_pszBase;
	size_t m_nLenBase;
	CUtlVector<char *> m_Names;
};

#include "tier0/memdbgon.h"

inline CDatadescGeneratedNameHolder::CDatadescGeneratedNameHolder( const char *pszBase )
 : m_pszBase(pszBase)
{
	m_nLenBase = strlen( m_pszBase );
}

inline CDatadescGeneratedNameHolder::~CDatadescGeneratedNameHolder()
{
	for ( int i = 0; i < m_Names.Count(); i++ )
	{
		delete m_Names[i];
	}
}

inline const char *CDatadescGeneratedNameHolder::GenerateName( const char *pszIdentifier )
{
	char *pBuf = new char[m_nLenBase + strlen(pszIdentifier) + 1];
	strcpy( pBuf, m_pszBase );
	strcat( pBuf, pszIdentifier );
	m_Names.AddToTail( pBuf );
	return pBuf;
}

#include "tier0/memdbgoff.h"

#endif // DATAMAP_H
