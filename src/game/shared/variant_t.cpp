//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "variant_t.h"
#include "entitylist_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

variant_t &variant_t::operator=(const variant_t &rhs)
{
	switch(rhs.fieldType) {
	case FIELD_VOID:
		break;
	case FIELD_FLOAT:
		flVal = rhs.flVal;
		break;
	case FIELD_STRING:
		iszVal = rhs.iszVal;
		break;
	case FIELD_INTEGER:
		iVal = rhs.iVal;
		break;
	case FIELD_INTEGER64:
		iVal64 = rhs.iVal64;
		break;
	case FIELD_BOOLEAN:
		bVal = rhs.bVal;
		break;
	case FIELD_EHANDLE:
		ehVal = rhs.ehVal;
		break;
	case FIELD_CLASSPTR:
		epVal = rhs.epVal;
		break;
	case FIELD_POSITION_VECTOR:
	case FIELD_VECTOR:
		for(int i = 0; i < 3; ++i)
			vecVal[i] = rhs.vecVal[i];
		break;
	case FIELD_CHARACTER:
		cVal = rhs.cVal;
		break;
	case FIELD_SHORT:
		hVal = rhs.hVal;
		break;
	case FIELD_COLOR32:
		rgbaVal = rhs.rgbaVal;
		break;
	default:
		break;
	}
	fieldType = rhs.fieldType;
	return *this;
}

void variant_t::SetEntity( CSharedBaseEntity *val ) 
{ 
	ehVal = val;
	fieldType = FIELD_EHANDLE; 
}

const char *variant_t::GetDebug()
{
	/*
	case FIELD_BOOLEAN:		*((bool *)data) = bVal != 0;		break;
	case FIELD_CHARACTER:	*((char *)data) = iVal;				break;
	case FIELD_SHORT:		*((short *)data) = iVal;			break;
	case FIELD_INTEGER:		*((int *)data) = iVal;				break;
	case FIELD_STRING:		*((string_t *)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float *)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32 *)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float *)data)[0] = vecVal[0];
		((float *)data)[1] = vecVal[1];
		((float *)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE *)data) = eVal;			break;
	case FIELD_CLASSPTR:	*((CBaseEntity **)data) = eVal;		break;
	*/

	const char *fieldtype = "unknown";
	switch (FieldType())
	{
	case FIELD_VOID:			fieldtype = "Void"; break;
	case FIELD_FLOAT:			fieldtype = "Float"; break;
	case FIELD_STRING:			fieldtype = "String"; break;
	case FIELD_INTEGER:			fieldtype = "Integer"; break;
	case FIELD_INTEGER64:			fieldtype = "Integer64"; break;
	case FIELD_BOOLEAN:			fieldtype = "Boolean"; break;
	case FIELD_EHANDLE:			fieldtype = "Entity"; break;
	case FIELD_CLASSPTR:		fieldtype = "EntityPtr"; break;
	case FIELD_POSITION_VECTOR:
	case FIELD_VECTOR:			fieldtype = "Vector"; break;
	case FIELD_CHARACTER:		fieldtype = "Character"; break;
	case FIELD_SHORT:			fieldtype = "Short"; break;
	case FIELD_COLOR32:			fieldtype = "Color32"; break;
	default:					fieldtype = UTIL_VarArgs("unknown: %i", FieldType());
	}
	return UTIL_VarArgs("%s (%s)", String(), fieldtype);
}

// cmp1 = val1 float
// cmp2 = val2 float
#define VariantToFloat(val1, val2, lenallowed) \
	float cmp1 = val1.Float() ? val1.Float() : val1.Int(); \
	float cmp2 = val2.Float() ? val2.Float() : val2.Int(); \
	if (lenallowed && val2.FieldType() == FIELD_STRING) \
		cmp2 = strlen(val2.String());

// Integer parsing has been deactivated for consistency's sake. They now become floats only.
#define INTEGER_PARSING_DEACTIVATED 1

// "intchar" is the result of me not knowing where to find a version of isdigit that applies to negative numbers and floats.
#define intchar(c) (c >= '-' && c <= '9')

// Attempts to determine the field type from whatever is in the string and creates a variant_t with the converted value and resulting field type.
// Right now, Int/Float, String, and Vector are the only fields likely to be used by entities in variant_t parsing, so they're the only ones supported.
// Expand to other fields when necessary.
variant_t Variant_Parse(const char *szValue)
{
#ifdef INTEGER_PARSING_DEACTIVATED
	bool isint = true;
	bool isvector = false;
	for (size_t i = 0; i < strlen(szValue); i++)
	{
		if (!intchar(szValue[i]))
		{
			isint = false;

			if (szValue[i] == ' ')
				isvector = true;
			else
				isvector = false;
		}
	}

	variant_t var;

	if (isint)
		var.SetFloat(atof(szValue));
	else if (isvector)
	{
		var.SetString(MAKE_STRING(szValue));
		var.Convert(FIELD_VECTOR);
	}
	else
		var.SetString(MAKE_STRING(szValue));
#else
	bool isint = true;
	bool isfloat = false;
	for (size_t i = 0; i < strlen(szValue); i++)
	{
		if (szValue[i] == '.')
			isfloat = true;
		else if (!intchar(szValue[i]))
			isint = false;
	}

	variant_t var = variant_t();

	if (isint)
	{
		if (isfloat)
			var.SetFloat(atof(szValue));
		else
			var.SetInt(atoi(szValue));
	}
	else
		var.SetString(MAKE_STRING(szValue));
#endif

	return var;
}

// Passes strings to Variant_Parse, uses the other input data for finding procedural entities.
variant_t Variant_ParseInput(const inputdata_t &inputdata)
{
	if (inputdata.value.FieldType() == FIELD_STRING)
	{
		if (inputdata.value.String()[0] == '!')
		{
			variant_t var = variant_t();
			var.SetEntity(g_pEntityList->FindEntityProcedural(inputdata.value.String(), inputdata.pCaller, inputdata.pActivator, inputdata.pCaller));
			if (var.Entity())
				return var;
		}
	}

	return Variant_Parse(inputdata.value.String());
}

// Passes string variants to Variant_Parse
variant_t Variant_ParseString(const variant_t &value)
{
	if (value.FieldType() != FIELD_STRING)
		return value;

	return Variant_Parse(value.String());
}

bool Variant_Equal(const variant_t &val1, const variant_t &val2, bool bLenAllowed)
{
	//if (!val2.Convert(val1.FieldType()))
	//	return false;

	// Add more fields if they become necessary
	switch (val1.FieldType())
	{
	case FIELD_INTEGER:
	case FIELD_INTEGER64:
	case FIELD_FLOAT:
	{
		VariantToFloat(val1, val2, bLenAllowed);
		return cmp1 == cmp2;
	}
	case FIELD_BOOLEAN:		return val1.Bool() == val2.Bool();
	case FIELD_EHANDLE:		return val1.Entity() == val2.Entity();
	case FIELD_VECTOR:
	{
		Vector vec1; val1.Vector3D(vec1);
		Vector vec2; val2.Vector3D(vec2);
		return vec1 == vec2;
	}
	// logic_compare allows wildcards on either string
	default:				return Matcher_NamesMatch_MutualWildcard(val1.String(), val2.String());
	}

	return false;
}

// val1 > val2
bool Variant_Greater(const variant_t &val1, const variant_t &val2, bool bLenAllowed)
{
	//if (!val2.Convert(val1.FieldType()))
	//	return false;

	// Add more fields if they become necessary
	switch (val1.FieldType())
	{
	case FIELD_INTEGER:
	case FIELD_INTEGER64:
	case FIELD_FLOAT:
	{
		VariantToFloat(val1, val2, bLenAllowed);
		return cmp1 > cmp2;
	}
	case FIELD_BOOLEAN:		return val1.Bool() && !val2.Bool();
	case FIELD_VECTOR:
	{
		Vector vec1; val1.Vector3D(vec1);
		Vector vec2; val2.Vector3D(vec2);
		return (vec1.x > vec2.x) && (vec1.y > vec2.y) && (vec1.z > vec2.z);
	}
	default:				return strlen(val1.String()) > strlen(val2.String());
	}

	return false;
}

// val1 >= val2
bool Variant_GreaterOrEqual(const variant_t &val1, const variant_t &val2, bool bLenAllowed)
{
	//if (!val2.Convert(val1.FieldType()))
	//	return false;

	// Add more fields if they become necessary
	switch (val1.FieldType())
	{
	case FIELD_INTEGER:
	case FIELD_INTEGER64:
	case FIELD_FLOAT:
	{
		VariantToFloat(val1, val2, bLenAllowed);
		return cmp1 >= cmp2;
	}
	case FIELD_BOOLEAN:		return val1.Bool() >= val2.Bool();
	case FIELD_VECTOR:
	{
		Vector vec1; val1.Vector3D(vec1);
		Vector vec2; val2.Vector3D(vec2);
		return (vec1.x >= vec2.x) && (vec1.y >= vec2.y) && (vec1.z >= vec2.z);
	}
	default:				return strlen(val1.String()) >= strlen(val2.String());
	}

	return false;
}

////////////////////////// variant_t implementation //////////////////////////

// BUGBUG: Add support for function pointer save/restore to variants
// BUGBUG: Must pass datamap_t to read/write fields 
void variant_t::Set( fieldtype_t ftype, void *data )
{
	fieldType = ftype;

	switch ( ftype )
	{
	case FIELD_BOOLEAN:		bVal = *((bool *)data);				break;
	case FIELD_CHARACTER:	iVal = *((char *)data);				break;
	case FIELD_SHORT:		iVal = *((short *)data);			break;
	case FIELD_INTEGER:		iVal = *((int *)data);				break;
	case FIELD_INTEGER64:		iVal = *((int64 *)data);				break;
	case FIELD_STRING:		iszVal = *((string_t *)data);		break;
	case FIELD_FLOAT:		flVal = *((float *)data);			break;
	case FIELD_COLOR32:		rgbaVal = *((color32 *)data);		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		vecVal = *((Vector *)data);
		break;
	}

	// There's this output class called COutputVariant which could output any data type, like a FIELD_INPUT input function.
	// Well...nobody added support for it. It was there, but it wasn't functional.
	// Mapbase adds support for it so you could variant your outputs as you please.
	case FIELD_INPUT:
	{
		variant_t *variant = (variant_t*)data;

		// Pretty much just copying over its stored value.
		fieldType = variant->FieldType();
		variant->SetOther(data);

		Set(fieldType, data);
		break;
	}

	case FIELD_EHANDLE:		ehVal = *((EHANDLE *)data);			break;
	case FIELD_CLASSPTR:	ehVal = *((CSharedBaseEntity **)data);		break;
	case FIELD_VOID:		
	default:
		iVal = 0; fieldType = FIELD_VOID;	
		break;
	}
}

// This way we don't have to use string comparisons when reading failed conversions
static const char *g_szNoConversion = "No conversion to string";

//-----------------------------------------------------------------------------
// Purpose: Copies the value in the variant into a block of memory
// Input  : *data - the block to write into
//-----------------------------------------------------------------------------
void variant_t::SetOther( void *data )
{
	switch ( fieldType )
	{
	case FIELD_BOOLEAN:		*((bool *)data) = bVal != 0;		break;
	case FIELD_CHARACTER:	*((char *)data) = iVal;				break;
	case FIELD_SHORT:		*((short *)data) = iVal;			break;
	case FIELD_INTEGER:		*((int *)data) = iVal;				break;
	case FIELD_INTEGER64:		*((int64 *)data) = iVal;				break;
	case FIELD_STRING:		*((string_t *)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float *)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32 *)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		*((Vector *)data) = vecVal;
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE *)data) = ehVal;			break;
	case FIELD_CLASSPTR:	*((CSharedBaseEntity **)data) = ehVal;		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Converts the variant to a new type. This function defines which I/O
//			types can be automatically converted between. Connections that require
//			an unsupported conversion will cause an error message at runtime.
// Input  : newType - the type to convert to
// Output : Returns true on success, false if the conversion is not legal
//-----------------------------------------------------------------------------
bool variant_t::Convert( fieldtype_t newType )
{
	if ( newType == fieldType )
	{
		return true;
	}

	//
	// Converting to a null value is easy.
	//
	if ( newType == FIELD_VOID )
	{
		Set( FIELD_VOID, NULL );
		return true;
	}

	if (newType == FIELD_STRING)
	{
		// I got a conversion error when I tried to convert int to string. I'm actually quite baffled.
		// Was that case really not handled before? Did I do something that overrode something that already did this?
		const char *szString = ToString();

		// g_szNoConversion is returned in ToString() when we can't convert to a string,
		// so this is safe and it lets us get away with a pointer comparison.
		if (szString != g_szNoConversion)
		{
			SetString(AllocPooledString(szString));
			return true;
		}
		return false;
	}

	//
	// FIELD_INPUT accepts the variant type directly.
	//
	if ( newType == FIELD_INPUT )
	{
		return true;
	}

	switch ( fieldType )
	{
		case FIELD_INTEGER:
		{
			switch ( newType )
			{
				case FIELD_FLOAT:
				{
					SetFloat( (float) iVal );
					return true;
				}

				case FIELD_BOOLEAN:
				{
					SetBool( iVal != 0 );
					return true;
				}
			}
			break;
		}

		case FIELD_INTEGER64:
		{
			switch ( newType )
			{
				case FIELD_FLOAT:
				{
					SetFloat( (float) iVal64 );
					return true;
				}

				case FIELD_BOOLEAN:
				{
					SetBool( iVal64 != 0 );
					return true;
				}
			}
			break;
		}

		case FIELD_FLOAT:
		{
			switch ( newType )
			{
				case FIELD_INTEGER:
				{
					SetInt( (int) flVal );
					return true;
				}

				case FIELD_INTEGER64:
				{
					SetInt64( (int64) flVal );
					return true;
				}

				case FIELD_BOOLEAN:
				{
					SetBool( flVal != 0 );
					return true;
				}
			}
			break;
		}

		//
		// Everyone must convert from FIELD_STRING if possible, since
		// parameter overrides are always passed as strings.
		//
		case FIELD_STRING:
		{
			switch ( newType )
			{
				case FIELD_INTEGER:
				{
					if (iszVal != NULL_STRING)
					{
						SetInt(atoi(STRING(iszVal)));
					}
					else
					{
						SetInt(0);
					}
					return true;
				}

				case FIELD_INTEGER64:
				{
					if (iszVal != NULL_STRING)
					{
						SetInt64(strtoull(STRING(iszVal), NULL, 10));
					}
					else
					{
						SetInt64(0);
					}
					return true;
				}

				case FIELD_FLOAT:
				{
					if (iszVal != NULL_STRING)
					{
						SetFloat(atof(STRING(iszVal)));
					}
					else
					{
						SetFloat(0);
					}
					return true;
				}

				case FIELD_BOOLEAN:
				{
					if (iszVal != NULL_STRING)
					{
						SetBool( atoi(STRING(iszVal)) != 0 );
					}
					else
					{
						SetBool(false);
					}
					return true;
				}

				case FIELD_VECTOR:
				{
					Vector tmpVec = vec3_origin;
					if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
					{
						// Try sucking out 3 floats with no []s
						sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
					}
					SetVector3D( tmpVec );
					return true;
				}

				case FIELD_COLOR32:
				{
					int nRed = 0;
					int nGreen = 0;
					int nBlue = 0;
					int nAlpha = 255;

					sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
					SetColor32( nRed, nGreen, nBlue, nAlpha );
					return true;
				}

				case FIELD_EHANDLE:
				{
					// convert the string to an entity by locating it by classname
					CSharedBaseEntity *ent = NULL;
					if ( iszVal != NULL_STRING )
					{
						// FIXME: do we need to pass an activator in here?
						// We search by both entity name and class name now.
						// We also have an entirely new version of Convert specifically for !activators on FIELD_EHANDLE.
						ent = g_pEntityList->FindEntityGeneric( NULL, STRING(iszVal) );
					}
					SetEntity( ent );
					return true;
				}
			}
		
			break;
		}

		case FIELD_VOID:
		{
			// Many fields already turn into some equivalent of "NULL" when given a null string_t.
			// This takes advantage of that and allows FIELD_VOID to be converted to more than just empty strings.
			SetString(NULL_STRING);
			return Convert(newType);
		}
	}

	// invalid conversion
	return false;
}

//-----------------------------------------------------------------------------
// Only for when something like !activator needs to become a FIELD_EHANDLE, or when that's a possibility.
//-----------------------------------------------------------------------------
bool variant_t::Convert( fieldtype_t newType, CSharedBaseEntity *pSelf, CSharedBaseEntity *pActivator, CSharedBaseEntity *pCaller )
{
	// Support for turning !activator, !caller, and !self into a FIELD_EHANDLE.
	// Extremely necessary.
	if (newType == FIELD_EHANDLE)
	{
		if (newType == fieldType)
			return true;

		CSharedBaseEntity *ent = NULL;
		if (iszVal != NULL_STRING)
		{
			ent = g_pEntityList->FindEntityGeneric(NULL, STRING(iszVal), pSelf, pActivator, pCaller);
		}
		SetEntity(ent);
		return true;
	}

#if 0 // This was scrapped almost immediately. See the Trello card for details.
	// Serves as a way of converting the name of the !activator, !caller, or !self into a string
	// without passing the text "!activator" and stuff.
	else if (fieldType == FIELD_STRING && STRING(iszVal)[0] == '&')
	{
		const char *val = STRING(iszVal) + 1;

		#define GetRealName(string, ent) if (FStrEq(val, string)) { if (ent) {SetString(ent->GetEntityName());} return true; }

		GetRealName("!activator", pActivator)
		else GetRealName("!caller", pCaller)
		else GetRealName("!self", pSelf)
	}
#endif

	return Convert(newType);
}

//-----------------------------------------------------------------------------
// Purpose: All types must be able to display as strings for debugging purposes.
// Output : Returns a pointer to the string that represents this value.
//
//			NOTE: The returned pointer should not be stored by the caller as
//				  subsequent calls to this function will overwrite the contents
//				  of the buffer!
//-----------------------------------------------------------------------------
const char *variant_t::ToString( void ) const
{
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
		{
			return(STRING(iszVal));
		}

	case FIELD_BOOLEAN:
		{
			if (bVal == 0)
			{
				Q_strncpy(szBuf, "false",sizeof(szBuf));
			}
			else
			{
				Q_strncpy(szBuf, "true",sizeof(szBuf));
			}
			return(szBuf);
		}

	case FIELD_INTEGER:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
			return(szBuf);
		}

	case FIELD_INTEGER64:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%lli", iVal64 );
			return(szBuf);
		}

	case FIELD_FLOAT:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
			return(szBuf);
		}

	case FIELD_COLOR32:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", rgbaVal.r(), (int)rgbaVal.g(), (int)rgbaVal.b(), (int)rgbaVal.a() );
			return(szBuf);
		}

	case FIELD_VECTOR:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
			return(szBuf);
		}

	case FIELD_VOID:
		{
			szBuf[0] = '\0';
			return(szBuf);
		}

	case FIELD_EHANDLE:
		{
			// This is a really bad idea.
			const char *pszName = (Entity()) ? Entity()->GetDebugName() : "<<null entity>>";
			Q_strncpy( szBuf, pszName, 512 );
			return (szBuf);
		}
	}

	return g_szNoConversion;
}

class CVariantFieldOps : public CDefCustomFieldOps
{
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		// check all the elements of the array (usually only 1)
		variant_t *var = (variant_t*)fieldInfo.pField;
		for ( int i = 0; i < fieldInfo.pTypeDesc->fieldSize; i++, var++ )
		{
			if ( var->FieldType() != FIELD_VOID )
				return 0;
		}

		// variant has no data
		return 1;
	}

	virtual void MakeEmpty( const FieldInfo_t &fieldInfo )
	{
		// Don't no how to. This is okay, since objects of this type
		// are always born clean before restore, and not reused
	}

	// Parses a keyvalue string into a variant_t.
	// We could just turn it into a string since variant_t can convert it later, but this keyvalue is probably a variant_t for a reason,
	// meaning it might use strings and numbers completely differently without converting them.
	// As a result, we try to read it to figure out what type it is.
	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue )
	{
		variant_t *var = (variant_t*)fieldInfo.pField;

		*var = Variant_Parse(szValue);

		return true;
	}
};

CVariantFieldOps g_VariantFieldOps;
ICustomFieldOps *variantFuncs = &g_VariantFieldOps;
