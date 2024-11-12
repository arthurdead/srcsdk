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

#include "tier1/utlvector.h"

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

typedef enum _fieldtypes
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_COLOR24,
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t *

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_MODELINDEX,		// a model index
	FIELD_MATERIALINDEX,	// a material index (using the material precache string table)
	
	FIELD_VECTOR2D,			// 2 floats

	FIELD_INTEGER64,		// 64bit integer

	FIELD_VECTOR4D,			// 4 floats

	FIELD_TYPECOUNT,		// MUST BE LAST
} fieldtype_t;


//
// Function prototype for all input handlers.
//
#ifdef GAME_DLL
typedef void (CBaseEntity::*inputfunc_t)(inputdata_t &data);
#elif defined CLIENT_DLL
typedef void (C_BaseEntity::*inputfunc_t)(inputdata_t &data);
#else
typedef void (CGameBaseEntity::*inputfunc_t)(inputdata_t &data);
#endif

//-----------------------------------------------------------------------------
// Field sizes... 
//-----------------------------------------------------------------------------
template <int FIELD_TYPE>
class CDatamapFieldSizeDeducer
{
public:
	enum
	{
		SIZE = 0
	};

	static int FieldSize( )
	{
		return 0;
	}
};

#define DECLARE_FIELD_SIZE( _fieldType, _fieldSize )	\
	template< > class CDatamapFieldSizeDeducer<(_fieldType)> { public: enum { SIZE = (_fieldSize) }; static int FieldSize() { return (_fieldSize); } };
#define FIELD_SIZE( _fieldType )	CDatamapFieldSizeDeducer<(_fieldType)>::SIZE
#define FIELD_BITS( _fieldType )	(FIELD_SIZE( _fieldType ) * 8)

DECLARE_FIELD_SIZE( FIELD_FLOAT,		sizeof(float) )
DECLARE_FIELD_SIZE( FIELD_STRING,		sizeof(string_t) )
DECLARE_FIELD_SIZE( FIELD_VECTOR,		sizeof(Vector) )
DECLARE_FIELD_SIZE( FIELD_QUATERNION,	sizeof(Quaternion))
DECLARE_FIELD_SIZE( FIELD_INTEGER,		sizeof(int))
DECLARE_FIELD_SIZE( FIELD_BOOLEAN,		sizeof(bool))
DECLARE_FIELD_SIZE( FIELD_SHORT,		sizeof(short))
DECLARE_FIELD_SIZE( FIELD_CHARACTER,	sizeof(char))
DECLARE_FIELD_SIZE( FIELD_COLOR32,		sizeof(color32))
DECLARE_FIELD_SIZE( FIELD_COLOR24,		sizeof(color24))
DECLARE_FIELD_SIZE( FIELD_CLASSPTR,		sizeof(CGameBaseEntity *))
DECLARE_FIELD_SIZE( FIELD_EHANDLE,		sizeof(CBaseHandle))
DECLARE_FIELD_SIZE( FIELD_EDICT,		sizeof(edict_t *))
DECLARE_FIELD_SIZE( FIELD_POSITION_VECTOR, sizeof(Vector))
DECLARE_FIELD_SIZE( FIELD_TIME,			sizeof(float))
DECLARE_FIELD_SIZE( FIELD_TICK,			sizeof(int))
DECLARE_FIELD_SIZE( FIELD_MODELNAME,	sizeof(string_t))
DECLARE_FIELD_SIZE( FIELD_SOUNDNAME,	sizeof(string_t))
DECLARE_FIELD_SIZE( FIELD_FUNCTION,		sizeof(inputfunc_t))
DECLARE_FIELD_SIZE( FIELD_VMATRIX,		sizeof(VMatrix))
DECLARE_FIELD_SIZE( FIELD_VMATRIX_WORLDSPACE,	sizeof(VMatrix))
DECLARE_FIELD_SIZE( FIELD_MATRIX3X4_WORLDSPACE,	sizeof(matrix3x4_t))
DECLARE_FIELD_SIZE( FIELD_INTERVAL,		sizeof( interval_t) )  // NOTE:  Must match interval.h definition
DECLARE_FIELD_SIZE( FIELD_MODELINDEX,	sizeof(int) )
DECLARE_FIELD_SIZE( FIELD_MATERIALINDEX,	sizeof(int) )
DECLARE_FIELD_SIZE( FIELD_VECTOR2D,		sizeof(Vector2D) )
DECLARE_FIELD_SIZE( FIELD_INTEGER64,	sizeof(int64))
DECLARE_FIELD_SIZE( FIELD_VECTOR4D,		sizeof( Vector4D ) )

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
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL, 0)
#define DEFINE_FIELD_FLAGS(name,fieldtype, flags) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL, 0)
#define DEFINE_FIELD_FLAGS_TOL(name,fieldtype, flags,tolerance) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL, (tolerance))

#define DEFINE_KEYFIELD(name,fieldtype, mapname) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, mapname, 0)

#define DEFINE_FIELD_NAME(localname,netname,fieldtype) \
	typedescription_t((fieldtype), #localname, sizeof(((classNameTypedef *)0)->localname), 1, offsetof(classNameTypedef, localname), 0, netname, 0)
#define DEFINE_FIELD_NAME_TOL(localname,netname,fieldtolerance) \
	typedescription_t((fieldtype), #localname, sizeof(((classNameTypedef *)0)->localname), 1, offsetof(classNameTypedef, localname), 0, netname, (fieldtolerance))

#define DEFINE_KEYFIELD_ARRAYELEM(name,i,fieldtype, mapname) \
	typedescription_t((fieldtype), #name "[" #i "]", sizeof(((classNameTypedef *)0)->name[(i)]), 1, (offsetof(classNameTypedef, name) + (sizeof(((classNameTypedef *)0)->name[(i)]) * (i))), FTYPEDESC_KEY, mapname, 0)

#define DEFINE_AUTO_ARRAY(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), 0, NULL, 0)
#define DEFINE_AUTO_ARRAY_KEYFIELD(name,fieldtype,mapname) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), 0, mapname, 0)

#define DEFINE_ARRAY(name,fieldtype, count) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), 0, NULL, 0)
#define DEFINE_ARRAY_FLAGS(name,fieldtype, count,flags) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), (flags), NULL, 0)
#define DEFINE_ARRAY_FLAGS_TOL(name,fieldtype, count,flags,tolerance) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), (count), offsetof(classNameTypedef, name), (flags), NULL, (tolerance))

#define DEFINE_ENTITY_FIELD(name,fieldtype) \
	typedescription_t((fieldtype), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, #name, 0)

#define DEFINE_CUSTOM_FIELD(name,datafuncs) \
	typedescription_t((datafuncs), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL, 0)
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
	typedescription_t(&(((classNameTypedef *)0)->name.m_DataMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL)
#define DEFINE_PRED_EMBEDDED( name )						\
	typedescription_t(&(((classNameTypedef *)0)->name.m_PredMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL)
#define DEFINE_PRED_EMBEDDED_FLAGS( name, flags )						\
	typedescription_t(&(((classNameTypedef *)0)->name.m_PredMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), (flags), NULL)

#define DEFINE_EMBEDDED_OVERRIDE( name, overridetype )	\
	typedescription_t(&(((overridetype *)0)->name.m_DataMap), #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL)

#define DEFINE_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_DataMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_MAP_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_MapDataDesc), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_PRED_EMBEDDED_PTR( name )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_PredMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR, NULL)
#define DEFINE_PRED_EMBEDDED_PTR_FLAGS( name, flags )					\
	typedescription_t(&(((classNameTypedef *)0)->name->m_PredMap), #name, sizeof(*(((classNameTypedef *)0)->name)), 1, offsetof(classNameTypedef, name), FTYPEDESC_PTR|(flags), NULL)

#define DEFINE_EMBEDDED_ARRAY( name, count )			\
	typedescription_t(&(((classNameTypedef *)0)->name[0].m_DataMap), #name, sizeof(((classNameTypedef *)0)->name[0]), (count), offsetof(classNameTypedef, name), 0, NULL)
#define DEFINE_EMBEDDED_AUTO_ARRAY( name )			\
	typedescription_t(&(((classNameTypedef *)0)->name[0].m_DataMap), #name, sizeof(((classNameTypedef *)0)->name[0]), SIZE_OF_ARRAY(((classNameTypedef *)0)->name), offsetof(classNameTypedef, name), 0, NULL)

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
	typedescription_t(variantFuncs, #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), 0, NULL, 0)
#define DEFINE_KEYVARIANT(name,mapname) \
	typedescription_t(variantFuncs, #name, sizeof(((classNameTypedef *)0)->name), 1, offsetof(classNameTypedef, name), FTYPEDESC_KEY, mapname, 0)

// replaces EXPORT table for portability and non-DLL based systems (xbox)
#define DEFINE_FUNCTION_RAW( function, func_type ) \
	typedescription_t(FIELD_VOID, nameHolder.GenerateName(#function), ((func_type)(&classNameTypedef::function)), FTYPEDESC_FUNCTIONTABLE, NULL)
#define DEFINE_FUNCTION( function ) \
	typedescription_t(FIELD_VOID, nameHolder.GenerateName(#function), ((inputfunc_t)(&classNameTypedef::function)), FTYPEDESC_FUNCTIONTABLE, NULL)


#define FTYPEDESC_KEY				0x0004		// This field can be requested and written to by string name at load time
#define FTYPEDESC_INPUT				0x0008		// This field can be written to by string name at run time, and a function called
#define FTYPEDESC_OUTPUT			0x0010		// This field propogates it's value to all targets whenever it changes
#define FTYPEDESC_FUNCTIONTABLE		0x0020		// This is a table entry for a member function pointer
#define FTYPEDESC_PTR				0x0040		// This field is a pointer, not an embedded object
#define FTYPEDESC_OVERRIDE			0x0080		// The field is an override for one in a base class (only used by prediction system for now)

// Flags used by other systems (e.g., prediction system)
#define FTYPEDESC_INSENDTABLE		0x0100		// This field is present in a network SendTable
#define FTYPEDESC_PRIVATE			0x0200		// The field is local to the client or server only (not referenced by prediction code and not replicated by networking)
#define FTYPEDESC_NOERRORCHECK		0x0400		// The field is part of the prediction typedescription, but doesn't get compared when checking for errors

#define FTYPEDESC_MODELINDEX		0x0800		// The field is a model index (used for debugging output)

#define FTYPEDESC_INDEX				0x1000		// The field is an index into file data, used for byteswapping. 

// These flags apply to C_BasePlayer derived objects only
#define FTYPEDESC_VIEW_OTHER_PLAYER		0x2000		// By default you can only view fields on the local player (yourself), 
													//   but if this is set, then we allow you to see fields on other players
#define FTYPEDESC_VIEW_OWN_TEAM			0x4000		// Only show this data if the player is on the same team as the local player
#define FTYPEDESC_VIEW_NEVER			0x8000		// Never show this field to anyone, even the local player (unusual)

#define TD_MSECTOLERANCE		0.001f		// This is a FIELD_FLOAT and should only be checked to be within 0.001 of the networked info

struct datamap_t;
struct typedescription_t;

struct FieldInfo_t
{
	void *			   pField;

	// Note that it is legal for the following two fields to be NULL,
	// though it may be disallowed by implementors of ISaveRestoreOps
	void *			   pOwner;
	typedescription_t *pTypeDesc;
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

	bool IsEmpty( void *pField)							{ FieldInfo_t fieldInfo = { pField, NULL, NULL }; return IsEmpty( fieldInfo ); }
	void MakeEmpty( void *pField)						{ FieldInfo_t fieldInfo = { pField, NULL, NULL }; MakeEmpty( fieldInfo ); }
	bool Parse( void *pField, char const *pszValue )	{ FieldInfo_t fieldInfo = { pField, NULL, NULL }; return Parse( fieldInfo, pszValue ); }
};

enum
{
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,

	// Must be last
	TD_OFFSET_COUNT,
};

struct typedescription_t
{
	typedescription_t();
	typedescription_t(const typedescription_t &) = delete;
	typedescription_t &operator=(const typedescription_t &) = delete;
	typedescription_t(typedescription_t &&) = default;
	typedescription_t &operator=(typedescription_t &&) = delete;

	typedescription_t(fieldtype_t type, const char *name, int bytes, int count, int offset, int flags_, const char *fgdname, float tol);
	typedescription_t(fieldtype_t type, const char *name, inputfunc_t func, int flags_, const char *fgdname);
	typedescription_t(ICustomFieldOps *funcs, const char *name, int bytes, int count, int offset, int flags_, const char *fgdname, float tol);
	typedescription_t(datamap_t *embed, const char *name, int bytes, int count, int offset, int flags_, const char *fgdname);

	fieldtype_t			fieldType;
	const char			*fieldName;
	int					fieldOffset[ TD_OFFSET_COUNT ]; // 0 == normal, 1 == packed offset
	unsigned short		fieldSize;
	short				flags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char			*externalName;	
	// pointer to the function set for save/restoring of custom data types
	ICustomFieldOps		*pFieldOps; 
	// for associating function with string names
	inputfunc_t			inputFunc; 
	// For embedding additional datatables inside this one
	datamap_t			*td;

	// Stores the actual member variable size in bytes
	int					fieldSizeInBytes;

	// FTYPEDESC_OVERRIDE point to first baseclass instance if chains_validated has occurred
	struct typedescription_t *override_field;

	// Used to track exclusion of baseclass fields
	int					override_count;
  
	// Tolerance for field errors for float fields
	float				fieldTolerance;

	const char *m_pDefValue;
	const char *m_pGuiName;
	const char *m_pDescription;
};

class CDefCustomFieldOps : public ICustomFieldOps
{
public:
	// save data type interface
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo ) { return false; }
	virtual void MakeEmpty( const FieldInfo_t &fieldInfo ) {}
	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue ) { return false; }
};


//-----------------------------------------------------------------------------
// Used by ops that deal with pointers
//-----------------------------------------------------------------------------
class CClassPtrFieldOps : public CDefCustomFieldOps
{
public:
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		void **ppClassPtr = (void **)fieldInfo.pField;
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
		memset( fieldInfo.pField, 0, fieldInfo.pTypeDesc->fieldSize * sizeof( void * ) );
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
