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
private:
	union
	{
		bool bVal;
		string_t iszVal;
		interval_t interVal;
		char *szVal;
		modelindex_t mdlVal;
		int iVal;
		unsigned int uVal;
		int64 iVal64;
		uint64 uVal64;
		short hVal;
		unsigned short uhVal;
		char cVal;
		signed char scVal;
		unsigned char ucVal;
		float flVal;
		Vector vecVal;
		Vector2D vec2dVal;
		Vector4D vec4dval;
		QAngle angVal;
		Quaternion quatVal;
		VMatrix vmatVal;
		matrix3x4_t matVal;
		color24 rgbVal;
		color32 rgbaVal;
		ColorRGBExp32 rgbeVal;
		EHANDLE ehVal;
		edict_t *edpVal;
		CSharedBaseEntity *epVal;
		inputfunc_t fpVal;
	};

	fieldtype_t fieldType_;

	void dealloc();

	void assign_plain(const variant_t &other);

public:
	// constructor
	variant_t()
		: fieldType_(FIELD_VOID),
			uVal64(0)
	{
	}

	variant_t(variant_t &&other);
	variant_t &operator=(variant_t &&other);

	variant_t(const variant_t &other);
	variant_t &operator=(const variant_t &other);

	~variant_t();

	bool operator==(const variant_t &other) const = delete;
	bool operator!=(const variant_t &other) const = delete;

	bool operator>(const variant_t &other) const = delete;
	bool operator>=(const variant_t &other) const = delete;
	bool operator<(const variant_t &other) const = delete;
	bool operator<=(const variant_t &other) const = delete;

	bool Bool( void ) const;
	const char *String( void ) const;
	string_t StringID( void ) const;
	char Char( void ) const;
	signed char SChar( void ) const;
	unsigned char UChar( void ) const;
	int Int( void ) const;
	unsigned int UInt( void ) const;
	short Short( void ) const;
	unsigned short UShort( void ) const;
	int64 Int64( void ) const;
	uint64 UInt64( void ) const;
	float Float( void ) const;
	CSharedBaseEntity *EntityP(void) const;
	const EHANDLE &EntityH(void) const;
	color24 Color24(void) const;
	color32 Color32(void) const;
	ColorRGBExp32 Color32E(void) const;
	Vector Vector3D() const;
	// Gets angles from a vector
	QAngle Angle3D() const;

	fieldtype_t baseFieldType( void ) const
	{ return (fieldtype_t)((unsigned short)fieldType_ & (unsigned short)FIELD_TYPE_MASK); }
	fieldtype_t rawFieldType( void ) const
	{ return fieldType_; }

	void Parse(const char *value);

	void SetVoid()
	{ dealloc(); uVal64 = 0; fieldType_ = FIELD_VOID; }

	void SetBool( bool b )
	{ dealloc(); bVal = b; fieldType_ = FIELD_BOOLEAN; }
	void SetStringT( string_t str )
	{ dealloc(); iszVal = str; fieldType_ = FIELD_POOLED_STRING; }
	void SetModelnameT( string_t str )
	{ dealloc(); iszVal = str; fieldType_ = FIELD_POOLED_MODELNAME; }
	void SetSoundnameT( string_t str )
	{ dealloc(); iszVal = str; fieldType_ = FIELD_POOLED_SOUNDNAME; }
	void SetSpritenameT( string_t str )
	{ dealloc(); iszVal = str; fieldType_ = FIELD_POOLED_SPRITENAME; }
	void SetChar( char val )
	{ dealloc(); cVal = val; fieldType_ = FIELD_CHARACTER; }
	void SetSChar( signed char val )
	{ dealloc(); scVal = val; fieldType_ = FIELD_SCHARACTER; }
	void SetUChar( unsigned char val )
	{ dealloc(); ucVal = val; fieldType_ = FIELD_UCHARACTER; }
	void SetInt( int val )
	{ dealloc(); iVal = val; fieldType_ = FIELD_INTEGER; }
	void SetUInt( unsigned int val )
	{ dealloc(); iVal = val; fieldType_ = FIELD_UINTEGER; }
	void SetShort( short val )
	{ dealloc(); hVal = val; fieldType_ = FIELD_SHORT; }
	void SetUShort( unsigned short val )
	{ dealloc(); uhVal = val; fieldType_ = FIELD_USHORT; }
	void SetInt64( int64 val )
	{ dealloc(); iVal64 = val; fieldType_ = FIELD_INTEGER64; }
	void SetUInt64( int64 val )
	{ dealloc(); iVal64 = val; fieldType_ = FIELD_UINTEGER64; }
	void SetFloat( float val )
	{ dealloc(); flVal = val; fieldType_ = FIELD_FLOAT; }
	void SetEntityH( EHANDLE val )
	{ dealloc(); ehVal = val; fieldType_ = FIELD_EHANDLE; }
	void SetEntityH( CSharedBaseEntity *val );
	void SetVector3D( const Vector &val )
	{ dealloc(); vecVal = val; fieldType_ = FIELD_VECTOR; }
	void SetPositionVector3D( const Vector &val )
	{ dealloc(); vecVal = val; fieldType_ = FIELD_VECTOR_WORLDSPACE; }
	// Passes in angles as a vector
	void SetAngle3D( const QAngle &val )
	{ dealloc(); angVal = val; fieldType_ = FIELD_QANGLE; }

	void SetModelindex( modelindex_t val )
	{ dealloc(); mdlVal = val; fieldType_ = FIELD_MODELINDEX; }
	void SetBrushModelindex( modelindex_t val )
	{ dealloc(); mdlVal = val; fieldType_ = FIELD_BRUSH_MODELIDNEX; }
	void SetStudioModelindex( modelindex_t val )
	{ dealloc(); mdlVal = val; fieldType_ = FIELD_STUDIO_MODELIDNEX; }
	void SetSpriteModelindex( modelindex_t val )
	{ dealloc(); mdlVal = val; fieldType_ = FIELD_SPRITE_MODELINDEX; }

	void SetColor32( color32 val )
	{ dealloc(); rgbaVal = val; fieldType_ = FIELD_COLOR32; }
	void SetColor32( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
	{ dealloc(); rgbaVal.SetColor( r, g, b, a ); fieldType_ = FIELD_COLOR32; }

	void SetColor32E( ColorRGBExp32 val )
	{ dealloc(); rgbeVal = val; fieldType_ = FIELD_COLOR32E; }
	void SetColor32E( unsigned char r, unsigned char g, unsigned char b, signed char e )
	{ dealloc(); rgbeVal.SetColor( r, g, b, e ); fieldType_ = FIELD_COLOR32E; }

	void SetColor24( color32 val )
	{ dealloc(); rgbVal = val; fieldType_ = FIELD_COLOR24; }
	void SetColor24( unsigned char r, unsigned char g, unsigned char b )
	{ dealloc(); rgbVal.SetColor( r, g, b ); fieldType_ = FIELD_COLOR24; }

	// Hands over the value + the field type.
	// ex: "Otis (String)", "3 (Integer)", or "npc_combine_s (Entity)"
	const char *GetDebug() const;
};

DECLARE_FIELD_INFO( FIELD_VARIANT,		 variant_t ) 

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

#endif // VARIANT_T_H
