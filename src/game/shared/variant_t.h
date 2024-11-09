//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VARIANT_T_H
#define VARIANT_T_H
#pragma once

#include "ehandle.h"
#include "mathlib/vmatrix.h"
#include "string_t.h"
#include "datamap.h"

DECLARE_LOGGING_CHANNEL( LOG_ENTITYIO );

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CSharedBaseEntity;
#else
class C_BaseEntity;
typedef C_BaseEntity CSharedBaseEntity;
#endif

//
// A variant class for passing data in entity input/output connections.
//
class variant_t
{
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		int64 iVal64;
		short hVal;
		char cVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
		EHANDLE eVal;
		CSharedBaseEntity *pVal;
	};

	fieldtype_t fieldType;

public:

	// constructor
	variant_t() : fieldType(FIELD_VOID), iVal(0) {}

	variant_t(const variant_t &rhs)
	{ operator=(rhs); }
	variant_t &operator=(const variant_t &rhs);

	inline bool Bool( void ) const						{ return( fieldType == FIELD_BOOLEAN ) ? bVal : false; }
	inline const char *String( void ) const				{ return( fieldType == FIELD_STRING ) ? STRING(iszVal) : ToString(); }
	inline string_t StringID( void ) const				{ return( fieldType == FIELD_STRING ) ? iszVal : NULL_STRING; }
	inline int Int( void ) const						{ return( fieldType == FIELD_INTEGER ) ? iVal : 0; }
	inline int64 Int64( void ) const					{ return( fieldType == FIELD_INTEGER64 ) ? iVal64 : 0; }
	inline float Float( void ) const					{ return( fieldType == FIELD_FLOAT ) ? flVal : 0; }
	inline const EHANDLE &Entity(void) const;
	inline color32 Color32(void) const					{ return rgbaVal; }
	inline void Vector3D(Vector &vec) const;
	// Gets angles from a vector
	inline void Angle3D(QAngle &ang) const;

	fieldtype_t FieldType( void ) const { return fieldType; }

	void SetBool( bool b ) { bVal = b; fieldType = FIELD_BOOLEAN; }
	void SetString( string_t str ) { iszVal = str, fieldType = FIELD_STRING; }
	void SetInt( int val ) { iVal = val, fieldType = FIELD_INTEGER; }
	void SetInt64( int64 val ) { iVal64 = val, fieldType = FIELD_INTEGER64; }
	void SetFloat( float val ) { flVal = val, fieldType = FIELD_FLOAT; }
	void SetEntity( CSharedBaseEntity *val );
	void SetVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_VECTOR; }
	void SetPositionVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_POSITION_VECTOR; }
	// Passes in angles as a vector
	void SetAngle3D( const QAngle &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_VECTOR; }

	void SetColor32( color32 val ) { rgbaVal = val; fieldType = FIELD_COLOR32; }
	void SetColor32( int r, int g, int b, int a ) { rgbaVal.r = r; rgbaVal.g = g; rgbaVal.b = b; rgbaVal.a = a; fieldType = FIELD_COLOR32; }
	void Set( fieldtype_t ftype, void *data );
	void SetOther( void *data );
	bool Convert( fieldtype_t newType );
	// Special conversion specifically for FIELD_EHANDLE with !activator, etc.
	bool Convert( fieldtype_t newType, CSharedBaseEntity *pSelf, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller );
	// Hands over the value + the field type.
	// ex: "Otis (String)", "3 (Integer)", or "npc_combine_s (Entity)"
	const char *GetDebug();

	static typedescription_t m_SaveBool[];
	static typedescription_t m_SaveInt[];
	static typedescription_t m_SaveInt64[];
	static typedescription_t m_SaveFloat[];
	static typedescription_t m_SaveEHandle[];
	static typedescription_t m_SaveString[];
	static typedescription_t m_SaveColor[];
	static typedescription_t m_SaveVector[];
	static typedescription_t m_SavePositionVector[];
	static typedescription_t m_SaveVMatrix[];
	static typedescription_t m_SaveVMatrixWorldspace[];
	static typedescription_t m_SaveMatrix3x4Worldspace[];

protected:

	//
	// Returns a string representation of the value without modifying the variant.
	//
	const char *ToString( void ) const;

	friend class CVariantSaveDataOps;
};

//
// Structure passed to input handlers.
//
struct inputdata_t
{
	CSharedBaseEntity *pActivator;		// The entity that initially caused this chain of output events.
	CSharedBaseEntity *pCaller;			// The entity that fired this particular output.
	variant_t value;				// The data parameter for this output.
	int nOutputID;					// The unique ID of the output that was fired.
};

//-----------------------------------------------------------------------------
// Purpose: Returns this variant as a vector.
//-----------------------------------------------------------------------------
inline void variant_t::Vector3D(Vector &vec) const
{
	if (( fieldType == FIELD_VECTOR ) || ( fieldType == FIELD_POSITION_VECTOR ))
	{
		vec[0] =  vecVal[0];
		vec[1] =  vecVal[1];
		vec[2] =  vecVal[2];
	}
	else
	{
		vec = vec3_origin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns this variant as angles.
//-----------------------------------------------------------------------------
inline void variant_t::Angle3D(QAngle &ang) const
{
	if (( fieldType == FIELD_VECTOR ) || ( fieldType == FIELD_POSITION_VECTOR ))
	{
		ang[0] =  vecVal[0];
		ang[1] =  vecVal[1];
		ang[2] =  vecVal[2];
	}
	else
	{
		ang = vec3_angle;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns this variant as an EHANDLE.
//-----------------------------------------------------------------------------
inline const EHANDLE &variant_t::Entity(void) const
{
	if ( fieldType == FIELD_EHANDLE )
		return eVal;

	return NULL_EHANDLE;
}

// Most of these are defined in variant_t.cpp.

// Creates a variant_t from the given string.
// It could return as a String or a Float.
variant_t Variant_Parse(const char *szValue);

// Intended to convert FIELD_INPUT I/O parameters to other values, like integers, floats, or even entities.
// This only changes FIELD_STRING variants. Other data like FIELD_EHANDLE or FIELD_INTEGER are not affected.
variant_t Variant_ParseInput(const inputdata_t &inputdata);

// A simpler version of Variant_ParseInput that does not allow FIELD_EHANDLE.
variant_t Variant_ParseString(variant_t value);

// val1 == val2
bool Variant_Equal(variant_t val1, variant_t val2, bool bLenAllowed = true);

// val1 > val2
bool Variant_Greater(variant_t val1, variant_t val2, bool bLenAllowed = true);

// val1 >= val2
bool Variant_GreaterOrEqual(variant_t val1, variant_t val2, bool bLenAllowed = true);

#endif // VARIANT_T_H
